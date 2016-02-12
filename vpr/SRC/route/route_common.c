#include <cstdio>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <vector>
#include <iostream>
using namespace std;

#include <assert.h>

#include "stats.h"
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "vpr_utils.h"
#include "route_export.h"
#include "route_common.h"
#include "route_tree_timing.h"
#include "route_timing.h"
#include "route_breadth_first.h"
#include "place_and_route.h"
#include "rr_graph.h"
#include "read_xml_arch_file.h"
#include "ReadOptions.h"

#include "route_profiling.h"


// Disable the routing predictor for circuits with less that this number of nets.
// This was experimentally determined, by Matthew Walker, to be the most useful
// metric, and this is also somewhat larger than largest circuit observed to have
// inaccurate predictions, thought this is still only affects quite small circuits.
#define MIN_NETS_TO_ACTIVATE_PREDICTOR 150

/***************** Variables shared only by route modules *******************/

t_rr_node_route_inf *rr_node_route_inf = NULL; /* [0..num_rr_nodes-1] */

struct s_bb *route_bb = NULL; /* [0..g_clbs_nlist.net.size()-1]. Limits area in which each  */

/* net must be routed.                         */

/**************** Static variables local to route_common.c ******************/

static struct s_heap **heap; /* Indexed from [1..heap_size] */
static int heap_size; /* Number of slots in the heap array */
static int heap_tail; /* Index of first unused slot in the heap array */

/* For managing my own list of currently free heap data structures.     */
static struct s_heap *heap_free_head = NULL;
/* For keeping track of the sudo malloc memory for the heap*/
static t_chunk heap_ch = {NULL, 0, NULL};

/* For managing my own list of currently free trace data structures.    */
static struct s_trace *trace_free_head = NULL;
/* For keeping track of the sudo malloc memory for the trace*/
static t_chunk trace_ch = {NULL, 0, NULL};

#ifdef DEBUG
static int num_trace_allocated = 0; /* To watch for memory leaks. */
static int num_heap_allocated = 0;
static int num_linked_f_pointer_allocated = 0;
#endif

static struct s_linked_f_pointer *rr_modified_head = NULL;
static struct s_linked_f_pointer *linked_f_pointer_free_head = NULL;

static t_chunk linked_f_pointer_ch = {NULL, 0, NULL};

/*  The numbering relation between the channels and clbs is:				*
 *																	        *
 *  |    IO     | chan_   |   CLB     | chan_   |   CLB     |               *
 *  |grid[0][2] | y[0][2] |grid[1][2] | y[1][2] | grid[2][2]|               *
 *  +-----------+         +-----------+         +-----------+               *
 *                                                            } capacity in *
 *   No channel           chan_x[1][1]          chan_x[2][1]  } chan_width  *
 *                                                            } _x[1]       *
 *  +-----------+         +-----------+         +-----------+               *
 *  |           |  chan_  |           |  chan_  |           |               *
 *  |    IO     | y[0][1] |   CLB     | y[1][1] |   CLB     |               *
 *  |grid[0][1] |         |grid[1][1] |         |grid[2][1] |               *
 *  |           |         |           |         |           |               *
 *  +-----------+         +-----------+         +-----------+               *
 *                                                            } capacity in *
 *                        chan_x[1][0]          chan_x[2][0]  } chan_width  * 
 *                                                            } _x[0]       *
 *                        +-----------+         +-----------+               *
 *                 No     |           |	   No   |           |               *
 *               Channel  |    IO     | Channel |    IO     |               *
 *                        |grid[1][0] |         |grid[2][0] |               *
 *                        |           |         |           |               *
 *                        +-----------+         +-----------+               *
 *                                                                          *
 *               {=======}              {=======}                           *
 *              Capacity in            Capacity in                          *
 *            chan_width_y[0]        chan_width_y[1]                        *
 *                                                                          */

/******************** Subroutines local to route_common.c *******************/

static void load_route_bb(int bb_factor);

static void add_to_heap(struct s_heap *hptr);
static struct s_heap *alloc_heap_data(void);
static struct s_linked_f_pointer *alloc_linked_f_pointer(void);

static t_ivec **alloc_and_load_clb_opins_used_locally(void);
static void adjust_one_rr_occ_and_apcost(int inode, int add_or_sub,
		float pres_fac, float acc_fac);

/************************** Subroutine definitions ***************************/

void save_routing(struct s_trace **best_routing,
		t_ivec ** clb_opins_used_locally,
		t_ivec ** saved_clb_opins_used_locally) {

	/* This routing frees any routing currently held in best routing,       *
	 * then copies over the current routing (held in trace_head), and       *
	 * finally sets trace_head and trace_tail to all NULLs so that the      *
	 * connection to the saved routing is broken.  This is necessary so     *
	 * that the next iteration of the router does not free the saved        *
	 * routing elements.  Also saves any data about locally used clb_opins, *
	 * since this is also part of the routing.                              */

	unsigned int inet;
	int iblk, iclass, ipin, num_local_opins;
	struct s_trace *tptr, *tempptr;
	t_type_ptr type;

	for (inet = 0; inet < g_clbs_nlist.net.size(); inet++) {

		/* Free any previously saved routing.  It is no longer best. */
		tptr = best_routing[inet];
		while (tptr != NULL) {
			tempptr = tptr->next;
			free_trace_data(tptr);
			tptr = tempptr;
		}

		/* Save a pointer to the current routing in best_routing. */
		best_routing[inet] = trace_head[inet];

		/* Set the current (working) routing to NULL so the current trace       *
		 * elements won't be reused by the memory allocator.                    */

		trace_head[inet] = NULL;
		trace_tail[inet] = NULL;
	}

	/* Save which OPINs are locally used.                           */

	for (iblk = 0; iblk < num_blocks; iblk++) {
		type = block[iblk].type;
		for (iclass = 0; iclass < type->num_class; iclass++) {
			num_local_opins = clb_opins_used_locally[iblk][iclass].nelem;
			for (ipin = 0; ipin < num_local_opins; ipin++) {
				saved_clb_opins_used_locally[iblk][iclass].list[ipin] =
						clb_opins_used_locally[iblk][iclass].list[ipin];
			}
		}
	}
}

void restore_routing(struct s_trace **best_routing,
		t_ivec ** clb_opins_used_locally,
		t_ivec ** saved_clb_opins_used_locally) {

	/* Deallocates any current routing in trace_head, and replaces it with    *
	 * the routing in best_routing.  Best_routing is set to NULL to show that *
	 * it no longer points to a valid routing.  NOTE:  trace_tail is not      *
	 * restored -- it is set to all NULLs since it is only used in            *
	 * update_traceback.  If you need trace_tail restored, modify this        *
	 * routine.  Also restores the locally used opin data.                    */

	unsigned int inet;
	int iblk, ipin, iclass, num_local_opins;
	t_type_ptr type;

	for (inet = 0; inet < g_clbs_nlist.net.size(); inet++) {

		/* Free any current routing. */
		free_traceback(inet);

		/* Set the current routing to the saved one. */
		trace_head[inet] = best_routing[inet];
		best_routing[inet] = NULL; /* No stored routing. */
	}

	/* Save which OPINs are locally used.                           */

	for (iblk = 0; iblk < num_blocks; iblk++) {
		type = block[iblk].type;
		for (iclass = 0; iclass < type->num_class; iclass++) {
			num_local_opins = clb_opins_used_locally[iblk][iclass].nelem;
			for (ipin = 0; ipin < num_local_opins; ipin++) {
				clb_opins_used_locally[iblk][iclass].list[ipin] =
						saved_clb_opins_used_locally[iblk][iclass].list[ipin];
			}
		}

	}
}

void get_serial_num(void) {

	/* This routine finds a "magic cookie" for the routing and prints it.    *
	 * Use this number as a routing serial number to ensure that programming *
	 * changes do not break the router.                                      */

	unsigned int inet;
	int serial_num, inode;
	struct s_trace *tptr;

	serial_num = 0;

	for (inet = 0; inet < g_clbs_nlist.net.size(); inet++) {

		/* Global nets will have null trace_heads (never routed) so they *
		 * are not included in the serial number calculation.            */

		tptr = trace_head[inet];
		while (tptr != NULL) {
			inode = tptr->index;
			serial_num += (inet + 1)
					* (rr_node[inode].get_xlow() * (nx + 1) - rr_node[inode].get_yhigh());

			serial_num -= rr_node[inode].get_ptc_num() * (inet + 1) * 10;

			serial_num -= rr_node[inode].type * (inet + 1) * 100;
			serial_num %= 2000000000; /* Prevent overflow */
			tptr = tptr->next;
		}
	}
	vpr_printf_info("Serial number (magic cookie) for the routing is: %d\n", serial_num);
}

void try_graph(int width_fac, struct s_router_opts router_opts,
		struct s_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
		t_timing_inf timing_inf, t_chan_width_dist chan_width_dist,
		t_direct_inf *directs, int num_directs) {

	t_graph_type graph_type;
	if (router_opts.route_type == GLOBAL) {
		graph_type = GRAPH_GLOBAL;
	} else {
		graph_type = (det_routing_arch->directionality == BI_DIRECTIONAL ?
						GRAPH_BIDIR : GRAPH_UNIDIR);
	}

	/* Set the channel widths */
	init_chan(width_fac, &router_opts.fixed_channel_width, chan_width_dist);

	/* Free any old routing graph, if one exists. */
	free_rr_graph();

	clock_t begin = clock();

	/* Set up the routing resource graph defined by this FPGA architecture. */
	int warning_count;
	build_rr_graph(graph_type, num_types, type_descriptors, nx, ny, grid,
			&chan_width, NULL, det_routing_arch->switch_block_type,
			det_routing_arch->Fs, det_routing_arch->switchblocks,
			det_routing_arch->num_segment,
			g_num_arch_switches, segment_inf,
			det_routing_arch->global_route_switch,
			det_routing_arch->delayless_switch, timing_inf,
			det_routing_arch->wire_to_arch_ipin_switch,
			router_opts.base_cost_type, 
			router_opts.trim_empty_channels,
			router_opts.trim_obs_channels,
			directs, num_directs, false,
			det_routing_arch->dump_rr_structs_file,
			&det_routing_arch->wire_to_rr_ipin_switch,
			&g_num_rr_switches,
			&warning_count);

	clock_t end = clock();

	vpr_printf_info("Build rr_graph took %g seconds.\n", (float)(end - begin) / CLOCKS_PER_SEC);
}

bool try_route(int width_fac, struct s_router_opts router_opts,
		struct s_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
		t_timing_inf timing_inf, float **net_delay, t_slack * slacks,
		t_chan_width_dist chan_width_dist, t_ivec ** clb_opins_used_locally,
		bool * Fc_clipped, t_direct_inf *directs, int num_directs) {

	/* Attempts a routing via an iterated maze router algorithm.  Width_fac *
	 * specifies the relative width of the channels, while the members of   *
	 * router_opts determine the value of the costs assigned to routing     *
	 * resource node, etc.  det_routing_arch describes the detailed routing *
	 * architecture (connection and switch boxes) of the FPGA; it is used   *
	 * only if a DETAILED routing has been selected.                        */

	t_graph_type graph_type;
	if (router_opts.route_type == GLOBAL) {
		graph_type = GRAPH_GLOBAL;
	} else {
		graph_type = (det_routing_arch->directionality == BI_DIRECTIONAL ?
						GRAPH_BIDIR : GRAPH_UNIDIR);
	}

	/* Set the channel widths */
	init_chan(width_fac, &router_opts.fixed_channel_width, chan_width_dist);

	/* Free any old routing graph, if one exists. */
	free_rr_graph();

	clock_t begin = clock();

	/* Set up the routing resource graph defined by this FPGA architecture. */
	int warning_count;
	build_rr_graph(graph_type, num_types, type_descriptors, nx, ny, grid,
			&chan_width, NULL, det_routing_arch->switch_block_type,
			det_routing_arch->Fs, det_routing_arch->switchblocks,
			det_routing_arch->num_segment,
			g_num_arch_switches, segment_inf,
			det_routing_arch->global_route_switch,
			det_routing_arch->delayless_switch, timing_inf,
			det_routing_arch->wire_to_arch_ipin_switch, 
			router_opts.base_cost_type, 
			router_opts.trim_empty_channels,
			router_opts.trim_obs_channels,
			directs, num_directs, false,
			det_routing_arch->dump_rr_structs_file,
			&det_routing_arch->wire_to_rr_ipin_switch,
			&g_num_rr_switches,
			&warning_count);

	clock_t end = clock();

	vpr_printf_info("Build rr_graph took %g seconds.\n", (float)(end - begin) / CLOCKS_PER_SEC);

	bool success = true;

	/* Allocate and load additional rr_graph information needed only by the router. */
	alloc_and_load_rr_node_route_structs();

	init_route_structs(router_opts.bb_factor);

	if (router_opts.router_algorithm == BREADTH_FIRST) {
		vpr_printf_info("Confirming router algorithm: BREADTH_FIRST.\n");
		success = try_breadth_first_route(router_opts, clb_opins_used_locally,
				width_fac);
	} else { /* TIMING_DRIVEN route */
		vpr_printf_info("Confirming router algorithm: TIMING_DRIVEN.\n");
		assert(router_opts.route_type != GLOBAL);
		success = try_timing_driven_route(router_opts, net_delay, slacks,
			clb_opins_used_locally,timing_inf.timing_analysis_enabled, timing_inf);
		profiling::time_on_fanout_analysis();
		profiling::report_total_route_time();
	}

	free_rr_node_route_structs();
	return (success);
}

bool feasible_routing(void) {

	/* This routine checks to see if this is a resource-feasible routing.      *
	 * That is, are all rr_node capacity limitations respected?  It assumes    *
	 * that the occupancy arrays are up to date when it is called.             */

	int inode;

	for (inode = 0; inode < num_rr_nodes; inode++) {
		if (rr_node[inode].get_occ() > rr_node[inode].get_capacity()) {
			return (false);
		}
	}

	return (true);
}

int predict_success_route_iter(const std::vector<double>& historical_overuse_ratio, const t_router_opts& router_opts) {

	// invalid condition for prediction
	if (router_opts.routing_failure_predictor == OFF || num_nets > MIN_NETS_TO_ACTIVATE_PREDICTOR) return 0;

	// use the last 5 iterations in prediction
	size_t itry {historical_overuse_ratio.size() - 1};
	double congestion_slope {linear_regression_vector(historical_overuse_ratio, itry - 5)};

	// linear extrapolation
	int expected_success_route_iter = (itry*congestion_slope - historical_overuse_ratio.back()) / congestion_slope;

	if (expected_success_route_iter > 1.25 * router_opts.max_router_iterations) {
		vpr_printf_info("Routing aborted, the predicted iteration for a successful route (%d) is too high.\n", expected_success_route_iter);
		return UNDEFINED;
	}
	return expected_success_route_iter;
}

void pathfinder_update_path_cost(struct s_trace *route_segment_start,
		int add_or_sub, float pres_fac) {

	/* This routine updates the occupancy and pres_cost of the rr_nodes that are *
	 * affected by the portion of the routing of one net that starts at          *
	 * route_segment_start.  If route_segment_start is trace_head[inet], the     *
	 * cost of all the nodes in the routing of net inet are updated.  If         *
	 * add_or_sub is -1 the net (or net portion) is ripped up, if it is 1 the    *
	 * net is added to the routing.  The size of pres_fac determines how severly *
	 * oversubscribed rr_nodes are penalized.                                    */

	struct s_trace *tptr;

	tptr = route_segment_start;
	if (tptr == NULL) /* No routing yet. */
		return;

	for (;;) {
		pathfinder_update_single_node_cost(tptr->index, add_or_sub, pres_fac);

		if (rr_node[tptr->index].type == SINK) {
			tptr = tptr->next; /* Skip next segment. */
			if (tptr == NULL)
				break;
		}

		tptr = tptr->next;

	} /* End while loop -- did an entire traceback. */
}

void pathfinder_update_single_node_cost(int inode, int add_or_sub, float pres_fac) {

	/* Updates pathfinder's congestion cost by either adding or removing the 
	 * usage of a resource node. pres_cost is Pn in the Pathfinder paper.
	 * pres_cost is set according to the overuse that would result from having
	 * ONE MORE net use this routing node.     */

	int occ = rr_node[inode].get_occ() + add_or_sub;
	rr_node[inode].set_occ(occ);
	// can't have negative occupancy
	assert(occ >= 0);

	int	capacity = rr_node[inode].get_capacity();
	if (occ < capacity) {
		rr_node_route_inf[inode].pres_cost = 1.0;
	} else {
		rr_node_route_inf[inode].pres_cost = 1.0
				+ (occ + 1 - capacity) * pres_fac;
	}
}
void pathfinder_update_cost(float pres_fac, float acc_fac) {

	/* This routine recomputes the pres_cost and acc_cost of each routing        *
	 * resource for the pathfinder algorithm after all nets have been routed.    *
	 * It updates the accumulated cost to by adding in the number of extra       *
	 * signals sharing a resource right now (i.e. after each complete iteration) *
	 * times acc_fac.  It also updates pres_cost, since pres_fac may have        *
	 * changed.  THIS ROUTINE ASSUMES THE OCCUPANCY VALUES IN RR_NODE ARE UP TO  *
	 * DATE.                                                                     */

	int inode, occ, capacity;

	for (inode = 0; inode < num_rr_nodes; inode++) {
		occ = rr_node[inode].get_occ();
		capacity = rr_node[inode].get_capacity();

		if (occ > capacity) {
			rr_node_route_inf[inode].acc_cost += (occ - capacity) * acc_fac;
			rr_node_route_inf[inode].pres_cost = 1.0
					+ (occ + 1 - capacity) * pres_fac;
		}

		/* If occ == capacity, we don't need to increase acc_cost, but a change    *
		 * in pres_fac could have made it necessary to recompute the cost anyway.  */

		else if (occ == capacity) {
			rr_node_route_inf[inode].pres_cost = 1.0 + pres_fac;
		}
	}
}

void init_route_structs(int bb_factor) {

	/* Call this before you route any nets.  It frees any old traceback and   *
	 * sets the list of rr_nodes touched to empty.                            */

	unsigned int i;

	for (i = 0; i < g_clbs_nlist.net.size(); i++)
		free_traceback(i);

	load_route_bb(bb_factor);

	/* Check that things that should have been emptied after the last routing *
	 * really were.                                                           */

	if (rr_modified_head != NULL) {
		vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
			"in init_route_structs. List of modified rr nodes is not empty.\n");
	}

	if (heap_tail != 1) {
		vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
			"in init_route_structs. Heap is not empty.\n");
	}
}

struct s_trace *
update_traceback(struct s_heap *hptr, int inet) {

	/* This routine adds the most recently finished wire segment to the         *
	 * traceback linked list.  The first connection starts with the net SOURCE  *
	 * and begins at the structure pointed to by trace_head[inet]. Each         *
	 * connection ends with a SINK.  After each SINK, the next connection       *
	 * begins (if the net has more than 2 pins).  The first element after the   *
	 * SINK gives the routing node on a previous piece of the routing, which is *
	 * the link from the existing net to this new piece of the net.             *
	 * In each traceback I start at the end of a path and trace back through    *
	 * its predecessors to the beginning.  I have stored information on the     *
	 * predecesser of each node to make traceback easy -- this sacrificies some *
	 * memory for easier code maintenance.  This routine returns a pointer to   *
	 * the first "new" node in the traceback (node not previously in trace).    */

	struct s_trace *tptr, *prevptr, *temptail, *ret_ptr;
	int inode;
	short iedge;

#ifdef DEBUG
	t_rr_type rr_type;
#endif

	// hptr points to the end of a connection
	inode = hptr->index;

#ifdef DEBUG
	rr_type = rr_node[inode].type;
	if (rr_type != SINK) {
		vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
			"in update_traceback. Expected type = SINK (%d).\n"
			"\tGot type = %d while tracing back net %d.\n", SINK, rr_type, inet);
	}
#endif

	tptr = alloc_trace_data(); /* SINK on the end of the connection */
	tptr->index = inode;
	tptr->iswitch = OPEN;
	tptr->next = NULL;
	temptail = tptr; /* This will become the new tail at the end */
	/* of the routine.                          */

	/* Now do it's predecessor. */

	inode = hptr->u.prev_node;
	iedge = hptr->prev_edge;

	while (inode != NO_PREVIOUS) {
		prevptr = alloc_trace_data();
		prevptr->index = inode;
		prevptr->iswitch = rr_node[inode].switches[iedge];
		prevptr->next = tptr;
		tptr = prevptr;

		iedge = rr_node_route_inf[inode].prev_edge;
		inode = rr_node_route_inf[inode].prev_node;
	}

	if (trace_tail[inet] != NULL) {
		trace_tail[inet]->next = tptr; /* Traceback ends with tptr */
		ret_ptr = tptr->next; /* First new segment.       */
	} else { /* This was the first "chunk" of the net's routing */
		trace_head[inet] = tptr;
		ret_ptr = tptr; /* Whole traceback is new. */
	}

	trace_tail[inet] = temptail;
	return (ret_ptr);
}

void reset_path_costs(void) {

	/* The routine sets the path_cost to HUGE_POSITIVE_FLOAT for all channel segments   *
	 * touched by previous routing phases.                                     */

	struct s_linked_f_pointer *mod_ptr;

#ifdef DEBUG
	int num_mod_ptrs;
#endif

	/* The traversal method below is slightly painful to make it faster. */

	if (rr_modified_head != NULL) {
		mod_ptr = rr_modified_head;

#ifdef DEBUG
		num_mod_ptrs = 1;
#endif

		while (mod_ptr->next != NULL) {
			*(mod_ptr->fptr) = HUGE_POSITIVE_FLOAT;
			mod_ptr = mod_ptr->next;
#ifdef DEBUG
			num_mod_ptrs++;
#endif
		}
		*(mod_ptr->fptr) = HUGE_POSITIVE_FLOAT; /* Do last one. */

		/* Reset the modified list and put all the elements back in the free   *
		 * list.                                                               */

		mod_ptr->next = linked_f_pointer_free_head;
		linked_f_pointer_free_head = rr_modified_head;
		rr_modified_head = NULL;

#ifdef DEBUG
		num_linked_f_pointer_allocated -= num_mod_ptrs;
#endif
	}
}

float get_rr_cong_cost(int inode) {

	/* Returns the *congestion* cost of using this rr_node. */

	short cost_index;
	float cost;

	cost_index = rr_node[inode].get_cost_index();
	cost = rr_indexed_data[cost_index].base_cost
			* rr_node_route_inf[inode].acc_cost
			* rr_node_route_inf[inode].pres_cost;
	return (cost);
}

void mark_ends(int inet) {

	/* Mark all the SINKs of this net as targets by setting their target flags  *
	 * to the number of times the net must connect to each SINK.  Note that     *
	 * this number can occassionally be greater than 1 -- think of connecting   *
	 * the same net to two inputs of an and-gate (and-gate inputs are logically *
	 * equivalent, so both will connect to the same SINK).                      */

	unsigned int ipin;
	int inode;

	for (ipin = 1; ipin < g_clbs_nlist.net[inet].pins.size(); ipin++) {
		inode = net_rr_terminals[inet][ipin];
		rr_node_route_inf[inode].target_flag++;
	}
}

void mark_remaining_ends(int inet, const vector<int>& remaining_sinks) {
	// like mark_ends, but only performs it for the remaining sinks of a net
	for (int sink_node : remaining_sinks)
		++rr_node_route_inf[sink_node].target_flag;
}

void node_to_heap(int inode, float total_cost, int prev_node, int prev_edge,
		float backward_path_cost, float R_upstream) {

	/* Puts an rr_node on the heap, if the new cost given is lower than the     *
	 * current path_cost to this channel segment.  The index of its predecessor *
	 * is stored to make traceback easy.  The index of the edge used to get     *
	 * from its predecessor to it is also stored to make timing analysis, etc.  *
	 * easy.  The backward_path_cost and R_upstream values are used only by the *
	 * timing-driven router -- the breadth-first router ignores them.           */

	if (total_cost >= rr_node_route_inf[inode].path_cost)
		return;

	s_heap* hptr = alloc_heap_data();
	hptr->index = inode;
	hptr->cost = total_cost;
	hptr->u.prev_node = prev_node;
	hptr->prev_edge = prev_edge;
	hptr->backward_path_cost = backward_path_cost;
	hptr->R_upstream = R_upstream;
	add_to_heap(hptr);
}

void free_traceback(int inet) {

	/* Puts the entire traceback (old routing) for this net on the free list *
	 * and sets the trace_head pointers etc. for the net to NULL.            */

	struct s_trace *tptr, *tempptr;

	if(trace_head == NULL) {
		return;
	}

	tptr = trace_head[inet];

	while (tptr != NULL) {
		tempptr = tptr->next;
		free_trace_data(tptr);
		tptr = tempptr;
	}

	trace_head[inet] = NULL;
	trace_tail[inet] = NULL;
}

t_ivec **
alloc_route_structs(void) {

	/* Allocates the data structures needed for routing.    */

	t_ivec **clb_opins_used_locally;

	alloc_route_static_structs();
	clb_opins_used_locally = alloc_and_load_clb_opins_used_locally();

	return (clb_opins_used_locally);
}

void alloc_route_static_structs(void) {
	trace_head = (struct s_trace **) my_calloc(num_nets,
			sizeof(struct s_trace *));
	trace_tail = (struct s_trace **) my_malloc(
			num_nets * sizeof(struct s_trace *));

	heap_size = nx * ny;
	heap = (struct s_heap **) my_malloc(heap_size * sizeof(struct s_heap *));
	heap--; /* heap stores from [1..heap_size] */
	heap_tail = 1;

	route_bb = (struct s_bb *) my_malloc(num_nets * sizeof(struct s_bb));
}

struct s_trace **
alloc_saved_routing(t_ivec ** clb_opins_used_locally,
		t_ivec *** saved_clb_opins_used_locally_ptr) {

	/* Allocates data structures into which the key routing data can be saved,   *
	 * allowing the routing to be recovered later (e.g. after a another routing  *
	 * is attempted).                                                            */

	struct s_trace **best_routing;
	t_ivec **saved_clb_opins_used_locally;
	int iblk, iclass, num_local_opins;
	t_type_ptr type;

	best_routing = (struct s_trace **) my_calloc(g_clbs_nlist.net.size(),
			sizeof(struct s_trace *));

	saved_clb_opins_used_locally = (t_ivec **) my_malloc(
			num_blocks * sizeof(t_ivec *));

	for (iblk = 0; iblk < num_blocks; iblk++) {
		type = block[iblk].type;
		saved_clb_opins_used_locally[iblk] = (t_ivec *) my_malloc(
				type->num_class * sizeof(t_ivec));
		for (iclass = 0; iclass < type->num_class; iclass++) {
			num_local_opins = clb_opins_used_locally[iblk][iclass].nelem;
			saved_clb_opins_used_locally[iblk][iclass].nelem = num_local_opins;

			if (num_local_opins == 0) {
				saved_clb_opins_used_locally[iblk][iclass].list = NULL;
			} else {
				saved_clb_opins_used_locally[iblk][iclass].list =
						(int *) my_malloc(num_local_opins * sizeof(int));
			}
		}
	}

	*saved_clb_opins_used_locally_ptr = saved_clb_opins_used_locally;
	return (best_routing);
}

/* TODO: super hacky, jluu comment, I need to rethink this whole function, without it, logically equivalent output pins incorrectly use more pins than needed.  I force that CLB output pin uses at most one output pin  */
static t_ivec **
alloc_and_load_clb_opins_used_locally(void) {

	/* Allocates and loads the data needed to make the router reserve some CLB  *
	 * output pins for connections made locally within a CLB (if the netlist    *
	 * specifies that this is necessary).                                       */

	t_ivec **clb_opins_used_locally;
	int iblk, clb_pin, iclass, num_local_opins;
	int class_low, class_high;
	t_type_ptr type;

	clb_opins_used_locally = (t_ivec **) my_malloc(
			num_blocks * sizeof(t_ivec *));

	for (iblk = 0; iblk < num_blocks; iblk++) {
		type = block[iblk].type;
		get_class_range_for_block(iblk, &class_low, &class_high);
		clb_opins_used_locally[iblk] = (t_ivec *) my_malloc(
				type->num_class * sizeof(t_ivec));
		for (iclass = 0; iclass < type->num_class; iclass++)
			clb_opins_used_locally[iblk][iclass].nelem = 0;

		for (clb_pin = 0; clb_pin < type->num_pins; clb_pin++) {
			// another hack to avoid I/Os, whole function needs a rethink
			if(type == IO_TYPE) {
				continue;
			}
		
			if ((block[iblk].nets[clb_pin] != OPEN
					&& g_clbs_nlist.net[block[iblk].nets[clb_pin]].num_sinks() == 0) || block[iblk].nets[clb_pin] == OPEN) {
				iclass = type->pin_class[clb_pin];
				if(type->class_inf[iclass].type == DRIVER) {
					/* Check to make sure class is in same range as that assigned to block */
					assert(iclass >= class_low && iclass <= class_high);
					clb_opins_used_locally[iblk][iclass].nelem++;
				}
			}
		}

		for (iclass = 0; iclass < type->num_class; iclass++) {
			num_local_opins = clb_opins_used_locally[iblk][iclass].nelem;

			if (num_local_opins == 0)
				clb_opins_used_locally[iblk][iclass].list = NULL;
			else
				clb_opins_used_locally[iblk][iclass].list = (int *) my_malloc(
						num_local_opins * sizeof(int));
		}
	}

	return (clb_opins_used_locally);
}

void free_trace_structs(void) {
	/*the trace lists are only freed after use by the timing-driven placer */
	/*Do not  free them after use by the router, since stats, and draw  */
	/*routines use the trace values */
	unsigned int i;

	for (i = 0; i < g_clbs_nlist.net.size(); i++)
		free_traceback(i);

	if(trace_head) {
		free(trace_head);
		free(trace_tail);
	}
	trace_head = NULL;
	trace_tail = NULL;
}

void free_route_structs() {

	/* Frees the temporary storage needed only during the routing.  The  *
	 * final routing result is not freed.                                */
	if(heap != NULL) {
		free(heap + 1);
	}
	if(route_bb != NULL) {
		free(route_bb);
	}

	heap = NULL; /* Defensive coding:  crash hard if I use these. */
	route_bb = NULL;

	/*free the memory chunks that were used by heap and linked f pointer */
	free_chunk_memory(&heap_ch);
	free_chunk_memory(&linked_f_pointer_ch);
	heap_free_head = NULL;
	linked_f_pointer_free_head = NULL;
}

void free_saved_routing(struct s_trace **best_routing,
		t_ivec ** saved_clb_opins_used_locally) {

	/* Frees the data structures needed to save a routing.                     */
	int i;

	free(best_routing);
	for (i = 0; i < num_blocks; i++) {
		free_ivec_vector(saved_clb_opins_used_locally[i], 0,
				block[i].type->num_class - 1);
	}
	free(saved_clb_opins_used_locally);
}

void alloc_and_load_rr_node_route_structs(void) {

	/* Allocates some extra information about each rr_node that is used only   *
	 * during routing.                                                         */

	int inode;

	if (rr_node_route_inf != NULL) {
		vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
			"in alloc_and_load_rr_node_route_structs: old rr_node_route_inf array exists.\n");
	}

	rr_node_route_inf = (t_rr_node_route_inf *) my_malloc(num_rr_nodes * sizeof(t_rr_node_route_inf));

	for (inode = 0; inode < num_rr_nodes; inode++) {
		rr_node_route_inf[inode].prev_node = NO_PREVIOUS;
		rr_node_route_inf[inode].prev_edge = NO_PREVIOUS;
		rr_node_route_inf[inode].pres_cost = 1.0;
		rr_node_route_inf[inode].acc_cost = 1.0;
		rr_node_route_inf[inode].path_cost = HUGE_POSITIVE_FLOAT;
		rr_node_route_inf[inode].target_flag = 0;
	}
}

void reset_rr_node_route_structs(void) {

	/* Allocates some extra information about each rr_node that is used only   *
	 * during routing.                                                         */

	int inode;

	assert(rr_node_route_inf != NULL);

	for (inode = 0; inode < num_rr_nodes; inode++) {
		rr_node_route_inf[inode].prev_node = NO_PREVIOUS;
		rr_node_route_inf[inode].prev_edge = NO_PREVIOUS;
		rr_node_route_inf[inode].pres_cost = 1.0;
		rr_node_route_inf[inode].acc_cost = 1.0;
		rr_node_route_inf[inode].path_cost = HUGE_POSITIVE_FLOAT;
		rr_node_route_inf[inode].target_flag = 0;
	}
}

void free_rr_node_route_structs(void) {

	/* Frees the extra information about each rr_node that is needed only      *
	 * during routing.                                                         */

	free(rr_node_route_inf);
	rr_node_route_inf = NULL; /* Mark as free */
}

/* RESEARCH TODO: Bounding box heuristic needs to be redone for heterogeneous blocks */
static void load_route_bb(int bb_factor) {

	/* This routine loads the bounding box arrays used to limit the space  *
	 * searched by the maze router when routing each net.  The search is   *
	 * limited to channels contained with the net bounding box expanded    *
	 * by bb_factor channels on each side.  For example, if bb_factor is   *
	 * 0, the maze router must route each net within its bounding box.     *
	 * If bb_factor = nx, the maze router will search every channel in     *
	 * the FPGA if necessary.  The bounding boxes returned by this routine *
	 * are different from the ones used by the placer in that they are     * 
	 * clipped to lie within (0,0) and (nx+1,ny+1) rather than (1,1) and   *
	 * (nx,ny).                                                            */

	unsigned int k, inet;
	int xmax, ymax, xmin, ymin, x, y;

	for (inet = 0; inet < g_clbs_nlist.net.size(); inet++) {
		x = block[g_clbs_nlist.net[inet].pins[0].block].x
			+ block[g_clbs_nlist.net[inet].pins[0].block].type->pin_width[g_clbs_nlist.net[inet].pins[0].block_pin];
		y = block[g_clbs_nlist.net[inet].pins[0].block].y
			+ block[g_clbs_nlist.net[inet].pins[0].block].type->pin_height[g_clbs_nlist.net[inet].pins[0].block_pin];

		xmin = x;
		ymin = y;
		xmax = x;
		ymax = y;

		for (k = 1; k < g_clbs_nlist.net[inet].pins.size(); k++) {
			x = block[g_clbs_nlist.net[inet].pins[k].block].x
				+ block[g_clbs_nlist.net[inet].pins[k].block].type->pin_width[g_clbs_nlist.net[inet].pins[k].block_pin];
			y = block[g_clbs_nlist.net[inet].pins[k].block].y
				+ block[g_clbs_nlist.net[inet].pins[k].block].type->pin_height[g_clbs_nlist.net[inet].pins[k].block_pin];

			if (x < xmin) {
				xmin = x;
			} else if (x > xmax) {
				xmax = x;
			}

			if (y < ymin) {
				ymin = y;
			} else if (y > ymax) {
				ymax = y;
			}
		}

		/* Want the channels on all 4 sides to be usuable, even if bb_factor = 0. */

		xmin -= 1;
		ymin -= 1;

		/* Expand the net bounding box by bb_factor, then clip to the physical *
		 * chip area.                                                          */

		route_bb[inet].xmin = max(xmin - bb_factor, 0);
		route_bb[inet].xmax = min(xmax + bb_factor, nx + 1);
		route_bb[inet].ymin = max(ymin - bb_factor, 0);
		route_bb[inet].ymax = min(ymax + bb_factor, ny + 1);
	}
}

void add_to_mod_list(float *fptr) {

	/* This routine adds the floating point pointer (fptr) into a  *
	 * linked list that indicates all the pathcosts that have been *
	 * modified thus far.                                          */

	struct s_linked_f_pointer *mod_ptr;

	mod_ptr = alloc_linked_f_pointer();

	/* Add this element to the start of the modified list. */

	mod_ptr->next = rr_modified_head;
	mod_ptr->fptr = fptr;
	rr_modified_head = mod_ptr;
}
namespace heap_ {
	size_t parent(size_t i) {return i >> 1;}
	// child indices of a heap
	size_t left(size_t i) {return i << 1;}
	size_t right(size_t i) {return (i << 1) + 1;}
	size_t size() {return static_cast<size_t>(heap_tail - 1);}	// heap[0] is not valid element

	// make a heap rooted at index i by **sifting down** in O(lgn) time
	void sift_down(size_t hole) {
		s_heap* head {heap[hole]};
		size_t child {left(hole)};
		while ((int)child < heap_tail) {
			if ((int)child + 1 < heap_tail && heap[child + 1]->cost < heap[child]->cost)
				++child;
			if (heap[child]->cost < head->cost) {
				heap[hole] = heap[child];
				hole = child;
				child = left(child);
			}
			else break;
		}
		heap[hole] = head;
	}


	// runs in O(n) time by sifting down; the least work is done on the most elements: 1 swap for bottom layer, 2 swap for 2nd, ... lgn swap for top
	// 1*(n/2) + 2*(n/4) + 3*(n/8) + ... + lgn*1 = 2n (sum of i/2^i)
	void build_heap() {
		// second half of heap are leaves
		for (size_t i = heap_tail >> 1; i != 0; --i)
			sift_down(i);
	}


	// O(lgn) sifting up to maintain heap property after insertion (should sift down when building heap)
	void sift_up(size_t leaf, s_heap* const node) {
		while ((leaf > 1) && (node->cost < heap[parent(leaf)]->cost)) {
			// sift hole up
			heap[leaf] = heap[parent(leaf)];
			leaf = parent(leaf);
		}
		heap[leaf] = node;
	}


	void expand_heap_if_full() {
		if (heap_tail > heap_size) { /* Heap is full */
			heap_size *= 2;
			heap = (struct s_heap **) my_realloc((void *) (heap + 1),
					heap_size * sizeof(struct s_heap *));
			heap--; /* heap goes from [1..heap_size] */
		}		
	}

	// adds an element to the back of heap and expand if necessary, but does not maintain heap property
	void push_back(s_heap* const hptr) {
		expand_heap_if_full();
		heap[heap_tail] = hptr;
		++heap_tail;
	}


	void push_back_node(int inode, float total_cost, int prev_node, int prev_edge,
		float backward_path_cost, float R_upstream) {

		/* Puts an rr_node on the heap with the same condition as node_to_heap,
		   but do not fix heap property yet as that is more efficiently done from
		   bottom up with build_heap    */
	
		if (total_cost >= rr_node_route_inf[inode].path_cost)
			return;

		s_heap* hptr = alloc_heap_data();
		hptr->index = inode;
		hptr->cost = total_cost;
		hptr->u.prev_node = prev_node;
		hptr->prev_edge = prev_edge;
		hptr->backward_path_cost = backward_path_cost;
		hptr->R_upstream = R_upstream;
		push_back(hptr);
	}

	bool is_valid() {
		for (size_t i = 1; (int)i <= heap_tail >> 1; ++i) {
			if ((int)left(i) < heap_tail && heap[left(i)]->cost < heap[i]->cost) return false;
			if ((int)right(i) < heap_tail && heap[right(i)]->cost < heap[i]->cost) return false;
		}
		return true;
	}
	// extract every element and print it
	void pop_heap() {
		while (!is_empty_heap()) vpr_printf_info("%e ", get_heap_head()->cost);
		vpr_printf_info("\n");
	}
	// print every element; not necessarily in order for minheap
	void print_heap() {
		for (int i = 1; i < heap_tail >> 1; ++i) vpr_printf_info("(%e %e %e) ", heap[i]->cost, heap[left(i)]->cost, heap[right(i)]->cost);
		vpr_printf_info("\n");
	}
	// verify correctness of extract top by making a copy, sorting it, and iterating it at the same time as extraction
	void verify_extract_top() {
		constexpr float float_epsilon = 1e-20;
		std::cout << "copying heap\n";
		std::vector<s_heap*> heap_copy {heap + 1, heap + heap_tail};
		// sort based on cost with cheapest first
		assert(heap_copy.size() == size());
		std::sort(begin(heap_copy), end(heap_copy), 
			[](const s_heap* a, const s_heap* b){
			return a->cost < b->cost;
		});
		std::cout << "starting to compare top elements\n";
		size_t i = 0;
		while (!is_empty_heap()) {
			while (heap_copy[i]->index == OPEN) ++i;	// skip the ones that won't be extracted
			auto top = get_heap_head();
			if (abs(top->cost - heap_copy[i]->cost) > float_epsilon)	
				std::cout << "mismatch with sorted " << top << '(' << top->cost << ") " << heap_copy[i] << '(' << heap_copy[i]->cost << ")\n";
			++i;
		}
		if (i != heap_copy.size()) std::cout << "did not finish extracting: " << i << " vs " << heap_copy.size() << std::endl;
		else std::cout << "extract top working as intended\n";
	}
}
// adds to heap and maintains heap quality
static void add_to_heap(struct s_heap *hptr) {
	heap_::expand_heap_if_full();
	// start with undefined hole
	++heap_tail;	
	heap_::sift_up(heap_tail - 1, hptr);
}

/*WMF: peeking accessor :) */
bool is_empty_heap(void) {
	return (bool)(heap_tail == 1);
}

struct s_heap *
get_heap_head(void) {

	/* Returns a pointer to the smallest element on the heap, or NULL if the     *
	 * heap is empty.  Invalid (index == OPEN) entries on the heap are never     *
	 * returned -- they are just skipped over.                                   */

	struct s_heap *cheapest;
	size_t hole, child;

	do {
		if (heap_tail == 1) { /* Empty heap. */
			vpr_printf_warning(__FILE__, __LINE__, "Empty heap occurred in get_heap_head.\n");
			vpr_printf_warning(__FILE__, __LINE__, "Some blocks are impossible to connect in this architecture.\n");
			return (NULL);
		}

		cheapest = heap[1]; 

		hole = 1;
		child = 2;
		--heap_tail;
		while ((int)child < heap_tail) {
			if (heap[child + 1]->cost < heap[child]->cost)
				++child;	// become right child
			heap[hole] = heap[child];
			hole = child;
			child = heap_::left(child);
		}
		heap_::sift_up(hole, heap[heap_tail]);

	} while (cheapest->index == OPEN); /* Get another one if invalid entry. */

	return (cheapest);
}

void empty_heap(void) {

	for (int i = 1; i < heap_tail; i++)
		free_heap_data(heap[i]);

	heap_tail = 1;
}

static struct s_heap *
alloc_heap_data(void) {

	struct s_heap *temp_ptr;

	if (heap_free_head == NULL) { /* No elements on the free list */
		heap_free_head = (struct s_heap *) my_chunk_malloc(sizeof(struct s_heap),&heap_ch);
		heap_free_head->u.next = NULL;
	}

	temp_ptr = heap_free_head;
	heap_free_head = heap_free_head->u.next;
#ifdef DEBUG
	num_heap_allocated++;
#endif
	return (temp_ptr);
}

void free_heap_data(struct s_heap *hptr) {

	hptr->u.next = heap_free_head;
	heap_free_head = hptr;
#ifdef DEBUG
	num_heap_allocated--;
#endif
}

void invalidate_heap_entries(int sink_node, int ipin_node) {

	/* Marks all the heap entries consisting of sink_node, where it was reached *
	 * via ipin_node, as invalid (OPEN).  Used only by the breadth_first router *
	 * and even then only in rare circumstances.                                */

	for (int i = 1; i < heap_tail; i++) {
		if (heap[i]->index == sink_node && heap[i]->u.prev_node == ipin_node)
			heap[i]->index = OPEN; /* Invalid. */
	}
}

struct s_trace *
alloc_trace_data(void) {

	struct s_trace *temp_ptr;

	if (trace_free_head == NULL) { /* No elements on the free list */
		trace_free_head = (struct s_trace *) my_chunk_malloc(sizeof(struct s_trace),&trace_ch);
		trace_free_head->next = NULL;
	}
	temp_ptr = trace_free_head;
	trace_free_head = trace_free_head->next;
#ifdef DEBUG
	num_trace_allocated++;
#endif
	return (temp_ptr);
}

void free_trace_data(struct s_trace *tptr) {

	/* Puts the traceback structure pointed to by tptr on the free list. */

	tptr->next = trace_free_head;
	trace_free_head = tptr;
#ifdef DEBUG
	num_trace_allocated--;
#endif
}

static struct s_linked_f_pointer *
alloc_linked_f_pointer(void) {

	/* This routine returns a linked list element with a float pointer as *
	 * the node data.                                                     */

	/*int i;*/
	struct s_linked_f_pointer *temp_ptr;

	if (linked_f_pointer_free_head == NULL) {
		/* No elements on the free list */	
	linked_f_pointer_free_head = (struct s_linked_f_pointer *) my_chunk_malloc(sizeof(struct s_linked_f_pointer),&linked_f_pointer_ch);
	linked_f_pointer_free_head->next = NULL;
	}

	temp_ptr = linked_f_pointer_free_head;
	linked_f_pointer_free_head = linked_f_pointer_free_head->next;

#ifdef DEBUG
	num_linked_f_pointer_allocated++;
#endif

	return (temp_ptr);
}

void print_route(char *route_file) {

	/* Prints out the routing to file route_file.  */

	unsigned int inet;
	int inode, bnum, ilow, jlow, node_block_pin, iclass;
	unsigned int ipin;
	t_rr_type rr_type;
	struct s_trace *tptr;
	FILE *fp;

	fp = fopen(route_file, "w");

	fprintf(fp, "Array size: %d x %d logic blocks.\n", nx, ny);
	fprintf(fp, "\nRouting:");
	for (inet = 0; inet < g_clbs_nlist.net.size(); inet++) {
		if (g_clbs_nlist.net[inet].is_global == false) {
			if (g_clbs_nlist.net[inet].num_sinks() == false) {
				fprintf(fp, "\n\nNet %d (%s)\n\n", inet, g_clbs_nlist.net[inet].name);
				fprintf(fp, "\n\nUsed in local cluster only, reserved one CLB pin\n\n");
			} else {
				fprintf(fp, "\n\nNet %d (%s)\n\n", inet, g_clbs_nlist.net[inet].name);
				tptr = trace_head[inet];

				while (tptr != NULL) {
					inode = tptr->index;
					rr_type = rr_node[inode].type;
					ilow = rr_node[inode].get_xlow();
					jlow = rr_node[inode].get_ylow();

					fprintf(fp, "Node:\t%d\t%6s (%d,%d) ", inode, 
							rr_node[inode].rr_get_type_string(), ilow, jlow);

					if ((ilow != rr_node[inode].get_xhigh())
							|| (jlow != rr_node[inode].get_yhigh()))
						fprintf(fp, "to (%d,%d) ", rr_node[inode].get_xhigh(),
								rr_node[inode].get_yhigh());

					switch (rr_type) {

					case IPIN:
					case OPIN:
						if (grid[ilow][jlow].type == IO_TYPE) {
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
						if (grid[ilow][jlow].type == IO_TYPE) {
							fprintf(fp, " Pad: ");
						} else { /* IO Pad. */
							fprintf(fp, " Class: ");
						}
						break;

					default:
						vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
								  "in print_route: Unexpected traceback element type: %d (%s).\n", 
								  rr_type, rr_node[inode].rr_get_type_string());
						break;
					}

					fprintf(fp, "%d  ", rr_node[inode].get_ptc_num());

					if (grid[ilow][jlow].type != IO_TYPE && (rr_type == IPIN || rr_type == OPIN)) {
						int pin_num = rr_node[inode].get_ptc_num();
						int offset = grid[ilow][jlow].height_offset;
						int iblock = grid[ilow][jlow - offset].blocks[0];
						assert(iblock != OPEN);
						t_pb_graph_pin *pb_pin = get_pb_graph_node_pin_from_block_pin(iblock, pin_num);
						t_pb_type *pb_type = pb_pin->parent_node->pb_type;
						fprintf(fp, " %s.%s[%d] ", pb_type->name, pb_pin->port->name, pb_pin->pin_number);
					}

					/* Uncomment line below if you're debugging and want to see the switch types *
					 * used in the routing.                                                      */
					/*          fprintf (fp, "Switch: %d", tptr->iswitch);    */

					fprintf(fp, "\n");

					tptr = tptr->next;
				}
			}
		}

		else { /* Global net.  Never routed. */
			fprintf(fp, "\n\nNet %d (%s): global net connecting:\n\n", inet,
					g_clbs_nlist.net[inet].name);

			for (ipin = 0; ipin < g_clbs_nlist.net[inet].pins.size(); ipin++) {
				bnum = g_clbs_nlist.net[inet].pins[ipin].block;

				node_block_pin = g_clbs_nlist.net[inet].pins[ipin].block_pin;
				iclass = block[bnum].type->pin_class[node_block_pin];

				fprintf(fp, "Block %s (#%d) at (%d, %d), Pin class %d.\n",
						block[bnum].name, bnum, block[bnum].x, block[bnum].y,
						iclass);
			}
		}
	}

	fclose(fp);

	if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_MEM)) {
		fp = my_fopen(getEchoFileName(E_ECHO_MEM), "w", 0);
		fprintf(fp, "\nNum_heap_allocated: %d   Num_trace_allocated: %d\n",
				num_heap_allocated, num_trace_allocated);
		fprintf(fp, "Num_linked_f_pointer_allocated: %d\n",
				num_linked_f_pointer_allocated);
		fclose(fp);
	}

}

/* TODO: jluu: I now always enforce logically equivalent outputs to use at most one output pin, should rethink how to do this */
void reserve_locally_used_opins(float pres_fac, float acc_fac, bool rip_up_local_opins,
		t_ivec ** clb_opins_used_locally) {

	/* In the past, this function implicitly allowed LUT duplication when there are free LUTs. 
	 This was especially important for logical equivalence; however, now that we have a very general
	 logic cluster, it does not make sense to allow LUT duplication implicitly. we'll need to look into how we want to handle this case

	 */

	int iblk, num_local_opin, inode, from_node, iconn, num_edges, to_node;
	int iclass, ipin;
	float cost;
	struct s_heap *heap_head_ptr;
	t_type_ptr type;

	if (rip_up_local_opins) {
		for (iblk = 0; iblk < num_blocks; iblk++) {
			type = block[iblk].type;
			for (iclass = 0; iclass < type->num_class; iclass++) {
				num_local_opin = clb_opins_used_locally[iblk][iclass].nelem;
				/* Always 0 for pads and for RECEIVER (IPIN) classes */
				for (ipin = 0; ipin < num_local_opin; ipin++) {
					inode = clb_opins_used_locally[iblk][iclass].list[ipin];
					adjust_one_rr_occ_and_apcost(inode, -1, pres_fac, acc_fac);
				}
			}
		}
	}

	for (iblk = 0; iblk < num_blocks; iblk++) {
		type = block[iblk].type;
		for (iclass = 0; iclass < type->num_class; iclass++) {
			num_local_opin = clb_opins_used_locally[iblk][iclass].nelem;
			/* Always 0 for pads and for RECEIVER (IPIN) classes */

			if (num_local_opin != 0) { /* Have to reserve (use) some OPINs */
				from_node = rr_blk_source[iblk][iclass];
				num_edges = rr_node[from_node].get_num_edges();
				for (iconn = 0; iconn < num_edges; iconn++) {
					to_node = rr_node[from_node].edges[iconn];
					cost = get_rr_cong_cost(to_node);
					node_to_heap(to_node, cost, OPEN, OPEN, 0., 0.);
				}

				for (ipin = 0; ipin < num_local_opin; ipin++) {
					heap_head_ptr = get_heap_head();
					inode = heap_head_ptr->index;
					adjust_one_rr_occ_and_apcost(inode, 1, pres_fac, acc_fac);
					clb_opins_used_locally[iblk][iclass].list[ipin] = inode;
					free_heap_data(heap_head_ptr);
				}

				empty_heap();
			}
		}
	}
}

static void adjust_one_rr_occ_and_apcost(int inode, int add_or_sub,
		float pres_fac, float acc_fac) {

	/* Increments or decrements (depending on add_or_sub) the occupancy of    *
	 * one rr_node, and adjusts the present cost of that node appropriately.  */

	int occ, capacity;

	occ = rr_node[inode].get_occ() + add_or_sub;
	capacity = rr_node[inode].get_capacity();
	rr_node[inode].set_occ(occ);

	if (occ < capacity) {
		rr_node_route_inf[inode].pres_cost = 1.0;
	} else {
		rr_node_route_inf[inode].pres_cost = 1.0 + (occ + 1 - capacity) * pres_fac;
		if (add_or_sub == 1) {
			rr_node_route_inf[inode].acc_cost += (occ - capacity) * acc_fac;
		}
	}
}

void free_chunk_memory_trace(void) {
	if (trace_ch.chunk_ptr_head != NULL) {
		free_chunk_memory(&trace_ch);
	}
}


// connection based overhaul (more specificity than nets)
// utility and debugging functions -----------------------
void print_traceback(int inet) {
	// linearly print linked list
	vpr_printf_info("traceback %d: ", inet);
	t_trace* head = trace_head[inet];
	while (head) {
		int inode {head->index};
		if (rr_node[inode].type == SINK) 
			vpr_printf_info("%d(sink)(%d)->",inode, rr_node[inode].get_occ());
		else 
			vpr_printf_info("%d(%d)->",inode, rr_node[inode].get_occ());
		head = head->next;
	}
	vpr_printf_info("\n");
}
