#include <cstdio>
using namespace std;

#include "util.h"
#include "vpr_utils.h"
#include "vpr_types.h"
#include "globals.h"
#include "route_common.h"
#include "route_tree_timing.h"
#include "route_timing.h"
#include "profile_lookahead.h"
#include <cassert>
#include <cmath>

/* This module keeps track of the partial routing tree for timing-driven     *
 * routing.  The normal traceback structure doesn't provide enough info      *
 * about the partial routing during timing-driven routing, so the routines   *
 * in this module are used to keep a tree representation of the partial      *
 * routing during timing-driven routing.  This allows rapid incremental      *
 * timing analysis.  The net_delay module does timing analysis in one step   *
 * (not incrementally as pieces of the routing are added).  I could probably *
 * one day remove a lot of net_delay.c and call the corresponding routines   *
 * here, but it's useful to have a from-scratch delay calculator to check    *
 * the results of this one.                                                  */

/********************** Variables local to this module ***********************/

/* Array below allows mapping from any rr_node to any rt_node currently in   *
 * the rt_tree.                                                              */

static t_rt_node **rr_node_to_rt_node = NULL; /* [0..num_rr_nodes-1] */

/* Frees lists for fast addition and deletion of nodes and edges. */

static t_rt_node *rt_node_free_list = NULL;
static t_linked_rt_edge *rt_edge_free_list = NULL;

/********************** Subroutines local to this module *********************/

static t_rt_node *alloc_rt_node(void);

static void free_rt_node(t_rt_node * rt_node);

static t_linked_rt_edge *alloc_linked_rt_edge(void);

static void free_linked_rt_edge(t_linked_rt_edge * rt_edge);

static t_rt_node *add_path_to_route_tree(struct s_heap *hptr,
		t_rt_node ** sink_rt_node_ptr, bool lookahead_eval, float target_criticality);

static void load_new_path_R_upstream(t_rt_node * start_of_new_path_rt_node);

static t_rt_node *update_unbuffered_ancestors_C_downstream(
		t_rt_node * start_of_new_path_rt_node);

static void load_rt_subtree_Tdel(t_rt_node * subtree_rt_root, float Tarrival);

/************************** Subroutine definitions ***************************/

void alloc_route_tree_timing_structs(void) {

	/* Allocates any structures needed to build the routing trees. */

	if (rr_node_to_rt_node != NULL || rt_node_free_list != NULL
			|| rt_node_free_list != NULL) {
			vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
				"in alloc_route_tree_timing_structs: old structures already exist.\n");
	}

	rr_node_to_rt_node = (t_rt_node **) my_malloc(
			num_rr_nodes * sizeof(t_rt_node *));
}

void free_route_tree_timing_structs(void) {

	/* Frees the structures needed to build routing trees, and really frees      *
	 * (i.e. calls free) all the data on the free lists.                         */

	t_rt_node *rt_node, *next_node;
	t_linked_rt_edge *rt_edge, *next_edge;

	free(rr_node_to_rt_node);
	rr_node_to_rt_node = NULL;

	rt_node = rt_node_free_list;

	while (rt_node != NULL) {
		next_node = rt_node->u.next;
		free(rt_node);
		rt_node = next_node;
	}

	rt_node_free_list = NULL;

	rt_edge = rt_edge_free_list;

	while (rt_edge != NULL) {
		next_edge = rt_edge->next;
		free(rt_edge);
		rt_edge = next_edge;
	}

	rt_edge_free_list = NULL;
}

static t_rt_node *
alloc_rt_node(void) {

	/* Allocates a new rt_node, from the free list if possible, from the free   *
	 * store otherwise.                                                         */

	t_rt_node *rt_node;

	rt_node = rt_node_free_list;

	if (rt_node != NULL) {
		rt_node_free_list = rt_node->u.next;
	} else {
		rt_node = (t_rt_node *) my_malloc(sizeof(t_rt_node));
	}

	return (rt_node);
}

static void free_rt_node(t_rt_node * rt_node) {

	/* Adds rt_node to the proper free list.          */

	rt_node->u.next = rt_node_free_list;
	rt_node_free_list = rt_node;
}

static t_linked_rt_edge *
alloc_linked_rt_edge(void) {

	/* Allocates a new linked_rt_edge, from the free list if possible, from the  *
	 * free store otherwise.                                                     */

	t_linked_rt_edge *linked_rt_edge;

	linked_rt_edge = rt_edge_free_list;

	if (linked_rt_edge != NULL) {
		rt_edge_free_list = linked_rt_edge->next;
	} else {
		linked_rt_edge = (t_linked_rt_edge *) my_malloc(
				sizeof(t_linked_rt_edge));
	}

	return (linked_rt_edge);
}

static void free_linked_rt_edge(t_linked_rt_edge * rt_edge) {

	/* Adds the rt_edge to the rt_edge free list.                       */

	rt_edge->next = rt_edge_free_list;
	rt_edge_free_list = rt_edge;
}

t_rt_node *
init_route_tree_to_source(int inet) {

	/* Initializes the routing tree to just the net source, and returns the root *
	 * node of the rt_tree (which is just the net source).                       */

	t_rt_node *rt_root;
	int inode;

	rt_root = alloc_rt_node();
	rt_root->u.child_list = NULL;
	rt_root->parent_node = NULL;
	rt_root->parent_switch = OPEN;
	rt_root->re_expand = true;

	inode = net_rr_terminals[inet][0]; /* Net source */

	rt_root->inode = inode;
	rt_root->C_downstream = rr_node[inode].C;
	rt_root->R_upstream = rr_node[inode].R;
	rt_root->Tdel = 0.5 * rr_node[inode].R * rr_node[inode].C;
	rr_node_to_rt_node[inode] = rt_root;

	return (rt_root);
}

t_rt_node *
update_route_tree(struct s_heap * hptr, bool lookahead_eval, float target_criticality) {

	/* Adds the most recently finished wire segment to the routing tree, and    *
	 * updates the Tdel, etc. numbers for the rest of the routing tree.  hptr   *
	 * is the heap pointer of the SINK that was reached.  This routine returns  *
	 * a pointer to the rt_node of the SINK that it adds to the routing.        */

	t_rt_node *start_of_new_path_rt_node, *sink_rt_node;
	t_rt_node *unbuffered_subtree_rt_root, *subtree_parent_rt_node;
	float Tdel_start;
	short iswitch;

	start_of_new_path_rt_node = add_path_to_route_tree(hptr, &sink_rt_node, lookahead_eval, target_criticality);
	load_new_path_R_upstream(start_of_new_path_rt_node);
	unbuffered_subtree_rt_root = update_unbuffered_ancestors_C_downstream(
			start_of_new_path_rt_node);

	subtree_parent_rt_node = unbuffered_subtree_rt_root->parent_node;

	if (subtree_parent_rt_node != NULL) { /* Parent exists. */
		Tdel_start = subtree_parent_rt_node->Tdel;
		iswitch = unbuffered_subtree_rt_root->parent_switch;
		Tdel_start += g_rr_switch_inf[iswitch].R
				* unbuffered_subtree_rt_root->C_downstream;
		Tdel_start += g_rr_switch_inf[iswitch].Tdel;
	} else { /* Subtree starts at SOURCE */
		Tdel_start = 0.;
	}

	load_rt_subtree_Tdel(unbuffered_subtree_rt_root, Tdel_start);

	return (sink_rt_node);
}

#if LOOKAHEADBYHISTORY == 1
/*
 * util function called by add_path_to_route_tree
 */
static void get_rt_subtree_bb_coord (int inode, int *bb_coord_x, int *bb_coord_y) {
    int type = rr_node[inode].type;
    int dir = rr_node[inode].get_direction();
    int xhigh = rr_node[inode].get_xhigh();
    int xlow = rr_node[inode].get_xlow();
    int yhigh = rr_node[inode].get_yhigh();
    int ylow = rr_node[inode].get_ylow();
    if (type == CHANX || type == CHANY) {
        if (dir == INC_DIRECTION) {
            *bb_coord_x = xlow;
            *bb_coord_x = ylow;
        } else if (dir == DEC_DIRECTION) {
            *bb_coord_x = xlow;
            *bb_coord_y = ylow;
        } else {
            // Bi-directional
            *bb_coord_x = (xhigh + xlow) / 2;
            *bb_coord_y = (yhigh + ylow) / 2;
        }
    } else {
        // OPIN
        *bb_coord_x = (xhigh + xlow) / 2;
        *bb_coord_y = (yhigh + ylow) / 2;
    }
}
#endif
static t_rt_node *
add_path_to_route_tree(struct s_heap *hptr, t_rt_node ** sink_rt_node_ptr, bool lookahead_eval, float target_criticality) {

	/* Adds the most recent wire segment, ending at the SINK indicated by hptr, *
	 * to the routing tree.  It returns the first (most upstream) new rt_node,  *
	 * and (via a pointer) the rt_node of the new SINK.                         */

	int inode, remaining_connections_to_sink, no_route_throughs;
	short iedge, iswitch;
	float C_downstream;
	t_rt_node *rt_node, *downstream_rt_node, *sink_rt_node;
	t_linked_rt_edge *linked_rt_edge;

	inode = hptr->index;
    int target_node = inode;
    float actual_tot_cost = 0;
    actual_tot_cost += 0;
    if (lookahead_eval) {
        assert(hptr->cost == hptr->backward_path_cost);
    }
    actual_tot_cost = hptr->backward_path_cost;
    float actual_Tdel = hptr->back_Tdel;
    actual_Tdel += 0;
#ifdef DEBUG
	if (rr_node[inode].type != SINK) {
		vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
			"in add_path_to_route_tree. Expected type = SINK (%d).\n"
			"Got type = %d.",  SINK, rr_node[inode].type);
	}
#endif

	remaining_connections_to_sink = rr_node_route_inf[inode].target_flag;
	sink_rt_node = alloc_rt_node();
	sink_rt_node->u.child_list = NULL;
	sink_rt_node->inode = inode;
	C_downstream = rr_node[inode].C;
	sink_rt_node->C_downstream = C_downstream;
	rr_node_to_rt_node[inode] = sink_rt_node;

	/* In the code below I'm marking SINKs and IPINs as not to be re-expanded.  *
	 * Undefine NO_ROUTE_THROUGHS if you want route-throughs or ipin doglegs.   *
	 * It makes the code more efficient (though not vastly) to prune this way   *
	 * when there aren't route-throughs or ipin doglegs.                        */

#define NO_ROUTE_THROUGHS 1	/* Can't route through unused CLB outputs */
	no_route_throughs = 1;
	if (no_route_throughs == 1)
		sink_rt_node->re_expand = false;
	else {
		if (remaining_connections_to_sink == 0) { /* Usual case */
			sink_rt_node->re_expand = true;
		}

		/* Weird case.  This net connects several times to the same SINK.  Thus I   *
		 * can't re_expand this node as part of the partial routing for subsequent  *
		 * connections, since I need to reach it again via another path.            */

		else {
			sink_rt_node->re_expand = false;
		}
	}

	/* Now do it's predecessor. */
	downstream_rt_node = sink_rt_node;
	inode = hptr->u.prev_node;
	iedge = hptr->prev_edge;
	iswitch = rr_node[inode].switches[iedge];

	/* For all "new" nodes in the path */
    int new_nodes_count = 0;
    float new_nodes_dev = 0;
    new_nodes_count += 0;
    new_nodes_dev += 0;
    float new_nodes_abs_dev = 0;
    new_nodes_abs_dev += 0;

    float total_Tdel = rr_node_route_inf[inode].back_Tdel;
    total_Tdel += 0.;
    // used to get the Tdel eval for wire only
    int furthest_wire_inode = -1;
	while (rr_node_route_inf[inode].prev_node != NO_PREVIOUS) {
        if (rr_node[inode].type == CHANX
         || rr_node[inode].type == CHANY)
            furthest_wire_inode = inode;
        new_nodes_count ++;
        if (lookahead_eval) {
#if NETWISELOOKAHEADEVAL == 1 || DEPTHWISELOOKAHEADEVAL == 1
            float estimated_future_cost, c_downstream, basecost = 0;
            estimated_future_cost = get_timing_driven_future_Tdel(inode, target_node, &c_downstream, &basecost);
            float actual_future_cost = total_Tdel - rr_node_route_inf[inode].back_Tdel;
            estimated_future_cost += 0.0;
            actual_future_cost += 0.0;
#endif
#if NETWISELOOKAHEADEVAL == 1
            node_on_path ++;
            if ( actual_future_cost != 0 ) {
                estimated_cost_deviation += (estimated_future_cost / actual_future_cost - 1);
                estimated_cost_deviation_abs += abs(estimated_future_cost / actual_future_cost - 1);
            }
#endif
#if DEPTHWISELOOKAHEADEVAL == 1
            /*
            if (actual_future_cost != 0) {
                new_nodes_dev += (estimated_future_cost / total_Tdel - 1);
                new_nodes_abs_dev += abs(estimated_future_cost / total_Tdel - 1);
            }
            */
#endif
        }
		linked_rt_edge = alloc_linked_rt_edge();
		linked_rt_edge->child = downstream_rt_node;
		linked_rt_edge->iswitch = iswitch;
		linked_rt_edge->next = NULL;

		rt_node = alloc_rt_node();
		downstream_rt_node->parent_node = rt_node;
		downstream_rt_node->parent_switch = iswitch;

		rt_node->u.child_list = linked_rt_edge;
		rt_node->inode = inode;

		if (g_rr_switch_inf[iswitch].buffered == false)
			C_downstream += rr_node[inode].C;
		else
			C_downstream = rr_node[inode].C;

		rt_node->C_downstream = C_downstream;
		rr_node_to_rt_node[inode] = rt_node;

		if (no_route_throughs == 1)
			if (rr_node[inode].type == IPIN)
				rt_node->re_expand = false;
			else
				rt_node->re_expand = true;

		else {
			if (remaining_connections_to_sink == 0) { /* Normal case */
				rt_node->re_expand = true;
			} else { /* This is the IPIN before a multiply-connected SINK */
				rt_node->re_expand = false;

				/* Reset flag so wire segments get reused */

				remaining_connections_to_sink = 0;
			}
		}

		downstream_rt_node = rt_node;
		iedge = rr_node_route_inf[inode].prev_edge;
		inode = rr_node_route_inf[inode].prev_node;
		iswitch = rr_node[inode].switches[iedge];
	}
    // XXX: only print out for critical path
#if PRINTCRITICALPATH == 1
    if (target_node == DB_TARGET_NODE && itry_share == DB_ITRY) {
        int iinode = hptr->u.prev_node;
        printf("CRITICAL PATH %d: back trace start\tcrit: %fNEW NODES %d\n", target_node, target_criticality, new_nodes_count);
        while (rr_node_route_inf[iinode].prev_node != NO_PREVIOUS) {
            // this is backtrace, so print in anti-direction order
            int ix_s, ix_e, iy_s, iy_e;
            get_unidir_seg_end(iinode, &ix_s, &iy_s);
            get_unidir_seg_start(iinode, &ix_e, &iy_e);
            int prev_iinode = rr_node_route_inf[iinode].prev_node;
            float bTdel = rr_node_route_inf[iinode].back_Tdel;
            float bbTdel = rr_node_route_inf[prev_iinode].back_Tdel;
            int wire_type = rr_indexed_data[rr_node[iinode].get_cost_index()].seg_index;
            float dummy, basecost = 0;
            printf("\tid:%d - %d\tstart(%d,%d)\tend(%d,%d)\tbackTdel:%einodeTdel:%e\tactual future Tdel:%.3f\test future Tdel:%.3f\n", 
                    iinode, wire_type, ix_s, iy_s, ix_e, iy_e,
                    bTdel, (bTdel - bbTdel), 
                    (actual_Tdel - bTdel) * pow(10, 10), 
                    get_timing_driven_future_Tdel(iinode, target_node, &dummy, &basecost) * pow(10, 10));
            iinode = prev_iinode;
        }
    }
#endif
	/* Inode is the join point to the old routing */
    if (rr_node[inode].type == CHANX
     || rr_node[inode].type == CHANY)
        furthest_wire_inode = inode;
    if (furthest_wire_inode == -1)
        furthest_wire_inode = target_node;
	rt_node = rr_node_to_rt_node[inode];

	linked_rt_edge = alloc_linked_rt_edge();
	linked_rt_edge->child = downstream_rt_node;
	linked_rt_edge->iswitch = iswitch;
	linked_rt_edge->next = rt_node->u.child_list;
	rt_node->u.child_list = linked_rt_edge;

	downstream_rt_node->parent_node = rt_node;
	downstream_rt_node->parent_switch = iswitch;

    new_nodes_count ++ ;
    // new_nodes_count ++ / node_on_path ++ accouts for the intersection node
    //if (new_nodes_count == 14)
    //    printf("TARGET NODE %d\n", target_node);

    if (lookahead_eval){
#if NETWISELOOKAHEADEVAL == 1 || DEPTHWISELOOKAHEADEVAL == 1
        /*
        float estimated_future_cost = rr_node_route_inf[inode].path_cost 
                                - rr_node_route_inf[inode].backward_path_cost;
        float actual_future_cost = actual_tot_cost 
                                - rr_node_route_inf[inode].backward_path_cost;
        */
        float estimated_future_cost, c_downstream, basecost = 0;
        estimated_future_cost = get_timing_driven_future_Tdel(furthest_wire_inode, target_node, &c_downstream, &basecost);
        float actual_future_cost = total_Tdel - rr_node_route_inf[furthest_wire_inode].back_Tdel;

#endif
#if NETWISELOOKAHEADEVAL == 1
        // netwise lookahead_eval finishing up
        node_on_path ++;
        if ( actual_future_cost != 0 ) {
            estimated_cost_deviation += (estimated_future_cost / actual_future_cost - 1);
            estimated_cost_deviation_abs += abs(estimated_future_cost / actual_future_cost - 1);
        }
#endif
#if DEPTHWISELOOKAHEADEVAL == 1
        // depthwise lookahead_eval setting up
        if (subtree_count.count(new_nodes_count) <= 0) {
            subtree_count[new_nodes_count] = 0;
            subtree_size_avg[new_nodes_count] = 0;
            subtree_est_dev_avg[new_nodes_count] = 0;
            subtree_est_dev_abs_avg[new_nodes_count] = 0;
        }
        if (actual_future_cost != 0) {
            new_nodes_dev += (estimated_future_cost / actual_future_cost - 1);
            new_nodes_abs_dev += abs(estimated_future_cost / actual_future_cost - 1);
        }
        subtree_count[new_nodes_count] ++;
        int tree_counter = subtree_count[new_nodes_count];
        // running avg
        // arithmetic avg
        //subtree_size_avg[new_nodes_count] = ((tree_counter - 1) * subtree_size_avg[new_nodes_count]
        //                                + subtree_size_avg[0]) / (float)tree_counter;
        // geometric avg
        if (subtree_size_avg[new_nodes_count] == 0) {
            subtree_size_avg[new_nodes_count] = subtree_size_avg[0];
            /*
            if (new_nodes_count == 19)
                printf("\nitry %d\tTARGET NODE %d\tsubtree count %d\tsubtree expanded nodes %f\n", itry_share, target_node, subtree_count[19], subtree_size_avg[0]);
            */
            if (target_node == DB_TARGET_NODE) {
                print_db_node_inf(DB_TARGET_NODE);
                print_db_node_inf(hptr->u.prev_node);
            }
        } else {
            float temp = pow(subtree_size_avg[new_nodes_count], (tree_counter - 1) / (float)tree_counter);
            subtree_size_avg[new_nodes_count] = temp * pow(subtree_size_avg[0], 1.0/(float)tree_counter);
        }
        float cur_dev = new_nodes_dev; // / new_nodes_count;
        /*
        if (cur_dev == -1) {
            printf("@@@@@@@\titry %d\ttarget_node %d\tnew nodes count %d\n", itry_share, target_node, new_nodes_count);
            assert(cur_dev != -1);
        }
        */
        if (cur_dev < -0.6 && new_nodes_count > 10)
            printf("\nXXXXX TARGET NODE %d\n\n", target_node);
        float cur_dev_abs = new_nodes_abs_dev / new_nodes_count;
        subtree_est_dev_avg[new_nodes_count] = ((tree_counter - 1) * subtree_est_dev_avg[new_nodes_count]
                                        + cur_dev) / (float)tree_counter;
        //subtree_est_dev_abs_avg[new_nodes_count] = ((tree_counter - 1) * subtree_est_dev_abs_avg[new_nodes_count]
        //                                + cur_dev_abs) / (float)tree_counter;
        if (subtree_est_dev_abs_avg[new_nodes_count] == 0) {
            subtree_est_dev_abs_avg[new_nodes_count] = cur_dev_abs;
        } else {
            float temp = pow(subtree_est_dev_abs_avg[new_nodes_count], (tree_counter - 1) / (float)tree_counter);
            subtree_est_dev_abs_avg[new_nodes_count] = temp * pow(cur_dev_abs, 1.0/(float)tree_counter);
        }
#endif
    }
	*sink_rt_node_ptr = sink_rt_node;
	return (downstream_rt_node);
}

static void load_new_path_R_upstream(t_rt_node * start_of_new_path_rt_node) {

	/* Sets the R_upstream values of all the nodes in the new path to the       *
	 * correct value.                                                           */

	float R_upstream;
	int inode;
	short iswitch;
	t_rt_node *rt_node, *parent_rt_node;
	t_linked_rt_edge *linked_rt_edge;

	rt_node = start_of_new_path_rt_node;
	iswitch = rt_node->parent_switch;
	inode = rt_node->inode;
	parent_rt_node = rt_node->parent_node;

	R_upstream = g_rr_switch_inf[iswitch].R + rr_node[inode].R;

	if (g_rr_switch_inf[iswitch].buffered == false)
		R_upstream += parent_rt_node->R_upstream;

	rt_node->R_upstream = R_upstream;

	/* Note:  the traversal below makes use of the fact that this new path      *
	 * really is a path (not a tree with branches) to do a traversal without    *
	 * recursion, etc.                                                          */

	linked_rt_edge = rt_node->u.child_list;

	while (linked_rt_edge != NULL) { /* While SINK not reached. */

#ifdef DEBUG
		if (linked_rt_edge->next != NULL) {
			vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
				"in load_new_path_R_upstream: new routing addition is a tree (not a path).\n");
		}
#endif

		rt_node = linked_rt_edge->child;
		iswitch = linked_rt_edge->iswitch;
		inode = rt_node->inode;

		if (g_rr_switch_inf[iswitch].buffered)
			R_upstream = g_rr_switch_inf[iswitch].R + rr_node[inode].R;
		else
			R_upstream += g_rr_switch_inf[iswitch].R + rr_node[inode].R;

		rt_node->R_upstream = R_upstream;
		linked_rt_edge = rt_node->u.child_list;
	}
}

static t_rt_node *
update_unbuffered_ancestors_C_downstream(t_rt_node * start_of_new_path_rt_node) {

	/* Updates the C_downstream values for the ancestors of the new path.  Once *
	 * a buffered switch is found amongst the ancestors, no more ancestors are  *
	 * affected.  Returns the root of the "unbuffered subtree" whose Tdel       *
	 * values are affected by the new path's addition.                          */

	t_rt_node *rt_node, *parent_rt_node;
	short iswitch;
	float C_downstream_addition;

	rt_node = start_of_new_path_rt_node;
	C_downstream_addition = rt_node->C_downstream;
	parent_rt_node = rt_node->parent_node;
	iswitch = rt_node->parent_switch;

	while (parent_rt_node != NULL && g_rr_switch_inf[iswitch].buffered == false) {
		rt_node = parent_rt_node;
		rt_node->C_downstream += C_downstream_addition;
		parent_rt_node = rt_node->parent_node;
		iswitch = rt_node->parent_switch;
	}

	return (rt_node);
}

static void load_rt_subtree_Tdel(t_rt_node * subtree_rt_root, float Tarrival) {

	/* Updates the Tdel values of the subtree rooted at subtree_rt_root by      *
	 * by calling itself recursively.  The C_downstream values of all the nodes *
	 * must be correct before this routine is called.  Tarrival is the time at  *
	 * at which the signal arrives at this node's *input*.                      */

	int inode;
	short iswitch;
	t_rt_node *child_node;
	t_linked_rt_edge *linked_rt_edge;
	float Tdel, Tchild;

	inode = subtree_rt_root->inode;

	/* Assuming the downstream connections are, on average, connected halfway    *
	 * along a wire segment's length.  See discussion in net_delay.c if you want *
	 * to change this.                                                           */

	Tdel = Tarrival + 0.5 * subtree_rt_root->C_downstream * rr_node[inode].R;
	subtree_rt_root->Tdel = Tdel;

	/* Now expand the children of this node to load their Tdel values (depth-   *
	 * first pre-order traversal).                                              */

	linked_rt_edge = subtree_rt_root->u.child_list;

	while (linked_rt_edge != NULL) {
		iswitch = linked_rt_edge->iswitch;
		child_node = linked_rt_edge->child;

		Tchild = Tdel + g_rr_switch_inf[iswitch].R * child_node->C_downstream;
		Tchild += g_rr_switch_inf[iswitch].Tdel; /* Intrinsic switch delay. */
		load_rt_subtree_Tdel(child_node, Tchild);

		linked_rt_edge = linked_rt_edge->next;
	}
}

void free_route_tree(t_rt_node * rt_node) {

	/* Puts the rt_nodes and edges in the tree rooted at rt_node back on the    *
	 * free lists.  Recursive, depth-first post-order traversal.                */

	t_rt_node *child_node;
	t_linked_rt_edge *rt_edge, *next_edge;

	rt_edge = rt_node->u.child_list;

	while (rt_edge != NULL) { /* For all children */
		child_node = rt_edge->child;
		free_route_tree(child_node);
		next_edge = rt_edge->next;
		free_linked_rt_edge(rt_edge);
		rt_edge = next_edge;
	}

	free_rt_node(rt_node);
}

void update_net_delays_from_route_tree(float *net_delay,
		t_rt_node ** rt_node_of_sink, int inet) {

	/* Goes through all the sinks of this net and copies their delay values from *
	 * the route_tree to the net_delay array.                                    */

	unsigned int isink;
	t_rt_node *sink_rt_node;

	for (isink = 1; isink < g_clbs_nlist.net[inet].pins.size(); isink++) {
		sink_rt_node = rt_node_of_sink[isink];
		net_delay[isink] = sink_rt_node->Tdel;
	}
}
