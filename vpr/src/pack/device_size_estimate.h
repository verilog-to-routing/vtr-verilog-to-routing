#pragma once
/**
 * @file
 * @author  Haydar Cakan
 * @date    March 2026
 * @brief   Estimates the device size required to fit a design before packing
 *          begins, so that an appropriate FPGA grid can be selected and 
 *          optimizations before packing (e.g. analytical placement, ram mapping)
 *          can be done when the device layout is set to "auto".
 */

#include <map>
#include <vector>

#include "logical_ram_infer.h"

// Forward declarations
class Prepacker;
struct t_arch;
struct t_vpr_setup;

/**
 * @brief Estimates the device size required to fit the design before packing.
 *
 * When the device layout is set to "auto", this class estimates the number of
 * instances needed for each logical block type and selects the smallest device
 * grid that can accommodate them. For fixed device layouts, the grid is created
 * directly without estimation.
 *
 * Logical RAM groups (sibling-feasible memory atoms) computed during estimation
 * are exposed via ram_groups() so that RamMapper can reuse them and skip redundant work.
 */
class DeviceSizeEstimator {
  public:
    /**
     * @param always_estimate_resource_requirement
     *      If true, the resource requirement estimate (see
     *      estimated_type_instance_counts()) is computed even when the
     *      device size itself does not need estimating (fixed device layout,
     *      RR graph provided, or auto layout with a fixed width). Defaults to
     *      false since this estimate involves a mini-packing simulation, and
     *      most callers only need the device grid sized, not the raw
     *      per-type estimate.
     */
    DeviceSizeEstimator(t_vpr_setup& vpr_setup,
                        const t_arch& arch,
                        const Prepacker& prepacker,
                        bool always_estimate_resource_requirement = false);

    /// @brief Returns the RAM groups computed during estimation.
    ///        Empty if the device layout was fixed (no estimation needed).
    const vtr::vector<LogicalRamGroupId, LogicalRamGroup>& ram_groups() const {
        return ram_groups_;
    }

    /// @brief Returns the estimated number of instances required for each
    ///        logical block type, computed during device size estimation.
    ///        Empty unless estimation was performed (device layout was
    ///        "auto" with no fixed width, or always_estimate_resource_requirement
    ///        was set to true in the constructor).
    const std::map<t_logical_block_type_ptr, size_t>& estimated_type_instance_counts() const {
        return estimated_num_type_instances_;
    }

  private:
    /**
     * @brief Estimates the number of instances required for each logical block
     *        type to fit the design.
     *
     * Groups RAM atoms, assigns them to minimum-area types, and runs a
     * mini-packing simulation for non-RAM types using pin capacity and
     * cluster placement feasibility constraints to get required number of
     * instances for each type.
     *
     * @param prepacker
     *      Used to query molecule structure and placement feasibility.
     * @param store_ram_groups
     *      If true, the RAM groups computed as part of this estimate are
     *      published to ram_groups_ for reuse by RamMapper. Callers that
     *      only want the type instance estimate (e.g. for a fixed device,
     *      where RamMapper must instead do its own capacity-aware grouping)
     *      should pass false so ram_groups() is left untouched.
     * @return Map from logical block type to estimated cluster instance count.
     */
    std::map<t_logical_block_type_ptr, size_t> estimate_resource_requirement(
        const Prepacker& prepacker,
        bool store_ram_groups = true);

    /// @brief RAM groups computed during estimation; exposed via ram_groups()
    ///        for reuse by RamMapper to avoid redundant grouping and area assignment.
    vtr::vector<LogicalRamGroupId, LogicalRamGroup> ram_groups_;

    /// @brief Estimated cluster instance counts per logical block type;
    ///        exposed via estimated_type_instance_counts().
    std::map<t_logical_block_type_ptr, size_t> estimated_num_type_instances_;
};
