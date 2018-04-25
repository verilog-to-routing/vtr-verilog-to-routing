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
	vtr::printf_info("Timing analysis: %s\n", (vpr_setup.TimingEnabled? "ON" : "OFF"));
    vtr::printf_info("Slack definition: %s\n", vpr_setup.Timing.slack_definition.c_str());

	vtr::printf_info("Circuit netlist file: %s\n", vpr_setup.FileNameOpts.NetFile.c_str());
	vtr::printf_info("Circuit placement file: %s\n", vpr_setup.FileNameOpts.PlaceFile.c_str());
	vtr::printf_info("Circuit routing file: %s\n", vpr_setup.FileNameOpts.RouteFile.c_str());
	vtr::printf_info("Circuit SDC file: %s\n", vpr_setup.Timing.SDCFile.c_str());
	vtr::printf_info("\n");

	vtr::printf_info("Packer: %s\n", (vpr_setup.PackerOpts.doPacking ? "ENABLED" : "DISABLED"));
	vtr::printf_info("Placer: %s\n", (vpr_setup.PlacerOpts.doPlacement ? "ENABLED" : "DISABLED"));
	vtr::printf_info("Router: %s\n", (vpr_setup.RouterOpts.doRouting ? "ENABLED" : "DISABLED"));
	vtr::printf_info("Analysis: %s\n", (vpr_setup.AnalysisOpts.doAnalysis ? "ENABLED" : "DISABLED"));
	vtr::printf_info("\n");

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
	int *num_blocks_type;

    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

	num_blocks_type = (int*) vtr::calloc(device_ctx.num_block_types, sizeof(int));

	vtr::printf_info("\n");
	vtr::printf_info("Netlist num_nets: %d\n", (int)cluster_ctx.clb_nlist.nets().size());
	vtr::printf_info("Netlist num_blocks: %d\n", (int)cluster_ctx.clb_nlist.blocks().size());

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
						VTR_ASSERT(type->class_inf[type-> pin_class[j]]. type == RECEIVER);
						L_num_p_outputs++;
					}
				}
			}
		}
	}

	for (i = 0; i < device_ctx.num_block_types; i++) {
        vtr::printf_info("Netlist %s blocks: %d.\n", device_ctx.block_types[i].name, num_blocks_type[i]);
	}

	/* Print out each block separately instead */
	vtr::printf_info("Netlist inputs pins: %d\n", L_num_p_inputs);
	vtr::printf_info("Netlist output pins: %d\n", L_num_p_outputs);
	vtr::printf_info("\n");
	free(num_blocks_type);
}

static void ShowRoutingArch(const t_det_routing_arch& RoutingArch) {

	vtr::printf_info("RoutingArch.directionality: ");
	switch (RoutingArch.directionality) {
	case BI_DIRECTIONAL:
		vtr::printf_info("BI_DIRECTIONAL\n");
		break;
	case UNI_DIRECTIONAL:
		vtr::printf_info("UNI_DIRECTIONAL\n");
		break;
	default:
		vpr_throw(VPR_ERROR_UNKNOWN, __FILE__, __LINE__, "<Unknown>\n");
		break;
	}

	vtr::printf_info("RoutingArch.switch_block_type: ");
	switch (RoutingArch.switch_block_type) {
	case SUBSET:
		vtr::printf_info("SUBSET\n");
		break;
	case WILTON:
		vtr::printf_info("WILTON\n");
		break;
	case UNIVERSAL:
		vtr::printf_info("UNIVERSAL\n");
		break;
	case FULL:
		vtr::printf_info("FULL\n");
		break;
	case CUSTOM:
		vtr::printf_info("CUSTOM\n");
		break;
	default:
		vtr::printf_error(__FILE__, __LINE__, "switch block type\n");
	}

	vtr::printf_info("RoutingArch.Fs: %d\n", RoutingArch.Fs);

	vtr::printf_info("\n");
}

static void ShowAnnealSched(const t_annealing_sched& AnnealSched) {

	vtr::printf_info("AnnealSched.type: ");
	switch (AnnealSched.type) {
	case AUTO_SCHED:
		vtr::printf_info("AUTO_SCHED\n");
		break;
	case USER_SCHED:
		vtr::printf_info("USER_SCHED\n");
		break;
	default:
		vtr::printf_error(__FILE__, __LINE__, "Unknown annealing schedule\n");
	}

	vtr::printf_info("AnnealSched.inner_num: %f\n", AnnealSched.inner_num);

	if (USER_SCHED == AnnealSched.type) {
		vtr::printf_info("AnnealSched.init_t: %f\n", AnnealSched.init_t);
		vtr::printf_info("AnnealSched.alpha_t: %f\n", AnnealSched.alpha_t);
		vtr::printf_info("AnnealSched.exit_t: %f\n", AnnealSched.exit_t);
	}
}

static void ShowRouterOpts(const t_router_opts& RouterOpts) {

	vtr::printf_info("RouterOpts.route_type: ");
	switch (RouterOpts.route_type) {
	case GLOBAL:
		vtr::printf_info("GLOBAL\n");
		break;
	case DETAILED:
		vtr::printf_info("DETAILED\n");
		break;
	default:
		vtr::printf_error(__FILE__, __LINE__, "Unknown router opt\n");
	}

	if (DETAILED == RouterOpts.route_type) {
		vtr::printf_info("RouterOpts.router_algorithm: ");
		switch (RouterOpts.router_algorithm) {
		case BREADTH_FIRST:
			vtr::printf_info("BREADTH_FIRST\n");
			break;
		case TIMING_DRIVEN:
			vtr::printf_info("TIMING_DRIVEN\n");
			break;
		case NO_TIMING:
			vtr::printf_info("NO_TIMING\n");
			break;
		default:
			vpr_throw(VPR_ERROR_UNKNOWN, __FILE__, __LINE__, "<Unknown>\n");
		}

		vtr::printf_info("RouterOpts.base_cost_type: ");
		switch (RouterOpts.base_cost_type) {
		case DELAY_NORMALIZED:
			vtr::printf_info("DELAY_NORMALIZED\n");
			break;
		case DEMAND_ONLY:
			vtr::printf_info("DEMAND_ONLY\n");
			break;
		default:
			vtr::printf_error(__FILE__, __LINE__, "Unknown base_cost_type\n");
		}

		vtr::printf_info("RouterOpts.fixed_channel_width: ");
		if (NO_FIXED_CHANNEL_WIDTH == RouterOpts.fixed_channel_width) {
			vtr::printf_info("NO_FIXED_CHANNEL_WIDTH\n");
		} else {
			vtr::printf_info("%d\n", RouterOpts.fixed_channel_width);
		}

		vtr::printf_info("RouterOpts.trim_empty_chan: %s\n", (RouterOpts.trim_empty_channels ? "true" : "false"));
		vtr::printf_info("RouterOpts.trim_obs_chan: %s\n", (RouterOpts.trim_obs_channels ? "true" : "false"));
		vtr::printf_info("RouterOpts.acc_fac: %f\n", RouterOpts.acc_fac);
		vtr::printf_info("RouterOpts.bb_factor: %d\n", RouterOpts.bb_factor);
		vtr::printf_info("RouterOpts.bend_cost: %f\n", RouterOpts.bend_cost);
		vtr::printf_info("RouterOpts.first_iter_pres_fac: %f\n", RouterOpts.first_iter_pres_fac);
		vtr::printf_info("RouterOpts.initial_pres_fac: %f\n", RouterOpts.initial_pres_fac);
		vtr::printf_info("RouterOpts.pres_fac_mult: %f\n", RouterOpts.pres_fac_mult);
		vtr::printf_info("RouterOpts.max_router_iterations: %d\n", RouterOpts.max_router_iterations);
		vtr::printf_info("RouterOpts.min_incremental_reroute_fanout: %d\n", RouterOpts.min_incremental_reroute_fanout);

		if (TIMING_DRIVEN == RouterOpts.router_algorithm) {
			vtr::printf_info("RouterOpts.astar_fac: %f\n", RouterOpts.astar_fac);
			vtr::printf_info("RouterOpts.criticality_exp: %f\n", RouterOpts.criticality_exp);
			vtr::printf_info("RouterOpts.max_criticality: %f\n", RouterOpts.max_criticality);
		}
		if (RouterOpts.routing_failure_predictor == SAFE)
			vtr::printf_info("RouterOpts.routing_failure_predictor = SAFE\n");
		else if (RouterOpts.routing_failure_predictor == AGGRESSIVE)
			vtr::printf_info("RouterOpts.routing_failure_predictor = AGGRESSIVE\n");
		else if (RouterOpts.routing_failure_predictor == OFF)
			vtr::printf_info("RouterOpts.routing_failure_predictor = OFF\n");

                if(RouterOpts.routing_budgets_algorithm == DISABLE){
			vtr::printf_info("RouterOpts.routing_budgets_algorithm = DISABLE\n");
                }else if(RouterOpts.routing_budgets_algorithm == MINIMAX){
			vtr::printf_info("RouterOpts.routing_budgets_algorithm = MINIMAX\n");
                }else if(RouterOpts.routing_budgets_algorithm == SCALE_DELAY){
			vtr::printf_info("RouterOpts.routing_budgets_algorithm = SCALE_DELAY\n");
                }


	} else {
		VTR_ASSERT(GLOBAL == RouterOpts.route_type);

		vtr::printf_info("RouterOpts.router_algorithm: ");
		switch (RouterOpts.router_algorithm) {
		case BREADTH_FIRST:
			vtr::printf_info("BREADTH_FIRST\n");
			break;
		case TIMING_DRIVEN:
			vtr::printf_info("TIMING_DRIVEN\n");
			break;
		case NO_TIMING:
			vtr::printf_info("NO_TIMING\n");
			break;
		default:
			vtr::printf_error(__FILE__, __LINE__, "Unknown router algorithm\n");
		}

		vtr::printf_info("RouterOpts.base_cost_type: ");
		switch (RouterOpts.base_cost_type) {
		case DELAY_NORMALIZED:
			vtr::printf_info("DELAY_NORMALIZED\n");
			break;
		case DEMAND_ONLY:
			vtr::printf_info("DEMAND_ONLY\n");
			break;
		default:
			vtr::printf_error(__FILE__, __LINE__, "Unknown router base cost type\n");
		}

		vtr::printf_info("RouterOpts.fixed_channel_width: ");
		if (NO_FIXED_CHANNEL_WIDTH == RouterOpts.fixed_channel_width) {
			vtr::printf_info("NO_FIXED_CHANNEL_WIDTH\n");
		} else {
			vtr::printf_info("%d\n", RouterOpts.fixed_channel_width);
		}

		vtr::printf_info("RouterOpts.trim_empty_chan: %s\n", (RouterOpts.trim_empty_channels ? "true" : "false"));
		vtr::printf_info("RouterOpts.trim_obs_chan: %s\n", (RouterOpts.trim_obs_channels ? "true" : "false"));
		vtr::printf_info("RouterOpts.acc_fac: %f\n", RouterOpts.acc_fac);
		vtr::printf_info("RouterOpts.bb_factor: %d\n", RouterOpts.bb_factor);
		vtr::printf_info("RouterOpts.bend_cost: %f\n", RouterOpts.bend_cost);
		vtr::printf_info("RouterOpts.first_iter_pres_fac: %f\n", RouterOpts.first_iter_pres_fac);
		vtr::printf_info("RouterOpts.initial_pres_fac: %f\n", RouterOpts.initial_pres_fac);
		vtr::printf_info("RouterOpts.pres_fac_mult: %f\n", RouterOpts.pres_fac_mult);
		vtr::printf_info("RouterOpts.max_router_iterations: %d\n", RouterOpts.max_router_iterations);
		vtr::printf_info("RouterOpts.min_incremental_reroute_fanout: %d\n", RouterOpts.min_incremental_reroute_fanout);
		if (TIMING_DRIVEN == RouterOpts.router_algorithm) {
			vtr::printf_info("RouterOpts.astar_fac: %f\n", RouterOpts.astar_fac);
			vtr::printf_info("RouterOpts.criticality_exp: %f\n", RouterOpts.criticality_exp);
			vtr::printf_info("RouterOpts.max_criticality: %f\n", RouterOpts.max_criticality);
		}
	}
	vtr::printf_info("\n");
}

static void ShowPlacerOpts(const t_placer_opts& PlacerOpts,
		const t_annealing_sched& AnnealSched) {

	vtr::printf_info("PlacerOpts.place_freq: ");
	switch (PlacerOpts.place_freq) {
	case PLACE_ONCE:
		vtr::printf_info("PLACE_ONCE\n");
		break;
	case PLACE_ALWAYS:
		vtr::printf_info("PLACE_ALWAYS\n");
		break;
	case PLACE_NEVER:
		vtr::printf_info("PLACE_NEVER\n");
		break;
	default:
		vtr::printf_error(__FILE__, __LINE__, "Unknown Place Freq\n");
	}
	if ((PLACE_ONCE == PlacerOpts.place_freq)
			|| (PLACE_ALWAYS == PlacerOpts.place_freq)) {

		vtr::printf_info("PlacerOpts.place_algorithm: ");
		switch (PlacerOpts.place_algorithm) {
		case BOUNDING_BOX_PLACE:
			vtr::printf_info("BOUNDING_BOX_PLACE\n");
			break;
		case PATH_TIMING_DRIVEN_PLACE:
			vtr::printf_info("PATH_TIMING_DRIVEN_PLACE\n");
			break;
		default:
			vtr::printf_error(__FILE__, __LINE__, "Unknown placement algorithm\n");
		}

		vtr::printf_info("PlacerOpts.pad_loc_type: ");
		switch (PlacerOpts.pad_loc_type) {
		case FREE:
			vtr::printf_info("FREE\n");
			break;
		case RANDOM:
			vtr::printf_info("RANDOM\n");
			break;
		case USER:
			vtr::printf_info("USER '%s'\n", PlacerOpts.pad_loc_file.c_str());
			break;
		default:
			vpr_throw(VPR_ERROR_UNKNOWN, __FILE__, __LINE__, "Unknown I/O pad location type\n");
		}

		vtr::printf_info("PlacerOpts.place_cost_exp: %f\n", PlacerOpts.place_cost_exp);

        vtr::printf_info("PlacerOpts.place_chan_width: %d\n", PlacerOpts.place_chan_width);

		if (PATH_TIMING_DRIVEN_PLACE == PlacerOpts.place_algorithm) {
			vtr::printf_info("PlacerOpts.inner_loop_recompute_divider: %d\n", PlacerOpts.inner_loop_recompute_divider);
			vtr::printf_info("PlacerOpts.recompute_crit_iter: %d\n", PlacerOpts.recompute_crit_iter);
			vtr::printf_info("PlacerOpts.timing_tradeoff: %f\n", PlacerOpts.timing_tradeoff);
			vtr::printf_info("PlacerOpts.td_place_exp_first: %f\n", PlacerOpts.td_place_exp_first);
			vtr::printf_info("PlacerOpts.td_place_exp_last: %f\n", PlacerOpts.td_place_exp_last);
		}

		vtr::printf_info("PlaceOpts.seed: %d\n", PlacerOpts.seed);

		ShowAnnealSched(AnnealSched);
	}
	vtr::printf_info("\n");

}

static void ShowNetlistOpts(const t_netlist_opts& NetlistOpts) {
    vtr::printf_info("NetlistOpts.abosrb_buffer_luts            : %s\n", (NetlistOpts.absorb_buffer_luts)             ? "true" : "false");
    vtr::printf_info("NetlistOpts.sweep_dangling_primary_ios    : %s\n", (NetlistOpts.sweep_dangling_primary_ios)     ? "true" : "false");
    vtr::printf_info("NetlistOpts.sweep_dangling_nets           : %s\n", (NetlistOpts.sweep_dangling_nets)            ? "true" : "false");
    vtr::printf_info("NetlistOpts.sweep_dangling_blocks         : %s\n", (NetlistOpts.sweep_dangling_blocks)          ? "true" : "false");
    vtr::printf_info("NetlistOpts.sweep_constant_primary_outputs: %s\n", (NetlistOpts.sweep_constant_primary_outputs) ? "true" : "false");
	vtr::printf_info("\n");
}

static void ShowAnalysisOpts(const t_analysis_opts& AnalysisOpts) {
    vtr::printf_info("AnalysisOpts.gen_post_synthesis_netlist: %s\n", (AnalysisOpts.gen_post_synthesis_netlist) ? "true" : "false");
	vtr::printf_info("\n");
}

static void ShowPackerOpts(const t_packer_opts& PackerOpts) {

	vtr::printf_info("PackerOpts.allow_unrelated_clustering: %s", (PackerOpts.allow_unrelated_clustering ? "true\n" : "false\n"));
	vtr::printf_info("PackerOpts.alpha_clustering: %f\n", PackerOpts.alpha);
	vtr::printf_info("PackerOpts.beta_clustering: %f\n", PackerOpts.beta);
	vtr::printf_info("PackerOpts.cluster_seed_type: ");
	switch (PackerOpts.cluster_seed_type) {
	case VPACK_TIMING:
		vtr::printf_info("TIMING\n");
		break;
	case VPACK_MAX_INPUTS:
		vtr::printf_info("MAX_INPUTS\n");
		break;
	case VPACK_BLEND:
		vtr::printf_info("BLEND\n");
		break;
	default:
		vpr_throw(VPR_ERROR_UNKNOWN, __FILE__, __LINE__, "Unknown packer cluster_seed_type\n");
	}
	vtr::printf_info("PackerOpts.connection_driven: %s", (PackerOpts.connection_driven ? "true\n" : "false\n"));
	vtr::printf_info("PackerOpts.global_clocks: %s", (PackerOpts.global_clocks ? "true\n" : "false\n"));
	vtr::printf_info("PackerOpts.hill_climbing_flag: %s", (PackerOpts.hill_climbing_flag ? "true\n" : "false\n"));
	vtr::printf_info("PackerOpts.inter_cluster_net_delay: %f\n", PackerOpts.inter_cluster_net_delay);
	vtr::printf_info("PackerOpts.timing_driven: %s", (PackerOpts.timing_driven ? "true\n" : "false\n"));
	vtr::printf_info("\n");
}

