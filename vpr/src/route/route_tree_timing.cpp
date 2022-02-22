#include <cstdio>
#include <cmath>
#include <vector>

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_memory.h"
#include "vtr_math.h"

#include "vpr_types.h"
#include "vpr_utils.h"
#include "vpr_error.h"

#include "globals.h"
#include "route_common.h"
#include "route_tree_timing.h"

/* This module keeps track of the partial routing tree for timing-driven     *
 * routing.  The normal traceback structure doesn't provide enough info      *
 * about the partial routing during timing-driven routing, so the routines   *
 * in this module are used to keep a tree representation of the partial      *
 * routing during timing-driven routing.  This allows rapid incremental      *
 * timing analysis.                                                          */

/********************** Variables local to this module ***********************/

/* Array below allows mapping from any rr_node to any rt_node currently in
 * the rt_tree.                                                              */

/* In some cases the same SINK node is put into the tree multiple times in a     *
 * single route. To model this, we are putting in separate rt_nodes in the route *
 * tree if we go to the same SINK more than once. rr_node_to_rt_node[inode] will *
 * therefore store the last rt_node created of all the SINK nodes with the same  *
 * index "inode". This is okay because the mapping is only used in this file to  *
 * quickly figure out where rt_nodes that we are branching off of (for nets with *
 * fanout > 1) are, and we will never branch off a SINK.                         */
static std::vector<t_rt_node*> rr_node_to_rt_node; /* [0..device_ctx.rr_nodes.size()-1] */

/* Frees lists for fast addition and deletion of nodes and edges. */

static t_rt_node* rt_node_free_list = nullptr;
static t_linked_rt_edge* rt_edge_free_list = nullptr;

/********************** Subroutines local to this module *********************/

static t_rt_node* alloc_rt_node();

static void free_rt_node(t_rt_node* rt_node);

static t_linked_rt_edge* alloc_linked_rt_edge();

static void free_linked_rt_edge(t_linked_rt_edge* rt_edge);

static t_rt_node* add_subtree_to_route_tree(t_heap* hptr,
                                            int target_net_pin_index,
                                            t_rt_node** sink_rt_node_ptr);

static t_rt_node* add_non_configurable_to_route_tree(const int rr_node, const bool reached_by_non_configurable_edge, std::unordered_set<int>& visited);

static t_rt_node* update_unbuffered_ancestors_C_downstream(t_rt_node* start_of_new_subtree_rt_node);

bool verify_route_tree_recurr(t_rt_node* node, std::set<int>& seen_nodes);

static t_rt_node* prune_route_tree_recurr(t_rt_node* node, CBRR& connections_inf, bool congested, std::vector<int>* non_config_node_set_usage);

static t_trace* traceback_to_route_tree_branch(t_trace* trace, std::map<int, t_rt_node*>& rr_node_to_rt, std::vector<int>* non_config_node_set_usage);

static std::pair<t_trace*, t_trace*> traceback_from_route_tree_recurr(t_trace* head, t_trace* tail, const t_rt_node* node);

void collect_route_tree_connections(const t_rt_node* node, std::multiset<std::tuple<int, int, int>>& connections);

/************************** Subroutine definitions ***************************/

constexpr float epsilon = 1e-15;
static bool equal_approx(float a, float b) {
    return fabs(a - b) < epsilon;
}

bool alloc_route_tree_timing_structs(bool exists_ok) {
    /* Allocates any structures needed to build the routing trees. */

    auto& device_ctx = g_vpr_ctx.device();
    bool route_tree_structs_are_allocated = (rr_node_to_rt_node.size() == size_t(device_ctx.rr_graph.num_nodes())
                                             || rt_node_free_list != nullptr);
    if (route_tree_structs_are_allocated) {
        if (exists_ok) {
            return false;
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "in alloc_route_tree_timing_structs: old structures already exist.\n");
        }
    }

    rr_node_to_rt_node = std::vector<t_rt_node*>(device_ctx.rr_graph.num_nodes(), nullptr);

    return true;
}

void free_route_tree_timing_structs() {
    /* Frees the structures needed to build routing trees, and really frees
     * (i.e. deletes) all the data on the free lists.                         */

    t_rt_node *rt_node, *next_node;
    t_linked_rt_edge *rt_edge, *next_edge;

    rr_node_to_rt_node.clear();

    rt_node = rt_node_free_list;

    while (rt_node != nullptr) {
        next_node = rt_node->u.next;
        delete rt_node;
        rt_node = next_node;
    }

    rt_node_free_list = nullptr;

    rt_edge = rt_edge_free_list;

    while (rt_edge != nullptr) {
        next_edge = rt_edge->next;
        delete rt_edge;
        rt_edge = next_edge;
    }

    rt_edge_free_list = nullptr;
}

static t_rt_node*
alloc_rt_node() {
    /* Allocates a new rt_node, from the free list if possible, from the free
     * store otherwise.                                                         */

    t_rt_node* rt_node;

    rt_node = rt_node_free_list;

    if (rt_node != nullptr) {
        rt_node_free_list = rt_node->u.next;
    } else {
        rt_node = new t_rt_node;
    }

    return (rt_node);
}

static void free_rt_node(t_rt_node* rt_node) {
    /* Adds rt_node to the proper free list.          */

    rt_node->u.next = rt_node_free_list;
    rt_node_free_list = rt_node;
}

static t_linked_rt_edge*
alloc_linked_rt_edge() {
    /* Allocates a new linked_rt_edge, from the free list if possible, from the
     * free store otherwise.                                                     */

    t_linked_rt_edge* linked_rt_edge;

    linked_rt_edge = rt_edge_free_list;

    if (linked_rt_edge != nullptr) {
        rt_edge_free_list = linked_rt_edge->next;
    } else {
        linked_rt_edge = new t_linked_rt_edge;
    }

    VTR_ASSERT(linked_rt_edge != nullptr);
    return (linked_rt_edge);
}

/* Adds the rt_edge to the rt_edge free list.                       */
static void free_linked_rt_edge(t_linked_rt_edge* rt_edge) {
    rt_edge->next = rt_edge_free_list;
    rt_edge_free_list = rt_edge;
}

/* Initializes the routing tree to just the net source, and returns the root
 * node of the rt_tree (which is just the net source).                       */
t_rt_node* init_route_tree_to_source(ClusterNetId inet) {
    t_rt_node* rt_root;
    int inode;

    auto& route_ctx = g_vpr_ctx.routing();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    rt_root = alloc_rt_node();
    rt_root->u.child_list = nullptr;
    rt_root->parent_node = nullptr;
    rt_root->parent_switch = OPEN;
    rt_root->re_expand = true;

    inode = route_ctx.net_rr_terminals[inet][0]; /* Net source */

    rt_root->inode = inode;
    rt_root->net_pin_index = OPEN;
    rt_root->C_downstream = rr_graph.node_C(RRNodeId(inode));
    rt_root->R_upstream = rr_graph.node_R(RRNodeId(inode));
    rt_root->Tdel = 0.5 * rr_graph.node_R(RRNodeId(inode)) * rr_graph.node_C(RRNodeId(inode));
    rr_node_to_rt_node[inode] = rt_root;

    return (rt_root);
}

/* Adds the most recently finished wire segment to the routing tree, and
 * updates the Tdel, etc. numbers for the rest of the routing tree.  hptr
 * is the heap pointer of the SINK that was reached, and target_net_pin_index
 * is the net pin index corresponding to the SINK that was reached. This routine
 * returns a pointer to the rt_node of the SINK that it adds to the routing.        */
t_rt_node* update_route_tree(t_heap* hptr, int target_net_pin_index, SpatialRouteTreeLookup* spatial_rt_lookup) {
    t_rt_node *start_of_new_subtree_rt_node, *sink_rt_node;
    t_rt_node *unbuffered_subtree_rt_root, *subtree_parent_rt_node;
    float Tdel_start;
    short iswitch;

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    //Create a new subtree from the target in hptr to existing routing
    start_of_new_subtree_rt_node = add_subtree_to_route_tree(hptr, target_net_pin_index, &sink_rt_node);

    //Propagate R_upstream down into the new subtree
    load_new_subtree_R_upstream(start_of_new_subtree_rt_node);

    //Propagate C_downstream up from new subtree sinks to subtree root
    load_new_subtree_C_downstream(start_of_new_subtree_rt_node);

    //Propagate C_downstream up from the subtree root
    unbuffered_subtree_rt_root = update_unbuffered_ancestors_C_downstream(start_of_new_subtree_rt_node);

    subtree_parent_rt_node = unbuffered_subtree_rt_root->parent_node;

    if (subtree_parent_rt_node != nullptr) { /* Parent exists. */
        Tdel_start = subtree_parent_rt_node->Tdel;
        iswitch = unbuffered_subtree_rt_root->parent_switch;
        /*TODO Just a note (no action needed for this PR):In future, we need to consider APIs that returns
         * the Tdel for a routing trace in RRGraphView.*/
        Tdel_start += rr_graph.rr_switch_inf(RRSwitchId(iswitch)).R * unbuffered_subtree_rt_root->C_downstream;
        Tdel_start += rr_graph.rr_switch_inf(RRSwitchId(iswitch)).Tdel;
    } else { /* Subtree starts at SOURCE */
        Tdel_start = 0.;
    }

    load_route_tree_Tdel(unbuffered_subtree_rt_root, Tdel_start);

    if (spatial_rt_lookup) {
        update_route_tree_spatial_lookup_recur(start_of_new_subtree_rt_node, *spatial_rt_lookup);
    }

    return (sink_rt_node);
}

/* Records all nodes from the current routing (rt_tree) into the rr_node_to_rt_node
 * lookup, which maps the node's corresponding rr_node index (inode) to the node
 * itself. This is done recursively, starting from the root of the tree to its leafs
 * (SINKs) in a depth-first traversal. The rt_node we are currently processing has
 * either not been added to the routing for this net before or if it was added, the
 * rr_node_to_rt_node mapping structure should point back at the rt_node itself so
 * we are just branching off that point. Exceptions are the SINK nodes, some
 * netlists and input pin equivalence can lead to us routing to the same SINK more
 * than once on a net (resulting in different rt_nodes sharing the same rr_node index).
 * Hence for SINKs we assert on a weaker condition that if this SINK is already in the
 * rt_tree, the rr_node_to_rt_node mapping structure points to a legal rt_node (but
 * not necessarily the only one) containing the SINK */
void add_route_tree_to_rr_node_lookup(t_rt_node* node) {
    if (node) {
        auto& device_ctx = g_vpr_ctx.device();
        const auto& rr_graph = device_ctx.rr_graph;
        if (rr_graph.node_type(RRNodeId(node->inode)) == SINK) {
            VTR_ASSERT(rr_node_to_rt_node[node->inode] == nullptr || rr_node_to_rt_node[node->inode]->inode == node->inode);
        } else {
            VTR_ASSERT(rr_node_to_rt_node[node->inode] == nullptr || rr_node_to_rt_node[node->inode] == node);
        }

        rr_node_to_rt_node[node->inode] = node;

        for (auto edge = node->u.child_list; edge != nullptr; edge = edge->next) {
            add_route_tree_to_rr_node_lookup(edge->child);
        }
    }
}

/* Adds the most recent wire segment, ending at the SINK indicated by hptr,
 * to the routing tree. target_net_pin_index is the net pin index correspinding
 * to the SINK indicated by hptr. Returns the first (most upstream) new rt_node,
 * and (via a pointer) the rt_node of the new SINK. Traverses up from SINK  */
static t_rt_node*
add_subtree_to_route_tree(t_heap* hptr, int target_net_pin_index, t_rt_node** sink_rt_node_ptr) {
    t_rt_node *rt_node, *downstream_rt_node, *sink_rt_node;
    t_linked_rt_edge* linked_rt_edge;

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    int inode = hptr->index;

    //if (rr_graph.node_type(RRNodeId(inode)) != SINK) {
    //VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
    //"in add_subtree_to_route_tree. Expected type = SINK (%d).\n"
    //"Got type = %d.",  SINK, rr_graph.node_type(RRNodeId(inode)));
    //}

    sink_rt_node = alloc_rt_node();
    sink_rt_node->u.child_list = nullptr;
    sink_rt_node->inode = inode;
    sink_rt_node->net_pin_index = target_net_pin_index; //hptr is the heap pointer of the SINK that was reached, which corresponds to the target pin
    rr_node_to_rt_node[inode] = sink_rt_node;

    /* In the code below I'm marking SINKs and IPINs as not to be re-expanded.
     * It makes the code more efficient (though not vastly) to prune this way
     * when there aren't route-throughs or ipin doglegs.                        */

    sink_rt_node->re_expand = false;

    /* Now do it's predecessor. */

    downstream_rt_node = sink_rt_node;

    std::unordered_set<int> main_branch_visited; //does not include sink
    std::unordered_set<int> all_visited;         //does not include sink
    inode = hptr->prev_node();
    RREdgeId edge = hptr->prev_edge();
    short iswitch = rr_graph.rr_nodes().edge_switch(edge);

    /* For all "new" nodes in the main path */
    // inode is node index of previous node
    // NO_PREVIOUS tags a previously routed node

    while (rr_node_to_rt_node[inode] == nullptr) { //Not connected to existing routing
        main_branch_visited.insert(inode);
        all_visited.insert(inode);

        linked_rt_edge = alloc_linked_rt_edge();
        linked_rt_edge->child = downstream_rt_node;

        // Also mark downstream_rt_node->inode as visited to prevent
        // add_non_configurable_to_route_tree from potentially adding
        // downstream_rt_node to the rt tree twice.
        all_visited.insert(downstream_rt_node->inode);
        linked_rt_edge->iswitch = iswitch;
        linked_rt_edge->next = nullptr;

        rt_node = alloc_rt_node();
        downstream_rt_node->parent_node = rt_node;
        downstream_rt_node->parent_switch = iswitch;

        rt_node->u.child_list = linked_rt_edge;
        rt_node->inode = inode;
        rt_node->net_pin_index = OPEN; //net pin index is invalid for non-SINK nodes

        rr_node_to_rt_node[inode] = rt_node;

        if (rr_graph.node_type(RRNodeId(inode)) == IPIN) {
            rt_node->re_expand = false;
        } else {
            rt_node->re_expand = true;
        }

        downstream_rt_node = rt_node;
        edge = route_ctx.rr_node_route_inf[inode].prev_edge;
        inode = route_ctx.rr_node_route_inf[inode].prev_node;
        iswitch = rr_graph.rr_nodes().edge_switch(edge);
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
    //Sink is not included, so no need to pass in the node's ipin value.
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
        const auto& rr_graph = device_ctx.rr_graph;

        rt_node = rr_node_to_rt_node[rr_node];

        if (!reached_by_non_configurable_edge) { //An existing main branch node
            VTR_ASSERT(rt_node);
        } else {
            VTR_ASSERT(reached_by_non_configurable_edge);

            if (!rt_node) {
                rt_node = alloc_rt_node();
                rt_node->u.child_list = nullptr;
                rt_node->inode = rr_node;
                rt_node->net_pin_index = OPEN;

                if (rr_graph.node_type(RRNodeId(rr_node)) == IPIN) {
                    rt_node->re_expand = false;
                } else {
                    rt_node->re_expand = true;
                }
            } else {
                VTR_ASSERT(rt_node->inode == rr_node);
            }
        }
        for (int iedge : rr_graph.non_configurable_edges(RRNodeId(rr_node))) {
            //Recursive case: expand children
            VTR_ASSERT(!rr_graph.edge_is_configurable(RRNodeId(rr_node), iedge));
            int to_rr_node = size_t(rr_graph.edge_sink_node(RRNodeId(rr_node), iedge));

            //Recurse
            t_rt_node* child_rt_node = add_non_configurable_to_route_tree(to_rr_node, true, visited);

            if (!child_rt_node) continue;
            int iswitch = rr_graph.edge_switch(RRNodeId(rr_node), iedge);

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
    const auto& rr_graph = device_ctx.rr_graph;

    t_rt_node* parent_rt_node = rt_node->parent_node;
    int inode = rt_node->inode;

    //Calculate upstream resistance
    float R_upstream = 0.;
    if (parent_rt_node) {
        int iswitch = rt_node->parent_switch;
        bool switch_buffered = rr_graph.rr_switch_inf(RRSwitchId(iswitch)).buffered();

        if (!switch_buffered) {
            R_upstream += parent_rt_node->R_upstream; //Parent upstream R
        }
        R_upstream += rr_graph.rr_switch_inf(RRSwitchId(iswitch)).R; //Parent switch R
    }
    R_upstream += rr_graph.node_R(RRNodeId(inode)); //Current node R

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
        const auto& rr_graph = device_ctx.rr_graph;
        C_downstream += rr_graph.node_C(RRNodeId(rt_node->inode));
        for (t_linked_rt_edge* edge = rt_node->u.child_list; edge != nullptr; edge = edge->next) {
            /*Similar to net_delay.cpp, this for loop traverses a rc subtree, whose edges represent enabled switches.
             * When switches such as multiplexers and tristate buffers are enabled, their fanout
             * produces an "internal capacitance". We account for this internal capacitance of the
             * switch by adding it to the total capacitance of the node.*/

            C_downstream += rr_graph.rr_switch_inf(RRSwitchId(edge->iswitch)).Cinternal;

            float C_downstream_child = load_new_subtree_C_downstream(edge->child);

            if (!rr_graph.rr_switch_inf(RRSwitchId(edge->iswitch)).buffered()) {
                C_downstream += C_downstream_child;
            }
        }

        rt_node->C_downstream = C_downstream;
    }

    return C_downstream;
}

static t_rt_node*
update_unbuffered_ancestors_C_downstream(t_rt_node* start_of_new_path_rt_node) {
    /* Updates the C_downstream values for the ancestors of the new path.  Once
     * a buffered switch is found amongst the ancestors, no more ancestors are
     * affected.  Returns the root of the "unbuffered subtree" whose Tdel
     * values are affected by the new path's addition.                          */

    t_rt_node *rt_node, *parent_rt_node;
    short iswitch;
    float C_downstream_addition;

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    rt_node = start_of_new_path_rt_node;
    parent_rt_node = rt_node->parent_node;
    iswitch = rt_node->parent_switch;

    /* Now that a connection has been made between rt_node and its parent we must also consider
     * the potential effect of internal capacitance. We will first assume that parent is connected
     * by an unbuffered switch, so the ancestors downstream capacitance must be equal to the sum
     * of the child's downstream capacitance with the internal capacitance of the switch.*/

    C_downstream_addition = rt_node->C_downstream + rr_graph.rr_switch_inf(RRSwitchId(iswitch)).Cinternal;

    /* Having set the value of C_downstream_addition, we must check whethere the parent switch
     * is a buffered or unbuffered switch with the if statement below. If the parent switch is
     * a buffered switch, then the parent node's downsteam capacitance is increased by the
     * value of the parent switch's internal capacitance in the if statement below.
     * Correspondingly, the ancestors' downstream capacitance will be updated by the same
     * value through the while loop. Otherwise, if the parent switch is unbuffered, then
     * the if statement will be ignored, and the parent and ancestral nodes' downstream
     * capacitance will be increased by the sum of the child's downstream capacitance with
     * the internal capacitance of the parent switch in the while loop/.*/

    if (parent_rt_node != nullptr && rr_graph.rr_switch_inf(RRSwitchId(iswitch)).buffered() == true) {
        C_downstream_addition = rr_graph.rr_switch_inf(RRSwitchId(iswitch)).Cinternal;
        rt_node = parent_rt_node;
        rt_node->C_downstream += C_downstream_addition;
        parent_rt_node = rt_node->parent_node;
        iswitch = rt_node->parent_switch;
    }

    while (parent_rt_node != nullptr && rr_graph.rr_switch_inf(RRSwitchId(iswitch)).buffered() == false) {
        rt_node = parent_rt_node;
        rt_node->C_downstream += C_downstream_addition;
        parent_rt_node = rt_node->parent_node;
        iswitch = rt_node->parent_switch;
    }
    return (rt_node);
}

void load_route_tree_Tdel(t_rt_node* subtree_rt_root, float Tarrival) {
    /* Updates the Tdel values of the subtree rooted at subtree_rt_root by
     * by calling itself recursively.  The C_downstream values of all the nodes
     * must be correct before this routine is called.  Tarrival is the time at
     * at which the signal arrives at this node's *input*.                      */

    int inode;
    short iswitch;
    t_rt_node* child_node;
    t_linked_rt_edge* linked_rt_edge;
    float Tdel, Tchild;

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    inode = subtree_rt_root->inode;

    /* Assuming the downstream connections are, on average, connected halfway
     * along a wire segment's length.  See discussion in net_delay.c if you want
     * to change this.                                                           */
    Tdel = Tarrival + 0.5 * subtree_rt_root->C_downstream * rr_graph.node_R(RRNodeId(inode));
    subtree_rt_root->Tdel = Tdel;

    /* Now expand the children of this node to load their Tdel values (depth-
     * first pre-order traversal).                                              */

    linked_rt_edge = subtree_rt_root->u.child_list;

    while (linked_rt_edge != nullptr) {
        iswitch = linked_rt_edge->iswitch;
        child_node = linked_rt_edge->child;

        Tchild = Tdel + rr_graph.rr_switch_inf(RRSwitchId(iswitch)).R * child_node->C_downstream;
        Tchild += rr_graph.rr_switch_inf(RRSwitchId(iswitch)).Tdel; /* Intrinsic switch delay. */
        load_route_tree_Tdel(child_node, Tchild);

        linked_rt_edge = linked_rt_edge->next;
    }
}

void load_route_tree_rr_route_inf(t_rt_node* root) {
    /* Traverses down a route tree and updates rr_node_inf for all nodes
     * to reflect that these nodes have already been routed to 			 */

    VTR_ASSERT(root != nullptr);

    auto& route_ctx = g_vpr_ctx.mutable_routing();

    t_linked_rt_edge* edge{root->u.child_list};

    for (;;) {
        int inode = root->inode;
        route_ctx.rr_node_route_inf[inode].prev_node = NO_PREVIOUS;
        route_ctx.rr_node_route_inf[inode].prev_edge = RREdgeId::INVALID();
        // path cost should be unset
        VTR_ASSERT(std::isinf(route_ctx.rr_node_route_inf[inode].path_cost));
        VTR_ASSERT(std::isinf(route_ctx.rr_node_route_inf[inode].backward_path_cost));

        // reached a sink
        if (!edge) { return; }
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
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Duplicate route tree nodes found for node %d", node->inode);
    }

    seen_nodes.insert(node->inode);

    for (t_linked_rt_edge* edge = node->u.child_list; edge != nullptr; edge = edge->next) {
        verify_route_tree_recurr(edge->child, seen_nodes);
    }
    return true;
}

void free_route_tree(t_rt_node* rt_node) {
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

    if (!rr_node_to_rt_node.empty()) {
        rr_node_to_rt_node.at(rt_node->inode) = nullptr;
    }

    free_rt_node(rt_node);
}

void print_route_tree(const t_rt_node* rt_node) {
    print_route_tree(rt_node, 0);
}

void print_route_tree(const t_rt_node* rt_node, int depth) {
    std::string indent;
    for (int i = 0; i < depth; ++i) {
        indent += "  ";
    }

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    VTR_LOG("%srt_node: %d (%s) \t ipin: %d \t R: %g \t C: %g \t delay: %g",
            indent.c_str(), rt_node->inode, rr_graph.node_type_string(RRNodeId(rt_node->inode)), rt_node->net_pin_index, rt_node->R_upstream, rt_node->C_downstream, rt_node->Tdel);

    if (rt_node->parent_switch != OPEN) {
        bool parent_edge_configurable = rr_graph.rr_switch_inf(RRSwitchId(rt_node->parent_switch)).configurable();
        if (!parent_edge_configurable) {
            VTR_LOG("*");
        }
    }

    auto& route_ctx = g_vpr_ctx.routing();
    if (route_ctx.rr_node_route_inf[rt_node->inode].occ() > rr_graph.node_capacity(RRNodeId(rt_node->inode))) {
        VTR_LOG(" x");
    }

    VTR_LOG("\n");

    for (t_linked_rt_edge* rt_edge = rt_node->u.child_list; rt_edge != nullptr; rt_edge = rt_edge->next) {
        print_route_tree(rt_edge->child, depth + 1);
    }
}

void update_net_delays_from_route_tree(float* net_delay,
                                       const t_rt_node* const* rt_node_of_sink,
                                       ClusterNetId inet,
                                       TimingInfo* timing_info,
                                       ClusteredPinTimingInvalidator* pin_timing_invalidator) {
    /* Goes through all the sinks of this net and copies their delay values from
     * the route_tree to the net_delay array.                                    */

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& clb_nlist = cluster_ctx.clb_nlist;

    for (unsigned int isink = 1; isink < cluster_ctx.clb_nlist.net_pins(inet).size(); isink++) {
        float new_delay = rt_node_of_sink[isink]->Tdel;

        if (pin_timing_invalidator && new_delay != net_delay[isink]) {
            //Delay changed, invalidate for incremental timing update
            VTR_ASSERT_SAFE(timing_info);
            ClusterPinId pin = clb_nlist.net_pin(inet, isink);
            pin_timing_invalidator->invalidate_connection(pin, timing_info);
        }

        net_delay[isink] = new_delay;
    }
}

/***************  Conversion between traceback and route tree *******************/
t_rt_node* traceback_to_route_tree(ClusterNetId inet, std::vector<int>* non_config_node_set_usage) {
    auto& route_ctx = g_vpr_ctx.routing();
    return traceback_to_route_tree(route_ctx.trace[inet].head, non_config_node_set_usage);
}

t_rt_node* traceback_to_route_tree(ClusterNetId inet) {
    return traceback_to_route_tree(inet, nullptr);
}

t_rt_node* traceback_to_route_tree(t_trace* head) {
    return traceback_to_route_tree(head, nullptr);
}

t_rt_node* traceback_to_route_tree(t_trace* head, std::vector<int>* non_config_node_set_usage) {
    /* Builds a skeleton route tree from a traceback
     * does not calculate R_upstream, C_downstream, or Tdel at all (left uninitialized)
     * returns the root of the converted route tree
     * initially points at the traceback equivalent of root 							  */

    if (head == nullptr) {
        return nullptr;
    }

    VTR_ASSERT_DEBUG(validate_traceback(head));

    std::map<int, t_rt_node*> rr_node_to_rt;

    t_trace* trace = head;
    while (trace) { //Each branch
        trace = traceback_to_route_tree_branch(trace, rr_node_to_rt, non_config_node_set_usage);
    }
    // Due to the recursive nature of traceback_to_route_tree_branch,
    // the source node is not properly configured.
    // Here, for the source we set the parent node and switch to be
    // nullptr and OPEN respectively.

    rr_node_to_rt[head->index]->parent_node = nullptr;
    rr_node_to_rt[head->index]->parent_switch = OPEN;

    return rr_node_to_rt[head->index];
}

//Constructs a single branch of a route tree from a traceback
//Note that R_upstream and C_downstream are initialized to NaN
//
//Returns the t_trace defining the start of the next branch
static t_trace* traceback_to_route_tree_branch(t_trace* trace,
                                               std::map<int, t_rt_node*>& rr_node_to_rt,
                                               std::vector<int>* non_config_node_set_usage) {
    t_trace* next = nullptr;

    if (trace) {
        t_rt_node* node = nullptr;

        int inode = trace->index;
        int ipin = trace->net_pin_index;
        int iswitch = trace->iswitch;

        auto& device_ctx = g_vpr_ctx.device();
        const auto& rr_graph = device_ctx.rr_graph;
        auto itr = rr_node_to_rt.find(trace->index);

        // In some cases, the same sink node is put into the tree multiple times in a single route.
        // So it is possible to hit the same node index multiple times during traceback. Create a
        // separate rt_node for each sink with the same node index.
        if (itr == rr_node_to_rt.end() || rr_graph.node_type(RRNodeId(inode)) == SINK) {
            //Create

            //Initialize route tree node
            node = alloc_rt_node();
            node->inode = inode;
            node->net_pin_index = ipin;
            node->u.child_list = nullptr;

            node->R_upstream = std::numeric_limits<float>::quiet_NaN();
            node->C_downstream = std::numeric_limits<float>::quiet_NaN();
            node->Tdel = std::numeric_limits<float>::quiet_NaN();

            auto node_type = rr_graph.node_type(RRNodeId(inode));
            if (node_type == IPIN || node_type == SINK)
                node->re_expand = false;
            else
                node->re_expand = true;

            if (node_type == SINK) {
                // A non-configurable edge to a sink is also a usage of the
                // set.
                auto set_itr = device_ctx.rr_node_to_non_config_node_set.find(inode);
                if (non_config_node_set_usage != nullptr && set_itr != device_ctx.rr_node_to_non_config_node_set.end()) {
                    if (rr_graph.rr_switch_inf(RRSwitchId(iswitch)).configurable()) {
                        (*non_config_node_set_usage)[set_itr->second] += 1;
                    }
                }
            }

            //Save
            rr_node_to_rt[inode] = node;
        } else {
            //Found
            node = itr->second;
        }
        VTR_ASSERT(node);

        next = trace->next;
        if (iswitch != OPEN) {
            // Keep track of non-configurable set usage by looking for
            // configurable edges that extend out of a non-configurable set.
            //
            // Each configurable edges from the non-configurable set is a
            // usage of the set.
            auto set_itr = device_ctx.rr_node_to_non_config_node_set.find(inode);
            if (non_config_node_set_usage != nullptr && set_itr != device_ctx.rr_node_to_non_config_node_set.end()) {
                if (rr_graph.rr_switch_inf(RRSwitchId(iswitch)).configurable()) {
                    (*non_config_node_set_usage)[set_itr->second] += 1;
                }
            }

            //Recursively construct the remaining branch
            next = traceback_to_route_tree_branch(next, rr_node_to_rt, non_config_node_set_usage);

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

static std::pair<t_trace*, t_trace*> traceback_from_route_tree_recurr(t_trace* head, t_trace* tail, const t_rt_node* node) {
    if (node) {
        if (node->u.child_list) {
            //Recursively add children
            for (t_linked_rt_edge* edge = node->u.child_list; edge != nullptr; edge = edge->next) {
                t_trace* curr = alloc_trace_data();
                curr->index = node->inode;
                curr->net_pin_index = node->net_pin_index;
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
            curr->net_pin_index = node->net_pin_index;
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

    return {head, tail};
}

t_trace* traceback_from_route_tree(ClusterNetId inet, const t_rt_node* root, int num_routed_sinks) {
    /* Creates the traceback for net inet from the route tree rooted at root
     * properly sets route_ctx.trace_head and route_ctx.trace_tail for this net
     * returns the trace head for inet 					 					 */

    auto& route_ctx = g_vpr_ctx.mutable_routing();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

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
        if (rr_graph.node_type(RRNodeId(trace->index)) == SINK) {
            num_trace_sinks += 1;
        }
    }
    VTR_ASSERT(num_routed_sinks == num_trace_sinks);

    route_ctx.trace[inet].tail = tail;
    route_ctx.trace[inet].head = head;
    route_ctx.trace_nodes[inet] = nodes;

    return head;
}

//Prunes a route tree (recursively) based on congestion and the 'force_prune' argument
//
//Returns true if the current node was pruned
static t_rt_node* prune_route_tree_recurr(t_rt_node* node, CBRR& connections_inf, bool force_prune, std::vector<int>* non_config_node_set_usage) {
    //Recursively traverse the route tree rooted at node and remove any congested
    //sub-trees

    VTR_ASSERT(node);

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();
    bool congested = (route_ctx.rr_node_route_inf[node->inode].occ() > rr_graph.node_capacity(RRNodeId(node->inode)));
    int node_set = -1;
    auto itr = device_ctx.rr_node_to_non_config_node_set.find(node->inode);
    if (itr != device_ctx.rr_node_to_non_config_node_set.end()) {
        node_set = itr->second;
    }

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
        t_rt_node* child = prune_route_tree_recurr(edge->child,
                                                   connections_inf, force_prune, non_config_node_set_usage);

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

            // After removing an edge, check if non_config_node_set_usage
            // needs an update.
            if (non_config_node_set_usage != nullptr && node_set != -1 && rr_graph.rr_switch_inf(RRSwitchId(old_edge->iswitch)).configurable()) {
                (*non_config_node_set_usage)[node_set] -= 1;
                VTR_ASSERT((*non_config_node_set_usage)[node_set] >= 0);
            }

            free_linked_rt_edge(old_edge);

            //Note prev_edge is unchanged

        } else { //Child not pruned
            all_children_pruned = false;

            //Edge not removed
            prev_edge = edge;
            edge = edge->next;
        }
    }

    if (rr_graph.node_type(RRNodeId(node->inode)) == SINK) {
        if (!force_prune) {
            //Valid path to sink

            //Record sink as reachable
            connections_inf.reached_rt_sink(node);

            return node; //Not pruned
        } else {
            VTR_ASSERT(force_prune);

            //Record as not reached
            connections_inf.toreach_rr_sink(node->net_pin_index);

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
        //   2) Prune the node only if the node set is unused or if force_prune
        //      is set.
        //
        //   3) Prune the node.
        //
        //      This avoid the creation of unused 'stubs'. (For example if
        //      we left the stubs in and they were subsequently not used
        //      they would uselessly consume routing resources).
        VTR_ASSERT(node->u.child_list == nullptr);

        bool reached_non_configurably = false;
        if (node->parent_node) {
            reached_non_configurably = !rr_graph.rr_switch_inf(RRSwitchId(node->parent_switch)).configurable();

            if (reached_non_configurably) {
                // Check if this non-configurable node set is in use.
                VTR_ASSERT(node_set != -1);
                if (non_config_node_set_usage != nullptr && (*non_config_node_set_usage)[node_set] == 0) {
                    force_prune = true;
                }
            }
        }

        if (reached_non_configurably && !force_prune) {
            return node; //Not pruned
        } else {
            free_rt_node(node);
            return nullptr; //Pruned
        }

    } else {
        // If this node is:
        //   1. Part of a non-configurable node set
        //   2. The first node in the tree that is part of the non-configurable
        //      node set
        //
        //      -- and --
        //
        //   3. The node set is not active
        //
        //  Then prune this node.
        //
        if (non_config_node_set_usage != nullptr && node_set != -1 && rr_graph.rr_switch_inf(RRSwitchId(node->parent_switch)).configurable() && (*non_config_node_set_usage)[node_set] == 0) {
            // This node should be pruned, re-prune edges once more.
            //
            // If the following is true:
            //
            //  - The node set is unused
            //    (e.g. (*non_config_node_set_usage)[node_set] == 0)
            //  - This particular node still had children
            //    (which is true by virtue of being in this else statement)
            //
            // Then that indicates that the node set became unused during the
            // pruning. One or more of the children of this node will be
            // pruned if prune_route_tree_recurr is called again, and
            // eventually the whole node will be prunable.
            //
            //  Consider the following graph:
            //
            //  1 -> 2
            //  2 -> 3 [non-configurable]
            //  2 -> 4 [non-configurable]
            //  3 -> 5
            //  4 -> 6
            //
            //  Assume that nodes 5 and 6 do not connect to a sink, so they
            //  will be pruned (as normal). When prune_route_tree_recurr
            //  visits 2 for the first time, node 3 or 4 will remain. This is
            //  because when prune_route_tree_recurr visits 3 or 4, it will
            //  not have visited 4 or 3 (respectively). As a result, either
            //  node 3 or 4 will not be pruned on the first pass, because the
            //  node set usage count will be > 0. However after
            //  prune_route_tree_recurr visits 2, 3 and 4, the node set usage
            //  will be 0, so everything can be pruned.
            return prune_route_tree_recurr(node, connections_inf,
                                           /*force_prune=*/false, non_config_node_set_usage);
        }

        //An unpruned intermediate node
        VTR_ASSERT(!force_prune);

        return node; //Not pruned
    }
}

t_rt_node* prune_route_tree(t_rt_node* rt_root, CBRR& connections_inf) {
    return prune_route_tree(rt_root, connections_inf, nullptr);
}

t_rt_node* prune_route_tree(t_rt_node* rt_root, CBRR& connections_inf, std::vector<int>* non_config_node_set_usage) {
    /* Prune a skeleton route tree of illegal branches - when there is at least 1 congested node on the path to a sink
     * This is the top level function to be called with the SOURCE node as root.
     * Returns true if the entire tree has been pruned.
     *
     * Note: does not update R_upstream/C_downstream
     */

    VTR_ASSERT(rt_root);

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    VTR_ASSERT_MSG(rr_graph.node_type(RRNodeId(rt_root->inode)) == SOURCE, "Root of route tree must be SOURCE");

    VTR_ASSERT_MSG(route_ctx.rr_node_route_inf[rt_root->inode].occ() <= rr_graph.node_capacity(RRNodeId(rt_root->inode)),
                   "Route tree root/SOURCE should never be congested");

    return prune_route_tree_recurr(rt_root, connections_inf, false, non_config_node_set_usage);
}

void pathfinder_update_cost_from_route_tree(const t_rt_node* rt_root, int add_or_sub) {
    /* Update pathfinder cost of all nodes rooted at rt_root, including rt_root itself */

    VTR_ASSERT(rt_root != nullptr);

    t_linked_rt_edge* edge{rt_root->u.child_list};

    // update every node once, so even do it for sinks and branch points once
    for (;;) {
        pathfinder_update_single_node_occupancy(rt_root->inode, add_or_sub);

        // reached a sink
        if (!edge) {
            return;
        }
        // branch point (sibling edges)
        else if (edge->next) {
            // recursively update for each of its sibling branches
            do {
                pathfinder_update_cost_from_route_tree(edge->child, add_or_sub);
                edge = edge->next;
            } while (edge);
            return;
        }

        rt_root = edge->child;
        edge = rt_root->u.child_list;
    }
}

/***************** Debugging and printing for incremental rerouting ****************/
template<typename Op>
static void traverse_indented_route_tree(const t_rt_node* rt_root, int branch_level, bool new_branch, Op op, int indent_level) {
    /* pretty print the route tree; what's printed depends on the printer Op passed in */

    // rely on preorder depth first traversal
    VTR_ASSERT(rt_root != nullptr);
    t_linked_rt_edge* edges = rt_root->u.child_list;
    // print branch indent
    if (new_branch) VTR_LOG("\n%*s", indent_level * branch_level, " \\ ");

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
    if (!edge) {
        VTR_LOG("null");
        return;
    }
    while (edge) {
        VTR_LOG("%d(%d) ", edge->child->inode, edge->iswitch);
        edge = edge->next;
    }
    VTR_LOG("\n");
}

static void print_node(const t_rt_node* rt_node) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    int inode = rt_node->inode;
    t_rr_type node_type = rr_graph.node_type(RRNodeId(inode));
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
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    int inode = rt_node->inode;
    const auto& node_inf = route_ctx.rr_node_route_inf[inode];
    const short& node_capacity = rr_graph.node_capacity(RRNodeId(inode));
    VTR_LOG("%2d %2d|%-6d-> ", node_inf.acc_cost, rt_node->Tdel,
            node_inf.occ(), node_capacity, inode);
}

void print_route_tree_inf(const t_rt_node* rt_root) {
    traverse_indented_route_tree(rt_root, 0, false, print_node_inf, 34);
    VTR_LOG("\n");
}

void print_route_tree_node(const t_rt_node* rt_root) {
    traverse_indented_route_tree(rt_root, 0, false, print_node, 34);
    VTR_LOG("\n");
}

void print_route_tree_congestion(const t_rt_node* rt_root) {
    traverse_indented_route_tree(rt_root, 0, false, print_node_congestion, 15);
    VTR_LOG("\n");
}

/* the following is_* functions are for debugging correctness of pruned route tree
 * these should only be called when the debug switch DEBUG_INCREMENTAL_REROUTING is on */
bool is_equivalent_route_tree(const t_rt_node* root, const t_rt_node* root_clone) {
    if (!root && !root_clone) return true;
    if (!root || !root_clone) return false; // one of them is null
    if ((root->inode != root_clone->inode) || (!equal_approx(root->R_upstream, root_clone->R_upstream)) || (!equal_approx(root->C_downstream, root_clone->C_downstream)) || (!equal_approx(root->Tdel, root_clone->Tdel))) {
        VTR_LOG("mismatch i %d|%d R %e|%e C %e|%e T %e %e\n",
                root->inode, root_clone->inode,
                root->R_upstream, root_clone->R_upstream,
                root->C_downstream, root_clone->C_downstream,
                root->Tdel, root_clone->Tdel);
        return false;
    }
    t_linked_rt_edge* orig_edge{root->u.child_list};
    t_linked_rt_edge* clone_edge{root_clone->u.child_list};
    while (orig_edge && clone_edge) {
        if (orig_edge->iswitch != clone_edge->iswitch)
            VTR_LOG("mismatch i %d|%d edge switch %d|%d\n",
                    root->inode, root_clone->inode,
                    orig_edge->iswitch, clone_edge->iswitch);
        if (!is_equivalent_route_tree(orig_edge->child, clone_edge->child)) return false; // child trees not equivalent
        orig_edge = orig_edge->next;
        clone_edge = clone_edge->next;
    }
    if (orig_edge || clone_edge) {
        VTR_LOG("one of the trees have an extra edge!\n");
        return false;
    }
    return true; // passed all tests
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
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    constexpr float CAP_REL_TOL = 1e-6;
    constexpr float CAP_ABS_TOL = vtr::DEFAULT_ABS_TOL;
    constexpr float RES_REL_TOL = 1e-6;
    constexpr float RES_ABS_TOL = vtr::DEFAULT_ABS_TOL;

    int inode = root->inode;
    short iswitch = root->parent_switch;
    if (root->parent_node) {
        if (rr_graph.rr_switch_inf(RRSwitchId(iswitch)).buffered()) {
            float R_upstream_check = rr_graph.node_R(RRNodeId(inode)) + rr_graph.rr_switch_inf(RRSwitchId(iswitch)).R;
            if (!vtr::isclose(root->R_upstream, R_upstream_check, RES_REL_TOL, RES_ABS_TOL)) {
                VTR_LOG("%d mismatch R upstream %e supposed %e\n", inode, root->R_upstream, R_upstream_check);
                return false;
            }
        } else {
            float R_upstream_check = rr_graph.node_R(RRNodeId(inode)) + root->parent_node->R_upstream + rr_graph.rr_switch_inf(RRSwitchId(iswitch)).R;
            if (!vtr::isclose(root->R_upstream, R_upstream_check, RES_REL_TOL, RES_ABS_TOL)) {
                VTR_LOG("%d mismatch R upstream %e supposed %e\n", inode, root->R_upstream, R_upstream_check);
                return false;
            }
        }
    } else if (root->R_upstream != rr_graph.node_R(RRNodeId(inode))) {
        VTR_LOG("%d mismatch R upstream %e supposed %e\n", inode, root->R_upstream, rr_graph.node_R(RRNodeId(inode)));
        return false;
    }

    // check downstream C
    t_linked_rt_edge* edge = root->u.child_list;
    float C_downstream_children{0};
    // sink, must not be congested
    if (!edge) {
        int occ = route_ctx.rr_node_route_inf[inode].occ();
        int capacity = rr_graph.node_capacity(RRNodeId(inode));
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

        C_downstream_children += rr_graph.rr_switch_inf(RRSwitchId(edge->iswitch)).Cinternal;

        if (!rr_graph.rr_switch_inf(RRSwitchId(edge->iswitch)).buffered()) {
            C_downstream_children += edge->child->C_downstream;
        }

        if (!is_valid_route_tree(edge->child)) {
            VTR_LOG("subtree %d invalid, propagating up\n", edge->child->inode);
            return false;
        }
        edge = edge->next;
    }
    float C_downstream_check = C_downstream_children + rr_graph.node_C(RRNodeId(inode));
    if (!vtr::isclose(root->C_downstream, C_downstream_check, CAP_REL_TOL, CAP_ABS_TOL)) {
        VTR_LOG("%d mismatch C downstream %e supposed %e\n", inode, root->C_downstream, C_downstream_check);
        return false;
    }

    return true;
}

//Returns true if the route tree rooted at 'root' is not congested
bool is_uncongested_route_tree(const t_rt_node* root) {
    auto& route_ctx = g_vpr_ctx.routing();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    int inode = root->inode;
    if (route_ctx.rr_node_route_inf[inode].occ() > rr_graph.node_capacity(RRNodeId(inode))) {
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

    t_rt_node* rt_root;

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    rt_root = alloc_rt_node();
    rt_root->u.child_list = nullptr;
    rt_root->parent_node = nullptr;
    rt_root->parent_switch = OPEN;
    rt_root->re_expand = true;
    rt_root->inode = inode;
    rt_root->net_pin_index = OPEN;
    rt_root->C_downstream = rr_graph.node_C(RRNodeId(inode));
    rt_root->R_upstream = rr_graph.node_R(RRNodeId(inode));
    rt_root->Tdel = 0.5 * rr_graph.node_R(RRNodeId(inode)) * rr_graph.node_C(RRNodeId(inode));
    rr_node_to_rt_node[inode] = rt_root;

    return (rt_root);
}

bool verify_traceback_route_tree_equivalent(const t_trace* head, const t_rt_node* rt_root) {
    //Walk the route tree saving all the used connections
    std::multiset<std::tuple<int, int, int>> route_tree_connections;
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
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Route tree missing traceback connection: node %d -> %d (switch %d)\n",
                                prev_node, to_node, prev_switch);
            } else {
                route_tree_connections.erase(route_tree_connections.lower_bound(conn)); //Remove the first found connections
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

        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, msg.c_str());
    }

    return true;
}

void collect_route_tree_connections(const t_rt_node* node, std::multiset<std::tuple<int, int, int>>& connections) {
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

t_rt_node* find_sink_rt_node(t_rt_node* rt_root, ClusterNetId net_id, ClusterPinId sink_pin) {
    //Given the net_id and the sink_pin, this two-step function finds a pointer to the
    //route tree sink corresponding to sink_pin. This function constitutes the first step,
    //in which, we loop through the pins of the net and terminate the search once the mapping
    //of (net_id, ipin) -> sink_pin is found. Conveniently, the pair (net_id, ipin) can
    //be further translated to the index of the routing resource node sink_rr_inode.
    //In the second step, we pass the root of the route tree and sink_rr_inode in order to
    //recursively traverse the route tree until we reach the sink node that corresponds
    //to sink_rr_inode.

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

    int ipin = cluster_ctx.clb_nlist.pin_net_index(sink_pin);
    int sink_rr_inode = route_ctx.net_rr_terminals[net_id][ipin]; //obtain the value of the routing resource sink

    t_rt_node* sink_rt_node = find_sink_rt_node_recurr(rt_root, sink_rr_inode); //find pointer to route tree node corresponding to sink_rr_inode
    VTR_ASSERT(sink_rt_node);
    return sink_rt_node;
}
t_rt_node* find_sink_rt_node_recurr(t_rt_node* node, int sink_rr_inode) {
    if (node->inode == sink_rr_inode) { //check if current node matches sink_rr_inode
        return node;
    }

    for (t_linked_rt_edge* edge = node->u.child_list; edge != nullptr; edge = edge->next) {
        t_rt_node* found_node = find_sink_rt_node_recurr(edge->child, sink_rr_inode); //process each of the children
        if (found_node && found_node->inode == sink_rr_inode) {
            //If the sink has been found downstream in the branch, we would like to immediately exit the search
            return found_node;
        }
    }
    return nullptr; //We have not reached the sink node
}
