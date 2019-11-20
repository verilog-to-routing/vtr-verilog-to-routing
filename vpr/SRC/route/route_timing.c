#include <stdio.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "route_export.h"
#include "route_common.h"
#include "route_tree_timing.h"
#include "route_timing.h"
#include "heapsort.h"
#include "path_delay.h"
#include "net_delay.h"
#include "stats.h"
#include "ReadOptions.h"

/******************** Subroutines local to route_timing.c ********************/

static int get_max_pins_per_net(void);

static void add_route_tree_to_heap(t_rt_node * rt_node, int target_node,
		float target_criticality, float astar_fac);

static void timing_driven_expand_neighbours(struct s_heap *current, int inet,
		float bend_cost, float criticality_fac, int target_node,
		float astar_fac, int highfanout_rlim);

static float get_timing_driven_expected_cost(int inode, int target_node,
		float criticality_fac, float R_upstream);

static int get_expected_segs_to_target(int inode, int target_node,
		int *num_segs_ortho_dir_ptr);

static void update_rr_base_costs(int inet, float largest_criticality);

static void timing_driven_check_net_delays(float **net_delay);

static int mark_node_expansion_by_bin(int inet, int target_node,
		t_rt_node * rt_node);

/************************ Subroutine definitions *****************************/

boolean try_timing_driven_route(struct s_router_opts router_opts,
		float **net_delay, t_slack * slacks, t_ivec ** clb_opins_used_locally, boolean timing_analysis_enabled) {

	/* Timing-driven routing algorithm.  The timing graph (includes slack)   *
	 * must have already been allocated, and net_delay must have been allocated. *
	 * Returns TRUE if the routing succeeds, FALSE otherwise.                    */

	int itry, inet, ipin, i, bends, wirelength, total_wirelength, available_wirelength, 
		segments, *net_index, *sink_order /* [1..max_pins_per_net-1] */;
	boolean success, is_routable, rip_up_local_opins;
	float *pin_criticality /* [1..max_pins_per_net-1] */, pres_fac, *sinks, 
		critical_path_delay, init_timing_criticality_val;
	t_rt_node **rt_node_of_sink; /* [1..max_pins_per_net-1] */
	clock_t begin,end;
	sinks = (float*)my_malloc(sizeof(float) * num_nets);
	net_index = (int*)my_malloc(sizeof(int) * num_nets);

	for (i = 0; i < num_nets; i++) {
		sinks[i] = clb_net[i].num_sinks;
		net_index[i] = i;
	}
	heapsort(net_index, sinks, num_nets, 1);

	alloc_timing_driven_route_structs(&pin_criticality, &sink_order,
			&rt_node_of_sink);

	/* First do one routing iteration ignoring congestion to	
	get reasonable net delay estimates. Set criticalities to 1 
	when timing analysis is on to optimize timing, and to 0 
	when timing analysis is off to optimize routability. */

	if (timing_analysis_enabled) {
		init_timing_criticality_val = 1.;
	} else {
		init_timing_criticality_val = 0.;
	}

	for (inet = 0; inet < num_nets; inet++) {
		if (clb_net[inet].is_global == FALSE) {
			for (ipin = 1; ipin <= clb_net[inet].num_sinks; ipin++)
				slacks->timing_criticality[inet][ipin] = init_timing_criticality_val;
#ifdef PATH_COUNTING
				slacks->path_criticality[inet][ipin] = init_timing_criticality_val;
#endif		
		} else { 
			/* Set delay of global signals to zero. Non-global net 
			delays are set by update_net_delays_from_route_tree() 
			inside timing_driven_route_net(), which is only called
			for non-global nets. */
			for (ipin = 1; ipin <= clb_net[inet].num_sinks; ipin++) {
				net_delay[inet][ipin] = 0.;
			}
		}
	}

	pres_fac = router_opts.first_iter_pres_fac; /* Typically 0 -> ignore cong. */

	for (itry = 1; itry <= router_opts.max_router_iterations; itry++) {
		begin = clock();
		vpr_printf(TIO_MESSAGE_INFO, "\n");
		vpr_printf(TIO_MESSAGE_INFO, "Routing iteration: %d\n", itry);

		for (i = 0; i < num_nets; i++) {
			inet = net_index[i];
			if (clb_net[inet].is_global == FALSE) { /* Skip global nets. */

				is_routable = timing_driven_route_net(inet, pres_fac,
					router_opts.max_criticality,
					router_opts.criticality_exp, router_opts.astar_fac,
					router_opts.bend_cost, pin_criticality,
					sink_order, rt_node_of_sink, net_delay[inet], slacks);

				/* Impossible to route? (disconnected rr_graph) */

				if (!is_routable) {
					vpr_printf(TIO_MESSAGE_INFO, "Routing failed.\n");
					free_timing_driven_route_structs(pin_criticality,
							sink_order, rt_node_of_sink);
					free(net_index);
					free(sinks);
					return (FALSE);
				}
			}
		}

		if (itry == 1) {
			/* Early exit code for cases where it is obvious that a successful route will not be found 
			 Heuristic: If total wirelength used in first routing iteration is X% of total available wirelength, exit
			 */
			total_wirelength = 0;
			available_wirelength = 0;

			for (i = 0; i < num_rr_nodes; i++) {
				if (rr_node[i].type == CHANX || rr_node[i].type == CHANY) {
					available_wirelength += 1 + rr_node[i].xhigh
							- rr_node[i].xlow + rr_node[i].yhigh
							- rr_node[i].ylow;
				}
			}

			for (inet = 0; inet < num_nets; inet++) {
				if (clb_net[inet].is_global == FALSE
						&& clb_net[inet].num_sinks != 0) { /* Globals don't count. */
					get_num_bends_and_length(inet, &bends, &wirelength,
							&segments);

					total_wirelength += wirelength;
				}
			}
			vpr_printf(TIO_MESSAGE_INFO, "Wire length after first iteration %d, total available wire length %d, ratio %g\n",
					total_wirelength, available_wirelength,
					(float) (total_wirelength) / (float) (available_wirelength));
			if ((float) (total_wirelength) / (float) (available_wirelength)> FIRST_ITER_WIRELENTH_LIMIT) {
				vpr_printf(TIO_MESSAGE_INFO, "Wire length usage ratio exceeds limit of %g, fail routing.\n",
						FIRST_ITER_WIRELENTH_LIMIT);
				free_timing_driven_route_structs(pin_criticality, sink_order,
						rt_node_of_sink);
				free(net_index);
				free(sinks);
				return FALSE;
			}
		}

		/* Make sure any CLB OPINs used up by subblocks being hooked directly     *
		 * to them are reserved for that purpose.                                 */

		if (itry == 1)
			rip_up_local_opins = FALSE;
		else
			rip_up_local_opins = TRUE;

		reserve_locally_used_opins(pres_fac, rip_up_local_opins,
				clb_opins_used_locally);

		/* Pathfinder guys quit after finding a feasible route. I may want to keep *
		 * going longer, trying to improve timing.  Think about this some.         */

		success = feasible_routing();
		if (success) {
			vpr_printf(TIO_MESSAGE_INFO, "Successfully routed after %d routing iterations.\n", itry);
			free_timing_driven_route_structs(pin_criticality, sink_order, rt_node_of_sink);
#ifdef DEBUG
			timing_driven_check_net_delays(net_delay);
#endif
			free(net_index);
			free(sinks);
			return (TRUE);
		}

		if (itry == 1) {
			pres_fac = router_opts.initial_pres_fac;
			pathfinder_update_cost(pres_fac, 0.); /* Acc_fac=0 for first iter. */
		} else {
			pres_fac *= router_opts.pres_fac_mult;

			/* Avoid overflow for high iteration counts, even if acc_cost is big */
			pres_fac = std::min(pres_fac, static_cast<float>(HUGE_POSITIVE_FLOAT / 1e5));

			pathfinder_update_cost(pres_fac, router_opts.acc_fac);
		}

		if (timing_analysis_enabled) {		
			/* Update slack values by doing another timing analysis.                 *
			 * Timing_driven_route_net updated the net delay values.                 */

			load_timing_graph_net_delays(net_delay);

	#ifdef HACK_LUT_PIN_SWAPPING
			do_timing_analysis(slacks, FALSE, TRUE, FALSE);
	#else
			do_timing_analysis(slacks, FALSE, FALSE, FALSE);
	#endif

			/* Print critical path delay - convert to nanoseconds. */
			critical_path_delay = get_critical_path_delay();
			vpr_printf(TIO_MESSAGE_INFO, "Critical path: %g ns\n", critical_path_delay);
		} else {
			/* If timing analysis is not enabled, make sure that the criticalities and the 	*
			 * net_delays stay as 0 so that wirelength can be optimized. 			*/
			
			for (inet = 0; inet < num_nets; inet++) {
				for (ipin = 1; ipin <= clb_net[inet].num_sinks; ipin++) {
					slacks->timing_criticality[inet][ipin] = 0.;
#ifdef PATH_COUNTING 		
					slacks->path_criticality[inet][ipin] = 0.; 		
#endif
					net_delay[inet][ipin] = 0.;
				}
			}
		}
		
		end = clock();
		#ifdef CLOCKS_PER_SEC
			vpr_printf(TIO_MESSAGE_INFO, "Routing iteration took %g seconds.\n", (float)(end - begin) / CLOCKS_PER_SEC);
		#else
			vpr_printf(TIO_MESSAGE_INFO, "Routing iteration took %g seconds.\n", (float)(end - begin) / CLK_PER_SEC);
		#endif
		
		fflush(stdout);
	}

	vpr_printf(TIO_MESSAGE_INFO, "Routing failed.\n");
	free_timing_driven_route_structs(pin_criticality, sink_order,
			rt_node_of_sink);
	free(net_index);
	free(sinks);
	return (FALSE);
}

void alloc_timing_driven_route_structs(float **pin_criticality_ptr,
		int **sink_order_ptr, t_rt_node *** rt_node_of_sink_ptr) {

	/* Allocates all the structures needed only by the timing-driven router.   */

	int max_pins_per_net;
	float *pin_criticality;
	int *sink_order;
	t_rt_node **rt_node_of_sink;

	max_pins_per_net = get_max_pins_per_net();

	pin_criticality = (float *) my_malloc(
			(max_pins_per_net - 1) * sizeof(float));
	*pin_criticality_ptr = pin_criticality - 1; /* First sink is pin #1. */

	sink_order = (int *) my_malloc((max_pins_per_net - 1) * sizeof(int));
	*sink_order_ptr = sink_order - 1;

	rt_node_of_sink = (t_rt_node **) my_malloc(
			(max_pins_per_net - 1) * sizeof(t_rt_node *));
	*rt_node_of_sink_ptr = rt_node_of_sink - 1;

	alloc_route_tree_timing_structs();
}

void free_timing_driven_route_structs(float *pin_criticality, int *sink_order,
		t_rt_node ** rt_node_of_sink) {

	/* Frees all the stuctures needed only by the timing-driven router.        */

	free(pin_criticality + 1); /* Starts at index 1. */
	free(sink_order + 1);
	free(rt_node_of_sink + 1);
	free_route_tree_timing_structs();
}

static int get_max_pins_per_net(void) {

	/* Returns the largest number of pins on any non-global net.    */

	int inet, max_pins_per_net;

	max_pins_per_net = 0;
	for (inet = 0; inet < num_nets; inet++) {
		if (clb_net[inet].is_global == FALSE) {
			max_pins_per_net = std::max(max_pins_per_net,
					(clb_net[inet].num_sinks + 1));
		}
	}

	return (max_pins_per_net);
}

boolean timing_driven_route_net(int inet, float pres_fac, float max_criticality,
		float criticality_exp, float astar_fac, float bend_cost,
		float *pin_criticality, int *sink_order,
		t_rt_node ** rt_node_of_sink, float *net_delay, t_slack * slacks) {

	/* Returns TRUE as long is found some way to hook up this net, even if that *
	 * way resulted in overuse of resources (congestion).  If there is no way   *
	 * to route this net, even ignoring congestion, it returns FALSE.  In this  *
	 * case the rr_graph is disconnected and you can give up. If slacks = NULL, *
	 * give each net a dummy criticality of 0.									*/

	int ipin, num_sinks, itarget, target_pin, target_node, inode;
	float target_criticality, old_tcost, new_tcost, largest_criticality,
		old_back_cost, new_back_cost;
	t_rt_node *rt_root;
	struct s_heap *current;
	struct s_trace *new_route_start_tptr;
	int highfanout_rlim;

	/* Rip-up any old routing. */

	pathfinder_update_one_cost(trace_head[inet], -1, pres_fac);
	free_traceback(inet);
	
	for (ipin = 1; ipin <= clb_net[inet].num_sinks; ipin++) { 
		if (!slacks) {
			/* Use criticality of 1. This makes all nets critical.  Note: There is a big difference between setting pin criticality to 0
			compared to 1.  If pin criticality is set to 0, then the current path delay is completely ignored during routing.  By setting
			pin criticality to 1, the current path delay to the pin will always be considered and optimized for */
			pin_criticality[ipin] = 1.0;
		} else { 
#ifdef PATH_COUNTING
			/* Pin criticality is based on a weighted sum of timing and path criticalities. */	
			pin_criticality[ipin] =		 ROUTE_PATH_WEIGHT	* slacks->path_criticality[inet][ipin]
								  + (1 - ROUTE_PATH_WEIGHT) * slacks->timing_criticality[inet][ipin]; 
#else
			/* Pin criticality is based on only timing criticality. */
			pin_criticality[ipin] = slacks->timing_criticality[inet][ipin];
#endif
			/* Currently, pin criticality is between 0 and 1. Now shift it downwards 
			by 1 - max_criticality (max_criticality is 0.99 by default, so shift down
			by 0.01) and cut off at 0.  This means that all pins with small criticalities 
			(<0.01) get criticality 0 and are ignored entirely, and everything
			else becomes a bit less critical. This effect becomes more pronounced if
			max_criticality is set lower. */
			assert(pin_criticality[ipin] > -0.01 && pin_criticality[ipin] < 1.01);
			pin_criticality[ipin] = std::max(pin_criticality[ipin] - (1.0 - max_criticality), 0.0);

			/* Take pin criticality to some power (1 by default). */
			pin_criticality[ipin] = pow(pin_criticality[ipin], criticality_exp);
			
			/* Cut off pin criticality at max_criticality. */
			pin_criticality[ipin] = std::min(pin_criticality[ipin], max_criticality);
		}
	}

	num_sinks = clb_net[inet].num_sinks;
	heapsort(sink_order, pin_criticality, num_sinks, 0);

	/* Update base costs according to fanout and criticality rules */

	largest_criticality = pin_criticality[sink_order[1]];
	update_rr_base_costs(inet, largest_criticality);

	mark_ends(inet); /* Only needed to check for multiply-connected SINKs */

	rt_root = init_route_tree_to_source(inet);

	for (itarget = 1; itarget <= num_sinks; itarget++) {
		target_pin = sink_order[itarget];
		target_node = net_rr_terminals[inet][target_pin];

		target_criticality = pin_criticality[target_pin];

		highfanout_rlim = mark_node_expansion_by_bin(inet, target_node,
				rt_root);

		add_route_tree_to_heap(rt_root, target_node, target_criticality,
				astar_fac);

		current = get_heap_head();

		if (current == NULL) { /* Infeasible routing.  No possible path for net. */
			vpr_printf(TIO_MESSAGE_INFO, "Cannot route net #%d (%s) to sink #%d -- no possible path.\n",
					   inet, clb_net[inet].name, itarget);
			reset_path_costs();
			free_route_tree(rt_root);
			return (FALSE);
		}

		inode = current->index;

		while (inode != target_node) {
			old_tcost = rr_node_route_inf[inode].path_cost;
			new_tcost = current->cost;

			if (old_tcost > 0.99 * HUGE_POSITIVE_FLOAT) /* First time touched. */
				old_back_cost = HUGE_POSITIVE_FLOAT;
			else
				old_back_cost = rr_node_route_inf[inode].backward_path_cost;

			new_back_cost = current->backward_path_cost;

			/* I only re-expand a node if both the "known" backward cost is lower  *
			 * in the new expansion (this is necessary to prevent loops from       *
			 * forming in the routing and causing havoc) *and* the expected total  *
			 * cost to the sink is lower than the old value.  Different R_upstream *
			 * values could make a path with lower back_path_cost less desirable   *
			 * than one with higher cost.  Test whether or not I should disallow   *
			 * re-expansion based on a higher total cost.                          */

			if (old_tcost > new_tcost && old_back_cost > new_back_cost) {
				rr_node_route_inf[inode].prev_node = current->u.prev_node;
				rr_node_route_inf[inode].prev_edge = current->prev_edge;
				rr_node_route_inf[inode].path_cost = new_tcost;
				rr_node_route_inf[inode].backward_path_cost = new_back_cost;

				if (old_tcost > 0.99 * HUGE_POSITIVE_FLOAT) /* First time touched. */
					add_to_mod_list(&rr_node_route_inf[inode].path_cost);

				timing_driven_expand_neighbours(current, inet, bend_cost,
						target_criticality, target_node, astar_fac,
						highfanout_rlim);
			}

			free_heap_data(current);
			current = get_heap_head();

			if (current == NULL) { /* Impossible routing.  No path for net. */
				vpr_printf(TIO_MESSAGE_INFO, "Cannot route net #%d (%s) to sink #%d -- no possible path.\n",
						 inet, clb_net[inet].name, itarget);
				reset_path_costs();
				free_route_tree(rt_root);
				return (FALSE);
			}

			inode = current->index;
		}

		/* NB:  In the code below I keep two records of the partial routing:  the   *
		 * traceback and the route_tree.  The route_tree enables fast recomputation *
		 * of the Elmore delay to each node in the partial routing.  The traceback  *
		 * lets me reuse all the routines written for breadth-first routing, which  *
		 * all take a traceback structure as input.  Before this routine exits the  *
		 * route_tree structure is destroyed; only the traceback is needed at that  *
		 * point.                                                                   */

		rr_node_route_inf[inode].target_flag--; /* Connected to this SINK. */
		new_route_start_tptr = update_traceback(current, inet);
		rt_node_of_sink[target_pin] = update_route_tree(current);
		free_heap_data(current);
		pathfinder_update_one_cost(new_route_start_tptr, 1, pres_fac);

		empty_heap();
		reset_path_costs();
	}

	/* For later timing analysis. */

	update_net_delays_from_route_tree(net_delay, rt_node_of_sink, inet);
	free_route_tree(rt_root);
	return (TRUE);
}

static void add_route_tree_to_heap(t_rt_node * rt_node, int target_node,
		float target_criticality, float astar_fac) {

	/* Puts the entire partial routing below and including rt_node onto the heap *
	 * (except for those parts marked as not to be expanded) by calling itself   *
	 * recursively.                                                              */

	int inode;
	t_rt_node *child_node;
	t_linked_rt_edge *linked_rt_edge;
	float tot_cost, backward_path_cost, R_upstream;

	/* Pre-order depth-first traversal */

	if (rt_node->re_expand) {
		inode = rt_node->inode;
		backward_path_cost = target_criticality * rt_node->Tdel;
		R_upstream = rt_node->R_upstream;
		tot_cost = backward_path_cost
				+ astar_fac
						* get_timing_driven_expected_cost(inode, target_node,
								target_criticality, R_upstream);
		node_to_heap(inode, tot_cost, NO_PREVIOUS, NO_PREVIOUS,
				backward_path_cost, R_upstream);
	}

	linked_rt_edge = rt_node->u.child_list;

	while (linked_rt_edge != NULL) {
		child_node = linked_rt_edge->child;
		add_route_tree_to_heap(child_node, target_node, target_criticality,
				astar_fac);
		linked_rt_edge = linked_rt_edge->next;
	}
}

static void timing_driven_expand_neighbours(struct s_heap *current, int inet,
		float bend_cost, float criticality_fac, int target_node,
		float astar_fac, int highfanout_rlim) {

	/* Puts all the rr_nodes adjacent to current on the heap.  rr_nodes outside *
	 * the expanded bounding box specified in route_bb are not added to the     *
	 * heap.                                                                    */

	int iconn, to_node, num_edges, inode, iswitch, target_x, target_y;
	t_rr_type from_type, to_type;
	float new_tot_cost, old_back_pcost, new_back_pcost, R_upstream;
	float new_R_upstream, Tdel;

	inode = current->index;
	old_back_pcost = current->backward_path_cost;
	R_upstream = current->R_upstream;
	num_edges = rr_node[inode].num_edges;

	target_x = rr_node[target_node].xhigh;
	target_y = rr_node[target_node].yhigh;

	for (iconn = 0; iconn < num_edges; iconn++) {
		to_node = rr_node[inode].edges[iconn];

		if (rr_node[to_node].xhigh < route_bb[inet].xmin
				|| rr_node[to_node].xlow > route_bb[inet].xmax
				|| rr_node[to_node].yhigh < route_bb[inet].ymin
				|| rr_node[to_node].ylow > route_bb[inet].ymax)
			continue; /* Node is outside (expanded) bounding box. */

		if (clb_net[inet].num_sinks >= HIGH_FANOUT_NET_LIM) {
			if (rr_node[to_node].xhigh < target_x - highfanout_rlim
					|| rr_node[to_node].xlow > target_x + highfanout_rlim
					|| rr_node[to_node].yhigh < target_y - highfanout_rlim
					|| rr_node[to_node].ylow > target_y + highfanout_rlim)
				continue; /* Node is outside high fanout bin. */
		}

		/* Prune away IPINs that lead to blocks other than the target one.  Avoids  *
		 * the issue of how to cost them properly so they don't get expanded before *
		 * more promising routes, but makes route-throughs (via CLBs) impossible.   *
		 * Change this if you want to investigate route-throughs.                   */

		to_type = rr_node[to_node].type;
		if (to_type == IPIN
				&& (rr_node[to_node].xhigh != target_x
						|| rr_node[to_node].yhigh != target_y))
			continue;

		/* new_back_pcost stores the "known" part of the cost to this node -- the   *
		 * congestion cost of all the routing resources back to the existing route  *
		 * plus the known delay of the total path back to the source.  new_tot_cost *
		 * is this "known" backward cost + an expected cost to get to the target.   */

		new_back_pcost = old_back_pcost
				+ (1. - criticality_fac) * get_rr_cong_cost(to_node);

		iswitch = rr_node[inode].switches[iconn];
		if (switch_inf[iswitch].buffered) {
			new_R_upstream = switch_inf[iswitch].R;
		} else {
			new_R_upstream = R_upstream + switch_inf[iswitch].R;
		}

		Tdel = rr_node[to_node].C * (new_R_upstream + 0.5 * rr_node[to_node].R);
		Tdel += switch_inf[iswitch].Tdel;
		new_R_upstream += rr_node[to_node].R;
		new_back_pcost += criticality_fac * Tdel;

		if (bend_cost != 0.) {
			from_type = rr_node[inode].type;
			to_type = rr_node[to_node].type;
			if ((from_type == CHANX && to_type == CHANY)
					|| (from_type == CHANY && to_type == CHANX))
				new_back_pcost += bend_cost;
		}

		new_tot_cost = new_back_pcost
				+ astar_fac
						* get_timing_driven_expected_cost(to_node, target_node,
								criticality_fac, new_R_upstream);

		node_to_heap(to_node, new_tot_cost, inode, iconn, new_back_pcost,
				new_R_upstream);

	} /* End for all neighbours */
}

static float get_timing_driven_expected_cost(int inode, int target_node,
		float criticality_fac, float R_upstream) {

	/* Determines the expected cost (due to both delay and resouce cost) to reach *
	 * the target node from inode.  It doesn't include the cost of inode --       *
	 * that's already in the "known" path_cost.                                   */

	t_rr_type rr_type;
	int cost_index, ortho_cost_index, num_segs_same_dir, num_segs_ortho_dir;
	float expected_cost, cong_cost, Tdel;

	rr_type = rr_node[inode].type;

	if (rr_type == CHANX || rr_type == CHANY) {
		num_segs_same_dir = get_expected_segs_to_target(inode, target_node,
				&num_segs_ortho_dir);
		cost_index = rr_node[inode].cost_index;
		ortho_cost_index = rr_indexed_data[cost_index].ortho_cost_index;

		cong_cost = num_segs_same_dir * rr_indexed_data[cost_index].base_cost
				+ num_segs_ortho_dir
						* rr_indexed_data[ortho_cost_index].base_cost;
		cong_cost += rr_indexed_data[IPIN_COST_INDEX].base_cost
				+ rr_indexed_data[SINK_COST_INDEX].base_cost;

		Tdel =
				num_segs_same_dir * rr_indexed_data[cost_index].T_linear
						+ num_segs_ortho_dir
								* rr_indexed_data[ortho_cost_index].T_linear
						+ num_segs_same_dir * num_segs_same_dir
								* rr_indexed_data[cost_index].T_quadratic
						+ num_segs_ortho_dir * num_segs_ortho_dir
								* rr_indexed_data[ortho_cost_index].T_quadratic
						+ R_upstream
								* (num_segs_same_dir
										* rr_indexed_data[cost_index].C_load
										+ num_segs_ortho_dir
												* rr_indexed_data[ortho_cost_index].C_load);

		Tdel += rr_indexed_data[IPIN_COST_INDEX].T_linear;

		expected_cost = criticality_fac * Tdel
				+ (1. - criticality_fac) * cong_cost;
		return (expected_cost);
	}

	else if (rr_type == IPIN) { /* Change if you're allowing route-throughs */
		return (rr_indexed_data[SINK_COST_INDEX].base_cost);
	}

	else { /* Change this if you want to investigate route-throughs */
		return (0.);
	}
}

/* Macro used below to ensure that fractions are rounded up, but floating   *
 * point values very close to an integer are rounded to that integer.       */

#define ROUND_UP(x) (ceil (x - 0.001))

static int get_expected_segs_to_target(int inode, int target_node,
		int *num_segs_ortho_dir_ptr) {

	/* Returns the number of segments the same type as inode that will be needed *
	 * to reach target_node (not including inode) in each direction (the same    *
	 * direction (horizontal or vertical) as inode and the orthogonal direction).*/

	t_rr_type rr_type;
	int target_x, target_y, num_segs_same_dir, cost_index, ortho_cost_index;
	int no_need_to_pass_by_clb;
	float inv_length, ortho_inv_length, ylow, yhigh, xlow, xhigh;

	target_x = rr_node[target_node].xlow;
	target_y = rr_node[target_node].ylow;
	cost_index = rr_node[inode].cost_index;
	inv_length = rr_indexed_data[cost_index].inv_length;
	ortho_cost_index = rr_indexed_data[cost_index].ortho_cost_index;
	ortho_inv_length = rr_indexed_data[ortho_cost_index].inv_length;
	rr_type = rr_node[inode].type;

	if (rr_type == CHANX) {
		ylow = rr_node[inode].ylow;
		xhigh = rr_node[inode].xhigh;
		xlow = rr_node[inode].xlow;

		/* Count vertical (orthogonal to inode) segs first. */

		if (ylow > target_y) { /* Coming from a row above target? */
			*num_segs_ortho_dir_ptr =
					(int)(ROUND_UP((ylow - target_y + 1.) * ortho_inv_length));
			no_need_to_pass_by_clb = 1;
		} else if (ylow < target_y - 1) { /* Below the CLB bottom? */
			*num_segs_ortho_dir_ptr = (int)(ROUND_UP((target_y - ylow) *
					ortho_inv_length));
			no_need_to_pass_by_clb = 1;
		} else { /* In a row that passes by target CLB */
			*num_segs_ortho_dir_ptr = 0;
			no_need_to_pass_by_clb = 0;
		}

		/* Now count horizontal (same dir. as inode) segs. */

		if (xlow > target_x + no_need_to_pass_by_clb) {
			num_segs_same_dir = (int)(ROUND_UP((xlow - no_need_to_pass_by_clb -
							target_x) * inv_length));
		} else if (xhigh < target_x - no_need_to_pass_by_clb) {
			num_segs_same_dir = (int)(ROUND_UP((target_x - no_need_to_pass_by_clb -
							xhigh) * inv_length));
		} else {
			num_segs_same_dir = 0;
		}
	}

	else { /* inode is a CHANY */
		ylow = rr_node[inode].ylow;
		yhigh = rr_node[inode].yhigh;
		xlow = rr_node[inode].xlow;

		/* Count horizontal (orthogonal to inode) segs first. */

		if (xlow > target_x) { /* Coming from a column right of target? */
			*num_segs_ortho_dir_ptr = (int)(
					ROUND_UP((xlow - target_x + 1.) * ortho_inv_length));
			no_need_to_pass_by_clb = 1;
		} else if (xlow < target_x - 1) { /* Left of and not adjacent to the CLB? */
			*num_segs_ortho_dir_ptr = (int)(ROUND_UP((target_x - xlow) *
					ortho_inv_length));
			no_need_to_pass_by_clb = 1;
		} else { /* In a column that passes by target CLB */
			*num_segs_ortho_dir_ptr = 0;
			no_need_to_pass_by_clb = 0;
		}

		/* Now count vertical (same dir. as inode) segs. */

		if (ylow > target_y + no_need_to_pass_by_clb) {
			num_segs_same_dir = (int)(ROUND_UP((ylow - no_need_to_pass_by_clb -
							target_y) * inv_length));
		} else if (yhigh < target_y - no_need_to_pass_by_clb) {
			num_segs_same_dir = (int)(ROUND_UP((target_y - no_need_to_pass_by_clb -
							yhigh) * inv_length));
		} else {
			num_segs_same_dir = 0;
		}
	}

	return (num_segs_same_dir);
}

static void update_rr_base_costs(int inet, float largest_criticality) {

	/* Changes the base costs of different types of rr_nodes according to the  *
	 * criticality, fanout, etc. of the current net being routed (inet).       */

	float fanout, factor;
	int index;

	fanout = clb_net[inet].num_sinks;

	/* Other reasonable values for factor include fanout and 1 */
	factor = sqrt(fanout);

	for (index = CHANX_COST_INDEX_START; index < num_rr_indexed_data; index++) {
		if (rr_indexed_data[index].T_quadratic > 0.) { /* pass transistor */
			rr_indexed_data[index].base_cost =
					rr_indexed_data[index].saved_base_cost * factor;
		} else {
			rr_indexed_data[index].base_cost =
					rr_indexed_data[index].saved_base_cost;
		}
	}
}

/* Nets that have high fanout can take a very long time to route.  Each sink should be routed contained within a bin instead of the entire bounding box to speed things up */
static int mark_node_expansion_by_bin(int inet, int target_node,
		t_rt_node * rt_node) {
	int target_x, target_y;
	int rlim = 1;
	int inode;
	float area;
	boolean success;
	t_linked_rt_edge *linked_rt_edge;
	t_rt_node * child_node;

	target_x = rr_node[target_node].xlow;
	target_y = rr_node[target_node].ylow;

	if (clb_net[inet].num_sinks < HIGH_FANOUT_NET_LIM) {
		/* This algorithm only applies to high fanout nets */
		return 1;
	}

	area = (route_bb[inet].xmax - route_bb[inet].xmin)
			* (route_bb[inet].ymax - route_bb[inet].ymin);
	if (area <= 0) {
		area = 1;
	}

	rlim = (int)(ceil(sqrt((float) area / (float) clb_net[inet].num_sinks)));
	if (rt_node == NULL || rt_node->u.child_list == NULL) {
		/* If unknown traceback, set radius of bin to be size of chip */
		rlim = std::max(nx + 2, ny + 2);
		return rlim;
	}

	success = FALSE;
	/* determine quickly a feasible bin radius to route sink for high fanout nets 
	 this is necessary to prevent super long runtimes for high fanout nets; in best case, a reduction in complexity from O(N^2logN) to O(NlogN) (Swartz fast router)
	 */
	linked_rt_edge = rt_node->u.child_list;
	while (success == FALSE && linked_rt_edge != NULL) {
		while (linked_rt_edge != NULL && success == FALSE) {
			child_node = linked_rt_edge->child;
			inode = child_node->inode;
			if (!(rr_node[inode].type == IPIN || rr_node[inode].type == SINK)) {
				if (rr_node[inode].xlow <= target_x + rlim
						&& rr_node[inode].xhigh >= target_x - rlim
						&& rr_node[inode].ylow <= target_y + rlim
						&& rr_node[inode].yhigh >= target_y - rlim) {
					success = TRUE;
				}
			}
			linked_rt_edge = linked_rt_edge->next;
		}

		if (success == FALSE) {
			if (rlim > std::max(nx + 2, ny + 2)) {
				vpr_printf(TIO_MESSAGE_ERROR, "VPR internal error, net %s has paths that are not found in traceback.\n",
						clb_net[inet].name);
				exit(1);
			}
			/* if sink not in bin, increase bin size until fit */
			rlim *= 2;
		} else {
			/* Sometimes might just catch a wire in the end segment, need to give it some channel space to explore */
			rlim += 4;
		}
		linked_rt_edge = rt_node->u.child_list;
	}

	/* redetermine expansion based on rlim */
	linked_rt_edge = rt_node->u.child_list;
	while (linked_rt_edge != NULL) {
		child_node = linked_rt_edge->child;
		inode = child_node->inode;
		if (!(rr_node[inode].type == IPIN || rr_node[inode].type == SINK)) {
			if (rr_node[inode].xlow <= target_x + rlim
					&& rr_node[inode].xhigh >= target_x - rlim
					&& rr_node[inode].ylow <= target_y + rlim
					&& rr_node[inode].yhigh >= target_y - rlim) {
				child_node->re_expand = TRUE;
			} else {
				child_node->re_expand = FALSE;
			}
		}
		linked_rt_edge = linked_rt_edge->next;
	}
	return rlim;
}

#define ERROR_TOL 0.0001

static void timing_driven_check_net_delays(float **net_delay) {

	/* Checks that the net delays computed incrementally during timing driven    *
	 * routing match those computed from scratch by the net_delay.c module.      */

	int inet, ipin;
	float **net_delay_check;

	t_chunk list_head_net_delay_check_ch = {NULL, 0, NULL};

	/*struct s_linked_vptr *ch_list_head_net_delay_check;*/

	net_delay_check = alloc_net_delay(&list_head_net_delay_check_ch, clb_net,
			num_nets);
	load_net_delay_from_routing(net_delay_check, clb_net, num_nets);

	for (inet = 0; inet < num_nets; inet++) {
		for (ipin = 1; ipin <= clb_net[inet].num_sinks; ipin++) {
			if (net_delay_check[inet][ipin] == 0.) { /* Should be only GLOBAL nets */
				if (fabs(net_delay[inet][ipin]) > ERROR_TOL) {
					vpr_printf(TIO_MESSAGE_ERROR, "in timing_driven_check_net_delays: net %d pin %d.\n",
							inet, ipin);
					vpr_printf(TIO_MESSAGE_ERROR, "\tIncremental calc. net_delay is %g, but from scratch net delay is %g.\n",
							net_delay[inet][ipin], net_delay_check[inet][ipin]);
					exit(1);
				}
			} else {
				if (fabs(1.0 - net_delay[inet][ipin] / net_delay_check[inet][ipin]) > ERROR_TOL) {
					vpr_printf(TIO_MESSAGE_ERROR, "in timing_driven_check_net_delays: net %d pin %d.\n",
							inet, ipin);
					vpr_printf(TIO_MESSAGE_ERROR, "\tIncremental calc. net_delay is %g, but from scratch net delay is %g.\n",
							net_delay[inet][ipin], net_delay_check[inet][ipin]);
					exit(1);
				}
			}
		}
	}

	free_net_delay(net_delay_check, &list_head_net_delay_check_ch);
	vpr_printf(TIO_MESSAGE_INFO, "Completed net delay value cross check successfully.\n");
}
