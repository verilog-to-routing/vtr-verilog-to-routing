#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "mst.h"
#include "route_export.h"
#include "route_common.h"
#include "route_directed_search.h"
#include "draw.h"


/********************* Subroutines local to this module *********************/

static boolean directed_search_route_net(int inet,
					 float pres_fac,
					 float astar_fac,
					 float bend_cost,
					 t_mst_edge ** mst);

static void directed_search_expand_trace_segment(struct s_trace *start_ptr,
						 int target_node,
						 float astar_fac,
						 int
						 remaining_connections_to_sink);

static void directed_search_expand_neighbours(struct s_heap *current,
					      int inet,
					      float bend_cost,
					      int target_node,
					      float astar_fac);

static void directed_search_add_source_to_heap(int inet,
					       int target_node,
					       float astar_fac);

static float get_directed_search_expected_cost(int inode,
					       int target_node);

static int get_expected_segs_to_target(int inode,
				       int target_node,
				       int *num_segs_ortho_dir_ptr);

/************************ Subroutine definitions ****************************/

boolean
try_directed_search_route(struct s_router_opts router_opts,
			  t_ivec ** fb_opins_used_locally,
			  int width_fac,
			  t_mst_edge ** mst)
{

/* Iterated maze router ala Pathfinder Negotiated Congestion algorithm,  *
 * (FPGA 95 p. 111).  Returns TRUE if it can route this FPGA, FALSE if   *
 * it can't.                                                             */

    float pres_fac;
    boolean success, is_routable, rip_up_local_opins;
    int itry, inet;

    /* char msg[100]; */

    /* mst not built as good as it should, ideally, just have it after placement once only
	 however, rr_node numbers changed when the channel width changes so forced to do it here */
    if(mst)
	{
	    for(inet = 0; inet < num_nets; inet++)
		{
		    free(mst[inet]);
		    mst[inet] = get_mst_of_net(inet);
		}
	}

/* Usually the first iteration uses a very small (or 0) pres_fac to find  *
 * the shortest path and get a congestion map.  For fast compiles, I set  *
 * pres_fac high even for the first iteration.                            */

    pres_fac = router_opts.first_iter_pres_fac;

    for(itry = 1; itry <= router_opts.max_router_iterations; itry++)
	{

	    for(inet = 0; inet < num_nets; inet++)
		{
		    if(net[inet].is_global == FALSE)
			{	/* Skip global nets. */

			    is_routable =
				directed_search_route_net(inet, pres_fac,
							  router_opts.
							  astar_fac,
							  router_opts.
							  bend_cost, mst);

			    /* Impossible to route? (disconnected rr_graph) */

			    if(!is_routable)
				{
				    printf("Routing failed.\n");
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

	    success = feasible_routing();
	    if(success)
		{
		    printf
			("Successfully routed after %d routing iterations by Directed Search.\n",
			 itry);
		    return (TRUE);
		}
#if 0
	    else
		{
		    sprintf(msg,
			    "After iteration %d routing failed (A*) with a channel width factor of %d and Fc_int of %d, Fs_int of %d.",
			    itry, width_fac, Fc_int, Fs_int);
		    init_draw_coords(pins_per_clb);
		    update_screen(MAJOR, msg, ROUTING, FALSE);
		}
#endif


	    if(itry == 1)
		{
		    pres_fac = router_opts.initial_pres_fac;
		    pathfinder_update_cost(pres_fac, 0.);	/* Acc_fac=0 for first iter. */
		}
	    else
		{
		    pres_fac *= router_opts.pres_fac_mult;
		    pathfinder_update_cost(pres_fac, router_opts.acc_fac);
		}

	}

    printf("Routing failed.\n");

    return (FALSE);
}


static boolean
directed_search_route_net(int inet,
			  float pres_fac,
			  float astar_fac,
			  float bend_cost,
			  t_mst_edge ** mst)
{

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
 * lack of potential paths, rather than congestion), it returns FALSE, as    *
 * routing is impossible on this architecture.  Otherwise it returns TRUE.   */
/* WMF: This is the directed search (A-star) version of maze router. */

    int inode, remaining_connections_to_sink;
    int itarget, target_pin, target_node;
    struct s_heap *current;
    struct s_trace *new_route_start_tptr;
    float old_tcost, new_tcost, old_back_cost, new_back_cost;

/* Rip-up any old routing. */
    /* WMF: For the 1st router iteration trace_head[inet] is NULL, as it is 
     * my_calloc'ed in alloc_route_structs() so the following does nothing.
     * However, for subsequent iterations, trace_head[inet] contains the previous
     * ieration's routing for this net. */
    pathfinder_update_one_cost(trace_head[inet], -1, pres_fac);
    free_traceback(inet);	/* kills trace, and set the trace head and tail to NULL */

    /* adding the SOURCE node to the heap with correct total cost */
    target_pin = mst[inet][0].to_node;
    target_node = net_rr_terminals[inet][target_pin];
    directed_search_add_source_to_heap(inet, target_node, astar_fac);

    mark_ends(inet);

    remaining_connections_to_sink = 0;

    for(itarget = 0; itarget < net[inet].num_sinks; itarget++)
	{
	    target_pin = mst[inet][itarget].to_node;
	    target_node = net_rr_terminals[inet][target_pin];

	    /*    printf ("Target #%d, pin number %d, target_node: %d.\n",
	     * itarget, target_pin, target_node);  */

	    /* WMF: since the heap has been emptied, need to restart the wavefront
	     * from the current partial routing, starting at the trace_head (SOURCE) 
	     * Note: in the 1st iteration, there is no trace (no routing at all for this net)
	     * i.e. trace_head[inet] == NULL (found in free_traceback() in route_common.c, 
	     * which is called before the routing of any net), 
	     * so this routine does nothing, but the heap does have the SOURCE node due 
	     * to directed_search_add_source_to_heap (inet) before the loop */
	    directed_search_expand_trace_segment(trace_head[inet],
						 target_node, astar_fac,
						 remaining_connections_to_sink);

	    current = get_heap_head();

	    if(current == NULL)
		{		/* Infeasible routing.  No possible path for net. */
		    reset_path_costs();	/* Clean up before leaving. */
		    return (FALSE);
		}

	    inode = current->index;

	    while(inode != target_node)
		{
		    old_tcost = rr_node_route_inf[inode].path_cost;
		    new_tcost = current->cost;

		    /* WMF: not needed if Vaughn initialized rr_node_route_inf[inode].backward_path_cost
		     * to HUGE_FLOAT along with rr_node_route_inf[inode].path_cost */
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

		    /* updating the maze (Dijktra's min dist algorithm) if found "shorter" path */
		    if(old_tcost > new_tcost && old_back_cost > new_back_cost)
			{
/*       if (old_tcost > new_tcost) {     */
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

			    directed_search_expand_neighbours(current, inet,
							      bend_cost,
							      target_node,
							      astar_fac);
			}

		    free_heap_data(current);
		    current = get_heap_head();

		    if(current == NULL)
			{	/* Impossible routing.  No path for net. */
			    reset_path_costs();
			    return (FALSE);
			}

		    inode = current->index;
		}

	    rr_node_route_inf[inode].target_flag--;	/* Connected to this SINK. */
	    remaining_connections_to_sink =
		rr_node_route_inf[inode].target_flag;

	    /* keep info on the current routing of this net */
	    new_route_start_tptr = update_traceback(current, inet);

	    free_heap_data(current);
	    /* update the congestion costs of rr_nodes due to the routing to this sink 
	     * so only those nodes used in the partial routing of this sink and not 
	     * of the entire net (remember we're in a loop for this net over its sinks) */
	    pathfinder_update_one_cost(new_route_start_tptr, 1, pres_fac);

	    /* WMF: MUST empty heap and recalculate all total costs, because
	     * for the next sink, the target destination is changed, so the expected
	     * cost calculation is changed also, meaning all the nodes on the heap have
	     * "stale" total costs (costs based on last sink). */
	    empty_heap();
	    reset_path_costs();
	}

    return (TRUE);
}

static void
directed_search_expand_trace_segment(struct s_trace *start_ptr,
				     int target_node,
				     float astar_fac,
				     int remaining_connections_to_sink)
{

/* Adds all the rr_nodes in the entire traceback from SOURCE to all SINKS   *
 * routed so far (partial routing). 
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

    struct s_trace *tptr;
    int inode, backward_path_cost, tot_cost;

    tptr = start_ptr;

    if(remaining_connections_to_sink == 0)
	{			/* Usual case. */
	    while(tptr != NULL)
		{
		    /* WMF: partial routing is added to the heap with path cost of 0, because
		     * new extension to the next sink can start at any point on current partial 
		     * routing. However, for directed search the total cost must be made to favor
		     * the points of current partial routing that are NEAR the next sink (target sink) */

		    /* WMF: IPINs and SINKs should be excluded from the heap in this
		     * since they NEVER connect TO any rr_node (no to_edges), but since they have
		     * no to_edges, it's ok (ROUTE_THROUGHS are disabled). To clarify, see 
		     * rr_graph.c to find out rr_node[inode].num_edges = 0 for SINKs and
		     * rr_node[inode].num_edges = 1 for INPINs */

		    inode = tptr->index;

		    if(!
		       (rr_node[inode].type == IPIN
			|| rr_node[inode].type == SINK))
			{

			    backward_path_cost = 0;
			    tot_cost =
				backward_path_cost +
				astar_fac *
				get_directed_search_expected_cost(inode,
								  target_node);
			    node_to_heap(inode, tot_cost, NO_PREVIOUS,
					 NO_PREVIOUS, backward_path_cost,
					 OPEN);
			}

		    tptr = tptr->next;
		}
	}

    else
	{			/* This case never executes for most logic blocks. */
	    printf("Warning: Multiple connections from net to the same sink. "
		   "This should not happen for LUT/Cluster based logic blocks. Aborting.\n");
	    exit(1);
	}
}

static void
directed_search_expand_neighbours(struct s_heap *current,
				  int inet,
				  float bend_cost,
				  int target_node,
				  float astar_fac)
{

/* Puts all the rr_nodes adjacent to current on the heap.  rr_nodes outside   *
 * the expanded bounding box specified in route_bb are not added to the     *
 * heap.  back_cost is the path_cost to get to inode. total cost i.e.
 * tot_cost is path_cost + (expected_cost to target sink) */

    int iconn, to_node, num_edges, inode, target_x, target_y;
    t_rr_type from_type, to_type;
    float new_tot_cost, old_back_pcost, new_back_pcost;

    inode = current->index;
    old_back_pcost = current->backward_path_cost;
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
 * new_tot_cost 
 * is this "known" backward cost + an expected cost to get to the target.   */

	    new_back_pcost = old_back_pcost + get_rr_cong_cost(to_node);

	    if(bend_cost != 0.)
		{
		    from_type = rr_node[inode].type;
		    to_type = rr_node[to_node].type;
		    if((from_type == CHANX && to_type == CHANY) ||
		       (from_type == CHANY && to_type == CHANX))
			new_back_pcost += bend_cost;
		}

	    /* Calculate the new total cost = path cost + astar_fac * remaining distance to target */
	    new_tot_cost = new_back_pcost + astar_fac *
		get_directed_search_expected_cost(to_node, target_node);

	    node_to_heap(to_node, new_tot_cost, inode, iconn, new_back_pcost,
			 OPEN);
	}
}


static void
directed_search_add_source_to_heap(int inet,
				   int target_node,
				   float astar_fac)
{

/* Adds the SOURCE of this net to the heap.  Used to start a net's routing. */

    int inode;
    float back_cost, tot_cost;

    inode = net_rr_terminals[inet][0];	/* SOURCE */
    back_cost = 0.0 + get_rr_cong_cost(inode);

    /* setting the total cost to 0 because it's the only element on the heap */
    if(!is_empty_heap())
	{
	    printf
		("Error: Wrong Assumption: in directed_search_add_source_to_heap "
		 "the heap is not empty. Need to properly calculate source node's cost.\n");
	    exit(1);
	}

    /* WMF: path cost is 0. could use tot_cost = 0 to save some computation time, but
     * for consistency, I chose to do the expected cost calculation. */
    tot_cost =
	back_cost + astar_fac * get_directed_search_expected_cost(inode,
								  target_node);
    node_to_heap(inode, tot_cost, NO_PREVIOUS, NO_PREVIOUS, back_cost, OPEN);

}


static float
get_directed_search_expected_cost(int inode,
				  int target_node)
{

/* Determines the expected cost (due to resouce cost i.e. distance) to reach  *
 * the target node from inode.  It doesn't include the cost of inode --       *
 * that's already in the "known" path_cost.                                   */

    t_rr_type rr_type;
    int cost_index, ortho_cost_index, num_segs_same_dir, num_segs_ortho_dir;
    float cong_cost;

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

	    return (cong_cost);
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
