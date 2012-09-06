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
	vpr_printf(TIO_MESSAGE_INFO, "Timing analysis: %s\n", (vpr_setup.TimingEnabled? "ON" : "OFF"));

	vpr_printf(TIO_MESSAGE_INFO, "Circuit netlist file: %s\n", vpr_setup.FileNameOpts.NetFile);
	vpr_printf(TIO_MESSAGE_INFO, "Circuit placement file: %s\n", vpr_setup.FileNameOpts.PlaceFile);
	vpr_printf(TIO_MESSAGE_INFO, "Circuit routing file: %s\n", vpr_setup.FileNameOpts.RouteFile);
	vpr_printf(TIO_MESSAGE_INFO, "Circuit SDC file: %s\n", vpr_setup.Timing.SDCFile);

	ShowOperation(vpr_setup.Operation);
	vpr_printf(TIO_MESSAGE_INFO, "Packer: %s\n", (vpr_setup.PackerOpts.doPacking ? "ENABLED" : "DISABLED"));
	vpr_printf(TIO_MESSAGE_INFO, "Placer: %s\n", (vpr_setup.PlacerOpts.doPlacement ? "ENABLED" : "DISABLED"));
	vpr_printf(TIO_MESSAGE_INFO, "Router: %s\n", (vpr_setup.RouterOpts.doRouting ? "ENABLED" : "DISABLED"));

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

	vpr_printf(TIO_MESSAGE_INFO, "\n");
	vpr_printf(TIO_MESSAGE_INFO, "Netlist num_nets: %d\n", num_nets);
	vpr_printf(TIO_MESSAGE_INFO, "Netlist num_blocks: %d\n", num_blocks);

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
			vpr_printf(TIO_MESSAGE_INFO, "Netlist %s blocks: %d.\n", type_descriptors[i].name, num_blocks_type[i]);
		}
	}

	/* Print out each block separately instead */
	vpr_printf(TIO_MESSAGE_INFO, "Netlist inputs pins: %d\n", L_num_p_inputs);
	vpr_printf(TIO_MESSAGE_INFO, "Netlist output pins: %d\n", L_num_p_outputs);
	vpr_printf(TIO_MESSAGE_INFO, "\n");
	free(num_blocks_type);
}

static void ShowRoutingArch(INP struct s_det_routing_arch RoutingArch) {

	vpr_printf(TIO_MESSAGE_INFO, "RoutingArch.directionality: ");
	switch (RoutingArch.directionality) {
	case BI_DIRECTIONAL:
		vpr_printf(TIO_MESSAGE_INFO, "BI_DIRECTIONAL\n");
		break;
	case UNI_DIRECTIONAL:
		vpr_printf(TIO_MESSAGE_INFO, "UNI_DIRECTIONAL\n");
		break;
	default:
		vpr_printf(TIO_MESSAGE_INFO, "<Unknown>\n");
		exit(1);
	}

	vpr_printf(TIO_MESSAGE_INFO, "RoutingArch.switch_block_type: ");
	switch (RoutingArch.switch_block_type) {
	case SUBSET:
		vpr_printf(TIO_MESSAGE_INFO, "SUBSET\n");
		break;
	case WILTON:
		vpr_printf(TIO_MESSAGE_INFO, "WILTON\n");
		break;
	case UNIVERSAL:
		vpr_printf(TIO_MESSAGE_INFO, "UNIVERSAL\n");
		break;
	case FULL:
		vpr_printf(TIO_MESSAGE_INFO, "FULL\n");
		break;
	default:
		vpr_printf(TIO_MESSAGE_ERROR, "switch block type\n");
	}

	vpr_printf(TIO_MESSAGE_INFO, "RoutingArch.Fs: %d\n", RoutingArch.Fs);

	vpr_printf(TIO_MESSAGE_INFO, "\n");
}

static void ShowAnnealSched(INP struct s_annealing_sched AnnealSched) {

	vpr_printf(TIO_MESSAGE_INFO, "AnnealSched.type: ");
	switch (AnnealSched.type) {
	case AUTO_SCHED:
		vpr_printf(TIO_MESSAGE_INFO, "AUTO_SCHED\n");
		break;
	case USER_SCHED:
		vpr_printf(TIO_MESSAGE_INFO, "USER_SCHED\n");
		break;
	default:
		vpr_printf(TIO_MESSAGE_ERROR, "Unknown annealing schedule\n");
	}

	vpr_printf(TIO_MESSAGE_INFO, "AnnealSched.inner_num: %f\n", AnnealSched.inner_num);

	if (USER_SCHED == AnnealSched.type) {
		vpr_printf(TIO_MESSAGE_INFO, "AnnealSched.init_t: %f\n", AnnealSched.init_t);
		vpr_printf(TIO_MESSAGE_INFO, "AnnealSched.alpha_t: %f\n", AnnealSched.alpha_t);
		vpr_printf(TIO_MESSAGE_INFO, "AnnealSched.exit_t: %f\n", AnnealSched.exit_t);
	}
}

static void ShowRouterOpts(INP struct s_router_opts RouterOpts) {

	vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.route_type: ");
	switch (RouterOpts.route_type) {
	case GLOBAL:
		vpr_printf(TIO_MESSAGE_INFO, "GLOBAL\n");
		break;
	case DETAILED:
		vpr_printf(TIO_MESSAGE_INFO, "DETAILED\n");
		break;
	default:
		vpr_printf(TIO_MESSAGE_ERROR, "Unknown router opt\n");
	}

	if (DETAILED == RouterOpts.route_type) {
		vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.router_algorithm: ");
		switch (RouterOpts.router_algorithm) {
		case BREADTH_FIRST:
			vpr_printf(TIO_MESSAGE_INFO, "BREADTH_FIRST\n");
			break;
		case TIMING_DRIVEN:
			vpr_printf(TIO_MESSAGE_INFO, "TIMING_DRIVEN\n");
			break;
		case NO_TIMING:
			vpr_printf(TIO_MESSAGE_INFO, "NO_TIMING\n");
			break;
		default:
			vpr_printf(TIO_MESSAGE_INFO, "<Unknown>\n");
			exit(1);
		}

		vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.base_cost_type: ");
		switch (RouterOpts.base_cost_type) {
		case INTRINSIC_DELAY:
			vpr_printf(TIO_MESSAGE_INFO, "INTRINSIC_DELAY\n");
			break;
		case DELAY_NORMALIZED:
			vpr_printf(TIO_MESSAGE_INFO, "DELAY_NORMALIZED\n");
			break;
		case DEMAND_ONLY:
			vpr_printf(TIO_MESSAGE_INFO, "DEMAND_ONLY\n");
			break;
		default:
			vpr_printf(TIO_MESSAGE_ERROR, "Unknown base_cost_type\n");
		}

		vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.fixed_channel_width: ");
		if (NO_FIXED_CHANNEL_WIDTH == RouterOpts.fixed_channel_width) {
			vpr_printf(TIO_MESSAGE_INFO, "NO_FIXED_CHANNEL_WIDTH\n");
		} else {
			vpr_printf(TIO_MESSAGE_INFO, "%d\n", RouterOpts.fixed_channel_width);
		}

		vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.acc_fac: %f\n", RouterOpts.acc_fac);
		vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.bb_factor: %d\n", RouterOpts.bb_factor);
		vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.bend_cost: %f\n", RouterOpts.bend_cost);
		vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.first_iter_pres_fac: %f\n", RouterOpts.first_iter_pres_fac);
		vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.initial_pres_fac: %f\n", RouterOpts.initial_pres_fac);
		vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.pres_fac_mult: %f\n", RouterOpts.pres_fac_mult);
		vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.max_router_iterations: %d\n", RouterOpts.max_router_iterations);

		if (TIMING_DRIVEN == RouterOpts.router_algorithm) {
			vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.astar_fac: %f\n", RouterOpts.astar_fac);
			vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.criticality_exp: %f\n", RouterOpts.criticality_exp);
			vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.max_criticality: %f\n", RouterOpts.max_criticality);
		}
	} else {
		assert(GLOBAL == RouterOpts.route_type);

		vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.router_algorithm: ");
		switch (RouterOpts.router_algorithm) {
		case BREADTH_FIRST:
			vpr_printf(TIO_MESSAGE_INFO, "BREADTH_FIRST\n");
			break;
		case TIMING_DRIVEN:
			vpr_printf(TIO_MESSAGE_INFO, "TIMING_DRIVEN\n");
			break;
		case NO_TIMING:
			vpr_printf(TIO_MESSAGE_INFO, "NO_TIMING\n");
			break;
		default:
			vpr_printf(TIO_MESSAGE_ERROR, "Unknown router algorithm\n");
		}

		vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.base_cost_type: ");
		switch (RouterOpts.base_cost_type) {
		case INTRINSIC_DELAY:
			vpr_printf(TIO_MESSAGE_INFO, "INTRINSIC_DELAY\n");
			break;
		case DELAY_NORMALIZED:
			vpr_printf(TIO_MESSAGE_INFO, "DELAY_NORMALIZED\n");
			break;
		case DEMAND_ONLY:
			vpr_printf(TIO_MESSAGE_INFO, "DEMAND_ONLY\n");
			break;
		default:
			vpr_printf(TIO_MESSAGE_ERROR, "Unknown router base cost type\n");
		}

		vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.fixed_channel_width: ");
		if (NO_FIXED_CHANNEL_WIDTH == RouterOpts.fixed_channel_width) {
			vpr_printf(TIO_MESSAGE_INFO, "NO_FIXED_CHANNEL_WIDTH\n");
		} else {
			vpr_printf(TIO_MESSAGE_INFO, "%d\n", RouterOpts.fixed_channel_width);
		}

		vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.acc_fac: %f\n", RouterOpts.acc_fac);
		vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.bb_factor: %d\n", RouterOpts.bb_factor);
		vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.bend_cost: %f\n", RouterOpts.bend_cost);
		vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.first_iter_pres_fac: %f\n", RouterOpts.first_iter_pres_fac);
		vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.initial_pres_fac: %f\n", RouterOpts.initial_pres_fac);
		vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.pres_fac_mult: %f\n", RouterOpts.pres_fac_mult);
		vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.max_router_iterations: %d\n", RouterOpts.max_router_iterations);
		if (TIMING_DRIVEN == RouterOpts.router_algorithm) {
			vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.astar_fac: %f\n", RouterOpts.astar_fac);
			vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.criticality_exp: %f\n", RouterOpts.criticality_exp);
			vpr_printf(TIO_MESSAGE_INFO, "RouterOpts.max_criticality: %f\n", RouterOpts.max_criticality);
		}
	}
	vpr_printf(TIO_MESSAGE_INFO, "\n");
}

static void ShowOperation(INP enum e_operation Operation) {
	vpr_printf(TIO_MESSAGE_INFO, "Operation: ");
	switch (Operation) {
	case RUN_FLOW:
		vpr_printf(TIO_MESSAGE_INFO, "RUN_FLOW\n");
		break;
	case TIMING_ANALYSIS_ONLY:
		vpr_printf(TIO_MESSAGE_INFO, "TIMING_ANALYSIS_ONLY\n");
		break;
	default:
		vpr_printf(TIO_MESSAGE_ERROR, "Unknown VPR operation\n");
	}
	vpr_printf(TIO_MESSAGE_INFO, "\n");
}

static void ShowPlacerOpts(INP t_options Options,
		INP struct s_placer_opts PlacerOpts,
		INP struct s_annealing_sched AnnealSched) {

	vpr_printf(TIO_MESSAGE_INFO, "PlacerOpts.place_freq: ");
	switch (PlacerOpts.place_freq) {
	case PLACE_ONCE:
		vpr_printf(TIO_MESSAGE_INFO, "PLACE_ONCE\n");
		break;
	case PLACE_ALWAYS:
		vpr_printf(TIO_MESSAGE_INFO, "PLACE_ALWAYS\n");
		break;
	case PLACE_NEVER:
		vpr_printf(TIO_MESSAGE_INFO, "PLACE_NEVER\n");
		break;
	default:
		vpr_printf(TIO_MESSAGE_ERROR, "Unknown Place Freq\n");
	}
	if ((PLACE_ONCE == PlacerOpts.place_freq)
			|| (PLACE_ALWAYS == PlacerOpts.place_freq)) {

		vpr_printf(TIO_MESSAGE_INFO, "PlacerOpts.place_algorithm: ");
		switch (PlacerOpts.place_algorithm) {
		case BOUNDING_BOX_PLACE:
			vpr_printf(TIO_MESSAGE_INFO, "BOUNDING_BOX_PLACE\n");
			break;
		case NET_TIMING_DRIVEN_PLACE:
			vpr_printf(TIO_MESSAGE_INFO, "NET_TIMING_DRIVEN_PLACE\n");
			break;
		case PATH_TIMING_DRIVEN_PLACE:
			vpr_printf(TIO_MESSAGE_INFO, "PATH_TIMING_DRIVEN_PLACE\n");
			break;
		default:
			vpr_printf(TIO_MESSAGE_ERROR, "Unknown placement algorithm\n");
		}

		vpr_printf(TIO_MESSAGE_INFO, "PlacerOpts.pad_loc_type: ");
		switch (PlacerOpts.pad_loc_type) {
		case FREE:
			vpr_printf(TIO_MESSAGE_INFO, "FREE\n");
			break;
		case RANDOM:
			vpr_printf(TIO_MESSAGE_INFO, "RANDOM\n");
			break;
		case USER:
			vpr_printf(TIO_MESSAGE_INFO, "USER '%s'\n", PlacerOpts.pad_loc_file);
			break;
		default:
			vpr_printf(TIO_MESSAGE_INFO, "Unknown I/O pad location type\n");
			exit(1);
		}

		vpr_printf(TIO_MESSAGE_INFO, "PlacerOpts.place_cost_exp: %f\n", PlacerOpts.place_cost_exp);

		if (Options.Count[OT_PLACE_CHAN_WIDTH]) {
			vpr_printf(TIO_MESSAGE_INFO, "PlacerOpts.place_chan_width: %d\n", PlacerOpts.place_chan_width);
		}

		if ((NET_TIMING_DRIVEN_PLACE == PlacerOpts.place_algorithm)
				|| (PATH_TIMING_DRIVEN_PLACE == PlacerOpts.place_algorithm)) {
			vpr_printf(TIO_MESSAGE_INFO, "PlacerOpts.inner_loop_recompute_divider: %d\n", PlacerOpts.inner_loop_recompute_divider);
			vpr_printf(TIO_MESSAGE_INFO, "PlacerOpts.recompute_crit_iter: %d\n", PlacerOpts.recompute_crit_iter);
			vpr_printf(TIO_MESSAGE_INFO, "PlacerOpts.timing_tradeoff: %f\n", PlacerOpts.timing_tradeoff);
			vpr_printf(TIO_MESSAGE_INFO, "PlacerOpts.td_place_exp_first: %f\n", PlacerOpts.td_place_exp_first);
			vpr_printf(TIO_MESSAGE_INFO, "PlacerOpts.td_place_exp_last: %f\n", PlacerOpts.td_place_exp_last);
		}

		vpr_printf(TIO_MESSAGE_INFO, "PlaceOpts.seed: %d\n", PlacerOpts.seed);

		ShowAnnealSched(AnnealSched);
	}
	vpr_printf(TIO_MESSAGE_INFO, "\n");

}


static void ShowPackerOpts(INP struct s_packer_opts PackerOpts) {

	vpr_printf(TIO_MESSAGE_INFO, "PackerOpts.allow_early_exit: %s", (PackerOpts.allow_early_exit ? "TRUE\n" : "FALSE\n"));
	vpr_printf(TIO_MESSAGE_INFO, "PackerOpts.allow_unrelated_clustering: %s", (PackerOpts.allow_unrelated_clustering ? "TRUE\n" : "FALSE\n"));
	vpr_printf(TIO_MESSAGE_INFO, "PackerOpts.alpha_clustering: %f\n", PackerOpts.alpha);
	vpr_printf(TIO_MESSAGE_INFO, "PackerOpts.aspect: %f\n", PackerOpts.aspect);
	vpr_printf(TIO_MESSAGE_INFO, "PackerOpts.beta_clustering: %f\n", PackerOpts.beta);
	vpr_printf(TIO_MESSAGE_INFO, "PackerOpts.block_delay: %f\n", PackerOpts.block_delay);
	vpr_printf(TIO_MESSAGE_INFO, "PackerOpts.cluster_seed_type: ");
	switch (PackerOpts.cluster_seed_type) {
	case VPACK_TIMING:
		vpr_printf(TIO_MESSAGE_INFO, "TIMING\n");
		break;
	case VPACK_MAX_INPUTS:
		vpr_printf(TIO_MESSAGE_INFO, "MAX_INPUTS\n");
		break;
	default:
		vpr_printf(TIO_MESSAGE_INFO, "Unknown packer cluster_seed_type\n");
		exit(1);
	}
	vpr_printf(TIO_MESSAGE_INFO, "PackerOpts.connection_driven: %s", (PackerOpts.connection_driven ? "TRUE\n" : "FALSE\n"));
	vpr_printf(TIO_MESSAGE_INFO, "PackerOpts.global_clocks: %s", (PackerOpts.global_clocks ? "TRUE\n" : "FALSE\n"));
	vpr_printf(TIO_MESSAGE_INFO, "PackerOpts.hill_climbing_flag: %s", (PackerOpts.hill_climbing_flag ? "TRUE\n" : "FALSE\n"));
	vpr_printf(TIO_MESSAGE_INFO, "PackerOpts.inter_cluster_net_delay: %f\n", PackerOpts.inter_cluster_net_delay);
	vpr_printf(TIO_MESSAGE_INFO, "PackerOpts.intra_cluster_net_delay: %f\n", PackerOpts.intra_cluster_net_delay);
	vpr_printf(TIO_MESSAGE_INFO, "PackerOpts.recompute_timing_after: %d\n", PackerOpts.recompute_timing_after);
	vpr_printf(TIO_MESSAGE_INFO, "PackerOpts.sweep_hanging_nets_and_inputs: %s", (PackerOpts.sweep_hanging_nets_and_inputs ? "TRUE\n" : "FALSE\n"));
	vpr_printf(TIO_MESSAGE_INFO, "PackerOpts.timing_driven: %s", (PackerOpts.timing_driven ? "TRUE\n" : "FALSE\n"));
	vpr_printf(TIO_MESSAGE_INFO, "\n");
}

