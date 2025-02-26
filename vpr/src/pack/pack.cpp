
#include "pack.h"

#include <unordered_set>
#include "FlatPlacementInfo.h"
#include "SetupGrid.h"
#include "attraction_groups.h"
#include "cluster_legalizer.h"
#include "cluster_util.h"
#include "constraints_report.h"
#include "globals.h"
#include "greedy_clusterer.h"
#include "partition_region.h"
#include "physical_types_util.h"
#include "prepack.h"
#include "verify_flat_placement.h"
#include "vpr_context.h"
#include "vpr_error.h"
#include "vpr_types.h"
#include "vtr_assert.h"
#include "vtr_log.h"

static bool try_size_device_grid(const t_arch& arch,
                                 const std::map<t_logical_block_type_ptr, size_t>& num_type_instances,
                                 float target_device_utilization,
                                 const std::string& device_layout_name);

/**
 * Since the parameters of a switch may change as a function of its fanin,
 * to get an estimation of inter-cluster delays we need a reasonable estimation
 * of the fan-ins of switches that connect clusters together. These switches are
 * 1) opin to wire switch
 * 2) wire to wire switch
 * 3) wire to ipin switch
 * We can estimate the fan-in of these switches based on the Fc_in/Fc_out of
 * a logic block, and the switch block Fs value
 */
static void get_intercluster_switch_fanin_estimates(const t_arch& arch,
                                                    const t_det_routing_arch& routing_arch,
                                                    const std::string& device_layout,
                                                    const int wire_segment_length,
                                                    int* opin_switch_fanin,
                                                    int* wire_switch_fanin,
                                                    int* ipin_switch_fanin);

static float get_arch_switch_info(short switch_index, int switch_fanin,
                                  float& Tdel_switch, float& R_switch,
                                  float& Cout_switch);

static float approximate_inter_cluster_delay(const t_arch& arch,
                                             const t_det_routing_arch& routing_arch,
                                             const std::string& device_layout);

bool try_pack(t_packer_opts* packer_opts,
              const t_analysis_opts* analysis_opts,
              const t_arch& arch,
              const t_det_routing_arch& routing_arch,
              std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
              const FlatPlacementInfo& flat_placement_info) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    // The clusterer modifies the device context by increasing the size of the
    // device if needed.
    DeviceContext& mutable_device_ctx = g_vpr_ctx.mutable_device();

    std::unordered_set<AtomNetId> is_clock, is_global;
    VTR_LOG("Begin packing '%s'.\n", packer_opts->circuit_file_name.c_str());

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

    // Run the prepacker, packing the atoms into molecules.
    // The Prepacker object performs prepacking and stores the pack molecules.
    // As long as the molecules are used, this object must persist.
    VTR_LOG("Begin prepacking.\n");
    const Prepacker prepacker(atom_ctx.netlist(), device_ctx.logical_block_types);

    /* We keep attraction groups off in the first iteration,  and
     * only turn on in later iterations if some floorplan regions turn out to be overfull.
     */
    AttractionInfo attraction_groups(false);
    VTR_LOG("%d attraction groups were created during prepacking.\n", attraction_groups.num_attraction_groups());
    VTR_LOG("Finish prepacking.\n");

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

    if (packer_opts->auto_compute_inter_cluster_net_delay) {
        float interc_delay = UNDEFINED;
        if (packer_opts->timing_driven) {
            interc_delay = approximate_inter_cluster_delay(arch,
                                                           routing_arch,
                                                           packer_opts->device_layout);
        }
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

    // Initialize the cluster legalizer.
    ClusterLegalizer cluster_legalizer(atom_ctx.netlist(),
                                       prepacker,
                                       lb_type_rr_graphs,
                                       packer_opts->target_external_pin_util,
                                       high_fanout_thresholds,
                                       ClusterLegalizationStrategy::SKIP_INTRA_LB_ROUTE,
                                       packer_opts->enable_pin_feasibility_filter,
                                       packer_opts->pack_verbosity);

    VTR_LOG("Packing with pin utilization targets: %s\n", cluster_legalizer.get_target_external_pin_util().to_string().c_str());
    VTR_LOG("Packing with high fanout thresholds: %s\n", high_fanout_thresholds.to_string().c_str());

    // Initialize the greedy clusterer.
    GreedyClusterer clusterer(*packer_opts,
                              *analysis_opts,
                              atom_ctx.netlist(),
                              arch,
                              high_fanout_thresholds,
                              is_clock,
                              is_global,
                              flat_placement_info);

    while (true) {
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

        //Try to size/find a device
        bool fits_on_device = try_size_device_grid(arch, num_used_type_instances, packer_opts->target_device_utilization, packer_opts->device_layout);

        /* We use this bool to determine the cause for the clustering not being dense enough. If the clustering
         * is not dense enough and there are floorplan constraints, it is presumed that the constraints are the cause
         * of the floorplan not fitting, so attraction groups are turned on for later iterations.
         */
        bool floorplan_regions_overfull = floorplan_constraints_regions_overfull(overfull_partition_regions,
                                                                                 cluster_legalizer,
                                                                                 device_ctx.logical_block_types);

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
            attraction_groups.create_att_groups_for_overfull_regions(overfull_partition_regions);
            attraction_groups.set_att_group_pulls(1);

        } else if (pack_iteration >= 2 && pack_iteration < 5 && floorplan_not_fitting) {
            if (pack_iteration == 2) {
                VTR_LOG("Floorplan regions are overfull: trying to pack again with more attraction groups exploration. \n");
                attraction_groups.create_att_groups_for_overfull_regions(overfull_partition_regions);
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

                resource_reqs += iter->first->name + ": " + std::to_string(iter->second);

                int num_instances = 0;
                for (auto type : iter->first->equivalent_tiles)
                    num_instances += grid.num_instances(type, -1);

                resource_avail += iter->first->name + ": " + std::to_string(num_instances);
            }

            VPR_FATAL_ERROR(VPR_ERROR_OTHER, "Failed to find device which satisfies resource requirements required: %s (available %s)", resource_reqs.c_str(), resource_avail.c_str());
        }

        //Reset clustering for re-packing
        for (auto blk : g_vpr_ctx.atom().netlist().blocks()) {
            g_vpr_ctx.mutable_atom().mutable_lookup().set_atom_clb(blk, ClusterBlockId::INVALID());
            g_vpr_ctx.mutable_atom().mutable_lookup().set_atom_pb(blk, nullptr);
        }
        for (auto net : g_vpr_ctx.atom().netlist().nets()) {
            g_vpr_ctx.mutable_atom().mutable_lookup().remove_atom_net(net);
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
    check_and_output_clustering(cluster_legalizer, *packer_opts, is_clock, &arch);

    VTR_LOG("\n");
    VTR_LOG("Netlist conversion complete.\n");
    VTR_LOG("\n");

    return true;
}

static float get_arch_switch_info(short switch_index, int switch_fanin, float& Tdel_switch, float& R_switch, float& Cout_switch) {
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
        VTR_LOG("\tBlock Utilization: %.2f Type: %s\n", util, type.name.c_str());
    }
    VTR_LOG("\n");

    return fits_on_device;
}

static void get_intercluster_switch_fanin_estimates(const t_arch& arch,
                                                    const t_det_routing_arch& routing_arch,
                                                    const std::string& device_layout,
                                                    const int wire_segment_length,
                                                    int* opin_switch_fanin,
                                                    int* wire_switch_fanin,
                                                    int* ipin_switch_fanin) {
    // W is unknown pre-packing, so *if* we need W here, we will assume a value of 100
    constexpr int W = 100;

    //Build a dummy 10x10 device to determine the 'best' block type to use
    auto grid = create_device_grid(device_layout, arch.grid_layouts, 10, 10);

    auto type = find_most_common_tile_type(grid);
    /* get Fc_in/out for most common block (e.g. logic blocks) */
    VTR_ASSERT(!type->fc_specs.empty());

    //Estimate the maximum Fc_in/Fc_out
    float Fc_in = 0.f;
    float Fc_out = 0.f;
    for (const t_fc_specification& fc_spec : type->fc_specs) {
        float Fc = fc_spec.fc_value;

        if (fc_spec.fc_value_type == e_fc_value_type::ABSOLUTE) {
            //Convert to estimated fractional
            Fc /= W;
        }
        VTR_ASSERT_MSG(Fc >= 0 && Fc <= 1., "Fc should be fractional");

        for (int ipin : fc_spec.pins) {
            e_pin_type pin_type = get_pin_type_from_pin_physical_num(type, ipin);

            if (pin_type == DRIVER) {
                Fc_out = std::max(Fc, Fc_out);
            } else {
                VTR_ASSERT(pin_type == RECEIVER);
                Fc_in = std::max(Fc, Fc_in);
            }
        }
    }

    /* Estimates of switch fan-in are done as follows:
     * 1) opin to wire switch:
     * 2 CLBs connect to a channel, each with #opins/4 pins. Each pin has Fc_out*W
     * switches, and then we assume the switches are distributed evenly over the W wires.
     * In the unidirectional case, all these switches are then crammed down to W/wire_segment_length wires.
     *
     * Unidirectional: 2 * #opins_per_side * Fc_out * wire_segment_length
     * Bidirectional:  2 * #opins_per_side * Fc_out
     *
     * 2) wire to wire switch
     * A wire segment in a switchblock connects to Fs other wires. Assuming these connections are evenly
     * distributed, each target wire receives Fs connections as well. In the unidirectional case,
     * source wires can only connect to W/wire_segment_length wires.
     *
     * Unidirectional: Fs * wire_segment_length
     * Bidirectional:  Fs
     *
     * 3) wire to ipin switch
     * An input pin of a CLB simply receives Fc_in connections.
     *
     * Unidirectional: Fc_in
     * Bidirectional:  Fc_in
     */

    /* Fan-in to opin/ipin/wire switches depends on whether the architecture is unidirectional/bidirectional */
    (*opin_switch_fanin) = 2.f * type->num_drivers / 4.f * Fc_out;
    (*wire_switch_fanin) = routing_arch.Fs;
    (*ipin_switch_fanin) = Fc_in;
    if (routing_arch.directionality == UNI_DIRECTIONAL) {
        /* adjustments to opin-to-wire and wire-to-wire switch fan-ins */
        (*opin_switch_fanin) *= wire_segment_length;
        (*wire_switch_fanin) *= wire_segment_length;
    } else if (routing_arch.directionality == BI_DIRECTIONAL) {
        /* no adjustments need to be made here */
    } else {
        VPR_FATAL_ERROR(VPR_ERROR_PACK, "Unrecognized directionality: %d\n",
                        (int)routing_arch.directionality);
    }
}

static float approximate_inter_cluster_delay(const t_arch& arch,
                                             const t_det_routing_arch& routing_arch,
                                             const std::string& device_layout) {

    /* If needed, estimate inter-cluster delay. Assume the average routing hop goes out of
     * a block through an opin switch to a length-4 wire, then through a wire switch to another
     * length-4 wire, then through a wire-to-ipin-switch into another block. */
    constexpr int wire_segment_length = 4;

    /* We want to determine a reasonable fan-in to the opin, wire, and ipin switches, based
     * on which the intercluster delays can be estimated. The fan-in of a switch influences its
     * delay.
     *
     * The fan-in of the switch depends on the architecture (unidirectional/bidirectional), as
     * well as Fc_in/out and Fs */
    int opin_switch_fanin, wire_switch_fanin, ipin_switch_fanin;
    get_intercluster_switch_fanin_estimates(arch, routing_arch, device_layout, wire_segment_length, &opin_switch_fanin,
                                            &wire_switch_fanin, &ipin_switch_fanin);

    float Tdel_opin_switch, R_opin_switch, Cout_opin_switch;
    float opin_switch_del = get_arch_switch_info(arch.Segments[0].arch_opin_switch, opin_switch_fanin,
                                                 Tdel_opin_switch, R_opin_switch, Cout_opin_switch);

    float Tdel_wire_switch, R_wire_switch, Cout_wire_switch;
    float wire_switch_del = get_arch_switch_info(arch.Segments[0].arch_wire_switch, wire_switch_fanin,
                                                 Tdel_wire_switch, R_wire_switch, Cout_wire_switch);

    float Tdel_wtoi_switch, R_wtoi_switch, Cout_wtoi_switch;
    float wtoi_switch_del = get_arch_switch_info(routing_arch.wire_to_arch_ipin_switch, ipin_switch_fanin,
                                                 Tdel_wtoi_switch, R_wtoi_switch, Cout_wtoi_switch);

    float Rmetal = arch.Segments[0].Rmetal;
    float Cmetal = arch.Segments[0].Cmetal;

    /* The delay of a wire with its driving switch is the switch delay plus the
     * product of the equivalent resistance and capacitance experienced by the wire. */

    float first_wire_seg_delay = opin_switch_del
                                 + (R_opin_switch + Rmetal * (float)wire_segment_length / 2)
                                       * (Cout_opin_switch + Cmetal * (float)wire_segment_length);
    float second_wire_seg_delay = wire_switch_del
                                  + (R_wire_switch + Rmetal * (float)wire_segment_length / 2)
                                        * (Cout_wire_switch + Cmetal * (float)wire_segment_length);

    /* multiply by 4 to get a more conservative estimate */
    return 4 * (first_wire_seg_delay + second_wire_seg_delay + wtoi_switch_del);
}

