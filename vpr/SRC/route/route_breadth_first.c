#include <cstdio>
using namespace std;

#include "vtr_log.h"

#include "vpr_types.h"
#include "globals.h"
#include "route_export.h"
#include "route_common.h"
#include "route_breadth_first.h"

/********************* Subroutines local to this module *********************/

static bool breadth_first_route_net(int inet, int itry, float bend_cost);

static void breadth_first_expand_trace_segment(struct s_trace *start_ptr,
		int remaining_connections_to_sink);

static void breadth_first_expand_neighbours(int inode, float pcost, 
		int inet, int itry, float bend_cost);

static void breadth_first_add_source_to_heap(int inet);

/************************ Subroutine definitions ****************************/

bool try_breadth_first_route(struct s_router_opts router_opts,
		vtr::t_ivec ** clb_opins_used_locally, int width_fac) {

	/* Iterated maze router ala Pathfinder Negotiated Congestion algorithm,  *
	 * (FPGA 95 p. 111).  Returns true if it can route this FPGA, false if   *
	 * it can't.                                                             */

	float pres_fac;
	bool success, is_routable, rip_up_local_opins;
	int itry;
	unsigned int inet;

	/* Usually the first iteration uses a very small (or 0) pres_fac to find  *
	 * the shortest path and get a congestion map.  For fast compiles, I set  *
	 * pres_fac high even for the first iteration.                            */

	pres_fac = router_opts.first_iter_pres_fac;

	for (itry = 1; itry <= router_opts.max_router_iterations; itry++) {

		/* Reset "is_routed" and "is_fixed" flags to indicate nets not pre-routed (yet) */
		for (inet = 0; inet < g_clbs_nlist.net.size(); inet++) {
			g_clbs_nlist.net[inet].is_routed = false;
			g_clbs_nlist.net[inet].is_fixed = false;
		}

		for (inet = 0; inet < g_clbs_nlist.net.size(); inet++) {
			is_routable = try_breadth_first_route_net(inet, itry, pres_fac, 
					router_opts);
			if (!is_routable) {
				return (false);
			}
		}

		/* Make sure any CLB OPINs used up by subblocks being hooked directly     *
		 * to them are reserved for that purpose.                                 */

		if (itry == 1)
			rip_up_local_opins = false;
		else
			rip_up_local_opins = true;

		reserve_locally_used_opins(pres_fac, router_opts.acc_fac, rip_up_local_opins,
				clb_opins_used_locally);

		success = feasible_routing();
		if (success) {
			vtr::printf_info("Successfully routed after %d routing iterations.\n", itry);
			return (true);
		}

		if (itry == 1)
			pres_fac = router_opts.initial_pres_fac;
		else
			pres_fac *= router_opts.pres_fac_mult;

		pres_fac = min(pres_fac, static_cast<float>(HUGE_POSITIVE_FLOAT / 1e5));

		pathfinder_update_cost(pres_fac, router_opts.acc_fac);
	}

	vtr::printf_info("Routing failed.\n");
	return (false);
}

bool try_breadth_first_route_net(int inet, int itry, float pres_fac, 
		struct s_router_opts router_opts) {

	bool is_routed = false;

	if (g_clbs_nlist.net[inet].is_fixed) { /* Skip pre-routed nets. */

		is_routed = true;

	} else if (g_clbs_nlist.net[inet].is_global) { /* Skip global nets. */

		is_routed = true;

	} else {

		pathfinder_update_path_cost(trace_head[inet], -1, pres_fac);
		is_routed = breadth_first_route_net(inet, itry, router_opts.bend_cost);

		/* Impossible to route? (disconnected rr_graph) */
		if (is_routed) {
			g_clbs_nlist.net[inet].is_routed = true;
			g_atoms_nlist.net[clb_to_vpack_net_mapping[inet]].is_routed = true;
		} else {
			vtr::printf_info("Routing failed.\n");
		}

		pathfinder_update_path_cost(trace_head[inet], 1, pres_fac);
	}
	return (is_routed);
}

static bool breadth_first_route_net(int inet, int itry, float bend_cost) {

	/* Uses a maze routing (Dijkstra's) algorithm to route a net.  The net       *
	 * begins at the net output, and expands outward until it hits a target      *
	 * pin.  The algorithm is then restarted with the entire first wire segment  *
	 * included as part of the source this time.  For an n-pin net, the maze     *
	 * router is invoked n-1 times to complete all the connections.  Inet is     *
	 * the index of the net to be routed.  Bends are penalized by bend_cost      *
	 * (which is typically zero for detailed routing and nonzero only for global *
	 * routing), since global routes with lots of bends are tougher to detailed  *
	 * route (using a detailed router like SEGA).                                *
	 * If this routine finds that a net *cannot* be connected (due to a complete *
	 * lack of potential paths, rather than congestion), it returns false, as    *
	 * routing is impossible on this architecture.  Otherwise it returns true.   */

	int inode, prev_node, remaining_connections_to_sink;
	unsigned int i;
	float pcost, new_pcost;
	struct s_heap *current;
	struct s_trace *tptr;

	free_traceback(inet);
	breadth_first_add_source_to_heap(inet);
	mark_ends(inet);

	tptr = NULL;
	remaining_connections_to_sink = 0;

	for (i = 1; i < g_clbs_nlist.net[inet].pins.size(); i++) { /* Need n-1 wires to connect n pins */
		breadth_first_expand_trace_segment(tptr, remaining_connections_to_sink);
		current = get_heap_head();

		if (current == NULL) { /* Infeasible routing.  No possible path for net. */
			vtr::printf_info("Cannot route net #%d (%s) to sink #%d -- no possible path.\n",
					inet, g_clbs_nlist.net[inet].name, i);
			reset_path_costs(); /* Clean up before leaving. */
			return (false);
		}

		inode = current->index;

		while (rr_node_route_inf[inode].target_flag == 0) {
			pcost = rr_node_route_inf[inode].path_cost;
			new_pcost = current->cost;
			if (pcost > new_pcost) { /* New path is lowest cost. */
				rr_node_route_inf[inode].path_cost = new_pcost;
				prev_node = current->u.prev_node;
				rr_node_route_inf[inode].prev_node = prev_node;
				rr_node_route_inf[inode].prev_edge = current->prev_edge;

				if (pcost > 0.99 * HUGE_POSITIVE_FLOAT) /* First time touched. */
					add_to_mod_list(&rr_node_route_inf[inode].path_cost);

				breadth_first_expand_neighbours(inode, new_pcost, inet, itry,
						bend_cost);
			}

			free_heap_data(current);
			current = get_heap_head();

			if (current == NULL) { /* Impossible routing. No path for net. */
				vtr::printf_info("Cannot route net #%d (%s) to sink #%d -- no possible path.\n",
						inet, g_clbs_nlist.net[inet].name, i);
				reset_path_costs();
				return (false);
			}

			inode = current->index;
		}

		rr_node_route_inf[inode].target_flag--; /* Connected to this SINK. */
		remaining_connections_to_sink = rr_node_route_inf[inode].target_flag;
		tptr = update_traceback(current, inet);
		free_heap_data(current);
	}

	empty_heap();
	reset_path_costs();
	return (true);
}

static void breadth_first_expand_trace_segment(struct s_trace *start_ptr,
		int remaining_connections_to_sink) {

	/* Adds all the rr_nodes in the traceback segment starting at tptr (and     *
	 * continuing to the end of the traceback) to the heap with a cost of zero. *
	 * This allows expansion to begin from the existing wiring.  The            *
	 * remaining_connections_to_sink value is 0 if the route segment ending     *
	 * at this location is the last one to connect to the SINK ending the route *
	 * segment.  This is the usual case.  If it is not the last connection this *
	 * net must make to this SINK, I have a hack to ensure the next connection  *
	 * to this SINK goes through a different IPIN.  Without this hack, the      *
	 * router would always put all the connections from this net to this SINK   *
	 * through the same IPIN.  With LUTs or cluster-based logic blocks, you     *
	 * should never have a net connecting to two logically-equivalent pins on   *
	 * the same logic block, so the hack will never execute.  If your logic     *
	 * block is an and-gate, however, nets might connect to two and-inputs on   *
	 * the same logic block, and since the and-inputs are logically-equivalent, *
	 * this means two connections to the same SINK.                             */

	struct s_trace *tptr, *next_ptr;
	int inode, sink_node, last_ipin_node;

	tptr = start_ptr;
	if(tptr != NULL && rr_node[tptr->index].type == SINK) {
		/* During logical equivalence case, only use one opin */
		tptr = tptr->next;
	}

	if (remaining_connections_to_sink == 0) { /* Usual case. */
		while (tptr != NULL) {
			node_to_heap(tptr->index, 0., NO_PREVIOUS, NO_PREVIOUS, OPEN, OPEN);
			tptr = tptr->next;
		}
	}

	else { /* This case never executes for most logic blocks. */

		/* Weird case.  Lots of hacks. The cleanest way to do this would be to empty *
		 * the heap, update the congestion due to the partially-completed route, put *
		 * the whole route so far (excluding IPINs and SINKs) on the heap with cost  *
		 * 0., and expand till you hit the next SINK.  That would be slow, so I      *
		 * do some hacks to enable incremental wavefront expansion instead.          */

		if (tptr == NULL)
			return; /* No route yet */

		next_ptr = tptr->next;
		last_ipin_node = OPEN; /* Stops compiler from complaining. */

		/* Can't put last SINK on heap with NO_PREVIOUS, etc, since that won't let  *
		 * us reach it again.  Instead, leave the last traceback element (SINK) off *
		 * the heap.                                                                */

		while (next_ptr != NULL) {
			inode = tptr->index;
			node_to_heap(inode, 0., NO_PREVIOUS, NO_PREVIOUS, OPEN, OPEN);

			if (rr_node[inode].type == IPIN)
				last_ipin_node = inode;

			tptr = next_ptr;
			next_ptr = tptr->next;
		}

		/* This will stop the IPIN node used to get to this SINK from being         *
		 * reexpanded for the remainder of this net's routing.  This will make us   *
		 * hook up more IPINs to this SINK (which is what we want).  If IPIN        *
		 * doglegs are allowed in the graph, we won't be able to use this IPIN to   *
		 * do a dogleg, since it won't be re-expanded.  Shouldn't be a big problem. */

		rr_node_route_inf[last_ipin_node].path_cost = -HUGE_POSITIVE_FLOAT;

		/* Also need to mark the SINK as having high cost, so another connection can *
		 * be made to it.                                                            */

		sink_node = tptr->index;
		rr_node_route_inf[sink_node].path_cost = HUGE_POSITIVE_FLOAT;

		/* Finally, I need to remove any pending connections to this SINK via the    *
		 * IPIN I just used (since they would result in congestion).  Scan through   *
		 * the heap to do this.                                                      */

		invalidate_heap_entries(sink_node, last_ipin_node);
	}
}

static void breadth_first_expand_neighbours(int inode, float pcost, 
		int inet, int itry, float bend_cost) {

	/* Puts all the rr_nodes adjacent to inode on the heap.  rr_nodes outside   *
	 * the expanded bounding box specified in route_bb are not added to the     *
	 * heap.  pcost is the path_cost to get to inode.                           */

	int iconn, to_node, num_edges;
	t_rr_type from_type, to_type;
	float tot_cost;

	num_edges = rr_node[inode].get_num_edges();
	for (iconn = 0; iconn < num_edges; iconn++) {
		to_node = rr_node[inode].edges[iconn];

		if (rr_node[to_node].get_xhigh() < route_bb[inet].xmin
				|| rr_node[to_node].get_xlow() > route_bb[inet].xmax
				|| rr_node[to_node].get_yhigh() < route_bb[inet].ymin
				|| rr_node[to_node].get_ylow() > route_bb[inet].ymax)
			continue; /* Node is outside (expanded) bounding box. */


		tot_cost = pcost + get_rr_cong_cost(to_node);

		if (bend_cost != 0.) {
			from_type = rr_node[inode].type;
			to_type = rr_node[to_node].type;
			if ((from_type == CHANX && to_type == CHANY)
					|| (from_type == CHANY && to_type == CHANX))
				tot_cost += bend_cost;
		}

		node_to_heap(to_node, tot_cost, inode, iconn, OPEN, OPEN);
	}
}

static void breadth_first_add_source_to_heap(int inet) {

	/* Adds the SOURCE of this net to the heap.  Used to start a net's routing. */

	int inode;
	float cost;

	inode = net_rr_terminals[inet][0]; /* SOURCE */
	cost = get_rr_cong_cost(inode);

	node_to_heap(inode, cost, NO_PREVIOUS, NO_PREVIOUS, OPEN, OPEN);
}


