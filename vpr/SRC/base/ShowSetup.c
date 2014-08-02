#include <assert.h>

#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "OptionTokens.h"
#include "ReadOptions.h"
#include "read_xml_arch_file.h"
#include "SetupVPR.h"

/******** Function Prototypes ********/
static void ShowPackerOpts(INP struct s_packer_opts PackerOpts);
static void ShowPlacerOpts(INP t_options Options,
		INP struct s_placer_opts PlacerOpts,
		INP struct s_annealing_sched AnnealSched);
static void ShowOperation(INP enum e_operation Operation);
static void ShowRouterOpts(INP struct s_router_opts RouterOpts);
static void ShowAnnealSched(INP struct s_annealing_sched AnnealSched);
static void ShowRoutingArch(INP struct s_det_routing_arch RoutingArch);

/******** Function Implementations ********/

void ShowSetup(INP t_options options, INP t_vpr_setup vpr_setup) {
	vpr_printf_info("Timing analysis: %s\n", (vpr_setup.TimingEnabled? "ON" : "OFF"));

	vpr_printf_info("Circuit netlist file: %s\n", vpr_setup.FileNameOpts.NetFile);
	vpr_printf_info("Circuit placement file: %s\n", vpr_setup.FileNameOpts.PlaceFile);
	vpr_printf_info("Circuit routing file: %s\n", vpr_setup.FileNameOpts.RouteFile);
	vpr_printf_info("Circuit SDC file: %s\n", vpr_setup.Timing.SDCFile);

	ShowOperation(vpr_setup.Operation);
	vpr_printf_info("Packer: %s\n", (vpr_setup.PackerOpts.doPacking ? "ENABLED" : "DISABLED"));
	vpr_printf_info("Placer: %s\n", (vpr_setup.PlacerOpts.doPlacement ? "ENABLED" : "DISABLED"));
	vpr_printf_info("Router: %s\n", (vpr_setup.RouterOpts.doRouting ? "ENABLED" : "DISABLED"));

#ifdef INTERPOSER_BASED_ARCHITECTURE
	vpr_printf_info("Percent wires cut: %d\n", percent_wires_cut);
	vpr_printf_info("Number of cuts: %d\n", num_cuts);
	vpr_printf_info("Delay increase: %d\n", delay_increase);
	vpr_printf_info("Placer cost constant: %f\n", placer_cost_constant);
	vpr_printf_info("Constant type: %d\n", constant_type);

	vpr_printf_info("\nallow_chanx_interposer_connections: %s\n\n", allow_chanx_interposer_connections? "YES":"NO");
	vpr_printf_info("transfer_interposer_fanins: %s\n", transfer_interposer_fanins? "YES":"NO");
	vpr_printf_info("allow_additional_interposer_fanins: %s\n", allow_additional_interposer_fanins? "YES":"NO");
	vpr_printf_info("pct_of_interposer_nodes_each_chany_can_drive: %d\n\n", pct_of_interposer_nodes_each_chany_can_drive);
	vpr_printf_info("transfer_interposer_fanouts: %s\n", transfer_interposer_fanouts? "YES":"NO");
	vpr_printf_info("allow_additional_interposer_fanouts: %s\n", allow_additional_interposer_fanouts? "YES":"NO");
	vpr_printf_info("pct_of_chany_wires_an_interposer_node_can_drive: %d\n\n", pct_of_chany_wires_an_interposer_node_can_drive);
	vpr_printf_info("allow_bidir_interposer_wires: %s\n\n", allow_bidir_interposer_wires? "YES":"NO");
#endif

	if (vpr_setup.PackerOpts.doPacking) {
		ShowPackerOpts(vpr_setup.PackerOpts);
	}
	if (vpr_setup.PlacerOpts.doPlacement) {
		ShowPlacerOpts(options, vpr_setup.PlacerOpts, vpr_setup.AnnealSched);
	}
	if (vpr_setup.RouterOpts.doRouting) {
		ShowRouterOpts(vpr_setup.RouterOpts);
	}

	if (DETAILED == vpr_setup.RouterOpts.route_type)
		ShowRoutingArch(vpr_setup.RoutingArch);

}

void printClusteredNetlistStats() {
	int i, j, L_num_p_inputs, L_num_p_outputs;
	int *num_blocks_type;
	num_blocks_type = (int*) my_calloc(num_types, sizeof(int));

	vpr_printf_info("\n");
	vpr_printf_info("Netlist num_nets: %d\n", (int) g_clbs_nlist.net.size());
	vpr_printf_info("Netlist num_blocks: %d\n", num_blocks);

	for (i = 0; i < num_types; i++) {
		num_blocks_type[i] = 0;
	}
	/* Count I/O input and output pads */
	L_num_p_inputs = 0;
	L_num_p_outputs = 0;

	for (i = 0; i < num_blocks; i++) {
		num_blocks_type[block[i].type->index]++;
		if (block[i].type == IO_TYPE) {
			for (j = 0; j < IO_TYPE->num_pins; j++) {
				if (block[i].nets[j] != OPEN) {
					if (IO_TYPE->class_inf[IO_TYPE->pin_class[j]].type
							== DRIVER) {
						L_num_p_inputs++;
					} else {
						assert(
								IO_TYPE-> class_inf[IO_TYPE-> pin_class[j]]. type == RECEIVER);
						L_num_p_outputs++;
					}
				}
			}
		}
	}

	for (i = 0; i < num_types; i++) {
		if (IO_TYPE != &type_descriptors[i]) {
			vpr_printf_info("Netlist %s blocks: %d.\n", type_descriptors[i].name, num_blocks_type[i]);
		}
	}

	/* Print out each block separately instead */
	vpr_printf_info("Netlist inputs pins: %d\n", L_num_p_inputs);
	vpr_printf_info("Netlist output pins: %d\n", L_num_p_outputs);
	vpr_printf_info("\n");
	free(num_blocks_type);
}

static void ShowRoutingArch(INP struct s_det_routing_arch RoutingArch) {

	vpr_printf_info("RoutingArch.directionality: ");
	switch (RoutingArch.directionality) {
	case BI_DIRECTIONAL:
		vpr_printf_info("BI_DIRECTIONAL\n");
		break;
	case UNI_DIRECTIONAL:
		vpr_printf_info("UNI_DIRECTIONAL\n");
		break;
	default:
		vpr_throw(VPR_ERROR_UNKNOWN, __FILE__, __LINE__, "<Unknown>\n");
		break;
	}

	vpr_printf_info("RoutingArch.switch_block_type: ");
	switch (RoutingArch.switch_block_type) {
	case SUBSET:
		vpr_printf_info("SUBSET\n");
		break;
	case WILTON:
		vpr_printf_info("WILTON\n");
		break;
	case UNIVERSAL:
		vpr_printf_info("UNIVERSAL\n");
		break;
	case FULL:
		vpr_printf_info("FULL\n");
		break;
	default:
		vpr_printf_error(__FILE__, __LINE__, "switch block type\n");
	}

	vpr_printf_info("RoutingArch.Fs: %d\n", RoutingArch.Fs);

	vpr_printf_info("\n");
}

static void ShowAnnealSched(INP struct s_annealing_sched AnnealSched) {

	vpr_printf_info("AnnealSched.type: ");
	switch (AnnealSched.type) {
	case AUTO_SCHED:
		vpr_printf_info("AUTO_SCHED\n");
		break;
	case USER_SCHED:
		vpr_printf_info("USER_SCHED\n");
		break;
	default:
		vpr_printf_error(__FILE__, __LINE__, "Unknown annealing schedule\n");
	}

	vpr_printf_info("AnnealSched.inner_num: %f\n", AnnealSched.inner_num);

	if (USER_SCHED == AnnealSched.type) {
		vpr_printf_info("AnnealSched.init_t: %f\n", AnnealSched.init_t);
		vpr_printf_info("AnnealSched.alpha_t: %f\n", AnnealSched.alpha_t);
		vpr_printf_info("AnnealSched.exit_t: %f\n", AnnealSched.exit_t);
	}
}

static void ShowRouterOpts(INP struct s_router_opts RouterOpts) {

	vpr_printf_info("RouterOpts.route_type: ");
	switch (RouterOpts.route_type) {
	case GLOBAL:
		vpr_printf_info("GLOBAL\n");
		break;
	case DETAILED:
		vpr_printf_info("DETAILED\n");
		break;
	default:
		vpr_printf_error(__FILE__, __LINE__, "Unknown router opt\n");
	}

	if (DETAILED == RouterOpts.route_type) {
		vpr_printf_info("RouterOpts.router_algorithm: ");
		switch (RouterOpts.router_algorithm) {
		case BREADTH_FIRST:
			vpr_printf_info("BREADTH_FIRST\n");
			break;
		case TIMING_DRIVEN:
			vpr_printf_info("TIMING_DRIVEN\n");
			break;
		case NO_TIMING:
			vpr_printf_info("NO_TIMING\n");
			break;
		default:
			vpr_throw(VPR_ERROR_UNKNOWN, __FILE__, __LINE__, "<Unknown>\n");
		}

		vpr_printf_info("RouterOpts.base_cost_type: ");
		switch (RouterOpts.base_cost_type) {
		case INTRINSIC_DELAY:
			vpr_printf_info("INTRINSIC_DELAY\n");
			break;
		case DELAY_NORMALIZED:
			vpr_printf_info("DELAY_NORMALIZED\n");
			break;
		case DEMAND_ONLY:
			vpr_printf_info("DEMAND_ONLY\n");
			break;
		default:
			vpr_printf_error(__FILE__, __LINE__, "Unknown base_cost_type\n");
		}

		vpr_printf_info("RouterOpts.fixed_channel_width: ");
		if (NO_FIXED_CHANNEL_WIDTH == RouterOpts.fixed_channel_width) {
			vpr_printf_info("NO_FIXED_CHANNEL_WIDTH\n");
		} else {
			vpr_printf_info("%d\n", RouterOpts.fixed_channel_width);
		}

		vpr_printf_info("RouterOpts.trim_empty_chan: %s\n", (RouterOpts.trim_empty_channels ? "TRUE" : "FALSE"));
		vpr_printf_info("RouterOpts.trim_obs_chan: %s\n", (RouterOpts.trim_obs_channels ? "TRUE" : "FALSE"));
		vpr_printf_info("RouterOpts.acc_fac: %f\n", RouterOpts.acc_fac);
		vpr_printf_info("RouterOpts.bb_factor: %d\n", RouterOpts.bb_factor);
		vpr_printf_info("RouterOpts.bend_cost: %f\n", RouterOpts.bend_cost);
		vpr_printf_info("RouterOpts.first_iter_pres_fac: %f\n", RouterOpts.first_iter_pres_fac);
		vpr_printf_info("RouterOpts.initial_pres_fac: %f\n", RouterOpts.initial_pres_fac);
		vpr_printf_info("RouterOpts.pres_fac_mult: %f\n", RouterOpts.pres_fac_mult);
		vpr_printf_info("RouterOpts.max_router_iterations: %d\n", RouterOpts.max_router_iterations);

		if (TIMING_DRIVEN == RouterOpts.router_algorithm) {
			vpr_printf_info("RouterOpts.astar_fac: %f\n", RouterOpts.astar_fac);
			vpr_printf_info("RouterOpts.criticality_exp: %f\n", RouterOpts.criticality_exp);
			vpr_printf_info("RouterOpts.max_criticality: %f\n", RouterOpts.max_criticality);
		}
		if (RouterOpts.routing_failure_predictor == SAFE)
			vpr_printf_info("RouterOpts.routing_failure_predictor = SAFE\n");
		else if (RouterOpts.routing_failure_predictor == AGGRESSIVE)
			vpr_printf_info("RouterOpts.routing_failure_predictor = AGGRESSIVE\n");
		else if (RouterOpts.routing_failure_predictor == OFF)
			vpr_printf_info("RouterOpts.routing_failure_predictor = OFF\n");
	} else {
		assert(GLOBAL == RouterOpts.route_type);

		vpr_printf_info("RouterOpts.router_algorithm: ");
		switch (RouterOpts.router_algorithm) {
		case BREADTH_FIRST:
			vpr_printf_info("BREADTH_FIRST\n");
			break;
		case TIMING_DRIVEN:
			vpr_printf_info("TIMING_DRIVEN\n");
			break;
		case NO_TIMING:
			vpr_printf_info("NO_TIMING\n");
			break;
		default:
			vpr_printf_error(__FILE__, __LINE__, "Unknown router algorithm\n");
		}

		vpr_printf_info("RouterOpts.base_cost_type: ");
		switch (RouterOpts.base_cost_type) {
		case INTRINSIC_DELAY:
			vpr_printf_info("INTRINSIC_DELAY\n");
			break;
		case DELAY_NORMALIZED:
			vpr_printf_info("DELAY_NORMALIZED\n");
			break;
		case DEMAND_ONLY:
			vpr_printf_info("DEMAND_ONLY\n");
			break;
		default:
			vpr_printf_error(__FILE__, __LINE__, "Unknown router base cost type\n");
		}

		vpr_printf_info("RouterOpts.fixed_channel_width: ");
		if (NO_FIXED_CHANNEL_WIDTH == RouterOpts.fixed_channel_width) {
			vpr_printf_info("NO_FIXED_CHANNEL_WIDTH\n");
		} else {
			vpr_printf_info("%d\n", RouterOpts.fixed_channel_width);
		}

		vpr_printf_info("RouterOpts.trim_empty_chan: %s\n", (RouterOpts.trim_empty_channels ? "TRUE" : "FALSE"));
		vpr_printf_info("RouterOpts.trim_obs_chan: %s\n", (RouterOpts.trim_obs_channels ? "TRUE" : "FALSE"));
		vpr_printf_info("RouterOpts.acc_fac: %f\n", RouterOpts.acc_fac);
		vpr_printf_info("RouterOpts.bb_factor: %d\n", RouterOpts.bb_factor);
		vpr_printf_info("RouterOpts.bend_cost: %f\n", RouterOpts.bend_cost);
		vpr_printf_info("RouterOpts.first_iter_pres_fac: %f\n", RouterOpts.first_iter_pres_fac);
		vpr_printf_info("RouterOpts.initial_pres_fac: %f\n", RouterOpts.initial_pres_fac);
		vpr_printf_info("RouterOpts.pres_fac_mult: %f\n", RouterOpts.pres_fac_mult);
		vpr_printf_info("RouterOpts.max_router_iterations: %d\n", RouterOpts.max_router_iterations);
		if (TIMING_DRIVEN == RouterOpts.router_algorithm) {
			vpr_printf_info("RouterOpts.astar_fac: %f\n", RouterOpts.astar_fac);
			vpr_printf_info("RouterOpts.criticality_exp: %f\n", RouterOpts.criticality_exp);
			vpr_printf_info("RouterOpts.max_criticality: %f\n", RouterOpts.max_criticality);
		}
	}
	vpr_printf_info("\n");
}

static void ShowOperation(INP enum e_operation Operation) {
	vpr_printf_info("Operation: ");
	switch (Operation) {
	case RUN_FLOW:
		vpr_printf_info("RUN_FLOW\n");
		break;
	case TIMING_ANALYSIS_ONLY:
		vpr_printf_info("TIMING_ANALYSIS_ONLY\n");
		break;
	default:
		vpr_printf_error(__FILE__, __LINE__, "Unknown VPR operation\n");
	}
	vpr_printf_info("\n");
}

static void ShowPlacerOpts(INP t_options Options,
		INP struct s_placer_opts PlacerOpts,
		INP struct s_annealing_sched AnnealSched) {

	vpr_printf_info("PlacerOpts.place_freq: ");
	switch (PlacerOpts.place_freq) {
	case PLACE_ONCE:
		vpr_printf_info("PLACE_ONCE\n");
		break;
	case PLACE_ALWAYS:
		vpr_printf_info("PLACE_ALWAYS\n");
		break;
	case PLACE_NEVER:
		vpr_printf_info("PLACE_NEVER\n");
		break;
	default:
		vpr_printf_error(__FILE__, __LINE__, "Unknown Place Freq\n");
	}
	if ((PLACE_ONCE == PlacerOpts.place_freq)
			|| (PLACE_ALWAYS == PlacerOpts.place_freq)) {

		vpr_printf_info("PlacerOpts.place_algorithm: ");
		switch (PlacerOpts.place_algorithm) {
		case BOUNDING_BOX_PLACE:
			vpr_printf_info("BOUNDING_BOX_PLACE\n");
			break;
		case NET_TIMING_DRIVEN_PLACE:
			vpr_printf_info("NET_TIMING_DRIVEN_PLACE\n");
			break;
		case PATH_TIMING_DRIVEN_PLACE:
			vpr_printf_info("PATH_TIMING_DRIVEN_PLACE\n");
			break;
		default:
			vpr_printf_error(__FILE__, __LINE__, "Unknown placement algorithm\n");
		}

		vpr_printf_info("PlacerOpts.pad_loc_type: ");
		switch (PlacerOpts.pad_loc_type) {
		case FREE:
			vpr_printf_info("FREE\n");
			break;
		case RANDOM:
			vpr_printf_info("RANDOM\n");
			break;
		case USER:
			vpr_printf_info("USER '%s'\n", PlacerOpts.pad_loc_file);
			break;
		default:
			vpr_throw(VPR_ERROR_UNKNOWN, __FILE__, __LINE__, "Unknown I/O pad location type\n");
		}

		vpr_printf_info("PlacerOpts.place_cost_exp: %f\n", PlacerOpts.place_cost_exp);

		if (Options.Count[OT_PLACE_CHAN_WIDTH]) {
			vpr_printf_info("PlacerOpts.place_chan_width: %d\n", PlacerOpts.place_chan_width);
		}

		if ((NET_TIMING_DRIVEN_PLACE == PlacerOpts.place_algorithm)
				|| (PATH_TIMING_DRIVEN_PLACE == PlacerOpts.place_algorithm)) {
			vpr_printf_info("PlacerOpts.inner_loop_recompute_divider: %d\n", PlacerOpts.inner_loop_recompute_divider);
			vpr_printf_info("PlacerOpts.recompute_crit_iter: %d\n", PlacerOpts.recompute_crit_iter);
			vpr_printf_info("PlacerOpts.timing_tradeoff: %f\n", PlacerOpts.timing_tradeoff);
			vpr_printf_info("PlacerOpts.td_place_exp_first: %f\n", PlacerOpts.td_place_exp_first);
			vpr_printf_info("PlacerOpts.td_place_exp_last: %f\n", PlacerOpts.td_place_exp_last);
		}

		vpr_printf_info("PlaceOpts.seed: %d\n", PlacerOpts.seed);

		ShowAnnealSched(AnnealSched);
	}
	vpr_printf_info("\n");

}


static void ShowPackerOpts(INP struct s_packer_opts PackerOpts) {

	vpr_printf_info("PackerOpts.allow_early_exit: %s", (PackerOpts.allow_early_exit ? "TRUE\n" : "FALSE\n"));
	vpr_printf_info("PackerOpts.allow_unrelated_clustering: %s", (PackerOpts.allow_unrelated_clustering ? "TRUE\n" : "FALSE\n"));
	vpr_printf_info("PackerOpts.alpha_clustering: %f\n", PackerOpts.alpha);
	vpr_printf_info("PackerOpts.aspect: %f\n", PackerOpts.aspect);
	vpr_printf_info("PackerOpts.beta_clustering: %f\n", PackerOpts.beta);
	vpr_printf_info("PackerOpts.block_delay: %f\n", PackerOpts.block_delay);
	vpr_printf_info("PackerOpts.cluster_seed_type: ");
	switch (PackerOpts.cluster_seed_type) {
	case VPACK_TIMING:
		vpr_printf_info("TIMING\n");
		break;
	case VPACK_MAX_INPUTS:
		vpr_printf_info("MAX_INPUTS\n");
		break;
	case VPACK_BLEND:
		vpr_printf_info("BLEND\n");
		break;
	default:
		vpr_throw(VPR_ERROR_UNKNOWN, __FILE__, __LINE__, "Unknown packer cluster_seed_type\n");
	}
	vpr_printf_info("PackerOpts.connection_driven: %s", (PackerOpts.connection_driven ? "TRUE\n" : "FALSE\n"));
	vpr_printf_info("PackerOpts.global_clocks: %s", (PackerOpts.global_clocks ? "TRUE\n" : "FALSE\n"));
	vpr_printf_info("PackerOpts.hill_climbing_flag: %s", (PackerOpts.hill_climbing_flag ? "TRUE\n" : "FALSE\n"));
	vpr_printf_info("PackerOpts.inter_cluster_net_delay: %f\n", PackerOpts.inter_cluster_net_delay);
	vpr_printf_info("PackerOpts.intra_cluster_net_delay: %f\n", PackerOpts.intra_cluster_net_delay);
	vpr_printf_info("PackerOpts.recompute_timing_after: %d\n", PackerOpts.recompute_timing_after);
	vpr_printf_info("PackerOpts.sweep_hanging_nets_and_inputs: %s", (PackerOpts.sweep_hanging_nets_and_inputs ? "TRUE\n" : "FALSE\n"));
	vpr_printf_info("PackerOpts.timing_driven: %s", (PackerOpts.timing_driven ? "TRUE\n" : "FALSE\n"));
	vpr_printf_info("\n");
}

