#include <unordered_set>

#include "SetupGrid.h"
#include "cluster.h"
#include "cluster_legalizer.h"
#include "cluster_util.h"
#include "globals.h"
#include "pack.h"
#include "prepack.h"
#include "vpr_context.h"
#include "vpr_error.h"
#include "vpr_types.h"
#include "vtr_assert.h"
#include "vtr_log.h"

static bool try_size_device_grid(const t_arch& arch,
                                 const std::map<t_logical_block_type_ptr, size_t>& num_type_instances,
                                 float target_device_utilization,
                                 const std::string& device_layout_name);

bool try_pack(t_packer_opts* packer_opts,
              const t_analysis_opts* analysis_opts,
              const t_arch* arch,
              const t_model* user_models,
              const t_model* library_models,
              float interc_delay,
              std::vector<t_lb_type_rr_node>* lb_type_rr_graphs) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    const DeviceContext& device_ctx = g_vpr_ctx.device();

    std::unordered_set<AtomNetId> is_clock, is_global;
    t_clustering_data clustering_data;
    VTR_LOG("Begin packing '%s'.\n", packer_opts->circuit_file_name.c_str());

    is_clock = alloc_and_load_is_clock();
    is_global.insert(is_clock.begin(), is_clock.end());

    size_t num_p_inputs = 0;
    size_t num_p_outputs = 0;
    for (auto blk_id : atom_ctx.nlist.blocks()) {
        auto type = atom_ctx.nlist.block_type(blk_id);
        if (type == AtomBlockType::INPAD) {
            ++num_p_inputs;
        } else if (type == AtomBlockType::OUTPAD) {
            ++num_p_outputs;
        }
    }

    VTR_LOG("\n");
    VTR_LOG("After removing unused inputs...\n");
    VTR_LOG("\ttotal blocks: %zu, total nets: %zu, total inputs: %zu, total outputs: %zu\n",
            atom_ctx.nlist.blocks().size(), atom_ctx.nlist.nets().size(), num_p_inputs, num_p_outputs);

    // Run the prepacker, packing the atoms into molecules.
    // The Prepacker object performs prepacking and stores the pack molecules.
    // As long as the molecules are used, this object must persist.
    VTR_LOG("Begin prepacking.\n");
    Prepacker prepacker;
    prepacker.init(atom_ctx.nlist, device_ctx.logical_block_types);

    /* We keep attraction groups off in the first iteration,  and
     * only turn on in later iterations if some floorplan regions turn out to be overfull.
     */
    AttractionInfo attraction_groups(false);
    VTR_LOG("%d attraction groups were created during prepacking.\n", attraction_groups.num_attraction_groups());
    VTR_LOG("Finish prepacking.\n");

    if (packer_opts->auto_compute_inter_cluster_net_delay) {
        packer_opts->inter_cluster_net_delay = interc_delay;
        VTR_LOG("Using inter-cluster delay: %g\n", packer_opts->inter_cluster_net_delay);
    }

    // During clustering, a block is related to un-clustered primitives with nets.
    // This relation has three types: low fanout, high fanout, and transitive
    // high_fanout_thresholds stores the threshold for nets to a block type to
    // be considered high fanout.
    t_pack_high_fanout_thresholds high_fanout_thresholds(packer_opts->high_fanout_threshold);

    bool allow_unrelated_clustering = false;
    if (packer_opts->allow_unrelated_clustering == e_unrelated_clustering::ON) {
        allow_unrelated_clustering = true;
    } else if (packer_opts->allow_unrelated_clustering == e_unrelated_clustering::OFF) {
        allow_unrelated_clustering = false;
    }

    bool balance_block_type_util = false;
    if (packer_opts->balance_block_type_utilization == e_balance_block_type_util::ON) {
        balance_block_type_util = true;
    } else if (packer_opts->balance_block_type_utilization == e_balance_block_type_util::OFF) {
        balance_block_type_util = false;
    }

    int pack_iteration = 1;
    bool floorplan_regions_overfull = false;

    // Initialize the cluster legalizer.
    ClusterLegalizer cluster_legalizer(atom_ctx.nlist,
                                       prepacker,
                                       device_ctx.logical_block_types,
                                       lb_type_rr_graphs,
                                       user_models,
                                       library_models,
                                       packer_opts->target_external_pin_util,
                                       high_fanout_thresholds,
                                       ClusterLegalizationStrategy::SKIP_INTRA_LB_ROUTE,
                                       packer_opts->enable_pin_feasibility_filter,
                                       packer_opts->feasible_block_array_size,
                                       packer_opts->pack_verbosity);

    VTR_LOG("Packing with pin utilization targets: %s\n", cluster_legalizer.get_target_external_pin_util().to_string().c_str());
    VTR_LOG("Packing with high fanout thresholds: %s\n", high_fanout_thresholds.to_string().c_str());

    while (true) {
        free_clustering_data(*packer_opts, clustering_data);


        //Cluster the netlist
        //  num_used_type_instances: A map used to save the number of used
        //                           instances from each logical block type.
        std::map<t_logical_block_type_ptr, size_t> num_used_type_instances;
        num_used_type_instances = do_clustering(*packer_opts,
                                                *analysis_opts,
                                                arch,
                                                prepacker,
                                                cluster_legalizer,
                                                is_clock,
                                                is_global,
                                                allow_unrelated_clustering,
                                                balance_block_type_util,
                                                attraction_groups,
                                                floorplan_regions_overfull,
                                                high_fanout_thresholds,
                                                clustering_data);

        //Try to size/find a device
        bool fits_on_device = try_size_device_grid(*arch, num_used_type_instances, packer_opts->target_device_utilization, packer_opts->device_layout);

        /* We use this bool to determine the cause for the clustering not being dense enough. If the clustering
         * is not dense enough and there are floorplan constraints, it is presumed that the constraints are the cause
         * of the floorplan not fitting, so attraction groups are turned on for later iterations.
         */
        bool floorplan_not_fitting = (floorplan_regions_overfull || g_vpr_ctx.floorplanning().constraints.get_num_partitions() > 0);

        if (fits_on_device && !floorplan_regions_overfull) {
            break; //Done
        } else if (pack_iteration == 1 && !floorplan_not_fitting) {
            //1st pack attempt was unsuccessful (i.e. not dense enough) and we have control of unrelated clustering
            //
            //Turn it on to increase packing density
            if (packer_opts->allow_unrelated_clustering == e_unrelated_clustering::AUTO) {
                VTR_ASSERT(allow_unrelated_clustering == false);
                allow_unrelated_clustering = true;
            }
            if (packer_opts->balance_block_type_utilization == e_balance_block_type_util::AUTO) {
                VTR_ASSERT(balance_block_type_util == false);
                balance_block_type_util = true;
            }
            VTR_LOG("Packing failed to fit on device. Re-packing with: unrelated_logic_clustering=%s balance_block_type_util=%s\n",
                    (allow_unrelated_clustering ? "true" : "false"),
                    (balance_block_type_util ? "true" : "false"));
            /*
             * When running with tight floorplan constraints, some regions may become overfull with clusters (i.e.
             * the number of blocks assigned to the region exceeds the number of blocks available). When this occurs, we
             * cluster more densely to be able to adhere to the floorplan constraints. However, we do not want to cluster more
             * densely unnecessarily, as this can negatively impact wirelength. So, we have iterative approach. We check at the end
             * of every iteration if any floorplan regions are overfull. In the first iteration, we run
             * with no attraction groups (not packing more densely). If regions are overfull at the end of the first iteration,
             * we create attraction groups for partitions with overfull regions (pack those atoms more densely). We continue this way
             * until the last iteration, when we create attraction groups for every partition, if needed.
             */
        } else if (pack_iteration == 1 && floorplan_not_fitting) {
            VTR_LOG("Floorplan regions are overfull: trying to pack again using cluster attraction groups. \n");
            attraction_groups.create_att_groups_for_overfull_regions();
            attraction_groups.set_att_group_pulls(1);

        } else if (pack_iteration >= 2 && pack_iteration < 5 && floorplan_not_fitting) {
            if (pack_iteration == 2) {
                VTR_LOG("Floorplan regions are overfull: trying to pack again with more attraction groups exploration. \n");
                attraction_groups.create_att_groups_for_overfull_regions();
                VTR_LOG("Pack iteration is %d\n", pack_iteration);
            } else if (pack_iteration == 3) {
                attraction_groups.create_att_groups_for_all_regions();
                VTR_LOG("Floorplan regions are overfull: trying to pack again with more attraction groups exploration. \n");
                VTR_LOG("Pack iteration is %d\n", pack_iteration);
            } else if (pack_iteration == 4) {
                attraction_groups.create_att_groups_for_all_regions();
                VTR_LOG("Floorplan regions are overfull: trying to pack again with more attraction groups exploration and higher target pin utilization. \n");
                VTR_LOG("Pack iteration is %d\n", pack_iteration);
                attraction_groups.set_att_group_pulls(4);
                t_ext_pin_util pin_util(1.0, 1.0);
                // TODO: This line assumes the logic block name is "clb" which
                //       may not be the case. This may need to be investigated.
                //       Probably we should do this update of ext_pin_util for
                //       all types that were overused. Or if that is hard, just
                //       do it for all block types. Doing it only for a clb
                //       string is dangerous -VB.
                cluster_legalizer.get_target_external_pin_util().set_block_pin_util("clb", pin_util);
            }

        } else { //Unable to pack densely enough: Give Up
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

                resource_reqs += std::string(iter->first->name) + ": " + std::to_string(iter->second);

                int num_instances = 0;
                for (auto type : iter->first->equivalent_tiles)
                    num_instances += grid.num_instances(type, -1);

                resource_avail += std::string(iter->first->name) + ": " + std::to_string(num_instances);
            }

            VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Failed to find device which satisfies resource requirements required: %s (available %s)", resource_reqs.c_str(), resource_avail.c_str());
        }

        //Reset clustering for re-packing
        for (auto blk : g_vpr_ctx.atom().nlist.blocks()) {
            g_vpr_ctx.mutable_atom().lookup.set_atom_clb(blk, ClusterBlockId::INVALID());
            g_vpr_ctx.mutable_atom().lookup.set_atom_pb(blk, nullptr);
        }
        for (auto net : g_vpr_ctx.atom().nlist.nets()) {
            g_vpr_ctx.mutable_atom().lookup.remove_atom_net(net);
        }
        g_vpr_ctx.mutable_floorplanning().cluster_constraints.clear();
        //attraction_groups.reset_attraction_groups();

        // Reset the cluster legalizer for re-clustering.
        cluster_legalizer.reset();

        ++pack_iteration;
    }

    /* Packing iterative improvement can be done here */
    /*       Use the re-cluster API to edit it        */
    /******************* Start *************************/
    VTR_LOG("Start the iterative improvement process\n");
    //iteratively_improve_packing(*packer_opts, clustering_data, 2);
    VTR_LOG("the iterative improvement process is done\n");

    /*
     * auto& cluster_ctx = g_vpr_ctx.clustering();
     * for (auto& blk_id : g_vpr_ctx.clustering().clb_nlist.blocks()) {
     * free_pb_stats_recursive(cluster_ctx.clb_nlist.block_pb(blk_id));
     * }
     */
    /******************** End **************************/

    //check clustering and output it
    check_and_output_clustering(cluster_legalizer, *packer_opts, is_clock, arch);

    // Free Data Structures
    free_clustering_data(*packer_opts, clustering_data);

    VTR_LOG("\n");
    VTR_LOG("Netlist conversion complete.\n");
    VTR_LOG("\n");

    return true;
}

float get_arch_switch_info(short switch_index, int switch_fanin, float& Tdel_switch, float& R_switch, float& Cout_switch) {
    /* Fetches delay, resistance and output capacitance of the architecture switch at switch_index.
     * Returns the total delay through the switch. Used to calculate inter-cluster net delay. */

    /* The intrinsic delay may depend on fanin to the switch. If the delay map of a
     * switch from the architecture file has multiple (#inputs, delay) entries, we
     * interpolate/extrapolate to get the delay at 'switch_fanin'. */
    auto& device_ctx = g_vpr_ctx.device();

    Tdel_switch = device_ctx.arch_switch_inf[switch_index].Tdel(switch_fanin);
    R_switch = device_ctx.arch_switch_inf[switch_index].R;
    Cout_switch = device_ctx.arch_switch_inf[switch_index].Cout;

    /* The delay through a loaded switch is its intrinsic (unloaded)
     * delay plus the product of its resistance and output capacitance. */
    return Tdel_switch + R_switch * Cout_switch;
}

std::unordered_set<AtomNetId> alloc_and_load_is_clock() {
    /* Looks through all the atom blocks to find and mark all the clocks, by setting
     * the corresponding entry by adding the clock to is_clock.
     * only for an error check.                                                */

    std::unordered_set<AtomNetId> is_clock;

    /* Want to identify all the clock nets.  */
    auto& atom_ctx = g_vpr_ctx.atom();

    for (auto blk_id : atom_ctx.nlist.blocks()) {
        for (auto pin_id : atom_ctx.nlist.block_clock_pins(blk_id)) {
            auto net_id = atom_ctx.nlist.pin_net(pin_id);
            if (!is_clock.count(net_id)) {
                is_clock.insert(net_id);
            }
        }
    }

    return (is_clock);
}

static bool try_size_device_grid(const t_arch& arch,
                                 const std::map<t_logical_block_type_ptr, size_t>& num_type_instances,
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
    std::map<t_logical_block_type_ptr, float> type_util;
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
        VTR_LOG("\tBlock Utilization: %.2f Type: %s\n", util, type.name);
    }
    VTR_LOG("\n");

    return fits_on_device;
}

