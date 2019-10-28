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
#include "route_tree_timing_obj.h"

/* This module keeps track of the partial routing tree for timing-driven     *
 * routing.  The normal traceback structure doesn't provide enough info      *
 * about the partial routing during timing-driven routing, so the routines   *
 * in this module are used to keep a tree representation of the partial      *
 * routing during timing-driven routing.  This allows rapid incremental      *
 * timing analysis.                                                          */

/********************** Variables local to this module ***********************/

static std::unique_ptr<RouteTreeTiming<
    decltype(DeviceContext::rr_nodes),
    decltype(DeviceContext::rr_node_to_non_config_node_set),
    decltype(DeviceContext::rr_switch_inf)>>
    tree_timing;

/********************** Subroutines local to this module *********************/

static bool verify_route_tree_recurr(t_rt_node* node, std::set<int>& seen_nodes);

static std::pair<t_trace*, t_trace*> traceback_from_route_tree_recurr(t_trace* head, t_trace* tail, const t_rt_node* node);

static void collect_route_tree_connections(const t_rt_node* node, std::set<std::tuple<int, int, int>>& connections);

/************************** Subroutine definitions ***************************/

constexpr float epsilon = 1e-15;
static bool equal_approx(float a, float b) {
    return fabs(a - b) < epsilon;
}

bool alloc_route_tree_timing_structs(bool exists_ok) {
    /* Allocates any structures needed to build the routing trees. */

    auto& device_ctx = g_vpr_ctx.device();

    bool route_tree_structs_are_allocated = bool(tree_timing);
    if (route_tree_structs_are_allocated) {
        if (exists_ok) {
            return false;
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "in alloc_route_tree_timing_structs: old structures already exist.\n");
        }
    }

    tree_timing = std::make_unique<RouteTreeTiming<
        decltype(device_ctx.rr_nodes),
        decltype(device_ctx.rr_node_to_non_config_node_set),
        decltype(device_ctx.rr_switch_inf)>>(
        device_ctx.rr_nodes,
        device_ctx.rr_node_to_non_config_node_set,
        device_ctx.rr_switch_inf);

    return true;
}

void free_route_tree_timing_structs() {
    /* Frees the structures needed to build routing trees, and really frees
     * (i.e. calls free) all the data on the free lists.                         */
    tree_timing.reset();
}

/* Initializes the routing tree to just the net source, and returns the root
 * node of the rt_tree (which is just the net source).                       */
t_rt_node* init_route_tree_to_source(ClusterNetId inet) {
    auto& route_ctx = g_vpr_ctx.routing();
    return tree_timing->init_route_tree_to_source(route_ctx.net_rr_terminals, inet);
}

/* Adds the most recently finished wire segment to the routing tree, and
 * updates the Tdel, etc. numbers for the rest of the routing tree.  hptr
 * is the heap pointer of the SINK that was reached.  This routine returns
 * a pointer to the rt_node of the SINK that it adds to the routing.        */
t_rt_node* update_route_tree(t_heap* hptr, SpatialRouteTreeLookup* spatial_rt_lookup) {
    auto& route_ctx = g_vpr_ctx.routing();
    return tree_timing->update_route_tree<decltype(route_ctx.rr_node_route_inf)>(route_ctx.rr_node_route_inf, hptr, spatial_rt_lookup);
}

void add_route_tree_to_rr_node_lookup(t_rt_node* node) {
    tree_timing->add_route_tree_to_rr_node_lookup(node);
}

void load_new_subtree_R_upstream(t_rt_node* rt_node) {
    tree_timing->load_new_subtree_R_upstream(rt_node);
}

float load_new_subtree_C_downstream(t_rt_node* rt_node) {
    return tree_timing->load_new_subtree_C_downstream(rt_node);
}

void load_route_tree_Tdel(t_rt_node* subtree_rt_root, float Tarrival) {
    tree_timing->load_route_tree_Tdel(subtree_rt_root, Tarrival);
}

void load_route_tree_rr_route_inf(t_rt_node* root) {
    auto& route_ctx = g_vpr_ctx.mutable_routing();
    return tree_timing->load_route_tree_rr_route_inf<decltype(route_ctx.rr_node_route_inf)>(
        &route_ctx.rr_node_route_inf, root);
}

bool verify_route_tree(t_rt_node* root) {
    std::set<int> seen_nodes;
    return verify_route_tree_recurr(root, seen_nodes);
}

static bool verify_route_tree_recurr(t_rt_node* node, std::set<int>& seen_nodes) {
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
    tree_timing->free_route_tree(rt_node);
}

void print_route_tree(const t_rt_node* rt_node) {
    auto& route_ctx = g_vpr_ctx.routing();
    return tree_timing->print_route_tree<decltype(route_ctx.rr_node_route_inf)>(
        route_ctx.rr_node_route_inf,
        rt_node);
}

void print_route_tree(const t_rt_node* rt_node, int depth) {
    auto& route_ctx = g_vpr_ctx.routing();
    return tree_timing->print_route_tree<decltype(route_ctx.rr_node_route_inf)>(
        route_ctx.rr_node_route_inf,
        rt_node, depth);
}

void update_net_delays_from_route_tree(float* net_delay,
                                       const t_rt_node* const* rt_node_of_sink,
                                       ClusterNetId inet) {
    /* Goes through all the sinks of this net and copies their delay values from
     * the route_tree to the net_delay array.                                    */

    auto& cluster_ctx = g_vpr_ctx.clustering();
    for (unsigned int isink = 1; isink < cluster_ctx.clb_nlist.net_pins(inet).size(); isink++) {
        net_delay[isink] = rt_node_of_sink[isink]->Tdel;
    }
}

void update_remaining_net_delays_from_route_tree(float* net_delay,
                                                 const t_rt_node* const* rt_node_of_sink,
                                                 const std::vector<int>& remaining_sinks) {
    /* Like update_net_delays_from_route_tree, but only updates the sinks that were not already routed
     * this function doesn't actually need to know about the net, just what sink pins need their net delays updated */

    for (int sink_pin : remaining_sinks)
        net_delay[sink_pin] = rt_node_of_sink[sink_pin]->Tdel;
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
    return tree_timing->traceback_to_route_tree(head, non_config_node_set_usage);
}

static std::pair<t_trace*, t_trace*> traceback_from_route_tree_recurr(t_trace* head, t_trace* tail, const t_rt_node* node) {
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

    return {head, tail};
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

    route_ctx.trace[inet].tail = tail;
    route_ctx.trace[inet].head = head;
    route_ctx.trace_nodes[inet] = nodes;

    return head;
}

t_rt_node* prune_route_tree(t_rt_node* rt_root, CBRR& connections_inf) {
    auto& route_ctx = g_vpr_ctx.routing();
    return tree_timing->prune_route_tree<decltype(route_ctx.rr_node_route_inf)>(
        route_ctx.rr_node_route_inf, rt_root, connections_inf, nullptr);
}

t_rt_node* prune_route_tree(t_rt_node* rt_root, CBRR& connections_inf, std::vector<int>* non_config_node_set_usage) {
    auto& route_ctx = g_vpr_ctx.routing();
    return tree_timing->prune_route_tree<decltype(route_ctx.rr_node_route_inf)>(
        route_ctx.rr_node_route_inf, rt_root, connections_inf, non_config_node_set_usage);
}

void pathfinder_update_cost_from_route_tree(const t_rt_node* rt_root, int add_or_sub, float pres_fac) {
    /* Update pathfinder cost of all nodes rooted at rt_root, including rt_root itself */

    VTR_ASSERT(rt_root != nullptr);

    t_linked_rt_edge* edge{rt_root->u.child_list};

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
    auto& route_ctx = g_vpr_ctx.routing();

    constexpr float CAP_REL_TOL = 1e-6;
    constexpr float CAP_ABS_TOL = vtr::DEFAULT_ABS_TOL;
    constexpr float RES_REL_TOL = 1e-6;
    constexpr float RES_ABS_TOL = vtr::DEFAULT_ABS_TOL;

    int inode = root->inode;
    short iswitch = root->parent_switch;
    if (root->parent_node) {
        if (device_ctx.rr_switch_inf[iswitch].buffered()) {
            float R_upstream_check = device_ctx.rr_nodes[inode].R() + device_ctx.rr_switch_inf[iswitch].R;
            if (!vtr::isclose(root->R_upstream, R_upstream_check, RES_REL_TOL, RES_ABS_TOL)) {
                VTR_LOG("%d mismatch R upstream %e supposed %e\n", inode, root->R_upstream, R_upstream_check);
                return false;
            }
        } else {
            float R_upstream_check = device_ctx.rr_nodes[inode].R() + root->parent_node->R_upstream + device_ctx.rr_switch_inf[iswitch].R;
            if (!vtr::isclose(root->R_upstream, R_upstream_check, RES_REL_TOL, RES_ABS_TOL)) {
                VTR_LOG("%d mismatch R upstream %e supposed %e\n", inode, root->R_upstream, R_upstream_check);
                return false;
            }
        }
    } else if (root->R_upstream != device_ctx.rr_nodes[inode].R()) {
        VTR_LOG("%d mismatch R upstream %e supposed %e\n", inode, root->R_upstream, device_ctx.rr_nodes[inode].R());
        return false;
    }

    // check downstream C
    t_linked_rt_edge* edge = root->u.child_list;
    float C_downstream_children{0};
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

        if (!device_ctx.rr_switch_inf[edge->iswitch].buffered()) {
            C_downstream_children += edge->child->C_downstream;
        }

        if (!is_valid_route_tree(edge->child)) {
            VTR_LOG("subtree %d invalid, propagating up\n", edge->child->inode);
            return false;
        }
        edge = edge->next;
    }

    float C_downstream_check = C_downstream_children + device_ctx.rr_nodes[inode].C();
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
    return tree_timing->init_route_tree_to_source_no_net(inode);
}

bool verify_traceback_route_tree_equivalent(const t_trace* head, const t_rt_node* rt_root) {
    //Walk the route tree saving all the used connections
    std::set<std::tuple<int, int, int>> route_tree_connections;
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

        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, msg.c_str());
    }

    return true;
}

static void collect_route_tree_connections(const t_rt_node* node, std::set<std::tuple<int, int, int>>& connections) {
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
