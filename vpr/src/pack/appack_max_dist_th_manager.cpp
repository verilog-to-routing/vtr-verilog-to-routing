/**
 * @file
 * @author  Alex Singer
 * @date    May 2025
 * @brief   Definition of the max distance threshold manager class.
 */

#include "appack_max_dist_th_manager.h"
#include <string>
#include <unordered_map>
#include <vector>
#include "ap_argparse_utils.h"
#include "arch_util.h"
#include "device_grid.h"
#include "physical_types.h"
#include "physical_types_util.h"
#include "vpr_error.h"
#include "vpr_utils.h"
#include "vtr_assert.h"
#include "vtr_log.h"

void APPackMaxDistThManager::init(const std::vector<std::string>& max_dist_ths,
                                  const std::vector<t_logical_block_type>& logical_block_types,
                                  const DeviceGrid& device_grid) {
    // Compute the max device distance based on the width and height of the
    // device. This is the L1 (manhattan) distance.
    max_distance_on_device_ = device_grid.width() + device_grid.height();

    // Automatically set the max distance thresholds.
    auto_set_max_distance_thresholds(logical_block_types, device_grid);

    // If the max distance threshold strings have been set (they are not set to
    // auto), set the max distance thresholds based on the user-provided strings.
    VTR_ASSERT(!max_dist_ths.empty());
    if (max_dist_ths.size() != 1 || max_dist_ths[0] != "auto") {
        set_max_distance_thresholds_from_strings(max_dist_ths, logical_block_types);
    }

    // Set the initilized flag to true.
    is_initialized_ = true;

    // Log the max distance thresholds for each logical block type. This is
    // similar to how the input and output pin utilizations are printed.
    VTR_LOG("APPack is using max distance thresholds: ");
    for (const t_logical_block_type& lb_ty : logical_block_types) {
        if (lb_ty.is_empty())
            continue;
        VTR_LOG("%s:%g ",
                lb_ty.name.c_str(),
                get_max_dist_threshold(lb_ty));
    }
    VTR_LOG("\n");
}

void APPackMaxDistThManager::auto_set_max_distance_thresholds(const std::vector<t_logical_block_type>& logical_block_types,
                                                              const DeviceGrid& device_grid) {

    // Compute the max distance thresholds of the different logical block types.
    float default_max_distance_th = std::max(default_max_dist_th_scale_ * max_distance_on_device_,
                                             default_max_dist_th_offset_);
    float logic_block_max_distance_th = std::max(logic_block_max_dist_th_scale_ * max_distance_on_device_,
                                                 logic_block_max_dist_th_offset_);
    float memory_max_distance_th = std::max(memory_max_dist_th_scale_ * max_distance_on_device_,
                                            memory_max_dist_th_offset_);
    float io_block_max_distance_th = std::max(io_max_dist_th_scale_ * max_distance_on_device_,
                                              io_max_dist_th_offset_);

    // Set all logical block types to have the default max distance threshold.
    logical_block_dist_thresholds_.resize(logical_block_types.size(), default_max_distance_th);

    // Find which (if any) of the logical block types most looks like a CLB block.
    t_logical_block_type_ptr logic_block_type = infer_logic_block_type(device_grid);

    // Go through each of the logical block types.
    for (const t_logical_block_type& lb_ty : logical_block_types) {
        // Skip the empty logical block type. This should not have any blocks.
        if (lb_ty.is_empty())
            continue;

        // Find which type(s) this logical block type looks like.
        bool has_memory = pb_type_contains_memory_pbs(lb_ty.pb_type);
        bool is_logic_block_type = (lb_ty.index == logic_block_type->index);
        bool is_io_block = pick_physical_type(&lb_ty)->is_io();

        // Update the max distance threshold based on the type. If the logical
        // block type looks like many block types at the same time (for example
        // a CLB which has memory slices within it), then take the average
        // of the max distance thresholds of those types.
        float max_distance_th_sum = 0.0f;
        unsigned block_category_count = 0;
        if (is_logic_block_type) {
            max_distance_th_sum += logic_block_max_distance_th;
            block_category_count++;
        }
        if (has_memory) {
            max_distance_th_sum += memory_max_distance_th;
            block_category_count++;
        }
        if (is_io_block) {
            max_distance_th_sum += io_block_max_distance_th;
            block_category_count++;
        }
        if (block_category_count > 0) {
            logical_block_dist_thresholds_[lb_ty.index] = max_distance_th_sum / static_cast<float>(block_category_count);
        }
    }
}

void APPackMaxDistThManager::set_max_distance_thresholds_from_strings(
    const std::vector<std::string>& max_dist_ths,
    const std::vector<t_logical_block_type>& logical_block_types) {

    std::vector<std::string> lb_type_names;
    std::unordered_map<std::string, int> lb_type_name_to_index;
    for (const t_logical_block_type& lb_ty : logical_block_types) {
        lb_type_names.push_back(lb_ty.name);
        lb_type_name_to_index[lb_ty.name] = lb_ty.index;
    }

    auto lb_to_floats_map = key_to_float_argument_parser(max_dist_ths, lb_type_names, 2);

    for (const auto& lb_name_to_floats_pair : lb_to_floats_map) {
        const std::string& lb_name = lb_name_to_floats_pair.first;
        const std::vector<float>& lb_floats = lb_name_to_floats_pair.second;
        VTR_ASSERT(lb_floats.size() == 2);
        float logical_block_max_dist_th_scale = lb_floats[0];
        float logical_block_max_dist_th_offset = lb_floats[1];

        if (logical_block_max_dist_th_scale < 0.0) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "APPack: Cannot have negative max distance threshold scale");
        }
        if (logical_block_max_dist_th_offset < 0.0) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "APPack: Cannot have negative max distance threshold offset");
        }

        // Compute the max distance threshold the user selected.
        float logical_block_max_dist_th = std::max(max_distance_on_device_ * logical_block_max_dist_th_scale,
                                                   logical_block_max_dist_th_offset);

        int lb_ty_index = lb_type_name_to_index[lb_name];
        logical_block_dist_thresholds_[lb_ty_index] = logical_block_max_dist_th;
    }
}
