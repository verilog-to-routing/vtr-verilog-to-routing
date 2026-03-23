#pragma once
/**
 * @file
 * @author  Haydar Cakan
 * @date    March 2026
 * @brief   Logical RAM inference: groups RAM atoms into sibling-feasible
 *          equivalence classes and assigns each group to a physical
 *          RAM type by minimizing area and then tries to improve timing
 *          of the most critical groups.
 *
 *          The logical RAM inference groups the RAM atom and pre-assigns a
 *          physical type to that logical RAM. However, which atoms of that
 *          logical RAM to be clustered together inside an instance of a physical
 *          RAM is not determined in that step. The next step of physical RAM
 *          inference determined that.
 *
 *          Physical RAM inference: given a logical RAM atoms, determines which
 *          atoms of that logical RAM to be clustered together inside a physical
 *          RAM instance of pre-assigned type. The assignmend is aware of the
 *          capacity of the pre-assigned physical RAM type.
 *
 * For example, a 128-bit wide RAM will generally have been cut into
 * 128 one-bit wide atoms which all have the same address and control signals.
 * This module determines those 128 atoms form a single 'logical RAM group'.
 *
 * If we pre-assign that logical RAM to a physical RAM type that can support at
 * most 64-bit wide mode, we need two instance of that physical RAM. Assigning
 * which 64 atoms to be assigned together to a physical RAM is called 'physical
 * ram mapping' and each created group is called a 'physical RAM group'.
 */

#include <string>
#include <unordered_map>
#include <vector>

#include "atom_netlist_fwd.h"
#include "physical_types.h"
#include "PreClusterTimingManager.h"
#include "prepack.h"
#include "vtr_range.h"
#include "vtr_strong_id.h"
#include "vtr_vector.h"

// Forward declarations
class AtomNetlist;

/// @brief Unique identifier for a logical RAM group.
typedef vtr::StrongId<struct logical_ram_group_id_tag, size_t> LogicalRamGroupId;

/// @brief Unique identifier for a physical RAM group.
typedef vtr::StrongId<struct physical_ram_group_id_tag, size_t> PhysicalRamGroupId;

/**
 * @brief Represents a group of RAM atom blocks that share the same non-data
 *        port configuration (sibling-feasible). All atoms in a group can be
 *        implemented by the same physical RAM block type.
 */
struct LogicalRamGroup {
    /// @brief Representative atom used for sibling-feasibility checks.
    AtomBlockId rep_blk;
    /// @brief Primitive pb_type of the representative atom.
    const t_pb_type* rep_pb_type = nullptr;
    /// @brief All atoms in this group.
    std::vector<AtomBlockId> atoms;
    /// @brief Physical block types that can implement this group.
    ///        Sorted by area cost (ascending) after assign_ram_groups_by_min_area().
    std::vector<t_logical_block_type_ptr> candidate_types;
    /// @brief Number of primitives per physical instance for each candidate type.
    std::unordered_map<t_logical_block_type_ptr, int> candidate_capacity;
    /// @brief Map from each candidate type to its total area cost,
    ///        computed as ceil(slices / capacity) × tile_area.
    ///        Populated by assign_ram_groups_by_min_area().
    std::unordered_map<t_logical_block_type_ptr, int> candidate_area_cost;
    /// @brief Total number of RAM slices in the group.
    int total_memory_slices = 0;
    /// @brief Preferred physical RAM block type for this group, selected by
    ///        area/timing-driven assignment. The clusterer will attempt to use
    ///        this type, but may choose a different one if resources are unavailable.
    t_logical_block_type_ptr pre_assigned_type = nullptr;
    /// @brief True if the RAM output is registered.
    bool is_output_registered = false;
    /// @brief Maximum setup timing criticality among atoms in this group.
    float max_atom_criticality = 0.0f;
};

/**
 * @brief Represents a group of atoms that map to a single physical RAM tile.
 *
 * A LogicalRamGroup is partitioned into one or more PhysicalRamGroups based
 * on the capacity of the pre-assigned physical tile type. Each PhysicalRamGroup
 * holds the subset of atoms that fit within one tile without exceeding its
 * memory slice capacity.
 */
struct PhysicalRamGroup {
    /// The logical RAM group this physical group was derived from.
    LogicalRamGroupId logical_ram_group_id;
    /// Atoms assigned to this physical RAM tile.
    std::vector<AtomBlockId> atoms;
    /// Molecules corresponding to the atoms in this physical RAM tile.
    /// Populated by create_physical_ram_groups() for use in AP block construction.
    std::vector<PackMoleculeId> molecules;
    /// Total memory slices consumed by the atoms in this group.
    int total_memory_slices = 0;
};

/**
 * @brief Groups RAM atoms from the netlist into sibling-feasible equivalence
 *        classes and initializes per-type capacities for each group.
 *
 * Two atoms are sibling-feasible if their non-data ports match bit for bit,
 * meaning they can be packed into the same physical RAM block.
 *
 * @param atom_nlist  Used to iterate over all atom blocks and identify RAM primitives.
 * @param prepacker   Used to query each atom's expected primitive mapping and to
 *                    compute per type capacities for each group.
 * @return            A vector of groups indexed by LogicalRamGroupId, each with
 *                    candidate_types and candidate_capacity populated.
 */
vtr::vector<LogicalRamGroupId, LogicalRamGroup>
group_ram_atoms(const AtomNetlist& atom_nlist, const Prepacker& prepacker);

/**
 * @brief Assigns each group's pre_assigned_type to the minimum area cost candidate type.
 *
 * No device-grid feasibility check is performed; call
 * check_assigned_rams_fit_on_device() afterwards if needed.
 *
 * @param groups           RAM groups whose candidate_types will be sorted by area cost
 *                         and whose pre_assigned_type will be set to the minimum area choice.
 * @param is_fixed_device  When true, respects device capacity limits during assignment.
 *                         Groups that would overflow their minimum-area type are spilled
 *                         to the next smallest area cost candidate type that is not overflowing.
 */
void assign_ram_groups_by_min_area(vtr::vector<LogicalRamGroupId, LogicalRamGroup>& groups,
                                   bool is_fixed_device = false);

/**
 * @brief Infers logical RAM groups from the atom netlist and assigns each
 *        group to a physical RAM type, first minimizing area and then
 *        considering timing.
 *
 * In the auto-device flow, the area assignment is already performed by
 * DeviceSizeEstimator. The pre-computed groups can be passed directly to
 * skip the area assignment and only run the timing pass.
 */
class RamMapper {
  public:
    /**
     * @brief Constructs a RamMapper.
     *
     * If precomputed_groups is non-empty (typically from DeviceSizeEstimator
     * in the auto-device flow), the area assignment is skipped and only the
     * timing pass is performed. Otherwise, group_ram_atoms() followed by
     * assign_ram_groups_by_min_area() are called internally before the timing pass.
     *
     * @param atom_nlist          Used to iterate atoms during grouping and to evaluate
     *                            timing criticality per atom in the timing pass.
     * @param prepacker           Used to query primitive mappings and per type capacities
     *                            during RAM grouping.
     * @param timing_manager      Used to compute setup timing criticality per atom,
     *                            driving the timing based remap decisions.
     * @param precomputed_groups  RAM groups previously computed by DeviceSizeEstimator;
     *                            if non-empty, grouping and area assignment are skipped.
     * @param verbosity           The verbosity level used to determine logging option for
     *                            remappings in timing pass.
     * @param is_fixed_device     When true, area assignment respects device capacity limits.
     */
    RamMapper(const AtomNetlist& atom_nlist,
              const Prepacker& prepacker,
              const PreClusterTimingManager& timing_manager,
              const vtr::vector<LogicalRamGroupId, LogicalRamGroup>& precomputed_groups,
              int verbosity = 0,
              bool is_fixed_device = false);


    RamMapper() = default;
    RamMapper(RamMapper&&) = default;
    RamMapper& operator=(RamMapper&&) = default;

    /// @brief Returns the logical group ID for an atom, or INVALID if not a RAM atom.
    LogicalRamGroupId group_id_of(AtomBlockId blk) const;

    /// @brief Returns the physical group ID for an atom, or INVALID if not a RAM atom.
    PhysicalRamGroupId physical_group_id_of(AtomBlockId blk) const;

    /// @brief Returns a const reference to the group for a given ID.
    const LogicalRamGroup& group(LogicalRamGroupId id) const;

    /// @brief Returns the number of logical RAM groups.
    size_t num_groups() const { return logical_ram_groups_.size(); }

    typedef typename vtr::vector<PhysicalRamGroupId, PhysicalRamGroup>::const_iterator physical_ram_group_iterator;
    typedef typename vtr::Range<physical_ram_group_iterator> physical_ram_group_range;

    /// @brief Returns the range of all physical RAM group IDs for iteration.
    physical_ram_group_range physical_ram_groups() const {
        return vtr::make_range(physical_ram_groups_.begin(), physical_ram_groups_.end());
    }

    /// @brief Returns the physical RAM group for a given ID.
    const PhysicalRamGroup& physical_ram_group(PhysicalRamGroupId id) const {
        VTR_ASSERT(id.is_valid() && size_t(id) < physical_ram_groups_.size());
        return physical_ram_groups_[id];
    }

  private:
    /// @brief Builds the atom-to-logical-group lookup map.
    void build_atom_to_logical_group_map(const AtomNetlist& atom_nlist);

    /// @brief Builds the atom-to-physical-group lookup map from physical_ram_groups_.
    void build_atom_to_physical_group_map();

    /// @brief Partitions logical RAM groups into physical RAM groups.
    /// TODO:  Currently we are assigning the atoms to physical ram groups
    ///        by iterating over their order in the logical ram group. We should
    ///        evaluate that decision and investigate mimicking the greedy packer here.
    vtr::vector<PhysicalRamGroupId, PhysicalRamGroup> create_physical_ram_groups(const Prepacker& prepacker) const;

    /// @brief Tries to remap timing-critical groups to the smallest (believed to be fastest) RAM type.
    void timing_pass(const AtomNetlist& atom_nlist,
                     const PreClusterTimingManager& timing_manager);

    /// @brief Checks the integrity of the current RAM type assignment.
    ///        Issues a fatal error if any group has no candidate types or no
    ///        assigned type. Warns if any type exceeds the available device capacity.
    void check_assigned_rams_fit_on_device() const;

    /// @brief Logs device utilization per RAM type with a given label.
    void log_utilizations(const std::string& label) const;

    /// @brief All logical RAM groups, indexed by LogicalRamGroupId.
    vtr::vector<LogicalRamGroupId, LogicalRamGroup> logical_ram_groups_;

    /// @brief Physical RAM groups derived from logical groups, indexed by PhysicalRamGroupId.
    vtr::vector<PhysicalRamGroupId, PhysicalRamGroup> physical_ram_groups_;

    /// @brief Maps each RAM atom to its logical group ID for fast lookup during packing.
    vtr::vector<AtomBlockId, LogicalRamGroupId> atom_to_group_;

    /// @brief Maps each RAM atom to its physical group ID for fast lookup during packing.
    vtr::vector<AtomBlockId, PhysicalRamGroupId> atom_to_physical_group_;

    /// @brief Packing verbosity level.
    int log_verbosity_ = 0;
};
