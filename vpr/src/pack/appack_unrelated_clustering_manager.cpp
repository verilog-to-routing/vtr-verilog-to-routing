/**
 * @file
 * @author  Alex Singer
 * @date    August 2025
 * @brief   Definition of the unrelated clustering manager class.
 */

#include "appack_unrelated_clustering_manager.h"
#include "ap_argparse_utils.h"
#include "arch_util.h"
#include "vpr_error.h"

void APPackUnrelatedClusteringManager::init(
    const std::vector<std::string>& unrelated_clustering_args,
    const std::vector<t_logical_block_type>& logical_block_types) {

    // Set the max unrelated tile distances for all logical block types.
    // By default, we set this to a low value to only allow unrelated molecules
    // that are very close to the cluster being created.
    // NOTE: Molecules within the same tile as the centroid are considered to have
    //       0 distance. The distance is computed relative to the bounds of the
    //       tile containing the centroid.
    max_unrelated_tile_distance_.resize(logical_block_types.size(),
                                        default_max_unrelated_tile_distance_);
    max_unrelated_clustering_attempts_.resize(logical_block_types.size(),
                                              default_max_unrelated_clustering_attempts_);

    // For memories (such as BRAMs and MLABs), we do not perform unrelated clustering.
    for (const t_logical_block_type& lb_ty : logical_block_types) {
        // Skip the empty logical block type. This should not have any blocks.
        if (lb_ty.is_empty())
            continue;

        // If the logical block type contains a pb_type which is a memory class,
        // it is assumed to be a BRAM or an MLAB.
        bool has_memory = pb_type_contains_memory_pbs(lb_ty.pb_type);
        if (!has_memory)
            continue;

        // Do not do unrelated clustering on memories. It is not worth it.
        max_unrelated_clustering_attempts_[lb_ty.index] = 0;
    }

    // At this point, the data structures have been initialized properly.
    is_initialized_ = true;

    // If the unrelated clustering args are set to auto, nothing to do further.
    if (unrelated_clustering_args[0] == "auto")
        return;

    // Read the args passed by the user to update the unrelated clustering.
    std::vector<std::string> lb_type_names;
    std::unordered_map<std::string, int> lb_type_name_to_index;
    for (const t_logical_block_type& lb_ty : logical_block_types) {
        lb_type_names.push_back(lb_ty.name);
        lb_type_name_to_index[lb_ty.name] = lb_ty.index;
    }

    auto lb_to_floats_map = key_to_float_argument_parser(unrelated_clustering_args,
                                                         lb_type_names,
                                                         2);

    for (const auto& lb_name_to_floats_pair : lb_to_floats_map) {
        const std::string& lb_name = lb_name_to_floats_pair.first;
        const std::vector<float>& lb_floats = lb_name_to_floats_pair.second;
        VTR_ASSERT(lb_floats.size() == 2);

        float logical_block_max_unrel_dist = lb_floats[0];
        float logical_block_max_unrel_attempts = lb_floats[1];

        if (logical_block_max_unrel_dist < 0.0) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "APPack: Cannot have negative max unrelated distance");
        }
        if (logical_block_max_unrel_attempts < 0.0) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "APPack: Cannot have negative max unrelated attempts");
        }
        int lb_ty_index = lb_type_name_to_index[lb_name];
        max_unrelated_tile_distance_[lb_ty_index] = logical_block_max_unrel_dist;
        max_unrelated_clustering_attempts_[lb_ty_index] = logical_block_max_unrel_attempts;
    }
}
