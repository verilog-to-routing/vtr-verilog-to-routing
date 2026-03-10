#pragma once

#include <map>
#include <vector>

#include "logical_ram_infer.h"
#include "physical_types.h"
#include "prepack.h"
#include "vtr_vector.h"

// Forward declarations
struct t_grid_def;
struct t_packer_opts;
class LogicalModels;

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

    std::map<t_logical_block_type_ptr, size_t> estimate_resource_requirement(
        const Prepacker& prepacker) const;
};
