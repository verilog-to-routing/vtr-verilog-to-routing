#include "traces.h"
#include "rr_node.h"

void Traces::save_routing(vtr::vector<ClusterNetId, t_trace*>& best_routing,
                          const t_clb_opins_used& clb_opins_used_locally,
                          t_clb_opins_used& saved_clb_opins_used_locally) {
    /* This routing frees any routing currently held in best routing,       *
     * then copies over the current routing (held in route_ctx.trace), and       *
     * finally sets route_ctx.trace_head and route_ctx.trace_tail to all NULLs so that the      *
     * connection to the saved routing is broken.  This is necessary so     *
     * that the next iteration of the router does not free the saved        *
     * routing elements.  Also saves any data about locally used clb_opins, *
     * since this is also part of the routing.                              */

    t_trace *tptr, *tempptr;
    VTR_ASSERT(best_routing.size() == trace_.size());

    for (auto net_id : trace_.keys()) {
        /* Free any previously saved routing.  It is no longer best. */
        tptr = best_routing[net_id];
        while (tptr != nullptr) {
            tempptr = tptr->next;
            traces_.free(tptr);
            tptr = tempptr;
        }

        /* Save a pointer to the current routing in best_routing. */
        best_routing[net_id] = trace_[net_id].head;

        /* Set the current (working) routing to NULL so the current trace       *
         * elements won't be reused by the memory allocator.                    */

        trace_[net_id].head = nullptr;
        trace_[net_id].tail = nullptr;
        trace_nodes_[net_id].clear();
    }

    /* Save which OPINs are locally used.                           */
    saved_clb_opins_used_locally = clb_opins_used_locally;
}

/* Deallocates any current routing in route_ctx.trace_head, and replaces it with    *
 * the routing in best_routing.  Best_routing is set to NULL to show that *
 * it no longer points to a valid routing.  NOTE:  route_ctx.trace_tail is not      *
 * restored -- it is set to all NULLs since it is only used in            *
 * update_traceback.  If you need route_ctx.trace_tail restored, modify this        *
 * routine.  Also restores the locally used opin data.                    */
void Traces::restore_routing(vtr::vector<ClusterNetId, t_trace*>& best_routing,
                             t_clb_opins_used& clb_opins_used_locally,
                             const t_clb_opins_used& saved_clb_opins_used_locally) {
    VTR_ASSERT(best_routing.size() == trace_.size());

    for (auto net_id : trace_.keys()) {
        /* Free any current routing. */
        pathfinder_update_path_cost(trace_[net_id].head, -1, 0.f);
        free_traceback(net_id);

        /* Set the current routing to the saved one. */
        trace_[net_id].head = best_routing[net_id];
        pathfinder_update_path_cost(trace_[net_id].head, 1, 0.f);
        best_routing[net_id] = nullptr; /* No stored routing. */
    }

    /* Restore which OPINs are locally used.                           */
    clb_opins_used_locally = saved_clb_opins_used_locally;
}

bool Traces::validate_trace_nodes(t_trace* head, const std::unordered_set<int>& trace_nodes) {
    //Verifies that all nodes in the traceback 'head' are conatined in 'trace_nodes'

    if (!head) {
        return true;
    }

    std::vector<int> missing_from_trace_nodes;
    for (t_trace* tptr = head; tptr != nullptr; tptr = tptr->next) {
        if (!trace_nodes.count(tptr->index)) {
            missing_from_trace_nodes.push_back(tptr->index);
        }
    }

    if (!missing_from_trace_nodes.empty()) {
        std::string msg = vtr::string_fmt(
            "The following %zu nodes were found in traceback"
            " but were missing from trace_nodes: %s\n",
            missing_from_trace_nodes.size(),
            vtr::join(missing_from_trace_nodes, ", ").c_str());

        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, msg.c_str());
        return false;
    }

    return true;
}

const t_trace*
Traces::update_traceback(
    const std::vector<t_rr_node>& rr_nodes,
    const std::vector<t_rr_node_route_inf>& rr_node_route_inf,
    t_heap* hptr,
    ClusterNetId net_id) {
    /* This routine adds the most recently finished wire segment to the         *
     * traceback linked list.  The first connection starts with the net SOURCE  *
     * and begins at the structure pointed to by route_ctx.trace[net_id].head. Each         *
     * connection ends with a SINK.  After each SINK, the next connection       *
     * begins (if the net has more than 2 pins).  The first element after the   *
     * SINK gives the routing node on a previous piece of the routing, which is *
     * the link from the existing net to this new piece of the net.             *
     * In each traceback I start at the end of a path and trace back through    *
     * its predecessors to the beginning.  I have stored information on the     *
     * predecesser of each node to make traceback easy -- this sacrificies some *
     * memory for easier code maintenance.  This routine returns a pointer to   *
     * the first "new" node in the traceback (node not previously in trace).    */
    auto& trace_nodes = trace_nodes_[net_id];

    VTR_ASSERT_SAFE(validate_trace_nodes(trace_[net_id].head, trace_nodes));

    t_trace_branch branch = traceback_branch(rr_nodes, rr_node_route_inf, hptr->index, trace_nodes);

    VTR_ASSERT_SAFE(validate_trace_nodes(branch.head, trace_nodes));

    t_trace* ret_ptr = nullptr;
    if (trace_[net_id].tail != nullptr) {
        trace_[net_id].tail->next = branch.head; /* Traceback ends with tptr */
        ret_ptr = branch.head->next;             /* First new segment.       */
    } else {                                     /* This was the first "chunk" of the net's routing */
        trace_[net_id].head = branch.head;
        ret_ptr = branch.head; /* Whole traceback is new. */
    }

    trace_[net_id].tail = branch.tail;
    return (ret_ptr);
}

//Traces back a new routing branch starting from the specified 'node' and working backwards to any existing routing.
//Returns the new branch, and also updates trace_nodes for any new nodes which are included in the branches traceback.
Traces::t_trace_branch Traces::traceback_branch(
    const std::vector<t_rr_node>& rr_nodes,
    const std::vector<t_rr_node_route_inf>& rr_node_route_inf,
    int node,
    std::unordered_set<int>& trace_nodes) {
    auto rr_type = rr_nodes[node].type();
    if (rr_type != SINK) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "in traceback_branch: Expected type = SINK (%d).\n");
    }

    //We construct the main traceback by walking from the given node back to the source,
    //according to the previous edges/nodes recorded in rr_node_route_inf by the router.
    t_trace* branch_head = traces_.alloc();
    t_trace* branch_tail = branch_head;
    branch_head->index = node;
    branch_head->iswitch = OPEN;
    branch_head->next = nullptr;

    trace_nodes.insert(node);

    std::vector<int> new_nodes_added_to_traceback = {node};

    auto iedge = rr_node_route_inf[node].prev_edge;
    int inode = rr_node_route_inf[node].prev_node;

    while (inode != NO_PREVIOUS) {
        //Add the current node to the head of traceback
        t_trace* prev_ptr = traces_.alloc();
        prev_ptr->index = inode;
        prev_ptr->iswitch = rr_nodes[inode].edge_switch(iedge);
        prev_ptr->next = branch_head;
        branch_head = prev_ptr;

        if (trace_nodes.count(inode)) {
            break; //Connected to existing routing
        }

        trace_nodes.insert(inode); //Record this node as visited
        new_nodes_added_to_traceback.push_back(inode);

        iedge = rr_node_route_inf[inode].prev_edge;
        inode = rr_node_route_inf[inode].prev_node;
    }

    //We next re-expand all the main-branch nodes to add any non-configurably connected side branches
    // We are careful to do this *after* the main branch is constructed to ensure nodes which are both
    // non-configurably connected *and* part of the main branch are only added to the traceback once.
    for (int new_node : new_nodes_added_to_traceback) {
        //Expand each main branch node
        std::tie(branch_head, branch_tail) = add_trace_non_configurable(rr_nodes, branch_head, branch_tail, new_node, trace_nodes);
    }

    return {branch_head, branch_tail};
}

//Traces any non-configurable subtrees from branch_head, returning the new branch_head and updating trace_nodes
//
//This effectively does a depth-first traversal
std::pair<t_trace*, t_trace*> Traces::add_trace_non_configurable(
    const std::vector<t_rr_node>& rr_nodes,
    t_trace* head,
    t_trace* tail,
    int node,
    std::unordered_set<int>& trace_nodes) {
    //Trace any non-configurable subtrees
    t_trace* subtree_head = nullptr;
    t_trace* subtree_tail = nullptr;
    std::tie(subtree_head, subtree_tail) = add_trace_non_configurable_recurr(rr_nodes, node, trace_nodes, 0);

    //Add any non-empty subtree to tail of traceback
    if (subtree_head && subtree_tail) {
        if (!head) { //First subtree becomes head
            head = subtree_head;
        } else { //Later subtrees added to tail
            VTR_ASSERT(tail);
            tail->next = subtree_head;
        }

        tail = subtree_tail;
    } else {
        VTR_ASSERT(subtree_head == nullptr && subtree_tail == nullptr);
    }

    return {head, tail};
}

//Recursive helper function for add_trace_non_configurable()
std::pair<t_trace*, t_trace*> Traces::add_trace_non_configurable_recurr(
    const std::vector<t_rr_node>& rr_nodes,
    int node,
    std::unordered_set<int>& trace_nodes,
    int depth) {
    t_trace* head = nullptr;
    t_trace* tail = nullptr;

    //Record the non-configurable out-going edges
    std::vector<t_edge_size> unvisited_non_configurable_edges;
    for (auto iedge : rr_nodes[node].non_configurable_edges()) {
        VTR_ASSERT_SAFE(!rr_nodes[node].edge_is_configurable(iedge));

        int to_node = rr_nodes[node].edge_sink_node(iedge);

        if (!trace_nodes.count(to_node)) {
            unvisited_non_configurable_edges.push_back(iedge);
        }
    }

    if (unvisited_non_configurable_edges.size() == 0) {
        //Base case: leaf node with no non-configurable edges
        if (depth > 0) { //Arrived via non-configurable edge
            VTR_ASSERT(!trace_nodes.count(node));
            head = traces_.alloc();
            head->index = node;
            head->iswitch = -1;
            head->next = nullptr;
            tail = head;

            trace_nodes.insert(node);
        }

    } else {
        //Recursive case: intermediate node with non-configurable edges
        for (auto iedge : unvisited_non_configurable_edges) {
            int to_node = rr_nodes[node].edge_sink_node(iedge);
            int iswitch = rr_nodes[node].edge_switch(iedge);

            VTR_ASSERT(!trace_nodes.count(to_node));
            trace_nodes.insert(node);

            //Recurse
            t_trace* subtree_head = nullptr;
            t_trace* subtree_tail = nullptr;
            std::tie(subtree_head, subtree_tail) = add_trace_non_configurable_recurr(rr_nodes, to_node, trace_nodes, depth + 1);

            if (subtree_head && subtree_tail) {
                //Add the non-empty sub-tree

                //Duplicate the original head as the new tail (for the new branch)
                t_trace* intermediate_head = traces_.alloc();
                intermediate_head->index = node;
                intermediate_head->iswitch = iswitch;
                intermediate_head->next = nullptr;

                intermediate_head->next = subtree_head;

                if (!head) { //First subtree becomes head
                    head = intermediate_head;
                } else { //Later subtrees added to tail
                    VTR_ASSERT(tail);
                    tail->next = intermediate_head;
                }

                tail = subtree_tail;
            } else {
                VTR_ASSERT(subtree_head == nullptr && subtree_tail == nullptr);
            }
        }
    }

    return {head, tail};
}

t_trace*
TraceAllocator::alloc() {
    t_trace* temp_ptr;

    if (trace_free_head_ == nullptr) { /* No elements on the free list */
        trace_free_head_ = (t_trace*)vtr::chunk_malloc(sizeof(t_trace), &trace_ch_);
        trace_free_head_->next = nullptr;
    }
    temp_ptr = trace_free_head_;
    trace_free_head_ = trace_free_head_->next;
    num_trace_allocated_++;
    return (temp_ptr);
}

void TraceAllocator::free(t_trace* tptr) {
    /* Puts the traceback structure pointed to by tptr on the free list. */

    tptr->next = trace_free_head_;
    trace_free_head_ = tptr;
    num_trace_allocated_--;
}

void Traces::free_traceback(ClusterNetId net_id) {
    /* Puts the entire traceback (old routing) for this net on the free list *
     * and sets the route_ctx.trace_head pointers etc. for the net to NULL.            */

    if (trace_.empty()) {
        return;
    }

    if (trace_[net_id].head == nullptr) {
        return;
    }

    free_traceback(trace_[net_id].head);

    trace_[net_id].head = nullptr;
    trace_[net_id].tail = nullptr;
    trace_nodes_[net_id].clear();
}

void Traces::free_traceback(t_trace* tptr) {
    while (tptr != nullptr) {
        t_trace* tempptr = tptr->next;
        traces_.free(tptr);
        tptr = tempptr;
    }
}

void Traces::free_all() {
    if (trace_.empty()) {
        return;
    }

    for (auto net_id : trace_.keys()) {
        free_traceback(net_id);

        if (trace_[net_id].head) {
            free(trace_[net_id].head);
            free(trace_[net_id].tail);
        }
        trace_[net_id].head = nullptr;
        trace_[net_id].tail = nullptr;
    }
}

void Traces::assert_trace_empty(ClusterNetId net_id) const {
    VTR_ASSERT(trace_[net_id].head == nullptr);
    VTR_ASSERT(trace_[net_id].tail == nullptr);
    VTR_ASSERT(trace_nodes_[net_id].empty());
}

void Traces::set_traceback(ClusterNetId inet, const std::vector<traceback_element>& traceback) {
    assert_trace_empty(inet);

    t_trace* tptr = nullptr;
    auto& trace_nodes = trace_nodes_[inet];
    for (size_t i = 0; i < traceback.size(); ++i) {
        trace_nodes.insert(traceback[i].inode);

        if (i == 0) {
            trace_[inet].head = traces_.alloc();
            trace_[inet].head->index = traceback[i].inode;
            trace_[inet].head->iswitch = traceback[i].switch_id;
            trace_[inet].head->next = nullptr;
            tptr = trace_[inet].head;
        } else {
            tptr->next = traces_.alloc();
            tptr = tptr->next;
            tptr->index = traceback[i].inode;
            tptr->iswitch = traceback[i].switch_id;
            tptr->next = nullptr;
        }
    }

    trace_[inet].tail = tptr;
}

void Traces::free_chunk_memory_trace() {
    traces_.free_chunk_memory_trace();
}

void TraceAllocator::free_chunk_memory_trace() {
    if (trace_ch_.chunk_ptr_head != nullptr) {
        free_chunk_memory(&trace_ch_);
        trace_ch_.chunk_ptr_head = nullptr;
        trace_free_head_ = nullptr;
    }
}

const t_trace* Traces::traceback_from_route_tree(
    const std::vector<t_rr_node>& rr_nodes,
    ClusterNetId inet,
    const t_rt_node* root,
    int num_routed_sinks) {
    /* Creates the traceback for net inet from the route tree rooted at root
     * properly sets route_ctx.trace_head and route_ctx.trace_tail for this net
     * returns the trace head for inet 					 					 */

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
        if (rr_nodes[trace->index].type() == SINK) {
            num_trace_sinks += 1;
        }
    }
    VTR_ASSERT(num_routed_sinks == num_trace_sinks);

    trace_[inet].tail = tail;
    trace_[inet].head = head;
    trace_nodes_[inet] = nodes;

    return head;
}

std::pair<t_trace*, t_trace*> Traces::traceback_from_route_tree_recurr(t_trace* head, t_trace* tail, const t_rt_node* node) {
    if (node) {
        if (node->u.child_list) {
            //Recursively add children
            for (t_linked_rt_edge* edge = node->u.child_list; edge != nullptr; edge = edge->next) {
                t_trace* curr = traces_.alloc();
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
            t_trace* curr = traces_.alloc();
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

void Traces::save_traces(vtr::vector<ClusterNetId, std::vector<traceback_element>>* routing) const {
    routing->resize(trace_.size());
    for (auto inet : trace_.keys()) {
        std::vector<traceback_element>& trace_out = routing->at(inet);
        trace_out.resize(0);
        for (const t_trace* trace = get_trace_head(inet); trace != nullptr; trace = trace->next) {
            traceback_element elem;
            elem.inode = trace->index;
            elem.switch_id = trace->iswitch;
            trace_out.push_back(elem);
        }
    }
}

void Traces::restore_traces(const vtr::vector<ClusterNetId, std::vector<traceback_element>>& routing, float pres_fac) {
    for (auto inet : trace_.keys()) {
        pathfinder_update_path_cost(get_trace_head(inet), -1, pres_fac);
        free_traceback(inet);
        set_traceback(inet, routing[inet]);
        pathfinder_update_path_cost(get_trace_head(inet), 1, pres_fac);
    }
}

TraceAllocator::~TraceAllocator() {
    free_chunk_memory_trace();
}
