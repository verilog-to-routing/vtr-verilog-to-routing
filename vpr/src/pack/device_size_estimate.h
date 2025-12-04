#pragma once

#include <map>
#include "physical_types.h"   // for t_logical_block_type_ptr
#include "vpr_context.h"      // for g_vpr_ctx
#include "prepack.h"

class DeviceSizeEstimator {
  public:
    DeviceSizeEstimator(const std::string& device_layout,
                        const t_packer_opts& packer_opts,
                        const std::vector<t_grid_def>& grid_layouts,
                        const LogicalModels& models,
                        const Prepacker& prepacker);

    std::map<t_logical_block_type_ptr, size_t> estimate_resource_requirement(const Prepacker& prepacker,
                                                                             const t_packer_opts& packer_opts,
                                                                             const LogicalModels& models) const;
};