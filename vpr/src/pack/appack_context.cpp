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

void APPackContext::adjust_for_device_size_estimate(
    const std::map<t_logical_block_type_ptr, size_t>& estimated_type_instance_counts,
    const std::vector<t_logical_block_type>& logical_block_types,
    const DeviceGrid& device_grid) {

    // This reaction only makes sense when APPack is in use; the managers it
    // touches are not initialized otherwise.
    if (!appack_options.use_appack)
        return;

    bool any_type_needs_denser_packing = false;
    for (const t_logical_block_type& type : logical_block_types) {
        if (is_empty_type(&type))
            continue;

        size_t num_total_instances = 0;
        for (const t_physical_tile_type_ptr equivalent_tile : type.equivalent_tiles)
            num_total_instances += device_grid.num_instances(equivalent_tile, -1);

        auto itr = estimated_type_instance_counts.find(&type);
        size_t estimated_instances = (itr != estimated_type_instance_counts.end()) ? itr->second : 0;

        if (num_total_instances == 0) {
            if (estimated_instances == 0)
                continue; // Nothing of this type needed; leave untouched.
            // No capacity at all for this type on the device; ignoring for now.
            continue;
        }

        // Compute the utilization of this block type.
        float utilization = static_cast<float>(estimated_instances) / static_cast<float>(num_total_instances);
        if (utilization < device_size_min_utilization_for_th_bump)
            continue; // Comfortably fits.

        any_type_needs_denser_packing = true;

        // Linearly scale the multiplier applied to this type's normal max
        // distance threshold from 1x to device_size_max_dist_th_scale_multiplier
        // as utilization goes from device_size_min_utilization_for_th_bump to
        // device_size_severe_utilization_cutoff.
        float utilization_clamped = std::min(utilization, device_size_severe_utilization_cutoff);
        float th_multiplier = 1.0f + (utilization_clamped - device_size_min_utilization_for_th_bump) / (device_size_severe_utilization_cutoff - device_size_min_utilization_for_th_bump) * (device_size_max_dist_th_scale_multiplier - 1.0f);
        float base_max_dist_th = max_distance_threshold_manager.get_max_dist_threshold(type);
        max_distance_threshold_manager.set_max_dist_threshold(type, base_max_dist_th * th_multiplier);
    }

    if (any_type_needs_denser_packing) {
        VTR_LOG("Device size estimate predicts a tight packing; increased the max candidate distance threshold for the affected block type(s).\n");
        max_distance_threshold_manager.print_max_dist_thresholds(logical_block_types);
    }

    // TODO: This should be capable of turning on unrelated clustering as well if the
    //       utilization is high enough. I chose to keep that out for now as I explore
    //       when it is best to use unrelated clustering. From many experiments, I have
    //       found that it is always better to turn off unrelated clustering if you can.

    return;
}
