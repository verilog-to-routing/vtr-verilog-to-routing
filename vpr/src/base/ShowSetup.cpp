#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_memory.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "echo_files.h"
#include "read_options.h"
#include "read_xml_arch_file.h"
#include "ShowSetup.h"

/******** Function Prototypes ********/
static void ShowPackerOpts(const t_packer_opts& PackerOpts);
static void ShowNetlistOpts(const t_netlist_opts& NetlistOpts);
static void ShowPlacerOpts(const t_placer_opts& PlacerOpts,
                           const t_annealing_sched& AnnealSched);
static void ShowRouterOpts(const t_router_opts& RouterOpts);
static void ShowAnalysisOpts(const t_analysis_opts& AnalysisOpts);

static void ShowAnnealSched(const t_annealing_sched& AnnealSched);
static void ShowRoutingArch(const t_det_routing_arch& RoutingArch);

/******** Function Implementations ********/

void ShowSetup(const t_vpr_setup& vpr_setup) {
    VTR_LOG("Timing analysis: %s\n", (vpr_setup.TimingEnabled ? "ON" : "OFF"));

    VTR_LOG("Circuit netlist file: %s\n", vpr_setup.FileNameOpts.NetFile.c_str());
    VTR_LOG("Circuit placement file: %s\n", vpr_setup.FileNameOpts.PlaceFile.c_str());
    VTR_LOG("Circuit routing file: %s\n", vpr_setup.FileNameOpts.RouteFile.c_str());
    VTR_LOG("Circuit SDC file: %s\n", vpr_setup.Timing.SDCFile.c_str());
    VTR_LOG("\n");

    VTR_LOG("Packer: %s\n", (vpr_setup.PackerOpts.doPacking ? "ENABLED" : "DISABLED"));
    VTR_LOG("Placer: %s\n", (vpr_setup.PlacerOpts.doPlacement ? "ENABLED" : "DISABLED"));
    VTR_LOG("Router: %s\n", (vpr_setup.RouterOpts.doRouting ? "ENABLED" : "DISABLED"));
    VTR_LOG("Analysis: %s\n", (vpr_setup.AnalysisOpts.doAnalysis ? "ENABLED" : "DISABLED"));
    VTR_LOG("\n");

    ShowNetlistOpts(vpr_setup.NetlistOpts);

    if (vpr_setup.PackerOpts.doPacking) {
        ShowPackerOpts(vpr_setup.PackerOpts);
    }
    if (vpr_setup.PlacerOpts.doPlacement) {
        ShowPlacerOpts(vpr_setup.PlacerOpts, vpr_setup.AnnealSched);
    }
    if (vpr_setup.RouterOpts.doRouting) {
        ShowRouterOpts(vpr_setup.RouterOpts);
    }
    if (vpr_setup.AnalysisOpts.doAnalysis) {
        ShowAnalysisOpts(vpr_setup.AnalysisOpts);
    }

    if (DETAILED == vpr_setup.RouterOpts.route_type)
        ShowRoutingArch(vpr_setup.RoutingArch);
}

void printClusteredNetlistStats() {
    int i, j, L_num_p_inputs, L_num_p_outputs;
    int* num_blocks_type;

    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    num_blocks_type = (int*)vtr::calloc(device_ctx.num_block_types, sizeof(int));

    VTR_LOG("\n");
    VTR_LOG("Netlist num_nets: %d\n", (int)cluster_ctx.clb_nlist.nets().size());
    VTR_LOG("Netlist num_blocks: %d\n", (int)cluster_ctx.clb_nlist.blocks().size());

    for (i = 0; i < device_ctx.num_block_types; i++) {
        num_blocks_type[i] = 0;
    }
    /* Count I/O input and output pads */
    L_num_p_inputs = 0;
    L_num_p_outputs = 0;

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        num_blocks_type[cluster_ctx.clb_nlist.block_type(blk_id)->index]++;
        auto type = cluster_ctx.clb_nlist.block_type(blk_id);
        if (is_io_type(type)) {
            for (j = 0; j < type->num_pins; j++) {
                if (cluster_ctx.clb_nlist.block_net(blk_id, j) != ClusterNetId::INVALID()) {
                    if (type->class_inf[type->pin_class[j]].type == DRIVER) {
                        L_num_p_inputs++;
                    } else {
                        VTR_ASSERT(type->class_inf[type->pin_class[j]].type == RECEIVER);
                        L_num_p_outputs++;
                    }
                }
            }
        }
    }

    for (i = 0; i < device_ctx.num_block_types; i++) {
        VTR_LOG("Netlist %s blocks: %d.\n", device_ctx.block_types[i].name, num_blocks_type[i]);
    }

    /* Print out each block separately instead */
    VTR_LOG("Netlist inputs pins: %d\n", L_num_p_inputs);
    VTR_LOG("Netlist output pins: %d\n", L_num_p_outputs);
    VTR_LOG("\n");
    free(num_blocks_type);
}

static void ShowRoutingArch(const t_det_routing_arch& RoutingArch) {
    VTR_LOG("RoutingArch.directionality: ");
    switch (RoutingArch.directionality) {
        case BI_DIRECTIONAL:
            VTR_LOG("BI_DIRECTIONAL\n");
            break;
        case UNI_DIRECTIONAL:
            VTR_LOG("UNI_DIRECTIONAL\n");
            break;
        default:
            vpr_throw(VPR_ERROR_UNKNOWN, __FILE__, __LINE__, "<Unknown>\n");
            break;
    }

    VTR_LOG("RoutingArch.switch_block_type: ");
    switch (RoutingArch.switch_block_type) {
        case SUBSET:
            VTR_LOG("SUBSET\n");
            break;
        case WILTON:
            VTR_LOG("WILTON\n");
            break;
        case UNIVERSAL:
            VTR_LOG("UNIVERSAL\n");
            break;
        case FULL:
            VTR_LOG("FULL\n");
            break;
        case CUSTOM:
            VTR_LOG("CUSTOM\n");
            break;
        default:
            VTR_LOG_ERROR("switch block type\n");
    }

    VTR_LOG("RoutingArch.Fs: %d\n", RoutingArch.Fs);

    VTR_LOG("\n");
}

static void ShowAnnealSched(const t_annealing_sched& AnnealSched) {
    VTR_LOG("AnnealSched.type: ");
    switch (AnnealSched.type) {
        case AUTO_SCHED:
            VTR_LOG("AUTO_SCHED\n");
            break;
        case USER_SCHED:
            VTR_LOG("USER_SCHED\n");
            break;
        default:
            VTR_LOG_ERROR("Unknown annealing schedule\n");
    }

    VTR_LOG("AnnealSched.inner_num: %f\n", AnnealSched.inner_num);

    if (USER_SCHED == AnnealSched.type) {
        VTR_LOG("AnnealSched.init_t: %f\n", AnnealSched.init_t);
        VTR_LOG("AnnealSched.alpha_t: %f\n", AnnealSched.alpha_t);
        VTR_LOG("AnnealSched.exit_t: %f\n", AnnealSched.exit_t);
    }
}

static void ShowRouterOpts(const t_router_opts& RouterOpts) {
    VTR_LOG("RouterOpts.route_type: ");
    switch (RouterOpts.route_type) {
        case GLOBAL:
            VTR_LOG("GLOBAL\n");
            break;
        case DETAILED:
            VTR_LOG("DETAILED\n");
            break;
        default:
            VTR_LOG_ERROR("Unknown router opt\n");
    }

    if (DETAILED == RouterOpts.route_type) {
        VTR_LOG("RouterOpts.router_algorithm: ");
        switch (RouterOpts.router_algorithm) {
            case BREADTH_FIRST:
                VTR_LOG("BREADTH_FIRST\n");
                break;
            case TIMING_DRIVEN:
                VTR_LOG("TIMING_DRIVEN\n");
                break;
            default:
                vpr_throw(VPR_ERROR_UNKNOWN, __FILE__, __LINE__, "<Unknown>\n");
        }

        VTR_LOG("RouterOpts.base_cost_type: ");
        switch (RouterOpts.base_cost_type) {
            case DELAY_NORMALIZED:
                VTR_LOG("DELAY_NORMALIZED\n");
                break;
            case DELAY_NORMALIZED_LENGTH:
                VTR_LOG("DELAY_NORMALIZED_LENGTH\n");
                break;
            case DELAY_NORMALIZED_FREQUENCY:
                VTR_LOG("DELAY_NORMALIZED_FREQUENCY\n");
                break;
            case DELAY_NORMALIZED_LENGTH_FREQUENCY:
                VTR_LOG("DELAY_NORMALIZED_LENGTH_FREQUENCY\n");
                break;
            case DEMAND_ONLY:
                VTR_LOG("DEMAND_ONLY\n");
                break;
            case DEMAND_ONLY_NORMALIZED_LENGTH:
                VTR_LOG("DEMAND_ONLY_NORMALIZED_LENGTH\n");
                break;
            default:
                vpr_throw(VPR_ERROR_UNKNOWN, __FILE__, __LINE__, "Unknown base_cost_type\n");
        }

        VTR_LOG("RouterOpts.fixed_channel_width: ");
        if (NO_FIXED_CHANNEL_WIDTH == RouterOpts.fixed_channel_width) {
            VTR_LOG("NO_FIXED_CHANNEL_WIDTH\n");
        } else {
            VTR_LOG("%d\n", RouterOpts.fixed_channel_width);
        }

        VTR_LOG("RouterOpts.trim_empty_chan: %s\n", (RouterOpts.trim_empty_channels ? "true" : "false"));
        VTR_LOG("RouterOpts.trim_obs_chan: %s\n", (RouterOpts.trim_obs_channels ? "true" : "false"));
        VTR_LOG("RouterOpts.acc_fac: %f\n", RouterOpts.acc_fac);
        VTR_LOG("RouterOpts.bb_factor: %d\n", RouterOpts.bb_factor);
        VTR_LOG("RouterOpts.bend_cost: %f\n", RouterOpts.bend_cost);
        VTR_LOG("RouterOpts.first_iter_pres_fac: %f\n", RouterOpts.first_iter_pres_fac);
        VTR_LOG("RouterOpts.initial_pres_fac: %f\n", RouterOpts.initial_pres_fac);
        VTR_LOG("RouterOpts.pres_fac_mult: %f\n", RouterOpts.pres_fac_mult);
        VTR_LOG("RouterOpts.max_router_iterations: %d\n", RouterOpts.max_router_iterations);
        VTR_LOG("RouterOpts.min_incremental_reroute_fanout: %d\n", RouterOpts.min_incremental_reroute_fanout);

        if (TIMING_DRIVEN == RouterOpts.router_algorithm) {
            VTR_LOG("RouterOpts.astar_fac: %f\n", RouterOpts.astar_fac);
            VTR_LOG("RouterOpts.criticality_exp: %f\n", RouterOpts.criticality_exp);
            VTR_LOG("RouterOpts.max_criticality: %f\n", RouterOpts.max_criticality);
        }
        if (RouterOpts.routing_failure_predictor == SAFE)
            VTR_LOG("RouterOpts.routing_failure_predictor = SAFE\n");
        else if (RouterOpts.routing_failure_predictor == AGGRESSIVE)
            VTR_LOG("RouterOpts.routing_failure_predictor = AGGRESSIVE\n");
        else if (RouterOpts.routing_failure_predictor == OFF)
            VTR_LOG("RouterOpts.routing_failure_predictor = OFF\n");

        if (RouterOpts.routing_budgets_algorithm == DISABLE) {
            VTR_LOG("RouterOpts.routing_budgets_algorithm = DISABLE\n");
        } else if (RouterOpts.routing_budgets_algorithm == MINIMAX) {
            VTR_LOG("RouterOpts.routing_budgets_algorithm = MINIMAX\n");
        } else if (RouterOpts.routing_budgets_algorithm == SCALE_DELAY) {
            VTR_LOG("RouterOpts.routing_budgets_algorithm = SCALE_DELAY\n");
        }

    } else {
        VTR_ASSERT(GLOBAL == RouterOpts.route_type);

        VTR_LOG("RouterOpts.router_algorithm: ");
        switch (RouterOpts.router_algorithm) {
            case BREADTH_FIRST:
                VTR_LOG("BREADTH_FIRST\n");
                break;
            case TIMING_DRIVEN:
                VTR_LOG("TIMING_DRIVEN\n");
                break;
            default:
                VTR_LOG_ERROR("Unknown router algorithm\n");
        }

        VTR_LOG("RouterOpts.base_cost_type: ");
        switch (RouterOpts.base_cost_type) {
            case DELAY_NORMALIZED:
                VTR_LOG("DELAY_NORMALIZED\n");
                break;
            case DEMAND_ONLY:
                VTR_LOG("DEMAND_ONLY\n");
                break;
            default:
                VTR_LOG_ERROR("Unknown router base cost type\n");
        }

        VTR_LOG("RouterOpts.fixed_channel_width: ");
        if (NO_FIXED_CHANNEL_WIDTH == RouterOpts.fixed_channel_width) {
            VTR_LOG("NO_FIXED_CHANNEL_WIDTH\n");
        } else {
            VTR_LOG("%d\n", RouterOpts.fixed_channel_width);
        }

        VTR_LOG("RouterOpts.trim_empty_chan: %s\n", (RouterOpts.trim_empty_channels ? "true" : "false"));
        VTR_LOG("RouterOpts.trim_obs_chan: %s\n", (RouterOpts.trim_obs_channels ? "true" : "false"));
        VTR_LOG("RouterOpts.acc_fac: %f\n", RouterOpts.acc_fac);
        VTR_LOG("RouterOpts.bb_factor: %d\n", RouterOpts.bb_factor);
        VTR_LOG("RouterOpts.bend_cost: %f\n", RouterOpts.bend_cost);
        VTR_LOG("RouterOpts.first_iter_pres_fac: %f\n", RouterOpts.first_iter_pres_fac);
        VTR_LOG("RouterOpts.initial_pres_fac: %f\n", RouterOpts.initial_pres_fac);
        VTR_LOG("RouterOpts.pres_fac_mult: %f\n", RouterOpts.pres_fac_mult);
        VTR_LOG("RouterOpts.max_router_iterations: %d\n", RouterOpts.max_router_iterations);
        VTR_LOG("RouterOpts.min_incremental_reroute_fanout: %d\n", RouterOpts.min_incremental_reroute_fanout);
        if (TIMING_DRIVEN == RouterOpts.router_algorithm) {
            VTR_LOG("RouterOpts.astar_fac: %f\n", RouterOpts.astar_fac);
            VTR_LOG("RouterOpts.criticality_exp: %f\n", RouterOpts.criticality_exp);
            VTR_LOG("RouterOpts.max_criticality: %f\n", RouterOpts.max_criticality);
        }
    }
    VTR_LOG("\n");
}

static void ShowPlacerOpts(const t_placer_opts& PlacerOpts,
                           const t_annealing_sched& AnnealSched) {
    VTR_LOG("PlacerOpts.place_freq: ");
    switch (PlacerOpts.place_freq) {
        case PLACE_ONCE:
            VTR_LOG("PLACE_ONCE\n");
            break;
        case PLACE_ALWAYS:
            VTR_LOG("PLACE_ALWAYS\n");
            break;
        case PLACE_NEVER:
            VTR_LOG("PLACE_NEVER\n");
            break;
        default:
            VTR_LOG_ERROR("Unknown Place Freq\n");
    }
    if ((PLACE_ONCE == PlacerOpts.place_freq)
        || (PLACE_ALWAYS == PlacerOpts.place_freq)) {
        VTR_LOG("PlacerOpts.place_algorithm: ");
        switch (PlacerOpts.place_algorithm) {
            case BOUNDING_BOX_PLACE:
                VTR_LOG("BOUNDING_BOX_PLACE\n");
                break;
            case PATH_TIMING_DRIVEN_PLACE:
                VTR_LOG("PATH_TIMING_DRIVEN_PLACE\n");
                break;
            default:
                VTR_LOG_ERROR("Unknown placement algorithm\n");
        }

        VTR_LOG("PlacerOpts.pad_loc_type: ");
        switch (PlacerOpts.pad_loc_type) {
            case FREE:
                VTR_LOG("FREE\n");
                break;
            case RANDOM:
                VTR_LOG("RANDOM\n");
                break;
            case USER:
                VTR_LOG("USER '%s'\n", PlacerOpts.pad_loc_file.c_str());
                break;
            default:
                vpr_throw(VPR_ERROR_UNKNOWN, __FILE__, __LINE__, "Unknown I/O pad location type\n");
        }

        VTR_LOG("PlacerOpts.place_cost_exp: %f\n", PlacerOpts.place_cost_exp);

        VTR_LOG("PlacerOpts.place_chan_width: %d\n", PlacerOpts.place_chan_width);

        if (PATH_TIMING_DRIVEN_PLACE == PlacerOpts.place_algorithm) {
            VTR_LOG("PlacerOpts.inner_loop_recompute_divider: %d\n", PlacerOpts.inner_loop_recompute_divider);
            VTR_LOG("PlacerOpts.recompute_crit_iter: %d\n", PlacerOpts.recompute_crit_iter);
            VTR_LOG("PlacerOpts.timing_tradeoff: %f\n", PlacerOpts.timing_tradeoff);
            VTR_LOG("PlacerOpts.td_place_exp_first: %f\n", PlacerOpts.td_place_exp_first);
            VTR_LOG("PlacerOpts.td_place_exp_last: %f\n", PlacerOpts.td_place_exp_last);
        }

        VTR_LOG("PlaceOpts.seed: %d\n", PlacerOpts.seed);

        ShowAnnealSched(AnnealSched);
    }
    VTR_LOG("\n");
}

static void ShowNetlistOpts(const t_netlist_opts& NetlistOpts) {
    VTR_LOG("NetlistOpts.abosrb_buffer_luts            : %s\n", (NetlistOpts.absorb_buffer_luts) ? "true" : "false");
    VTR_LOG("NetlistOpts.sweep_dangling_primary_ios    : %s\n", (NetlistOpts.sweep_dangling_primary_ios) ? "true" : "false");
    VTR_LOG("NetlistOpts.sweep_dangling_nets           : %s\n", (NetlistOpts.sweep_dangling_nets) ? "true" : "false");
    VTR_LOG("NetlistOpts.sweep_dangling_blocks         : %s\n", (NetlistOpts.sweep_dangling_blocks) ? "true" : "false");
    VTR_LOG("NetlistOpts.sweep_constant_primary_outputs: %s\n", (NetlistOpts.sweep_constant_primary_outputs) ? "true" : "false");
    VTR_LOG("\n");
}

static void ShowAnalysisOpts(const t_analysis_opts& AnalysisOpts) {
    VTR_LOG("AnalysisOpts.gen_post_synthesis_netlist: %s\n", (AnalysisOpts.gen_post_synthesis_netlist) ? "true" : "false");
    VTR_LOG("\n");
}

static void ShowPackerOpts(const t_packer_opts& PackerOpts) {
    VTR_LOG("PackerOpts.allow_unrelated_clustering: ");
    if (PackerOpts.allow_unrelated_clustering == e_unrelated_clustering::ON) {
        VTR_LOG("true\n");
    } else if (PackerOpts.allow_unrelated_clustering == e_unrelated_clustering::OFF) {
        VTR_LOG("false\n");
    } else if (PackerOpts.allow_unrelated_clustering == e_unrelated_clustering::AUTO) {
        VTR_LOG("auto\n");
    } else {
        vpr_throw(VPR_ERROR_UNKNOWN, __FILE__, __LINE__, "Unknown packer allow_unrelated_clustering\n");
    }
    VTR_LOG("PackerOpts.alpha_clustering: %f\n", PackerOpts.alpha);
    VTR_LOG("PackerOpts.beta_clustering: %f\n", PackerOpts.beta);
    VTR_LOG("PackerOpts.cluster_seed_type: ");
    switch (PackerOpts.cluster_seed_type) {
        case e_cluster_seed::TIMING:
            VTR_LOG("TIMING\n");
            break;
        case e_cluster_seed::MAX_INPUTS:
            VTR_LOG("MAX_INPUTS\n");
            break;
        case e_cluster_seed::BLEND:
            VTR_LOG("BLEND\n");
            break;
        case e_cluster_seed::MAX_PINS:
            VTR_LOG("MAX_PINS\n");
            break;
        case e_cluster_seed::MAX_INPUT_PINS:
            VTR_LOG("MAX_INPUT_PINS\n");
            break;
        case e_cluster_seed::BLEND2:
            VTR_LOG("BLEND2\n");
            break;
        default:
            vpr_throw(VPR_ERROR_UNKNOWN, __FILE__, __LINE__, "Unknown packer cluster_seed_type\n");
    }
    VTR_LOG("PackerOpts.connection_driven: %s", (PackerOpts.connection_driven ? "true\n" : "false\n"));
    VTR_LOG("PackerOpts.global_clocks: %s", (PackerOpts.global_clocks ? "true\n" : "false\n"));
    VTR_LOG("PackerOpts.hill_climbing_flag: %s", (PackerOpts.hill_climbing_flag ? "true\n" : "false\n"));
    VTR_LOG("PackerOpts.inter_cluster_net_delay: %f\n", PackerOpts.inter_cluster_net_delay);
    VTR_LOG("PackerOpts.timing_driven: %s", (PackerOpts.timing_driven ? "true\n" : "false\n"));
    VTR_LOG("PackerOpts.target_external_pin_util: %s", vtr::join(PackerOpts.target_external_pin_util, " ").c_str());
    VTR_LOG("\n");
}
