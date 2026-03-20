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
 * Logical RAM groups (siblif-feasible memory atoms) computed during estimation
 * are exposed via ram_groups() so that RamMapper can reuse them and skip redundant work.
 */
class DeviceSizeEstimator {
  public:
    DeviceSizeEstimator(t_vpr_setup& vpr_setup,
                        const t_arch& arch,
                        const Prepacker& prepacker);

    /// @brief Returns the RAM groups computed during estimation.
    ///        Empty if the device layout was fixed (no estimation needed).
    const vtr::vector<LogicalRamGroupId, LogicalRamGroup>& ram_groups() const {
        return ram_groups_;
    }

  private:
    /**
     * @brief Estimates the number of instances required for each logical block
     *        type to fit the design.
     *
     * Groups RAM atoms, assigns them to minimum-area types (stored in
     * ram_groups_ for RamMapper), and runs a mini-packing simulation for
     * non-RAM types using pin capacity and cluster placement feasibility
     * constraints to get required number of instances for each type.
     *
     * @return Map from logical block type to estimated cluster instance count.
     */
    std::map<t_logical_block_type_ptr, size_t> estimate_resource_requirement(
        const Prepacker& prepacker);

    /// @brief RAM groups computed during estimation; exposed via ram_groups()
    ///        for reuse by RamMapper to avoid redundant grouping and area assignment.
    vtr::vector<LogicalRamGroupId, LogicalRamGroup> ram_groups_;
};
