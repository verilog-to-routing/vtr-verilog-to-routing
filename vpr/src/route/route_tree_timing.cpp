#include <cstdio>
#include <cmath>
#include <vector>
using namespace std;

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_memory.h"

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

static std::vector<t_rt_node *> rr_node_to_rt_node; /* [0..device_ctx.rr_nodes.size()-1] */


/* Frees lists for fast addition and deletion of nodes and edges. */

static t_rt_node *rt_node_free_list = nullptr;
static t_linked_rt_edge *rt_edge_free_list = nullptr;

/********************** Subroutines local to this module *********************/

static t_rt_node *alloc_rt_node();

static void free_rt_node(t_rt_node * rt_node);

static t_linked_rt_edge *alloc_linked_rt_edge();

static void free_linked_rt_edge(t_linked_rt_edge * rt_edge);

static t_rt_node *add_subtree_to_route_tree(t_heap *hptr,
		t_rt_node ** sink_rt_node_ptr);

static t_rt_node* add_non_configurable_to_route_tree(const int rr_node, const bool reached_by_non_configurable_edge, std::unordered_set<int>& visited);

static t_rt_node *update_unbuffered_ancestors_C_downstream(
		t_rt_node * start_of_new_subtree_rt_node);

bool verify_route_tree_recurr(t_rt_node* node, std::set<int>& seen_nodes);

t_rt_node* prune_route_tree_recurr(t_rt_node* node, CBRR& connections_inf, bool congested);

t_rt_node* traceback_to_route_tree(t_trace* head);
static t_trace* traceback_to_route_tree_branch(t_trace* trace, std::map<int,t_rt_node*>& rr_node_to_rt);

static std::pair<t_trace*,t_trace*> traceback_from_route_tree_recurr(t_trace* head, t_trace* tail, const t_rt_node* node);

void collect_route_tree_connections(const t_rt_node* node, std::set<std::tuple<int,int,int>>& connections);
/************************** Subroutine definitions ***************************/

constexpr float epsilon = 1e-15;
static bool equal_approx(float a, float b) {
	return fabs(a - b) < epsilon;
}

bool alloc_route_tree_timing_structs(bool exists_ok) {

	/* Allocates any structures needed to build the routing trees. */

    auto& device_ctx = g_vpr_ctx.device();

    bool route_tree_structs_are_allocated =
        (rr_node_to_rt_node.size() == size_t(device_ctx.rr_nodes.size())
			                                 || rt_node_free_list != nullptr);
    if (route_tree_structs_are_allocated) {
        if (exists_ok) {
            return false;
        } else {
                vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                    "in alloc_route_tree_timing_structs: old structures already exist.\n");
        }
	}

	rr_node_to_rt_node = std::vector<t_rt_node*>(device_ctx.rr_nodes.size(), nullptr);

    return true;
}

void free_route_tree_timing_structs() {

	/* Frees the structures needed to build routing trees, and really frees
	 * (i.e. calls free) all the data on the free lists.                         */

	t_rt_node *rt_node, *next_node;
	t_linked_rt_edge *rt_edge, *next_edge;

	rr_node_to_rt_node.clear();

	rt_node = rt_node_free_list;

	while (rt_node != nullptr) {
		next_node = rt_node->u.next;
		free(rt_node);
		rt_node = next_node;
	}

	rt_node_free_list = nullptr;

	rt_edge = rt_edge_free_list;

	while (rt_edge != nullptr) {
		next_edge = rt_edge->next;
		free(rt_edge);
		rt_edge = next_edge;
	}

	rt_edge_free_list = nullptr;
}

static t_rt_node*
alloc_rt_node() {

	/* Allocates a new rt_node, from the free list if possible, from the free
	 * store otherwise.                                                         */

	t_rt_node *rt_node;

	rt_node = rt_node_free_list;

	if (rt_node != nullptr) {
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
alloc_linked_rt_edge() {

	/* Allocates a new linked_rt_edge, from the free list if possible, from the
	 * free store otherwise.                                                     */

	t_linked_rt_edge *linked_rt_edge;

	linked_rt_edge = rt_edge_free_list;

	if (linked_rt_edge != nullptr) {
		rt_edge_free_list = linked_rt_edge->next;
	} else {
		linked_rt_edge = (t_linked_rt_edge *) vtr::malloc(
				sizeof(t_linked_rt_edge));
	}

	VTR_ASSERT(linked_rt_edge != nullptr);
	return (linked_rt_edge);
}

/* Adds the rt_edge to the rt_edge free list.                       */
static void free_linked_rt_edge(t_linked_rt_edge * rt_edge) {
	rt_edge->next = rt_edge_free_list;
	rt_edge_free_list = rt_edge;
}

/* Initializes the routing tree to just the net source, and returns the root
* node of the rt_tree (which is just the net source).                       */
t_rt_node* init_route_tree_to_source(ClusterNetId inet) {
	t_rt_node *rt_root;
	int inode;

    auto& route_ctx = g_vpr_ctx.routing();
    auto& device_ctx = g_vpr_ctx.device();

	rt_root = alloc_rt_node();
	rt_root->u.child_list = nullptr;
	rt_root->parent_node = nullptr;
	rt_root->parent_switch = OPEN;
	rt_root->re_expand = true;

	inode = route_ctx.net_rr_terminals[inet][0]; /* Net source */

	rt_root->inode = inode;
	rt_root->C_downstream = device_ctx.rr_nodes[inode].C();
	rt_root->R_upstream = device_ctx.rr_nodes[inode].R();
	rt_root->Tdel = 0.5 * device_ctx.rr_nodes[inode].R() * device_ctx.rr_nodes[inode].C();
	rr_node_to_rt_node[inode] = rt_root;

	return (rt_root);
}

/* Adds the most recently finished wire segment to the routing tree, and
* updates the Tdel, etc. numbers for the rest of the routing tree.  hptr
* is the heap pointer of the SINK that was reached.  This routine returns
* a pointer to the rt_node of the SINK that it adds to the routing.        */
t_rt_node* update_route_tree(t_heap * hptr) {
	t_rt_node *start_of_new_subtree_rt_node, *sink_rt_node;
	t_rt_node *unbuffered_subtree_rt_root, *subtree_parent_rt_node;
	float Tdel_start;
	short iswitch;

    auto& device_ctx = g_vpr_ctx.device();

    //Create a new subtree from the target in hptr to existing routing
	start_of_new_subtree_rt_node = add_subtree_to_route_tree(hptr, &sink_rt_node);

    //Propagate R_upstream down into the new subtree
	load_new_subtree_R_upstream(start_of_new_subtree_rt_node);

    //Propagate C_downstream up from new subtree sinks to subtree root
    load_new_subtree_C_downstream(start_of_new_subtree_rt_node);

    //Propagate C_downstream up from the subtree root
	unbuffered_subtree_rt_root = update_unbuffered_ancestors_C_downstream(
			start_of_new_subtree_rt_node);

	subtree_parent_rt_node = unbuffered_subtree_rt_root->parent_node;

	if (subtree_parent_rt_node != nullptr) { /* Parent exists. */
		Tdel_start = subtree_parent_rt_node->Tdel;
		iswitch = unbuffered_subtree_rt_root->parent_switch;
		Tdel_start += device_ctx.rr_switch_inf[iswitch].R * unbuffered_subtree_rt_root->C_downstream;
		Tdel_start += device_ctx.rr_switch_inf[iswitch].Tdel;
	} else { /* Subtree starts at SOURCE */
		Tdel_start = 0.;
	}

	load_route_tree_Tdel(unbuffered_subtree_rt_root, Tdel_start);

	return (sink_rt_node);
}

void add_route_tree_to_rr_node_lookup(t_rt_node* node) {
    if (node) {
        VTR_ASSERT(rr_node_to_rt_node[node->inode] == nullptr || rr_node_to_rt_node[node->inode] == node);

        rr_node_to_rt_node[node->inode] = node;

        for (auto edge = node->u.child_list; edge != nullptr; edge = edge->next) {
            add_route_tree_to_rr_node_lookup(edge->child);
        }
    }
}

static t_rt_node*
add_subtree_to_route_tree(t_heap *hptr, t_rt_node ** sink_rt_node_ptr) {

	/* Adds the most recent wire segment, ending at the SINK indicated by hptr,
	 * to the routing tree.  It returns the first (most upstream) new rt_node,
	 * and (via a pointer) the rt_node of the new SINK. Traverses up from SINK  */

	int inode;
	short iedge, iswitch = -1;
	t_rt_node *rt_node, *downstream_rt_node, *sink_rt_node;
	t_linked_rt_edge *linked_rt_edge;

    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.routing();

	inode = hptr->index;

	if (device_ctx.rr_nodes[inode].type() != SINK) {
		vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
			"in add_subtree_to_route_tree. Expected type = SINK (%d).\n"
			"Got type = %d.",  SINK, device_ctx.rr_nodes[inode].type());
	}

	sink_rt_node = alloc_rt_node();
	sink_rt_node->u.child_list = nullptr;
	sink_rt_node->inode = inode;
	rr_node_to_rt_node[inode] = sink_rt_node;

	/* In the code below I'm marking SINKs and IPINs as not to be re-expanded.
	 * It makes the code more efficient (though not vastly) to prune this way
	 * when there aren't route-throughs or ipin doglegs.                        */

    sink_rt_node->re_expand = false;

	/* Now do it's predecessor. */

	downstream_rt_node = sink_rt_node;

    std::unordered_set<int> main_branch_visited;
    for (t_heap_prev prev : hptr->previous) {
        inode = prev.from_node;
        iedge = prev.from_edge;
        iswitch = device_ctx.rr_nodes[inode].edge_switch(iedge);

        /* For all "new" nodes in the main path */
        // inode is node index of previous node
        // NO_PREVIOUS tags a previously routed node

        while (rr_node_to_rt_node[inode] == nullptr) { //Not connected to existing routing
            main_branch_visited.insert(inode);

            linked_rt_edge = alloc_linked_rt_edge();
            linked_rt_edge->child = downstream_rt_node;
            linked_rt_edge->iswitch = iswitch;
            linked_rt_edge->next = nullptr;

            rt_node = alloc_rt_node();
            downstream_rt_node->parent_node = rt_node;
            downstream_rt_node->parent_switch = iswitch;

            rt_node->u.child_list = linked_rt_edge;
            rt_node->inode = inode;

            rr_node_to_rt_node[inode] = rt_node;

            if (device_ctx.rr_nodes[inode].type() == IPIN) {
                rt_node->re_expand = false;
            } else {
                rt_node->re_expand = true;
            }

            downstream_rt_node = rt_node;
            iedge = route_ctx.rr_node_route_inf[inode].prev_edge;
            inode = route_ctx.rr_node_route_inf[inode].prev_node;
            iswitch = device_ctx.rr_nodes[inode].edge_switch(iedge);
        }
    }

	//Inode is now the branch point to the old routing; do not need
    //to alloc another node since the old routing has done so already
	rt_node = rr_node_to_rt_node[inode];
    VTR_ASSERT_MSG(rt_node, "Previous routing branch should exist");

	linked_rt_edge = alloc_linked_rt_edge();
	linked_rt_edge->child = downstream_rt_node;
	linked_rt_edge->iswitch = iswitch;
	linked_rt_edge->next = rt_node->u.child_list; //Add to head
	rt_node->u.child_list = linked_rt_edge;

	downstream_rt_node->parent_node = rt_node;
	downstream_rt_node->parent_switch = iswitch;

    //Expand (recursively) each of the main-branch nodes adding any
    //non-configurably connected nodes
    std::unordered_set<int> all_visited = main_branch_visited;
    for (int rr_node : main_branch_visited) {
        add_non_configurable_to_route_tree(rr_node, false, all_visited);
    }

	*sink_rt_node_ptr = sink_rt_node;
	return (downstream_rt_node);
}

static t_rt_node* add_non_configurable_to_route_tree(const int rr_node, const bool reached_by_non_configurable_edge, std::unordered_set<int>& visited) {
    t_rt_node* rt_node = nullptr;

    if (!visited.count(rr_node) || !reached_by_non_configurable_edge) {
        visited.insert(rr_node);

        auto& device_ctx = g_vpr_ctx.device();

        rt_node = rr_node_to_rt_node[rr_node];

        if (!reached_by_non_configurable_edge) { //An existing main branch node
            VTR_ASSERT(rt_node);
        } else {
            VTR_ASSERT(reached_by_non_configurable_edge);

            if (!rt_node) {
                rt_node = alloc_rt_node();
                rt_node->u.child_list = nullptr;
                rt_node->inode = rr_node;

                if (device_ctx.rr_nodes[rr_node].type() == IPIN) {
                    rt_node->re_expand = false;
                } else {
                    rt_node->re_expand = true;
                }
            } else {
                VTR_ASSERT(rt_node->inode == rr_node);
            }
        }

        for (int iedge : device_ctx.rr_nodes[rr_node].non_configurable_edges()) {
            //Recursive case: expand children
            VTR_ASSERT (!device_ctx.rr_nodes[rr_node].edge_is_configurable(iedge));

            int to_rr_node = device_ctx.rr_nodes[rr_node].edge_sink_node(iedge);

            //Recurse
            t_rt_node* child_rt_node = add_non_configurable_to_route_tree(to_rr_node, true, visited);

            if (!child_rt_node) continue;

            int iswitch = device_ctx.rr_nodes[rr_node].edge_switch(iedge);

            //Create the edge
            t_linked_rt_edge* linked_rt_edge = alloc_linked_rt_edge();
            linked_rt_edge->child = child_rt_node;
            linked_rt_edge->iswitch = iswitch;

            //Add edge at head of parent linked list
            linked_rt_edge->next = rt_node->u.child_list;
            rt_node->u.child_list = linked_rt_edge;

            //Update child to parent ref
            child_rt_node->parent_node = rt_node;
            child_rt_node->parent_switch = iswitch;
        }
        rr_node_to_rt_node[rr_node] = rt_node;
    }

    return rt_node;
}

void load_new_subtree_R_upstream(t_rt_node* rt_node) {

	/* Sets the R_upstream values of all the nodes in the new path to the
	 * correct value by traversing down to SINK from the start of the new path. */

    if (!rt_node) {
        return;
    }

    auto& device_ctx = g_vpr_ctx.device();

    t_rt_node* parent_rt_node = rt_node->parent_node;
    int inode = rt_node->inode;

    //Calculate upstream resistance
    float R_upstream = 0.;
    if (parent_rt_node) {
        int iswitch = rt_node->parent_switch;
        bool switch_buffered = device_ctx.rr_switch_inf[iswitch].buffered();

        if (!switch_buffered) {
                R_upstream += parent_rt_node->R_upstream; //Parent upstream R
        }
        R_upstream += device_ctx.rr_switch_inf[iswitch].R; //Parent switch R
    }
    R_upstream += device_ctx.rr_nodes[inode].R(); //Current node R

    rt_node->R_upstream = R_upstream;

    //Update children
    for (t_linked_rt_edge* edge = rt_node->u.child_list; edge != nullptr; edge = edge->next) {
        load_new_subtree_R_upstream(edge->child); //Recurse
    }
}


float load_new_subtree_C_downstream(t_rt_node* rt_node) {
    float C_downstream = 0.;

    if (rt_node) {
        auto& device_ctx = g_vpr_ctx.device();

        C_downstream += device_ctx.rr_nodes[rt_node->inode].C();
        for (t_linked_rt_edge* edge = rt_node->u.child_list; edge != nullptr; edge = edge->next) {

            float C_downstream_child = load_new_subtree_C_downstream(edge->child);

            if (!device_ctx.rr_switch_inf[edge->iswitch].buffered()) {
                C_downstream += C_downstream_child;
            }
        }

        rt_node->C_downstream = C_downstream;
    }

    return C_downstream;
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

    auto& device_ctx = g_vpr_ctx.device();

	rt_node = start_of_new_path_rt_node;
	C_downstream_addition = rt_node->C_downstream;
	parent_rt_node = rt_node->parent_node;
	iswitch = rt_node->parent_switch;

	while (parent_rt_node != nullptr && device_ctx.rr_switch_inf[iswitch].buffered() == false) {
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

    auto& device_ctx = g_vpr_ctx.device();

	inode = subtree_rt_root->inode;

	/* Assuming the downstream connections are, on average, connected halfway
	 * along a wire segment's length.  See discussion in net_delay.c if you want
	 * to change this.                                                           */

	Tdel = Tarrival + 0.5 * subtree_rt_root->C_downstream * device_ctx.rr_nodes[inode].R();
	subtree_rt_root->Tdel = Tdel;

	/* Now expand the children of this node to load their Tdel values (depth-
	 * first pre-order traversal).                                              */

	linked_rt_edge = subtree_rt_root->u.child_list;

	while (linked_rt_edge != nullptr) {
		iswitch = linked_rt_edge->iswitch;
		child_node = linked_rt_edge->child;

		Tchild = Tdel + device_ctx.rr_switch_inf[iswitch].R * child_node->C_downstream;
		Tchild += device_ctx.rr_switch_inf[iswitch].Tdel; /* Intrinsic switch delay. */
		load_route_tree_Tdel(child_node, Tchild);

		linked_rt_edge = linked_rt_edge->next;
	}
}

void load_route_tree_rr_route_inf(t_rt_node* root) {

	/* Traverses down a route tree and updates rr_node_inf for all nodes
	 * to reflect that these nodes have already been routed to 			 */

	VTR_ASSERT(root != nullptr);

    auto& route_ctx = g_vpr_ctx.mutable_routing();

	t_linked_rt_edge* edge {root->u.child_list};

	for (;;) {
		int inode = root->inode;
		route_ctx.rr_node_route_inf[inode].prev_node = NO_PREVIOUS;
		route_ctx.rr_node_route_inf[inode].prev_edge = NO_PREVIOUS;
		// path cost should be unset
		VTR_ASSERT(std::isinf(route_ctx.rr_node_route_inf[inode].path_cost));
		VTR_ASSERT(std::isinf(route_ctx.rr_node_route_inf[inode].backward_path_cost));

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

bool verify_route_tree(t_rt_node* root) {

    std::set<int> seen_nodes;
    return verify_route_tree_recurr(root, seen_nodes);
}

bool verify_route_tree_recurr(t_rt_node* node, std::set<int>& seen_nodes) {
    if (seen_nodes.count(node->inode)) {
        VPR_THROW(VPR_ERROR_ROUTE, "Duplicate route tree nodes found for node %d", node->inode);
    }

    seen_nodes.insert(node->inode);

    for (t_linked_rt_edge* edge = node->u.child_list; edge != nullptr; edge = edge->next) {
        verify_route_tree_recurr(edge->child, seen_nodes);
    }
    return true;
}

void free_route_tree(t_rt_node * rt_node) {

	/* Puts the rt_nodes and edges in the tree rooted at rt_node back on the
	 * free lists.  Recursive, depth-first post-order traversal.                */

	t_linked_rt_edge *rt_edge, *next_edge;

	rt_edge = rt_node->u.child_list;

	while (rt_edge != nullptr) { /* For all children */
		t_rt_node* child_node = rt_edge->child;
        free_route_tree(child_node);
		next_edge = rt_edge->next;
		free_linked_rt_edge(rt_edge);
		rt_edge = next_edge;
	}

    rr_node_to_rt_node[rt_node->inode] = nullptr;
	free_rt_node(rt_node);
}

void print_route_tree(const t_rt_node* rt_node, int depth) {
    std::string indent;
    for (int i = 0; i < depth; ++i) {
        indent += "  ";
    }

    auto& device_ctx = g_vpr_ctx.device();
    VTR_LOG("%srt_node: %d (%s)", indent.c_str(), rt_node->inode, device_ctx.rr_nodes[rt_node->inode].type_string());

    if (rt_node->parent_switch != OPEN) {
        bool parent_edge_configurable = device_ctx.rr_switch_inf[rt_node->parent_switch].configurable();
        if (!parent_edge_configurable) {
            VTR_LOG("*");
        }
    }

    auto& route_ctx = g_vpr_ctx.routing();

    if (route_ctx.rr_node_route_inf[rt_node->inode].occ() > device_ctx.rr_nodes[rt_node->inode].capacity()) {
        VTR_LOG(" x");
    }

    VTR_LOG("\n");

    for (t_linked_rt_edge* rt_edge = rt_node->u.child_list; rt_edge != nullptr; rt_edge = rt_edge->next) {
        print_route_tree(rt_edge->child, depth + 1);
    }
}

void update_net_delays_from_route_tree(float *net_delay,
		const t_rt_node* const * rt_node_of_sink, ClusterNetId inet) {

	/* Goes through all the sinks of this net and copies their delay values from
	 * the route_tree to the net_delay array.                                    */

    auto& cluster_ctx = g_vpr_ctx.clustering();
	for (unsigned int isink = 1; isink < cluster_ctx.clb_nlist.net_pins(inet).size(); isink++) {
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
t_rt_node* traceback_to_route_tree(ClusterNetId inet) {
    auto& route_ctx = g_vpr_ctx.routing();
    return traceback_to_route_tree(route_ctx.trace_head[inet]);
}

t_rt_node* traceback_to_route_tree(t_trace* head) {

	/* Builds a skeleton route tree from a traceback
	 * does not calculate R_upstream, C_downstream, or Tdel at all (left uninitialized)
	 * returns the root of the converted route tree
	 * initially points at the traceback equivalent of root 							  */

    if (head == nullptr) {
        return nullptr;
    }

    VTR_ASSERT_DEBUG(validate_traceback(head));

    std::map<int,t_rt_node*> rr_node_to_rt;

    t_trace* trace = head;
    while (trace) { //Each branch
        trace = traceback_to_route_tree_branch(trace, rr_node_to_rt);
    }

    return rr_node_to_rt[head->index];
}

//Constructs a single branch of a route tree from a traceback
//Note that R_upstream and C_downstream are initialized to NaN
//
//Returns the t_trace defining the start of the next branch
static t_trace* traceback_to_route_tree_branch(t_trace* trace, std::map<int,t_rt_node*>& rr_node_to_rt) {

    t_trace* next = nullptr;

    if (trace) {
        t_rt_node* node = nullptr;

        int inode = trace->index;
        int iswitch = trace->iswitch;

        auto itr = rr_node_to_rt.find(trace->index);
        if (itr == rr_node_to_rt.end()) {
            //Create

            //Initialize route tree node
            node = alloc_rt_node();
            node->inode = inode;
            node->u.child_list = nullptr;

            node->R_upstream = std::numeric_limits<float>::quiet_NaN();
            node->C_downstream = std::numeric_limits<float>::quiet_NaN();
            node->Tdel = std::numeric_limits<float>::quiet_NaN();

            auto& device_ctx = g_vpr_ctx.device();
            auto node_type = device_ctx.rr_nodes[inode].type();
            if (node_type == IPIN || node_type == SINK) node->re_expand = false;
            else node->re_expand = true;

            //Save
            rr_node_to_rt[inode] = node;
        } else {
            //Found
            node = itr->second;
        }
        VTR_ASSERT(node);

        next = trace->next;
        if (iswitch != OPEN) {

            //Recursively construct the remaining branch
            next = traceback_to_route_tree_branch(next, rr_node_to_rt);

            //Get the created child
            itr = rr_node_to_rt.find(trace->next->index);
            VTR_ASSERT_MSG(itr != rr_node_to_rt.end(), "Child must exist");
            t_rt_node* child = itr->second;

            //Create the edge
            t_linked_rt_edge* edge = alloc_linked_rt_edge();
            edge->iswitch = trace->iswitch;
            edge->child = child;
            edge->next = nullptr;

            //Insert edge at tail of list
            edge->next = node->u.child_list;
            node->u.child_list = edge;

            //Set child -> parent ref's
            child->parent_node = node;
            child->parent_switch = iswitch;

            //Parent and child should be mutual
            VTR_ASSERT(node->u.child_list->child == child);
            VTR_ASSERT(child->parent_node == node);
        }
    }
    return next;
}

static std::pair<t_trace*,t_trace*> traceback_from_route_tree_recurr(t_trace* head, t_trace* tail, const t_rt_node* node) {

    if (node) {
        if (node->u.child_list) {
            //Recursively add children
            for (t_linked_rt_edge* edge = node->u.child_list; edge != nullptr; edge = edge->next) {
                t_trace* curr = alloc_trace_data();
                curr->index = node->inode;
                curr->iswitch = edge->iswitch;
                curr->next = nullptr;


                if (tail) {
                    VTR_ASSERT(tail->next == nullptr);
                    tail->next = curr;
                }

                tail = curr;

                if (!head) {
                    head = tail;
                }

                std::tie(head, tail) = traceback_from_route_tree_recurr(head, tail, edge->child);
            }
        } else {
            //Leaf
            t_trace* curr = alloc_trace_data();
            curr->index = node->inode;
            curr->iswitch = OPEN;
            curr->next = nullptr;

            if (tail) {
                VTR_ASSERT(tail->next == nullptr);
                tail->next = curr;
            }

            tail = curr;

            if (!head) {
                head = tail;
            }
        }
    }


    return {head,tail};
}

t_trace* traceback_from_route_tree(ClusterNetId inet, const t_rt_node* root, int num_routed_sinks) {

	/* Creates the traceback for net inet from the route tree rooted at root
	 * properly sets route_ctx.trace_head and route_ctx.trace_tail for this net
	 * returns the trace head for inet 					 					 */

    auto& route_ctx = g_vpr_ctx.mutable_routing();
    auto& device_ctx = g_vpr_ctx.device();

    t_trace* head;
    t_trace* tail;
    std::unordered_set<int> nodes;

    std::tie(head, tail) = traceback_from_route_tree_recurr(nullptr, nullptr, root);

	VTR_ASSERT(head);
	VTR_ASSERT(tail);
	VTR_ASSERT(tail->next == nullptr);

    int num_trace_sinks = 0;
    for (t_trace* trace = head; trace != nullptr; trace = trace->next) {
        nodes.insert(trace->index);

        //Sanity check that number of sinks match expected
        if (device_ctx.rr_nodes[trace->index].type() == SINK) {
            num_trace_sinks += 1;
        }
    }
	VTR_ASSERT(num_routed_sinks == num_trace_sinks);


	route_ctx.trace_tail[inet] = tail;
	route_ctx.trace_head[inet] = head;
	route_ctx.trace_nodes[inet] = nodes;

	return head;
}


//Prunes a route tree (recursively) based on congestion and the 'force_prune' argument
//
//Returns true if the current node was pruned
t_rt_node* prune_route_tree_recurr(t_rt_node* node, CBRR& connections_inf, bool force_prune) {

    //Recursively traverse the route tree rooted at node and remove any congested
    //sub-trees

    VTR_ASSERT(node);

    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.routing();


    bool congested = (route_ctx.rr_node_route_inf[node->inode].occ() > device_ctx.rr_nodes[node->inode].capacity());

    if (congested) {
        //This connection is congested -- prune it
        force_prune = true;
    }

    if (connections_inf.should_force_reroute_connection(node->inode)) {
        //Forcibly re-route (e.g. to improve delay)
        force_prune = true;
    }

    //Recursively prune children
    bool all_children_pruned = true;
    t_linked_rt_edge* prev_edge = nullptr;
    t_linked_rt_edge* edge = node->u.child_list;
    while (edge) {
        t_rt_node* child = prune_route_tree_recurr(edge->child, connections_inf, force_prune);

        if (!child) { //Child was pruned

            //Remove the edge
            if (edge == node->u.child_list) { //Was Head
                node->u.child_list = edge->next;
            } else { //Was intermediate
                VTR_ASSERT(prev_edge);
                prev_edge->next = edge->next;
            }

            t_linked_rt_edge* old_edge = edge;
            edge = edge->next;

            free_linked_rt_edge(old_edge);

            //Note prev_edge is unchanged

        } else { //Child not pruned
            all_children_pruned = false;

            //Edge not removed
            prev_edge = edge;
            edge = edge->next;
        }
    }

    if (device_ctx.rr_nodes[node->inode].type() == SINK) {

        if (!force_prune) {
            //Valid path to sink

            //Record sink as reachable
            connections_inf.reached_rt_sink(node);

            return node; //Not pruned
        } else {
            VTR_ASSERT(force_prune);

            //Record as not reached
            connections_inf.toreach_rr_sink(node->inode);

            free_rt_node(node);
            return nullptr; //Pruned
        }
    } else if (all_children_pruned) {
        //This node has no children
        //
        // This can happen in three scenarios:
        //   1) This node is being pruned. As a result any child nodes
        //      (subtrees) will have been pruned.
        //
        //   2) This node was reached by a non-configurable edge but
        //      was otherwise unused (forming a 'stub' off the main
        //      branch).
        //
        //   3) This node is uncongested, but all its connected sub-trees
        //      have been pruned.
        //
        // We take the corresponding actions:
        //   1) Prune the node.
        //
        //   2) Prune the node only if force_prune is set
        //
        //      Since the node was reached non-configurably it is illegal to
        //      remove it independently of any other nodes which are non-
        //      configurably connected to it.
        //
        //      As a result we only remove a non-configurably connected node
        //      if force_prune is set.
        //
        //      Note that if a node upstream of the non-configurably connected
        //      nodes becomes congested, force_prune will be set and the *entire*
        //      set of non-configurably connected nodes will be removed.
        //
        //   3) Prune the node.
        //
        //      This avoid the creation of unused 'stubs'. (For example if
        //      we left the stubs in and they were subsequently not used
        //      they would uselessly consume routing resources).
        VTR_ASSERT(node->u.child_list == nullptr);

        bool reached_non_configurably = false;
        if (node->parent_node) {
            reached_non_configurably = !device_ctx.rr_switch_inf[node->parent_switch].configurable();
        }

        if (reached_non_configurably && !force_prune) {
            return node; //Not pruned
        } else {
            free_rt_node(node);
            return nullptr; //Pruned
        }

    } else {
        //An unpruned intermediate node
        VTR_ASSERT(!force_prune);

        return node; //Not pruned
    }
}


t_rt_node* prune_route_tree(t_rt_node* rt_root, CBRR& connections_inf) {
	/* Prune a skeleton route tree of illegal branches - when there is at least 1 congested node on the path to a sink
	 * This is the top level function to be called with the SOURCE node as root.
	 * Returns true if the entire tree has been pruned.
     *
     * Note: does not update R_upstream/C_downstream
     */

    VTR_ASSERT(rt_root);

    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.routing();

	VTR_ASSERT_MSG(device_ctx.rr_nodes[rt_root->inode].type() == SOURCE, "Root of route tree must be SOURCE");

	VTR_ASSERT_MSG(route_ctx.rr_node_route_inf[rt_root->inode].occ() <= device_ctx.rr_nodes[rt_root->inode].capacity(),
            "Route tree root/SOURCE should never be congested");

    return prune_route_tree_recurr(rt_root, connections_inf, false);
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
	if (new_branch) VTR_LOG("\n%*s", indent_level*branch_level, " \\ ");

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
	VTR_LOG("edges to ");
	if (!edge) {VTR_LOG("null"); return;}
	while (edge) {
		VTR_LOG("%d(%d) ", edge->child->inode, edge->iswitch);
		edge = edge->next;
	}
	VTR_LOG("\n");
}


static void print_node(const t_rt_node* rt_node) {
    auto& device_ctx = g_vpr_ctx.device();

	int inode = rt_node->inode;
	t_rr_type node_type = device_ctx.rr_nodes[inode].type();
	VTR_LOG("%5.1e %5.1e %2d%6s|%-6d-> ", rt_node->C_downstream, rt_node->R_upstream,
		rt_node->re_expand, rr_node_typename[node_type], inode);
}


static void print_node_inf(const t_rt_node* rt_node) {
    auto& route_ctx = g_vpr_ctx.routing();

	int inode = rt_node->inode;
	const auto& node_inf = route_ctx.rr_node_route_inf[inode];
	VTR_LOG("%5.1e %5.1e%6d%3d|%-6d-> ", node_inf.path_cost, node_inf.backward_path_cost,
		node_inf.prev_node, node_inf.prev_edge, inode);
}


static void print_node_congestion(const t_rt_node* rt_node) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.routing();

	int inode = rt_node->inode;
	const auto& node_inf = route_ctx.rr_node_route_inf[inode];
	const auto& node = device_ctx.rr_nodes[inode];
	const auto& node_state = route_ctx.rr_node_route_inf[inode];
	VTR_LOG("%2d %2d|%-6d-> ", node_inf.pres_cost, rt_node->Tdel,
		node_state.occ(), node.capacity(), inode);
}


void print_route_tree_inf(const t_rt_node* rt_root) {
	traverse_indented_route_tree(rt_root, 0, false, print_node_inf, 34);
	VTR_LOG("\n");
}

void print_route_tree(const t_rt_node* rt_root) {
	traverse_indented_route_tree(rt_root, 0, false, print_node, 34);
	VTR_LOG("\n");
}

void print_route_tree_congestion(const t_rt_node* rt_root) {
	traverse_indented_route_tree(rt_root, 0, false, print_node_congestion, 15);
	VTR_LOG("\n");
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
		VTR_LOG("mismatch i %d|%d R %e|%e C %e|%e T %e %e\n",
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
			VTR_LOG("mismatch i %d|%d edge switch %d|%d\n",
				root->inode, root_clone->inode,
				orig_edge->iswitch, clone_edge->iswitch);
		if (!is_equivalent_route_tree(orig_edge->child, clone_edge->child)) return false;	// child trees not equivalent
		orig_edge = orig_edge->next;
		clone_edge = clone_edge->next;
	}
	if (orig_edge || clone_edge) {
		VTR_LOG("one of the trees have an extra edge!\n");
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
			VTR_LOG("parent-child relationship not mutually acknowledged by parent %d->%d child %d<-%d\n",
				inode, edge->child->inode,
				edge->child->inode, edge->child->parent_node->inode);
			return false;
		}
		if (edge->iswitch != edge->child->parent_switch) {
			VTR_LOG("parent(%d)-child(%d) connected switch not equivalent parent %d child %d\n",
				inode, edge->child->inode, edge->iswitch, edge->child->parent_switch);
			return false;
		}

		if (!is_valid_skeleton_tree(edge->child)) {
			VTR_LOG("subtree %d invalid, propagating up\n", edge->child->inode);
			return false;
		}
		edge = edge->next;
	}
	return true;
}

bool is_valid_route_tree(const t_rt_node* root) {
	// check upstream resistance
    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.routing();

	int inode = root->inode;
	short iswitch = root->parent_switch;
	if (root->parent_node) {
		if (device_ctx.rr_switch_inf[iswitch].buffered()) {
			if (root->R_upstream != device_ctx.rr_nodes[inode].R() + device_ctx.rr_switch_inf[iswitch].R) {
				VTR_LOG("%d mismatch R upstream %e supposed %e\n", inode, root->R_upstream,
					device_ctx.rr_nodes[inode].R() + device_ctx.rr_switch_inf[iswitch].R);
				return false;
			}
		}
		else if (root->R_upstream != device_ctx.rr_nodes[inode].R() + root->parent_node->R_upstream + device_ctx.rr_switch_inf[iswitch].R) {
			VTR_LOG("%d mismatch R upstream %e supposed %e\n", inode, root->R_upstream,
				device_ctx.rr_nodes[inode].R() + root->parent_node->R_upstream + device_ctx.rr_switch_inf[iswitch].R);
			return false;
		}
	}
	else if (root->R_upstream != device_ctx.rr_nodes[inode].R()) {
		VTR_LOG("%d mismatch R upstream %e supposed %e\n", inode, root->R_upstream, device_ctx.rr_nodes[inode].R());
		return false;
	}

	// check downstream C
	t_linked_rt_edge* edge = root->u.child_list;
	float C_downstream_children {0};
	// sink, must not be congested
	if (!edge) {
		int occ = route_ctx.rr_node_route_inf[inode].occ();
		int capacity = device_ctx.rr_nodes[inode].capacity();
		if (occ > capacity) {
			VTR_LOG("SINK %d occ %d > cap %d\n", inode, occ, capacity);
			return false;
		}
	}
	while (edge) {
		if (edge->child->parent_node != root) {
			VTR_LOG("parent-child relationship not mutually acknowledged by parent %d->%d child %d<-%d\n",
				inode, edge->child->inode,
				edge->child->inode, edge->child->parent_node->inode);
			return false;
		}
		if (edge->iswitch != edge->child->parent_switch) {
			VTR_LOG("parent(%d)-child(%d) connected switch not equivalent parent %d child %d\n",
				inode, edge->child->inode, edge->iswitch, edge->child->parent_switch);
			return false;
		}

		if (!device_ctx.rr_switch_inf[edge->iswitch].buffered())
			C_downstream_children += edge->child->C_downstream;

		if (!is_valid_route_tree(edge->child)) {
			VTR_LOG("subtree %d invalid, propagating up\n", edge->child->inode);
			return false;
		}
		edge = edge->next;
	}

	if (root->C_downstream != C_downstream_children + device_ctx.rr_nodes[inode].C()) {
		VTR_LOG("mismatch C downstream %e supposed %e\n", root->C_downstream, C_downstream_children + device_ctx.rr_nodes[inode].C());
		return false;
	}

	return true;
}

//Returns true if the route tree rooted at 'root' is not congested
bool is_uncongested_route_tree(const t_rt_node* root) {
    auto& route_ctx = g_vpr_ctx.routing();
    auto& device_ctx = g_vpr_ctx.device();

    int inode = root->inode;
    if (route_ctx.rr_node_route_inf[inode].occ() > device_ctx.rr_nodes[inode].capacity()) {
        //This node is congested
        return false;
    }

    for (t_linked_rt_edge* edge = root->u.child_list; edge != nullptr; edge = edge->next) {
        if (!is_uncongested_route_tree(edge->child)) {
            //The sub-tree connected to this edge is congested
            return false;
        }
    }

    //The sub-tree below the curret node is unconngested
    return true;
}


t_rt_node*
init_route_tree_to_source_no_net(int inode) {

	/* Initializes the routing tree to just the net source, and returns the root
	 * node of the rt_tree (which is just the net source).                       */

    t_rt_node *rt_root;

    auto& device_ctx = g_vpr_ctx.device();

    rt_root = alloc_rt_node();
    rt_root->u.child_list = nullptr;
    rt_root->parent_node = nullptr;
    rt_root->parent_switch = OPEN;
    rt_root->re_expand = true;
    rt_root->inode = inode;
    rt_root->C_downstream = device_ctx.rr_nodes[inode].C();
    rt_root->R_upstream = device_ctx.rr_nodes[inode].R();
    rt_root->Tdel = 0.5 * device_ctx.rr_nodes[inode].R() * device_ctx.rr_nodes[inode].C();
    rr_node_to_rt_node[inode] = rt_root;

    return (rt_root);
}


bool verify_traceback_route_tree_equivalent(const t_trace* head, const t_rt_node* rt_root) {

    //Walk the route tree saving all the used connections
    std::set<std::tuple<int,int,int>> route_tree_connections;
    collect_route_tree_connections(rt_root, route_tree_connections);

    //Remove the extra parent connection to root (not included in traceback)
    route_tree_connections.erase(std::make_tuple(OPEN, OPEN, rt_root->inode));

    //Walk the traceback and verify that every connection exists in the route tree set
    int prev_node = OPEN;
    int prev_switch = OPEN;
    int to_node = OPEN;
    for (const t_trace* trace = head; trace != nullptr; trace = trace->next) {
        to_node = trace->index;

        auto conn = std::make_tuple(prev_node, prev_switch, to_node);
        if (prev_switch != OPEN) {
            //Not end of branch
            if (!route_tree_connections.count(conn)) {
                VPR_THROW(VPR_ERROR_ROUTE, "Route tree missing traceback connection: node %d -> %d (switch %d)\n",
                    prev_node, to_node, prev_switch);
            } else {
                route_tree_connections.erase(conn); //Remove found connections
            }
        }

        prev_node = trace->index;
        prev_switch = trace->iswitch;
    }

    if (!route_tree_connections.empty()) {
        std::string msg = "Found route tree connection(s) not in traceback:\n";
        for (auto conn : route_tree_connections) {
            std::tie(prev_node, prev_switch, to_node) = conn;
            msg += vtr::string_fmt("\tnode %d -> %d (switch %d)\n", prev_node, to_node, prev_switch);
        }

        VPR_THROW(VPR_ERROR_ROUTE, msg.c_str());
    }


    return true;
}

void collect_route_tree_connections(const t_rt_node* node, std::set<std::tuple<int,int,int>>& connections) {
    if (node) {

        //Record reaching connection
        int prev_node = OPEN;
        int prev_switch = OPEN;
        int to_node = node->inode;
        if (node->parent_node) {
            prev_node = node->parent_node->inode;
            prev_switch = node->parent_switch;
        }
        auto conn = std::make_tuple(prev_node, prev_switch, to_node);
        connections.insert(conn);

        //Recurse
        for (auto edge = node->u.child_list; edge != nullptr; edge = edge->next) {
            collect_route_tree_connections(edge->child, connections);
        }
    }
}
