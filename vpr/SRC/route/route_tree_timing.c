#include <cstdio>
#include <cmath>
#include <vector>
using namespace std;

#include "vtr_assert.h"
#include "vtr_log.h"

#include "vpr_types.h"
#include "vpr_utils.h"
#include "vpr_error.h"

#include "globals.h"
#include "route_common.h"
#include "route_tree_timing.h"

/* This module keeps track of the partial routing tree for timing-driven
 * routing.  The normal traceback structure doesn't provide enough info
 * about the partial routing during timing-driven routing, so the routines
 * in this module are used to keep a tree representation of the partial
 * routing during timing-driven routing.  This allows rapid incremental
 * timing analysis.  The net_delay module does timing analysis in one step
 * (not incrementally as pieces of the routing are added).  I could probably
 * one day remove a lot of net_delay.c and call the corresponding routines
 * here, but it's useful to have a from-scratch delay calculator to check
 * the results of this one.                                                  */

/********************** Variables local to this module ***********************/

/* Array below allows mapping from any rr_node to any rt_node currently in
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

// preceeding_edge_to_subtree->next is the edge to be removed
static t_linked_rt_edge* free_route_subtree(t_rt_node* parent, t_linked_rt_edge* preceeding_edge_to_subtree);


/************************** Subroutine definitions ***************************/

constexpr float epsilon = 1e-15;
static bool equal_approx(float a, float b) {
	return fabs(a - b) < epsilon;
}

void alloc_route_tree_timing_structs(void) {

	/* Allocates any structures needed to build the routing trees. */

	if (rr_node_to_rt_node != NULL || rt_node_free_list != NULL
			|| rt_node_free_list != NULL) {
			vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
				"in alloc_route_tree_timing_structs: old structures already exist.\n");
	}

	rr_node_to_rt_node = (t_rt_node **) vtr::malloc(
			num_rr_nodes * sizeof(t_rt_node *));
}

void free_route_tree_timing_structs(void) {

	/* Frees the structures needed to build routing trees, and really frees
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

static t_rt_node*
alloc_rt_node(void) {

	/* Allocates a new rt_node, from the free list if possible, from the free
	 * store otherwise.                                                         */

	t_rt_node *rt_node;

	rt_node = rt_node_free_list;

	if (rt_node != NULL) {
		rt_node_free_list = rt_node->u.next;
	} else {
		rt_node = (t_rt_node *) vtr::malloc(sizeof(t_rt_node));
	}

	return (rt_node);
}

static void free_rt_node(t_rt_node * rt_node) {

	/* Adds rt_node to the proper free list.          */

	rt_node->u.next = rt_node_free_list;
	rt_node_free_list = rt_node;
}

static t_linked_rt_edge*
alloc_linked_rt_edge(void) {

	/* Allocates a new linked_rt_edge, from the free list if possible, from the
	 * free store otherwise.                                                     */

	t_linked_rt_edge *linked_rt_edge;

	linked_rt_edge = rt_edge_free_list;

	if (linked_rt_edge != NULL) {
		rt_edge_free_list = linked_rt_edge->next;
	} else {
		linked_rt_edge = (t_linked_rt_edge *) vtr::malloc(
				sizeof(t_linked_rt_edge));
	}

	VTR_ASSERT(linked_rt_edge != nullptr);
	return (linked_rt_edge);
}

static void free_linked_rt_edge(t_linked_rt_edge * rt_edge) {

	/* Adds the rt_edge to the rt_edge free list.                       */

	rt_edge->next = rt_edge_free_list;
	rt_edge_free_list = rt_edge;
}

t_rt_node*
init_route_tree_to_source(int inet) {

	/* Initializes the routing tree to just the net source, and returns the root
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

t_rt_node*
update_route_tree(struct s_heap * hptr) {

	/* Adds the most recently finished wire segment to the routing tree, and
	 * updates the Tdel, etc. numbers for the rest of the routing tree.  hptr
	 * is the heap pointer of the SINK that was reached.  This routine returns
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

	load_route_tree_Tdel(unbuffered_subtree_rt_root, Tdel_start);

	return (sink_rt_node);
}

static t_rt_node*
add_path_to_route_tree(struct s_heap *hptr, t_rt_node ** sink_rt_node_ptr) {

	/* Adds the most recent wire segment, ending at the SINK indicated by hptr,
	 * to the routing tree.  It returns the first (most upstream) new rt_node,
	 * and (via a pointer) the rt_node of the new SINK. Traverses up from SINK  */

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

	/* In the code below I'm marking SINKs and IPINs as not to be re-expanded.
	 * Undefine NO_ROUTE_THROUGHS if you want route-throughs or ipin doglegs.
	 * It makes the code more efficient (though not vastly) to prune this way
	 * when there aren't route-throughs or ipin doglegs.                        */

#define NO_ROUTE_THROUGHS 1	/* Can't route through unused CLB outputs */
	no_route_throughs = 1;
	if (no_route_throughs == 1)
		sink_rt_node->re_expand = false;
	else {
		if (remaining_connections_to_sink == 0) { /* Usual case */
			sink_rt_node->re_expand = true;
		}

		/* Weird case.  This net connects several times to the same SINK.  Thus I
		 * can't re_expand this node as part of the partial routing for subsequent
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
	// NO_PREVIOUS tags a previously routed node

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

	/* Inode is the branch point to the old routing */
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

	/* Sets the R_upstream values of all the nodes in the new path to the
	 * correct value by traversing down to SINK from the start of the new path. */

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

	/* Note:  the traversal below makes use of the fact that this new path
	 * really is a path (not a tree with branches) to do a traversal without
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

static t_rt_node*
update_unbuffered_ancestors_C_downstream(t_rt_node * start_of_new_path_rt_node) {

	/* Updates the C_downstream values for the ancestors of the new path.  Once
	 * a buffered switch is found amongst the ancestors, no more ancestors are
	 * affected.  Returns the root of the "unbuffered subtree" whose Tdel
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

void load_route_tree_Tdel(t_rt_node * subtree_rt_root, float Tarrival) {

	/* Updates the Tdel values of the subtree rooted at subtree_rt_root by
	 * by calling itself recursively.  The C_downstream values of all the nodes
	 * must be correct before this routine is called.  Tarrival is the time at
	 * at which the signal arrives at this node's *input*.                      */

	int inode;
	short iswitch;
	t_rt_node *child_node;
	t_linked_rt_edge *linked_rt_edge;
	float Tdel, Tchild;

	inode = subtree_rt_root->inode;

	/* Assuming the downstream connections are, on average, connected halfway
	 * along a wire segment's length.  See discussion in net_delay.c if you want
	 * to change this.                                                           */

	Tdel = Tarrival + 0.5 * subtree_rt_root->C_downstream * rr_node[inode].R;
	subtree_rt_root->Tdel = Tdel;

	/* Now expand the children of this node to load their Tdel values (depth-
	 * first pre-order traversal).                                              */

	linked_rt_edge = subtree_rt_root->u.child_list;

	while (linked_rt_edge != NULL) {
		iswitch = linked_rt_edge->iswitch;
		child_node = linked_rt_edge->child;

		Tchild = Tdel + g_rr_switch_inf[iswitch].R * child_node->C_downstream;
		Tchild += g_rr_switch_inf[iswitch].Tdel; /* Intrinsic switch delay. */
		load_route_tree_Tdel(child_node, Tchild);

		linked_rt_edge = linked_rt_edge->next;
	}
}

void load_route_tree_rr_route_inf(t_rt_node* root) {

	/* Traverses down a route tree and updates rr_node_inf for all nodes
	 * to reflect that these nodes have already been routed to 			 */

	VTR_ASSERT(root != nullptr);

	t_linked_rt_edge* edge {root->u.child_list};
	
	for (;;) {
		int inode = root->inode;
		rr_node_route_inf[inode].prev_node = NO_PREVIOUS;
		rr_node_route_inf[inode].prev_edge = NO_PREVIOUS;
		// path cost should be HUGE_POSITIVE_FLOAT to indicate it's unset
		VTR_ASSERT_SAFE(equal_approx(rr_node_route_inf[inode].path_cost, HUGE_POSITIVE_FLOAT));

		// reached a sink
		if (!edge) {return;}
		// branch point (sibling edges)
		else if (edge->next) {
			// recursively update for each of its sibling branches
			do {
				load_route_tree_rr_route_inf(edge->child);
				edge = edge->next;
			} while (edge);
			return;
		}

		root = edge->child;
		edge = root->u.child_list;
	}	
}


static t_linked_rt_edge* free_route_subtree(t_rt_node* parent, t_linked_rt_edge* previous_edge) {

	/* Frees the parent's child under the edge after the previous edge
	 * returns the parent's next edge
	 *
	 * previous edge being null indicates the edge to be removed is the first edge
	 * caller beware, the input edge pointer is invalidated upon call
	 * for the next edge, use ONLY the return value 									*/

	t_linked_rt_edge* edge_to_be_removed;
	// first edge
	if (!previous_edge)	{
		edge_to_be_removed = parent->u.child_list;
		free_route_tree(edge_to_be_removed->child);
		parent->u.child_list = edge_to_be_removed->next;	// if also last edge, then child list becomes null

		free_linked_rt_edge(edge_to_be_removed);
		return parent->u.child_list;
	}

	// edge is not first edge, remove by skipping over it
	edge_to_be_removed = previous_edge->next;
	previous_edge->next = previous_edge->next->next;
	free_route_tree(edge_to_be_removed->child);
	free_linked_rt_edge(edge_to_be_removed);

	return previous_edge->next;	
}

void free_route_tree(t_rt_node * rt_node) {

	/* Puts the rt_nodes and edges in the tree rooted at rt_node back on the
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
		const t_rt_node* const * rt_node_of_sink, int inet) {

	/* Goes through all the sinks of this net and copies their delay values from
	 * the route_tree to the net_delay array.                                    */

	for (unsigned int isink = 1; isink < g_clbs_nlist.net[inet].pins.size(); isink++) {
		net_delay[isink] = rt_node_of_sink[isink]->Tdel;
	}
}

void update_remaining_net_delays_from_route_tree(float* net_delay, 
		const t_rt_node* const * rt_node_of_sink, const vector<int>& remaining_sinks) {

	/* Like update_net_delays_from_route_tree, but only updates the sinks that were not already routed
	 * this function doesn't actually need to know about the net, just what sink pins need their net delays updated */

	for (int sink_pin : remaining_sinks)
		net_delay[sink_pin] = rt_node_of_sink[sink_pin]->Tdel;
}


/***************  Conversion between traceback and route tree *******************/
t_rt_node* traceback_to_route_tree(int inet) {

	/* Builds a skeleton route tree from a traceback
	 * does not calculate R_upstream, C_downstream, or Tdel at all (left uninitialized)
	 * returns the root of the converted route tree
	 * initially points at the traceback equivalent of root 							  */

	t_trace* head {trace_head[inet]};
	// always called after the 1st iteration, so should exist
	VTR_ASSERT(head != nullptr);

	t_rt_node* rt_root {init_route_tree_to_source(inet)};

	t_rt_node* parent_node {rt_root};
	// prev edge is always a dangling edge, waiting to point to next child, always belongs to parent_node
	t_linked_rt_edge* prev_edge {alloc_linked_rt_edge()};	
	prev_edge->iswitch = head->iswitch;
	prev_edge->next = nullptr;
	rt_root->u.child_list = prev_edge;

	t_trace* tail;

	while (true) {	// while the last sink has not been reached (tree has not been fully drawn)
		// head is always on the trunk of the tree (explored part) while tail is always on a new branch
		tail = head->next;
		// while the next sink has not been reached (this branch has not been fully drawn)
		while (rr_node[tail->index].type != SINK) tail = tail->next; 
		// tail is at the end of the branch (SINK)
		// [head+1, tail(sink)] is the new part of the route tree
		for (t_trace* new_node = head->next; new_node != tail; new_node = new_node->next) {
			int inode {new_node->index};
			t_rt_node* rt_node = alloc_rt_node();
			rt_node->inode = inode;
			// 1 before tail is an IPIN; do not re_expand IPINs
			if (new_node->next == tail) rt_node->re_expand = false;
			else rt_node->re_expand = true;
			rt_node->parent_node = parent_node;
			rt_node->parent_switch = prev_edge->iswitch;

			// add to dangling edge (switch determined when edge was created)
			prev_edge->child = rt_node;
			// at this point parent and child should be set and mutual
			VTR_ASSERT(parent_node->u.child_list->child == rt_node);
			VTR_ASSERT(rt_node->parent_node == parent_node);
			// create new dangling edge belonging to the current node for its child (there exists one since new_node isn't tail (sink))
			prev_edge = alloc_linked_rt_edge();
			prev_edge->iswitch = new_node->iswitch;
			prev_edge->next = nullptr;
			rt_node->u.child_list = prev_edge;
			// hold onto rt_node since we will need it when it becomes the branch point for another branch
			rr_node_to_rt_node[inode] = rt_node;
			parent_node = rt_node;
		}
		// still need to add the sink to the route tree
		t_rt_node* rt_node = alloc_rt_node();
		rt_node->u.child_list = nullptr;	// no children for sink (edge is not allocated, need to do that outside for next branch)
		rt_node->inode = tail->index;
		rt_node->re_expand = false;	// do not re_expand SINKS
		rt_node->parent_switch = prev_edge->iswitch;
		rt_node->parent_node = parent_node;
		prev_edge->child = rt_node;

		// at this point parent and child should be set and mutual
		VTR_ASSERT(parent_node->u.child_list->child == rt_node);
		VTR_ASSERT(rt_node->parent_node == parent_node);

		rr_node_to_rt_node[tail->index] = rt_node;
		// terminates this series of edges/branch
		prev_edge = nullptr;

		// either another prexisting part of the route tree and more sinks to come or end
		head = tail->next;
		// end of trunk
		if (!head) return rt_root;
		// continue on the next branch (1 after a sink for the traceback data structure)
		// the rt_node has already been alocated as it's part of the trunk, make a new dangling edge for it
		parent_node = rr_node_to_rt_node[head->index];
		prev_edge = alloc_linked_rt_edge();
		prev_edge->iswitch = head->iswitch;
		// advance to trunk node's next edge
		prev_edge->next = parent_node->u.child_list;
		parent_node->u.child_list = prev_edge;
		// edge is still dangling in preparation for next sink
	}
}

static t_trace* traceback_branch_from_route_tree(t_trace* head, const t_rt_node* root, int& sinks_left) {

	/* Transforms route tree (supposed to be a branch of the full tree) into
	 * a traceback segement
	 * returns the **sink trace (tail)** 
	 * assumes head has been allocated with index set, but no iswitch or next */

	t_linked_rt_edge* edge {root->u.child_list};
	
	for (;;) {
		// head has been allocated at the start of this loop and has correct index

		// reached a sink
		if (!edge) {
			head->iswitch = OPEN;
			// inform caller that a sink was reached from there
			--sinks_left;
			// the caller will set the sink's next
			return head;	// head is the sink
		}
		// branch point (sibling edges)
		else if (edge->next) {
			// recursively update for each of its sibling branches
			t_trace* sink;
			do {
				head->iswitch = edge->iswitch;
				// a sub-branch off the current one, such that branch_head's parent is head
				t_trace* branch_head = alloc_trace_data();
				branch_head->index = edge->child->inode;
				head->next = branch_head;

				// the tip/end of the sub-branch
				sink = traceback_branch_from_route_tree(branch_head, edge->child, sinks_left);
				// danger here of losing a pointer if sink came from a branch point since sink->next would already be allocated
				// don't allocate for last sink (no branches after the last sink)
				if (sinks_left) {
					sink->next = alloc_trace_data();	
					// copy the information to the other instances (1 for each branch start) of the branch point
					sink->next->index = head->index;
					head = sink->next;	// become the since each branch might have different switches
				}
				edge = edge->next;
			} while (edge);
			return sink;	// returns the last ("rightmost") sink rooted at this branch point
		}
		// only 1 edge (and must be valid)

		head->iswitch = edge->iswitch;
		root = edge->child;
		edge = root->u.child_list;

		head->next = alloc_trace_data();
		// head is now the next head, along with root being the next root
		head = head->next;
		head->index = root->inode;
		// keep going down this path, so no return like the other cases of a sink or branch point

	}
}
t_trace* traceback_from_route_tree(int inet, const t_rt_node* root, int num_routed_sinks) {

	/* Creates the traceback for net inet from the route tree rooted at root
	 * properly sets trace_head and trace_tail for this net
	 * returns the trace head for inet 					 					 */

	t_trace* head {alloc_trace_data()};
	head->index = root->inode;
	// the very last sink
	auto tail = traceback_branch_from_route_tree(head, root, num_routed_sinks);
	// should've reached all existing sinks
	VTR_ASSERT(num_routed_sinks == 0);

	// tag end of traceback
	tail->next = nullptr;

	trace_tail[inet] = tail;
	trace_head[inet] = head;

	return head;
}


/***************  Pruning and fixing cost of skeleton route tree ****************/
static void find_sinks_of_route_tree(const t_rt_node* rt_root, CBRR& connections_inf) {

	/* Push back the node index of all sinks rooted at rt_root onto sink_nodes */

	t_linked_rt_edge* edge {rt_root->u.child_list};
	for (;;) {
		// reached a sink
		if (!edge) {
			if (connections_inf.should_force_reroute_connection(rt_root->inode)) {
				// rerouting this sink due to congestion, so reset the force reroute flag anyway
				connections_inf.clear_force_reroute_for_connection(rt_root->inode);
			}

			// put in remaining targets, marked as a sink that hasn't been reached legally yet
			connections_inf.toreach_rr_sink(rt_root->inode);
			return;
		}
		// branch point (sibling edges)
		else if (edge->next) {
			// recursively update for each of its sibling branches
			do {
				find_sinks_of_route_tree(edge->child, connections_inf);
				edge = edge->next;
			} while (edge);
			return;
		}

		rt_root = edge->child;
		edge = rt_root->u.child_list;
	}
}


static bool prune_illegal_branches_from_route_tree(t_rt_node* rt_root, float pres_fac, 
	CBRR& connections_inf, float R_upstream)  {

	/* as we traverse down, do: 
	 *	1. removes illegal branches from pathfinder costs
	 *	2. populate remaining targets with the illegally routed sinks
	 *	3. calculate R_upstream and C_downstream for entire subtree
	 *
	 *	It is the **caller's responsibility** to actually
	 *	call pathfinder_update_cost and free_route_subtree
	 *
	 *	The removal is only done at branch points for its branches
	 *	returns whether the entire tree was pruned or not
	 *	assumes arriving rt_root does not have its R_upstream calculated
	 *	Tdel for the entire tree after this function is still unset,
	 *	call load_route_tree_Tdel(rt_root, 0) at SOURCE 					*/

	VTR_ASSERT(rt_root != nullptr);

	auto inode = rt_root->inode;

	// illegal tree, propagate information back up (actual removal done at an upstream branch point)
	if (rr_node[inode].get_occ() > rr_node[inode].get_capacity()) return true;
	// legal routing, allowed to do calculations now

	// can calculate R_upstream from just upstream information without considering children
	auto parent_switch = rt_root->parent_switch;
	if (g_rr_switch_inf[parent_switch].buffered)
		R_upstream = g_rr_switch_inf[parent_switch].R + rr_node[inode].R;
	else
		R_upstream += g_rr_switch_inf[parent_switch].R + rr_node[inode].R;
	rt_root->R_upstream = R_upstream;

	auto edge = rt_root->u.child_list;
	// no edge means it must be a sink; legally routed to sink!
	if (!edge) {
		if (connections_inf.should_force_reroute_connection(inode)) {
			// forced the reroute, clear so future iterations don't get a stale flag
			connections_inf.clear_force_reroute_for_connection(inode);
			return true;
		}
		// don't need to reroute connection so can safely claim to have reached rt_sink
		else {
			rt_root->C_downstream = 0;	// sink has no node downstream and always has 0 own capacitance
			connections_inf.reached_rt_sink(rt_root);
			return false;			
		}
	}
	
	// prune children and get their C_downstream
	float C_downstream_children {0};

	t_linked_rt_edge* prev_edge {nullptr};
	do {
		bool pruned {prune_illegal_branches_from_route_tree(edge->child, pres_fac, connections_inf, R_upstream)};
		// need to remove edge and possibly change first edge if pruned
		if (pruned) {
			// optimization for long paths (no sibling edges), propagate illegality up to branch point
			if (!rt_root->u.child_list->next) {
				return true;
			}
			// else this is a branch point and the removal should be done here
			find_sinks_of_route_tree(edge->child, connections_inf);
			edge = free_route_subtree(rt_root, prev_edge);	// returns next edge
			// previous edge does not change when the current edge gets cut
		}
		else {
			// child still exists and has calculated C_downstream
			if (!g_rr_switch_inf[edge->iswitch].buffered)
				C_downstream_children += edge->child->C_downstream;
			prev_edge = edge;
			edge = edge->next; // advance normally as branch still exists
		}
	} while (edge);
	// tree must never be entirely pruned here due to propagating illegality up to branch point for paths
	VTR_ASSERT(rt_root->u.child_list != nullptr);

	// the sum of its children and its own capacitance
	rt_root->C_downstream = C_downstream_children + rr_node[inode].C;
	return false;
}


bool prune_route_tree(t_rt_node* rt_root, float pres_fac, CBRR& connections_inf) {

	/* Prune a skeleton route tree of illegal branches - when there is at least 1 congested node on the path to a sink
	 * This is the top level function to be called with the SOURCE node as root
	 * returns whether the entire tree has been pruned 
	 * 	
	 * assumes R_upstream is already preset for SOURCE, and skip checking parent switch (index of -1) */

	// SOURCE node should never be congested
	VTR_ASSERT(rt_root != nullptr);
	VTR_ASSERT(rr_node[rt_root->inode].get_occ() <= rr_node[rt_root->inode].get_capacity());

	// prune illegal branches from root
	auto edge = rt_root->u.child_list;
	t_linked_rt_edge* prev_edge {nullptr};
	float C_downstream_branches {0};

	while (edge) {
		bool pruned {prune_illegal_branches_from_route_tree(edge->child, pres_fac, connections_inf, 0)};
		if (pruned) {
			find_sinks_of_route_tree(edge->child, connections_inf);
			edge = free_route_subtree(rt_root, prev_edge);	// returns next edge
			// previous edge does not change when the current edge gets cut
		}
		else {
			// child still exists and has calculated C_downstream
			if (!g_rr_switch_inf[edge->iswitch].buffered)
				C_downstream_branches += edge->child->C_downstream;
			prev_edge = edge;
			edge = edge->next; // advance normally as branch still exists
		}
	}
	// for root this can happen if all child branches are pruned
	if (!rt_root->u.child_list) {
		return true;
	}

	rt_root->C_downstream += C_downstream_branches;
	return false;	
}


void pathfinder_update_cost_from_route_tree(const t_rt_node* rt_root, int add_or_sub, float pres_fac) {

	/* Update pathfinder cost of all nodes rooted at rt_root, including rt_root itself */

	VTR_ASSERT(rt_root != nullptr);

	t_linked_rt_edge* edge {rt_root->u.child_list};
	
	// update every node once, so even do it for sinks and branch points once
	for (;;) {
		pathfinder_update_single_node_cost(rt_root->inode, add_or_sub, pres_fac);

		// reached a sink
		if (!edge) {
			return;
		}
		// branch point (sibling edges)
		else if (edge->next) {
			// recursively update for each of its sibling branches
			do {
				pathfinder_update_cost_from_route_tree(edge->child, add_or_sub, pres_fac);
				edge = edge->next;
			} while (edge);
			return;
		}

		rt_root = edge->child;
		edge = rt_root->u.child_list;
	}
}



/***************** Debugging and printing for incremental rerouting ****************/
template <typename Op>
static void traverse_indented_route_tree(const t_rt_node* rt_root, int branch_level, bool new_branch, Op op, int indent_level) {

	/* pretty print the route tree; what's printed depends on the printer Op passed in */

	// rely on preorder depth first traversal
	VTR_ASSERT(rt_root != nullptr);
	t_linked_rt_edge* edges = rt_root->u.child_list;
	// print branch indent
	if (new_branch) vtr::printf_info("\n%*s", indent_level*branch_level, " \\ ");

	op(rt_root);
	// reached sink, move onto next branch
	if (!edges) return;
	// branch point, has sibling edge
	else if (edges->next) {
		bool first_branch = true;
		do {
			// don't print a new line for the first branch
			traverse_indented_route_tree(edges->child, branch_level + 1, !first_branch, op, indent_level);
			edges = edges->next;
			first_branch = false;
		} while (edges);
	}
	// along a path, just propagate down
	else {
		traverse_indented_route_tree(edges->child, branch_level + 1, false, op, indent_level);
	}
}


void print_edge(const t_linked_rt_edge* edge) {
	vtr::printf_info("edges to ");
	if (!edge) {vtr::printf_info("null"); return;}
	while (edge) {
		vtr::printf_info("%d(%d) ", edge->child->inode, edge->iswitch);
		edge = edge->next;
	}
	vtr::printf_info("\n");
}


static void print_node(const t_rt_node* rt_node) {
	int inode = rt_node->inode;
	t_rr_type node_type = rr_node[inode].type;
	vtr::printf_info("%5.1e %5.1e %2d%6s|%-6d-> ", rt_node->C_downstream, rt_node->R_upstream,
		rt_node->re_expand, node_typename[node_type], inode);	
}


static void print_node_inf(const t_rt_node* rt_node) {
	int inode = rt_node->inode;
	const auto& node_inf = rr_node_route_inf[inode];
	vtr::printf_info("%5.1e %5.1e%6d%3d|%-6d-> ", node_inf.path_cost, node_inf.backward_path_cost,
		node_inf.prev_node, node_inf.prev_edge, inode);	
}


static void print_node_congestion(const t_rt_node* rt_node) {
	int inode = rt_node->inode;
	const auto& node_inf = rr_node_route_inf[inode];
	const auto& node = rr_node[inode];
	vtr::printf_info("%2d %2d|%-6d-> ", node_inf.pres_cost, rt_node->Tdel,
		node.get_occ(), node.get_capacity(), inode);		
}


void print_route_tree_inf(const t_rt_node* rt_root) {
	traverse_indented_route_tree(rt_root, 0, false, print_node_inf, 34);
	vtr::printf_info("\n");
}

void print_route_tree(const t_rt_node* rt_root) {
	traverse_indented_route_tree(rt_root, 0, false, print_node, 34);
	vtr::printf_info("\n");
}

void print_route_tree_congestion(const t_rt_node* rt_root) {
	traverse_indented_route_tree(rt_root, 0, false, print_node_congestion, 15);
	vtr::printf_info("\n");
}

/* the following is_* functions are for debugging correctness of pruned route tree 
   these should only be called when the debug switch DEBUG_INCREMENTAL_REROUTING is on */
bool is_equivalent_route_tree(const t_rt_node* root, const t_rt_node* root_clone) {
	if (!root && !root_clone) return true;
	if (!root || !root_clone) return false;	// one of them is null
	if ((root->inode != root_clone->inode) ||
		(!equal_approx(root->R_upstream, root_clone->R_upstream)) ||
		(!equal_approx(root->C_downstream, root_clone->C_downstream)) ||
		(!equal_approx(root->Tdel, root_clone->Tdel))) {
		vtr::printf_info("mismatch i %d|%d R %e|%e C %e|%e T %e %e\n", 
			root->inode, root_clone->inode,
			root->R_upstream, root_clone->R_upstream, 
			root->C_downstream, root_clone->C_downstream,
			root->Tdel, root_clone->Tdel);
		return false;
	}
	t_linked_rt_edge* orig_edge {root->u.child_list};
	t_linked_rt_edge* clone_edge {root_clone->u.child_list};
	while (orig_edge && clone_edge) {
		if (orig_edge->iswitch != clone_edge->iswitch)
			vtr::printf_info("mismatch i %d|%d edge switch %d|%d\n", 
				root->inode, root_clone->inode,
				orig_edge->iswitch, clone_edge->iswitch);
		if (!is_equivalent_route_tree(orig_edge->child, clone_edge->child)) return false;	// child trees not equivalent
		orig_edge = orig_edge->next;
		clone_edge = clone_edge->next;
	}
	if (orig_edge || clone_edge) {
		vtr::printf_info("one of the trees have an extra edge!\n");
		return false;
	}
	return true;	// passed all tests
}

// check only the connections are correct, ignore R and C
bool is_valid_skeleton_tree(const t_rt_node* root) {
	int inode = root->inode;
	t_linked_rt_edge* edge = root->u.child_list;
	while (edge) {
		if (edge->child->parent_node != root) {
			vtr::printf_info("parent-child relationship not mutually acknowledged by parent %d->%d child %d<-%d\n", 
				inode, edge->child->inode,
				edge->child->inode, edge->child->parent_node->inode);
			return false;
		}
		if (edge->iswitch != edge->child->parent_switch) {
			vtr::printf_info("parent(%d)-child(%d) connected switch not equivalent parent %d child %d\n", 
				inode, edge->child->inode, edge->iswitch, edge->child->parent_switch);
			return false;
		}

		if (!is_valid_skeleton_tree(edge->child)) {
			vtr::printf_info("subtree %d invalid, propagating up\n", edge->child->inode);
			return false;
		}
		edge = edge->next;
	}
	return true;	
}

bool is_valid_route_tree(const t_rt_node* root) {
	// check upstream resistance
	int inode = root->inode;
	short iswitch = root->parent_switch;
	if (root->parent_node) {
		if (g_rr_switch_inf[iswitch].buffered) {
			if (root->R_upstream != rr_node[inode].R + g_rr_switch_inf[iswitch].R) {
				vtr::printf_info("%d mismatch R upstream %e supposed %e\n", inode, root->R_upstream, 
					rr_node[inode].R + g_rr_switch_inf[iswitch].R);
				return false;
			}
		}
		else if (root->R_upstream != rr_node[inode].R + root->parent_node->R_upstream + g_rr_switch_inf[iswitch].R) {
			vtr::printf_info("%d mismatch R upstream %e supposed %e\n", inode, root->R_upstream, 
				rr_node[inode].R + root->parent_node->R_upstream + g_rr_switch_inf[iswitch].R);
			return false;
		}
	}
	else if (root->R_upstream != rr_node[inode].R) {
		vtr::printf_info("%d mismatch R upstream %e supposed %e\n", inode, root->R_upstream, rr_node[inode].R);
		return false;
	}

	// check downstream C
	t_linked_rt_edge* edge = root->u.child_list;
	float C_downstream_children {0};
	// sink, must not be congested
	if (!edge) {
		int occ = rr_node[inode].get_occ();
		int capacity = rr_node[inode].get_capacity();
		if (occ > capacity) {
			vtr::printf_info("SINK %d occ %d > cap %d\n", inode, occ, capacity);
			return false;
		}
	}
	while (edge) {
		if (edge->child->parent_node != root) {
			vtr::printf_info("parent-child relationship not mutually acknowledged by parent %d->%d child %d<-%d\n", 
				inode, edge->child->inode,
				edge->child->inode, edge->child->parent_node->inode);
			return false;
		}
		if (edge->iswitch != edge->child->parent_switch) {
			vtr::printf_info("parent(%d)-child(%d) connected switch not equivalent parent %d child %d\n", 
				inode, edge->child->inode, edge->iswitch, edge->child->parent_switch);
			return false;
		}

		if (!g_rr_switch_inf[edge->iswitch].buffered)
			C_downstream_children += edge->child->C_downstream;

		if (!is_valid_route_tree(edge->child)) {
			vtr::printf_info("subtree %d invalid, propagating up\n", edge->child->inode);
			return false;
		}
		edge = edge->next;
	}

	if (root->C_downstream != C_downstream_children + rr_node[inode].C) {
		vtr::printf_info("mismatch C downstream %e supposed %e\n", root->C_downstream, C_downstream_children + rr_node[inode].C);
		return false;
	}

	return true;
}

bool is_uncongested_route_tree(const t_rt_node* root) {
	// make sure the nodes are legally connected
	t_linked_rt_edge* edge {root->u.child_list};
	
	for (;;) {
		int inode = root->inode;
		if (rr_node[inode].get_occ() > rr_node[inode].get_capacity()) {
			// vtr::printf_info("node %d occ %d > cap %d\n", inode, rr_node[inode].get_occ(), rr_node[inode].get_capacity());
			return false;
		}

		// reached a sink
		if (!edge) {
			return true;
		}
		// branch point (sibling edges)
		else if (edge->next) {
			// recursively update for each of its sibling branches
			do {
				if (!is_uncongested_route_tree(edge->child)) return false;
				edge = edge->next;
			} while (edge);
			return true;
		}

		root = edge->child;
		edge = root->u.child_list;
	}
}
