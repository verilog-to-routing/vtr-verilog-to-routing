#include <cstdio>
#include <cassert>
#include <vector>
using namespace std;

#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "route_common.h"
#include "route_tree_timing.h"

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
		t_rt_node ** sink_rt_node_ptr);

static void load_new_path_R_upstream(t_rt_node * start_of_new_path_rt_node);

static t_rt_node *update_unbuffered_ancestors_C_downstream(
		t_rt_node * start_of_new_path_rt_node);

static void load_rt_subtree_Tdel(t_rt_node * subtree_rt_root, float Tarrival);

// static t_rt_node* update_route_tree_from_trace(int sink_node, int prev_node, short iswitch);

// static t_rt_node* add_path_to_route_tree_from_trace(int sink_node, int prev_node, short iswitch, t_rt_node** sink_rt_node_ptr);

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
update_route_tree(struct s_heap * hptr) {

	/* Adds the most recently finished wire segment to the routing tree, and    *
	 * updates the Tdel, etc. numbers for the rest of the routing tree.  hptr   *
	 * is the heap pointer of the SINK that was reached.  This routine returns  *
	 * a pointer to the rt_node of the SINK that it adds to the routing.        */

	t_rt_node *start_of_new_path_rt_node, *sink_rt_node;
	t_rt_node *unbuffered_subtree_rt_root, *subtree_parent_rt_node;
	float Tdel_start;
	short iswitch;

	start_of_new_path_rt_node = add_path_to_route_tree(hptr, &sink_rt_node);
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

static t_rt_node *
add_path_to_route_tree(struct s_heap *hptr, t_rt_node ** sink_rt_node_ptr) {

	/* Adds the most recent wire segment, ending at the SINK indicated by hptr, *
	 * to the routing tree.  It returns the first (most upstream) new rt_node,  *
	 * and (via a pointer) the rt_node of the new SINK.                         */

	int inode, remaining_connections_to_sink, no_route_throughs;
	short iedge, iswitch;
	float C_downstream;
	t_rt_node *rt_node, *downstream_rt_node, *sink_rt_node;
	t_linked_rt_edge *linked_rt_edge;

	inode = hptr->index;

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
	// inode is node index of previous node
	// NO_PREVIOUS for a valid node (part of existing route)

	while (rr_node_route_inf[inode].prev_node != NO_PREVIOUS) {
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

	/* Inode is the join point to the old routing */
	// do not need to alloc another node since the old routing has done so already
	rt_node = rr_node_to_rt_node[inode];

	linked_rt_edge = alloc_linked_rt_edge();
	linked_rt_edge->child = downstream_rt_node;
	linked_rt_edge->iswitch = iswitch;
	linked_rt_edge->next = rt_node->u.child_list;
	rt_node->u.child_list = linked_rt_edge;

	downstream_rt_node->parent_node = rt_node;
	downstream_rt_node->parent_switch = iswitch;

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

// conversion between route tree and traceback, creates skeleton without cost
t_rt_node* traceback_to_route_tree(int inet, t_rt_node** rt_node_of_sink) {
	// returns the root of the converted route tree
	// initially points at the traceback equivalent of root
	t_trace* head {trace_head[inet]};
	if (!head) return nullptr;

	t_rt_node* rt_root {init_route_tree_to_source(inet)};
	t_rt_node* parent_node {rt_root};

	t_trace* tail;
	// dangling edge waiting to point to next child
	t_linked_rt_edge* prev_edge {alloc_linked_rt_edge()};	
	prev_edge->iswitch = head->iswitch;
	prev_edge->next = nullptr;

	rt_root->u.child_list = prev_edge;

	vpr_printf_info("starting conversion with head %d\n", head->index);

	while (head) {
		// keep head on a branch of the traceback that has been allocated already and advance tail to a sink
		vpr_printf_info("head %d\n", head->index);
		tail = head->next;
		while (tail) {
			vpr_printf_info("tail %d\n", tail->index);
			// tail is always a new element to be converted into a rt_node
			if (rr_node[tail->index].type == SINK) {
				vpr_printf_info("tail reached sink %d\n", tail->index);
				// [head+1, tail(sink)] is the new part of the route tree
				for (t_trace* new_node = head->next; new_node != tail; new_node = new_node->next) {
					vpr_printf_info("allocating rt_node for %d\n", new_node->index);
					t_rt_node* rt_node = alloc_rt_node();
					rt_node->inode = new_node->index;
					rt_node->parent_node = parent_node;
					rt_node->parent_switch = prev_edge->iswitch;

					// add to dangling edge (switch determined when edge gets created)
					prev_edge->child = rt_node;
					vpr_printf_info("edge points to %d\n", prev_edge->child->inode);
					// create new dangling edge from the current node to the next (there exists one since new_node isn't tail (sink))
					prev_edge = alloc_linked_rt_edge();
					prev_edge->iswitch = new_node->iswitch;
					prev_edge->next = nullptr;
					rt_node->u.child_list = prev_edge;
					// hold onto rt_node since we will need it when it becomes the join point for another branch
					rr_node_to_rt_node[new_node->index] = rt_node;
					parent_node = rt_node;
				}
				// still need to add the sink to the route tree
				vpr_printf_info("allocating rt_node for sink %d\n", tail->index);
				t_rt_node* rt_node = alloc_rt_node();
				rt_node->u.child_list = nullptr;	// no child for sink
				rt_node->inode = tail->index;
				prev_edge->child = rt_node;

				rt_node_of_sink[tail->index] = rt_node;
				// terminates this series of edges
				break;
			}
			tail = tail->next;
		}
		// either another prexisting part of the route tree and more sinks to come or end
		head = tail->next;
		if (!head) break;
		// the rt_node has already been alocated, get a new dangling edge from it
		parent_node = rr_node_to_rt_node[head->index];
		prev_edge = alloc_linked_rt_edge();
		prev_edge->iswitch = head->iswitch;
		prev_edge->next = parent_node->u.child_list;
		parent_node->u.child_list = prev_edge;
		// edge is still dangling in preparation for next sink
	}

	return rt_root;
}

void prune_illegal_branches_from_route_tree(t_rt_node* rt_root, float pres_fac, vector<int>& remaining_targets) {
	// removes illegal branches, populating remaining targets with the illegally routed sinks and
	// removing from pathfinder costs as we traverse down
	if (!rt_root) return;
	t_linked_rt_edge* edge {rt_root->u.child_list};
	// traverse iteratively as long as there is no branching
	for (;;) {
		int inode {rt_root->inode};
		// hit an illegal node
		if (rr_node[inode].get_occ() > rr_node[inode].get_capacity()) {
			// go up until the parent is a join point, in which case the entire branch can be removed
			t_rt_node* parent {rt_root->parent_node};
			while (parent && !(parent->u.child_list->next)) {
				rt_root = parent;
				parent = parent->parent_node;
			}
			edge = parent->u.child_list;
			// need to advance the parent's child list if the direct edge is about to removed
			if (edge->child == rt_node) parent->u.child_list = edge->next;
			// need to get a handle on the edge from join point to illegal branch
			else {
				// need to peek because the previous edge must point to nullptr afterwards
				while (edge->next->child != rt_node) edge = edge->next;
				t_linked_rt_edge* to_be_removed_edge {edge->next};
				edge->next = nullptr;
				edge = to_be_removed_edge;
			}
			// edge at the end of the previous block will always be the edge that needs to be removed
			free_linked_rt_edge(edge);
			
			// parent either source or a join point while rt_root is the subtree under that
			pathfinder_update_one_cost_from_route_tree(rt_root, -1, pres_fac, &remaining_targets);
			free_route_tree(rt_root);
			// this branch is terminated
			return;
		}

		// legal routing reached sink, this branch is completed and can calculate downstream_C from here
		if (!edge) {
			return;
		}
		// join point, have to recurse now
		else if (edge->next) {
			t_linked_rt_edge* sibling_edge = edge->next;
			do {
				prune_illegal_branches_from_route_tree(sibling_edge->child, pres_fac, remaining_targets);
				sibling_edge = sibling_edge->next;
			} while (sibling_edge->next);
			// keep working on the current branch to avoid unnecessary function calls
		}
		rt_root = edge->child;
		edge = rt_root->u.child_list;
	}

}

template <typename Visitor>
void traverse_route_tree(t_rt_node* rt_root, Visitor visit) {
	// general traversal pattern to minimize function calls
	if (!rt_root) return;
	t_linked_rt_edge* edge {rt_root->u.child_list};
	for (;;) {
		visit(rt_root);
		// sink, terminate
		if (!edge) {
			visit.sink(rt_root);
			return;
		}
		// join point, spwan parallel traversals (multithreading potential)
		else if (edge->next) {
			t_linked_rt_edge* sibling_edge = edge->next;
			do {
				traverse_route_tree(sibling_edge->child, visit);
				sibling_edge = sibling_edge->next;
			} while (sibling_edge->next);
		}
		rt_root = edge->child;
		edge = rt_root->u.child_list;
	}
}

void pathfinder_update_one_cost_from_route_tree(t_rt_node* rt_root, int add_or_sub, float pres_fac, vector<int>* reached_sinks) {
	// Like pathfinder_update_one_cost, but works with a route tree instead     
	if (!rt_root) return;

	int inode, occ, capacity;
	t_linked_rt_edge* edge {rt_root->u.child_list};
	
	// update every node once, so even do it for sinks and join points once
	for (;;) {
		inode = rt_root->inode;

		occ = rr_node[inode].get_occ() + add_or_sub;
		rr_node[inode].set_occ(occ);

		capacity = rr_node[inode].get_capacity();
		if (occ < capacity) {
			rr_node_route_inf[inode].pres_cost = 1.0;
		} else {
			rr_node_route_inf[inode].pres_cost = 1.0
					+ (occ + 1 - capacity) * pres_fac;
		}

		// reached a sink
		if (!edge) {
			// caller decides what to do with reached sinks
			if (reached_sinks) reached_sinks->push_back(inode);
			return;
		}
		// join point (sibling edges)
		else if (edge->next) {
			// recursively update for each of its sibling branches (spawn sibling calls while keeping current call to save on stack usage)
			t_linked_rt_edge* sibling_edge = edge->next;
			do {
				pathfinder_update_one_cost_from_route_tree(sibling_edge->child, add_or_sub, pres_fac, remaining_targets);
				sibling_edge = sibling_edge->next;
			} while (sibling_edge->next);
			// keep working on this branch, only terminating when you hit a sink
		}

		rt_root = edge->child;
		edge = rt_root->u.child_list;
	}
}

// static t_rt_node* update_route_tree_from_trace(int sink_node, int prev_node, short iswitch) {
// 	// like update_route_tree, but acts on traceback instead of a heap element

// 	t_rt_node *start_of_new_path_rt_node, *sink_rt_node;
// 	t_rt_node *unbuffered_subtree_rt_root, *subtree_parent_rt_node;
// 	float Tdel_start;

// 	start_of_new_path_rt_node = add_path_to_route_tree_from_trace(sink_node, prev_node, iswitch, &sink_rt_node);
// 	load_new_path_R_upstream(start_of_new_path_rt_node);
// 	unbuffered_subtree_rt_root = update_unbuffered_ancestors_C_downstream(
// 			start_of_new_path_rt_node);

// 	subtree_parent_rt_node = unbuffered_subtree_rt_root->parent_node;

// 	if (subtree_parent_rt_node != NULL) { /* Parent exists. */
// 		Tdel_start = subtree_parent_rt_node->Tdel;
// 		iswitch = unbuffered_subtree_rt_root->parent_switch;
// 		Tdel_start += g_rr_switch_inf[iswitch].R
// 				* unbuffered_subtree_rt_root->C_downstream;
// 		Tdel_start += g_rr_switch_inf[iswitch].Tdel;
// 	} else { /* Subtree starts at SOURCE */
// 		Tdel_start = 0.;
// 	}

// 	load_rt_subtree_Tdel(unbuffered_subtree_rt_root, Tdel_start);

// 	return sink_rt_node;	
// }
// static t_rt_node* add_path_to_route_tree_from_trace(int sink_node, int prev_node, short iswitch, t_rt_node** sink_rt_node_ptr) {
// 	// like add_path_to_route_tree, but using traceback instead of heap element

// 	int remaining_connections_to_sink, no_route_throughs;
// 	float C_downstream;
// 	t_rt_node *rt_node, *downstream_rt_node, *sink_rt_node;
// 	t_linked_rt_edge *linked_rt_edge;

// 	remaining_connections_to_sink = rr_node_route_inf[sink_node].target_flag;
// 	sink_rt_node = alloc_rt_node();
// 	sink_rt_node->u.child_list = NULL;
// 	sink_rt_node->inode = sink_node;
// 	vpr_printf_info("add path to route tree from trace sink rt node for %d crated \n", sink_node);
// 	C_downstream = rr_node[sink_node].C;
// 	sink_rt_node->C_downstream = C_downstream;
// 	// will cause memory leak during debugging since tree will be cloned
// 	rr_node_to_rt_node[sink_node] = sink_rt_node;

// 	/* see add_path_to_route_tree above for explanation of below */
// 	no_route_throughs = 1;
// 	if (no_route_throughs == 1)
// 		sink_rt_node->re_expand = false;
// 	else {
// 		if (remaining_connections_to_sink == 0) {  Usual case 
// 			sink_rt_node->re_expand = true;
// 		}
// 		else {
// 			sink_rt_node->re_expand = false;
// 		}
// 	}

// 	downstream_rt_node = sink_rt_node;
// 	short prev_edge;

// 	while (rr_node_route_inf[prev_node].prev_node != NO_PREVIOUS) {
// 		vpr_printf_info("up one on %d\n", prev_node);
// 		linked_rt_edge = alloc_linked_rt_edge();
// 		linked_rt_edge->child = downstream_rt_node;
// 		linked_rt_edge->iswitch = iswitch;
// 		linked_rt_edge->next = NULL;

// 		rt_node = alloc_rt_node();
// 		downstream_rt_node->parent_node = rt_node;
// 		downstream_rt_node->parent_switch = iswitch;

// 		rt_node->u.child_list = linked_rt_edge;
// 		rt_node->inode = prev_node;

// 		if (g_rr_switch_inf[iswitch].buffered == false)
// 			C_downstream += rr_node[prev_node].C;
// 		else
// 			C_downstream = rr_node[prev_node].C;

// 		rt_node->C_downstream = C_downstream;
// 		// will cause memory leak during debugging since tree will be cloned
// 		rr_node_to_rt_node[prev_node] = rt_node;

// 		if (no_route_throughs == 1)
// 			if (rr_node[prev_node].type == IPIN)
// 				rt_node->re_expand = false;
// 			else
// 				rt_node->re_expand = true;

// 		else {
// 			if (remaining_connections_to_sink == 0) { /* Normal case */
// 				rt_node->re_expand = true;
// 			} else { /* This is the IPIN before a multiply-connected SINK */
// 				rt_node->re_expand = false;

// 				/* Reset flag so wire segments get reused */

// 				remaining_connections_to_sink = 0;
// 			}
// 		}

// 		downstream_rt_node = rt_node;
// 		prev_edge = rr_node_route_inf[prev_node].prev_edge;
// 		prev_node = rr_node_route_inf[prev_node].prev_node;
// 		iswitch = rr_node[prev_node].switches[prev_edge];
// 		vpr_printf_info("new prev node %d\n", prev_node);
// 	}

// 	/* prev_node is the join point to the old routing */
// 	// do not need to alloc another node since the old routing has done so already
// 	rt_node = rr_node_to_rt_node[prev_node];

// 	linked_rt_edge = alloc_linked_rt_edge();
// 	linked_rt_edge->child = downstream_rt_node;
// 	linked_rt_edge->iswitch = iswitch;
// 	linked_rt_edge->next = rt_node->u.child_list;
// 	rt_node->u.child_list = linked_rt_edge;

// 	downstream_rt_node->parent_node = rt_node;
// 	downstream_rt_node->parent_switch = iswitch;

// 	*sink_rt_node_ptr = sink_rt_node;
// 	return (downstream_rt_node);
// }

// utility and debug functions 
struct Rt_print {
	t_rt_node* node;
	size_t indent;	// leftmost child gets parent indent, each child after gets sibling indent + some amount
	size_t num_child;
	size_t branch_factor;
	Rt_print() = default;
	Rt_print(t_rt_node* n) : node{n}, num_child{0}, branch_factor{0} {}
};
using Layers = vector<vector<Rt_print>>;

static Layers get_route_tree_layers(t_rt_node* rt_root) {
	Layers layers {vector<Rt_print>{rt_root}};
	size_t level {0};
	while (true) {
		vector<Rt_print> layer;
		for (Rt_print& parent : layers[level]) {	// every parent in previous layer
			t_linked_rt_edge* edges = parent.node->u.child_list;
			// explore each of its children
			size_t num_child {0};
			while (edges) {
				layer.emplace_back(edges->child);
				edges = edges->next;
				++num_child;
			}
			parent.num_child = num_child;
		}
		// nothing on this layer, no parents for next layer
		if (layer.empty()) break;
		layers.emplace_back(move(layer));
		++level;
	}
	return layers;
}
static void calculate_layers_branch_factor(Layers& layers) {
	size_t level {layers.size() - 1};	
	// indent levels can only be figured out after the first traversal, start from the bottom
	while (level > 0) { // not on the root layer
		const auto& child_layer = layers[level];
		size_t child_index = 0;
		for (Rt_print& parent : layers[level-1]) {	// parent layer
			// children's branch factors get carried up to parent
			for (size_t child = 0; child < parent.num_child; ++child) {
				parent.branch_factor += child_layer[child_index + child].branch_factor;
			}
			// own branch factor
			if (parent.num_child > 1) parent.branch_factor += parent.num_child - 1;
			child_index += parent.num_child;	// now the 0 indexed child for the next parent
			assert(child_index <= child_layer.size());
		}
		--level;
	}
}

void print_route_tree(t_rt_node* rt_root) {
	constexpr int indent_level {16};

	if (!rt_root) return;
	Layers layers {get_route_tree_layers(rt_root)};
	calculate_layers_branch_factor(layers);
	// actual printing
	for (const auto& layer : layers) {
		for (const auto& rt_node : layer) {
			vpr_printf_info("%-*d", (rt_node.branch_factor+1) * indent_level, rt_node.node->inode);
		}
		vpr_printf_info("\n");
	}
}