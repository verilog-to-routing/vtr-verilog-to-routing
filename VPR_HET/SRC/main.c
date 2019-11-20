#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "graphics.h"
#include "read_netlist.h"
#include "print_netlist.h"
#include "draw.h"
#include "place_and_route.h"
#include "stats.h"
#include "path_delay.h"
#include "OptionTokens.h"
#include "ReadOptions.h"
#include "xml_arch.h"
#include "SetupVPR.h"
#include "rr_graph.h"

/******** Global variables ********/
int Fs_seed = -1;
boolean WMF_DEBUG = FALSE;

int W_seed = -1;
int binary_search = -1;
char *OutFilePrefix = NULL;

float grid_logic_tile_area = 0;
float ipin_mux_trans_size = 0;

/******** Netlist to be mapped stuff ********/

int num_nets = 0;
struct s_net *net = NULL;

int num_blocks = 0;
struct s_block *block = NULL;


/* This identifies the t_type_ptr of an IO block */
int num_types = 0;
struct s_type_descriptor *type_descriptors = NULL;

t_type_ptr IO_TYPE = NULL;
t_type_ptr EMPTY_TYPE = NULL;
t_type_ptr FILL_TYPE = NULL;


/******** Physical architecture stuff ********/

int nx = 0;
int ny = 0;

/* TRUE if this is a global clb pin -- an input pin to which the netlist can *
 * connect global signals, but which does not connect into the normal        *
 * routing via muxes etc.  Marking pins like this (only clocks in my work)   *
 * stops them from screwing up the input switch pattern in the rr_graph      *
 * generator and from creating extra switches that the area model would      *
 * count.                                                                    */

int *chan_width_x = NULL;	/* [0..ny] */
int *chan_width_y = NULL;	/* [0..nx] */

struct s_grid_tile **grid = NULL;	/* [0..(nx+1)][0..(ny+1)] Physical block list */


/******** Structures defining the routing ********/

/* Linked list start pointers.  Define the routing. */
struct s_trace **trace_head = NULL;	/* [0..(num_nets-1)] */
struct s_trace **trace_tail = NULL;	/* [0..(num_nets-1)] */


/******** Structures defining the FPGA routing architecture ********/

int num_rr_nodes = 0;
t_rr_node *rr_node = NULL;	/* [0..(num_rr_nodes-1)] */
t_ivec ***rr_node_indices = NULL;

int num_rr_indexed_data = 0;
t_rr_indexed_data *rr_indexed_data = NULL;	/* [0..(num_rr_indexed_data-1)] */

/* Gives the rr_node indices of net terminals. */

int **net_rr_terminals = NULL;	/* [0..num_nets-1][0..num_pins-1] */

/* Gives information about all the switch types                      *
 * (part of routing architecture, but loaded in read_arch.c          */

struct s_switch_inf *switch_inf = NULL;	/* [0..(det_routing_arch.num_switch-1)] */

/* Stores the SOURCE and SINK nodes of all CLBs (not valid for pads).     */

int **rr_blk_source = NULL;	/* [0..(num_blocks-1)][0..(num_class-1)] */


/********************** Subroutines local to this module ********************/

static void PrintUsage();
static void PrintTitle();
static void freeArch(t_arch* Arch);



/************************* Subroutine definitions ***************************/

int
main(int argc,
     char **argv)
{
    t_options Options;
    t_arch Arch = { 0 };

	enum e_operation Operation;
    struct s_placer_opts PlacerOpts;
    struct s_annealing_sched AnnealSched;
    struct s_router_opts RouterOpts;
    struct s_det_routing_arch RoutingArch;
    t_segment_inf *Segments;
    t_timing_inf Timing;
    t_subblock_data Subblocks;
    boolean ShowGraphics;
    boolean TimingEnabled;
    int GraphPause;


    /* Print title message */
    PrintTitle();

    /* Print usage message if no args */
    if(argc < 2)
	{
	    PrintUsage();
	    exit(1);
	}

    /* Read in available inputs  */
    ReadOptions(argc, argv, &Options);

    /* Determine whether timing is on or off */
    TimingEnabled = IsTimingEnabled(Options);

    /* Use inputs to configure VPR */
    SetupVPR(Options, TimingEnabled, &Arch, &Operation, &PlacerOpts,
	     &AnnealSched, &RouterOpts, &RoutingArch, &Segments,
	     &Timing, &Subblocks, &ShowGraphics, &GraphPause);

    /* Check inputs are reasonable */
    CheckOptions(Options, TimingEnabled);
    CheckArch(Arch, TimingEnabled);

    /* Verify settings don't conflict or otherwise not make sense */
    CheckSetup(Operation, PlacerOpts, AnnealSched, RouterOpts,
	       RoutingArch, Segments, Timing, Subblocks, Arch.Chans);

    /* Output the current settings to console. */
    ShowSetup(Options, Arch, TimingEnabled, Operation, PlacerOpts,
	      AnnealSched, RouterOpts, RoutingArch, Segments, Timing,
	      Subblocks);

	if(Operation == TIMING_ANALYSIS_ONLY) {
		do_constant_net_delay_timing_analysis(
			Timing, Subblocks, Options.constant_net_delay);
		return 0;
	}

    /* Startup X graphics */
    set_graphics_state(ShowGraphics, GraphPause, RouterOpts.route_type);
    if(ShowGraphics)
	{
	    init_graphics("VPR:  Versatile Place and Route for FPGAs");
	    alloc_draw_structs();
	}

    /* Do the actual operation */
    place_and_route(Operation, PlacerOpts, Options.PlaceFile,
		    Options.NetFile, Options.ArchFile, Options.RouteFile,
		    AnnealSched, RouterOpts, RoutingArch,
		    Segments, Timing, &Subblocks, Arch.Chans);

    /* Close down X Display */
    if(ShowGraphics)
	close_graphics();

	/* free data structures */
	free(Options.PlaceFile);
	free(Options.NetFile);
	free(Options.ArchFile);
	free(Options.RouteFile);

	freeArch(&Arch);
	
    /* Return 0 to single success to scripts */
    return 0;
}



/* Outputs usage message */
static void
PrintUsage()
{
    puts("Usage:  vpr circuit.net fpga.arch placed.out routed.out [Options ...]");
    puts("");
    puts("General Options:  [-nodisp] [-auto <int>] [-route_only]");
    puts("\t[-place_only] [-timing_analyze_only_with_net_delay <float>]");
    puts("\t[-fast] [-full_stats] [-timing_analysis on | off] [-outfile_prefix <string>]");
    puts("");
    puts("Placer Options:");
    puts("\t[-place_algorithm bounding_box | net_timing_driven | path_timing_driven]");
    puts("\t[-init_t <float>] [-exit_t <float>]");
    puts("\t[-alpha_t <float>] [-inner_num <float>] [-seed <int>]");
    puts("\t[-place_cost_exp <float>] [-place_cost_type linear | nonlinear]");
    puts("\t[-place_chan_width <int>] [-num_regions <int>] ");
    puts("\t[-fix_pins random | <file.pads>]");
    puts("\t[-enable_timing_computations on | off]");
    puts("\t[-block_dist <int>]");
    puts("");
    puts("Placement Options Valid Only for Timing-Driven Placement:");
    puts("\t[-timing_tradeoff <float>]");
    puts("\t[-recompute_crit_iter <int>]");
    puts("\t[-inner_loop_recompute_divider <int>]");
    puts("\t[-td_place_exp_first <float>]");
    puts("\t[-td_place_exp_last <float>]");
    puts("");
    puts("Router Options:  [-max_router_iterations <int>] [-bb_factor <int>]");
    puts("\t[-initial_pres_fac <float>] [-pres_fac_mult <float>]");
    puts("\t[-acc_fac <float>] [-first_iter_pres_fac <float>]");
    puts("\t[-bend_cost <float>] [-route_type global | detailed]");
    puts("\t[-verify_binary_search] [-route_chan_width <int>]");
    puts("\t[-router_algorithm breadth_first | timing_driven]");
    puts("\t[-base_cost_type intrinsic_delay | delay_normalized | demand_only]");
    puts("");
    puts("Routing options valid only for timing-driven routing:");
    puts("\t[-astar_fac <float>] [-max_criticality <float>]");
    puts("\t[-criticality_exp <float>]");
    puts("");
}



static void
PrintTitle()
{
    puts("");
    puts("VPR FPGA Placement and Routing.");
    puts("Version: Version 5.0.2");
    puts("Compiled: " __DATE__ ".");
    puts("Original VPR by V. Betz.");
    puts("Timing-driven placement enhancements by A. Marquardt.");
	puts("Single-drivers enhancements by Andy Ye with additions by.");
	puts("Mark Fang, Jason Luu, Ted Campbell");
	puts("Heterogeneous stucture support by Jason Luu and Ted Campbell.");
    puts("This code is licensed only for non-commercial use.");
    puts("");
}

static void freeArch(t_arch* Arch)
{
	int i;
	for(i = 0; i < Arch->num_switches; i++) {
		if(Arch->Switches->name != NULL) {
			free(Arch->Switches[i].name);
		}
	}
	free(Arch->Switches);
	for(i = 0; i < Arch->num_segments; i++) {
		if(Arch->Segments->cb != NULL) {
			free(Arch->Segments[i].cb);
		}
		if(Arch->Segments->sb != NULL) {
			free(Arch->Segments[i].sb);
		}
	}
	free(Arch->Segments);
}
