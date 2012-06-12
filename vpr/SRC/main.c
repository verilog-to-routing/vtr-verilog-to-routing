/**
 VPR is a CAD tool used to conduct FPGA architecture exploration.  It takes, as input, a technology-mapped netlist and a description of the FPGA architecture being investigated.  
 VPR then generates a packed, placed, and routed FPGA (in .net, .place, and .route files respectively) that implements the input netlist.
 
 This file is where VPR starts execution.

 Key files in VPR:
 1.  libvpr/physical_types.h - Data structures that define the properties of the FPGA architecture
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

/******** Global variables ********/
int Fs_seed = -1;

int W_seed = -1;
int binary_search = -1;

float grid_logic_tile_area = 0;
float ipin_mux_trans_size = 0;

/* t-vpack globals begin  */
int num_logical_nets = 0, num_logical_blocks = 0, num_saved_logical_blocks = 0,
		num_saved_logical_nets = 0, num_subckts = 0;
int num_p_inputs = 0, num_p_outputs = 0, num_luts = 0, num_latches = 0;
struct s_net *vpack_net = NULL, *saved_logical_nets = NULL;
struct s_logical_block *logical_block = NULL, *saved_logical_blocks = NULL;
struct s_subckt *subckt = NULL;
char *blif_circuit_name = NULL;
/* t-vpack globals end  */

/******** Netlist to be mapped stuff ********/

int num_nets = 0;
struct s_net *clb_net = NULL;

int num_blocks = 0;
struct s_block *block = NULL;

int num_ff = 0;
int num_const_gen = 0;

int *clb_to_vpack_net_mapping = NULL; /* [0..num_clb_nets - 1] */
int *vpack_to_clb_net_mapping = NULL; /* [0..num_vpack_nets - 1] */

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

int *chan_width_x = NULL; /* [0..ny] */
int *chan_width_y = NULL; /* [0..nx] */

struct s_grid_tile **grid = NULL; /* [0..(nx+1)][0..(ny+1)] Physical block list */

/******** Structures defining the routing ********/

/* Linked list start pointers.  Define the routing. */
struct s_trace **trace_head = NULL; /* [0..(num_nets-1)] */
struct s_trace **trace_tail = NULL; /* [0..(num_nets-1)] */

/******** Structures defining the FPGA routing architecture ********/

int num_rr_nodes = 0;
t_rr_node *rr_node = NULL; /* [0..(num_rr_nodes-1)] */
t_ivec ***rr_node_indices = NULL;

int num_rr_indexed_data = 0;
t_rr_indexed_data *rr_indexed_data = NULL; /* [0..(num_rr_indexed_data-1)] */

/* Gives the rr_node indices of net terminals. */

int **net_rr_terminals = NULL; /* [0..num_nets-1][0..num_pins-1] */

/* Gives information about all the switch types                      *
 * (part of routing architecture, but loaded in read_arch.c          */

struct s_switch_inf *switch_inf = NULL; /* [0..(det_routing_arch.num_switch-1)] */

/* Stores the SOURCE and SINK nodes of all CLBs (not valid for pads).     */

int **rr_blk_source = NULL; /* [0..(num_blocks-1)][0..(num_class-1)] */

/* primiary inputs removed from circuit */
struct s_linked_vptr *circuit_p_io_removed = NULL;

/********** Structures representing timing graph information */
t_tnode *tnode = NULL; /* [0..num_tnodes - 1] */
int num_tnodes = 0; /* Number of nodes (pins) in the timing graph */
float pb_max_internal_delay = UNDEFINED; /* biggest internal delay of physical block */
const t_pb_type *pbtype_max_internal_delay = NULL; /* physical block type with highest internal delay */

/********************** Subroutines local to this module ********************/

static void PrintUsage(void);
static void PrintTitle(void);
static void freeArch(t_arch* Arch);
static void freeOptions(t_options *options);
static void InitArch(INP t_arch Arch);
static void free_pb_type(t_pb_type *pb_type);
static void free_complex_block_types(void);

/************************* Subroutine definitions ***************************/

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
	PrintTitle();

	/* Print usage message if no args */
	if (argc < 3) {
		PrintUsage();
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

	InitArch(Arch);
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
	freeOptions(&Options);

	/* Free logical blocks and nets */
	if (logical_block != NULL) {
		free_logical_blocks();
		free_logical_nets();
	}

	freeArch(&Arch);
	free_complex_block_types();

	entire_flow_end = clock();
	
	#ifdef CLOCKS_PER_SEC
		printf("The entire flow of VPR took %g seconds.\n", (float)(entire_flow_end - entire_flow_begin) / CLOCKS_PER_SEC);
	#else
		printf("The entire flow of VPR took %g seconds.\n", (float)(entire_flow_end - entire_flow_begin) / CLK_PER_SEC);
	#endif
	
	/* Return 0 to single success to scripts */
	return 0;
}

/* Outputs usage message */
static void PrintUsage(void) {
	puts("Usage:  vpr fpga_architecture.xml circuit_name [Options ...]");
	puts("");
	puts("General Options:  [--nodisp] [--auto <int>] [--pack]");
	puts(
			"\t[--place] [--route] [--timing_analyze_only_with_net_delay <float>]");
	puts(
			"\t[--fast] [--full_stats] [--timing_analysis on | off] [--outfile_prefix <string>]");
	puts(
			"\t[--blif_file <string>][--net_file <string>][--place_file <string>]");
	puts("\t[--route_file <string>][--echo_file on | off]");
	puts("");
	puts("Packer Options:");
	/*    puts("\t[-global_clocks on|off]");
	 puts("\t[-hill_climbing on|off]");
	 puts("\t[-sweep_hanging_nets_and_inputs on|off]"); */
	puts("\t[--timing_driven_clustering on|off]");
	puts(
			"\t[--cluster_seed_type timing|max_inputs] [--alpha_clustering <float>] [--beta_clustering <float>]");
	/*    puts("\t[-recompute_timing_after <int>] [-cluster_block_delay <float>]"); */
	puts("\t[--allow_unrelated_clustering on|off]");
	/*    puts("\t[-allow_early_exit on|off]"); 
	 puts("\t[-intra_cluster_net_delay <float>] ");
	 puts("\t[-inter_cluster_net_delay <float>] "); */
	puts("\t[--connection_driven_clustering on|off] ");
	puts("");
	puts("Placer Options:");
	puts(
			"\t[--place_algorithm bounding_box | net_timing_driven | path_timing_driven]");
	puts("\t[--init_t <float>] [--exit_t <float>]");
	puts("\t[--alpha_t <float>] [--inner_num <float>] [--seed <int>]");
	puts("\t[--place_cost_exp <float>] [--place_cost_type linear | nonlinear]");
	puts("\t[--place_chan_width <int>] [--num_regions <int>] ");
	puts("\t[--fix_pins random | <file.pads>]");
	puts("\t[--enable_timing_computations on | off]");
	puts("\t[--block_dist <int>]");
	puts("");
	puts("Placement Options Valid Only for Timing-Driven Placement:");
	puts("\t[--timing_tradeoff <float>]");
	puts("\t[--recompute_crit_iter <int>]");
	puts("\t[--inner_loop_recompute_divider <int>]");
	puts("\t[--td_place_exp_first <float>]");
	puts("\t[--td_place_exp_last <float>]");
	puts("");
	puts("Router Options:  [-max_router_iterations <int>] [-bb_factor <int>]");
	puts("\t[--initial_pres_fac <float>] [--pres_fac_mult <float>]");
	puts("\t[--acc_fac <float>] [--first_iter_pres_fac <float>]");
	puts("\t[--bend_cost <float>] [--route_type global | detailed]");
	puts("\t[--verify_binary_search] [--route_chan_width <int>]");
	puts(
			"\t[--router_algorithm breadth_first | timing_driven | directed_search]");
	puts(
			"\t[--base_cost_type intrinsic_delay | delay_normalized | demand_only]");
	puts("");
	puts("Routing options valid only for timing-driven routing:");
	puts("\t[--astar_fac <float>] [--max_criticality <float>]");
	puts("\t[--criticality_exp <float>]");
	puts("");
}

static void PrintTitle(void) {
	puts("");
	puts("VPR FPGA Placement and Routing.");
	puts("Version: Version " VPR_VERSION);
	puts("Compiled: " __DATE__ ".");
	puts("Original VPR by V. Betz.");
	puts("Timing-driven placement enhancements by A. Marquardt.");
	puts("Single-drivers enhancements by Andy Ye with additions by.");
	puts("Mark Fang, Jason Luu, Ted Campbell");
	puts("Heterogeneous stucture support by Jason Luu and Ted Campbell.");
	puts("T-VPack clustering integration by Jason Luu.");
	puts("Area-driven AAPack added by Jason Luu.");
	puts("This is free open source code under MIT license.");
	puts("");
}

static void freeArch(t_arch* Arch) {
	int i;
	t_model *model, *prev;
	t_model_ports *port, *prev_port;
	struct s_linked_vptr *vptr, *vptr_prev;
	for (i = 0; i < Arch->num_switches; i++) {
		if (Arch->Switches->name != NULL) {
			free(Arch->Switches[i].name);
		}
	}
	free(Arch->Switches);
	for (i = 0; i < Arch->num_segments; i++) {
		if (Arch->Segments->cb != NULL) {
			free(Arch->Segments[i].cb);
		}
		if (Arch->Segments->sb != NULL) {
			free(Arch->Segments[i].sb);
		}
	}
	free(Arch->Segments);
	model = Arch->models;
	while (model) {
		port = model->inputs;
		while (port) {
			prev_port = port;
			port = port->next;
			free(prev_port->name);
			free(prev_port);
		}
		port = model->outputs;
		while (port) {
			prev_port = port;
			port = port->next;
			free(prev_port->name);
			free(prev_port);
		}
		vptr = model->pb_types;
		while (vptr) {
			vptr_prev = vptr;
			vptr = vptr->next;
			free(vptr_prev);
		}
		prev = model;

		model = model->next;
		if (prev->instances)
			free(prev->instances);
		free(prev->name);
		free(prev);
	}

	for (i = 0; i < 4; i++) {
		vptr = Arch->model_library[i].pb_types;
		while (vptr) {
			vptr_prev = vptr;
			vptr = vptr->next;
			free(vptr_prev);
		}
	}

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
}

static void free_complex_block_types(void) {
	int i, j, k, m;

	free_all_pb_graph_nodes();

	for (i = 0; i < num_types; i++) {
		if (&type_descriptors[i] == EMPTY_TYPE) {
			continue;
		}
		free(type_descriptors[i].name);
		for (j = 0; j < type_descriptors[i].height; j++) {
			for (k = 0; k < 4; k++) {
				for (m = 0;
						m < type_descriptors[i].num_pin_loc_assignments[j][k];
						m++) {
					if (type_descriptors[i].pin_loc_assignments[j][k][m])
						free(type_descriptors[i].pin_loc_assignments[j][k][m]);
				}
				free(type_descriptors[i].pinloc[j][k]);
				free(type_descriptors[i].pin_loc_assignments[j][k]);
			}
			free(type_descriptors[i].pinloc[j]);
			free(type_descriptors[i].pin_loc_assignments[j]);
			free(type_descriptors[i].num_pin_loc_assignments[j]);
		}
		free(type_descriptors[i].pinloc);
		free(type_descriptors[i].pin_loc_assignments);
		free(type_descriptors[i].num_pin_loc_assignments);
		free(type_descriptors[i].pin_height);

		free_pb_type(type_descriptors[i].pb_type);
		free(type_descriptors[i].pb_type);
	}
}

static void free_pb_type(t_pb_type *pb_type) {

	int i, j, k, m;

	free(pb_type->name);
	if (pb_type->blif_model)
		free(pb_type->blif_model);

	for (i = 0; i < pb_type->num_modes; i++) {
		for (j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
			free_pb_type(&pb_type->modes[i].pb_type_children[j]);
		}
		free(pb_type->modes[i].pb_type_children);
		free(pb_type->modes[i].name);
		for (j = 0; j < pb_type->modes[i].num_interconnect; j++) {
			free(pb_type->modes[i].interconnect[j].input_string);
			free(pb_type->modes[i].interconnect[j].output_string);
			free(pb_type->modes[i].interconnect[j].name);

			for (k = 0; k < pb_type->modes[i].interconnect[j].num_annotations;
					k++) {
				if (pb_type->modes[i].interconnect[j].annotations[k].clock)
					free(
							pb_type->modes[i].interconnect[j].annotations[k].clock);
				if (pb_type->modes[i].interconnect[j].annotations[k].input_pins) {
					free(
							pb_type->modes[i].interconnect[j].annotations[k].input_pins);
				}
				if (pb_type->modes[i].interconnect[j].annotations[k].output_pins) {
					free(
							pb_type->modes[i].interconnect[j].annotations[k].output_pins);
				}
				for (m = 0;
						m
								< pb_type->modes[i].interconnect[j].annotations[k].num_value_prop_pairs;
						m++) {
					free(
							pb_type->modes[i].interconnect[j].annotations[k].value[m]);
				}
				free(pb_type->modes[i].interconnect[j].annotations[k].prop);
				free(pb_type->modes[i].interconnect[j].annotations[k].value);
			}
			free(pb_type->modes[i].interconnect[j].annotations);
		}
		if (pb_type->modes[i].interconnect)
			free(pb_type->modes[i].interconnect);
	}
	if (pb_type->modes)
		free(pb_type->modes);

	for (i = 0; i < pb_type->num_annotations; i++) {
		for (j = 0; j < pb_type->annotations[i].num_value_prop_pairs; j++) {
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

	for (i = 0; i < pb_type->num_ports; i++) {
		free(pb_type->ports[i].name);
		if (pb_type->ports[i].port_class) {
			free(pb_type->ports[i].port_class);
		}
	}
	free(pb_type->ports);
}

/* This is a modification of the init_arch function to use Arch as param.
 * Sets globals: nx, ny
 * Allocs globals: chan_width_x, chan_width_y, grid
 * Depends on num_clbs, pins_per_clb */
static void InitArch(INP t_arch Arch) {
	int *num_instances_type, *num_blocks_type;
	int i;
	int current, high, low;
	boolean fit;

	current = nint(sqrt(num_blocks)); /* current is the value of the smaller side of the FPGA */
	low = 1;
	high = -1;

	num_instances_type = (int*) my_calloc(num_types, sizeof(int));
	num_blocks_type = (int*) my_calloc(num_types, sizeof(int));

	for (i = 0; i < num_blocks; i++) {
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
			printf("Auto-sizing FPGA, try x = %d y = %d\n", nx, ny);
#endif
			alloc_and_load_grid(num_instances_type);
			freeGrid();

			/* Test if netlist fits in grid */
			fit = TRUE;
			for (i = 0; i < num_types; i++) {
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
						printf(
								ERRTAG
								"FPGA required is too large for current architecture settings\n");
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
		alloc_and_load_grid(num_instances_type);
		printf("FPGA auto-sized to, x = %d y = %d\n", nx, ny);
	} else {
		nx = Arch.clb_grid.W;
		ny = Arch.clb_grid.H;
		alloc_and_load_grid(num_instances_type);
	}

	printf("The circuit will be mapped into a %d x %d array of clbs.\n", nx,
			ny);

	/* Test if netlist fits in grid */
	fit = TRUE;
	for (i = 0; i < num_types; i++) {
		if (num_blocks_type[i] > num_instances_type[i]) {
			fit = FALSE;
			break;
		}
	}
	if (!fit) {
		printf(ERRTAG "Not enough physical locations for type %s, "
		"number of blocks is %d but number of locations is %d\n",
				type_descriptors[i].name, num_blocks_type[i],
				num_instances_type[i]);
		exit(1);
	}

	printf("\nResource Usage:\n");
	for (i = 0; i < num_types; i++) {
		printf("Netlist      %d\tblocks of type %s\n", num_blocks_type[i],
				type_descriptors[i].name);
		printf("Architecture %d\tblocks of type %s\n", num_instances_type[i],
				type_descriptors[i].name);
	}
	printf("\n");
	chan_width_x = (int *) my_malloc((ny + 1) * sizeof(int));
	chan_width_y = (int *) my_malloc((nx + 1) * sizeof(int));

	free(num_blocks_type);
	free(num_instances_type);
}

static void freeOptions(t_options *options) {
	free(options->ArchFile);
	free(options->CircuitName);
	if (options->BlifFile)
		free(options->BlifFile);
	if (options->NetFile)
		free(options->NetFile);
	if (options->PlaceFile)
		free(options->PlaceFile);
	if (options->RouteFile)
		free(options->RouteFile);
	if (options->OutFilePrefix)
		free(options->OutFilePrefix);
	if (options->PinFile)
		free(options->PinFile);
}
