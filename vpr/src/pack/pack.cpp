
#include "pack.h"

#include <unordered_set>
#include "PreClusterTimingManager.h"
#include "setup_grid.h"
#include "appack_context.h"
#include "attraction_groups.h"
#include "cluster_legalizer.h"
#include "cluster_util.h"
#include "constraints_report.h"
#include "flat_placement_types.h"
#include "globals.h"
#include "greedy_clusterer.h"
#include "partition_region.h"
#include "prepack.h"
#include "stats.h"
#include "verify_flat_placement.h"
#include "vpr_context.h"
#include "vpr_error.h"
#include "vpr_types.h"
#include "vtr_assert.h"
#include "vtr_log.h"

namespace {

/**
 * @brief Enumeration for the state of the packer.
 *
 * If the packer fails to find a dense enough packing, depending on how the
 * packer failed, the packer may iteratively retry packing with different
 * settings to try and find a denser packing. These states represent a
 * different type of iteration.
 */
enum class e_packer_state {
    /// @brief Default packer state.
    DEFAULT,
    /// @brief Success state for the packer. The packing looks feasible to
    ///        fit on the device (does not exceed number of blocks of each
    ///        type in the grid) and meets floorplanning constraints.
    SUCCESS,
    /// @brief Turns on the unrelated clustering and balanced block type utilization
    ///        if they are not on already.
    SET_UNRELATED_AND_BALANCED,
    /// @brief Increases the target pin utilization of overused block types if
    ///        it can.
    INCREASE_OVERUSED_TARGET_PIN_UTILIZATION,
    /// @brief Region constraints: Turns on attraction groups for overfilled regions.
    CREATE_ATTRACTION_GROUPS,
    /// @brief Region constraints: Turns on more attraction groups for overfilled regions.
    CREATE_MORE_ATTRACTION_GROUPS,
    /// @brief Region constraints: Turns on more attraction groups for all regions.
    CREATE_ATTRACTION_GROUPS_FOR_ALL_REGIONS,
    /// @brief Region constraints: Turns on more attraction groups for all regions
    ///        and increases the pull on these groups.
    CREATE_ATTRACTION_GROUPS_FOR_ALL_REGIONS_AND_INCREASE_PULL,
    /// @brief APPack: Increase the max displacement threshold for overused block types.
    AP_INCREASE_MAX_DISPLACEMENT,
    /// @brief APPack: Increase the effort-level of unrelated clustering by allowing
    ///                more attempts and exploring more of the device.
    AP_USE_HIGH_EFFORT_UC,
    /// @brief The failure state.
    FAILURE
};

} // namespace

/**
 * @brief The packer iteratively re-packes the netlist if it fails to find a
 *        valid clustering. Each iteration is a state the packer is in, where
 *        each state uses a different set of options. This method gets the next
 *        state of the packer given the current state of the packer.
 *
 *  @param current_packer_state
 *      The current state of the packer (the state used to make the most recent
 *      clustering).
 *  @param fits_on_device
 *      Whether the current clustering fits on the device or not.
 *  @param floorplan_regions_overfull
 *      Whether the current clustering has overfilled regions.
 *  @param using_unrelated_clustering
 *      Whether the current clustering used unrelated clustering.
 *  @param using_balanced_block_type_util
 *      Whether the current clustering used balanced block type utilizations.
 *  @param block_type_utils
 *      The current block type utilizations for the clustering. If any of these
 *      are larger than 1.0, then fits_on_device will be false.
 *  @param external_pin_util_targets
 *      The current external pin utilization targets.
 *  @param packer_opts
 *      The options passed into the packer.
 *  @param appack_ctx
 *      The APPack context used when AP is turned on.
 */
static e_packer_state get_next_packer_state(e_packer_state current_packer_state,
                                            bool fits_on_device,
                                            bool floorplan_regions_overfull,
                                            bool using_unrelated_clustering,
                                            bool using_balanced_block_type_util,
                                            const std::map<t_logical_block_type_ptr, float>& block_type_utils,
                                            const t_ext_pin_util_targets& external_pin_util_targets,
                                            const t_packer_opts& packer_opts,
                                            const APPackContext& appack_ctx) {
    if (fits_on_device && !floorplan_regions_overfull) {
        // If everything fits on the device and the floorplan regions are
        // not overfilled, the next state is success.
        return e_packer_state::SUCCESS;
    }

    // First thing to try for APPack (after unrelated and balanced) is to increase the max distance threshold of overused blocks.
    // APPack: If packing was not successful, try to see if we should increase the
    //         max displacement threshold. This should have the smallest affect on
    //         quality, so we want to do this first.
    if (appack_ctx.appack_options.use_appack) {
        for (const auto& p : block_type_utils) {
            if (p.second <= 1.0f)
                continue;

            // If the utilization is so high that we know we will end up turning on unrelated clustering anyways, skip this increase.
            // When we have to increase the max distance threshold too much, we end up losing a lot of quality.
            // It is better to try and do low-effort unrelated clustering first.
            // We will increase after turning on low-effort unrelated clustering.
            if (p.second > 1.3f)
                continue;

            // Check if we can increase the max distance threshold for any of the
            // overused block types.
            float max_distance_th = appack_ctx.max_distance_threshold_manager.get_max_dist_threshold(*p.first);
            float max_device_distance = appack_ctx.max_distance_threshold_manager.get_max_device_distance();
            if (max_distance_th < max_device_distance)
                return e_packer_state::AP_INCREASE_MAX_DISPLACEMENT;
        }
    }

    // Check if there are overfilled floorplan regions.
    if (floorplan_regions_overfull) {
        // If there are overfilled region constraints, try to use attraction
        // groups to resolve it.

        // When running with tight floorplan constraints, some regions may become overfull with clusters (i.e.
        // the number of blocks assigned to the region exceeds the number of blocks available). When this occurs, we
        // cluster more densely to be able to adhere to the floorplan constraints. However, we do not want to cluster more
        // densely unnecessarily, as this can negatively impact wirelength. So, we have iterative approach. We check at the end
        // of every iteration if any floorplan regions are overfull. In the first iteration, we run
        // with no attraction groups (not packing more densely). If regions are overfull at the end of the first iteration,
        // we create attraction groups for partitions with overfull regions (pack those atoms more densely). We continue this way
        // until the last iteration, when we create attraction groups for every partition, if needed.

        switch (current_packer_state) {
            case e_packer_state::DEFAULT:
            case e_packer_state::SET_UNRELATED_AND_BALANCED:
            case e_packer_state::INCREASE_OVERUSED_TARGET_PIN_UTILIZATION:
                return e_packer_state::CREATE_ATTRACTION_GROUPS;
            case e_packer_state::CREATE_ATTRACTION_GROUPS:
                return e_packer_state::CREATE_MORE_ATTRACTION_GROUPS;
            case e_packer_state::CREATE_MORE_ATTRACTION_GROUPS:
                return e_packer_state::CREATE_ATTRACTION_GROUPS_FOR_ALL_REGIONS;
            case e_packer_state::CREATE_ATTRACTION_GROUPS_FOR_ALL_REGIONS:
                return e_packer_state::CREATE_ATTRACTION_GROUPS_FOR_ALL_REGIONS_AND_INCREASE_PULL;
            case e_packer_state::CREATE_ATTRACTION_GROUPS_FOR_ALL_REGIONS_AND_INCREASE_PULL:
            default:
                break;
        }
    }

    // Check if the utilizaion of any block types is over 100%
    if (!fits_on_device) {
        // The utilization of some block types are too high. Need to increase the
        // density of the block types available.

        // Check if we can turn on unrelated cluster and/or balanced block type
        // utilization and they have not been turned on already.
        if (packer_opts.allow_unrelated_clustering == e_unrelated_clustering::AUTO && !using_unrelated_clustering) {
            return e_packer_state::SET_UNRELATED_AND_BALANCED;
        }
        if (packer_opts.balance_block_type_utilization == e_balance_block_type_util::AUTO && !using_balanced_block_type_util) {
            return e_packer_state::SET_UNRELATED_AND_BALANCED;
        }
    }

    // APPack: If we already tried unrelated clustering, and we can increase the
    //         max displacement threshold of any overfilled block types, try to
    //         increase them.
    if (appack_ctx.appack_options.use_appack) {
        for (const auto& p : block_type_utils) {
            if (p.second <= 1.0f)
                continue;

            // Check if we can increase the max distance threshold for any of the
            // overused block types.
            float max_distance_th = appack_ctx.max_distance_threshold_manager.get_max_dist_threshold(*p.first);
            float max_device_distance = appack_ctx.max_distance_threshold_manager.get_max_device_distance();
            if (max_distance_th < max_device_distance)
                return e_packer_state::AP_INCREASE_MAX_DISPLACEMENT;
        }
    }

    // Check if we can increase the target density of the overused block types.
    // This is a last resort since increasing the target pin density can have
    // bad affects on quality and routability.
    for (const auto& p : block_type_utils) {
        const t_ext_pin_util& target_pin_util = external_pin_util_targets.get_pin_util(p.first->name);
        if (p.second > 1.0f && (target_pin_util.input_pin_util < 1.0f || target_pin_util.output_pin_util < 1.0f))
            return e_packer_state::INCREASE_OVERUSED_TARGET_PIN_UTILIZATION;
    }

    // APPack: Last resort is to increase the effort of the unrelated clustering.
    //         This will have the worst affect on routability, so we only want
    //         to try this if we have to.
    if (appack_ctx.appack_options.use_appack) {
        for (const auto& p : block_type_utils) {
            if (p.second <= 1.0f)
                continue;

            float max_unrelated_tile_distance = appack_ctx.unrelated_clustering_manager.get_max_unrelated_tile_dist(*p.first);
            float max_device_distance = appack_ctx.max_distance_threshold_manager.get_max_device_distance();
            int max_unrelated_clustering_attempts = appack_ctx.unrelated_clustering_manager.get_max_unrelated_clustering_attempts(*p.first);
            if (max_unrelated_tile_distance < max_device_distance || max_unrelated_clustering_attempts < appack_ctx.unrelated_clustering_manager.high_effort_max_unrelated_clustering_attempts_) {
                return e_packer_state::AP_USE_HIGH_EFFORT_UC;
            }
        }
    }

    // If we got down here, that means that we could not find any packer option that would
    // increase the density further. Fail.
    return e_packer_state::FAILURE;
}

bool try_pack(const t_packer_opts& packer_opts,
              const t_analysis_opts& analysis_opts,
              const t_ap_opts& ap_opts,
              const t_arch& arch,
              std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
              const Prepacker& prepacker,
              const PreClusterTimingManager& pre_cluster_timing_manager,
              const FlatPlacementInfo& flat_placement_info) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    // The clusterer modifies the device context by increasing the size of the
    // device if needed.
    DeviceContext& mutable_device_ctx = g_vpr_ctx.mutable_device();

    std::unordered_set<AtomNetId> is_clock, is_global;
    VTR_LOG("Begin packing '%s'.\n", packer_opts.circuit_file_name.c_str());

    is_clock = alloc_and_load_is_clock();
    is_global.insert(is_clock.begin(), is_clock.end());

    size_t num_p_inputs = 0;
    size_t num_p_outputs = 0;
    for (auto blk_id : atom_ctx.netlist().blocks()) {
        auto type = atom_ctx.netlist().block_type(blk_id);
        if (type == AtomBlockType::INPAD) {
            ++num_p_inputs;
        } else if (type == AtomBlockType::OUTPAD) {
            ++num_p_outputs;
        }
    }

    VTR_LOG("\n");
    VTR_LOG("After removing unused inputs...\n");
    VTR_LOG("\ttotal blocks: %zu, total nets: %zu, total inputs: %zu, total outputs: %zu\n",
            atom_ctx.netlist().blocks().size(), atom_ctx.netlist().nets().size(), num_p_inputs, num_p_outputs);

    /* We keep attraction groups off in the first iteration,  and
     * only turn on in later iterations if some floorplan regions turn out to be overfull.
     */
    AttractionInfo attraction_groups(false);

    // We keep track of the overfilled partition regions from all pack iterations in
    // this vector. This is so that if the first iteration fails due to overfilled
    // partition regions, and it fails again, we can carry over the previous failed
    // partition regions to the current iteration.
    std::vector<PartitionRegion> overfull_partition_regions;

    // Verify that the Flat Placement is valid for packing.
    if (flat_placement_info.valid) {
        unsigned num_errors = verify_flat_placement_for_packing(flat_placement_info,
                                                                atom_ctx.netlist(),
                                                                prepacker);
        if (num_errors == 0) {
            VTR_LOG("Completed flat placement consistency check successfully.\n");
        } else {
            // TODO: In the future, we can just erase the flat placement and
            //       continue. It depends on what we want to happen if the
            //       flat placement is not valid.
            VPR_ERROR(VPR_ERROR_PACK,
                      "%u errors found while performing flat placement "
                      "consistency check. Aborting program.\n",
                      num_errors);
        }
    }

    // During clustering, a block is related to un-clustered primitives with nets.
    // This relation has three types: low fanout, high fanout, and transitive
    // high_fanout_thresholds stores the threshold for nets to a block type to
    // be considered high fanout.
    t_pack_high_fanout_thresholds high_fanout_thresholds(packer_opts.high_fanout_threshold);

    bool allow_unrelated_clustering = false;
    if (packer_opts.allow_unrelated_clustering == e_unrelated_clustering::ON) {
        allow_unrelated_clustering = true;
    } else if (packer_opts.allow_unrelated_clustering == e_unrelated_clustering::OFF) {
        allow_unrelated_clustering = false;
    }

    bool balance_block_type_util = false;
    if (packer_opts.balance_block_type_utilization == e_balance_block_type_util::ON) {
        balance_block_type_util = true;
    } else if (packer_opts.balance_block_type_utilization == e_balance_block_type_util::OFF) {
        balance_block_type_util = false;
    }

    int pack_iteration = 1;
    // Initialize the cluster legalizer.
    ClusterLegalizer cluster_legalizer(atom_ctx.netlist(),
                                       prepacker,
                                       lb_type_rr_graphs,
                                       packer_opts.target_external_pin_util,
                                       high_fanout_thresholds,
                                       ClusterLegalizationStrategy::SKIP_INTRA_LB_ROUTE,
                                       packer_opts.enable_pin_feasibility_filter,
                                       arch.models,
                                       packer_opts.pack_verbosity);

    // Construct the APPack Context.
    APPackContext appack_ctx(flat_placement_info,
                             ap_opts,
                             device_ctx.logical_block_types,
                             device_ctx.grid);

    // Initialize the greedy clusterer.
    GreedyClusterer clusterer(packer_opts,
                              analysis_opts,
                              atom_ctx.netlist(),
                              arch,
                              high_fanout_thresholds,
                              is_clock,
                              is_global,
                              pre_cluster_timing_manager,
                              appack_ctx);

    g_vpr_ctx.mutable_atom().mutable_lookup().set_atom_pb_bimap_lock(true);

    // The current state of the packer during iterative packing.
    e_packer_state current_packer_state = e_packer_state::DEFAULT;

    while (current_packer_state != e_packer_state::SUCCESS && current_packer_state != e_packer_state::FAILURE) {
        VTR_LOG("Packing with pin utilization targets: %s\n", cluster_legalizer.get_target_external_pin_util().to_string().c_str());
        VTR_LOG("Packing with high fanout thresholds: %s\n", high_fanout_thresholds.to_string().c_str());
        //Cluster the netlist
        //  num_used_type_instances: A map used to save the number of used
        //                           instances from each logical block type.
        std::map<t_logical_block_type_ptr, size_t> num_used_type_instances;
        num_used_type_instances = clusterer.do_clustering(cluster_legalizer,
                                                          prepacker,
                                                          allow_unrelated_clustering,
                                                          balance_block_type_util,
                                                          attraction_groups,
                                                          mutable_device_ctx);

        // Try to size/find a device
        std::map<t_logical_block_type_ptr, float> block_type_utils;
        bool fits_on_device = try_size_device_grid(arch,
                                                   num_used_type_instances,
                                                   block_type_utils,
                                                   packer_opts.target_device_utilization,
                                                   packer_opts.device_layout);

        /* We use this bool to determine the cause for the clustering not being dense enough. If the clustering
         * is not dense enough and there are floorplan constraints, it is presumed that the constraints are the cause
         * of the floorplan not fitting, so attraction groups are turned on for later iterations.
         */
        bool floorplan_regions_overfull = floorplan_constraints_regions_overfull(overfull_partition_regions,
                                                                                 cluster_legalizer,
                                                                                 device_ctx.logical_block_types);

        // Next packer state logic
        e_packer_state next_packer_state = get_next_packer_state(current_packer_state,
                                                                 fits_on_device,
                                                                 floorplan_regions_overfull,
                                                                 allow_unrelated_clustering,
                                                                 balance_block_type_util,
                                                                 block_type_utils,
                                                                 cluster_legalizer.get_target_external_pin_util(),
                                                                 packer_opts,
                                                                 appack_ctx);

        // Set up for the options used for the next packer state.
        // NOTE: This must be done here (and not at the start of the next packer
        //       iteration) since we need to know information about the current
        //       clustering to change the options for the next iteration.
        switch (next_packer_state) {
            case e_packer_state::SET_UNRELATED_AND_BALANCED: {
                // 1st pack attempt was unsuccessful (i.e. not dense enough) and we have control of unrelated clustering
                //
                // Turn it on to increase packing density
                if (packer_opts.allow_unrelated_clustering == e_unrelated_clustering::AUTO) {
                    VTR_ASSERT(allow_unrelated_clustering == false);
                    allow_unrelated_clustering = true;
                }
                if (packer_opts.balance_block_type_utilization == e_balance_block_type_util::AUTO) {
                    VTR_ASSERT(balance_block_type_util == false);
                    balance_block_type_util = true;
                }
                if (appack_ctx.appack_options.use_appack) {
                    // Only do unrelated clustering on the overused type instances.
                    for (const auto& p : block_type_utils) {
                        // Any overutilized block types will use the default options.
                        if (p.second > 1.0f)
                            continue;

                        // Any underutilized block types should not do unrelated clustering.
                        // We can turn this off by just setting the max attempts to 0.
                        appack_ctx.unrelated_clustering_manager.set_max_unrelated_clustering_attempts(*p.first, 0);
                    }
                }
                VTR_LOG("Packing failed to fit on device. Re-packing with: unrelated_logic_clustering=%s balance_block_type_util=%s\n",
                        (allow_unrelated_clustering ? "true" : "false"),
                        (balance_block_type_util ? "true" : "false"));
                VTR_LOG("Pack iteration is %d\n", pack_iteration);
                break;
            }
            case e_packer_state::INCREASE_OVERUSED_TARGET_PIN_UTILIZATION: {
                // Get the names of the block types to increase the pin utilization of.
                std::vector<std::string> block_types_to_increase;
                for (const auto& p : block_type_utils) {
                    t_ext_pin_util current_util = cluster_legalizer.get_target_external_pin_util().get_pin_util(p.first->name);
                    if (p.second > 1.0f && (current_util.input_pin_util < 1.0f || current_util.output_pin_util < 1.0f)) {
                        block_types_to_increase.push_back(p.first->name);
                    }
                }
                // If something does not increase, then we may get an infinite
                // loop in this state. This assert checks to ensure that does
                // not happen.
                VTR_ASSERT_OPT(!block_types_to_increase.empty());

                // Increase the target pin utilization of over-utilized block types.
                VTR_LOG("Packing failed to fit on device. Increasing the target pin utilizations of overused block types: ");
                t_ext_pin_util pin_util(1.0, 1.0);
                for (size_t i = 0; i < block_types_to_increase.size(); i++) {
                    const std::string& block_type_name = block_types_to_increase[i];
                    cluster_legalizer.get_target_external_pin_util().set_block_pin_util(block_type_name, pin_util);
                    VTR_LOG("%s", block_type_name.c_str());
                    if (i < block_types_to_increase.size() - 1)
                        VTR_LOG(", ");
                }
                VTR_LOG("\n");
                VTR_LOG("Pack iteration is %d\n", pack_iteration);
                break;
            }
            case e_packer_state::CREATE_ATTRACTION_GROUPS: {
                VTR_LOG("Floorplan regions are overfull: trying to pack again using cluster attraction groups. \n");
                attraction_groups.create_att_groups_for_overfull_regions(overfull_partition_regions);
                attraction_groups.set_att_group_pulls(1);
                VTR_LOG("Pack iteration is %d\n", pack_iteration);
                break;
            }
            case e_packer_state::CREATE_MORE_ATTRACTION_GROUPS: {
                VTR_LOG("Floorplan regions are overfull: trying to pack again with more attraction groups exploration. \n");
                attraction_groups.create_att_groups_for_overfull_regions(overfull_partition_regions);
                VTR_LOG("Pack iteration is %d\n", pack_iteration);
                break;
            }
            case e_packer_state::CREATE_ATTRACTION_GROUPS_FOR_ALL_REGIONS: {
                attraction_groups.create_att_groups_for_all_regions();
                VTR_LOG("Floorplan regions are overfull: trying to pack again with more attraction groups exploration. \n");
                VTR_LOG("Pack iteration is %d\n", pack_iteration);
                break;
            }
            case e_packer_state::CREATE_ATTRACTION_GROUPS_FOR_ALL_REGIONS_AND_INCREASE_PULL: {
                attraction_groups.create_att_groups_for_all_regions();
                VTR_LOG("Floorplan regions are overfull: trying to pack again with more attraction groups exploration and higher pull. \n");
                VTR_LOG("Pack iteration is %d\n", pack_iteration);
                attraction_groups.set_att_group_pulls(4);
                break;
            }
            case e_packer_state::AP_USE_HIGH_EFFORT_UC: {
                VTR_ASSERT(appack_ctx.appack_options.use_appack);
                VTR_LOG("Packing failed to fit on device. Using high-effort unrelated clustering.\n");
                VTR_LOG("Pack iteration is %d\n", pack_iteration);
                for (const auto& p : block_type_utils) {
                    if (p.second <= 1.0f)
                        continue;

                    float max_device_distance = appack_ctx.max_distance_threshold_manager.get_max_device_distance();
                    appack_ctx.unrelated_clustering_manager.set_max_unrelated_tile_dist(*p.first, max_device_distance);
                    appack_ctx.unrelated_clustering_manager.set_max_unrelated_clustering_attempts(*p.first,
                                                                                                  appack_ctx.unrelated_clustering_manager.high_effort_max_unrelated_clustering_attempts_);
                }
                break;
            }
            case e_packer_state::AP_INCREASE_MAX_DISPLACEMENT: {
                VTR_ASSERT(appack_ctx.appack_options.use_appack);
                std::vector<t_logical_block_type_ptr> block_types_to_increase;
                for (const auto& p : block_type_utils) {
                    if (p.second <= 1.0f)
                        continue;

                    float max_device_distance = appack_ctx.max_distance_threshold_manager.get_max_device_distance();
                    float max_distance_th = appack_ctx.max_distance_threshold_manager.get_max_dist_threshold(*p.first);
                    if (max_distance_th < max_device_distance)
                        block_types_to_increase.push_back(p.first);
                }

                VTR_LOG("Packing failed to fit on device. Increasing the APPack max distance thresholds of block types: ");
                for (size_t i = 0; i < block_types_to_increase.size(); i++) {
                    t_logical_block_type_ptr block_type_ptr = block_types_to_increase[i];

                    // Increase the max distance threshold.
                    // This allows it to increase the threshold slowly without going
                    // all the way immediately.
                    float old_max_dist_th = appack_ctx.max_distance_threshold_manager.get_max_dist_threshold(*block_type_ptr);
                    // Note: The +1 is to account for the case when the max dist th is 0.
                    float dist_th_scale = appack_ctx.max_distance_threshold_manager.max_dist_th_fail_increase_scale;
                    float new_max_dist_th = old_max_dist_th * dist_th_scale + 1;
                    appack_ctx.max_distance_threshold_manager.set_max_dist_threshold(*block_type_ptr,
                                                                                     new_max_dist_th);

                    VTR_LOG("%s", block_type_ptr->name.c_str());
                    if (i < block_types_to_increase.size() - 1)
                        VTR_LOG(", ");
                }
                VTR_LOG("\n");
                VTR_LOG("Pack iteration is %d\n", pack_iteration);
                break;
            }
            case e_packer_state::DEFAULT:
            case e_packer_state::SUCCESS:
            case e_packer_state::FAILURE:
            default:
                // Nothing to set up.
                break;
        }

        // Raise an error if the packer failed to pack.
        if (next_packer_state == e_packer_state::FAILURE) {
            if (floorplan_regions_overfull) {
                VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                                "Failed to find pack clusters densely enough to fit in the designated floorplan regions.\n"
                                "The floorplan regions may need to be expanded to run successfully. \n");
            }

            //No suitable device found
            std::string resource_reqs;
            std::string resource_avail;
            auto& grid = g_vpr_ctx.device().grid;
            for (auto iter = num_used_type_instances.begin(); iter != num_used_type_instances.end(); ++iter) {
                if (iter != num_used_type_instances.begin()) {
                    resource_reqs += ", ";
                    resource_avail += ", ";
                }

                resource_reqs += iter->first->name + ": " + std::to_string(iter->second);

                int num_instances = 0;
                for (auto type : iter->first->equivalent_tiles)
                    num_instances += grid.num_instances(type, -1);

                resource_avail += iter->first->name + ": " + std::to_string(num_instances);
            }

            VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Failed to find device which satisfies resource requirements required: %s (available %s)", resource_reqs.c_str(), resource_avail.c_str());
        }

        // If the packer was unsuccessful, reset the packed solution and try again.
        if (next_packer_state != e_packer_state::SUCCESS) {
            // Reset floorplanning constraints for re-packing
            g_vpr_ctx.mutable_floorplanning().cluster_constraints.clear();

            // Reset the cluster legalizer for re-clustering.
            cluster_legalizer.reset();
        }

        // Set the current state to the next state.
        current_packer_state = next_packer_state;

        ++pack_iteration;
    }

    VTR_ASSERT(current_packer_state == e_packer_state::SUCCESS);

    g_vpr_ctx.mutable_atom().mutable_lookup().set_atom_pb_bimap_lock(false);
    g_vpr_ctx.mutable_atom().mutable_lookup().set_atom_to_pb_bimap(cluster_legalizer.atom_pb_lookup());

    //check clustering and output it
    check_and_output_clustering(cluster_legalizer, packer_opts, is_clock, &arch);

    VTR_LOG("\n");
    VTR_LOG("Netlist conversion complete.\n");
    VTR_LOG("\n");

    return true;
}

std::unordered_set<AtomNetId> alloc_and_load_is_clock() {
    /* Looks through all the atom blocks to find and mark all the clocks, by setting
     * the corresponding entry by adding the clock to is_clock.
     * only for an error check.                                                */

    std::unordered_set<AtomNetId> is_clock;

    /* Want to identify all the clock nets.  */
    auto& atom_ctx = g_vpr_ctx.atom();

    for (auto blk_id : atom_ctx.netlist().blocks()) {
        for (auto pin_id : atom_ctx.netlist().block_clock_pins(blk_id)) {
            auto net_id = atom_ctx.netlist().pin_net(pin_id);
            if (!is_clock.count(net_id)) {
                is_clock.insert(net_id);
            }
        }
    }

    return (is_clock);
}

bool try_size_device_grid(const t_arch& arch,
                          const std::map<t_logical_block_type_ptr, size_t>& num_type_instances,
                          std::map<t_logical_block_type_ptr, float>& type_util,
                          float target_device_utilization,
                          const std::string& device_layout_name) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    //Build the device
    auto grid = create_device_grid(device_layout_name, arch.grid_layouts, num_type_instances, target_device_utilization);

    /*
     *Report on the device
     */
    VTR_LOG("FPGA sized to %zu x %zu (%s)\n", grid.width(), grid.height(), grid.name().c_str());

    bool fits_on_device = true;

    float device_utilization = calculate_device_utilization(grid, num_type_instances);
    VTR_LOG("Device Utilization: %.2f (target %.2f)\n", device_utilization, target_device_utilization);
    for (const auto& type : device_ctx.logical_block_types) {
        if (is_empty_type(&type)) continue;

        auto itr = num_type_instances.find(&type);
        if (itr == num_type_instances.end()) continue;

        float num_instances = itr->second;
        float util = 0.;

        float num_total_instances = 0.;
        for (const auto& equivalent_tile : type.equivalent_tiles) {
            num_total_instances += device_ctx.grid.num_instances(equivalent_tile, -1);
        }

        if (num_total_instances != 0) {
            util = num_instances / num_total_instances;
        }
        type_util[&type] = util;

        if (util > 1.) {
            fits_on_device = false;
        }
        VTR_LOG("\tBlock Utilization: %.2f Type: %s\n", util, type.name.c_str());
    }
    VTR_LOG("\n");

    return fits_on_device;
}
