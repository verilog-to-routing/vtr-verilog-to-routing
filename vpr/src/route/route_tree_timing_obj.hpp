#include "clustered_netlist_fwd.h"

template<typename RrNodeInf, typename RrNodeSetInf, typename SwitchInf>
t_rt_node* RouteTreeTiming<RrNodeInf, RrNodeSetInf, SwitchInf>::init_route_tree_to_source(ClusterNetId inet) {
    /* Initializes the routing tree to just the net source, and returns the root
     * node of the rt_tree (which is just the net source).                       */

    t_rt_node* rt_root;

    rt_root = rt_nodes_.alloc();
    rt_root->u.child_list = nullptr;
    rt_root->parent_node = nullptr;
    rt_root->parent_switch = OPEN;
    rt_root->re_expand = true;

    int inode = net_rr_terminals_[inet][0]; /* Net source */

    rt_root->inode = inode;
    rt_root->C_downstream = node_inf_[inode].C();
    rt_root->R_upstream = node_inf_[inode].R();
    rt_root->Tdel = 0.5 * node_inf_[inode].R() * node_inf_[inode].C();
    rr_node_to_rt_node_[inode] = rt_root;

    return (rt_root);
}

template<typename RrNodeInf, typename RrNodeSetInf, typename SwitchInf>
void RouteTreeTiming<RrNodeInf, RrNodeSetInf, SwitchInf>::free_route_tree(t_rt_node* rt_node) {
    /* Puts the rt_nodes and edges in the tree rooted at rt_node back on the
     * free lists.  Recursive, depth-first post-order traversal.                */

    t_linked_rt_edge *rt_edge, *next_edge;

    rt_edge = rt_node->u.child_list;

    while (rt_edge != nullptr) { /* For all children */
        t_rt_node* child_node = rt_edge->child;
        free_route_tree(child_node);
        next_edge = rt_edge->next;
        rt_edges_.free(rt_edge);
        rt_edge = next_edge;
    }

    if (!rr_node_to_rt_node_.empty()) {
        rr_node_to_rt_node_.at(rt_node->inode) = nullptr;
    }

    rt_nodes_.free(rt_node);
}

template<typename RrNodeInf, typename RrNodeSetInf, typename SwitchInf>
template<typename RouteInf>
void RouteTreeTiming<RrNodeInf, RrNodeSetInf, SwitchInf>::print_route_tree(const RouteInf& route_inf, const t_rt_node* rt_node) const {
    print_route_tree(route_inf, rt_node, 0);
}

template<typename RrNodeInf, typename RrNodeSetInf, typename SwitchInf>
template<typename RouteInf>
void RouteTreeTiming<RrNodeInf, RrNodeSetInf, SwitchInf>::print_route_tree(const RouteInf& route_inf, const t_rt_node* rt_node, int depth) const {
    std::string indent;
    for (int i = 0; i < depth; ++i) {
        indent += "  ";
    }

    VTR_LOG("%srt_node: %d (%s)", indent.c_str(), rt_node->inode, node_inf_[rt_node->inode].type_string());

    if (rt_node->parent_switch != OPEN) {
        bool parent_edge_configurable = switch_inf_[rt_node->parent_switch].configurable();
        if (!parent_edge_configurable) {
            VTR_LOG("*");
        }
    }

    if (route_inf[rt_node->inode].occ() > node_inf_[rt_node->inode].capacity()) {
        VTR_LOG(" x");
    }

    VTR_LOG("\n");

    for (t_linked_rt_edge* rt_edge = rt_node->u.child_list; rt_edge != nullptr; rt_edge = rt_edge->next) {
        print_route_tree(route_inf, rt_edge->child, depth + 1);
    }
}

/* Adds the most recently finished wire segment to the routing tree, and
 * updates the Tdel, etc. numbers for the rest of the routing tree.  hptr
 * is the heap pointer of the SINK that was reached.  This routine returns
 * a pointer to the rt_node of the SINK that it adds to the routing.        */
template<typename RrNodeInf, typename RrNodeSetInf, typename SwitchInf>
template<typename RouteInf>
t_rt_node* RouteTreeTiming<RrNodeInf, RrNodeSetInf, SwitchInf>::update_route_tree(const RouteInf& route_inf, t_heap* hptr, SpatialRouteTreeLookup* spatial_rt_lookup) {
    t_rt_node *start_of_new_subtree_rt_node, *sink_rt_node;
    t_rt_node *unbuffered_subtree_rt_root, *subtree_parent_rt_node;
    float Tdel_start;
    short iswitch;

    //Create a new subtree from the target in hptr to existing routing
    start_of_new_subtree_rt_node = add_subtree_to_route_tree(route_inf, hptr, &sink_rt_node);

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
        Tdel_start += switch_inf_[iswitch].R * unbuffered_subtree_rt_root->C_downstream;
        Tdel_start += switch_inf_[iswitch].Tdel;
    } else { /* Subtree starts at SOURCE */
        Tdel_start = 0.;
    }

    load_route_tree_Tdel(unbuffered_subtree_rt_root, Tdel_start);

    if (spatial_rt_lookup) {
        update_route_tree_spatial_lookup_recur(start_of_new_subtree_rt_node, *spatial_rt_lookup);
    }

    return (sink_rt_node);
}

template<typename RrNodeInf, typename RrNodeSetInf, typename SwitchInf>
template<typename RouteInf>
t_rt_node*
RouteTreeTiming<RrNodeInf, RrNodeSetInf, SwitchInf>::add_subtree_to_route_tree(const RouteInf& route_inf, t_heap* hptr, t_rt_node** sink_rt_node_ptr) {
    /* Adds the most recent wire segment, ending at the SINK indicated by hptr,
     * to the routing tree.  It returns the first (most upstream) new rt_node,
     * and (via a pointer) the rt_node of the new SINK. Traverses up from SINK  */

    t_rt_node *rt_node, *downstream_rt_node, *sink_rt_node;
    t_linked_rt_edge* linked_rt_edge;

    int inode = hptr->index;

    sink_rt_node = rt_nodes_.alloc();
    sink_rt_node->u.child_list = nullptr;
    sink_rt_node->inode = inode;
    rr_node_to_rt_node_[inode] = sink_rt_node;

    /* In the code below I'm marking SINKs and IPINs as not to be re-expanded.
     * It makes the code more efficient (though not vastly) to prune this way
     * when there aren't route-throughs or ipin doglegs.                        */

    sink_rt_node->re_expand = false;

    /* Now do it's predecessor. */

    downstream_rt_node = sink_rt_node;

    std::unordered_set<int> main_branch_visited;
    std::unordered_set<int> all_visited;
    inode = hptr->u.prev.node;
    t_edge_size iedge = hptr->u.prev.edge;
    short iswitch = node_inf_[inode].edge_switch(iedge);

    /* For all "new" nodes in the main path */
    // inode is node index of previous node
    // NO_PREVIOUS tags a previously routed node

    while (rr_node_to_rt_node_[inode] == nullptr) { //Not connected to existing routing
        main_branch_visited.insert(inode);
        all_visited.insert(inode);

        linked_rt_edge = rt_edges_.alloc();
        linked_rt_edge->child = downstream_rt_node;

        // Also mark downstream_rt_node->inode as visited to prevent
        // add_non_configurable_to_route_tree from potentially adding
        // downstream_rt_node to the rt tree twice.
        all_visited.insert(downstream_rt_node->inode);
        linked_rt_edge->iswitch = iswitch;
        linked_rt_edge->next = nullptr;

        rt_node = rt_nodes_.alloc();
        downstream_rt_node->parent_node = rt_node;
        downstream_rt_node->parent_switch = iswitch;

        rt_node->u.child_list = linked_rt_edge;
        rt_node->inode = inode;

        rr_node_to_rt_node_[inode] = rt_node;

        if (node_inf_[inode].type() == IPIN) {
            rt_node->re_expand = false;
        } else {
            rt_node->re_expand = true;
        }

        downstream_rt_node = rt_node;
        iedge = route_inf[inode].prev_edge;
        inode = route_inf[inode].prev_node;
        iswitch = node_inf_[inode].edge_switch(iedge);
    }

    //Inode is now the branch point to the old routing; do not need
    //to alloc another node since the old routing has done so already
    rt_node = rr_node_to_rt_node_[inode];
    VTR_ASSERT_MSG(rt_node, "Previous routing branch should exist");

    linked_rt_edge = rt_edges_.alloc();
    linked_rt_edge->child = downstream_rt_node;
    linked_rt_edge->iswitch = iswitch;
    linked_rt_edge->next = rt_node->u.child_list; //Add to head
    rt_node->u.child_list = linked_rt_edge;

    downstream_rt_node->parent_node = rt_node;
    downstream_rt_node->parent_switch = iswitch;

    //Expand (recursively) each of the main-branch nodes adding any
    //non-configurably connected nodes
    for (int rr_node : main_branch_visited) {
        add_non_configurable_to_route_tree(rr_node, false, all_visited);
    }

    *sink_rt_node_ptr = sink_rt_node;
    return (downstream_rt_node);
}

template<typename RrNodeInf, typename RrNodeSetInf, typename SwitchInf>
t_rt_node* RouteTreeTiming<RrNodeInf, RrNodeSetInf, SwitchInf>::add_non_configurable_to_route_tree(const int rr_node, const bool reached_by_non_configurable_edge, std::unordered_set<int>& visited) {
    t_rt_node* rt_node = nullptr;

    if (!visited.count(rr_node) || !reached_by_non_configurable_edge) {
        visited.insert(rr_node);

        auto& device_ctx = g_vpr_ctx.device();

        rt_node = rr_node_to_rt_node_[rr_node];

        if (!reached_by_non_configurable_edge) { //An existing main branch node
            VTR_ASSERT(rt_node);
        } else {
            VTR_ASSERT(reached_by_non_configurable_edge);

            if (!rt_node) {
                rt_node = rt_nodes_.alloc();
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
            VTR_ASSERT(!device_ctx.rr_nodes[rr_node].edge_is_configurable(iedge));

            int to_rr_node = device_ctx.rr_nodes[rr_node].edge_sink_node(iedge);

            //Recurse
            t_rt_node* child_rt_node = add_non_configurable_to_route_tree(to_rr_node, true, visited);

            if (!child_rt_node) continue;

            int iswitch = device_ctx.rr_nodes[rr_node].edge_switch(iedge);

            //Create the edge
            t_linked_rt_edge* linked_rt_edge = rt_edges_.alloc();
            linked_rt_edge->child = child_rt_node;
            linked_rt_edge->iswitch = iswitch;

            //Add edge at head of parent linked list
            linked_rt_edge->next = rt_node->u.child_list;
            rt_node->u.child_list = linked_rt_edge;

            //Update child to parent ref
            child_rt_node->parent_node = rt_node;
            child_rt_node->parent_switch = iswitch;
        }
        rr_node_to_rt_node_[rr_node] = rt_node;
    }

    return rt_node;
}

template<typename RrNodeInf, typename RrNodeSetInf, typename SwitchInf>
t_rt_node*
RouteTreeTiming<RrNodeInf, RrNodeSetInf, SwitchInf>::update_unbuffered_ancestors_C_downstream(t_rt_node* start_of_new_path_rt_node) const {
    /* Updates the C_downstream values for the ancestors of the new path.  Once
     * a buffered switch is found amongst the ancestors, no more ancestors are
     * affected.  Returns the root of the "unbuffered subtree" whose Tdel
     * values are affected by the new path's addition.                          */

    t_rt_node *rt_node, *parent_rt_node;
    short iswitch;
    float C_downstream_addition;

    rt_node = start_of_new_path_rt_node;
    parent_rt_node = rt_node->parent_node;
    iswitch = rt_node->parent_switch;

    /* Now that a connection has been made between rt_node and its parent we must also consider
     * the potential effect of internal capacitance. We will first assume that parent is connected
     * by an unbuffered switch, so the ancestors downstream capacitance must be equal to the sum
     * of the child's downstream capacitance with the internal capacitance of the switch.*/

    C_downstream_addition = rt_node->C_downstream + switch_inf_[iswitch].Cinternal;

    /* Having set the value of C_downstream_addition, we must check whethere the parent switch
     * is a buffered or unbuffered switch with the if statement below. If the parent switch is
     * a buffered switch, then the parent node's downsteam capacitance is increased by the
     * value of the parent switch's internal capacitance in the if statement below.
     * Correspondingly, the ancestors' downstream capacitance will be updated by the same
     * value through the while loop. Otherwise, if the parent switch is unbuffered, then
     * the if statement will be ignored, and the parent and ancestral nodes' downstream
     * capacitance will be increased by the sum of the child's downstream capacitance with
     * the internal capacitance of the parent switch in the while loop/.*/

    if (parent_rt_node != nullptr && switch_inf_[iswitch].buffered() == true) {
        C_downstream_addition = switch_inf_[iswitch].Cinternal;
        rt_node = parent_rt_node;
        rt_node->C_downstream += C_downstream_addition;
        parent_rt_node = rt_node->parent_node;
        iswitch = rt_node->parent_switch;
    }

    while (parent_rt_node != nullptr && switch_inf_[iswitch].buffered() == false) {
        rt_node = parent_rt_node;
        rt_node->C_downstream += C_downstream_addition;
        parent_rt_node = rt_node->parent_node;
        iswitch = rt_node->parent_switch;
    }
    return (rt_node);
}

template<typename RrNodeInf, typename RrNodeSetInf, typename SwitchInf>
void RouteTreeTiming<RrNodeInf, RrNodeSetInf, SwitchInf>::load_route_tree_Tdel(t_rt_node* subtree_rt_root, float Tarrival) const {
    /* Updates the Tdel values of the subtree rooted at subtree_rt_root by
     * by calling itself recursively.  The C_downstream values of all the nodes
     * must be correct before this routine is called.  Tarrival is the time at
     * at which the signal arrives at this node's *input*.                      */

    int inode;
    short iswitch;
    t_rt_node* child_node;
    t_linked_rt_edge* linked_rt_edge;
    float Tdel, Tchild;

    inode = subtree_rt_root->inode;

    /* Assuming the downstream connections are, on average, connected halfway
     * along a wire segment's length.  See discussion in net_delay.c if you want
     * to change this.                                                           */

    Tdel = Tarrival + 0.5 * subtree_rt_root->C_downstream * node_inf_[inode].R();
    subtree_rt_root->Tdel = Tdel;

    /* Now expand the children of this node to load their Tdel values (depth-
     * first pre-order traversal).                                              */

    linked_rt_edge = subtree_rt_root->u.child_list;

    while (linked_rt_edge != nullptr) {
        iswitch = linked_rt_edge->iswitch;
        child_node = linked_rt_edge->child;

        Tchild = Tdel + switch_inf_[iswitch].R * child_node->C_downstream;
        Tchild += switch_inf_[iswitch].Tdel; /* Intrinsic switch delay. */
        load_route_tree_Tdel(child_node, Tchild);

        linked_rt_edge = linked_rt_edge->next;
    }
}

template<typename RrNodeInf, typename RrNodeSetInf, typename SwitchInf>
template<typename RouteInf>
void RouteTreeTiming<RrNodeInf, RrNodeSetInf, SwitchInf>::load_route_tree_rr_route_inf(RouteInf* route_inf_ptr, t_rt_node* root) const {
    /* Traverses down a route tree and updates rr_node_inf for all nodes
     * to reflect that these nodes have already been routed to 			 */

    VTR_ASSERT(root != nullptr);

    RouteInf& route_inf = *route_inf_ptr;

    t_linked_rt_edge* edge{root->u.child_list};

    for (;;) {
        int inode = root->inode;
        route_inf[inode].prev_node = NO_PREVIOUS;
        route_inf[inode].prev_edge = NO_PREVIOUS;
        // path cost should be unset
        VTR_ASSERT(std::isinf(route_inf[inode].path_cost));
        VTR_ASSERT(std::isinf(route_inf[inode].backward_path_cost));

        // reached a sink
        if (!edge) { return; }
        // branch point (sibling edges)
        else if (edge->next) {
            // recursively update for each of its sibling branches
            do {
                load_route_tree_rr_route_inf(route_inf_ptr, edge->child);
                edge = edge->next;
            } while (edge);
            return;
        }

        root = edge->child;
        edge = root->u.child_list;
    }
}

template<typename RrNodeInf, typename RrNodeSetInf, typename SwitchInf>
t_rt_node* RouteTreeTiming<RrNodeInf, RrNodeSetInf, SwitchInf>::init_route_tree_to_source_no_net(int inode) {
    /* Initializes the routing tree to just the net source, and returns the root
     * node of the rt_tree (which is just the net source).                       */

    t_rt_node* rt_root;

    rt_root = rt_nodes_.alloc();
    rt_root->u.child_list = nullptr;
    rt_root->parent_node = nullptr;
    rt_root->parent_switch = OPEN;
    rt_root->re_expand = true;
    rt_root->inode = inode;
    rt_root->C_downstream = node_inf_[inode].C();
    rt_root->R_upstream = node_inf_[inode].R();
    rt_root->Tdel = 0.5 * node_inf_[inode].R() * node_inf_[inode].C();
    rr_node_to_rt_node_[inode] = rt_root;

    return (rt_root);
}

template<typename RrNodeInf, typename RrNodeSetInf, typename SwitchInf>
void RouteTreeTiming<RrNodeInf, RrNodeSetInf, SwitchInf>::add_route_tree_to_rr_node_lookup(t_rt_node* node) {
    if (node) {
        VTR_ASSERT(rr_node_to_rt_node_[node->inode] == nullptr || rr_node_to_rt_node_[node->inode] == node);

        rr_node_to_rt_node_[node->inode] = node;

        for (auto edge = node->u.child_list; edge != nullptr; edge = edge->next) {
            add_route_tree_to_rr_node_lookup(edge->child);
        }
    }
}

template<typename RrNodeInf, typename RrNodeSetInf, typename SwitchInf>
t_rt_node* RouteTreeTiming<RrNodeInf, RrNodeSetInf, SwitchInf>::traceback_to_route_tree(t_trace* head, std::vector<int>* non_config_node_set_usage) {
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
        trace = traceback_to_route_tree_branch(trace, &rr_node_to_rt, non_config_node_set_usage);
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
template<typename RrNodeInf, typename RrNodeSetInf, typename SwitchInf>
t_trace* RouteTreeTiming<RrNodeInf, RrNodeSetInf, SwitchInf>::traceback_to_route_tree_branch(t_trace* trace,
                                                                                             std::map<int, t_rt_node*>* rr_node_to_rt,
                                                                                             std::vector<int>* non_config_node_set_usage) {
    t_trace* next = nullptr;

    if (trace) {
        t_rt_node* node = nullptr;

        int inode = trace->index;
        int iswitch = trace->iswitch;

        auto itr = rr_node_to_rt->find(trace->index);
        if (itr == rr_node_to_rt->end()) {
            //Create

            //Initialize route tree node
            node = rt_nodes_.alloc();
            node->inode = inode;
            node->u.child_list = nullptr;

            node->R_upstream = std::numeric_limits<float>::quiet_NaN();
            node->C_downstream = std::numeric_limits<float>::quiet_NaN();
            node->Tdel = std::numeric_limits<float>::quiet_NaN();

            auto node_type = node_inf_[inode].type();
            if (node_type == IPIN || node_type == SINK)
                node->re_expand = false;
            else
                node->re_expand = true;

            if (node_type == SINK) {
                // A non-configurable edge to a sink is also a usage of the
                // set.
                auto set_itr = node_set_inf_.find(inode);
                if (non_config_node_set_usage != nullptr && set_itr != node_set_inf_.end()) {
                    if (switch_inf_[iswitch].configurable()) {
                        (*non_config_node_set_usage)[set_itr->second] += 1;
                    }
                }
            }

            //Save
            rr_node_to_rt->insert(std::make_pair(inode, node));
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
            auto set_itr = node_set_inf_.find(inode);
            if (non_config_node_set_usage != nullptr && set_itr != node_set_inf_.end()) {
                if (switch_inf_[iswitch].configurable()) {
                    (*non_config_node_set_usage)[set_itr->second] += 1;
                }
            }

            //Recursively construct the remaining branch
            next = traceback_to_route_tree_branch(next, rr_node_to_rt, non_config_node_set_usage);

            //Get the created child
            itr = rr_node_to_rt->find(trace->next->index);
            VTR_ASSERT_MSG(itr != rr_node_to_rt->end(), "Child must exist");
            t_rt_node* child = itr->second;

            //Create the edge
            t_linked_rt_edge* edge = rt_edges_.alloc();
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

// Prune route tree
//
//  Note that non-configurable node will not be pruned unless the node is
//  being totally ripped up, or the node is congested.
template<typename RrNodeInf, typename RrNodeSetInf, typename SwitchInf>
template<typename RouteInf>
t_rt_node* RouteTreeTiming<RrNodeInf, RrNodeSetInf, SwitchInf>::prune_route_tree(const RouteInf& route_inf, t_rt_node* rt_root, CBRR& connections_inf) {
    return prune_route_tree(route_inf, rt_root, connections_inf, nullptr);
}

// Prune route tree
//
//  Note that non-configurable nodes will be pruned if
//  non_config_node_set_usage is provided.  prune_route_tree will update
//  non_config_node_set_usage after pruning.
template<typename RrNodeInf, typename RrNodeSetInf, typename SwitchInf>
template<typename RouteInf>
t_rt_node* RouteTreeTiming<RrNodeInf, RrNodeSetInf, SwitchInf>::prune_route_tree(const RouteInf& route_inf, t_rt_node* rt_root, CBRR& connections_inf, std::vector<int>* non_config_node_set_usage) {
    /* Prune a skeleton route tree of illegal branches - when there is at least 1 congested node on the path to a sink
     * This is the top level function to be called with the SOURCE node as root.
     * Returns true if the entire tree has been pruned.
     *
     * Note: does not update R_upstream/C_downstream
     */

    VTR_ASSERT(rt_root);

    VTR_ASSERT_MSG(node_inf_[rt_root->inode].type() == SOURCE, "Root of route tree must be SOURCE");

    VTR_ASSERT_MSG(route_inf[rt_root->inode].occ() <= node_inf_[rt_root->inode].capacity(),
                   "Route tree root/SOURCE should never be congested");

    return prune_route_tree_recurr(route_inf, rt_root, connections_inf, false, non_config_node_set_usage);
}

//Prunes a route tree (recursively) based on congestion and the 'force_prune' argument
//
//Returns true if the current node was pruned
template<typename RrNodeInf, typename RrNodeSetInf, typename SwitchInf>
template<typename RouteInf>
t_rt_node* RouteTreeTiming<RrNodeInf, RrNodeSetInf, SwitchInf>::prune_route_tree_recurr(const RouteInf& route_inf, t_rt_node* node, CBRR& connections_inf, bool force_prune, std::vector<int>* non_config_node_set_usage) {
    //Recursively traverse the route tree rooted at node and remove any congested
    //sub-trees

    VTR_ASSERT(node);

    bool congested = (route_inf[node->inode].occ() > node_inf_[node->inode].capacity());
    int node_set = -1;
    auto itr = node_set_inf_.find(node->inode);
    if (itr != node_set_inf_.end()) {
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
        t_rt_node* child = prune_route_tree_recurr(route_inf, edge->child,
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
            if (non_config_node_set_usage != nullptr && node_set != -1 && switch_inf_[old_edge->iswitch].configurable()) {
                (*non_config_node_set_usage)[node_set] -= 1;
                VTR_ASSERT((*non_config_node_set_usage)[node_set] >= 0);
            }

            rt_edges_.free(old_edge);

            //Note prev_edge is unchanged

        } else { //Child not pruned
            all_children_pruned = false;

            //Edge not removed
            prev_edge = edge;
            edge = edge->next;
        }
    }

    if (node_inf_[node->inode].type() == SINK) {
        if (!force_prune) {
            //Valid path to sink

            //Record sink as reachable
            connections_inf.reached_rt_sink(node);

            return node; //Not pruned
        } else {
            VTR_ASSERT(force_prune);

            //Record as not reached
            connections_inf.toreach_rr_sink(node->inode);

            rt_nodes_.free(node);
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
            reached_non_configurably = !switch_inf_[node->parent_switch].configurable();

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
            rt_nodes_.free(node);
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
        if (non_config_node_set_usage != nullptr && node_set != -1 && switch_inf_[node->parent_switch].configurable() && (*non_config_node_set_usage)[node_set] == 0) {
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
            return prune_route_tree_recurr(route_inf, node, connections_inf,
                                           /*force_prune=*/false, non_config_node_set_usage);
        }

        //An unpruned intermediate node
        VTR_ASSERT(!force_prune);

        return node; //Not pruned
    }
}

template<typename RrNodeInf, typename RrNodeSetInf, typename SwitchInf>
float RouteTreeTiming<RrNodeInf, RrNodeSetInf, SwitchInf>::load_new_subtree_C_downstream(t_rt_node* rt_node) const {
    float C_downstream = 0.;

    if (rt_node) {
        C_downstream += node_inf_[rt_node->inode].C();
        for (t_linked_rt_edge* edge = rt_node->u.child_list; edge != nullptr; edge = edge->next) {
            /*Similar to net_delay.cpp, this for loop traverses a rc subtree, whose edges represent enabled switches.
             * When switches such as multiplexers and tristate buffers are enabled, their fanout
             * produces an "internal capacitance". We account for this internal capacitance of the
             * switch by adding it to the total capacitance of the node.*/

            C_downstream += switch_inf_[edge->iswitch].Cinternal;

            float C_downstream_child = load_new_subtree_C_downstream(edge->child);

            if (!switch_inf_[edge->iswitch].buffered()) {
                C_downstream += C_downstream_child;
            }
        }

        rt_node->C_downstream = C_downstream;
    }

    return C_downstream;
}

template<typename RrNodeInf, typename RrNodeSetInf, typename SwitchInf>
void RouteTreeTiming<RrNodeInf, RrNodeSetInf, SwitchInf>::load_new_subtree_R_upstream(t_rt_node* rt_node) const {
    /* Sets the R_upstream values of all the nodes in the new path to the
     * correct value by traversing down to SINK from the start of the new path. */

    if (!rt_node) {
        return;
    }

    t_rt_node* parent_rt_node = rt_node->parent_node;
    int inode = rt_node->inode;

    //Calculate upstream resistance
    float R_upstream = 0.;
    if (parent_rt_node) {
        int iswitch = rt_node->parent_switch;
        bool switch_buffered = switch_inf_[iswitch].buffered();

        if (!switch_buffered) {
            R_upstream += parent_rt_node->R_upstream; //Parent upstream R
        }
        R_upstream += switch_inf_[iswitch].R; //Parent switch R
    }
    R_upstream += node_inf_[inode].R(); //Current node R

    rt_node->R_upstream = R_upstream;

    //Update children
    for (t_linked_rt_edge* edge = rt_node->u.child_list; edge != nullptr; edge = edge->next) {
        load_new_subtree_R_upstream(edge->child); //Recurse
    }
}
