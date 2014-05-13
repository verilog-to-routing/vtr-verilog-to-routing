/**
 General API for VPR
 Other software tools should generally call just the functions defined here
 For advanced/power users, you can call functions defined elsewhere in VPR or modify the data structures directly at your discretion but be aware that doing so can break the correctness of VPR

 Author: Jason Luu
 June 21, 2012
 */

#include <cstdio>
#include <cstring>
#include <ctime>
using namespace std;

#include <assert.h>

#include "util.h"
#include "vpr_types.h"
#include "vpr_utils.h"
#include "globals.h"
#include "graphics.h"
#include "read_netlist.h"
#include "check_netlist.h"
#include "print_netlist.h"
#include "read_blif.h"
#include "draw.h"
#include "place_and_route.h"
#include "pack.h"
#include "SetupGrid.h"
#include "stats.h"
#include "path_delay.h"
#include "OptionTokens.h"
#include "ReadOptions.h"
#include "read_xml_arch_file.h"
#include "SetupVPR.h"
#include "rr_graph.h"
#include "pb_type_graph.h"
#include "ReadOptions.h"
#include "route_common.h"
#include "timing_place_lookup.h"
#include "cluster_legality.h"
#include "route_export.h"
#include "vpr_api.h"
#include "read_sdc.h"
#include "power.h"
#include "pack_types.h"
#include "lb_type_rr_graph.h"
#include "print_netlist_as_blif.h"

#include "TFH_FabricGridHandler.h"
#include "TFH_FabricBlockHandler.h"
#include "TFH_FabricChannelHandler.h"
#include "TFH_FabricSwitchBoxHandler.h"
#include "TFH_FabricConnectionBlockHandler.h"
#include "TCH_RegionPlaceHandler.h"
#include "TCH_RelativePlaceHandler.h"
#include "TCH_PrePlacedHandler.h"
#include "TCH_PreRoutedHandler.h"

/* Local subroutines */
static void free_pb_type(t_pb_type *pb_type);
static void free_complex_block_types(void);

static void free_arch(t_arch* Arch);
static void free_options(t_options *options);
static void free_circuit(void);

static void toro_new_handlers(void);
static void toro_delete_handlers(void);

static boolean has_printhandler_pre_vpr = FALSE;

/* For resync of clustered netlist to the post-route solution. This function adds local nets to cluster */
static void reload_intra_cluster_nets(t_pb *pb);
static t_trace *alloc_and_load_final_routing_trace();
static t_trace *expand_routing_trace(t_trace *trace, int i_g_atoms_net);
static void print_complete_net_trace(t_trace* trace, const char *file_name);
static void resync_post_route_netlist();
static boolean clay_logical_equivalence_handling(const t_arch *arch);
static void clay_lut_input_rebalancing(int iblock, t_pb *pb);
static void clay_reload_ble_locations(int iblock);
static void resync_pb_graph_nodes_in_pb(t_pb_graph_node *pb_graph_node, t_pb *pb);

/* Local subroutines end */

/* Display general VPR information */
void vpr_print_title(void) {

	vpr_printf_info("\n");
	vpr_printf_info("VPR FPGA Placement and Routing.\n");
	vpr_printf_info("Version: Version " VPR_VERSION "\n");
	vpr_printf_info("Compiled: " __DATE__ ".\n");
	vpr_printf_info("University of Toronto\n");
	vpr_printf_info("vpr@eecg.utoronto.ca\n");
	vpr_printf_info("This is free open source code under MIT license.\n");
	vpr_printf_info("\n");

}

/* Display help screen */
void vpr_print_usage(void) {

	vpr_printf_info("Usage:  vpr fpga_architecture.xml circuit_name [Options ...]\n");
	vpr_printf_info("\n");
	vpr_printf_info("General Options:  [--nodisp] [--auto <int>] [--pack]\n");
	vpr_printf_info("\t[--place] [--route] [--timing_analyze_only_with_net_delay <float>]\n");
	vpr_printf_info("\t[--fast] [--full_stats] [--timing_analysis on | off] [--outfile_prefix <string>]\n");
	vpr_printf_info("\t[--blif_file <string>] [--net_file <string>] [--place_file <string>]\n");
	vpr_printf_info("\t[--route_file <string>] [--sdc_file <string>] [--echo_file on | off]\n");
	vpr_printf_info("\n");
	vpr_printf_info("Packer Options:\n");
	/* vpr_printf_info("\t[-global_clocks on | off]\n"); */
	/* vpr_printf_info("\t[-hill_climbing on | off]\n"); */
	/* vpr_printf_info("\t[-sweep_hanging_nets_and_inputs on | off]\n"); */
	vpr_printf_info("\t[--timing_driven_clustering on | off]\n");
	vpr_printf_info("\t[--cluster_seed_type blend|timing|max_inputs] [--alpha_clustering <float>] [--beta_clustering <float>]\n");
	/* vpr_printf_info("\t[-recompute_timing_after <int>] [-cluster_block_delay <float>]\n"); */
	vpr_printf_info("\t[--allow_unrelated_clustering on | off]\n");
	/* vpr_printf_info("\t[-allow_early_exit on | off]\n"); */
	/* vpr_printf_info("\t[-intra_cluster_net_delay <float>] \n"); */
	/* vpr_printf_info("\t[-inter_cluster_net_delay <float>] \n"); */
	vpr_printf_info("\t[--connection_driven_clustering on | off] \n");
	vpr_printf_info("\n");
	vpr_printf_info("Placer Options:\n");
	vpr_printf_info("\t[--place_algorithm bounding_box | net_timing_driven | path_timing_driven]\n");
	vpr_printf_info("\t[--init_t <float>] [--exit_t <float>]\n");
	vpr_printf_info("\t[--alpha_t <float>] [--inner_num <float>] [--seed <int>]\n");
	vpr_printf_info("\t[--place_cost_exp <float>]\n");
	vpr_printf_info("\t[--place_chan_width <int>] \n");
	vpr_printf_info("\t[--fix_pins random | <file.pads>]\n");
	vpr_printf_info("\t[--enable_timing_computations on | off]\n");
	vpr_printf_info("\t[--block_dist <int>]\n");
	vpr_printf_info("\n");
	vpr_printf_info("Placement Options Valid Only for Timing-Driven Placement:\n");
	vpr_printf_info("\t[--timing_tradeoff <float>]\n");
	vpr_printf_info("\t[--recompute_crit_iter <int>]\n");
	vpr_printf_info("\t[--inner_loop_recompute_divider <int>]\n");
	vpr_printf_info("\t[--td_place_exp_first <float>]\n");
	vpr_printf_info("\t[--td_place_exp_last <float>]\n");
	vpr_printf_info("\n");
	vpr_printf_info("Router Options:  [-max_router_iterations <int>] [-bb_factor <int>]\n");
	vpr_printf_info("\t[--initial_pres_fac <float>] [--pres_fac_mult <float>]\n");
	vpr_printf_info("\t[--acc_fac <float>] [--first_iter_pres_fac <float>]\n");
	vpr_printf_info("\t[--bend_cost <float>] [--route_type global | detailed]\n");
	vpr_printf_info("\t[--verify_binary_search] [--route_chan_width <int>] [--route_chan_trim on | off]\n");
	vpr_printf_info("\t[--router_algorithm breadth_first | timing_driven]\n");
	vpr_printf_info("\t[--base_cost_type intrinsic_delay | delay_normalized | demand_only]\n");
	vpr_printf_info("\n");
	vpr_printf_info("Routing options valid only for timing-driven routing:\n");
	vpr_printf_info("\t[--astar_fac <float>] [--max_criticality <float>]\n");
	vpr_printf_info("\t[--criticality_exp <float>]\n");
	vpr_printf_info("\t[--routing_failure_predictor safe | aggressive | off]\n");
	vpr_printf_info("\n");
#ifdef INTERPOSER_BASED_ARCHITECTURE
	vpr_printf_info("Options for controlling the interposer-based architectures:\n");
	vpr_printf_info("\t[--percent_wires_cut <int>]\n");
	vpr_printf_info("\t[--num_cuts <int>]\n");
	vpr_printf_info("\t[--delay_increase <int>]\n");
	vpr_printf_info("\t[--placer_cost_constant <float>]\n");
	vpr_printf_info("\n");
#endif
}

/* Initialize VPR 
 1. Read Options
 2. Read Arch
 3. Read Circuit
 4. Sanity check all three
 */
void vpr_init(INP int argc, INP char **argv, 
		OUTP t_options *options,
		OUTP t_vpr_setup *vpr_setup, 
		OUTP t_arch *arch) {

	if (PrintHandlerExists() == 1) {
		has_printhandler_pre_vpr = TRUE;
	} else {
		has_printhandler_pre_vpr = FALSE;

		char* pszLogFileName = "vpr_stdout.log";
		unsigned char enableTimeStamps = 1;
		unsigned char enableFileLines = 1;
		unsigned long maxWarningCount = 100000;
		unsigned long maxErrorCount = 1000;

		PrintHandlerNew(pszLogFileName);
		PrintHandlerInit(enableTimeStamps, enableFileLines, maxWarningCount, maxErrorCount);
	}

	/* Print title message */
	vpr_print_title();

	if (argc >= 3) {

		memset(options, 0, sizeof(t_options));
		memset(vpr_setup, 0, sizeof(t_vpr_setup));
		memset(arch, 0, sizeof(t_arch));

		/* Read in user options */
		ReadOptions(argc, argv, options);

		/* Timing option priorities */
		vpr_setup->TimingEnabled = IsTimingEnabled(options);

		/* Determine whether echo is on or off */
		setEchoEnabled(IsEchoEnabled(options));
		SetPostSynthesisOption(IsPostSynthesisEnabled(options));
		vpr_setup->constant_net_delay = options->constant_net_delay;
		vpr_setup->gen_netlist_as_blif = (boolean) (options->Count[OT_GEN_NELIST_AS_BLIF] > 0);

		/* Read in arch and circuit */
		SetupVPR(options, vpr_setup->TimingEnabled, TRUE, &vpr_setup->FileNameOpts,
				arch, &vpr_setup->Operation, &vpr_setup->user_models,
				&vpr_setup->library_models, &vpr_setup->PackerOpts,
				&vpr_setup->PlacerOpts, &vpr_setup->AnnealSched,
				&vpr_setup->RouterOpts, &vpr_setup->RoutingArch, 
				&vpr_setup->PackerRRGraph, &vpr_setup->Segments, 
				&vpr_setup->Timing, &vpr_setup->ShowGraphics,
				&vpr_setup->GraphPause, &vpr_setup->PowerOpts);

		/* Check inputs are reasonable */
		CheckOptions(*options, vpr_setup->TimingEnabled);
		CheckArch(*arch, vpr_setup->TimingEnabled);

		/* Verify settings don't conflict or otherwise not make sense */
		CheckSetup(vpr_setup->Operation, vpr_setup->PlacerOpts,
				vpr_setup->AnnealSched, vpr_setup->RouterOpts,
				vpr_setup->RoutingArch, vpr_setup->Segments, vpr_setup->Timing,
				arch->Chans);

		/* flush any messages to user still in stdout that hasn't gotten displayed */
		fflush(stdout);

		/* Read blif file and sweep unused components */
		read_and_process_blif(vpr_setup->PackerOpts.blif_file_name,
				vpr_setup->PackerOpts.sweep_hanging_nets_and_inputs,
				vpr_setup->user_models, vpr_setup->library_models,
				vpr_setup->PowerOpts.do_power, vpr_setup->FileNameOpts.ActFile);
		fflush(stdout);

		ShowSetup(*options, *vpr_setup);

		/* Finally, initialize Toro-specific handler 'singleton' objects */
		toro_new_handlers();

	} else {
		/* Print usage message if no args */
		vpr_print_usage();
		vpr_printf_error(__FILE__, __LINE__, 
			"Missing arguments, see above and try again!\n");
		exit(1);
	}
}

/* 
 * Sets globals: nx, ny
 * Allocs globals: chan_width_x, chan_width_y, grid
 * Depends on num_clbs, pins_per_clb */
void vpr_init_pre_place_and_route(INP t_vpr_setup vpr_setup, INP t_arch Arch) {

	/* Read in netlist file for placement and routing */
	if (vpr_setup.FileNameOpts.NetFile) {
		read_netlist(vpr_setup.FileNameOpts.NetFile, &Arch, 
				&num_blocks, &block, &num_nets, &clb_net);

		/* This is done so that all blocks have subblocks and can be treated the same */
		check_netlist();

		if(vpr_setup.gen_netlist_as_blif) {
			char *name = (char*)my_malloc((strlen(vpr_setup.FileNameOpts.CircuitName) + 16) * sizeof(char));
			sprintf(name, "%s.preplace.blif", vpr_setup.FileNameOpts.CircuitName);
			print_preplace_netlist(&Arch, name);
			free(name);
		}
	}

	/* Output the current settings to console. */
	printClusteredNetlistStats();

	if (vpr_setup.Operation == TIMING_ANALYSIS_ONLY) {
		do_constant_net_delay_timing_analysis(vpr_setup.Timing,	vpr_setup.constant_net_delay);
	} else {
		int current = nint((float)sqrt((float)num_blocks)); /* current is the value of the smaller side of the FPGA */
		int low = 1;
		int high = -1;

		int *num_instances_type = (int*) my_calloc(num_types, sizeof(int));
		int *num_blocks_type = (int*) my_calloc(num_types, sizeof(int));

		for (int i = 0; i < num_blocks; ++i) {
			num_blocks_type[block[i].type->index]++;
		}

		if (Arch.clb_grid.IsAuto) {

			/* Auto-size FPGA, perform a binary search */
			while (high == -1 || low < high) {

				/* Generate grid */
				if (Arch.clb_grid.Aspect >= 1.0) {
					ny = current;
					nx = nint(current * Arch.clb_grid.Aspect);
				} else {
					nx = current;
					ny = nint(current / Arch.clb_grid.Aspect);
				}
#if DEBUG
				vpr_printf_info("Auto-sizing FPGA at x = %d y = %d\n", nx, ny);
#endif
				alloc_and_load_grid(num_instances_type);
				freeGrid();

				/* Test if netlist fits in grid */
				boolean fit = TRUE;
				for (int i = 0; i < num_types; ++i) {
					if (num_blocks_type[i] > num_instances_type[i]) {
						fit = FALSE;
						break;
					}
				}

				/* get next value */
				if (!fit) {
					/* increase size of max */
					if (high == -1) {
						current = current * 2;
						if (current > MAX_SHORT) {
							vpr_printf_error(__FILE__, __LINE__,
									"FPGA required is too large for current architecture settings.\n");
							exit(1);
						}
					} else {
						if (low == current)
							current++;
						low = current;
						current = low + ((high - low) / 2);
					}
				} else {
					high = current;
					current = low + ((high - low) / 2);
				}
			}

			/* Generate grid */
			if (Arch.clb_grid.Aspect >= 1.0) {
				ny = current;
				nx = nint(current * Arch.clb_grid.Aspect);
			} else {
				nx = current;
				ny = nint(current / Arch.clb_grid.Aspect);
			}
                        nx = nx * 1.1;
                        ny = ny * 1.1;
			alloc_and_load_grid(num_instances_type);
			vpr_printf_info("FPGA auto-sized to x = %d y = %d\n", nx, ny);
		} else {
			nx = Arch.clb_grid.W;
			ny = Arch.clb_grid.H;
			alloc_and_load_grid(num_instances_type);
		}

		vpr_printf_info("The circuit will be mapped into a %d x %d array of clbs.\n", nx, ny);

		/* Test if netlist fits in grid */
		for (int i = 0; i < num_types; ++i) {
			if (num_blocks_type[i] > num_instances_type[i]) {

				vpr_printf_error(__FILE__, __LINE__,
						"Not enough physical locations for type %s, "
						"number of blocks is %d but number of locations is %d.\n",
						type_descriptors[i].name, num_blocks_type[i],
						num_instances_type[i]);
				exit(1);
			}
		}

		vpr_printf_info("\n");
		vpr_printf_info("Resource usage...\n");
		for (int i = 0; i < num_types; ++i) {
			vpr_printf_info("\tNetlist      %d\tblocks of type: %s\n",
					num_blocks_type[i], type_descriptors[i].name);
			vpr_printf_info("\tArchitecture %d\tblocks of type: %s\n",
					num_instances_type[i], type_descriptors[i].name);
		}
		vpr_printf_info("\n");
		chan_width.x_max = chan_width.y_max = 0;
		chan_width.x_min = chan_width.y_min = 0;
		chan_width.x_list = (int *) my_malloc((ny + 1) * sizeof(int));
		chan_width.y_list = (int *) my_malloc((nx + 1) * sizeof(int));

		free(num_blocks_type);
		free(num_instances_type);
	}
}

#ifdef INTERPOSER_BASED_ARCHITECTURE
/* This function determines locations where cuts should happen for an
 * interposer-based architecture. 
 * Notice that a cut cannot go through a block.
 *
 * For instance: here we have DSP blocks of height 4, and RAM blocks of height 6.
 * A cut at y=4 would split two DSP blocks nicely, but it would cut through a RAM block.
 * Similarly, a cut at y=6 would split 2 RAM blocks nicely, but it would cut through a DSP block.
 * The first possible place to have a cut without cutting through a block is the Least Common Multiple of
 * Block Heights of the blocks in the architecture
 *
 *  
 * _______    ______ ______________________> y = 12 OK. = LCM (4,6)
 * |     |    |    |
 * | DSP |    | RAM|
 * |     |    |    |
 * |_____|    |    | ______________________> y = 8 Not OK.
 * |     |    |    | 
 * | DSP |    |____| ______________________> y = 6 Not OK.
 * |     |    |    |
 * |_____|    |    | ______________________> y = 4 Not OK.
 * |     |    |    |  
 * | DSP |    | RAM|
 * |     |    |    |
 * |     |    |    |
 * -------    ------ ----------------------> y = 0
 *
 * Suppose ny = 98. LCM = 4 (e.g. both DSP and RAM are height 4). num_cuts = 2 ==> num_slices = 3.
 * 98 / 4 = 24, so at most you can make 23 cuts (24 slices without cutting through a block).
 * You want each slice to be as tall as possible.
 * 24 / 3 = 8. So, you can have at most 8 of the tallest block in each slice.
 * So, 
 * Slice #1 = y=0  --> y= 8*4= 32     (heigh of slice#1 = 32)
 * Slice #2 = y=32 --> y= 8*4*2 = 64  (heigh of slice#2 = 32)
 * Slice #3 = y=64 --> y=98           (heigh of slice#3 = 34)
 */
void vpr_setup_interposer_cut_locations(t_arch Arch)
{
	if(Arch.lcm_of_block_heights >= ny)
	{
		vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
		"Given the specifications of block heights in the architecture, it is not possible to "
		"use this architecture with %d cuts because a cut would go through a physical block!\n", num_cuts);
	}

        assert(Arch.lcm_of_block_heights > 0);

	int num_slices = num_cuts + 1;
		
	// see explanation above for slice_height
	int slice_height = ((int)(((int)(ny / Arch.lcm_of_block_heights)) / num_slices )) * Arch.lcm_of_block_heights;

	if(slice_height == 0)
	{
		// there's still hope
		slice_height = Arch.lcm_of_block_heights;
	}

	arch_cut_locations = (int*) my_malloc(num_cuts * sizeof(int));
	for(int cut_counter=0; cut_counter<num_cuts; ++cut_counter)
	{
		arch_cut_locations[cut_counter] = (cut_counter+1)*slice_height;
		if( arch_cut_locations[cut_counter] >= ny)
		{
			vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
			"Given the specifications of block heights in the architecture, it is not possible to "
			"use this architecture with %d cuts because a cut would go through a physical block!\n", num_cuts);
		}
		assert(arch_cut_locations[cut_counter]%Arch.lcm_of_block_heights==0);
	}

	vpr_printf_info("Info: Interposer cuts are located at the following coordinates:\n");
	for(int cut_counter=0; cut_counter<num_cuts; ++cut_counter)
	{
		vpr_printf_info("\tInfo: Cut#%d: y=%d\n", cut_counter+1, arch_cut_locations[cut_counter]);
	}

	vpr_printf_info("Info: Height of each SLR (Super Logic Region) is:\n");
	for(int cut_counter=0; cut_counter<=num_cuts; ++cut_counter)
	{
		if(cut_counter==0)
		{
			vpr_printf_info("\tInfo: Region#%d height: %d\n", cut_counter+1, arch_cut_locations[cut_counter]);
		}
		else if(cut_counter==num_cuts)
		{
			vpr_printf_info("\tInfo: Region#%d height: %d\n", cut_counter+1, ny-arch_cut_locations[cut_counter-1]);
		}
		else
		{
			vpr_printf_info("\tInfo: Region#%d height: %d\n", cut_counter+1, arch_cut_locations[cut_counter]-arch_cut_locations[cut_counter-1]);
		}
	}
}
#endif

void vpr_pack(INP t_vpr_setup vpr_setup, INP t_arch arch) {

	clock_t begin = clock();
	vpr_printf_info("Initialize packing.\n");

	/* If needed, estimate inter-cluster delay. Assume the average routing hop goes out of 
	 a block through an opin switch to a length-4 wire, then through a wire switch to another
	 length-4 wire, then through a wire-to-ipin-switch into another block. */

	float inter_cluster_delay = UNDEFINED;
	if (vpr_setup.PackerOpts.timing_driven
			&& vpr_setup.PackerOpts.auto_compute_inter_cluster_net_delay) {

		float Tdel_opin_switch, R_opin_switch, Cout_opin_switch;
		float opin_switch_del = get_switch_info(arch.Segments[0].opin_switch,
				Tdel_opin_switch, R_opin_switch, Cout_opin_switch);

		float Tdel_wire_switch, R_wire_switch, Cout_wire_switch;
		float wire_switch_del = get_switch_info(arch.Segments[0].wire_switch,
				Tdel_wire_switch, R_wire_switch, Cout_wire_switch);

		float Tdel_wtoi_switch, R_wtoi_switch, Cout_wtoi_switch;
		float wtoi_switch_del = get_switch_info(
				vpr_setup.RoutingArch.wire_to_ipin_switch, 
				Tdel_wtoi_switch, R_wtoi_switch, Cout_wtoi_switch);

		float Rmetal = arch.Segments[0].Rmetal;
		float Cmetal = arch.Segments[0].Cmetal;

		/* The delay of a wire with its driving switch is the switch delay plus the 
		 product of the equivalent resistance and capacitance experienced by the wire. */

#define WIRE_SEGMENT_LENGTH 4
		float first_wire_seg_delay = opin_switch_del
				+ (R_opin_switch + Rmetal * WIRE_SEGMENT_LENGTH / 2)
						* (Cout_opin_switch + Cmetal * WIRE_SEGMENT_LENGTH);
		float second_wire_seg_delay = wire_switch_del
				+ (R_wire_switch + Rmetal * WIRE_SEGMENT_LENGTH / 2)
						* (Cout_wire_switch + Cmetal * WIRE_SEGMENT_LENGTH);
		inter_cluster_delay = 4
				* (first_wire_seg_delay + second_wire_seg_delay
						+ wtoi_switch_del); /* multiply by 4 to get a more conservative estimate */
	}

	try_pack(&vpr_setup.PackerOpts, &arch, vpr_setup.user_models,
			vpr_setup.library_models, vpr_setup.Timing, inter_cluster_delay, vpr_setup.PackerRRGraph);

	clock_t end = clock();
#ifdef CLOCKS_PER_SEC
	vpr_printf_info("Packing took %g seconds.\n", (float) (end - begin) / CLOCKS_PER_SEC);
	vpr_printf_info("Packing completed.\n");
#else
	vpr_printf_info("Packing took %g seconds.\n", (float)(end - begin) / CLK_PER_SEC);
#endif
	fflush(stdout);
}

boolean vpr_place_and_route(INP t_vpr_setup vpr_setup, INP t_arch arch) {

	/* Startup X graphics */
	init_graphics_state(vpr_setup.ShowGraphics, vpr_setup.GraphPause,
			vpr_setup.RouterOpts.route_type);
	if (vpr_setup.ShowGraphics) {
		init_graphics("VPR:  Versatile Place and Route for FPGAs", WHITE);
		alloc_draw_structs();
	}

	/* Do placement and routing */
	boolean success = place_and_route(vpr_setup.Operation, vpr_setup.PlacerOpts,
			vpr_setup.FileNameOpts.PlaceFile, vpr_setup.FileNameOpts.NetFile,
			vpr_setup.FileNameOpts.ArchFile, vpr_setup.FileNameOpts.RouteFile,
			vpr_setup.AnnealSched, vpr_setup.RouterOpts, vpr_setup.RoutingArch,
			vpr_setup.Segments, vpr_setup.Timing, arch.Chans, arch.models,
			arch.Directs, arch.num_directs);
	fflush(stdout);

	/* Close down X Display */
	if (vpr_setup.ShowGraphics)
		close_graphics();
	free_draw_structs();

	return(success);
}

/* Free architecture data structures */
void free_arch(t_arch* Arch) {

	freeGrid();
	free(chan_width.x_list);
	free(chan_width.y_list);

	chan_width.x_list = chan_width.y_list = NULL;
	chan_width.max = chan_width.x_max = chan_width.y_max = chan_width.x_min = chan_width.y_min = 0;

	for (int i = 0; i < Arch->num_switches; ++i) {
		if (Arch->Switches->name != NULL) {
			free(Arch->Switches[i].name);
		}
	}
	free(Arch->Switches);
	free(switch_inf);
	for (int i = 0; i < Arch->num_segments; ++i) {
		if (Arch->Segments->cb != NULL) {
			free(Arch->Segments[i].cb);
		}
		if (Arch->Segments->sb != NULL) {
			free(Arch->Segments[i].sb);
		}
	}
	free(Arch->Segments);
	t_model *model = Arch->models;
	while (model) {
		t_model_ports *input_port = model->inputs;
		while (input_port) {
			t_model_ports *prev_port = input_port;
			input_port = input_port->next;
			free(prev_port->name);
			free(prev_port);
		}
		t_model_ports *output_port = model->outputs;
		while (output_port) {
			t_model_ports *prev_port = output_port;
			output_port = output_port->next;
			free(prev_port->name);
			free(prev_port);
		}
		struct s_linked_vptr *vptr = model->pb_types;
		while (vptr) {
			struct s_linked_vptr *vptr_prev = vptr;
			vptr = vptr->next;
			free(vptr_prev);
		}
		t_model *prev_model = model;

		model = model->next;
		if (prev_model->instances)
			free(prev_model->instances);
		free(prev_model->name);
		free(prev_model);
	}

	for (int i = 0; i < 4; ++i) {
		struct s_linked_vptr *vptr = Arch->model_library[i].pb_types;
		while (vptr) {
			struct s_linked_vptr *vptr_prev = vptr;
			vptr = vptr->next;
			free(vptr_prev);
		}
	}

	for (int i = 0; i < Arch->num_directs; ++i) {
		free(Arch->Directs[i].name);
		free(Arch->Directs[i].from_pin);
		free(Arch->Directs[i].to_pin);
	}
	free(Arch->Directs);

	free(Arch->model_library[0].name);
	free(Arch->model_library[0].outputs->name);
	free(Arch->model_library[0].outputs);
	free(Arch->model_library[1].inputs->name);
	free(Arch->model_library[1].inputs);
	free(Arch->model_library[1].name);
	free(Arch->model_library[2].name);
	free(Arch->model_library[2].inputs[0].name);
	free(Arch->model_library[2].inputs[1].name);
	free(Arch->model_library[2].inputs);
	free(Arch->model_library[2].outputs->name);
	free(Arch->model_library[2].outputs);
	free(Arch->model_library[3].name);
	free(Arch->model_library[3].inputs->name);
	free(Arch->model_library[3].inputs);
	free(Arch->model_library[3].outputs->name);
	free(Arch->model_library[3].outputs);
	free(Arch->model_library);

	if (Arch->clocks) {
		free(Arch->clocks->clock_inf);
	}

	free_complex_block_types();
	free_chunk_memory_trace();
}

void free_options(t_options *options) {

	free(options->ArchFile);
	free(options->CircuitName);
	if (options->ActFile)
		free(options->ActFile);
	if (options->BlifFile)
		free(options->BlifFile);
	if (options->NetFile)
		free(options->NetFile);
	if (options->PlaceFile)
		free(options->PlaceFile);
	if (options->PowerFile)
		free(options->PowerFile);
	if (options->CmosTechFile)
		free(options->CmosTechFile);
	if (options->RouteFile)
		free(options->RouteFile);
	if (options->out_file_prefix)
		free(options->out_file_prefix);
	if (options->PinFile)
		free(options->PinFile);
}

static void free_complex_block_types(void) {

	free_all_pb_graph_nodes();

	for (int i = 0; i < num_types; ++i) {
		if (&type_descriptors[i] == EMPTY_TYPE) {
			continue;
		}
		free(type_descriptors[i].name);
		for (int width = 0; width < type_descriptors[i].width; ++width) {
			for (int height = 0; height < type_descriptors[i].height; ++height) {
				for (int side = 0; side < 4; ++side) {
					for (int pin = 0; pin < type_descriptors[i].num_pin_loc_assignments[width][height][side]; ++pin) {
						if (type_descriptors[i].pin_loc_assignments[width][height][side][pin])
							free(type_descriptors[i].pin_loc_assignments[width][height][side][pin]);
					}
					free(type_descriptors[i].pinloc[width][height][side]);
					free(type_descriptors[i].pin_loc_assignments[width][height][side]);
				}
				free(type_descriptors[i].pinloc[width][height]);
				free(type_descriptors[i].pin_loc_assignments[width][height]);
				free(type_descriptors[i].num_pin_loc_assignments[width][height]);
			}
			free(type_descriptors[i].pinloc[width]);
			free(type_descriptors[i].pin_loc_assignments[width]);
			free(type_descriptors[i].num_pin_loc_assignments[width]);
		}
		free(type_descriptors[i].pinloc);
		free(type_descriptors[i].pin_loc_assignments);
		free(type_descriptors[i].num_pin_loc_assignments);

		free(type_descriptors[i].pin_width);
		free(type_descriptors[i].pin_height);

		for (int j = 0; j < type_descriptors[i].num_class; ++j) {
			free(type_descriptors[i].class_inf[j].pinlist);
		}
		free(type_descriptors[i].class_inf);
		free(type_descriptors[i].is_global_pin);
		free(type_descriptors[i].pin_class);
		free(type_descriptors[i].grid_loc_def);

		free(type_descriptors[i].is_Fc_frac);
		free(type_descriptors[i].is_Fc_full_flex);
		free(type_descriptors[i].Fc);

		free_pb_type(type_descriptors[i].pb_type);
		free(type_descriptors[i].pb_type);
	}
	free(type_descriptors);
}

static void free_pb_type(t_pb_type *pb_type) {

	free(pb_type->name);
	if (pb_type->blif_model)
		free(pb_type->blif_model);

	for (int i = 0; i < pb_type->num_modes; ++i) {
		for (int j = 0; j < pb_type->modes[i].num_pb_type_children; ++j) {
			free_pb_type(&pb_type->modes[i].pb_type_children[j]);
		}
		free(pb_type->modes[i].pb_type_children);
		free(pb_type->modes[i].name);
		for (int j = 0; j < pb_type->modes[i].num_interconnect; ++j) {
			free(pb_type->modes[i].interconnect[j].input_string);
			free(pb_type->modes[i].interconnect[j].output_string);
			free(pb_type->modes[i].interconnect[j].name);

			for (int k = 0; k < pb_type->modes[i].interconnect[j].num_annotations; ++k) {
				if (pb_type->modes[i].interconnect[j].annotations[k].clock)
					free(pb_type->modes[i].interconnect[j].annotations[k].clock);
				if (pb_type->modes[i].interconnect[j].annotations[k].input_pins) {
					free(pb_type->modes[i].interconnect[j].annotations[k].input_pins);
				}
				if (pb_type->modes[i].interconnect[j].annotations[k].output_pins) {
					free(pb_type->modes[i].interconnect[j].annotations[k].output_pins);
				}
				for (int m = 0; m < pb_type->modes[i].interconnect[j].annotations[k].num_value_prop_pairs; ++m) {
					free(pb_type->modes[i].interconnect[j].annotations[k].value[m]);
				}
				free(pb_type->modes[i].interconnect[j].annotations[k].prop);
				free(pb_type->modes[i].interconnect[j].annotations[k].value);
			}
			free(pb_type->modes[i].interconnect[j].annotations);
			if (pb_type->modes[i].interconnect[j].interconnect_power)
				free(pb_type->modes[i].interconnect[j].interconnect_power);
		}
		if (pb_type->modes[i].interconnect)
			free(pb_type->modes[i].interconnect);
		if (pb_type->modes[i].mode_power)
			free(pb_type->modes[i].mode_power);
	}
	if (pb_type->modes)
		free(pb_type->modes);

	for (int i = 0; i < pb_type->num_annotations; ++i) {
		for (int j = 0; j < pb_type->annotations[i].num_value_prop_pairs; ++j) {
			free(pb_type->annotations[i].value[j]);
		}
		free(pb_type->annotations[i].value);
		free(pb_type->annotations[i].prop);
		if (pb_type->annotations[i].input_pins) {
			free(pb_type->annotations[i].input_pins);
		}
		if (pb_type->annotations[i].output_pins) {
			free(pb_type->annotations[i].output_pins);
		}
		if (pb_type->annotations[i].clock) {
			free(pb_type->annotations[i].clock);
		}
	}
	if (pb_type->num_annotations > 0) {
		free(pb_type->annotations);
	}

	if (pb_type->pb_type_power) {
		free(pb_type->pb_type_power);
	}

	for (int i = 0; i < pb_type->num_ports; ++i) {
		free(pb_type->ports[i].name);
		if (pb_type->ports[i].port_class) {
			free(pb_type->ports[i].port_class);
		}
		if (pb_type->ports[i].port_power) {
			free(pb_type->ports[i].port_power);
		}
	}
	free(pb_type->ports);
}

void free_circuit() {

	/* Free netlist reference tables for nets */
	free(clb_to_vpack_net_mapping);
	free(vpack_to_clb_net_mapping);
	clb_to_vpack_net_mapping = NULL;
	vpack_to_clb_net_mapping = NULL;

	/* Free logical blocks and nets */
	if (logical_block != NULL) {
		free_logical_blocks();
		free_logical_nets();
	}

	if (clb_net != NULL) {
		for (int i = 0; i < num_nets; ++i) {
			free(clb_net[i].name);
			free(clb_net[i].node_block);
			free(clb_net[i].node_block_pin);
			free(clb_net[i].node_block_port);
		}
	}
	free(clb_net);
	clb_net = NULL;

	//Free new net structures
	free_global_nlist_net(&g_clbs_nlist);
	free_global_nlist_net(&g_atoms_nlist);

	if (block != NULL) {
		for (int i = 0; i < num_blocks; ++i) {
			if (block[i].pb != NULL) {
				free_cb(block[i].pb);
				free(block[i].pb);
			}
			free(block[i].nets);
			free(block[i].name);
		}
	}
	free(block);
	block = NULL;

	free(blif_circuit_name);
	free(default_output_name);
	blif_circuit_name = NULL;

	struct s_linked_vptr *p_io_removed = circuit_p_io_removed;
	while (p_io_removed != NULL) {
		circuit_p_io_removed = p_io_removed->next;
		free(p_io_removed->data_vptr);
		free(p_io_removed);
		p_io_removed = circuit_p_io_removed;
	}
}

void vpr_free_vpr_data_structures(INOUTP t_arch Arch, 
		INOUTP t_options options,
		INOUTP t_vpr_setup vpr_setup) {

	if (vpr_setup.Timing.SDCFile != NULL) {
		free(vpr_setup.Timing.SDCFile);
		vpr_setup.Timing.SDCFile = NULL;
	}

	free_all_lb_type_rr_graph(vpr_setup.PackerRRGraph);
	free_options(&options);
	free_circuit();
	free_arch(&Arch);
	free_echo_file_info();
	free_output_file_names();
	free_timing_stats();
	free_sdc_related_structs();
}

void vpr_free_all(INOUTP t_arch Arch, 
		INOUTP t_options options,
		INOUTP t_vpr_setup vpr_setup) {

	free_rr_graph();
	if (vpr_setup.RouterOpts.doRouting) {
		free_route_structs();
	}
	free_trace_structs();
	vpr_free_vpr_data_structures(Arch, options, vpr_setup);
	if (has_printhandler_pre_vpr == FALSE) {
		PrintHandlerDelete();
	}

	/* Finally, destroy Toro-specific handler 'singleton' objects */
	toro_delete_handlers();
}

//===========================================================================//
static void toro_new_handlers(void) {

	// Initialize various fabric handler 'singleton' objects
	TFH_FabricGridHandler_c::NewInstance();
	TFH_FabricBlockHandler_c::NewInstance();
	TFH_FabricChannelHandler_c::NewInstance();
	TFH_FabricSwitchBoxHandler_c::NewInstance();
	TFH_FabricConnectionBlockHandler_c::NewInstance();

	// Initialize various circuit handler 'singleton' objects
	TCH_RegionPlaceHandler_c::NewInstance();
	TCH_RelativePlaceHandler_c::NewInstance();
	TCH_PrePlacedHandler_c::NewInstance();
	TCH_PreRoutedHandler_c::NewInstance();
}

//===========================================================================//
static void toro_delete_handlers(void) {

	// Destroy various fabric handler 'singleton' objects
	TFH_FabricGridHandler_c::DeleteInstance();
	TFH_FabricBlockHandler_c::DeleteInstance();
	TFH_FabricChannelHandler_c::DeleteInstance();
	TFH_FabricSwitchBoxHandler_c::DeleteInstance();
	TFH_FabricConnectionBlockHandler_c::DeleteInstance();

	// Destroy various circuit handler 'singleton' objects
	TCH_RegionPlaceHandler_c::DeleteInstance();
	TCH_RelativePlaceHandler_c::DeleteInstance();
	TCH_PrePlacedHandler_c::DeleteInstance();
	TCH_PreRoutedHandler_c::DeleteInstance();
}

/****************************************************************************************************
 * Advanced functions
 *  Used when you need fine-grained control over VPR that the main VPR operations do not enable
 ****************************************************************************************************/
/* Read in user options */
void vpr_read_options(INP int argc, INP char **argv, OUTP t_options * options) {
	ReadOptions(argc, argv, options);
}

/* Read in arch and circuit */
void vpr_setup_vpr(INP t_options *Options, INP boolean TimingEnabled,
		INP boolean readArchFile, OUTP struct s_file_name_opts *FileNameOpts,
		INOUTP t_arch * Arch, OUTP enum e_operation *Operation,
		OUTP t_model ** user_models, OUTP t_model ** library_models,
		OUTP struct s_packer_opts *PackerOpts,
		OUTP struct s_placer_opts *PlacerOpts,
		OUTP struct s_annealing_sched *AnnealSched,
		OUTP struct s_router_opts *RouterOpts,
		OUTP struct s_det_routing_arch *RoutingArch,
		OUTP vector <t_lb_type_rr_node> **PackerRRGraph,
		OUTP t_segment_inf ** Segments, OUTP t_timing_inf * Timing,
		OUTP boolean * ShowGraphics, OUTP int *GraphPause,
		t_power_opts * PowerOpts) {
	SetupVPR(Options, TimingEnabled, readArchFile, FileNameOpts, Arch,
			Operation, user_models, library_models, PackerOpts, PlacerOpts,
			AnnealSched, RouterOpts, RoutingArch, PackerRRGraph, Segments, Timing,
			ShowGraphics, GraphPause, PowerOpts);
}
/* Check inputs are reasonable */
void vpr_check_options(INP t_options Options, INP boolean TimingEnabled) {
	CheckOptions(Options, TimingEnabled);
}
void vpr_check_arch(INP t_arch Arch, INP boolean TimingEnabled) {
	CheckArch(Arch, TimingEnabled);
}
/* Verify settings don't conflict or otherwise not make sense */
void vpr_check_setup(INP enum e_operation Operation,
		INP struct s_placer_opts PlacerOpts,
		INP struct s_annealing_sched AnnealSched,
		INP struct s_router_opts RouterOpts,
		INP struct s_det_routing_arch RoutingArch, INP t_segment_inf * Segments,
		INP t_timing_inf Timing, INP t_chan_width_dist Chans) {
	CheckSetup(Operation, PlacerOpts, AnnealSched, RouterOpts, RoutingArch,
			Segments, Timing, Chans);
}
/* Read blif file and sweep unused components */
void vpr_read_and_process_blif(INP char *blif_file,
		INP boolean sweep_hanging_nets_and_inputs, INP t_model *user_models,
		INP t_model *library_models, boolean read_activity_file,
		char * activity_file) {
	read_and_process_blif(blif_file, sweep_hanging_nets_and_inputs, user_models,
			library_models, read_activity_file, activity_file);
}
/* Show current setup */
void vpr_show_setup(INP t_options options, INP t_vpr_setup vpr_setup) {
	ShowSetup(options, vpr_setup);
}

/* Output file names management */
void vpr_alloc_and_load_output_file_names(const char* default_name) {
	alloc_and_load_output_file_names(default_name);
}
void vpr_set_output_file_name(enum e_output_files ename, const char *name,
		const char* default_name) {
	setOutputFileName(ename, name, default_name);
}
char *vpr_get_output_file_name(enum e_output_files ename) {
	return getOutputFileName(ename);
}

/* logical equivalence scrambles the packed netlist indices with the actual indices, need to resync then re-output clustered netlist, this code assumes I'm dealing with a TI CLAY v1 architecture */
/* Returns a trace array [0..g_atoms_nlist.net.size()-1] with the final routing of the circuit from the logical_block netlist, index of the trace array corresponds to the index of a g_atoms_nlist.net */
t_trace* vpr_resync_post_route_netlist_to_TI_CLAY_v1_architecture(
		INP boolean apply_logical_equivalence_handling,
		INP const t_arch *arch) {

	t_trace *trace = 0;

	/* Map post-routed traces to clb_nets and block */
	resync_post_route_netlist();

	/* Resolve logically equivalent inputs */
	boolean success = apply_logical_equivalence_handling ? 
			clay_logical_equivalence_handling(arch) : TRUE;
	if(success) {

		/* Finalize traceback */
		trace = alloc_and_load_final_routing_trace();
		if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_COMPLETE_NET_TRACE)) {
			print_complete_net_trace(trace,	getEchoFileName(E_ECHO_COMPLETE_NET_TRACE));
		}
	}
	return trace;
}

/* reload intra cluster nets to complex block */
static void reload_intra_cluster_nets(t_pb *pb) {

	const t_pb_type* pb_type = pb->pb_graph_node->pb_type;
	if (pb_type->blif_model != NULL) {
		setup_intracluster_routing_for_logical_block(pb->logical_block,	pb->pb_graph_node);
	} else if (pb->child_pbs != NULL) {
		set_pb_graph_mode(pb->pb_graph_node, pb->mode, 1);
		for (int i = 0; i < pb_type->modes[pb->mode].num_pb_type_children; ++i) {
			for (int j = 0; j < pb_type->modes[pb->mode].pb_type_children[i].num_pb; ++j) {
				if (pb->child_pbs[i] != NULL) {
					if (pb->child_pbs[i][j].name != NULL) {
						reload_intra_cluster_nets(&pb->child_pbs[i][j]);
					}
				}
			}
		}
	}
}

/* Determine trace from logical_block output to logical_block inputs
 Algorithm traverses intra-block routing, goes to inter-block routing, then returns to intra-block routing
 */
static t_trace *alloc_and_load_final_routing_trace() {

	t_trace* final_routing_trace = (t_trace*) my_calloc(g_atoms_nlist.net.size(), sizeof(t_trace));
	for (unsigned int i = 0; i < g_atoms_nlist.net.size(); ++i) {
		int iblock = logical_block[g_atoms_nlist.net[i].pins[0].block].clb_index;

		final_routing_trace[i].iblock = iblock;
		final_routing_trace[i].iswitch = OPEN;
		final_routing_trace[i].index = OPEN;
		final_routing_trace[i].next = NULL;

		t_pb_graph_pin *pin = get_pb_graph_node_pin_from_g_atoms_nlist_net(i, 0);
		if (!pin)
			continue;
		final_routing_trace[i].index = pin->pin_count_in_cluster;

		expand_routing_trace(&final_routing_trace[i], i);
	}
	return final_routing_trace;
}

/* Given a routing trace, expand until full trace is complete 
 returns pointer to last terminal trace
 */
static t_trace *expand_routing_trace(t_trace *trace, int i_g_atoms_net) {

	t_trace *current = trace;

	int iblock = trace->iblock;
	int inode = trace->index;
	t_rr_node *local_rr_graph = block[iblock].pb->rr_graph;

	if (local_rr_graph[inode].pb_graph_pin->num_output_edges == 0) {
		if (local_rr_graph[inode].pb_graph_pin->port->type == OUT_PORT) {
			/* connection to outside cb */
			if (g_atoms_nlist.net[i_g_atoms_net].is_global) {
				int inet = vpack_to_clb_net_mapping[i_g_atoms_net];
				if (inet != OPEN) {
					for (int ipin = 1; ipin < (int) g_clbs_nlist.net[inet].pins.size(); ++ipin) {
						t_pb_graph_pin *pb_graph_pin = get_pb_graph_node_pin_from_g_clbs_nlist_net(inet, ipin);
						t_trace *new_trace = (t_trace*) my_calloc(1, sizeof(t_trace));
						new_trace->iblock = g_clbs_nlist.net[inet].pins[ipin].block;
						new_trace->index = pb_graph_pin->pin_count_in_cluster;
						new_trace->iswitch = OPEN;
						new_trace->num_siblings = 0;
						new_trace->next = NULL;
						current->next = new_trace;
						current = expand_routing_trace(new_trace, i_g_atoms_net);
					}
				}
			} else {
				t_trace *inter_cb_trace = trace_head[vpack_to_clb_net_mapping[i_g_atoms_net]];
				if (inter_cb_trace != NULL) {
					inter_cb_trace = inter_cb_trace->next; /* skip source and go right to opin */
				}
				while (inter_cb_trace != NULL) {
					/* continue traversing inter cb trace */
					if (rr_node[inter_cb_trace->index].type != SINK) {
						t_trace *new_trace = (t_trace*) my_calloc(1, sizeof(t_trace));
						new_trace->iblock = OPEN;
						new_trace->index = inter_cb_trace->index;
						new_trace->iswitch = inter_cb_trace->iswitch;
						new_trace->num_siblings = 0;
						new_trace->next = NULL;
						current->next = new_trace;
						if (rr_node[inter_cb_trace->index].type == IPIN) {
							current = current->next;
							int gridx = rr_node[new_trace->index].xlow;
							int gridy = rr_node[new_trace->index].ylow;
							gridx = gridx - grid[gridx][gridy].width_offset;
							gridy = gridy - grid[gridx][gridy].height_offset;
							new_trace = (t_trace*) my_calloc(1, sizeof(t_trace));
							new_trace->iblock = grid[gridx][gridy].blocks[rr_node[inter_cb_trace->index].z];
							new_trace->index = rr_node[inter_cb_trace->index].pb_graph_pin->pin_count_in_cluster;
							new_trace->iswitch = OPEN;
							new_trace->num_siblings = 0;
							new_trace->next = NULL;
							current->next = new_trace;
							current = expand_routing_trace(new_trace, i_g_atoms_net);
						} else {
							current = current->next;
						}
					}
					inter_cb_trace = inter_cb_trace->next;
				}
			}
		}
	} else {
		/* connection to another intra-cluster pin */
		current = trace;
		boolean success = FALSE;
		for (int i = 0; i < local_rr_graph[inode].num_edges; ++i) {
			if (local_rr_graph[local_rr_graph[inode].edges[i]].prev_node
					== inode) {
				if (success == FALSE) {
					success = TRUE;
				} else {
					current->next = (t_trace*) my_calloc(1, sizeof(t_trace));
					current = current->next;
					current->iblock = trace->iblock;
					current->index = trace->index;
					current->iswitch = trace->iswitch;
					current->next = NULL;
				}
				t_trace *new_trace = (t_trace*) my_calloc(1, sizeof(t_trace));
				new_trace->iblock = trace->iblock;
				new_trace->index = local_rr_graph[inode].edges[i];
				new_trace->iswitch = OPEN;
				new_trace->num_siblings = 0;
				new_trace->next = NULL;
				current->next = new_trace;
				current = expand_routing_trace(new_trace, i_g_atoms_net);
			}
		}
	}
	return current;
}

static void print_complete_net_trace(t_trace* trace, const char *file_name) {

	FILE *fp = my_fopen(file_name, "w", 0);

	for (unsigned int i = 0; i < g_atoms_nlist.net.size(); ++i) {
		fprintf(fp, "Net %s (%d)\n\n", g_atoms_nlist.net[i].name, i);

		int iprev_block = OPEN;

		t_trace *current = &trace[i];
		while (current != NULL) {
			int iblock = current->iblock;
			int inode = current->index;
			if (iblock != OPEN) {
				if (iprev_block != iblock) {
					iprev_block = iblock;
					fprintf(fp, "Block %s (%d) (%d, %d, %d):\n",
							block[iblock].name, iblock, block[iblock].x,
							block[iblock].y, block[iblock].z);
				}
				t_rr_node *local_rr_graph = block[iblock].pb->rr_graph;
				fprintf(fp, "Node:\t%d\t%s[%d].%s[%d]", 
						inode,
						local_rr_graph[inode].pb_graph_pin->parent_node->pb_type->name,
						local_rr_graph[inode].pb_graph_pin->parent_node->placement_index,
						local_rr_graph[inode].pb_graph_pin->port->name,
						local_rr_graph[inode].pb_graph_pin->pin_number);
			} else {
				fprintf(fp, "Node:\t%d\t%6s (%d,%d) ", inode,
						rr_node[inode].rr_get_type_string(),
						rr_node[inode].xlow, rr_node[inode].ylow);

				if ((rr_node[inode].xlow != rr_node[inode].xhigh)
						|| (rr_node[inode].ylow != rr_node[inode].yhigh))
					fprintf(fp, "to (%d,%d) ", rr_node[inode].xhigh, rr_node[inode].yhigh);

				switch (rr_node[inode].type) {

				case IPIN:
				case OPIN:
					if (grid[rr_node[inode].xlow][rr_node[inode].ylow].type == IO_TYPE) {
						fprintf(fp, " Pad: ");
					} else { /* IO Pad. */
						fprintf(fp, " Pin: ");
					}
					break;

				case CHANX:
				case CHANY:
					fprintf(fp, " Track: ");
					break;

				case SOURCE:
				case SINK:
					if (grid[rr_node[inode].xlow][rr_node[inode].ylow].type == IO_TYPE) {
						fprintf(fp, " Pad: ");
					} else { /* IO Pad. */
						fprintf(fp, " Class: ");
					}
					break;

				default:
					vpr_printf_error(__FILE__, __LINE__,
							"in print_route: Unexpected traceback element type: %d (%s).\n",
							rr_node[inode].type,
							rr_node[inode].rr_get_type_string());
					exit(1);
					break;
				}

				fprintf(fp, "%d  ", rr_node[inode].ptc_num);

				/* Uncomment line below if you're debugging and want to see the switch types *
				 * used in the routing.                                                      */
				/* fprintf (fp, "Switch: %d", tptr->iswitch); */
			}
			fprintf(fp, "\n");

			current = current->next;
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
}

void resync_post_route_netlist() {

	for (int i = 0; i < num_blocks; ++i) {
		for (int j = 0; j < block[i].type->num_pins; ++j) {
			if (block[i].nets[j] != OPEN
					&& g_clbs_nlist.net[block[i].nets[j]].is_global == FALSE)
				block[i].nets[j] = OPEN;
		}
	}
	for (unsigned int i = 0; i < g_clbs_nlist.net.size(); ++i) {
		if (g_clbs_nlist.net[i].is_global == TRUE)
			continue;

		int j = 0;

		t_trace *trace = (trace_head ? trace_head[i] : 0);
		while (trace != NULL) {
			if (rr_node[trace->index].type == OPIN && j == 0) {
				int gridx = rr_node[trace->index].xlow;
				int gridy = rr_node[trace->index].ylow;
				gridx = gridx - grid[gridx][gridy].width_offset;
				gridy = gridy - grid[gridx][gridy].height_offset;

				int iblock = grid[gridx][gridy].blocks[rr_node[trace->index].z];
				assert(g_clbs_nlist.net[i].pins[j].block == iblock);
				g_clbs_nlist.net[i].pins[j].block_pin = rr_node[trace->index].ptc_num;
				clb_net[i].node_block_pin[j] = rr_node[trace->index].ptc_num; //Daniel to-do: take out clb_net
				block[iblock].nets[rr_node[trace->index].ptc_num] = i;
				j++;
			} else if (rr_node[trace->index].type == IPIN) {
				int gridx = rr_node[trace->index].xlow;
				int gridy = rr_node[trace->index].ylow;
				gridx = gridx - grid[gridx][gridy].width_offset;
				gridy = gridy - grid[gridx][gridy].height_offset;

				int iblock = grid[gridx][gridy].blocks[rr_node[trace->index].z];
				g_clbs_nlist.net[i].pins[j].block = iblock;
				g_clbs_nlist.net[i].pins[j].block_pin = rr_node[trace->index].ptc_num;
				clb_net[i].node_block[j] = iblock; //Daniel to-do: take out clb_net
				clb_net[i].node_block_pin[j] = rr_node[trace->index].ptc_num; //Daniel to-do: take out clb_net
				block[iblock].nets[rr_node[trace->index].ptc_num] = i;
				j++;
			}
			trace = trace->next;
		}
	}
}

static boolean clay_logical_equivalence_handling(const t_arch *arch) {

	boolean success = FALSE;

	for (int i = 0; i < num_blocks; ++i) {
		clay_reload_ble_locations(i);
	}

	/* Resolve logically equivalent inputs */
	t_trace **saved_ext_rr_trace_head = trace_head;
	t_trace **saved_ext_rr_trace_tail = trace_tail;
	t_rr_node *saved_ext_rr_node = rr_node;
	int num_ext_rr_node = num_rr_nodes;
	int num_ext_nets = (int) g_clbs_nlist.net.size();

	num_rr_nodes = 0;
	rr_node = NULL;
	trace_head = NULL;
	trace_tail = NULL;
	free_rr_graph(); /* free all data structures associated with rr_graph */

	alloc_and_load_cluster_legality_checker();
	for (int i = 0; i < num_blocks; ++i) {

		/* Regenerate rr_graph (note, can be more runtime efficient but this allows for more code reuse) */
		rr_node = block[i].pb->rr_graph;
		num_rr_nodes = block[i].pb->pb_graph_node->total_pb_pins;
		free_legalizer_for_cluster(&block[i], TRUE);
		alloc_and_load_legalizer_for_cluster(&block[i], i, arch);
		reload_intra_cluster_nets(block[i].pb);
		reload_ext_net_rr_terminal_cluster();
		success = force_post_place_route_cb_input_pins(i);
		if(!success)
			break;

#ifdef HACK_LUT_PIN_SWAPPING
		/* Resolve rebalancing of LUT inputs */
		clay_lut_input_rebalancing(i, block[i].pb);
#endif

		/* reset rr_graph */
		for (int j = 0; j < num_rr_nodes; ++j) {
			rr_node[j].occ = 0;
			rr_node[j].prev_edge = OPEN;
			rr_node[j].prev_node = OPEN;
		}
		if (try_breadth_first_route_cluster() == FALSE) {
			vpr_printf_error(__FILE__, __LINE__, 
					"Failed to resync post routed solution with clustered netlist.\n");
			vpr_printf_error(__FILE__, __LINE__, 
					"Cannot recover from error.\n");
			exit(1);
		}
		save_cluster_solution();
		reset_legalizer_for_cluster(&block[i]);
		free_legalizer_for_cluster(&block[i], FALSE);
	}
	free_cluster_legality_checker();

	trace_head = saved_ext_rr_trace_head;
	trace_tail = saved_ext_rr_trace_tail;
	rr_node = saved_ext_rr_node;
	num_rr_nodes = num_ext_rr_node;
	num_nets = num_ext_nets; //Daniel to-do: Take out?

	return(success);
}

/* Force router to use the LUT inputs designated by the timing engine post the LUT input rebalancing optimization */
static void clay_lut_input_rebalancing(int iblock, t_pb *pb) {

	if (pb->name != NULL) {
		t_pb_graph_node *pb_graph_node = pb->pb_graph_node;
		if (pb_graph_node->pb_type->blif_model != NULL) {
			int *lut_pin_remap = pb->lut_pin_remap;
			if (lut_pin_remap != NULL) {
				t_rr_node *local_rr_graph = block[iblock].pb->rr_graph;
				t_pb_graph_node *lut = pb->pb_graph_node;
				t_pb_graph_node *lut_wrapper = lut->parent_pb_graph_node;

				/* Ensure that this is actually a LUT */
				assert(lut->num_input_ports == 1 && lut_wrapper->num_input_ports == 1);
				assert(lut->num_input_pins[0] == lut_wrapper->num_input_pins[0]);
				assert(lut->num_output_ports == 1 && lut_wrapper->num_output_ports == 1);
				assert(lut->num_output_pins[0] == 1 && lut_wrapper->num_output_pins[0] == 1);

				int lut_size = lut->num_input_pins[0];
				for (int i = 0; i < lut_size; ++i) {
					int snode = lut_wrapper->input_pins[0][i].pin_count_in_cluster;
					free(local_rr_graph[snode].edges);
					local_rr_graph[snode].edges = NULL;
					local_rr_graph[snode].num_edges = 0;
				}
				for (int i = 0; i < lut_size; ++i) {
					int input = lut_pin_remap[i];
					if (input != OPEN) {
						int snode = lut_wrapper->input_pins[0][i].pin_count_in_cluster;
						assert(local_rr_graph[snode].num_edges == 0);
						local_rr_graph[snode].num_edges = 1;
						local_rr_graph[snode].edges = (int*) my_malloc(sizeof(int));
						local_rr_graph[snode].edges[0] = lut->input_pins[0][input].pin_count_in_cluster;
					}
				}
			}
		} else if (pb->child_pbs != NULL) {
			for (int i = 0; i < pb_graph_node->pb_type->modes[pb->mode].num_pb_type_children; ++i) {
				if (pb->child_pbs[i] != NULL) {
					for (int j = 0; j < pb_graph_node->pb_type->modes[pb->mode].pb_type_children[i].num_pb; ++j) {
						clay_lut_input_rebalancing(iblock, &pb->child_pbs[i][j]);
					}
				}
			}
		}
	}
}

/* Swaps BLEs to match output logical equivalence solution from routing solution
 Assumes classical cluster with full crossbar and BLEs, each BLE is a single LUT+FF pair
 */
static void clay_reload_ble_locations(int iblock) {

	if (block[iblock].type == IO_TYPE) {
		return;
	}

	t_pb_graph_node *pb_graph_node = block[iblock].pb->pb_graph_node;
	const t_pb_type *pb_type = pb_graph_node->pb_type;
	int mode = block[iblock].pb->mode;
	t_rr_node *local_rr_graph = block[iblock].pb->rr_graph;

	assert(block[iblock].pb->mode == 0);
	assert(pb_type->modes[mode].num_pb_type_children == 1);
	assert(pb_type->modes[mode].pb_type_children[0].num_output_pins == 1);

	t_pb** temp = (t_pb**) my_calloc(1, sizeof(t_pb*));
	temp[0] = (t_pb*) my_calloc(pb_type->modes[mode].pb_type_children[0].num_pb, sizeof(t_pb));

	/* determine new location for BLEs that route out of cluster */
	for (int i = 0; i < pb_type->modes[mode].pb_type_children[0].num_pb; ++i) {
		if (block[iblock].pb->child_pbs[0][i].name != NULL) {
			int i_g_atoms_net = local_rr_graph[pb_graph_node->child_pb_graph_nodes[mode][0][i].output_pins[0][0].pin_count_in_cluster].net_num;
			int inet = vpack_to_clb_net_mapping[i_g_atoms_net];
			if (inet != OPEN) {
				int ipin = OPEN;
				t_trace *trace = (trace_head ? trace_head[inet] : 0);
				while (trace) {
					if (rr_node[trace->index].type == OPIN) {
						ipin = rr_node[trace->index].ptc_num;
						break;
					}
					trace = trace->next;
				}
				assert(ipin);

				t_pb_graph_pin *pb_graph_pin = get_pb_graph_node_pin_from_block_pin(iblock, ipin);
				int new_loc = pb_graph_pin->pin_number;
				if (temp[0][new_loc].name)
					continue;
				temp[0][new_loc] = block[iblock].pb->child_pbs[0][i];
			}
		}
	}

	/* determine new location for BLEs that do not route out of cluster */
	int new_loc = 0;
	for (int i = 0; i < pb_type->modes[mode].pb_type_children[0].num_pb; ++i) {
		if (block[iblock].pb->child_pbs[0][i].name != NULL) {
			int i_g_atoms_net = local_rr_graph[pb_graph_node->child_pb_graph_nodes[mode][0][i].output_pins[0][0].pin_count_in_cluster].net_num;
			int inet = vpack_to_clb_net_mapping[i_g_atoms_net];
			if (inet == OPEN) {
				while (temp[0][new_loc].name != NULL) {
					new_loc++;
				}
				temp[0][new_loc] = block[iblock].pb->child_pbs[0][i];
			}
		}
	}

	free(block[iblock].pb->child_pbs);
	block[iblock].pb->child_pbs = temp;
	resync_pb_graph_nodes_in_pb(block[iblock].pb->pb_graph_node, block[iblock].pb);
}

static void resync_pb_graph_nodes_in_pb(t_pb_graph_node *pb_graph_node,
		t_pb *pb) {

	if (pb->name == NULL) {
		return;
	}

	assert(strcmp(pb->pb_graph_node->pb_type->name, pb_graph_node->pb_type->name) == 0);

	pb->pb_graph_node = pb_graph_node;
	if (pb->child_pbs != NULL) {
		for (int i = 0; i < pb_graph_node->pb_type->modes[pb->mode].num_pb_type_children; ++i) {
			if (pb->child_pbs[i] != NULL) {
				for (int j = 0; j < pb_graph_node->pb_type->modes[pb->mode].pb_type_children[i].num_pb; ++j) {
					resync_pb_graph_nodes_in_pb(
							&pb_graph_node->child_pb_graph_nodes[pb->mode][i][j],
							&pb->child_pbs[i][j]);
				}
			}
		}
	}
}

/* This function performs power estimation, and must be called
 * after packing, placement AND routing. Currently, this
 * will not work when running a partial flow (ex. only routing).
 */
void vpr_power_estimation(t_vpr_setup vpr_setup, t_arch Arch) {

	/* Ensure we are only using 1 clock */
	assert(count_netlist_clocks() == 1);

	/* Get the critical path of this clock */
	g_solution_inf.T_crit = get_critical_path_delay() / 1e9;
	assert(g_solution_inf.T_crit > 0.);

	vpr_printf_info("\n\nPower Estimation:\n");
	vpr_printf_info("-----------------\n");

	vpr_printf_info("Initializing power module\n");

	/* Initialize the power module */
	boolean power_error = power_init(vpr_setup.FileNameOpts.PowerFile,
			vpr_setup.FileNameOpts.CmosTechFile, &Arch, &vpr_setup.RoutingArch);
	if (power_error) {
		vpr_printf_error(__FILE__, __LINE__,
				"Power initialization failed.\n");
	}

	if (!power_error) {
		float power_runtime_s;

		vpr_printf_info("Running power estimation\n");

		/* Run power estimation */
		e_power_ret_code power_ret_code = power_total(&power_runtime_s, vpr_setup,
				&Arch, &vpr_setup.RoutingArch);

		/* Check for errors/warnings */
		if (power_ret_code == POWER_RET_CODE_ERRORS) {
			vpr_printf_error(__FILE__, __LINE__,
					"Power estimation failed. See power output for error details.\n");
		} else if (power_ret_code == POWER_RET_CODE_WARNINGS) {
			vpr_printf_warning(__FILE__, __LINE__,
					"Power estimation completed with warnings. See power output for more details.\n");
		} else if (power_ret_code == POWER_RET_CODE_SUCCESS) {
		}
		vpr_printf_info("Power estimation took %g seconds\n", power_runtime_s);
	}

	/* Uninitialize power module */
	if (!power_error) {
		vpr_printf_info("Uninitializing power module\n");
		power_error = power_uninit();
		if (power_error) {
			vpr_printf_error(__FILE__, __LINE__,
					"Power uninitialization failed.\n");
		} else {

		}
	}
	vpr_printf_info("\n");
}

void vpr_print_error(t_vpr_error* vpr_error){

	/* Determine the type of VPR error, To-do: can use some enum-to-string mechanism */
	char* error_type = (char *)my_calloc(1000, sizeof(char));
	switch(vpr_error->type){
	case VPR_ERROR_UNKNOWN:
		strcpy(error_type, "Unknown");
		break;
	case VPR_ERROR_ARCH:
		strcpy(error_type, "Architecture file");
		break;
	case VPR_ERROR_PACK:
		strcpy(error_type, "Packing");
		break;
	case VPR_ERROR_PLACE:
		strcpy(error_type, "Placement");
		break;
	case VPR_ERROR_ROUTE:
		strcpy(error_type, "Routing");
		break;
	case VPR_ERROR_TIMING:
		strcpy(error_type, "Timing");
		break;
	case VPR_ERROR_SDC:
		strcpy(error_type, "SDC file");
		break;
	case VPR_ERROR_NET_F:
		strcpy(error_type, "Netlist file");
		break;
	case VPR_ERROR_BLIF_F:
		strcpy(error_type, "Blif file");
		break;
	case VPR_ERROR_PLACE_F:
		strcpy(error_type, "Placement file");
		break;
	case VPR_ERROR_OTHER:
		strcpy(error_type, "Other");
		break;
	default:
		strcpy(error_type, "");
		break;
	}
			
	vpr_printf_error(__FILE__, __LINE__,
		"\nType: %s\nFile: %s\nLine: %d\nMessage: %s\n",
		error_type, vpr_error->file_name, vpr_error->line_num, 
		vpr_error->message);
	
	free (error_type);
}
