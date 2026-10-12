#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    October 2026
 * @brief   Methods for estimating the legality of a global placement solution.
 *
 * The legality estimate is a measure of how much a flat placement must change
 * in order to become legal (i.e. a lower bound on the work that the full
 * legalizer must do). It is intentionally independent of the mass abstraction
 * used by the partial legalizer and of every full legalizer (including the
 * cluster legalizer), so that it can be used to verify them.
 *
 * All penalties are in units of connected primitive pins, so that the
 * different sources of illegality (legality rules) can be fairly compared.
 * Two numbers are reported:
 *  - Pins in violation: the number of connected primitive pins which must move
 *    for the placement to become legal.
 *  - Pin-weighted displacement: the sum, over the primitives which must move,
 *    of their number of connected pins times the Manhattan distance (in tiles)
 *    to the nearest location they could legally move to.
 */

#include <array>
#include <cstddef>
#include <string>
#include <vector>
#include "physical_types.h"

struct PartialPlacement;
class APNetlist;
class AtomNetlist;
class DeviceGrid;
class Prepacker;
class UserPlaceConstraints;

/**
 * @brief The legality rules which are checked by the legality estimator.
 *
 * The rules are applied sequentially in the order they are listed here. Each
 * rule only considers the primitives which have not already been evicted by
 * an earlier rule, so each evicted primitive is charged to exactly one rule.
 *
 * Mode conflicts (primitives which need conflicting modes of the tile, e.g. a
 * fracturable LUT used as a LUT6 next to two LUT5s) are checked by
 * PRIMITIVE_CAPACITY, which counts the primitive slots of every model together.
 *
 * TODO: Chain offsets and tile-specific rules (e.g. RAM address conflicts) are
 *       not checked yet.
 */
enum class e_ap_legality_rule : size_t {
    INCOMPATIBLE_TILE,  ///< The primitive is in a tile which cannot implement it (or is not in a tile at all).
    OUTSIDE_REGION,     ///< The primitive is outside of its floorplanning region.
    CONTROL_PINS,       ///< More clock / global nets enter the tile than it has clock / global pins.
    PRIMITIVE_CAPACITY, ///< The tile cannot use enough primitive slots at the same time (in any of its modes) for the primitives placed in it.
    TILE_PINS,          ///< More nets enter / leave the tile than it has (non-global) input / output pins.
    NUM_RULES
};

/// @brief The number of legality rules checked by the legality estimator.
constexpr size_t NUM_AP_LEGALITY_RULES = static_cast<size_t>(e_ap_legality_rule::NUM_RULES);

/**
 * @brief Get a human-readable name of the given legality rule.
 */
const char* ap_legality_rule_name(e_ap_legality_rule rule);

/**
 * @brief A legality penalty, in units of connected primitive pins.
 */
struct t_ap_legality_penalty {
    /// @brief The number of connected primitive pins which must move.
    size_t pins_in_violation = 0;
    /// @brief The sum, over the primitives which must move, of their number of
    ///        connected pins times the distance (in tiles) they must move.
    double pin_weighted_displacement = 0.0;

    /// @brief Add the given penalty to this penalty.
    t_ap_legality_penalty& operator+=(const t_ap_legality_penalty& rhs) {
        pins_in_violation += rhs.pins_in_violation;
        pin_weighted_displacement += rhs.pin_weighted_displacement;
        return *this;
    }
};

/**
 * @brief The legality penalty of a single tile, broken down by rule.
 */
struct t_ap_tile_legality_penalty {
    /// @brief The root location of the tile.
    t_physical_tile_loc root_loc;
    /// @brief The index of the physical tile type of the tile.
    int tile_type_index = -1;
    /// @brief The total penalty of the primitives evicted from this tile.
    t_ap_legality_penalty total;
    /// @brief The penalty of the primitives evicted from this tile, per rule.
    std::array<t_ap_legality_penalty, NUM_AP_LEGALITY_RULES> rule_penalty;
};

/**
 * @brief The estimated legality of a flat placement, along with the breakdown
 *        used to build the legality summary.
 */
struct t_ap_legality_estimate {
    /// @brief The total legality penalty of the placement.
    t_ap_legality_penalty total;
    /// @brief The total number of connected primitive pins in the netlist.
    ///        Used to normalize the penalty.
    size_t num_connected_pins = 0;
    /// @brief The legality penalty per rule.
    std::array<t_ap_legality_penalty, NUM_AP_LEGALITY_RULES> rule_penalty;
    /// @brief The legality penalty per physical tile type (indexed by the
    ///        physical tile type index), charged to the type of the tile that
    ///        each evicted primitive was placed in. Primitives placed in empty
    ///        grid locations are charged to the empty tile type.
    std::vector<t_ap_legality_penalty> tile_type_penalty;
    /// @brief The legality penalty of primitives placed outside of the device.
    t_ap_legality_penalty off_device_penalty;
    /// @brief The tiles with the largest number of pins in violation, sorted
    ///        from worst to best. Only tiles with violations are included.
    std::vector<t_ap_tile_legality_penalty> worst_tiles;
    /// @brief The number of primitives placed outside of the device.
    size_t num_off_device_primitives = 0;
    /// @brief The number of evicted primitives which had no tile that they
    ///        could legally move to.
    size_t num_primitives_without_compatible_tile = 0;
    /// @brief The number of molecules whose atoms are not all placed within
    ///        the same tile. This should always be zero, and it is not part of
    ///        the score.
    size_t num_split_molecules = 0;

    /// @brief The pins in violation, normalized by the number of connected pins.
    double normalized_pins_in_violation() const;
    /// @brief The pin-weighted displacement, normalized by the number of
    ///        connected pins.
    double normalized_pin_weighted_displacement() const;
};

/**
 * @brief Estimate the legality of the given flat placement.
 *
 * Each atom is placed at the location of the AP block which contains it, and
 * is assigned to the physical tile whose bounding box contains it. Then the
 * legality rules (see e_ap_legality_rule) are applied in order. Each rule
 * evicts the primitives which must move for the placement to become legal,
 * charging their connected pins (and their pin-weighted displacement) to the
 * rule.
 *
 * If the legality estimate echo file is enabled, the penalty of every occupied
 * tile is written to it. This can be compared tile-by-tile against the mass
 * overflow of the partial legalizer.
 *
 *  @param p_placement          The flat placement to estimate the legality of.
 *  @param ap_netlist           The AP netlist that the placement is over.
 *  @param atom_netlist         The atom netlist (the ground truth of the netlist).
 *  @param prepacker            The prepacker which holds the molecules.
 *  @param device_grid          The device grid that the placement is over.
 *  @param physical_tile_types  The physical tile types of the device.
 *  @param constraints          The user floorplanning constraints.
 *
 *  @return The estimated legality of the placement.
 */
t_ap_legality_estimate estimate_legality(const PartialPlacement& p_placement,
                                         const APNetlist& ap_netlist,
                                         const AtomNetlist& atom_netlist,
                                         const Prepacker& prepacker,
                                         const DeviceGrid& device_grid,
                                         const std::vector<t_physical_tile_type>& physical_tile_types,
                                         const UserPlaceConstraints& constraints);

/**
 * @brief Print the legality summary of the given legality estimate.
 *
 *  @param estimate             The legality estimate to print.
 *  @param physical_tile_types  The physical tile types of the device. Used to
 *                              name the tile types in the summary.
 */
void print_legality_summary(const t_ap_legality_estimate& estimate,
                            const std::vector<t_physical_tile_type>& physical_tile_types);
