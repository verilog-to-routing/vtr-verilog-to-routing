/**
 * @file
 * @author  Alex Singer
 * @date    August 2026
 * @brief   Implementation of the APPack Context object.
 */

#include "appack_context.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <vector>
#include "device_grid.h"
#include "physical_types.h"
#include "vpr_utils.h"
#include "vtr_log.h"

/**
 * @brief Counts how many instances of the given logical block type the device
 *        can hold.
 */
static size_t count_available_instances(const t_logical_block_type& type,
                                        const DeviceGrid& device_grid) {
    size_t num_instances = 0;
    for (const t_physical_tile_type_ptr equivalent_tile : type.equivalent_tiles)
        num_instances += device_grid.num_instances(equivalent_tile, -1);
    return num_instances;
}

/**
 * @brief Logs a table showing, for each logical block type the netlist uses, how
 *        many instances the device has, how many the netlist is estimated to
 *        need, and the multiplier applied to that type's max candidate distance
 *        threshold (1.0 = unchanged).
 */
static void log_device_size_adjustments(
    const std::map<t_logical_block_type_ptr, size_t>& estimated_type_instance_counts,
    const std::map<t_logical_block_type_ptr, float>& type_th_multiplier,
    const std::vector<t_logical_block_type>& logical_block_types,
    const DeviceGrid& device_grid) {

    VTR_LOG("\nAPPack device size estimate reaction (per used block type):\n");
    VTR_LOG("%-20s %12s %12s %10s %10s\n",
            "Block Type", "Available", "Estimated", "Util(%)", "DistThMul");
    for (const t_logical_block_type& type : logical_block_types) {
        auto itr = estimated_type_instance_counts.find(&type);
        size_t estimated = (itr != estimated_type_instance_counts.end()) ? itr->second : 0;
        if (estimated == 0)
            continue; // Skip block types the netlist does not use.

        size_t available = count_available_instances(type, device_grid);
        float utilization = (available != 0) ? 100.0f * estimated / available : 0.0f;

        auto mul_itr = type_th_multiplier.find(&type);
        float th_multiplier = (mul_itr != type_th_multiplier.end()) ? mul_itr->second : 1.0f;

        VTR_LOG("%-20s %12zu %12zu %10.1f %10.2f\n",
                type.name.c_str(), available, estimated, utilization, th_multiplier);
    }
    VTR_LOG("\n");
}

void APPackContext::adjust_for_device_size_estimate(
    const std::map<t_logical_block_type_ptr, size_t>& estimated_type_instance_counts,
    const std::vector<t_logical_block_type>& logical_block_types,
    const DeviceGrid& device_grid) {

    // This reaction only makes sense when APPack is in use; the managers it
    // touches are not initialized otherwise.
    if (!appack_options.use_appack)
        return;

    // Max distance threshold multiplier applied to each block type, recorded for
    // logging. Types not present here were left unchanged (multiplier of 1.0).
    std::map<t_logical_block_type_ptr, float> type_th_multiplier;

    for (const t_logical_block_type& type : logical_block_types) {
        if (is_empty_type(&type))
            continue;

        size_t num_total_instances = count_available_instances(type, device_grid);
        if (num_total_instances == 0)
            continue; // No capacity for this type on the device; ignoring for now.

        auto itr = estimated_type_instance_counts.find(&type);
        size_t estimated_instances = (itr != estimated_type_instance_counts.end()) ? itr->second : 0;

        // Compute the utilization of this block type.
        float utilization = static_cast<float>(estimated_instances) / static_cast<float>(num_total_instances);
        if (utilization < device_size_min_utilization_for_th_bump)
            continue; // Comfortably fits.

        // Linearly scale the multiplier applied to this type's normal max
        // distance threshold from 1x to device_size_max_dist_th_scale_multiplier
        // as utilization goes from device_size_min_utilization_for_th_bump to
        // device_size_severe_utilization_cutoff.
        float utilization_clamped = std::min(utilization, device_size_severe_utilization_cutoff);
        float th_multiplier = 1.0f + (utilization_clamped - device_size_min_utilization_for_th_bump) / (device_size_severe_utilization_cutoff - device_size_min_utilization_for_th_bump) * (device_size_max_dist_th_scale_multiplier - 1.0f);
        float base_max_dist_th = max_distance_threshold_manager.get_max_dist_threshold(type);
        max_distance_threshold_manager.set_max_dist_threshold(type, base_max_dist_th * th_multiplier);

        type_th_multiplier[&type] = th_multiplier;
    }

    log_device_size_adjustments(estimated_type_instance_counts, type_th_multiplier,
                                logical_block_types, device_grid);

    // TODO: This should be capable of turning on unrelated clustering as well if the
    //       utilization is high enough. I chose to keep that out for now as I explore
    //       when it is best to use unrelated clustering. From many experiments, I have
    //       found that it is always better to turn off unrelated clustering if you can.

    return;
}
