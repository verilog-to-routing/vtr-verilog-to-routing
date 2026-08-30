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
#include <unordered_set>
#include <vector>
#include "device_grid.h"
#include "physical_types.h"
#include "vpr_utils.h"
#include "vtr_log.h"

t_appack_device_size_adjustment APPackContext::adjust_for_device_size_estimate(
    const std::map<t_logical_block_type_ptr, size_t>& estimated_type_instance_counts,
    const std::vector<t_logical_block_type>& logical_block_types,
    const DeviceGrid& device_grid) {

    t_appack_device_size_adjustment adjustment;

    // This reaction only makes sense when APPack is in use; the managers it
    // touches are not initialized otherwise.
    if (!appack_options.use_appack)
        return adjustment;

    std::unordered_set<t_logical_block_type_ptr> severe_block_types;
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
            // No capacity at all for this type on the device; widening the
            // search radius cannot help, so go straight to unrelated clustering.
            any_type_needs_denser_packing = true;
            severe_block_types.insert(&type);
            continue;
        }

        float utilization = static_cast<float>(estimated_instances) / static_cast<float>(num_total_instances);
        if (utilization < device_size_min_utilization_for_th_bump)
            continue; // Comfortably fits, even accounting for AP packing less densely than estimated.

        any_type_needs_denser_packing = true;

        // Linearly scale the multiplier applied to this type's normal max
        // distance threshold from 1x to device_size_max_dist_th_scale_multiplier
        // as utilization goes from device_size_min_utilization_for_th_bump to
        // device_size_severe_utilization_cutoff.
        float utilization_clamped = std::min(utilization, device_size_severe_utilization_cutoff);
        float th_multiplier = 1.0f + (utilization_clamped - device_size_min_utilization_for_th_bump) / (device_size_severe_utilization_cutoff - device_size_min_utilization_for_th_bump) * (device_size_max_dist_th_scale_multiplier - 1.0f);
        float base_max_dist_th = max_distance_threshold_manager.get_max_dist_threshold(type);
        max_distance_threshold_manager.set_max_dist_threshold(type, base_max_dist_th * th_multiplier);
        VTR_LOG("\t%s: %g\n", type.name.c_str(), base_max_dist_th * th_multiplier);

        if (utilization >= device_size_severe_utilization_cutoff)
            severe_block_types.insert(&type);
    }

    if (any_type_needs_denser_packing) {
        VTR_LOG("Device size estimate predicts a tight packing; increased the max candidate distance threshold for the affected block type(s).\n");
    }

    if (!severe_block_types.empty()) {
        adjustment.allow_unrelated_clustering = true;

        // Restrict unrelated clustering to only the block types the estimate
        // flagged as severely tight; other types keep their default (higher
        // quality) unrelated clustering settings off.
        for (const t_logical_block_type& type : logical_block_types) {
            if (is_empty_type(&type))
                continue;
            if (!severe_block_types.count(&type))
                unrelated_clustering_manager.set_max_unrelated_clustering_attempts(type, 0);
        }

        VTR_LOG("Device size estimate predicts a severe overage for block type(s): ");
        bool first = true;
        for (t_logical_block_type_ptr type : severe_block_types) {
            VTR_LOG("%s%s", first ? "" : ", ", type->name.c_str());
            first = false;
        }
        VTR_LOG(". Enabling unrelated clustering for these type(s) from the start.\n");
    }

    return adjustment;
}
