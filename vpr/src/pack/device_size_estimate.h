#pragma once
/**
 * @file
 * @author  Haydar Cakan
 * @date    March 12, 2026
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
struct t_grid_def;
struct t_packer_opts;

/**
 * @brief Estimates the device size required to fit the design before packing.
 *
 * When the device layout is set to "auto", this class estimates the number of
 * instances needed for each logical block type and selects the smallest device
 * grid that can accommodate them. For fixed device layouts, the grid is created
 * directly without estimation.
 *
 * RAM groups computed during estimation are exposed via ram_groups() so that
 * RamMapper can reuse them and skip redundant work.
 */
class DeviceSizeEstimator {
  public:
    DeviceSizeEstimator(const std::string& device_layout,
                        const t_packer_opts& packer_opts,
                        const std::vector<t_grid_def>& grid_layouts,
                        const Prepacker& prepacker);

    /// @brief Returns the RAM groups computed during estimation.
    ///        Empty if the device layout was fixed (no estimation needed).
    const vtr::vector<LogicalRamGroupId, LogicalRamGroup>& ram_groups() const {
        return ram_groups_;
    }

  private:
    vtr::vector<LogicalRamGroupId, LogicalRamGroup> ram_groups_;

    /**
     * @brief Estimates the number of instances required for each logical block
     *        type to fit the design.
     *
     * Groups RAM atoms, assigns them to minimum-area types (stored in
     * ram_groups_ for RamMapper), and runs a mini-packing simulation for
     * non-RAM types using pin capacity and cluster placement feasibility
     * constraints to get required number of instances for each type.
     *
     * @return Map from logical block type to estimated instance count.
     */
    std::map<t_logical_block_type_ptr, size_t> estimate_resource_requirement(
        const Prepacker& prepacker);
};
