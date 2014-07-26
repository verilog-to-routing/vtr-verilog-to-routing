#include <cstdio>
#include <ctime>
#include <cmath>
using namespace std;

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

//===========================================================================//
#include "TCH_PreRoutedHandler.h"

static bool timing_driven_order_prerouted_first(int try_count, 
		struct s_router_opts router_opts, float pres_fac, 
		float* pin_criticality, int* sink_order, 
		t_rt_node** rt_node_of_sink, float** net_delay, t_slack* slacks);
//===========================================================================//

/******************** Subroutines local to route_timing.c ********************/

static int get_max_pins_per_net(void);

static void add_route_tree_to_heap(t_rt_node * rt_node, int target_node,
		float target_criticality, float astar_fac);

static void timing_driven_expand_neighbours(struct s_heap *current, 
		int inet, int itry,
		float bend_cost, float criticality_fac, int target_node,
		float astar_fac, int highfanout_rlim);

static float get_timing_driven_expected_cost(int inode, int target_node,
		float criticality_fac, float R_upstream);

#ifdef INTERPOSER_BASED_ARCHITECTURE
static int get_num_expected_interposer_hops_to_target(int inode, int target_node);
#endif

static int get_expected_segs_to_target(int inode, int target_node,
		int *num_segs_ortho_dir_ptr);

static void update_rr_base_costs(int inet, float largest_criticality);

static void timing_driven_check_net_delays(float **net_delay);

static int mark_node_expansion_by_bin(int inet, int target_node,
		t_rt_node * rt_node);

static double get_overused_ratio();


/************************ Subroutine definitions *****************************/

boolean try_timing_driven_route(struct s_router_opts router_opts,
		float **net_delay, t_slack * slacks, t_ivec ** clb_opins_used_locally, 
		boolean timing_analysis_enabled) {

	/* Timing-driven routing algorithm.  The timing graph (includes slack)   *
	 * must have already been allocated, and net_delay must have been allocated. *
	 * Returns TRUE if the routing succeeds, FALSE otherwise.                    */

	float *sinks = (float*)my_malloc(sizeof(float) * g_clbs_nlist.net.size());
	int *net_index = (int*)my_malloc(sizeof(int) * g_clbs_nlist.net.size());
	for (unsigned int i = 0; i < g_clbs_nlist.net.size(); ++i) {
		sinks[i] = g_clbs_nlist.net[i].num_sinks();
		net_index[i] = i;
	}
	heapsort(net_index, sinks, (int) g_clbs_nlist.net.size(), 1);

	float *pin_criticality; /* [1..max_pins_per_net-1] */
	int *sink_order; /* [1..max_pins_per_net-1] */;
	t_rt_node **rt_node_of_sink; /* [1..max_pins_per_net-1] */
	alloc_timing_driven_route_structs(&pin_criticality, &sink_order, &rt_node_of_sink);

	/* First do one routing iteration ignoring congestion to get reasonable net 
	   delay estimates. Set criticalities to 1 when timing analysis is on to 
	   optimize timing, and to 0 when timing analysis is off to optimize routability. */

	float init_timing_criticality_val = (timing_analysis_enabled ? 1.0 : 0.0);

	/* Variables used to do the optimization of the routing, aborting visibly
	 * impossible Ws */
	double overused_ratio;
	double *historical_overuse_ratio = (double*)my_calloc(router_opts.max_router_iterations + 1, sizeof(double));
	/* Threshold defined according to command line argument --routing_failure_predictor */
	double overused_threshold = ROUTING_PREDICTOR_SAFE; /* Default */
	if(router_opts.routing_failure_predictor == AGGRESSIVE)
		overused_threshold = ROUTING_PREDICTOR_AGGRESSIVE;
	if(router_opts.routing_failure_predictor == OFF)
		overused_threshold = ROUTING_PREDICTOR_OFF;

	int times_exceeded_threshold = 0;

	for (unsigned int inet = 0; inet < g_clbs_nlist.net.size(); ++inet) {
		if (g_clbs_nlist.net[inet].is_global == FALSE) {
			for (unsigned int ipin = 1; ipin < g_clbs_nlist.net[inet].pins.size(); ++ipin) {
				slacks->timing_criticality[inet][ipin] = init_timing_criticality_val;
			}
#ifdef PATH_COUNTING
				slacks->path_criticality[inet][ipin] = init_timing_criticality_val;
#endif		
		} else { 
			/* Set delay of global signals to zero. Non-global net delays are set by
			   update_net_delays_from_route_tree() inside timing_driven_route_net(), 
			   which is only called for non-global nets. */
			for (unsigned int ipin = 1; ipin < g_clbs_nlist.net[inet].pins.size(); ++ipin) {
				net_delay[inet][ipin] = 0.;
			}
		}
	}

	float pres_fac = router_opts.first_iter_pres_fac; /* Typically 0 -> ignore cong. */

	for (int itry = 1; itry <= router_opts.max_router_iterations; ++itry) {
		clock_t begin = clock();

		/* Reset "is_routed" and "is_fixed" flags to indicate nets not pre-routed (yet) */
		for (unsigned int inet = 0; inet < g_clbs_nlist.net.size(); ++inet) {
			g_clbs_nlist.net[inet].is_routed = FALSE;
			g_clbs_nlist.net[inet].is_fixed = FALSE;
		}

		timing_driven_order_prerouted_first(itry, router_opts, pres_fac, 
				pin_criticality, sink_order, 
				rt_node_of_sink, net_delay, slacks);

		for (unsigned int i = 0; i < g_clbs_nlist.net.size(); ++i) {
			int inet = net_index[i];
			boolean is_routable = try_timing_driven_route_net(inet, itry, pres_fac,
					router_opts, pin_criticality, sink_order, 
					rt_node_of_sink, net_delay, slacks);
			if (!is_routable) {
				free(net_index);
				free(sinks);
				free(historical_overuse_ratio);
				return (FALSE);
			}
		}

		clock_t end = clock();
		#ifdef CLOCKS_PER_SEC
			float time = static_cast<float>(end - begin) / CLOCKS_PER_SEC;
		#else
			float time = static_cast<float>(end - begin) / CLK_PER_SEC;
		#endif

		if (itry == 1) {
			/* Early exit code for cases where it is obvious that a successful route will not be found 
			   Heuristic: If total wirelength used in first routing iteration is X% of total available wirelength, exit */
			int total_wirelength = 0;
			int available_wirelength = 0;

			for (int i = 0; i < num_rr_nodes; ++i) {
				if (rr_node[i].type == CHANX || rr_node[i].type == CHANY) {
					available_wirelength += 1 + 
							rr_node[i].xhigh - rr_node[i].xlow + 
							rr_node[i].yhigh - rr_node[i].ylow;
				}
			}

			for (unsigned int inet = 0; inet < g_clbs_nlist.net.size(); ++inet) {
				if (g_clbs_nlist.net[inet].is_global == FALSE
						&& g_clbs_nlist.net[inet].num_sinks() != 0) { /* Globals don't count. */
					int bends, wirelength, segments;
					get_num_bends_and_length(inet, &bends, &wirelength, &segments);

					total_wirelength += wirelength;
				}
			}
			vpr_printf_info("Wire length after first iteration %d, total available wire length %d, ratio %g\n",
					total_wirelength, available_wirelength,
					(float) (total_wirelength) / (float) (available_wirelength));
			if ((float) (total_wirelength) / (float) (available_wirelength)> FIRST_ITER_WIRELENTH_LIMIT) {
				vpr_printf_info("Wire length usage ratio exceeds limit of %g, fail routing.\n",
						FIRST_ITER_WIRELENTH_LIMIT);
				free_timing_driven_route_structs(pin_criticality, sink_order, rt_node_of_sink);
				free(net_index);
				free(sinks);
				free(historical_overuse_ratio);
				return FALSE;
			}
			vpr_printf_info("--------- ---------- ----------- ---------------------\n");
			vpr_printf_info("Iteration       Time   Crit Path     Overused RR Nodes\n");
			vpr_printf_info("--------- ---------- ----------- ---------------------\n");
		}

		/* Make sure any CLB OPINs used up by subblocks being hooked directly
		   to them are reserved for that purpose. */

		boolean rip_up_local_opins = (itry == 1 ? FALSE : TRUE);
		reserve_locally_used_opins(pres_fac, rip_up_local_opins, clb_opins_used_locally);

		/* Pathfinder guys quit after finding a feasible route. I may want to keep 
		   going longer, trying to improve timing.  Think about this some. */

		boolean success = feasible_routing();

		/* Verification to check the ratio of overused nodes, depending on the configuration
		 * may abort the routing if the ratio is too high. */
		overused_ratio = get_overused_ratio();
		historical_overuse_ratio[itry] = overused_ratio;

		/* Andre Pereira: The check splits the inverval in 3 intervals ([6,10), [10,20), [20,40)
		 * The values before 6 are not considered, as the behaviour is not interesting
		 * The threshold used is 4x, 2x, 1x overused_threshold, for each interval,
		 * respectively.
		 * The routing must exceed the threshold EXCEEDED_OVERUSED_COUNT_LIMIT (4, but might change)
		 * times in order to abort the routing */
		/* TODO: Add different configurations for the threshold: disabled, moderate, agressive */
		if (itry > 5){
			
			int routing_predictor_running_average = MAX_SHORT;
			if(historical_overuse_ratio[1] > 0 && historical_overuse_ratio[itry] > 0) {
				routing_predictor_running_average = ROUTING_PREDICTOR_RUNNING_AVERAGE_BASE + (historical_overuse_ratio[1] / historical_overuse_ratio[itry]) / 4;
			}

			/* Using double comparisons here, but just for inequality and they are coarse,
			 * I don't think tolerance needs to be used */
			if (itry < 10 && overused_ratio >= 4.0*overused_threshold)
				times_exceeded_threshold++;
			if (itry >= 10 && itry < 20 && overused_ratio >= 2.0*overused_threshold)
				times_exceeded_threshold++;
			if (itry >= 20 && overused_ratio > overused_threshold)
				times_exceeded_threshold++;

			//printf("jedit overused_ratio %g\n", overused_ratio);
			if (times_exceeded_threshold >= EXCEEDED_OVERUSED_COUNT_LIMIT) {
				vpr_printf_info("Routing aborted, the ratio of overused nodes is above the threshold.\n");
				free_timing_driven_route_structs(pin_criticality, sink_order, rt_node_of_sink);
				free(net_index);
				free(sinks);
				free(historical_overuse_ratio);
				return (FALSE);
			}
			if(itry > routing_predictor_running_average && router_opts.routing_failure_predictor == AGGRESSIVE) {
				/* linear extrapolation to predict what routing iteration will succeed */
				int expected_successful_route_iter;
				double removal_average = 0;
				for(int iavg = 0; iavg < routing_predictor_running_average; iavg++) {
					double first = historical_overuse_ratio[itry - routing_predictor_running_average + iavg];
					double second = historical_overuse_ratio[itry - routing_predictor_running_average + iavg + 1];
					removal_average += ((first - second) / routing_predictor_running_average);
				}
				expected_successful_route_iter = itry + historical_overuse_ratio[itry] / removal_average;
				//printf("jedit expected_successful_route_iter %d\n", expected_successful_route_iter);
				if (expected_successful_route_iter > 1.25 * router_opts.max_router_iterations || removal_average <= 0) {
					vpr_printf_info("Routing aborted, the predicted iteration for a successful route is too high.\n");
					free_timing_driven_route_structs(pin_criticality, sink_order, rt_node_of_sink);
					free(net_index);
					free(sinks);
					free(historical_overuse_ratio);
					return (FALSE);
				}
			}
		}


		if (success) {

			if (timing_analysis_enabled) {
				load_timing_graph_net_delays(net_delay);
				do_timing_analysis(slacks, FALSE, FALSE, FALSE);
				float critical_path_delay = get_critical_path_delay();
                vpr_printf_info("%9d %6.2f sec %8.5f ns   %3.2e (%3.4f %)\n", itry, time, critical_path_delay, overused_ratio*num_rr_nodes, overused_ratio*100);
				vpr_printf_info("Critical path: %g ns\n", critical_path_delay);
			} else {
                vpr_printf_info("%9d %6.2f sec         N/A   %3.2e (%3.4f %)\n", itry, time, overused_ratio*num_rr_nodes, overused_ratio*100);
			}

			vpr_printf_info("Successfully routed after %d routing iterations.\n", itry);
			free_timing_driven_route_structs(pin_criticality, sink_order, rt_node_of_sink);
#ifdef DEBUG
			timing_driven_check_net_delays(net_delay);
#endif
			free(net_index);
			free(sinks);
			free(historical_overuse_ratio);
			return (TRUE);
		}

		if (itry == 1) {
			pres_fac = router_opts.initial_pres_fac;
			pathfinder_update_cost(pres_fac, 0.); /* Acc_fac=0 for first iter. */
		} else {
			pres_fac *= router_opts.pres_fac_mult;

			/* Avoid overflow for high iteration counts, even if acc_cost is big */
			pres_fac = min(pres_fac, static_cast<float>(HUGE_POSITIVE_FLOAT / 1e5));

			pathfinder_update_cost(pres_fac, router_opts.acc_fac);
		}

		if (timing_analysis_enabled) {		
			/* Update slack values by doing another timing analysis.
			   Timing_driven_route_net updated the net delay values. */

			load_timing_graph_net_delays(net_delay);

	#ifdef HACK_LUT_PIN_SWAPPING
			do_timing_analysis(slacks, FALSE, TRUE, FALSE);
	#else
			do_timing_analysis(slacks, FALSE, FALSE, FALSE);
	#endif

		} else {
			/* If timing analysis is not enabled, make sure that the criticalities and the
			   net_delays stay as 0 so that wirelength can be optimized. */
			
			for (unsigned int inet = 0; inet < g_clbs_nlist.net.size(); ++inet) {
				for (unsigned int ipin = 1; ipin < g_clbs_nlist.net[inet].pins.size(); ++ipin) {
					slacks->timing_criticality[inet][ipin] = 0.;
#ifdef PATH_COUNTING 		
					slacks->path_criticality[inet][ipin] = 0.; 		
#endif
					net_delay[inet][ipin] = 0.;
				}
			}
		}

		
		if (timing_analysis_enabled) {
			float critical_path_delay = get_critical_path_delay();
            vpr_printf_info("%9d %6.2f sec %8.5f ns   %3.2e (%3.4f %)\n", itry, time, critical_path_delay, overused_ratio*num_rr_nodes, overused_ratio*100);
		} else {
            vpr_printf_info("%9d %6.2f sec         N/A   %3.2e (%3.4f %)\n", itry, time, overused_ratio*num_rr_nodes, overused_ratio*100);
		}
		fflush(stdout);
	}

	vpr_printf_info("Routing failed.\n");
	free_timing_driven_route_structs(pin_criticality, sink_order, rt_node_of_sink);
	free(net_index);
	free(sinks);
	free(historical_overuse_ratio);
	return (FALSE);
}

boolean try_timing_driven_route_net(int inet, int itry, float pres_fac, 
		struct s_router_opts router_opts,
		float* pin_criticality, int* sink_order,
		t_rt_node** rt_node_of_sink, float** net_delay, t_slack* slacks) {

	boolean is_routed = FALSE;

	if (g_clbs_nlist.net[inet].is_fixed) { /* Skip pre-routed nets. */

		is_routed = TRUE;

	} else if (g_clbs_nlist.net[inet].is_global) { /* Skip global nets. */

		is_routed = TRUE;

	} else {

		is_routed = timing_driven_route_net(inet, itry, pres_fac,
				router_opts.max_criticality, router_opts.criticality_exp, 
				router_opts.astar_fac, router_opts.bend_cost, 
				pin_criticality, sink_order, 
				rt_node_of_sink, net_delay[inet], slacks);

		/* Impossible to route? (disconnected rr_graph) */
		if (is_routed) {
			g_clbs_nlist.net[inet].is_routed = TRUE;
			g_atoms_nlist.net[clb_to_vpack_net_mapping[inet]].is_routed = TRUE;
		} else {
			vpr_printf_info("Routing failed.\n");
			free_timing_driven_route_structs(pin_criticality,
					sink_order, rt_node_of_sink);
		}
	}
	return (is_routed);
}

void alloc_timing_driven_route_structs(float **pin_criticality_ptr,
		int **sink_order_ptr, t_rt_node *** rt_node_of_sink_ptr) {

	/* Allocates all the structures needed only by the timing-driven router.   */

	int max_pins_per_net = get_max_pins_per_net();

	float *pin_criticality = (float *) my_malloc((max_pins_per_net - 1) * sizeof(float));
	*pin_criticality_ptr = pin_criticality - 1; /* First sink is pin #1. */

	int *sink_order = (int *) my_malloc((max_pins_per_net - 1) * sizeof(int));
	*sink_order_ptr = sink_order - 1;

	t_rt_node **rt_node_of_sink = (t_rt_node **) my_malloc((max_pins_per_net - 1) * sizeof(t_rt_node *));
	*rt_node_of_sink_ptr = rt_node_of_sink - 1;

	alloc_route_tree_timing_structs();
}

/* This function gets ratio of overused nodes (overused_nodes / num_rr_nodes) */
static double get_overused_ratio(){
	double overused_nodes = 0.0;
	int inode;
	for(inode = 0; inode < num_rr_nodes; inode++){
		if(rr_node[inode].occ > rr_node[inode].capacity)
			overused_nodes += (rr_node[inode].occ - rr_node[inode].capacity);
	}
	overused_nodes /= (double)num_rr_nodes;
	return overused_nodes;
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

	unsigned int inet;
	int max_pins_per_net;

	max_pins_per_net = 0;
	for (inet = 0; inet < g_clbs_nlist.net.size(); inet++) {
		if (g_clbs_nlist.net[inet].is_global == FALSE) {
			max_pins_per_net = max(max_pins_per_net,
					(int) g_clbs_nlist.net[inet].pins.size());
		}
	}

	return (max_pins_per_net);
}

boolean timing_driven_route_net(int inet, int itry, float pres_fac, float max_criticality,
		float criticality_exp, float astar_fac, float bend_cost,
		float *pin_criticality, int *sink_order,
		t_rt_node ** rt_node_of_sink, float *net_delay, t_slack * slacks) {

	/* Returns TRUE as long is found some way to hook up this net, even if that *
	 * way resulted in overuse of resources (congestion).  If there is no way   *
	 * to route this net, even ignoring congestion, it returns FALSE.  In this  *
	 * case the rr_graph is disconnected and you can give up. If slacks = NULL, *
	 * give each net a dummy criticality of 0.									*/

	unsigned int ipin;
	int num_sinks, itarget, target_pin, target_node, inode;
	float target_criticality, old_tcost, new_tcost, largest_criticality,
		old_back_cost, new_back_cost;
	t_rt_node *rt_root;
	struct s_heap *current;
	struct s_trace *new_route_start_tptr;
	int highfanout_rlim;

	/* Rip-up any old routing. */

	pathfinder_update_one_cost(trace_head[inet], -1, pres_fac);
	free_traceback(inet);
	
	for (ipin = 1; ipin < g_clbs_nlist.net[inet].pins.size(); ipin++) { 
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
			pin_criticality[ipin] = max(pin_criticality[ipin] - (1.0 - max_criticality), 0.0);

			/* Take pin criticality to some power (1 by default). */
			pin_criticality[ipin] = pow(pin_criticality[ipin], criticality_exp);
			
			/* Cut off pin criticality at max_criticality. */
			pin_criticality[ipin] = min(pin_criticality[ipin], max_criticality);
		}
	}

	num_sinks = g_clbs_nlist.net[inet].num_sinks();
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
			vpr_printf_info("Cannot route net #%d (%s) to sink #%d -- no possible path.\n",
					   inet, g_clbs_nlist.net[inet].name, itarget);
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
				timing_driven_expand_neighbours(current, inet, itry, bend_cost,
						target_criticality, target_node, astar_fac,
						highfanout_rlim);
			}

			free_heap_data(current);
			current = get_heap_head();

			if (current == NULL) { /* Impossible routing.  No path for net. */
				vpr_printf_info("Cannot route net #%d (%s) to sink #%d -- no possible path.\n",
						 inet, g_clbs_nlist.net[inet].name, itarget);
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

static void timing_driven_expand_neighbours(struct s_heap *current, 
		int inet, int itry,
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

		int src_node = net_rr_terminals[inet][0];
		int sink_node = target_node;
		int from_node = inode;
		if (restrict_prerouted_path(inet, itry, src_node, sink_node, from_node, to_node))
			continue;

		if (g_clbs_nlist.net[inet].num_sinks() >= HIGH_FANOUT_NET_LIM) {
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

		float expected_cost = get_timing_driven_expected_cost(to_node, target_node,
				criticality_fac, new_R_upstream);
		new_tot_cost = new_back_pcost + astar_fac * expected_cost;

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

#ifdef INTERPOSER_BASED_ARCHITECTURE		
		int num_interposer_hops = get_num_expected_interposer_hops_to_target(inode, target_node);
#endif

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

#ifdef INTERPOSER_BASED_ARCHITECTURE
		float interposer_hop_delay = (float)delay_increase * 1e-12;
		Tdel += num_interposer_hops * interposer_hop_delay;
#endif

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

#ifdef INTERPOSER_BASED_ARCHITECTURE
static int get_num_expected_interposer_hops_to_target(int inode, int target_node) 
{
	/* 
	Description:
		returns the expected number of times you have to cross the 
		interposer in order to go from inode to target_node.
		This does not include inode itself.
	
	Assumptions: 
		1- Cuts are horizontal
		2- target_node is a terminal pin (hence its ylow==yhigh)
		3- wires that go through the interposer are uni-directional

		---------	y=150
		|		|
		---------	y=100
		|		|
		---------	y=50
		|		|
		---------	y=0

		num_cuts = 2, cut_step = 50, cut_locations = {50, 100}
	*/
	int y_start; /* start point y-coordinate. (y-coordinate at the *end* of wire 'i') */
	int y_end;   /* destination (target) y-coordinate */
	int cut_i, y_cut_location, num_expected_hops;
	t_rr_type rr_type;

	num_expected_hops = 0;
	y_end   = rr_node[target_node].ylow; 
	rr_type = rr_node[inode].type;

	if(rr_type == CHANX) 
	{	/* inode is a horizontal wire */
		/* the ylow and yhigh are the same */
		assert(rr_node[inode].ylow == rr_node[inode].yhigh);
		y_start = rr_node[inode].ylow;
	}
	else
	{	/* inode is a CHANY */
		if(rr_node[inode].direction == INC_DIRECTION)
		{
			y_start = rr_node[inode].yhigh;
		}
		else if(rr_node[inode].direction == DEC_DIRECTION)
		{
			y_start = rr_node[inode].ylow;
		}
		else
		{
			// interposer wires can be bidirectional
			y_start = rr_node[inode].ylow;
		}
	}

	/* for every cut, is it between 'i' and 'target'? */
	for(cut_i=0 ; cut_i<num_cuts; ++cut_i) 
	{
		y_cut_location = arch_cut_locations[cut_i];
		if( (y_start < y_cut_location &&  y_cut_location < y_end) ||
			(y_end   < y_cut_location &&  y_cut_location < y_start)) 
		{
			++num_expected_hops;
		}
	}

	/* Make there is no off-by-1 error.For current node i: 
	   if it's a vertical wire, node 'i' itself may be crossing the interposer.
	*/
	if(rr_type == CHANY)
	{	
		/* for every cut, does it cut wire 'i'? */
		for(cut_i=0 ; cut_i<num_cuts; ++cut_i) 
		{
			y_cut_location = arch_cut_locations[cut_i];
			if(rr_node[inode].ylow < y_cut_location && y_cut_location < rr_node[inode].yhigh)
			{
				++num_expected_hops;
			}
		}
	}

	return num_expected_hops;
}
#endif

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

	#ifdef INTERPOSER_BASED_ARCHITECTURE
	int num_expected_hops = get_num_expected_interposer_hops_to_target(inode, target_node);
	#endif
	
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

#ifdef INTERPOSER_BASED_ARCHITECTURE		
		(*num_segs_ortho_dir_ptr) = (*num_segs_ortho_dir_ptr) + 2*num_expected_hops;
#endif
		
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
		
#ifdef INTERPOSER_BASED_ARCHITECTURE		
		num_segs_same_dir = num_segs_same_dir + 2*num_expected_hops;
#endif
	}

	return (num_segs_same_dir);
}

static void update_rr_base_costs(int inet, float largest_criticality) {

	/* Changes the base costs of different types of rr_nodes according to the  *
	 * criticality, fanout, etc. of the current net being routed (inet).       */

	float fanout, factor;
	int index;

	fanout = g_clbs_nlist.net[inet].num_sinks();

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

	if (g_clbs_nlist.net[inet].num_sinks() < HIGH_FANOUT_NET_LIM) {
		/* This algorithm only applies to high fanout nets */
		return 1;
	}

	area = (route_bb[inet].xmax - route_bb[inet].xmin)
			* (route_bb[inet].ymax - route_bb[inet].ymin);
	if (area <= 0) {
		area = 1;
	}

	rlim = (int)(ceil(sqrt((float) area / (float) g_clbs_nlist.net[inet].num_sinks())));
	if (rt_node == NULL || rt_node->u.child_list == NULL) {
		/* If unknown traceback, set radius of bin to be size of chip */
		rlim = max(nx + 2, ny + 2);
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
			if (rlim > max(nx + 2, ny + 2)) {
				vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
					 "VPR internal error, net %s has paths that are not found in traceback.\n", g_clbs_nlist.net[inet].name);
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

	unsigned int inet, ipin;
	float **net_delay_check;

	t_chunk list_head_net_delay_check_ch = {NULL, 0, NULL};

	/*struct s_linked_vptr *ch_list_head_net_delay_check;*/

	net_delay_check = alloc_net_delay(&list_head_net_delay_check_ch, g_clbs_nlist.net,
		g_clbs_nlist.net.size());
	load_net_delay_from_routing(net_delay_check, g_clbs_nlist.net, g_clbs_nlist.net.size());

	for (inet = 0; inet < g_clbs_nlist.net.size(); inet++) {
		for (ipin = 1; ipin < g_clbs_nlist.net[inet].pins.size(); ipin++) {
			if (net_delay_check[inet][ipin] == 0.) { /* Should be only GLOBAL nets */
				if (fabs(net_delay[inet][ipin]) > ERROR_TOL) {
					vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
						"in timing_driven_check_net_delays: net %d pin %d.\n"
						"\tIncremental calc. net_delay is %g, but from scratch net delay is %g.\n",
						inet, ipin, net_delay[inet][ipin], net_delay_check[inet][ipin]);
				}
			} else {
				if (fabs(1.0 - net_delay[inet][ipin] / net_delay_check[inet][ipin]) > ERROR_TOL) {
					vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
						"in timing_driven_check_net_delays: net %d pin %d.\n"
						"\tIncremental calc. net_delay is %g, but from scratch net delay is %g.\n",
						inet, ipin, net_delay[inet][ipin], net_delay_check[inet][ipin]);
				}
			}
		}
	}

	free_net_delay(net_delay_check, &list_head_net_delay_check_ch);
	vpr_printf_info("Completed net delay value cross check successfully.\n");
}

//===========================================================================//
static bool timing_driven_order_prerouted_first(
		int try_count, 
		struct s_router_opts router_opts, 
		float pres_fac,
		float* pin_criticality,	int* sink_order,
		t_rt_node** rt_node_of_sink, 
		float** net_delay, t_slack* slacks ) {

	bool ok= TRUE;

	// Verify at least one pre-routed constraint has been defined
	TCH_PreRoutedHandler_c& preRoutedHandler = TCH_PreRoutedHandler_c::GetInstance();
	if (preRoutedHandler.IsValid() &&
		(preRoutedHandler.GetOrderMode() == TCH_ROUTE_ORDER_FIRST)) {

		const TCH_NetList_t& tch_netList = preRoutedHandler.GetNetList();
		for (size_t i = 0; i < tch_netList.GetLength(); ++i )
		{
			const TCH_Net_c& tch_net = *tch_netList[i];

			if (tch_net.GetStatus() == TCH_ROUTE_STATUS_FLOAT)
				continue;

			if ((tch_net.GetStatus() == TCH_ROUTE_STATUS_ROUTED) && (try_count > 1))
				continue;

			int inet = tch_net.GetVPR_NetIndex();
			vpr_printf_info("  Prerouting net %s...\n", g_clbs_nlist.net[inet].name);

			// Call existing VPR route code based on the given VPR net index
			// (Note: this code will auto pre-route based on Toro callback handler)
			bool is_routable = try_timing_driven_route_net(inet, try_count, pres_fac,
								router_opts, 
								pin_criticality, sink_order, 
								rt_node_of_sink, 
								net_delay, slacks);
			if (!is_routable) {
				ok = false;
				break;
			}

			// Force net's "is_routed" or "is_fixed" state to TRUE 
			// (ie. indicate that net has been pre-routed)
			if (tch_net.GetStatus() == TCH_ROUTE_STATUS_ROUTED) {
				g_clbs_nlist.net[inet].is_routed = TRUE;
				g_atoms_nlist.net[clb_to_vpack_net_mapping[inet]].is_routed = TRUE;
			} else if (tch_net.GetStatus() == TCH_ROUTE_STATUS_FIXED) {
				g_clbs_nlist.net[inet].is_fixed = TRUE;
				g_atoms_nlist.net[clb_to_vpack_net_mapping[inet]].is_fixed = TRUE;
			}
		}
	}
	return (ok);
}
//===========================================================================//
