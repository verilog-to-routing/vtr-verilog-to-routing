/**
 VPR is a CAD tool used to conduct FPGA architecture exploration.  It takes, as input, a technology-mapped netlist and a description of the FPGA architecture being investigated.  
 VPR then generates a packed, placed, and routed FPGA (in .net, .place, and .route files respectively) that implements the input netlist.
 
 This file is where VPR starts execution.

 Key files in VPR:
 1.  libarchfpga/physical_types.h - Data structures that define the properties of the FPGA architecture
 2.  vpr_types.h - Very major file that defines the core data structures used in VPR.  This includes detailed architecture information, user netlist data structures, and data structures that describe the mapping between those two.
 3.  globals.h - Defines the global variables used by VPR.
 */


#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "util.h"
#include "vpr_types.h"
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
#include "vpr_api.h"


/**
 * VPR program
 * Generate FPGA architecture given architecture description
 * Pack, place, and route circuit into FPGA architecture
 * Electrical timing analysis on results
 *
 * Overall steps
 * 1.  Initialization
 * 2.  Pack
 * 3.  Place-and-route and timing analysis
 */
int main(int argc, char **argv) {
	t_options Options;
	t_arch Arch;
	t_model *user_models, *library_models;

	enum e_operation Operation;
	struct s_file_name_opts FileNameOpts;
	struct s_packer_opts PackerOpts;
	struct s_placer_opts PlacerOpts;
	struct s_annealing_sched AnnealSched;
	struct s_router_opts RouterOpts;
	struct s_det_routing_arch RoutingArch;
	t_segment_inf *Segments;
	t_timing_inf Timing;
	boolean ShowGraphics;
	boolean TimingEnabled;
	boolean echo_enabled;
	int GraphPause;
	clock_t entire_flow_begin,entire_flow_end;
	clock_t begin, end;

	/*#ifdef __WIN32__
	time_t begin, end;
	#endif

	#ifdef __UNIX__
		struct timeval begin, end;
	#endif*/
	
	entire_flow_begin = clock();

	/* Print title message */
	vpr_print_title();

	/* Print usage message if no args */
	if (argc < 3) {
		vpr_print_usage();
		exit(1);
	}

	/* Read in available inputs  */
	ReadOptions(argc, argv, &Options);

	/* Determine whether timing is on or off */
	TimingEnabled = IsTimingEnabled(Options);

	/* Determine whether echo is on or off */
	echo_enabled = IsEchoEnabled(Options);
	SetEchoOption(echo_enabled);

	/* Use inputs to configure VPR */
	memset(&Arch, 0, sizeof(t_arch));
	SetupVPR(&Options, TimingEnabled, &FileNameOpts, &Arch, &Operation,
			&user_models, &library_models, &PackerOpts, &PlacerOpts,
			&AnnealSched, &RouterOpts, &RoutingArch, &Segments, &Timing,
			&ShowGraphics, &GraphPause);

	/* Check inputs are reasonable */
	CheckOptions(Options, TimingEnabled);
	CheckArch(Arch, TimingEnabled);

	/* Verify settings don't conflict or otherwise not make sense */
	CheckSetup(Operation, PlacerOpts, AnnealSched, RouterOpts, RoutingArch,
			Segments, Timing, Arch.Chans);
	fflush(stdout);

	/* Read blif file and sweep unused components */
	read_and_process_blif(PackerOpts.blif_file_name, PackerOpts.sweep_hanging_nets_and_inputs, user_models,	library_models);

	/* Packing stage */
	if (PackerOpts.doPacking) {
		begin = clock();

		/*#ifdef __WIN32__
			begin = time (NULL);
		#endif

		#ifdef __UNIX__
			gettimeofday(&begin, NULL);
		#endif*/

		try_pack(&PackerOpts, &Arch, user_models, library_models, Timing);

		end = clock();
#ifdef CLOCKS_PER_SEC
		printf("Packing took %g seconds\n",
				(float) (end - begin) / CLOCKS_PER_SEC);
#else
		printf("Packing took %g seconds\n", (float)(end - begin) / CLK_PER_SEC);
#endif

		/*#ifdef __WIN32__
			end = time (NULL);
			printf("Packing took %g seconds\n", (float)(end - begin));
		#endif

		#ifdef __UNIX__
			gettimeofday(&end, NULL);
			printf("Packing took %g seconds\n", ((float)(end.tv_sec - begin.tv_sec) + (float)(end.tv_usec -begin.tv_usec)/1000000));
		#endif*/


		if (!PlacerOpts.doPlacement && !RouterOpts.doRouting) {
			/* Free logical blocks and nets */
			if (logical_block != NULL) {
				free_logical_blocks();
				free_logical_nets();
			}
			return 0;
		}
	}
	/* Packing stage complete */
	fflush(stdout);

	/* Read in netlist file for placement and routing */
	if (FileNameOpts.NetFile) {
		read_netlist(FileNameOpts.NetFile, &Arch, &num_blocks, &block,
				&num_nets, &clb_net);
		/* This is done so that all blocks have subblocks and can be treated the same */
		check_netlist();
	}

	/* Output the current settings to console. */
	ShowSetup(Options, Arch, TimingEnabled, Operation, FileNameOpts, PlacerOpts,
			AnnealSched, RouterOpts, RoutingArch, Segments, Timing);

	if (Operation == TIMING_ANALYSIS_ONLY) {
		do_constant_net_delay_timing_analysis(Timing,
				Options.constant_net_delay);
		return 0;
	}

	vpr_init_place_and_route_arch(Arch);
	fflush(stdout);

	/* Startup X graphics */
	set_graphics_state(ShowGraphics, GraphPause, RouterOpts.route_type);
	if (ShowGraphics) {
		init_graphics("VPR:  Versatile Place and Route for FPGAs");
		alloc_draw_structs();
	}

	/* Do placement and routing */
	place_and_route(Operation, PlacerOpts, FileNameOpts.PlaceFile,
			FileNameOpts.NetFile, FileNameOpts.ArchFile, FileNameOpts.RouteFile,
			AnnealSched, RouterOpts, RoutingArch, Segments, Timing, Arch.Chans,
			Arch.models);

	fflush(stdout);

	/* Close down X Display */
	if (ShowGraphics)
		close_graphics();

	/* free data structures */
	vpr_free_options(&Options);

	if(Timing.SDCFile != NULL) {
		free(Timing.SDCFile);
		Timing.SDCFile = NULL;
	}

	vpr_free_circuit();
	vpr_free_arch(&Arch);
	
	entire_flow_end = clock();
	
	#ifdef CLOCKS_PER_SEC
		printf("The entire flow of VPR took %g seconds.\n", (float)(entire_flow_end - entire_flow_begin) / CLOCKS_PER_SEC);
	#else
		printf("The entire flow of VPR took %g seconds.\n", (float)(entire_flow_end - entire_flow_begin) / CLK_PER_SEC);
	#endif
	
	/* Return 0 to single success to scripts */
	return 0;
}




