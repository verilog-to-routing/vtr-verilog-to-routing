#include <stdio.h>
#include <math.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "mst.h"
#include "route_export.h"
#include "route_common.h"
#include "route_tree_timing.h"
#include "route_timing.h"
#include "heapsort.h"
#include "path_delay.h"
#include "net_delay.h"


/******************** Subroutines local to route_timing.c ********************/

static int get_max_pins_per_net(void);

static void add_route_tree_to_heap(t_rt_node * rt_node,
				   int target_node,
				   float target_criticality,
				   float astar_fac);

static void timing_driven_expand_neighbours(struct s_heap *current,
					    int inet,
					    float bend_cost,
					    float criticality_fac,
					    int target_node,
					    float astar_fac);

static float get_timing_driven_expected_cost(int inode,
					     int target_node,
					     float criticality_fac,
					     float R_upstream);

static int get_expected_segs_to_target(int inode,
				       int target_node,
				       int *num_segs_ortho_dir_ptr);

static void update_rr_base_costs(int inet,
				 float largest_criticality);

static void timing_driven_check_net_delays(float **net_delay);


/************************ Subroutine definitions *****************************/

boolean
try_timing_driven_route(struct s_router_opts router_opts,
			float **net_slack,
			float **net_delay,
			t_ivec ** fb_opins_used_locally)
{

/* Timing-driven routing algorithm.  The timing graph (includes net_slack)   *
 * must have already been allocated, and net_delay must have been allocated. *
 * Returns TRUE if the routing succeeds, FALSE otherwise.                    */

    int itry, inet, ipin;
    boolean success, is_routable, rip_up_local_opins;
    float *pin_criticality;	/* [1..max_pins_per_net-1]. */
    int *sink_order;		/* [1..max_pins_per_net-1]. */
    t_rt_node **rt_node_of_sink;	/* [1..max_pins_per_net-1]. */
    float T_crit, pres_fac;

    alloc_timing_driven_route_structs(&pin_criticality, &sink_order,
				      &rt_node_of_sink);

/* First do one routing iteration ignoring congestion and marking all sinks  *
 * on each net as critical to get reasonable net delay estimates.            */

    for(inet = 0; inet < num_nets; inet++)
	{
	    if(net[inet].is_global == FALSE)
		{
		    for(ipin = 1; ipin <= net[inet].num_sinks; ipin++)
			net_slack[inet][ipin] = 0.;
		}
	    else
		{		/* Set delay of global signals to zero. */
		    for(ipin = 1; ipin <= net[inet].num_sinks; ipin++)
			net_delay[inet][ipin] = 0.;
		}
	}

    T_crit = 1.;
    pres_fac = router_opts.first_iter_pres_fac;	/* Typically 0 -> ignore cong. */

    for(itry = 1; itry <= router_opts.max_router_iterations; itry++)
	{

	    for(inet = 0; inet < num_nets; inet++)
		{
		    if(net[inet].is_global == FALSE)
			{	/* Skip global nets. */

			    is_routable =
				timing_driven_route_net(inet, pres_fac,
							router_opts.
							max_criticality,
							router_opts.
							criticality_exp,
							router_opts.astar_fac,
							router_opts.bend_cost,
							net_slack[inet],
							pin_criticality,
							sink_order,
							rt_node_of_sink,
							T_crit,
							net_delay[inet]);

			    /* Impossible to route? (disconnected rr_graph) */

			    if(!is_routable)
				{
				    printf("Routing failed.\n");
				    free_timing_driven_route_structs
					(pin_criticality, sink_order,
					 rt_node_of_sink);
				    return (FALSE);
				}
			}
		}

	    /* Make sure any FB OPINs used up by subblocks being hooked directly     *
	     * to them are reserved for that purpose.                                 */

	    if(itry == 1)
		rip_up_local_opins = FALSE;
	    else
		rip_up_local_opins = TRUE;

	    reserve_locally_used_opins(pres_fac, rip_up_local_opins,
				       fb_opins_used_locally);

	    /* Pathfinder guys quit after finding a feasible route. I may want to keep *
	     * going longer, trying to improve timing.  Think about this some.         */

	    success = feasible_routing();
	    if(success)
		{
		    printf
			("Successfully routed after %d routing iterations.\n",
			 itry);
		    free_timing_driven_route_structs(pin_criticality,
						     sink_order,
						     rt_node_of_sink);
#ifdef DEBUG
		    timing_driven_check_net_delays(net_delay);
#endif
		    return (TRUE);
		}

	    if(itry == 1)
		{
		    pres_fac = router_opts.initial_pres_fac;
		    pathfinder_update_cost(pres_fac, 0.);	/* Acc_fac=0 for first iter. */
		}
	    else
		{
		    pres_fac *= router_opts.pres_fac_mult;

			/* Avoid overflow for high iteration counts, even if acc_cost is big */
			pres_fac = min (pres_fac, HUGE_FLOAT / 1e5);

		    pathfinder_update_cost(pres_fac, router_opts.acc_fac);
		}

	    /* Update slack values by doing another timing analysis.                 *
	     * Timing_driven_route_net updated the net delay values.                 */

	    load_timing_graph_net_delays(net_delay);
	    T_crit = load_net_slack(net_slack, 0);
	    printf("T_crit: %g.\n", T_crit);
	}

    printf("Routing failed.\n");
    free_timing_driven_route_structs(pin_criticality, sink_order,
				     rt_node_of_sink);
    return (FALSE);
}


void
alloc_timing_driven_route_structs(float **pin_criticality_ptr,
				  int **sink_order_ptr,
				  t_rt_node *** rt_node_of_sink_ptr)
{

/* Allocates all the structures needed only by the timing-driven router.   */

    int max_pins_per_net;
    float *pin_criticality;
    int *sink_order;
    t_rt_node **rt_node_of_sink;

    max_pins_per_net = get_max_pins_per_net();

    pin_criticality =
	(float *)my_malloc((max_pins_per_net - 1) * sizeof(float));
    *pin_criticality_ptr = pin_criticality - 1;	/* First sink is pin #1. */

    sink_order = (int *)my_malloc((max_pins_per_net - 1) * sizeof(int));
    *sink_order_ptr = sink_order - 1;

    rt_node_of_sink = (t_rt_node **) my_malloc((max_pins_per_net - 1) *
					       sizeof(t_rt_node *));
    *rt_node_of_sink_ptr = rt_node_of_sink - 1;

    alloc_route_tree_timing_structs();
}


void
free_timing_driven_route_structs(float *pin_criticality,
				 int *sink_order,
				 t_rt_node ** rt_node_of_sink)
{

/* Frees all the stuctures needed only by the timing-driven router.        */

    free(pin_criticality + 1);	/* Starts at index 1. */
    free(sink_order + 1);
    free(rt_node_of_sink + 1);
    free_route_tree_timing_structs();
}


static int
get_max_pins_per_net(void)
{

/* Returns the largest number of pins on any non-global net.    */

    int inet, max_pins_per_net;

    max_pins_per_net = 0;
    for(inet = 0; inet < num_nets; inet++)
	{
	    if(net[inet].is_global == FALSE)
		{
		    max_pins_per_net =
			max(max_pins_per_net, (net[inet].num_sinks + 1));
		}
	}

    return (max_pins_per_net);
}


boolean
timing_driven_route_net(int inet,
			float pres_fac,
			float max_criticality,
			float criticality_exp,
			float astar_fac,
			float bend_cost,
			float *net_slack,
			float *pin_criticality,
			int *sink_order,
			t_rt_node ** rt_node_of_sink,
			float T_crit,
			float *net_delay)
{

/* Returns TRUE as long is found some way to hook up this net, even if that *
 * way resulted in overuse of resources (congestion).  If there is no way   *
 * to route this net, even ignoring congestion, it returns FALSE.  In this  *
 * case the rr_graph is disconnected and you can give up.                   */

    int ipin, num_sinks, itarget, target_pin, target_node, inode;
    float target_criticality, old_tcost, new_tcost, largest_criticality,
	pin_crit;
    float old_back_cost, new_back_cost;
    t_rt_node *rt_root;
    struct s_heap *current;
    struct s_trace *new_route_start_tptr;

/* Rip-up any old routing. */

    pathfinder_update_one_cost(trace_head[inet], -1, pres_fac);
    free_traceback(inet);

    for(ipin = 1; ipin <= net[inet].num_sinks; ipin++)
	{			/* For all sinks */
	    pin_crit = max(max_criticality - net_slack[ipin] / T_crit, 0.);
	    pin_crit = pow(pin_crit, criticality_exp);
	    pin_crit = min(pin_crit, max_criticality);
	    pin_criticality[ipin] = pin_crit;
	}

    num_sinks = net[inet].num_sinks;
    heapsort(sink_order, pin_criticality, num_sinks);

/* Update base costs according to fanout and criticality rules */

    largest_criticality = pin_criticality[sink_order[1]];
    update_rr_base_costs(inet, largest_criticality);

    mark_ends(inet);		/* Only needed to check for multiply-connected SINKs */

    rt_root = init_route_tree_to_source(inet);

    for(itarget = 1; itarget <= num_sinks; itarget++)
	{
	    target_pin = sink_order[itarget];
	    target_node = net_rr_terminals[inet][target_pin];

	    target_criticality = pin_criticality[target_pin];

	    add_route_tree_to_heap(rt_root, target_node, target_criticality,
				   astar_fac);

	    current = get_heap_head();

	    if(current == NULL)
		{		/* Infeasible routing.  No possible path for net. */
		    reset_path_costs();
		    free_route_tree(rt_root);
		    return (FALSE);
		}

	    inode = current->index;

	    while(inode != target_node)
		{
		    old_tcost = rr_node_route_inf[inode].path_cost;
		    new_tcost = current->cost;

		    if(old_tcost > 0.99 * HUGE_FLOAT)	/* First time touched. */
			old_back_cost = HUGE_FLOAT;
		    else
			old_back_cost =
			    rr_node_route_inf[inode].backward_path_cost;

		    new_back_cost = current->backward_path_cost;

		    /* I only re-expand a node if both the "known" backward cost is lower  *
		     * in the new expansion (this is necessary to prevent loops from       *
		     * forming in the routing and causing havoc) *and* the expected total  *
		     * cost to the sink is lower than the old value.  Different R_upstream *
		     * values could make a path with lower back_path_cost less desirable   *
		     * than one with higher cost.  Test whether or not I should disallow   *
		     * re-expansion based on a higher total cost.                          */

		    if(old_tcost > new_tcost && old_back_cost > new_back_cost)
			{
			    rr_node_route_inf[inode].prev_node =
				current->u.prev_node;
			    rr_node_route_inf[inode].prev_edge =
				current->prev_edge;
			    rr_node_route_inf[inode].path_cost = new_tcost;
			    rr_node_route_inf[inode].backward_path_cost =
				new_back_cost;

			    if(old_tcost > 0.99 * HUGE_FLOAT)	/* First time touched. */
				add_to_mod_list(&rr_node_route_inf[inode].
						path_cost);

			    timing_driven_expand_neighbours(current, inet,
							    bend_cost,
							    target_criticality,
							    target_node,
							    astar_fac);
			}

		    free_heap_data(current);
		    current = get_heap_head();

		    if(current == NULL)
			{	/* Impossible routing.  No path for net. */
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

	    rr_node_route_inf[inode].target_flag--;	/* Connected to this SINK. */
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


static void
add_route_tree_to_heap(t_rt_node * rt_node,
		       int target_node,
		       float target_criticality,
		       float astar_fac)
{

/* Puts the entire partial routing below and including rt_node onto the heap *
 * (except for those parts marked as not to be expanded) by calling itself   *
 * recursively.                                                              */

    int inode;
    t_rt_node *child_node;
    t_linked_rt_edge *linked_rt_edge;
    float tot_cost, backward_path_cost, R_upstream;

/* Pre-order depth-first traversal */

    if(rt_node->re_expand)
	{
	    inode = rt_node->inode;
	    backward_path_cost = target_criticality * rt_node->Tdel;
	    R_upstream = rt_node->R_upstream;
	    tot_cost =
		backward_path_cost +
		astar_fac * get_timing_driven_expected_cost(inode,
							    target_node,
							    target_criticality,
							    R_upstream);
	    node_to_heap(inode, tot_cost, NO_PREVIOUS, NO_PREVIOUS,
			 backward_path_cost, R_upstream);
	}

    linked_rt_edge = rt_node->u.child_list;

    while(linked_rt_edge != NULL)
	{
	    child_node = linked_rt_edge->child;
	    add_route_tree_to_heap(child_node, target_node,
				   target_criticality, astar_fac);
	    linked_rt_edge = linked_rt_edge->next;
	}
}


static void
timing_driven_expand_neighbours(struct s_heap *current,
				int inet,
				float bend_cost,
				float criticality_fac,
				int target_node,
				float astar_fac)
{

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

    for(iconn = 0; iconn < num_edges; iconn++)
	{
	    to_node = rr_node[inode].edges[iconn];

	    if(rr_node[to_node].xhigh < route_bb[inet].xmin ||
	       rr_node[to_node].xlow > route_bb[inet].xmax ||
	       rr_node[to_node].yhigh < route_bb[inet].ymin ||
	       rr_node[to_node].ylow > route_bb[inet].ymax)
		continue;	/* Node is outside (expanded) bounding box. */

/* Prune away IPINs that lead to blocks other than the target one.  Avoids  *
 * the issue of how to cost them properly so they don't get expanded before *
 * more promising routes, but makes route-throughs (via CLBs) impossible.   *
 * Change this if you want to investigate route-throughs.                   */

	    to_type = rr_node[to_node].type;
	    if(to_type == IPIN && (rr_node[to_node].xhigh != target_x ||
				   rr_node[to_node].yhigh != target_y))
		continue;


/* new_back_pcost stores the "known" part of the cost to this node -- the   *
 * congestion cost of all the routing resources back to the existing route  *
 * plus the known delay of the total path back to the source.  new_tot_cost *
 * is this "known" backward cost + an expected cost to get to the target.   */

	    new_back_pcost = old_back_pcost + (1. - criticality_fac) *
		get_rr_cong_cost(to_node);

	    iswitch = rr_node[inode].switches[iconn];
	    if(switch_inf[iswitch].buffered)
		{
		    new_R_upstream = switch_inf[iswitch].R;
		}
	    else
		{
		    new_R_upstream = R_upstream + switch_inf[iswitch].R;
		}

	    Tdel =
		rr_node[to_node].C * (new_R_upstream +
				      0.5 * rr_node[to_node].R);
	    Tdel += switch_inf[iswitch].Tdel;
	    new_R_upstream += rr_node[to_node].R;
	    new_back_pcost += criticality_fac * Tdel;

	    if(bend_cost != 0.)
		{
		    from_type = rr_node[inode].type;
		    to_type = rr_node[to_node].type;
		    if((from_type == CHANX && to_type == CHANY) ||
		       (from_type == CHANY && to_type == CHANX))
			new_back_pcost += bend_cost;
		}

	    new_tot_cost = new_back_pcost + astar_fac *
		get_timing_driven_expected_cost(to_node, target_node,
						criticality_fac,
						new_R_upstream);

	    node_to_heap(to_node, new_tot_cost, inode, iconn, new_back_pcost,
			 new_R_upstream);

	}			/* End for all neighbours */
}


static float
get_timing_driven_expected_cost(int inode,
				int target_node,
				float criticality_fac,
				float R_upstream)
{

/* Determines the expected cost (due to both delay and resouce cost) to reach *
 * the target node from inode.  It doesn't include the cost of inode --       *
 * that's already in the "known" path_cost.                                   */

    t_rr_type rr_type;
    int cost_index, ortho_cost_index, num_segs_same_dir, num_segs_ortho_dir;
    float expected_cost, cong_cost, Tdel;

    rr_type = rr_node[inode].type;

    if(rr_type == CHANX || rr_type == CHANY)
	{
	    num_segs_same_dir =
		get_expected_segs_to_target(inode, target_node,
					    &num_segs_ortho_dir);
	    cost_index = rr_node[inode].cost_index;
	    ortho_cost_index = rr_indexed_data[cost_index].ortho_cost_index;

	    cong_cost =
		num_segs_same_dir * rr_indexed_data[cost_index].base_cost +
		num_segs_ortho_dir *
		rr_indexed_data[ortho_cost_index].base_cost;
	    cong_cost +=
		rr_indexed_data[IPIN_COST_INDEX].base_cost +
		rr_indexed_data[SINK_COST_INDEX].base_cost;

	    Tdel = num_segs_same_dir * rr_indexed_data[cost_index].T_linear +
		num_segs_ortho_dir *
		rr_indexed_data[ortho_cost_index].T_linear +
		num_segs_same_dir * num_segs_same_dir *
		rr_indexed_data[cost_index].T_quadratic +
		num_segs_ortho_dir * num_segs_ortho_dir *
		rr_indexed_data[ortho_cost_index].T_quadratic +
		R_upstream * (num_segs_same_dir *
			      rr_indexed_data[cost_index].C_load +
			      num_segs_ortho_dir *
			      rr_indexed_data[ortho_cost_index].C_load);

	    Tdel += rr_indexed_data[IPIN_COST_INDEX].T_linear;

	    expected_cost =
		criticality_fac * Tdel + (1. - criticality_fac) * cong_cost;
	    return (expected_cost);
	}

    else if(rr_type == IPIN)
	{			/* Change if you're allowing route-throughs */
	    return (rr_indexed_data[SINK_COST_INDEX].base_cost);
	}

    else
	{			/* Change this if you want to investigate route-throughs */
	    return (0.);
	}
}


/* Macro used below to ensure that fractions are rounded up, but floating   *
 * point values very close to an integer are rounded to that integer.       */

#define ROUND_UP(x) (ceil (x - 0.001))


static int
get_expected_segs_to_target(int inode,
			    int target_node,
			    int *num_segs_ortho_dir_ptr)
{

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

    if(rr_type == CHANX)
	{
	    ylow = rr_node[inode].ylow;
	    xhigh = rr_node[inode].xhigh;
	    xlow = rr_node[inode].xlow;

	    /* Count vertical (orthogonal to inode) segs first. */

	    if(ylow > target_y)
		{		/* Coming from a row above target? */
		    *num_segs_ortho_dir_ptr =
			ROUND_UP((ylow - target_y + 1.) * ortho_inv_length);
		    no_need_to_pass_by_clb = 1;
		}
	    else if(ylow < target_y - 1)
		{		/* Below the FB bottom? */
		    *num_segs_ortho_dir_ptr = ROUND_UP((target_y - ylow) *
						       ortho_inv_length);
		    no_need_to_pass_by_clb = 1;
		}
	    else
		{		/* In a row that passes by target FB */
		    *num_segs_ortho_dir_ptr = 0;
		    no_need_to_pass_by_clb = 0;
		}

	    /* Now count horizontal (same dir. as inode) segs. */

	    if(xlow > target_x + no_need_to_pass_by_clb)
		{
		    num_segs_same_dir =
			ROUND_UP((xlow - no_need_to_pass_by_clb -
				  target_x) * inv_length);
		}
	    else if(xhigh < target_x - no_need_to_pass_by_clb)
		{
		    num_segs_same_dir =
			ROUND_UP((target_x - no_need_to_pass_by_clb -
				  xhigh) * inv_length);
		}
	    else
		{
		    num_segs_same_dir = 0;
		}
	}

    else
	{			/* inode is a CHANY */
	    ylow = rr_node[inode].ylow;
	    yhigh = rr_node[inode].yhigh;
	    xlow = rr_node[inode].xlow;

	    /* Count horizontal (orthogonal to inode) segs first. */

	    if(xlow > target_x)
		{		/* Coming from a column right of target? */
		    *num_segs_ortho_dir_ptr =
			ROUND_UP((xlow - target_x + 1.) * ortho_inv_length);
		    no_need_to_pass_by_clb = 1;
		}
	    else if(xlow < target_x - 1)
		{		/* Left of and not adjacent to the FB? */
		    *num_segs_ortho_dir_ptr = ROUND_UP((target_x - xlow) *
						       ortho_inv_length);
		    no_need_to_pass_by_clb = 1;
		}
	    else
		{		/* In a column that passes by target FB */
		    *num_segs_ortho_dir_ptr = 0;
		    no_need_to_pass_by_clb = 0;
		}

	    /* Now count vertical (same dir. as inode) segs. */

	    if(ylow > target_y + no_need_to_pass_by_clb)
		{
		    num_segs_same_dir =
			ROUND_UP((ylow - no_need_to_pass_by_clb -
				  target_y) * inv_length);
		}
	    else if(yhigh < target_y - no_need_to_pass_by_clb)
		{
		    num_segs_same_dir =
			ROUND_UP((target_y - no_need_to_pass_by_clb -
				  yhigh) * inv_length);
		}
	    else
		{
		    num_segs_same_dir = 0;
		}
	}

    return (num_segs_same_dir);
}


static void
update_rr_base_costs(int inet,
		     float largest_criticality)
{

/* Changes the base costs of different types of rr_nodes according to the  *
 * criticality, fanout, etc. of the current net being routed (inet).       */

    float fanout, factor;
    int index;

    fanout = net[inet].num_sinks;

    /* Other reasonable values for factor include fanout and 1 */
    factor = sqrt(fanout);

    for(index = CHANX_COST_INDEX_START; index < num_rr_indexed_data; index++)
	{
	    if(rr_indexed_data[index].T_quadratic > 0.)
		{		/* pass transistor */
		    rr_indexed_data[index].base_cost =
			rr_indexed_data[index].saved_base_cost * factor;
		}
	    else
		{
		    rr_indexed_data[index].base_cost =
			rr_indexed_data[index].saved_base_cost;
		}
	}
}


#define ERROR_TOL 0.0001

static void
timing_driven_check_net_delays(float **net_delay)
{

/* Checks that the net delays computed incrementally during timing driven    *
 * routing match those computed from scratch by the net_delay.c module.      */

    int inet, ipin;
    float **net_delay_check;
    struct s_linked_vptr *ch_list_head_net_delay_check;

    net_delay_check = alloc_net_delay(&ch_list_head_net_delay_check);
    load_net_delay_from_routing(net_delay_check);

    for(inet = 0; inet < num_nets; inet++)
	{
	    for(ipin = 1; ipin <= net[inet].num_sinks; ipin++)
		{
		    if(net_delay_check[inet][ipin] == 0.)
			{	/* Should be only GLOBAL nets */
			    if(net_delay[inet][ipin] != 0.)
				{
				    printf
					("Error in timing_driven_check_net_delays: net %d pin %d."
					 "\tIncremental calc. net_delay is %g, but from scratch "
					 "net delay is %g.\n", inet, ipin,
					 net_delay[inet][ipin],
					 net_delay_check[inet][ipin]);
				    exit(1);
				}
			}
		    else
			{
			    if(fabs
			       (1. -
				net_delay[inet][ipin] /
				net_delay_check[inet][ipin]) > ERROR_TOL)
				{
				    printf
					("Error in timing_driven_check_net_delays: net %d pin %d."
					 "\tIncremental calc. net_delay is %g, but from scratch "
					 "net delay is %g.\n", inet, ipin,
					 net_delay[inet][ipin],
					 net_delay_check[inet][ipin]);
				    exit(1);
				}
			}
		}
	}

    free_net_delay(net_delay_check, &ch_list_head_net_delay_check);
    printf("Completed net delay value cross check successfully.\n");
}
