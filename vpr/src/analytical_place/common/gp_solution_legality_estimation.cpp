/**
 * @file
 * @author  Alex Singer
 * @date    October 2026
 * @brief   Implementation of methods for estimating the legality of a global
 *          placement solution.
 */

#include "gp_solution_legality_estimation.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "ap_netlist.h"
#include "atom_netlist.h"
#include "device_grid.h"
#include "echo_files.h"
#include "logic_types.h"
#include "pack_patterns.h"
#include "partial_placement.h"
#include "partition_region.h"
#include "physical_types.h"
#include "physical_types_util.h"
#include "prepack.h"
#include "region.h"
#include "user_place_constraints.h"
#include "vpr_utils.h"
#include "vtr_assert.h"
#include "vtr_geometry.h"
#include "vtr_log.h"
#include "vtr_ndmatrix.h"
#include "vtr_strong_id.h"
#include "vtr_time.h"
#include "vtr_vector.h"

/// @brief The number of worst tiles reported in the legality summary.
static constexpr size_t NUM_WORST_TILES_TO_REPORT = 10;

namespace {

/**
 * @brief The classes of tile input pins.
 *
 * The cluster legalizer routes every net which enters a cluster through one
 * of the root input or clock pins of the cluster, so every net entering a tile
 * uses a tile pin (including global and constant nets). Which class of pin it
 * uses depends on which root pins can reach the primitive pins it connects to.
 */
enum class e_tile_pin_class : size_t {
    REGULAR, ///< Non-global input pins.
    GLOBAL,  ///< Non-clock global input pins (e.g. dedicated reset pins marked is_non_clock_global).
    CLOCK,   ///< Clock pins.
    NUM_CLASSES
};

/// @brief The number of classes of tile input pins.
constexpr size_t NUM_TILE_PIN_CLASSES = static_cast<size_t>(e_tile_pin_class::NUM_CLASSES);

/**
 * @brief The number of pins of a tile which can be used by the nets entering
 *        and leaving the tile.
 */
struct t_tile_pin_capacity {
    std::array<int, NUM_TILE_PIN_CLASSES> num_input_pins = {}; ///< The number of input pins of each class.
    int num_output_pins = 0;                                   ///< The number of output pins.
};

/**
 * @brief The ID of a kind of atom.
 *
 * Atoms of the same kind have the same model and the same number of used pins
 * on each port. Whether a primitive can implement an atom only depends on
 * these (see primitive_type_feasible), so a primitive which can implement one
 * atom of a kind can implement every atom of that kind.
 */
typedef vtr::StrongId<struct atom_kind_id_tag, size_t> AtomKindId;

/**
 * @brief The ID of a type of primitive slot.
 *
 * A slot is a physical primitive site within a tile which can implement an
 * atom. Its slot type is its primitive pb_type; every primitive pb_type of the
 * device which implements a model is given a slot type.
 */
typedef vtr::StrongId<struct slot_type_id_tag, size_t> SlotTypeId;

/**
 * @brief The ID of a dimension of a tile type's slot frontier.
 *
 * The slot types within a tile type which can implement exactly the same kinds
 * of atoms are interchangeable, so their slots are counted together in one
 * dimension (see t_slot_frontier). The dimensions are numbered separately for
 * each tile type.
 */
typedef vtr::StrongId<struct slot_dim_id_tag, size_t> SlotDimId;

/**
 * @brief A slot combination: the number of slots of each dimension which a
 *        tile uses at the same time.
 *
 * For example, a fracturable LUT6 which can either be used as one LUT6 or two
 * LUT5s can use the combinations {1 LUT6, 0 LUT5} and {0 LUT6, 2 LUT5}.
 *
 * A combination is dominated by another if it uses fewer or equal slots in
 * every dimension. A dominated combination adds no capacity to the tile, so it
 * can be removed.
 */
typedef vtr::vector<SlotDimId, int> t_slot_combination;

/**
 * @brief Compares two vtr::vectors in lexicographic order.
 *
 * vtr::vector does not provide comparison operators, so this is used to sort
 * them and to use them as the keys of ordered maps.
 */
struct t_vtr_vector_less {
    template<typename K, typename V>
    bool operator()(const vtr::vector<K, V>& lhs, const vtr::vector<K, V>& rhs) const {
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }
};

/// @brief The maximum number of slot combinations stored for a tile type. If
///        a tile type has more, its combinations are relaxed to a single
///        combination which over-estimates its capacity.
constexpr size_t MAX_SLOT_FRONTIER_COMBINATIONS = 256;

/**
 * @brief The slot frontier of a tile type: the slot combinations which the
 *        tile can use at the same time, excluding any dominated combination
 *        (see t_slot_combination).
 *
 * The modes of the tile decide which combinations of slots can be used at the
 * same time, including slots of different models. For example, a RAM block
 * whose modes each implement a different model (e.g. a different address
 * width) can only use the slots of one of those models at a time.
 *
 * Primitive pb_types which can implement exactly the same kinds of atoms in
 * the netlist are interchangeable, so their slots are counted together in one
 * dimension (see SlotDimId). Primitive pb_types which cannot implement any
 * atom in the netlist are not counted.
 */
struct t_slot_frontier {
    /// @brief A slot type (primitive pb_type) of each dimension. Every
    ///        primitive pb_type sharing a dimension can implement the same
    ///        kinds of atoms, so any one of them represents it.
    vtr::vector<SlotDimId, SlotTypeId> dim_slot_types;
    /// @brief The slot combinations which are not dominated by another.
    std::vector<t_slot_combination> combinations;
};

/**
 * @brief The information about a physical tile type needed by the rules.
 */
struct t_tile_type_info {
    /// @brief The pins of the tile type.
    t_tile_pin_capacity pin_capacity;
    /// @brief The class of tile pin used by the nets entering each model input
    ///        port implemented within the tile type.
    std::unordered_map<const t_model_ports*, e_tile_pin_class> port_pin_class;
    /// @brief The global slot types (primitive pb_types) within the tile type.
    std::vector<SlotTypeId> slot_types;
    /// @brief The primitive slots of the tile type.
    t_slot_frontier slot_frontier;
    /// @brief The number of RAM blocks of the tile type which can be used at
    ///        the same time. The RAM slices within a RAM block must share all
    ///        of their non-data nets.
    int num_ram_blocks = 0;
};

/**
 * @brief Where an atom is placed on the device grid.
 */
struct t_atom_placement {
    /// @brief The grid location containing the atom. If the atom is outside of
    ///        the device, this is the closest grid location on the device.
    t_physical_tile_loc grid_loc;
    /// @brief Whether the atom is placed outside of the device.
    bool is_off_device = false;
    /// @brief The root location of the tile containing the atom. This is
    ///        invalid if the atom is not in a tile (i.e. it is outside of the
    ///        device or in an empty grid location).
    t_physical_tile_loc tile_root;
};

/**
 * @brief How the pins of a single net within a tile are used.
 *
 * Only the pins of primitives which are still in the tile (i.e. not evicted)
 * are counted. The pins of evicted primitives are considered to be outside
 * of the tile, since they must move.
 */
struct t_net_tile_usage {
    bool has_driver = false;                                 ///< Whether the driver of the net is in the tile.
    int num_sinks = 0;                                       ///< The number of sinks of the net in the tile.
    std::array<int, NUM_TILE_PIN_CLASSES> num_class_sinks{}; ///< The number of sinks in the tile which need each class of tile pin.
};

/**
 * @brief The number of tile pins used by the nets entering and leaving a tile.
 */
struct t_tile_pin_demand {
    std::array<int, NUM_TILE_PIN_CLASSES> num_inputs{}; ///< The number of nets entering the tile through each class of input pin.
    int num_outputs = 0;                                ///< The number of nets leaving the tile.

    /// @brief Add the given change in demand to this demand.
    t_tile_pin_demand& operator+=(const t_tile_pin_demand& delta) {
        for (size_t pin_class = 0; pin_class < NUM_TILE_PIN_CLASSES; pin_class++)
            num_inputs[pin_class] += delta.num_inputs[pin_class];
        num_outputs += delta.num_outputs;
        return *this;
    }
};

/**
 * @brief The ports of a RAM slice model which must have the same nets
 *        (including unconnected pins) for RAM slices to share a RAM block,
 *        along with the width of each port.
 */
typedef std::vector<std::pair<const t_model_ports*, int>> t_ram_shared_ports;

/**
 * @brief A chain of molecules (e.g. a carry chain split across several tiles)
 *        which must be placed at a fixed offset from each other.
 */
struct t_chain {
    /// @brief The molecules of the chain, in order from the start of the chain.
    std::vector<PackMoleculeId> molecules;
    /// @brief The offset from the tile of each molecule to the tile of the
    ///        next molecule in the chain.
    int dx = 0;
    int dy = 0;
};

/**
 * @brief The primitives placed within an occupied tile, and their penalty.
 */
struct t_tile_state {
    /// @brief The atoms placed within the tile, sorted by ID.
    std::vector<AtomBlockId> atoms;
    /// @brief The penalty of the atoms evicted from the tile.
    t_ap_tile_legality_penalty penalty;
};

/**
 * @brief Estimates the legality of a flat placement.
 *
 * This class holds the state shared by the legality rules (which atoms have
 * been evicted, and the penalty so far). It is constructed and run once per
 * estimate.
 */
class LegalityEstimator {
  public:
    /**
     * @brief Pre-compute the information needed by the rules from the
     *        architecture, netlist, and placement (see estimate_legality for
     *        the parameters).
     */
    LegalityEstimator(const PartialPlacement& p_placement,
                      const APNetlist& ap_netlist,
                      const AtomNetlist& atom_netlist,
                      const Prepacker& prepacker,
                      const DeviceGrid& device_grid,
                      const std::vector<t_physical_tile_type>& physical_tile_types,
                      const std::vector<t_direct_inf>& directs,
                      const UserPlaceConstraints& constraints);

    /**
     * @brief Apply all of the legality rules and return the estimate.
     */
    t_ap_legality_estimate run();

  private:
    /**
     * @brief Compute the pins, RAM blocks, and primitive slot types of each
     *        tile type from the architecture.
     */
    void init_tile_type_info(const std::vector<t_physical_tile_type>& physical_tile_types);

    /**
     * @brief Group the atoms into kinds, and find which slot types and tile
     *        types can implement each kind.
     */
    void init_atom_kinds(const std::vector<t_physical_tile_type>& physical_tile_types);

    /**
     * @brief Compute the slot frontier of each tile type (see
     *        t_slot_frontier).
     */
    void init_slot_frontiers(const std::vector<t_physical_tile_type>& physical_tile_types);

    /**
     * @brief Find the chains of molecules which must be placed at a fixed
     *        offset from each other.
     */
    void init_chains(const std::vector<t_physical_tile_type>& physical_tile_types,
                     const std::vector<t_direct_inf>& directs);

    /**
     * @brief Find where each atom is placed, count the connected pins of each
     *        atom, and collect the atoms in each occupied tile.
     */
    void init_atom_placement(const PartialPlacement& p_placement, const APNetlist& ap_netlist);

    /**
     * @brief Evict the atoms which are in a tile that cannot implement them
     *        (INCOMPATIBLE_TILE), or which are outside of their region
     *        (OUTSIDE_REGION).
     */
    void apply_tile_type_and_region_rules();

    /**
     * @brief Evict the atoms of the given chain which are not at the required
     *        offset from the rest of the chain (CHAIN_OFFSET).
     */
    void apply_chain_offset_rule(const t_chain& chain);

    /**
     * @brief Evict groups of atoms which share a clock / global net until the
     *        nets entering the tile fit within the clock / global pins of the
     *        tile (CONTROL_PINS).
     */
    void apply_control_pin_rule(t_tile_state& tile);

    /**
     * @brief Evict the groups of RAM slices which cannot share a RAM block
     *        with the others until they fit within the RAM blocks of the tile
     *        (RAM_SHARING).
     */
    void apply_ram_sharing_rule(t_tile_state& tile);

    /**
     * @brief Evict atoms until the atoms fit within one of the slot
     *        combinations of the tile (PRIMITIVE_CAPACITY).
     */
    void apply_primitive_capacity_rule(t_tile_state& tile);

    /**
     * @brief Evict atoms until the nets entering and leaving the tile fit
     *        within the regular input and output pins of the tile (TILE_PINS).
     */
    void apply_tile_pin_rule(t_tile_state& tile);

    /**
     * @brief Count the number of molecules whose atoms are not all within the
     *        same tile. This is a sanity check and is not part of the score.
     */
    size_t count_split_molecules() const;

    /**
     * @brief Evict the given atom, charging its connected pins to the given
     *        rule.
     */
    void evict_atom(AtomBlockId atom, e_ap_legality_rule rule);

    /**
     * @brief Get the region the given atom is constrained to, or nullptr if
     *        it is unconstrained.
     */
    const PartitionRegion* get_atom_region(AtomBlockId atom) const;

    /**
     * @brief Get the class of tile pin which the net connected to the given
     *        sink pin uses to enter a tile of the given type.
     */
    e_tile_pin_class get_sink_pin_class(AtomPinId sink_pin, int tile_type_idx) const;

    /**
     * @brief Add (sign = 1) or remove (sign = -1) the pins of the given atom
     *        to / from the given per-net tile usage.
     */
    void update_net_tile_usage(AtomBlockId atom,
                               int tile_type_idx,
                               int sign,
                               std::unordered_map<AtomNetId, t_net_tile_usage>& net_usage) const;

    /**
     * @brief Get the per-net tile usage of the atoms in the given tile.
     *
     *  @param tile             The tile to get the net usage of.
     *  @param include_evicted  If true, the atoms evicted from the tile are
     *                          also counted (i.e. the usage of every atom
     *                          placed in the tile).
     */
    std::unordered_map<AtomNetId, t_net_tile_usage> get_net_tile_usage(const t_tile_state& tile,
                                                                       bool include_evicted = false) const;

    /**
     * @brief Get the number of tile pins used by the nets with the given
     *        per-net tile usage.
     */
    t_tile_pin_demand get_tile_pin_demand(const std::unordered_map<AtomNetId, t_net_tile_usage>& net_usage) const;

    /**
     * @brief Returns true if the net enters the tile through a pin of the
     *        given class (i.e. its driver is outside the tile and it has a
     *        sink in the tile which needs that class of pin).
     *
     * Nets driven within the tile are assumed to be routed within the tile.
     */
    static bool is_tile_input(const t_net_tile_usage& usage, e_tile_pin_class pin_class);

    /**
     * @brief Returns true if the net uses a tile output pin (i.e. its driver
     *        is in the tile and it has a sink outside the tile).
     */
    bool is_tile_output(AtomNetId net, const t_net_tile_usage& usage) const;

    /**
     * @brief Get the change in the tile pin demand if the given atom were
     *        evicted from the tile with the given net usage.
     */
    t_tile_pin_demand get_eviction_pin_demand_delta(AtomBlockId atom,
                                                    int tile_type_idx,
                                                    const std::unordered_map<AtomNetId, t_net_tile_usage>& net_usage) const;

    /**
     * @brief Write the penalty of every occupied tile to the given file.
     */
    void write_legality_estimate_echo(const std::string& filename) const;

    const AtomNetlist& atom_netlist_;
    const Prepacker& prepacker_;
    const DeviceGrid& device_grid_;
    const UserPlaceConstraints& constraints_;

    /// @brief The information about each physical tile type (indexed by the
    ///        physical tile type index).
    std::vector<t_tile_type_info> tile_type_info_;
    /// @brief The primitive pb_type of each global slot type.
    vtr::vector<SlotTypeId, const t_pb_type*> slot_pb_types_;
    /// @brief The shared ports of each RAM slice model.
    std::unordered_map<LogicalModelId, t_ram_shared_ports> ram_model_shared_ports_;

    /// @brief The kind of each atom. Atoms of the same kind have the same model
    ///        and the same number of used pins on each port, so they can be
    ///        implemented by the same primitives.
    vtr::vector<AtomBlockId, AtomKindId> atom_kind_;
    /// @brief [kind][slot type] Whether each slot type can implement each kind.
    vtr::vector<AtomKindId, vtr::vector<SlotTypeId, bool>> kind_feasible_slots_;
    /// @brief [kind][physical tile type index] Whether each tile type can
    ///        implement each kind.
    vtr::vector<AtomKindId, std::vector<bool>> kind_tile_compatible_;
    /// @brief Whether any tile on the device can implement each kind.
    vtr::vector<AtomKindId, bool> kind_has_tile_on_device_;

    /// @brief Where each atom is placed.
    vtr::vector<AtomBlockId, t_atom_placement> atom_placement_;
    /// @brief The number of connected pins of each atom.
    vtr::vector<AtomBlockId, int> atom_num_pins_;
    /// @brief Whether each atom has been evicted by a rule.
    vtr::vector<AtomBlockId, bool> atom_is_evicted_;

    /// @brief The chains whose molecules must be placed at a fixed offset
    ///        from each other.
    std::vector<t_chain> chains_;

    /// @brief The occupied tiles, keyed by the root location of each tile.
    ///        An ordered map is used so that the results are deterministic.
    std::map<t_physical_tile_loc, t_tile_state> tiles_;

    /// @brief The estimate being built.
    t_ap_legality_estimate estimate_;
};

} // namespace

const char* ap_legality_rule_name(e_ap_legality_rule rule) {
    switch (rule) {
        case e_ap_legality_rule::INCOMPATIBLE_TILE:
            return "incompatible_tile";
        case e_ap_legality_rule::OUTSIDE_REGION:
            return "outside_region";
        case e_ap_legality_rule::CHAIN_OFFSET:
            return "chain_offset";
        case e_ap_legality_rule::CONTROL_PINS:
            return "control_pins";
        case e_ap_legality_rule::RAM_SHARING:
            return "ram_sharing";
        case e_ap_legality_rule::PRIMITIVE_CAPACITY:
            return "primitive_capacity";
        case e_ap_legality_rule::TILE_PINS:
            return "tile_pins";
        default:
            VTR_ASSERT_MSG(false, "Unknown legality rule");
            return "";
    }
}

double t_ap_legality_estimate::normalized_pins_in_violation() const {
    if (num_connected_pins == 0)
        return 0.0;
    return static_cast<double>(pins_in_violation) / static_cast<double>(num_connected_pins);
}

// ============================================================================
// Primitive slots
// ============================================================================

namespace {

/**
 * @brief Computes the slot frontier of a tile type (see t_slot_frontier).
 *
 * Each instance of a pb_type is in one of its modes, so its combinations are
 * the union of the combinations of each mode. Within a mode, each child
 * instance is independent, so their combinations are added (every combination
 * of one child plus every combination of the other).
 *
 * The number of combinations can grow quickly in tiles with many independent
 * children with many modes. If a set of combinations grows larger than
 * MAX_SLOT_FRONTIER_COMBINATIONS, it is relaxed to the single combination
 * which uses the most slots in every dimension. This over-estimates the
 * capacity of the tile, so the estimate remains a lower bound.
 */
class SlotFrontierBuilder {
  public:
    /**
     *  @param slot_dims    The dimension of each primitive pb_type which is
     *                      counted. Other primitives are not counted.
     *  @param num_dims     The number of dimensions.
     */
    SlotFrontierBuilder(const std::unordered_map<const t_pb_type*, SlotDimId>& slot_dims, size_t num_dims)
        : slot_dims_(slot_dims)
        , num_dims_(num_dims) {}

    /**
     * @brief Get the slot frontier of the given tile type.
     *
     * Each sub-tile instance holds one of its equivalent sites, so its
     * combinations are the union of the combinations of each site. The
     * sub-tile instances are independent, so their combinations are added.
     */
    std::vector<t_slot_combination> get_tile_combinations(const t_physical_tile_type& tile_type) {
        std::vector<t_slot_combination> tile_combinations = {t_slot_combination(num_dims_, 0)};
        for (const t_sub_tile& sub_tile : tile_type.sub_tiles) {
            std::vector<t_slot_combination> site_combinations;
            for (t_logical_block_type_ptr site : sub_tile.equivalent_sites) {
                if (site->pb_type == nullptr)
                    continue;
                std::vector<t_slot_combination> combinations = get_pb_type_combinations(*site->pb_type);
                site_combinations.insert(site_combinations.end(), combinations.begin(), combinations.end());
            }
            if (site_combinations.empty())
                continue;
            site_combinations = prune_dominated_combinations(std::move(site_combinations));
            tile_combinations = add_combinations(tile_combinations, repeat_combinations(site_combinations, sub_tile.capacity.total()));
        }
        return tile_combinations;
    }

    /**
     * @brief Returns true if any of the combinations were relaxed (i.e. the
     *        capacity of the tile type is over-estimated).
     */
    bool is_relaxed() const {
        return is_relaxed_;
    }

  private:
    /**
     * @brief Remove the duplicate and dominated combinations (see
     *        t_slot_combination).
     *
     * If more than MAX_SLOT_FRONTIER_COMBINATIONS combinations remain, they are
     * relaxed to the single combination which uses the most slots in every
     * dimension.
     */
    std::vector<t_slot_combination> prune_dominated_combinations(std::vector<t_slot_combination> combinations) {
        // Sorted in decreasing lexicographic order, a combination can only be
        // dominated by a combination before it.
        std::sort(combinations.begin(), combinations.end(), [](const t_slot_combination& lhs, const t_slot_combination& rhs) {
            return t_vtr_vector_less()(rhs, lhs);
        });
        auto is_equal = [](const t_slot_combination& lhs, const t_slot_combination& rhs) {
            return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
        };
        combinations.erase(std::unique(combinations.begin(), combinations.end(), is_equal), combinations.end());

        std::vector<t_slot_combination> frontier;
        for (const t_slot_combination& combination : combinations) {
            bool is_dominated = std::any_of(frontier.begin(), frontier.end(), [&](const t_slot_combination& other) {
                for (SlotDimId dim : combination.keys()) {
                    if (other[dim] < combination[dim])
                        return false;
                }
                return true;
            });
            if (is_dominated)
                continue;
            frontier.push_back(combination);
            if (frontier.size() > MAX_SLOT_FRONTIER_COMBINATIONS) {
                is_relaxed_ = true;
                t_slot_combination max_combination(num_dims_, 0);
                for (const t_slot_combination& other : combinations) {
                    for (SlotDimId dim : max_combination.keys())
                        max_combination[dim] = std::max(max_combination[dim], other[dim]);
                }
                return {max_combination};
            }
        }
        return frontier;
    }

    /**
     * @brief Get the slot combinations which two independent blocks can use
     *        together: every combination of one block plus every combination
     *        of the other.
     */
    std::vector<t_slot_combination> add_combinations(const std::vector<t_slot_combination>& lhs,
                                                     const std::vector<t_slot_combination>& rhs) {
        std::vector<t_slot_combination> sum;
        sum.reserve(lhs.size() * rhs.size());
        for (const t_slot_combination& lhs_combination : lhs) {
            for (const t_slot_combination& rhs_combination : rhs) {
                t_slot_combination combination = lhs_combination;
                for (SlotDimId dim : combination.keys())
                    combination[dim] += rhs_combination[dim];
                sum.push_back(std::move(combination));
            }
        }
        return prune_dominated_combinations(std::move(sum));
    }

    /**
     * @brief Get the slot combinations which the given number of independent
     *        copies of a block can use together.
     */
    std::vector<t_slot_combination> repeat_combinations(const std::vector<t_slot_combination>& combinations, int num_copies) {
        std::vector<t_slot_combination> result = {t_slot_combination(num_dims_, 0)};
        for (int i = 0; i < num_copies; i++)
            result = add_combinations(result, combinations);
        return result;
    }

    /**
     * @brief Get the slot combinations which a single instance of the given
     *        pb_type can use at the same time, excluding dominated ones.
     */
    std::vector<t_slot_combination> get_pb_type_combinations(const t_pb_type& pb_type) {
        if (pb_type.is_primitive()) {
            t_slot_combination combination(num_dims_, 0);
            auto it = slot_dims_.find(&pb_type);
            if (it != slot_dims_.end())
                combination[it->second] = 1;
            return {combination};
        }

        std::vector<t_slot_combination> combinations;
        for (int imode = 0; imode < pb_type.num_modes; imode++) {
            const t_mode& mode = pb_type.modes[imode];
            std::vector<t_slot_combination> mode_combinations = {t_slot_combination(num_dims_, 0)};
            for (int ichild = 0; ichild < mode.num_pb_type_children; ichild++) {
                const t_pb_type& child_pb_type = mode.pb_type_children[ichild];
                mode_combinations = add_combinations(mode_combinations,
                                                     repeat_combinations(get_pb_type_combinations(child_pb_type), child_pb_type.num_pb));
            }
            combinations.insert(combinations.end(), mode_combinations.begin(), mode_combinations.end());
        }
        return prune_dominated_combinations(std::move(combinations));
    }

    const std::unordered_map<const t_pb_type*, SlotDimId>& slot_dims_;
    size_t num_dims_;
    bool is_relaxed_ = false;
};

} // namespace

/**
 * @brief Collect every primitive pb_type within the given pb_type.
 */
static void collect_primitive_pb_types(const t_pb_type& pb_type, std::vector<const t_pb_type*>& primitives) {
    if (pb_type.is_primitive()) {
        primitives.push_back(&pb_type);
        return;
    }
    for (int imode = 0; imode < pb_type.num_modes; imode++) {
        const t_mode& mode = pb_type.modes[imode];
        for (int ichild = 0; ichild < mode.num_pb_type_children; ichild++)
            collect_primitive_pb_types(mode.pb_type_children[ichild], primitives);
    }
}

/**
 * @brief Get the primitive pb_types of every logical block which can be placed
 *        in the given physical tile type.
 */
static std::vector<const t_pb_type*> get_tile_primitive_pb_types(const t_physical_tile_type& tile_type) {
    std::vector<const t_pb_type*> primitives;
    for (const t_sub_tile& sub_tile : tile_type.sub_tiles) {
        for (t_logical_block_type_ptr site : sub_tile.equivalent_sites) {
            if (site->pb_type != nullptr)
                collect_primitive_pb_types(*site->pb_type, primitives);
        }
    }
    // A logical block may be an equivalent site of more than one sub-tile.
    std::sort(primitives.begin(), primitives.end());
    primitives.erase(std::unique(primitives.begin(), primitives.end()), primitives.end());
    return primitives;
}

/**
 * @brief Get the minimum number of atoms of the given classes which must be
 *        evicted for them to fit within the given slot combination.
 *
 * Fitting the atoms into the slots is a matching problem: each atom needs its
 * own slot, and can only use the slots which can implement it. The number of
 * atoms which cannot be matched to a slot (the deficiency) is the number which
 * must be evicted.
 *
 * Comparing the total number of atoms against the total number of slots is not
 * enough, since some atoms can only use some of the slots. For example, with
 * one LUT6 slot and two LUT5 slots, two LUT6 atoms and one LUT2 atom do not
 * fit, even though there are three atoms and three slots: the two LUT6 atoms
 * only have one slot between them.
 *
 * The largest number of atoms which can be matched to slots is the maximum
 * flow through the network:
 *
 *      source --[atoms]--> class --> dimension --[slots]--> sink
 *
 * where the source is connected to each class with a capacity of the number
 * of atoms in the class, each class is connected to each dimension whose slots
 * can implement it, and each dimension is connected to the sink with a
 * capacity of the number of slots of the dimension. Each unit of flow assigns
 * one atom to one slot. The atoms of a class can all use the same slots, so
 * only the number of atoms in each class matters.
 *
 * The network has one node per class and per dimension, so it is small. The
 * maximum flow is found with the Edmonds-Karp algorithm, which repeatedly
 * pushes flow along the shortest path from the source to the sink which still
 * has capacity.
 *
 *  @param classes          The atom classes to check. These should not share
 *                          any dimension with the other classes.
 *  @param class_demand     The number of atoms of each class.
 *  @param class_slots      [class][dim] Whether the slots of each dimension
 *                          can implement each atom class.
 *  @param combination      The slot combination to fit the atoms within.
 */
static int get_slot_deficiency(const std::vector<size_t>& classes,
                               const std::vector<int>& class_demand,
                               const std::vector<vtr::vector<SlotDimId, bool>>& class_slots,
                               const t_slot_combination& combination) {
    // Node 0 is the source, followed by a node for each class, a node for each
    // dimension, and the sink.
    size_t num_classes = classes.size();
    size_t num_nodes = 1 + num_classes + combination.size() + 1;
    size_t source = 0;
    size_t sink = num_nodes - 1;
    auto class_node = [](size_t i) { return 1 + i; };
    auto dim_node = [&](SlotDimId dim) { return 1 + num_classes + static_cast<size_t>(dim); };

    // The remaining (residual) capacity of each edge. The edge from a class to
    // a dimension does not limit the flow, so it is given the number of atoms
    // in the class.
    vtr::NdMatrix<int, 2> capacity({num_nodes, num_nodes}, 0);
    int demand = 0;
    for (size_t i = 0; i < num_classes; i++) {
        size_t c = classes[i];
        demand += class_demand[c];
        capacity[source][class_node(i)] = class_demand[c];
        for (SlotDimId dim : combination.keys()) {
            if (class_slots[c][dim])
                capacity[class_node(i)][dim_node(dim)] = class_demand[c];
        }
    }
    for (SlotDimId dim : combination.keys())
        capacity[dim_node(dim)][sink] = combination[dim];

    int max_flow = 0;
    std::vector<size_t> parent(num_nodes);
    std::vector<bool> visited(num_nodes);
    std::queue<size_t> frontier;
    while (true) {
        // Find the shortest path from the source to the sink with capacity.
        std::fill(visited.begin(), visited.end(), false);
        visited[source] = true;
        frontier.push(source);
        while (!frontier.empty()) {
            size_t node = frontier.front();
            frontier.pop();
            for (size_t next = 0; next < num_nodes; next++) {
                if (visited[next] || capacity[node][next] <= 0)
                    continue;
                visited[next] = true;
                parent[next] = node;
                frontier.push(next);
            }
        }
        if (!visited[sink])
            break;

        // Push as much flow as the path allows. Pushing flow along an edge
        // allows it to be undone later through the reverse edge.
        int path_flow = std::numeric_limits<int>::max();
        for (size_t node = sink; node != source; node = parent[node])
            path_flow = std::min(path_flow, capacity[parent[node]][node]);
        for (size_t node = sink; node != source; node = parent[node]) {
            capacity[parent[node]][node] -= path_flow;
            capacity[node][parent[node]] += path_flow;
        }
        max_flow += path_flow;
    }

    return demand - max_flow;
}

/**
 * @brief Split the given atom classes into groups which do not share any
 *        dimension, directly or through other classes in the group.
 *
 * The deficiency of each group (see get_slot_deficiency) is independent of
 * the other groups, so evicting an atom only changes the deficiency of its
 * own group.
 *
 *  @param class_slots  [class][dim] Whether the slots of each dimension can
 *                      implement each atom class.
 *
 *  @return The classes in each group.
 */
static std::vector<std::vector<size_t>> get_independent_class_groups(const std::vector<vtr::vector<SlotDimId, bool>>& class_slots) {
    auto share_slot = [&](size_t lhs, size_t rhs) {
        for (SlotDimId dim : class_slots[lhs].keys()) {
            if (class_slots[lhs][dim] && class_slots[rhs][dim])
                return true;
        }
        return false;
    };

    // Each group is the connected component of the classes which share a
    // dimension.
    size_t num_classes = class_slots.size();
    std::vector<bool> is_grouped(num_classes, false);
    std::vector<std::vector<size_t>> groups;
    for (size_t first_c = 0; first_c < num_classes; first_c++) {
        if (is_grouped[first_c])
            continue;
        std::vector<size_t>& group = groups.emplace_back(1, first_c);
        is_grouped[first_c] = true;
        for (size_t i = 0; i < group.size(); i++) {
            for (size_t c = 0; c < num_classes; c++) {
                if (!is_grouped[c] && share_slot(group[i], c)) {
                    is_grouped[c] = true;
                    group.push_back(c);
                }
            }
        }
    }
    return groups;
}

// ============================================================================
// Tile pins
// ============================================================================

/**
 * @brief Get the number of input pins of each class, and the number of output
 *        pins, of the given physical tile type.
 */
static t_tile_pin_capacity get_tile_pin_capacity(const t_physical_tile_type& tile_type) {
    std::unordered_set<int> clock_pins(tile_type.clock_pin_indices.begin(), tile_type.clock_pin_indices.end());
    t_tile_pin_capacity pin_capacity;
    for (int pin = 0; pin < tile_type.num_pins; pin++) {
        e_pin_type pin_type = tile_type.class_inf[tile_type.pin_class[pin]].type;
        if (pin_type == e_pin_type::DRIVER) {
            pin_capacity.num_output_pins++;
        } else if (pin_type == e_pin_type::RECEIVER) {
            e_tile_pin_class pin_class = e_tile_pin_class::REGULAR;
            if (clock_pins.count(pin) != 0)
                pin_class = e_tile_pin_class::CLOCK;
            else if (tile_type.is_pin_global[pin])
                pin_class = e_tile_pin_class::GLOBAL;
            pin_capacity.num_input_pins[static_cast<size_t>(pin_class)]++;
        }
    }
    return pin_capacity;
}

/**
 * @brief Get the class of tile pin which the nets entering each model input
 *        port implemented within the given tile type use.
 *
 * The pb_graph of each logical block of the tile is searched forward from the
 * root input and clock pins of each class, to find which classes of root pins
 * can reach the primitive pins of each model port. If the regular input pins
 * can reach a port, the regular input pins are assumed (since any net can use
 * them); otherwise the global pins, and otherwise the clock pins.
 */
static std::unordered_map<const t_model_ports*, e_tile_pin_class> get_port_pin_classes(const t_physical_tile_type& tile_type) {
    std::unordered_map<const t_model_ports*, std::array<bool, NUM_TILE_PIN_CLASSES>> port_reached_by;
    for (const t_sub_tile& sub_tile : tile_type.sub_tiles) {
        for (t_logical_block_type_ptr site : sub_tile.equivalent_sites) {
            const t_pb_graph_node* head = site->pb_graph_head;
            if (head == nullptr)
                continue;

            // Collect the root pins of each class.
            std::array<std::vector<const t_pb_graph_pin*>, NUM_TILE_PIN_CLASSES> root_pins;
            for (int iport = 0; iport < head->num_input_ports; iport++) {
                for (int ipin = 0; ipin < head->num_input_pins[iport]; ipin++) {
                    const t_pb_graph_pin* pin = &head->input_pins[iport][ipin];
                    // The root block pins are numbered by their logical pin index.
                    int physical_pin = get_physical_pin(&tile_type, site, pin->pin_count_in_cluster);
                    e_tile_pin_class pin_class = tile_type.is_pin_global[physical_pin] ? e_tile_pin_class::GLOBAL
                                                                                       : e_tile_pin_class::REGULAR;
                    root_pins[static_cast<size_t>(pin_class)].push_back(pin);
                }
            }
            for (int iport = 0; iport < head->num_clock_ports; iport++) {
                for (int ipin = 0; ipin < head->num_clock_pins[iport]; ipin++)
                    root_pins[static_cast<size_t>(e_tile_pin_class::CLOCK)].push_back(&head->clock_pins[iport][ipin]);
            }

            // Search forward from the root pins of each class to the primitive
            // pins they can reach.
            for (size_t pin_class = 0; pin_class < NUM_TILE_PIN_CLASSES; pin_class++) {
                std::vector<const t_pb_graph_pin*> stack = root_pins[pin_class];
                std::unordered_set<const t_pb_graph_pin*> visited(stack.begin(), stack.end());
                while (!stack.empty()) {
                    const t_pb_graph_pin* pin = stack.back();
                    stack.pop_back();
                    for (const t_pb_graph_edge* edge : pin->output_edges) {
                        for (int ipin = 0; ipin < edge->num_output_pins; ipin++) {
                            const t_pb_graph_pin* next_pin = edge->output_pins[ipin];
                            if (!visited.insert(next_pin).second)
                                continue;
                            if (next_pin->is_primitive_pin()) {
                                if (next_pin->port->model_port != nullptr)
                                    port_reached_by[next_pin->port->model_port][pin_class] = true;
                                continue;
                            }
                            stack.push_back(next_pin);
                        }
                    }
                }
            }
        }
    }

    std::unordered_map<const t_model_ports*, e_tile_pin_class> port_pin_class;
    for (const auto& [model_port, reached_by] : port_reached_by) {
        e_tile_pin_class pin_class = e_tile_pin_class::REGULAR;
        if (!reached_by[static_cast<size_t>(e_tile_pin_class::REGULAR)]) {
            if (reached_by[static_cast<size_t>(e_tile_pin_class::GLOBAL)])
                pin_class = e_tile_pin_class::GLOBAL;
            else if (reached_by[static_cast<size_t>(e_tile_pin_class::CLOCK)])
                pin_class = e_tile_pin_class::CLOCK;
        }
        port_pin_class[model_port] = pin_class;
    }
    return port_pin_class;
}

// ============================================================================
// RAM blocks
// ============================================================================

/**
 * @brief Get the number of RAM blocks within the given pb_type which can be
 *        used at the same time.
 *
 * A RAM block is a mode whose children are RAM slices: the slices of one RAM
 * block must share all of their non-data nets (as checked by the cluster
 * legalizer). Each pb_type uses one of its modes, so this is the maximum over
 * its modes.
 */
static int get_num_ram_blocks(const t_pb_type& pb_type) {
    int num_ram_blocks = 0;
    for (int imode = 0; imode < pb_type.num_modes; imode++) {
        const t_mode& mode = pb_type.modes[imode];
        bool is_ram_block = false;
        int mode_num_ram_blocks = 0;
        for (int ichild = 0; ichild < mode.num_pb_type_children; ichild++) {
            const t_pb_type& child = mode.pb_type_children[ichild];
            is_ram_block |= child.is_memory_primitive();
            mode_num_ram_blocks += child.num_pb * get_num_ram_blocks(child);
        }
        if (is_ram_block)
            mode_num_ram_blocks = 1;
        num_ram_blocks = std::max(num_ram_blocks, mode_num_ram_blocks);
    }
    return num_ram_blocks;
}

/**
 * @brief Get the number of RAM blocks of the given tile type which can be
 *        used at the same time.
 */
static int get_tile_num_ram_blocks(const t_physical_tile_type& tile_type) {
    int num_ram_blocks = 0;
    for (const t_sub_tile& sub_tile : tile_type.sub_tiles) {
        int site_num_ram_blocks = 0;
        for (t_logical_block_type_ptr site : sub_tile.equivalent_sites) {
            if (site->pb_type != nullptr)
                site_num_ram_blocks = std::max(site_num_ram_blocks, get_num_ram_blocks(*site->pb_type));
        }
        num_ram_blocks += sub_tile.capacity.total() * site_num_ram_blocks;
    }
    return num_ram_blocks;
}

/**
 * @brief Get the ports of the given RAM slice which must have the same nets
 *        for RAM slices to share a RAM block.
 *
 * These are the ports whose port_class does not start with "data" (the same
 * ports the cluster legalizer checks, see primitive_memory_sibling_feasible).
 */
static t_ram_shared_ports get_ram_shared_ports(const t_pb_type& ram_slice) {
    t_ram_shared_ports shared_ports;
    for (int iport = 0; iport < ram_slice.num_ports; iport++) {
        const t_port& port = ram_slice.ports[iport];
        if (port.port_class != nullptr && std::string_view(port.port_class).starts_with("data"))
            continue;
        shared_ports.emplace_back(port.model_port, port.num_pins);
    }
    return shared_ports;
}

// ============================================================================
// Placement
// ============================================================================

/**
 * @brief Get where on the device grid an atom at the given position is placed.
 *
 * A position is contained within the grid location which is the floor of the
 * position (so each grid location includes its lower edge but not its upper
 * edge). Positions outside of the device are moved to the closest grid
 * location on the device, and are marked as off-device.
 */
static t_atom_placement get_atom_placement(double x, double y, double layer, const DeviceGrid& device_grid) {
    // Clamp each dimension onto the device.
    auto clamp_dim = [](double pos, size_t dim_size, bool& is_off_device) {
        double grid_pos = std::floor(pos);
        double clamped_pos = std::clamp(grid_pos, 0.0, static_cast<double>(dim_size) - 1.0);
        if (clamped_pos != grid_pos)
            is_off_device = true;
        return static_cast<int>(clamped_pos);
    };

    t_atom_placement atom_placement;
    atom_placement.grid_loc.x = clamp_dim(x, device_grid.width(), atom_placement.is_off_device);
    atom_placement.grid_loc.y = clamp_dim(y, device_grid.height(), atom_placement.is_off_device);
    atom_placement.grid_loc.layer_num = clamp_dim(layer, device_grid.get_num_layers(), atom_placement.is_off_device);

    // Off-device positions and empty grid locations are not in any tile.
    if (atom_placement.is_off_device)
        return atom_placement;
    if (device_grid.get_physical_type(atom_placement.grid_loc)->is_empty())
        return atom_placement;

    atom_placement.tile_root = device_grid.get_root_location(atom_placement.grid_loc);
    return atom_placement;
}

/**
 * @brief Returns true if the tile with the given root location is within the
 *        given region.
 *
 * The sub-tile of the region is ignored, since the estimator does not decide
 * which sub-tile a primitive goes into.
 */
static bool is_tile_in_region(const t_physical_tile_loc& tile_root, const PartitionRegion& pr) {
    for (const Region& region : pr.get_regions()) {
        const auto [layer_low, layer_high] = region.get_layer_range();
        if (tile_root.layer_num < layer_low || tile_root.layer_num > layer_high)
            continue;
        if (region.get_rect().contains_inclusive(vtr::Point<int>(tile_root.x, tile_root.y)))
            return true;
    }
    return false;
}

// ============================================================================
// Chains
// ============================================================================

/**
 * @brief Returns true if the given tile pin cannot connect to the general
 *        routing (i.e. every Fc of the pin is 0).
 *
 * Only these pins are used for the direct connections of placement macros
 * (see PlaceMacros).
 */
static bool is_dedicated_tile_pin(const t_physical_tile_type& tile_type, int physical_pin) {
    for (const t_fc_specification& fc_spec : tile_type.fc_specs) {
        if (fc_spec.fc_value > 0 && std::find(fc_spec.pins.begin(), fc_spec.pins.end(), physical_pin) != fc_spec.pins.end())
            return false;
    }
    return true;
}

/**
 * @brief Get the required offset (dx, dy) from each molecule of the given
 *        chain pack pattern to the next molecule of the chain.
 *
 * A chain enters a tile through the root block input pins whose edges are part
 * of its pack pattern (the same pins the prepacker uses to find the start of
 * a chain). If such a pin is the sink of a direct connection, the next
 * molecule of the chain must be at the offset of that direct connection from
 * the previous one; this is what forms the placement macros of the chain. As
 * in PlaceMacros, only pins which cannot connect to the general routing are
 * considered.
 *
 *  @return The offset, or std::nullopt if the chain is not connected by a
 *          direct connection (so its molecules can be placed anywhere) or can
 *          be connected at more than one offset (which is not checked).
 */
static std::optional<std::pair<int, int>> get_chain_offset(const t_pack_patterns& chain_pattern,
                                                           const std::vector<t_physical_tile_type>& physical_tile_types,
                                                           const std::vector<t_direct_inf>& directs) {
    // Find the tile pins (tile type index, physical pin) which the chain
    // enters a tile through.
    std::set<std::pair<int, int>> chain_input_tile_pins;
    for (const t_physical_tile_type& tile_type : physical_tile_types) {
        for (const t_sub_tile& sub_tile : tile_type.sub_tiles) {
            for (t_logical_block_type_ptr logical_block : sub_tile.equivalent_sites) {
                const t_pb_graph_node* root = logical_block->pb_graph_head;
                if (root == nullptr)
                    continue;
                for (int iport = 0; iport < root->num_input_ports; iport++) {
                    for (int ipin = 0; ipin < root->num_input_pins[iport]; ipin++) {
                        const t_pb_graph_pin& pin = root->input_pins[iport][ipin];
                        bool is_chain_input = std::any_of(pin.output_edges.begin(), pin.output_edges.end(),
                                                          [&](const t_pb_graph_edge* edge) {
                                                              return edge->belongs_to_pattern(chain_pattern.index);
                                                          });
                        if (!is_chain_input)
                            continue;
                        int physical_pin = get_physical_pin(&tile_type, logical_block, pin.pin_count_in_cluster);
                        if (is_dedicated_tile_pin(tile_type, physical_pin))
                            chain_input_tile_pins.insert({tile_type.index, physical_pin});
                    }
                }
            }
        }
    }

    // Find the direct connections whose sink is one of these pins.
    std::set<std::pair<int, int>> offsets;
    for (const t_direct_inf& direct : directs) {
        auto [start_pin_index, end_pin_index, tile_name, port_name] = parse_direct_pin_name(direct.to_pin, direct.line);
        t_physical_tile_type_ptr to_tile_type = find_tile_type_by_name(tile_name, physical_tile_types);
        if (to_tile_type == nullptr)
            continue;
        t_physical_tile_port to_port = find_tile_port_by_name(to_tile_type, port_name);
        if (start_pin_index == UNDEFINED) {
            start_pin_index = 0;
            end_pin_index = to_port.num_pins - 1;
        }
        int first_pin = to_port.absolute_first_pin_index + std::min(start_pin_index, end_pin_index);
        int last_pin = to_port.absolute_first_pin_index + std::max(start_pin_index, end_pin_index);
        for (int physical_pin = first_pin; physical_pin <= last_pin; physical_pin++) {
            if (chain_input_tile_pins.count({to_tile_type->index, physical_pin}) != 0) {
                offsets.insert({direct.x_offset, direct.y_offset});
                break;
            }
        }
    }

    if (offsets.empty())
        return std::nullopt;
    if (offsets.size() > 1) {
        VTR_LOG_WARN("Legality estimate: chain pack pattern '%s' can be connected at more than one offset; "
                     "its chain offsets are not checked.\n",
                     chain_pattern.name.c_str());
        return std::nullopt;
    }
    return *offsets.begin();
}

// ============================================================================
// Legality estimator
// ============================================================================

LegalityEstimator::LegalityEstimator(const PartialPlacement& p_placement,
                                     const APNetlist& ap_netlist,
                                     const AtomNetlist& atom_netlist,
                                     const Prepacker& prepacker,
                                     const DeviceGrid& device_grid,
                                     const std::vector<t_physical_tile_type>& physical_tile_types,
                                     const std::vector<t_direct_inf>& directs,
                                     const UserPlaceConstraints& constraints)
    : atom_netlist_(atom_netlist)
    , prepacker_(prepacker)
    , device_grid_(device_grid)
    , constraints_(constraints) {
    init_tile_type_info(physical_tile_types);
    init_atom_kinds(physical_tile_types);
    init_slot_frontiers(physical_tile_types);
    init_chains(physical_tile_types, directs);
    init_atom_placement(p_placement, ap_netlist);

    estimate_.tile_type_pins_in_violation.resize(physical_tile_types.size());
}

void LegalityEstimator::init_tile_type_info(const std::vector<t_physical_tile_type>& physical_tile_types) {
    tile_type_info_.resize(physical_tile_types.size());
    std::unordered_map<const t_pb_type*, SlotTypeId> slot_type_index;
    for (const t_physical_tile_type& tile_type : physical_tile_types) {
        t_tile_type_info& tile_info = tile_type_info_[tile_type.index];
        tile_info.pin_capacity = get_tile_pin_capacity(tile_type);
        tile_info.port_pin_class = get_port_pin_classes(tile_type);
        tile_info.num_ram_blocks = get_tile_num_ram_blocks(tile_type);

        // Give each primitive pb_type a slot type.
        for (const t_pb_type* primitive : get_tile_primitive_pb_types(tile_type)) {
            if (!primitive->model_id.is_valid())
                continue;
            auto [it, inserted] = slot_type_index.emplace(primitive, SlotTypeId(slot_pb_types_.size()));
            if (inserted) {
                slot_pb_types_.push_back(primitive);
                if (primitive->is_memory_primitive())
                    ram_model_shared_ports_.try_emplace(primitive->model_id, get_ram_shared_ports(*primitive));
            }
            tile_info.slot_types.push_back(it->second);
        }
    }
}

void LegalityEstimator::init_atom_kinds(const std::vector<t_physical_tile_type>& physical_tile_types) {
    // Group the atoms into kinds. Whether a primitive can implement an atom
    // only depends on the atom's model and the number of used pins on each of
    // its ports (see primitive_type_feasible), so atoms with the same model
    // and port usage are the same kind.
    typedef std::pair<LogicalModelId, std::vector<std::pair<const t_model_ports*, size_t>>> t_kind_signature;
    std::map<t_kind_signature, AtomKindId> kind_of_signature;
    vtr::vector<AtomKindId, AtomBlockId> kind_representative;
    atom_kind_.resize(atom_netlist_.blocks().size());
    for (AtomBlockId atom : atom_netlist_.blocks()) {
        t_kind_signature signature;
        signature.first = atom_netlist_.block_model(atom);
        for (AtomPortId port : atom_netlist_.block_ports(atom))
            signature.second.emplace_back(atom_netlist_.port_model(port), atom_netlist_.port_pins(port).size());
        std::sort(signature.second.begin(), signature.second.end());

        auto [it, inserted] = kind_of_signature.emplace(std::move(signature), AtomKindId(kind_representative.size()));
        if (inserted)
            kind_representative.push_back(atom);
        atom_kind_[atom] = it->second;
    }

    // Find which slot types and tile types can implement each kind.
    // NOTE: primitive_type_feasible is the same check the packer uses to decide
    //       if a primitive can implement an atom.
    size_t num_kinds = kind_representative.size();
    kind_feasible_slots_.assign(num_kinds, vtr::vector<SlotTypeId, bool>(slot_pb_types_.size(), false));
    kind_tile_compatible_.assign(num_kinds, std::vector<bool>(physical_tile_types.size(), false));
    for (AtomKindId kind : kind_representative.keys()) {
        AtomBlockId representative = kind_representative[kind];
        LogicalModelId model_id = atom_netlist_.block_model(representative);
        for (SlotTypeId slot_type : slot_pb_types_.keys()) {
            if (slot_pb_types_[slot_type]->model_id == model_id)
                kind_feasible_slots_[kind][slot_type] = primitive_type_feasible(representative, slot_pb_types_[slot_type]);
        }
        for (const t_physical_tile_type& tile_type : physical_tile_types) {
            for (SlotTypeId slot_type : tile_type_info_[tile_type.index].slot_types) {
                if (kind_feasible_slots_[kind][slot_type])
                    kind_tile_compatible_[kind][tile_type.index] = true;
            }
        }
    }

    // Find which kinds can be implemented by at least one tile on the device.
    // An evicted atom of a kind with no such tile cannot legally be placed
    // anywhere, which is counted as a sanity check.
    std::vector<bool> tile_type_on_device(physical_tile_types.size(), false);
    for (size_t layer = 0; layer < device_grid_.get_num_layers(); layer++) {
        for (size_t x = 0; x < device_grid_.width(); x++) {
            for (size_t y = 0; y < device_grid_.height(); y++)
                tile_type_on_device[device_grid_.get_physical_type(t_physical_tile_loc(x, y, layer))->index] = true;
        }
    }
    kind_has_tile_on_device_.assign(num_kinds, false);
    for (AtomKindId kind : kind_representative.keys()) {
        for (size_t tile_type_idx = 0; tile_type_idx < physical_tile_types.size(); tile_type_idx++) {
            if (tile_type_on_device[tile_type_idx] && kind_tile_compatible_[kind][tile_type_idx])
                kind_has_tile_on_device_[kind] = true;
        }
    }
}

void LegalityEstimator::init_slot_frontiers(const std::vector<t_physical_tile_type>& physical_tile_types) {
    // The slots of every model are counted together, since the modes of the
    // tile decide which slots (of any model) can be used at the same time.
    for (const t_physical_tile_type& tile_type : physical_tile_types) {
        t_tile_type_info& tile_info = tile_type_info_[tile_type.index];

        // Primitives which can implement the same kinds of atoms are
        // interchangeable, so they share a dimension. Primitives which cannot
        // implement any atom are not counted. This keeps the number of
        // dimensions (and combinations) small.
        std::map<vtr::vector<AtomKindId, bool>, SlotDimId, t_vtr_vector_less> dim_of_kinds;
        std::unordered_map<const t_pb_type*, SlotDimId> slot_dims;
        for (SlotTypeId slot_type : tile_info.slot_types) {
            vtr::vector<AtomKindId, bool> feasible_kinds(kind_feasible_slots_.size(), false);
            for (AtomKindId kind : kind_feasible_slots_.keys())
                feasible_kinds[kind] = kind_feasible_slots_[kind][slot_type];
            if (std::none_of(feasible_kinds.begin(), feasible_kinds.end(), [](bool feasible) { return feasible; }))
                continue;
            auto [it, inserted] = dim_of_kinds.emplace(std::move(feasible_kinds), SlotDimId(tile_info.slot_frontier.dim_slot_types.size()));
            if (inserted)
                tile_info.slot_frontier.dim_slot_types.push_back(slot_type);
            slot_dims[slot_pb_types_[slot_type]] = it->second;
        }

        SlotFrontierBuilder frontier_builder(slot_dims, tile_info.slot_frontier.dim_slot_types.size());
        tile_info.slot_frontier.combinations = frontier_builder.get_tile_combinations(tile_type);
        if (frontier_builder.is_relaxed()) {
            VTR_LOG_WARN("Legality estimate: tile type '%s' has too many combinations of primitives to count "
                         "exactly; its primitive capacity is over-estimated.\n",
                         tile_type.name.c_str());
        }
    }
}

void LegalityEstimator::init_chains(const std::vector<t_physical_tile_type>& physical_tile_types,
                                    const std::vector<t_direct_inf>& directs) {
    // The required offset between the molecules of each chain pack pattern.
    std::unordered_map<const t_pack_patterns*, std::optional<std::pair<int, int>>> pattern_offset;

    // The prepacker creates the molecules of a chain in order from the start
    // of the chain (see init_molecule_chain_info), so visiting the molecules
    // in ID order visits the molecules of each chain in order.
    std::unordered_map<MoleculeChainId, size_t> chain_index;
    for (PackMoleculeId mol_id : prepacker_.molecules()) {
        const t_pack_molecule& molecule = prepacker_.get_molecule(mol_id);
        // Only long chains are split into more than one molecule.
        if (!molecule.is_chain() || !prepacker_.get_molecule_chain_info(molecule.chain_id).is_long_chain)
            continue;

        auto [offset_it, is_new_pattern] = pattern_offset.try_emplace(molecule.pack_pattern);
        if (is_new_pattern)
            offset_it->second = get_chain_offset(*molecule.pack_pattern, physical_tile_types, directs);
        if (!offset_it->second.has_value())
            continue;

        auto [chain_it, is_new_chain] = chain_index.try_emplace(molecule.chain_id, chains_.size());
        if (is_new_chain) {
            t_chain& chain = chains_.emplace_back();
            std::tie(chain.dx, chain.dy) = offset_it->second.value();
        }
        chains_[chain_it->second].molecules.push_back(mol_id);
    }
    estimate_.num_chains = chains_.size();
}

void LegalityEstimator::init_atom_placement(const PartialPlacement& p_placement, const APNetlist& ap_netlist) {
    // Place each atom at the position of the AP block which contains it.
    // TODO: Once AP blocks contain multiple molecules with offsets, add the
    //       offset of the atom within its AP block here.
    atom_placement_.resize(atom_netlist_.blocks().size());
    vtr::vector<AtomBlockId, bool> atom_is_placed(atom_netlist_.blocks().size(), false);
    for (APBlockId ap_blk_id : ap_netlist.blocks()) {
        t_atom_placement blk_placement = get_atom_placement(p_placement.block_x_locs[ap_blk_id],
                                                            p_placement.block_y_locs[ap_blk_id],
                                                            p_placement.block_layer_nums[ap_blk_id],
                                                            device_grid_);
        for (PackMoleculeId mol_id : ap_netlist.block_molecules(ap_blk_id)) {
            for (AtomBlockId atom : prepacker_.get_molecule(mol_id).atom_block_ids) {
                if (!atom.is_valid())
                    continue;
                atom_placement_[atom] = blk_placement;
                atom_is_placed[atom] = true;
            }
        }
    }

    // Count the connected pins of each atom, and the total number of connected
    // pins (used to normalize the score).
    atom_num_pins_.resize(atom_netlist_.blocks().size(), 0);
    for (AtomBlockId atom : atom_netlist_.blocks()) {
        VTR_ASSERT_MSG(atom_is_placed[atom], "Every atom should be in a molecule of an AP block");
        for (AtomPinId pin : atom_netlist_.block_pins(atom)) {
            if (atom_netlist_.pin_net(pin).is_valid())
                atom_num_pins_[atom]++;
        }
        estimate_.num_connected_pins += atom_num_pins_[atom];
    }
    atom_is_evicted_.resize(atom_netlist_.blocks().size(), false);

    // Collect the atoms in each occupied tile. Atoms are visited in ID order,
    // so the atoms of each tile are sorted by ID.
    for (AtomBlockId atom : atom_netlist_.blocks()) {
        const t_physical_tile_loc& tile_root = atom_placement_[atom].tile_root;
        if (!tile_root.is_valid())
            continue;
        t_tile_state& tile = tiles_[tile_root];
        tile.atoms.push_back(atom);
        tile.penalty.root_loc = tile_root;
        tile.penalty.tile_type_index = device_grid_.get_physical_type(tile_root)->index;
    }
}

t_ap_legality_estimate LegalityEstimator::run() {
    // The atoms of a molecule are placed together by construction, so this
    // is only a sanity check.
    estimate_.num_split_molecules = count_split_molecules();
    if (estimate_.num_split_molecules != 0) {
        VTR_LOG_WARN("Legality estimate found %zu molecules split across tiles.\n",
                     estimate_.num_split_molecules);
    }

    // The rules are applied sequentially. Each rule only considers the atoms
    // which have not been evicted by an earlier rule, so each evicted atom is
    // charged to exactly one rule.

    // Tile type and region: these disqualify an atom on its own, regardless
    // of what else is in the tile.
    apply_tile_type_and_region_rules();

    // Chains: a relationship between atoms which may be in different tiles.
    for (const t_chain& chain : chains_)
        apply_chain_offset_rule(chain);

    // Control pins and RAM sharing: sharing limits.
    for (auto& [tile_root, tile] : tiles_)
        apply_control_pin_rule(tile);
    for (auto& [tile_root, tile] : tiles_)
        apply_ram_sharing_rule(tile);

    // Primitive capacity and tile pins: counting rules, which are applied
    // last since the earlier evictions reduce the demand on the tile.
    for (auto& [tile_root, tile] : tiles_)
        apply_primitive_capacity_rule(tile);
    for (auto& [tile_root, tile] : tiles_)
        apply_tile_pin_rule(tile);

    // Collect the worst tiles for the legality summary.
    for (const auto& [tile_root, tile] : tiles_) {
        if (tile.penalty.pins_in_violation > 0)
            estimate_.worst_tiles.push_back(tile.penalty);
    }
    // Stable sort, so tiles with the same penalty remain in location order.
    std::stable_sort(estimate_.worst_tiles.begin(), estimate_.worst_tiles.end(),
                     [](const t_ap_tile_legality_penalty& lhs, const t_ap_tile_legality_penalty& rhs) {
                         return lhs.pins_in_violation > rhs.pins_in_violation;
                     });
    if (estimate_.worst_tiles.size() > NUM_WORST_TILES_TO_REPORT)
        estimate_.worst_tiles.resize(NUM_WORST_TILES_TO_REPORT);

    // Write the per-tile penalties to an echo file if requested, so they can
    // be compared tile-by-tile against the mass overflow.
    if (isEchoFileEnabled(E_ECHO_AP_LEGALITY_ESTIMATE))
        write_legality_estimate_echo(getEchoFileName(E_ECHO_AP_LEGALITY_ESTIMATE));

    return estimate_;
}

void LegalityEstimator::apply_tile_type_and_region_rules() {
    for (AtomBlockId atom : atom_netlist_.blocks()) {
        const t_atom_placement& atom_placement = atom_placement_[atom];

        // The atom must be in a tile which can implement it. Atoms outside of
        // the device or in empty grid locations are not in a tile.
        if (atom_placement.is_off_device)
            estimate_.num_off_device_primitives++;
        bool is_in_compatible_tile_type = atom_placement.tile_root.is_valid()
                                          && kind_tile_compatible_[atom_kind_[atom]][device_grid_.get_physical_type(atom_placement.tile_root)->index];
        if (!is_in_compatible_tile_type) {
            evict_atom(atom, e_ap_legality_rule::INCOMPATIBLE_TILE);
            continue;
        }

        // The atom must be in its region (if it is constrained).
        const PartitionRegion* pr = get_atom_region(atom);
        if (pr != nullptr && !is_tile_in_region(atom_placement.tile_root, *pr))
            evict_atom(atom, e_ap_legality_rule::OUTSIDE_REGION);
    }
}

void LegalityEstimator::apply_chain_offset_rule(const t_chain& chain) {
    // For each atom, find the tile that the first molecule of the chain would
    // need to be in for the atom to be at its required offset (its anchor).
    // The atoms with the same anchor can stay where they are together; every
    // other atom must move. So the anchor with the most pins is kept, and the
    // atoms with any other anchor are evicted. Ties are broken in favour of
    // the anchor found first along the chain.
    std::map<t_physical_tile_loc, size_t> anchor_index;
    std::vector<size_t> anchor_pins;
    std::vector<std::pair<AtomBlockId, size_t>> chain_atom_anchors;
    for (size_t mol_idx = 0; mol_idx < chain.molecules.size(); mol_idx++) {
        int offset_mult = static_cast<int>(mol_idx);
        for (AtomBlockId atom : prepacker_.get_molecule(chain.molecules[mol_idx]).atom_block_ids) {
            // Evicted atoms must move anyway, so they do not decide where the
            // chain stays.
            if (!atom.is_valid() || atom_is_evicted_[atom])
                continue;
            const t_physical_tile_loc& tile_root = atom_placement_[atom].tile_root;
            t_physical_tile_loc anchor(tile_root.x - offset_mult * chain.dx,
                                       tile_root.y - offset_mult * chain.dy,
                                       tile_root.layer_num);
            auto [it, inserted] = anchor_index.emplace(anchor, anchor_pins.size());
            if (inserted)
                anchor_pins.push_back(0);
            anchor_pins[it->second] += atom_num_pins_[atom];
            chain_atom_anchors.emplace_back(atom, it->second);
        }
    }
    if (anchor_pins.size() <= 1)
        return;

    size_t kept_anchor = std::distance(anchor_pins.begin(), std::max_element(anchor_pins.begin(), anchor_pins.end()));
    for (const auto& [atom, anchor] : chain_atom_anchors) {
        if (anchor != kept_anchor)
            evict_atom(atom, e_ap_legality_rule::CHAIN_OFFSET);
    }
    estimate_.num_broken_chains++;
}

void LegalityEstimator::apply_control_pin_rule(t_tile_state& tile) {
    int tile_type_idx = tile.penalty.tile_type_index;
    const t_tile_pin_capacity& pin_capacity = tile_type_info_[tile_type_idx].pin_capacity;
    const std::array<e_tile_pin_class, 2> control_pin_classes = {e_tile_pin_class::CLOCK, e_tile_pin_class::GLOBAL};

    while (true) {
        // Find which classes of control pins are overused.
        std::unordered_map<AtomNetId, t_net_tile_usage> net_usage = get_net_tile_usage(tile);
        std::array<bool, NUM_TILE_PIN_CLASSES> is_overused{};
        bool any_overused = false;
        for (e_tile_pin_class pin_class : control_pin_classes) {
            int num_nets = 0;
            for (const auto& [net, usage] : net_usage)
                num_nets += is_tile_input(usage, pin_class);
            size_t class_idx = static_cast<size_t>(pin_class);
            is_overused[class_idx] = num_nets > pin_capacity.num_input_pins[class_idx];
            any_overused |= is_overused[class_idx];
        }
        if (!any_overused)
            return;

        // Group the remaining atoms by the overused control nets entering the
        // tile which they use. Atoms are visited in ID order, so the first
        // atom of each group has the smallest ID.
        std::map<std::pair<AtomNetId, size_t>, std::vector<AtomBlockId>> control_net_atoms;
        for (AtomBlockId atom : tile.atoms) {
            if (atom_is_evicted_[atom])
                continue;
            for (AtomPinId pin : atom_netlist_.block_pins(atom)) {
                AtomNetId net = atom_netlist_.pin_net(pin);
                if (!net.is_valid() || atom_netlist_.pin_type(pin) != PinType::SINK)
                    continue;
                size_t class_idx = static_cast<size_t>(get_sink_pin_class(pin, tile_type_idx));
                if (!is_overused[class_idx] || net_usage.at(net).has_driver)
                    continue;
                std::vector<AtomBlockId>& group = control_net_atoms[{net, class_idx}];
                if (group.empty() || group.back() != atom)
                    group.push_back(atom);
            }
        }

        // Evict the group with the fewest pins, with ties broken by the
        // smallest atom ID in the group.
        auto group_order = [&](const std::vector<AtomBlockId>& group) {
            int group_pins = 0;
            for (AtomBlockId atom : group)
                group_pins += atom_num_pins_[atom];
            return std::make_pair(group_pins, group.front());
        };
        VTR_ASSERT(!control_net_atoms.empty());
        auto group_to_evict = std::min_element(control_net_atoms.begin(), control_net_atoms.end(),
                                               [&](const auto& lhs, const auto& rhs) {
                                                   return group_order(lhs.second) < group_order(rhs.second);
                                               });
        for (AtomBlockId atom : group_to_evict->second) {
            // An atom may be in more than one group (e.g. a RAM with two
            // clocks), but groups are rebuilt after every eviction, so every
            // atom in this group is still in the tile.
            evict_atom(atom, e_ap_legality_rule::CONTROL_PINS);
        }
    }
}

void LegalityEstimator::apply_ram_sharing_rule(t_tile_state& tile) {
    int num_ram_blocks = tile_type_info_[tile.penalty.tile_type_index].num_ram_blocks;
    if (num_ram_blocks == 0)
        return;

    // Group the remaining RAM slices by their model and the nets on their
    // non-data ports. Only the slices within a group can share a RAM block.
    // Atoms are visited in ID order, so the groups are ordered by their
    // smallest atom ID.
    typedef std::pair<LogicalModelId, std::vector<AtomNetId>> t_ram_signature;
    std::map<t_ram_signature, size_t> group_of_signature;
    std::vector<std::vector<AtomBlockId>> groups;
    std::vector<int> group_pins;
    for (AtomBlockId atom : tile.atoms) {
        if (atom_is_evicted_[atom])
            continue;
        auto shared_ports_it = ram_model_shared_ports_.find(atom_netlist_.block_model(atom));
        if (shared_ports_it == ram_model_shared_ports_.end())
            continue;

        t_ram_signature signature;
        signature.first = atom_netlist_.block_model(atom);
        for (const auto& [model_port, num_pins] : shared_ports_it->second) {
            // Unconnected pins (and ports) must also match.
            AtomPortId port = atom_netlist_.find_atom_port(atom, model_port);
            for (int ipin = 0; ipin < num_pins; ipin++)
                signature.second.push_back(port.is_valid() ? atom_netlist_.port_net(port, ipin) : AtomNetId::INVALID());
        }

        auto [it, inserted] = group_of_signature.emplace(std::move(signature), groups.size());
        if (inserted) {
            groups.emplace_back();
            group_pins.push_back(0);
        }
        groups[it->second].push_back(atom);
        group_pins[it->second] += atom_num_pins_[atom];
    }
    if (groups.size() <= static_cast<size_t>(num_ram_blocks))
        return;

    // Keep the groups with the most pins (one per RAM block) and evict the
    // rest, i.e. the groups with the fewest pins are evicted. Ties are broken
    // by the smallest atom ID in the group.
    std::vector<size_t> group_order(groups.size());
    std::iota(group_order.begin(), group_order.end(), 0);
    std::stable_sort(group_order.begin(), group_order.end(), [&](size_t lhs, size_t rhs) {
        return group_pins[lhs] > group_pins[rhs];
    });
    for (size_t order_idx = num_ram_blocks; order_idx < group_order.size(); order_idx++) {
        for (AtomBlockId atom : groups[group_order[order_idx]])
            evict_atom(atom, e_ap_legality_rule::RAM_SHARING);
    }
}

void LegalityEstimator::apply_primitive_capacity_rule(t_tile_state& tile) {
    const t_slot_frontier& slot_frontier = tile_type_info_[tile.penalty.tile_type_index].slot_frontier;
    size_t num_dims = slot_frontier.dim_slot_types.size();

    // Group the remaining atoms into classes by which slots of the tile can
    // implement them. For example, with fracturable LUTs, a LUT6 can only use
    // the LUT6 slots while a LUT2 can use either the LUT6 or the LUT5 slots.
    std::map<vtr::vector<SlotDimId, bool>, size_t, t_vtr_vector_less> class_of_slots;
    std::vector<vtr::vector<SlotDimId, bool>> class_slots;
    std::vector<std::vector<AtomBlockId>> class_atoms;
    for (AtomBlockId atom : tile.atoms) {
        if (atom_is_evicted_[atom])
            continue;
        vtr::vector<SlotDimId, bool> slots(num_dims, false);
        for (SlotDimId dim : slot_frontier.dim_slot_types.keys())
            slots[dim] = kind_feasible_slots_[atom_kind_[atom]][slot_frontier.dim_slot_types[dim]];
        auto [it, inserted] = class_of_slots.emplace(std::move(slots), class_slots.size());
        if (inserted) {
            class_slots.push_back(it->first);
            class_atoms.emplace_back();
        }
        class_atoms[it->second].push_back(atom);
    }
    size_t num_classes = class_atoms.size();
    if (num_classes == 0)
        return;

    // Within each class, the atoms are evicted with the fewest pins first,
    // with ties broken by atom ID (the atoms are already sorted by ID).
    for (std::vector<AtomBlockId>& atoms : class_atoms) {
        std::stable_sort(atoms.begin(), atoms.end(), [&](AtomBlockId lhs, AtomBlockId rhs) {
            return atom_num_pins_[lhs] < atom_num_pins_[rhs];
        });
        // Reverse so the next atom to evict is at the back.
        std::reverse(atoms.begin(), atoms.end());
    }

    std::vector<int> class_demand(num_classes);
    for (size_t c = 0; c < num_classes; c++)
        class_demand[c] = class_atoms[c].size();

    // Split the classes into groups which do not share any dimension (e.g. the
    // classes of different models). For a given slot combination, the
    // deficiency of the tile is the sum of the deficiencies of the groups.
    std::vector<std::vector<size_t>> group_classes = get_independent_class_groups(class_slots);
    std::vector<size_t> class_group(num_classes);
    for (size_t g = 0; g < group_classes.size(); g++) {
        for (size_t c : group_classes[g])
            class_group[c] = g;
    }

    // The deficiency of each group, and of the whole tile, for each slot
    // combination of the tile's frontier. The overflow is the minimum number of
    // atoms which must be evicted for the atoms to fit within any combination.
    const std::vector<t_slot_combination>& combinations = slot_frontier.combinations;
    std::vector<std::vector<int>> combination_group_deficiency(combinations.size(), std::vector<int>(group_classes.size(), 0));
    std::vector<int> combination_deficiency(combinations.size(), 0);
    for (size_t ci = 0; ci < combinations.size(); ci++) {
        for (size_t g = 0; g < group_classes.size(); g++) {
            combination_group_deficiency[ci][g] = get_slot_deficiency(group_classes[g], class_demand, class_slots, combinations[ci]);
            combination_deficiency[ci] += combination_group_deficiency[ci][g];
        }
    }
    int overflow = *std::min_element(combination_deficiency.begin(), combination_deficiency.end());

    while (overflow > 0) {
        // Evict the atom with the fewest pins (ties broken by atom ID) whose
        // eviction reduces the number of atoms which do not fit. This is the
        // case if it reduces the deficiency of a combination with the minimum
        // deficiency.
        std::optional<size_t> evict_class;
        for (size_t c = 0; c < num_classes; c++) {
            if (class_atoms[c].empty())
                continue;
            AtomBlockId atom = class_atoms[c].back();
            if (evict_class.has_value()) {
                AtomBlockId best_atom = class_atoms[evict_class.value()].back();
                if (atom_num_pins_[atom] > atom_num_pins_[best_atom]
                    || (atom_num_pins_[atom] == atom_num_pins_[best_atom] && best_atom < atom))
                    continue;
            }
            size_t g = class_group[c];
            class_demand[c]--;
            bool reduces_overflow = false;
            for (size_t ci = 0; ci < combinations.size() && !reduces_overflow; ci++) {
                if (combination_deficiency[ci] != overflow)
                    continue;
                reduces_overflow = get_slot_deficiency(group_classes[g], class_demand, class_slots, combinations[ci]) < combination_group_deficiency[ci][g];
            }
            class_demand[c]++;
            if (reduces_overflow)
                evict_class = c;
        }
        // Since the overflow is the minimum number of atoms which must be
        // evicted, there is always an atom whose eviction reduces it.
        VTR_ASSERT(evict_class.has_value());

        size_t c = evict_class.value();
        evict_atom(class_atoms[c].back(), e_ap_legality_rule::PRIMITIVE_CAPACITY);
        class_atoms[c].pop_back();
        class_demand[c]--;

        // Only the deficiency of the evicted atom's group changes.
        size_t g = class_group[c];
        for (size_t ci = 0; ci < combinations.size(); ci++) {
            int group_deficiency = get_slot_deficiency(group_classes[g], class_demand, class_slots, combinations[ci]);
            combination_deficiency[ci] += group_deficiency - combination_group_deficiency[ci][g];
            combination_group_deficiency[ci][g] = group_deficiency;
        }
        overflow = *std::min_element(combination_deficiency.begin(), combination_deficiency.end());
    }
}

void LegalityEstimator::apply_tile_pin_rule(t_tile_state& tile) {
    int tile_type_idx = tile.penalty.tile_type_index;

    // Candidates are evicted with the fewest pins first, with ties broken by
    // atom ID. The atoms of the tile are already sorted by ID.
    std::vector<AtomBlockId> candidates;
    for (AtomBlockId atom : tile.atoms) {
        if (!atom_is_evicted_[atom])
            candidates.push_back(atom);
    }
    std::stable_sort(candidates.begin(), candidates.end(), [&](AtomBlockId lhs, AtomBlockId rhs) {
        return atom_num_pins_[lhs] < atom_num_pins_[rhs];
    });

    std::unordered_map<AtomNetId, t_net_tile_usage> net_usage = get_net_tile_usage(tile);
    t_tile_pin_demand demand = get_tile_pin_demand(net_usage);

    // The clock and global pins are checked by the control pin rule, so only
    // the regular input pins and the output pins are checked here.
    const t_tile_pin_capacity& pin_capacity = tile_type_info_[tile_type_idx].pin_capacity;
    size_t regular_idx = static_cast<size_t>(e_tile_pin_class::REGULAR);
    auto get_pin_overflow = [&](const t_tile_pin_demand& pin_demand) {
        return std::max(0, pin_demand.num_inputs[regular_idx] - pin_capacity.num_input_pins[regular_idx])
               + std::max(0, pin_demand.num_outputs - pin_capacity.num_output_pins);
    };

    while (get_pin_overflow(demand) > 0) {
        VTR_ASSERT_SAFE(!candidates.empty());
        // Evicting an atom does not always reduce the pin demand (for example,
        // evicting a FF driven by a LUT in the same tile makes the net between
        // them cross the tile boundary). So the candidate with the fewest pins
        // whose eviction reduces the overflow is evicted. If no single
        // eviction reduces the overflow, the candidate with the fewest pins is
        // evicted.
        size_t evict_idx = 0;
        for (size_t i = 0; i < candidates.size(); i++) {
            t_tile_pin_demand new_demand = demand;
            new_demand += get_eviction_pin_demand_delta(candidates[i], tile_type_idx, net_usage);
            if (get_pin_overflow(new_demand) < get_pin_overflow(demand)) {
                evict_idx = i;
                break;
            }
        }

        AtomBlockId atom = candidates[evict_idx];
        demand += get_eviction_pin_demand_delta(atom, tile_type_idx, net_usage);
        update_net_tile_usage(atom, tile_type_idx, -1, net_usage);
        evict_atom(atom, e_ap_legality_rule::TILE_PINS);
        candidates.erase(candidates.begin() + evict_idx);
    }
}

size_t LegalityEstimator::count_split_molecules() const {
    // Atoms not in a tile are identified by their grid location instead.
    auto get_atom_tile_key = [&](AtomBlockId atom) {
        const t_atom_placement& atom_placement = atom_placement_[atom];
        if (atom_placement.tile_root.is_valid())
            return atom_placement.tile_root;
        return atom_placement.grid_loc;
    };

    size_t num_split_molecules = 0;
    for (PackMoleculeId mol_id : prepacker_.molecules()) {
        AtomBlockId root_atom = prepacker_.get_molecule_root_atom(mol_id);
        t_physical_tile_loc root_key = get_atom_tile_key(root_atom);
        for (AtomBlockId atom : prepacker_.get_molecule(mol_id).atom_block_ids) {
            if (atom.is_valid() && get_atom_tile_key(atom) != root_key) {
                num_split_molecules++;
                break;
            }
        }
    }
    return num_split_molecules;
}

void LegalityEstimator::evict_atom(AtomBlockId atom, e_ap_legality_rule rule) {
    VTR_ASSERT_SAFE(!atom_is_evicted_[atom]);
    atom_is_evicted_[atom] = true;

    if (!kind_has_tile_on_device_[atom_kind_[atom]])
        estimate_.num_primitives_without_compatible_tile++;

    size_t num_pins = atom_num_pins_[atom];
    size_t rule_idx = static_cast<size_t>(rule);
    estimate_.pins_in_violation += num_pins;
    estimate_.rule_pins_in_violation[rule_idx] += num_pins;

    // Charge the pins to the tile type and tile the atom was placed in.
    const t_atom_placement& atom_placement = atom_placement_[atom];
    if (atom_placement.is_off_device) {
        estimate_.off_device_pins_in_violation += num_pins;
    } else {
        int tile_type_idx = device_grid_.get_physical_type(atom_placement.grid_loc)->index;
        estimate_.tile_type_pins_in_violation[tile_type_idx] += num_pins;
    }
    if (atom_placement.tile_root.is_valid()) {
        t_ap_tile_legality_penalty& tile_penalty = tiles_.at(atom_placement.tile_root).penalty;
        tile_penalty.pins_in_violation += num_pins;
        tile_penalty.rule_pins_in_violation[rule_idx] += num_pins;
    }
}

const PartitionRegion* LegalityEstimator::get_atom_region(AtomBlockId atom) const {
    PartitionId part_id = constraints_.get_atom_partition(atom);
    if (!part_id.is_valid())
        return nullptr;
    return &constraints_.get_partition_pr(part_id);
}

e_tile_pin_class LegalityEstimator::get_sink_pin_class(AtomPinId sink_pin, int tile_type_idx) const {
    const auto& port_pin_class = tile_type_info_[tile_type_idx].port_pin_class;
    auto it = port_pin_class.find(atom_netlist_.port_model(atom_netlist_.pin_port(sink_pin)));
    if (it == port_pin_class.end())
        return e_tile_pin_class::REGULAR;
    return it->second;
}

void LegalityEstimator::update_net_tile_usage(AtomBlockId atom,
                                              int tile_type_idx,
                                              int sign,
                                              std::unordered_map<AtomNetId, t_net_tile_usage>& net_usage) const {
    // Every net is counted (including global and constant nets), since the
    // cluster legalizer routes every net which enters or leaves a cluster
    // through the pins of the cluster.
    for (AtomPinId pin : atom_netlist_.block_pins(atom)) {
        AtomNetId net = atom_netlist_.pin_net(pin);
        if (!net.is_valid())
            continue;
        t_net_tile_usage& usage = net_usage[net];
        if (atom_netlist_.pin_type(pin) == PinType::DRIVER) {
            usage.has_driver = (sign > 0);
            continue;
        }
        usage.num_sinks += sign;
        usage.num_class_sinks[static_cast<size_t>(get_sink_pin_class(pin, tile_type_idx))] += sign;
    }
}

std::unordered_map<AtomNetId, t_net_tile_usage> LegalityEstimator::get_net_tile_usage(const t_tile_state& tile,
                                                                                      bool include_evicted) const {
    std::unordered_map<AtomNetId, t_net_tile_usage> net_usage;
    for (AtomBlockId atom : tile.atoms) {
        if (include_evicted || !atom_is_evicted_[atom])
            update_net_tile_usage(atom, tile.penalty.tile_type_index, 1, net_usage);
    }
    return net_usage;
}

t_tile_pin_demand LegalityEstimator::get_tile_pin_demand(const std::unordered_map<AtomNetId, t_net_tile_usage>& net_usage) const {
    t_tile_pin_demand demand;
    for (const auto& [net, usage] : net_usage) {
        for (size_t pin_class = 0; pin_class < NUM_TILE_PIN_CLASSES; pin_class++)
            demand.num_inputs[pin_class] += is_tile_input(usage, static_cast<e_tile_pin_class>(pin_class));
        demand.num_outputs += is_tile_output(net, usage);
    }
    return demand;
}

bool LegalityEstimator::is_tile_input(const t_net_tile_usage& usage, e_tile_pin_class pin_class) {
    return !usage.has_driver && usage.num_class_sinks[static_cast<size_t>(pin_class)] > 0;
}

bool LegalityEstimator::is_tile_output(AtomNetId net, const t_net_tile_usage& usage) const {
    return usage.has_driver && static_cast<int>(atom_netlist_.net_sinks(net).size()) > usage.num_sinks;
}

t_tile_pin_demand LegalityEstimator::get_eviction_pin_demand_delta(AtomBlockId atom,
                                                                   int tile_type_idx,
                                                                   const std::unordered_map<AtomNetId, t_net_tile_usage>& net_usage) const {
    // Compute the usage of the nets of this atom as if it were evicted.
    std::unordered_map<AtomNetId, t_net_tile_usage> new_net_usage;
    for (AtomPinId pin : atom_netlist_.block_pins(atom)) {
        AtomNetId net = atom_netlist_.pin_net(pin);
        if (net.is_valid())
            new_net_usage.emplace(net, net_usage.at(net));
    }
    update_net_tile_usage(atom, tile_type_idx, -1, new_net_usage);

    t_tile_pin_demand delta;
    for (const auto& [net, new_usage] : new_net_usage) {
        const t_net_tile_usage& usage = net_usage.at(net);
        for (size_t pin_class = 0; pin_class < NUM_TILE_PIN_CLASSES; pin_class++) {
            e_tile_pin_class cls = static_cast<e_tile_pin_class>(pin_class);
            delta.num_inputs[pin_class] += static_cast<int>(is_tile_input(new_usage, cls)) - static_cast<int>(is_tile_input(usage, cls));
        }
        delta.num_outputs += static_cast<int>(is_tile_output(net, new_usage)) - static_cast<int>(is_tile_output(net, usage));
    }
    return delta;
}

void LegalityEstimator::write_legality_estimate_echo(const std::string& filename) const {
    std::ofstream os(filename);
    if (!os) {
        VTR_LOG_WARN("Unable to open legality estimate echo file '%s' for writing.\n", filename.c_str());
        return;
    }

    os << "# Estimated legality penalty of each occupied tile.\n";
    os << "#   x/y/layer:    Root location of the tile.\n";
    os << "#   num_prims:    Number of primitives placed in the tile.\n";
    os << "#   num_pins:     Number of connected pins of the primitives placed in the tile.\n";
    os << "#   pins_in_viol: Number of connected pins of the primitives evicted from the tile.\n";
    os << "#   rule_*:       Pins in violation charged to each rule.\n";
    os << "#   in_*/out:     Number of tile pins of each class used by the nets entering / leaving the tile,\n";
    os << "#                 counting every primitive placed in the tile (before any evictions). Every net\n";
    os << "#                 is counted, including global and constant nets, since the cluster legalizer\n";
    os << "#                 routes them through the tile pins.\n";
    os << "#                   regular: Non-global input pins (checked by rule_tile_pins).\n";
    os << "#                   global:  Non-clock global input pins (checked by rule_control_pins).\n";
    os << "#                   clock:   Clock pins (checked by rule_control_pins).\n";
    os << "#                   out:     Output pins (checked by rule_tile_pins).\n";
    os << "#   *_cap:        Number of tile pins of each class.\n";
    os << "x y layer num_prims num_pins pins_in_viol";
    for (size_t rule_idx = 0; rule_idx < NUM_AP_LEGALITY_RULES; rule_idx++)
        os << " rule_" << ap_legality_rule_name(static_cast<e_ap_legality_rule>(rule_idx));
    os << " in_regular in_regular_cap in_global in_global_cap in_clock in_clock_cap out out_cap";
    os << " tile_type\n";

    for (const auto& [tile_root, tile] : tiles_) {
        int num_pins = 0;
        for (AtomBlockId atom : tile.atoms)
            num_pins += atom_num_pins_[atom];
        os << tile_root.x << " "
           << tile_root.y << " "
           << tile_root.layer_num << " "
           << tile.atoms.size() << " "
           << num_pins << " "
           << tile.penalty.pins_in_violation;
        for (size_t rule_pins_in_violation : tile.penalty.rule_pins_in_violation)
            os << " " << rule_pins_in_violation;

        t_tile_pin_demand demand = get_tile_pin_demand(get_net_tile_usage(tile, true));
        const t_tile_pin_capacity& pin_capacity = tile_type_info_[tile.penalty.tile_type_index].pin_capacity;
        for (e_tile_pin_class pin_class : {e_tile_pin_class::REGULAR, e_tile_pin_class::GLOBAL, e_tile_pin_class::CLOCK}) {
            size_t class_idx = static_cast<size_t>(pin_class);
            os << " " << demand.num_inputs[class_idx] << " " << pin_capacity.num_input_pins[class_idx];
        }
        os << " " << demand.num_outputs << " " << pin_capacity.num_output_pins;

        os << " " << device_grid_.get_physical_type(tile_root)->name << "\n";
    }
}

t_ap_legality_estimate estimate_legality(const PartialPlacement& p_placement,
                                         const APNetlist& ap_netlist,
                                         const AtomNetlist& atom_netlist,
                                         const Prepacker& prepacker,
                                         const DeviceGrid& device_grid,
                                         const std::vector<t_physical_tile_type>& physical_tile_types,
                                         const std::vector<t_direct_inf>& directs,
                                         const UserPlaceConstraints& constraints) {
    vtr::ScopedStartFinishTimer timer("Legality Estimation");
    LegalityEstimator estimator(p_placement,
                                ap_netlist,
                                atom_netlist,
                                prepacker,
                                device_grid,
                                physical_tile_types,
                                directs,
                                constraints);
    return estimator.run();
}

void print_legality_summary(const t_ap_legality_estimate& estimate,
                            const std::vector<t_physical_tile_type>& physical_tile_types) {
    VTR_LOG("Legality Estimate Summary:\n");
    VTR_LOG("\tPins in violation: %zu (%f of %zu connected pins)\n",
            estimate.pins_in_violation,
            estimate.normalized_pins_in_violation(),
            estimate.num_connected_pins);

    VTR_LOG("\tPer rule (pins in violation):\n");
    for (size_t rule_idx = 0; rule_idx < NUM_AP_LEGALITY_RULES; rule_idx++) {
        VTR_LOG("\t\tRule %s: %zu\n",
                ap_legality_rule_name(static_cast<e_ap_legality_rule>(rule_idx)),
                estimate.rule_pins_in_violation[rule_idx]);
    }

    // Every pin in violation is charged to a tile type or to off-device, so
    // this list is only empty when nothing is in violation.
    if (estimate.pins_in_violation != 0)
        VTR_LOG("\tPer tile type (pins in violation):\n");
    for (const t_physical_tile_type& tile_type : physical_tile_types) {
        size_t pins_in_violation = estimate.tile_type_pins_in_violation[tile_type.index];
        if (pins_in_violation == 0)
            continue;
        VTR_LOG("\t\t%s: %zu\n", tile_type.name.c_str(), pins_in_violation);
    }
    if (estimate.off_device_pins_in_violation != 0)
        VTR_LOG("\t\t<off-device>: %zu\n", estimate.off_device_pins_in_violation);

    if (!estimate.worst_tiles.empty())
        VTR_LOG("\tWorst %zu tiles (pins in violation per rule):\n", estimate.worst_tiles.size());
    for (const t_ap_tile_legality_penalty& tile_penalty : estimate.worst_tiles) {
        const t_physical_tile_loc& loc = tile_penalty.root_loc;
        VTR_LOG("\t\t%s at (%d, %d, %d): %zu pins (",
                physical_tile_types[tile_penalty.tile_type_index].name.c_str(),
                loc.x, loc.y, loc.layer_num,
                tile_penalty.pins_in_violation);
        bool is_first = true;
        for (size_t rule_idx = 0; rule_idx < NUM_AP_LEGALITY_RULES; rule_idx++) {
            if (tile_penalty.rule_pins_in_violation[rule_idx] == 0)
                continue;
            VTR_LOG("%s%s: %zu",
                    is_first ? "" : ", ",
                    ap_legality_rule_name(static_cast<e_ap_legality_rule>(rule_idx)),
                    tile_penalty.rule_pins_in_violation[rule_idx]);
            is_first = false;
        }
        VTR_LOG(")\n");
    }

    VTR_LOG("\tChains with a fixed offset: %zu (%zu broken)\n", estimate.num_chains, estimate.num_broken_chains);
    VTR_LOG("\tPrimitives placed outside of the device: %zu\n", estimate.num_off_device_primitives);
    VTR_LOG("\tEvicted primitives with no compatible tile: %zu\n", estimate.num_primitives_without_compatible_tile);
    VTR_LOG("\tMolecules split across tiles: %zu\n", estimate.num_split_molecules);
}
