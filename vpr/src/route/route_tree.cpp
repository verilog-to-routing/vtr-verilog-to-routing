#include "route_tree.h"
#include "globals.h"
#include "netlist_fwd.h"
#include "route_timing.h"
#include "rr_graph_fwd.h"
#include "vtr_math.h"

/* Construct a new RouteTreeNode.
 * Doesn't add the node to parent's child_nodes! (see add_child) */
RouteTreeNode::RouteTreeNode(RRNodeId _inode, RRSwitchId _parent_switch, RouteTreeNode* parent)
    : inode(_inode)
    , parent_switch(_parent_switch)
    , _parent(parent) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    re_expand = true;
    net_pin_index = OPEN;
    C_downstream = rr_graph.node_C(_inode);
    R_upstream = rr_graph.node_R(_inode);
    Tdel = 0.5 * R_upstream * C_downstream;

    _next = nullptr;
    _prev = nullptr;
    _subtree_end = nullptr;
    _is_leaf = true;
}

/** Print information about this subtree to stdout. */
void RouteTreeNode::print(void) const {
    print_x(0);
}

/** Helper function for print. */
void RouteTreeNode::print_x(int depth) const {
    std::string indent;
    for (int i = 0; i < depth; ++i) {
        indent += "  ";
    }

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    VTR_LOG("%srt_node: %d (%s) \t ipin: %d \t R: %g \t C: %g \t delay: %g \t",
            indent.c_str(),
            inode,
            rr_graph.node_type_string(inode),
            net_pin_index,
            R_upstream,
            C_downstream,
            Tdel);

    if (_parent) {
        VTR_LOG("parent: %d \t parent_switch: %d", _parent->inode, parent_switch);
        bool parent_edge_configurable = rr_graph.rr_switch_inf(parent_switch).configurable();
        if (!parent_edge_configurable) {
            VTR_LOG("*");
        }
    }

    auto& route_ctx = g_vpr_ctx.routing();
    if (route_ctx.rr_node_route_inf[inode].occ() > rr_graph.node_capacity(inode)) {
        VTR_LOG(" x");
    }

    VTR_LOG("\n");

    for (auto& child : child_nodes()) {
        child.print_x(depth + 1);
    }
}

/* Construct a top-level route tree. */
RouteTree::RouteTree(RRNodeId _inode) {
    _root = new RouteTreeNode(_inode, RRSwitchId::INVALID(), nullptr);
    _net_id = ParentNetId::INVALID();
    _rr_node_to_rt_node[_inode] = _root;
}

RouteTree::RouteTree(ParentNetId _inet) {
    auto& route_ctx = g_vpr_ctx.routing();

    RRNodeId inode = RRNodeId(route_ctx.net_rr_terminals[_inet][0]);
    _root = new RouteTreeNode(inode, RRSwitchId::INVALID(), nullptr);
    _net_id = _inet;
    _rr_node_to_rt_node[inode] = _root;

    _num_sinks = route_ctx.net_rr_terminals[_inet].size() - 1;
    _isink_to_rt_node.resize(_num_sinks);     /* 0-indexed */
    _is_isink_reached.resize(_num_sinks + 1); /* 1-indexed */
}

/** Make a copy of rhs and return it.
 * Traverse it as a tree so we can keep parent & child ptrs valid. */
RouteTreeNode* RouteTree::copy_tree(const RouteTreeNode* rhs) {
    RouteTreeNode* root = new RouteTreeNode(rhs->inode, RRSwitchId::INVALID(), nullptr);
    _rr_node_to_rt_node[root->inode] = root;
    copy_tree_x(root, *rhs);
    return root;
}

/* Helper for copy_list: copy child nodes of rhs into lhs */
void RouteTree::copy_tree_x(RouteTreeNode* lhs, const RouteTreeNode& rhs) {
    for (auto& rchild : rhs.child_nodes()) {
        RouteTreeNode* child = new RouteTreeNode(rchild);
        child->_is_leaf = true;
        add_node(lhs, child);
        copy_tree_x(child, rchild);
    }
}

/* Copy constructor */
RouteTree::RouteTree(const RouteTree& rhs) {
    _isink_to_rt_node.resize(rhs._isink_to_rt_node.size());
    _net_id = rhs._net_id;
    _root = copy_tree(rhs._root);
    _is_isink_reached = rhs._is_isink_reached;
    _num_sinks = rhs._num_sinks;
}

/* Move constructor:
 * Take over rhs' linked list & set it to null so it doesn't get freed.
 * Refs should stay valid after this?
 * I don't think there's a user crazy enough to move around route trees
 * from multiple threads, but better safe than sorry */
RouteTree::RouteTree(RouteTree&& rhs) {
    std::unique_lock<std::mutex> rhs_write_lock(rhs._write_mutex);
    _root = rhs._root;
    _net_id = rhs._net_id;
    rhs._root = nullptr;
    _rr_node_to_rt_node = std::move(rhs._rr_node_to_rt_node);
    _isink_to_rt_node = std::move(rhs._isink_to_rt_node);
    _is_isink_reached = std::move(rhs._is_isink_reached);
    _num_sinks = rhs._num_sinks;
}

/* Copy assignment: free list, clear lookup, reload list. */
RouteTree& RouteTree::operator=(const RouteTree& rhs) {
    if (this == &rhs)
        return *this;
    std::unique_lock<std::mutex> write_lock(_write_mutex);
    free_list(_root);
    _rr_node_to_rt_node.clear();
    _isink_to_rt_node.clear();
    _isink_to_rt_node.resize(rhs._isink_to_rt_node.size());
    _net_id = rhs._net_id;
    _root = copy_tree(rhs._root);
    _is_isink_reached = rhs._is_isink_reached;
    _num_sinks = rhs._num_sinks;
    return *this;
}

/* Move assignment:
 * Free my list, take over rhs' linked list & set it to null so it doesn't get freed.
 * Also ~steal~ acquire ownership of node lookup from rhs.
 * Refs should stay valid after this?
 * I don't think there's a user crazy enough to move around route trees
 * from multiple threads, but better safe than sorry */
RouteTree& RouteTree::operator=(RouteTree&& rhs) {
    if (this == &rhs)
        return *this;
    /* See https://stackoverflow.com/a/29988626 */
    std::unique_lock<std::mutex> write_lock(_write_mutex, std::defer_lock);
    std::unique_lock<std::mutex> rhs_write_lock(rhs._write_mutex, std::defer_lock);
    std::lock(write_lock, rhs_write_lock);
    free_list(_root);
    _root = rhs._root;
    _net_id = rhs._net_id;
    rhs._root = nullptr;
    _rr_node_to_rt_node = std::move(rhs._rr_node_to_rt_node);
    _isink_to_rt_node = std::move(rhs._isink_to_rt_node);
    _is_isink_reached = std::move(rhs._is_isink_reached);
    _num_sinks = rhs._num_sinks;
    return *this;
}

/** Reload timing values (R_upstream, C_downstream, Tdel).
 * Can take a RouteTreeNode& to do an incremental update.
 * Note that update_from_heap already calls this. */
void RouteTree::reload_timing(vtr::optional<RouteTreeNode&> from_node) {
    std::unique_lock<std::mutex> write_lock(_write_mutex);
    reload_timing_unlocked(from_node);
}

void RouteTree::reload_timing_unlocked(vtr::optional<RouteTreeNode&> from_node) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    if (!from_node)
        from_node = *_root;

    // Propagate R_upstream down into the new subtree
    load_new_subtree_R_upstream(*from_node);

    // Propagate C_downstream up from new subtree sinks to subtree root
    load_new_subtree_C_downstream(*from_node);

    // Propagate C_downstream up from the subtree root
    RouteTreeNode& unbuffered_subtree_rt_root = update_unbuffered_ancestors_C_downstream(*from_node);

    float Tdel_start = 0;

    if (unbuffered_subtree_rt_root._parent) {
        RouteTreeNode& subtree_parent_rt_node = *unbuffered_subtree_rt_root._parent;
        Tdel_start = subtree_parent_rt_node.Tdel;
        RRSwitchId iswitch = unbuffered_subtree_rt_root.parent_switch;
        /* TODO Just a note (no action needed for this PR): In future, we need to consider APIs that returns
         * the Tdel for a routing trace in RRGraphView.*/
        Tdel_start += rr_graph.rr_switch_inf(iswitch).R * unbuffered_subtree_rt_root.C_downstream;
        Tdel_start += rr_graph.rr_switch_inf(iswitch).Tdel;
    }

    load_route_tree_Tdel(unbuffered_subtree_rt_root, Tdel_start);
}

/** Sets the R_upstream values of all the nodes in the new path to the
 * correct value by traversing down to SINK from from_node. */
void RouteTree::load_new_subtree_R_upstream(RouteTreeNode& rt_node) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    RouteTreeNode* parent_rt_node = rt_node._parent;
    RRNodeId inode = rt_node.inode;

    //Calculate upstream resistance
    float R_upstream = 0.;
    if (parent_rt_node) {
        RRSwitchId iswitch = rt_node.parent_switch;
        bool switch_buffered = rr_graph.rr_switch_inf(iswitch).buffered();

        if (!switch_buffered) {
            R_upstream += parent_rt_node->R_upstream; //Parent upstream R
        }
        R_upstream += rr_graph.rr_switch_inf(iswitch).R; //Parent switch R
    }
    R_upstream += rr_graph.node_R(inode); //Current node R

    rt_node.R_upstream = R_upstream;

    //Update children
    for (RouteTreeNode& child : rt_node._child_nodes()) {
        load_new_subtree_R_upstream(child);
    }
}

/** Sets the R_upstream values of all the nodes in the new path to the
 * correct value by traversing down to SINK from from_node. */
float RouteTree::load_new_subtree_C_downstream(RouteTreeNode& from_node) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    float C_downstream = 0.;
    C_downstream += rr_graph.node_C(from_node.inode);
    for (RouteTreeNode& child : from_node._child_nodes()) {
        /* When switches such as multiplexers and tristate buffers are enabled, their fanout
         * produces an "internal capacitance". We account for this internal capacitance of the
         * switch by adding it to the total capacitance of the node. */
        C_downstream += rr_graph.rr_switch_inf(child.parent_switch).Cinternal;
        float C_downstream_child = load_new_subtree_C_downstream(child);
        if (!rr_graph.rr_switch_inf(child.parent_switch).buffered()) {
            C_downstream += C_downstream_child;
        }
    }

    from_node.C_downstream = C_downstream;
    return C_downstream;
}

/** Update the C_downstream values for the ancestors of from_node. Once
 * a buffered switch is found amongst the ancestors, no more ancestors are
 * affected. Returns the root of the "unbuffered subtree" whose Tdel
 * values are affected by the new path's addition. */
RouteTreeNode&
RouteTree::update_unbuffered_ancestors_C_downstream(RouteTreeNode& from_node) {
    if (!from_node.parent())
        return from_node;

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    RRSwitchId iswitch = from_node.parent_switch;

    /* Now that a connection has been made between rt_node and its parent we must also consider
     * the potential effect of internal capacitance. We will first assume that parent is connected
     * by an unbuffered switch, so the ancestors downstream capacitance must be equal to the sum
     * of the child's downstream capacitance with the internal capacitance of the switch.*/

    float C_downstream_addition = from_node.C_downstream + rr_graph.rr_switch_inf(iswitch).Cinternal;

    /* Having set the value of C_downstream_addition, we must check whether the parent switch
     * is a buffered or unbuffered switch with the if statement below. If the parent switch is
     * a buffered switch, then the parent node's downsteam capacitance is increased by the
     * value of the parent switch's internal capacitance in the if statement below.
     * Correspondingly, the ancestors' downstream capacitance will be updated by the same
     * value through the while loop. Otherwise, if the parent switch is unbuffered, then
     * the if statement will be ignored, and the parent and ancestral nodes' downstream
     * capacitance will be increased by the sum of the child's downstream capacitance with
     * the internal capacitance of the parent switch in the while loop.*/

    RouteTreeNode* last_node = std::addressof(from_node);
    RouteTreeNode* parent_rt_node = from_node._parent;

    if (rr_graph.rr_switch_inf(iswitch).buffered() == true) {
        C_downstream_addition = rr_graph.rr_switch_inf(iswitch).Cinternal;
        last_node = parent_rt_node;
        last_node->C_downstream += C_downstream_addition;
        parent_rt_node = last_node->_parent;
        iswitch = last_node->parent_switch;
    }

    while (parent_rt_node && rr_graph.rr_switch_inf(iswitch).buffered() == false) {
        last_node = parent_rt_node;
        last_node->C_downstream += C_downstream_addition;
        parent_rt_node = last_node->_parent;
        iswitch = last_node->parent_switch;
    }

    VTR_ASSERT(last_node);
    return *last_node;
}

/** Updates the Tdel values of the subtree rooted at rt_node by
 * by calling itself recursively. The C_downstream values of all the nodes
 * must be correct before this routine is called. Tarrival is the time at
 * at which the signal arrives at this node's *input*. */
void RouteTree::load_route_tree_Tdel(RouteTreeNode& from_node, float Tarrival) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    RRNodeId inode = from_node.inode;
    float Tdel, Tchild;

    /* Assuming the downstream connections are, on average, connected halfway
     * along a wire segment's length.  See discussion in net_delay.cpp if you want
     * to change this.                                                           */
    Tdel = Tarrival + 0.5 * from_node.C_downstream * rr_graph.node_R(inode);
    from_node.Tdel = Tdel;

    /* Now expand the children of this node to load their Tdel values */
    for (RouteTreeNode& child : from_node._child_nodes()) {
        RRSwitchId iswitch = child.parent_switch;

        Tchild = Tdel + rr_graph.rr_switch_inf(iswitch).R * child.C_downstream;
        Tchild += rr_graph.rr_switch_inf(iswitch).Tdel; /* Intrinsic switch delay. */
        load_route_tree_Tdel(child, Tchild);
    }
}

vtr::optional<const RouteTreeNode&> RouteTree::find_by_rr_id(RRNodeId rr_node) const {
    auto it = _rr_node_to_rt_node.find(rr_node);
    if (it != _rr_node_to_rt_node.end()) {
        return *it->second;
    }
    return vtr::nullopt;
}

/** Check the consistency of this route tree. Looks for:
 * - invalid parent-child links
 * - invalid timing values
 * - congested SINKs
 * Returns true if OK. */
bool RouteTree::is_valid(void) const {
    return is_valid_x(*_root);
}

/** Helper for is_valid */
bool RouteTree::is_valid_x(const RouteTreeNode& rt_node) const {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    constexpr float CAP_REL_TOL = 1e-6;
    constexpr float CAP_ABS_TOL = vtr::DEFAULT_ABS_TOL;
    constexpr float RES_REL_TOL = 1e-6;
    constexpr float RES_ABS_TOL = vtr::DEFAULT_ABS_TOL;

    RRNodeId inode = rt_node.inode;
    RRSwitchId iswitch = rt_node.parent_switch;
    if (rt_node.parent()) {
        if (rr_graph.rr_switch_inf(iswitch).buffered()) {
            float R_upstream_check = rr_graph.node_R(inode) + rr_graph.rr_switch_inf(iswitch).R;
            if (!vtr::isclose(rt_node.R_upstream, R_upstream_check, RES_REL_TOL, RES_ABS_TOL)) {
                VTR_LOG("%d mismatch R upstream %e supposed %e\n", inode, rt_node.R_upstream, R_upstream_check);
                return false;
            }
        } else {
            float R_upstream_check = rr_graph.node_R(inode) + rt_node.parent().value().R_upstream + rr_graph.rr_switch_inf(iswitch).R;
            if (!vtr::isclose(rt_node.R_upstream, R_upstream_check, RES_REL_TOL, RES_ABS_TOL)) {
                VTR_LOG("%d mismatch R upstream %e supposed %e\n", inode, rt_node.R_upstream, R_upstream_check);
                return false;
            }
        }
    } else if (rt_node.R_upstream != rr_graph.node_R(inode)) {
        VTR_LOG("%d mismatch R upstream %e supposed %e\n", inode, rt_node.R_upstream, rr_graph.node_R(inode));
        return false;
    }

    if (rr_graph.node_type(inode) == SINK) { // sink, must not be congested and must not have fanouts
        int occ = route_ctx.rr_node_route_inf[inode].occ();
        int capacity = rr_graph.node_capacity(inode);
        if (rt_node._next != nullptr && rt_node._next->_parent == &rt_node) {
            VTR_LOG("SINK %d has fanouts?\n", inode);
            return false;
        }
        if (!rt_node.is_leaf()) {
            VTR_LOG("SINK %d has fanouts?\n", inode);
            return false;
        }
        if (occ > capacity) {
            VTR_LOG("SINK %d occ %d > cap %d\n", inode, occ, capacity);
            return false;
        }
    }

    // check downstream C
    float C_downstream_children = 0;
    for (auto& child : rt_node.child_nodes()) {
        if (child._parent != std::addressof(rt_node)) {
            VTR_LOG("parent-child relationship not mutually acknowledged by parent %d->%d child %d<-%d\n",
                    inode, child.inode,
                    child.inode, rt_node.inode);
            return false;
        }
        C_downstream_children += rr_graph.rr_switch_inf(child.parent_switch).Cinternal;

        if (!rr_graph.rr_switch_inf(child.parent_switch).buffered()) {
            C_downstream_children += child.C_downstream;
        }
        if (!is_valid_x(child)) {
            VTR_LOG("subtree %d invalid, propagating up\n", child.inode);
            return false;
        }
    }

    float C_downstream_check = C_downstream_children + rr_graph.node_C(inode);
    if (!vtr::isclose(rt_node.C_downstream, C_downstream_check, CAP_REL_TOL, CAP_ABS_TOL)) {
        VTR_LOG("%d mismatch C downstream %e supposed %e\n", inode, rt_node.C_downstream, C_downstream_check);
        return false;
    }

    return true;
}

/** Check if the tree has any overused nodes (-> the tree is congested).
 * Returns true if not congested */
bool RouteTree::is_uncongested(void) const {
    return is_uncongested_x(*_root);
}

/** Helper for is_uncongested */
bool RouteTree::is_uncongested_x(const RouteTreeNode& rt_node) const {
    auto& route_ctx = g_vpr_ctx.routing();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    RRNodeId inode = rt_node.inode;
    if (route_ctx.rr_node_route_inf[inode].occ() > rr_graph.node_capacity(RRNodeId(inode))) {
        //This node is congested
        return false;
    }

    for (auto& child : rt_node.child_nodes()) {
        if (!is_uncongested_x(child)) {
            // The sub-tree connected to this edge is congested
            return false;
        }
    }

    // The sub-tree below the current node is uncongested
    return true;
}

/** Print information about this route tree to stdout. */
void RouteTree::print(void) const {
    _root->print();
}

/** Add the most recently finished wire segment to the routing tree, and
 * update the Tdel, etc. numbers for the rest of the routing tree. hptr
 * is the heap pointer of the SINK that was reached, and target_net_pin_index
 * is the net pin index corresponding to the SINK that was reached. This routine
 * returns a tuple: RouteTreeNode of the branch it adds to the route tree and
 * RouteTreeNode of the SINK it adds to the routing. */
std::tuple<vtr::optional<const RouteTreeNode&>, vtr::optional<const RouteTreeNode&>>
RouteTree::update_from_heap(t_heap* hptr, int target_net_pin_index, SpatialRouteTreeLookup* spatial_rt_lookup, bool is_flat) {
    /* Lock the route tree for writing. At least on Linux this shouldn't have an impact on single-threaded code */
    std::unique_lock<std::mutex> write_lock(_write_mutex);

    //Create a new subtree from the target in hptr to existing routing
    vtr::optional<RouteTreeNode&> start_of_new_subtree_rt_node, sink_rt_node;
    std::tie(start_of_new_subtree_rt_node, sink_rt_node) = add_subtree_from_heap(hptr, target_net_pin_index, is_flat);

    if (!start_of_new_subtree_rt_node)
        return {vtr::nullopt, *sink_rt_node};

    /* Reload timing values */
    reload_timing_unlocked(start_of_new_subtree_rt_node);

    if (spatial_rt_lookup) {
        update_route_tree_spatial_lookup_recur(*start_of_new_subtree_rt_node, *spatial_rt_lookup);
    }

    if (_net_id.is_valid()) /* We don't have this lookup if the tree isn't associated with a net */
        _is_isink_reached[target_net_pin_index] = true;

    return {*start_of_new_subtree_rt_node, *sink_rt_node};
}

/* Adds the most recent wire segment, ending at the SINK indicated by hptr,
 * to the routing tree. target_net_pin_index is the net pin index corresponding
 * to the SINK indicated by hptr. Returns the first (most upstream) new rt_node,
 * and the rt_node of the new SINK. Traverses up from SINK  */
std::tuple<vtr::optional<RouteTreeNode&>, vtr::optional<RouteTreeNode&>>
RouteTree::add_subtree_from_heap(t_heap* hptr, int target_net_pin_index, bool is_flat) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    RRNodeId sink_inode = RRNodeId(hptr->index);

    /* Walk rr_node_route_inf up until we reach an existing RouteTreeNode */
    std::vector<RRNodeId> new_branch_inodes;
    std::vector<RRSwitchId> new_branch_iswitches;

    std::unordered_set<RRNodeId> all_visited;
    std::unordered_set<RRNodeId> main_branch_visited;

    /* We need this information to build the branch:
     * node -> switch -> node -> ... -> switch -> sink.
     * Here we create two vectors:
     * new_branch_inodes: [sink, nodeN-1, nodeN-2, ... node 1] of length N
     * and new_branch_iswitches: [N-1->sink, N-2->N-1, ... 2->1, 1->found_node] of length N */
    RREdgeId edge = hptr->prev_edge();
    RRNodeId new_inode = RRNodeId(hptr->prev_node());
    RRSwitchId new_iswitch = RRSwitchId(rr_graph.rr_nodes().edge_switch(edge));

    /* build a path, looking up rr nodes and switches from rr_node_route_inf */
    new_branch_inodes.push_back(sink_inode);
    while (!_rr_node_to_rt_node.count(new_inode)) {
        new_branch_inodes.push_back(new_inode);
        new_branch_iswitches.push_back(new_iswitch);
        edge = route_ctx.rr_node_route_inf[new_inode].prev_edge;
        new_inode = RRNodeId(route_ctx.rr_node_route_inf[new_inode].prev_node);
        new_iswitch = RRSwitchId(rr_graph.rr_nodes().edge_switch(edge));
    }
    new_branch_iswitches.push_back(new_iswitch);

    /* Build the new tree branch starting from the existing node we found */
    RouteTreeNode* last_node = _rr_node_to_rt_node[new_inode];
    all_visited.insert(last_node->inode);

    /* In the code below I'm marking SINKs and IPINs as not to be re-expanded.
     * It makes the code more efficient (though not vastly) to prune this way
     * when there aren't route-throughs or ipin doglegs.
     * ---
     * Walk through new_branch_iswitches and corresponding new_branch_inodes. */
    for (int i = new_branch_inodes.size() - 1; i >= 0; i--) {
        RouteTreeNode* new_node = new RouteTreeNode(new_branch_inodes[i], new_branch_iswitches[i], last_node);

        e_rr_type node_type = rr_graph.node_type(new_branch_inodes[i]);
        // If is_flat is enabled, IPINs should be added, since they are used for intra-cluster routing
        if (node_type == IPIN && !is_flat) {
            new_node->re_expand = false;
        } else if (node_type == SINK) {
            new_node->re_expand = false;
            new_node->net_pin_index = target_net_pin_index; // net pin index is invalid for non-SINK nodes
        } else {
            new_node->re_expand = true;
        }

        add_node(last_node, new_node);

        last_node = new_node;

        main_branch_visited.insert(new_branch_inodes[i]);
        all_visited.insert(new_branch_inodes[i]);
    }

    // Expand (recursively) each of the main-branch nodes adding any
    // non-configurably connected nodes
    // Sink is not included, so no need to pass in the node's ipin value.
    for (RRNodeId rr_node : main_branch_visited) {
        add_non_configurable_nodes(_rr_node_to_rt_node.at(rr_node), false, all_visited, is_flat);
    }

    /* the first and last nodes we added.
     * vec[size-1] works, because new_branch_inodes is guaranteed to contain at least [sink, found_node] */
    vtr::optional<RouteTreeNode&> downstream_rt_node = *_rr_node_to_rt_node.at(new_branch_inodes[new_branch_inodes.size() - 1]);
    vtr::optional<RouteTreeNode&> sink_rt_node = *_rr_node_to_rt_node.at(new_branch_inodes[0]);

    return {downstream_rt_node, sink_rt_node};
}

/** Add non-configurable nodes reachable from rt_node to the tree.
 * We keep non-configurable nodes in the routing if a connected non-conf
 * node is in use. This function finds those nodes and adds them to the tree */
void RouteTree::add_non_configurable_nodes(RouteTreeNode* rt_node,
                                           bool reached_by_non_configurable_edge,
                                           std::unordered_set<RRNodeId>& visited,
                                           bool is_flat) {
    RRNodeId rr_node = rt_node->inode;

    if (visited.count(rr_node) && reached_by_non_configurable_edge)
        return;

    visited.insert(rr_node);

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    for (int iedge : rr_graph.non_configurable_edges(rr_node)) {
        // Recursive case: expand children
        VTR_ASSERT(!rr_graph.edge_is_configurable(rr_node, iedge));
        RRNodeId to_rr_node = rr_graph.edge_sink_node(rr_node, iedge);

        if (_rr_node_to_rt_node.count(to_rr_node)) // TODO: not 100% sure about this
            continue;

        RRSwitchId edge_switch(rr_graph.edge_switch(rr_node, iedge));

        RouteTreeNode* new_node = new RouteTreeNode(to_rr_node, edge_switch, rt_node);
        add_node(rt_node, new_node);

        new_node->net_pin_index = OPEN;
        if (rr_graph.node_type(to_rr_node) == IPIN && !is_flat) {
            new_node->re_expand = false;
        } else {
            new_node->re_expand = true;
        }

        add_non_configurable_nodes(new_node, true, visited, is_flat);
    }
}

/** Prune a route tree of illegal branches - when there is at least 1 congested node on the path to a sink
 * Returns nullopt if the entire tree has been pruned.
 * Updates "is_isink_reached" lookup! After prune(), if a sink is marked as reached in the lookup, it is reached
 * legally.
 *
 * Note: does not update R_upstream/C_downstream */
vtr::optional<RouteTree&>
RouteTree::prune(CBRR& connections_inf, std::vector<int>* non_config_node_set_usage) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    std::unique_lock<std::mutex> write_lock(_write_mutex);

    VTR_ASSERT_MSG(rr_graph.node_type(root().inode) == SOURCE, "Root of route tree must be SOURCE");

    VTR_ASSERT_MSG(_net_id, "RouteTree must be constructed using a ParentNetId");

    VTR_ASSERT_MSG(route_ctx.rr_node_route_inf[root().inode].occ() <= rr_graph.node_capacity(root().inode),
                   "Route tree root/SOURCE should never be congested");

    auto pruned_node = prune_x(*_root, connections_inf, false, non_config_node_set_usage);
    if (pruned_node)
        return *this;
    else
        return vtr::nullopt;
}

/** Helper for prune.
 * Recursively traverse the route tree rooted at node and remove any congested subtrees.
 * Returns nullopt if pruned */
vtr::optional<RouteTreeNode&>
RouteTree::prune_x(RouteTreeNode& rt_node, CBRR& connections_inf, bool force_prune, std::vector<int>* non_config_node_set_usage) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();
    bool congested = (route_ctx.rr_node_route_inf[rt_node.inode].occ() > rr_graph.node_capacity(rt_node.inode));

    int node_set = -1;
    auto itr = device_ctx.rr_node_to_non_config_node_set.find(rt_node.inode);
    if (itr != device_ctx.rr_node_to_non_config_node_set.end()) {
        node_set = itr->second;
    }

    if (congested) {
        //This connection is congested -- prune it
        force_prune = true;
    }

    if (connections_inf.should_force_reroute_connection(_net_id, rt_node.inode)) {
        //Forcibly re-route (e.g. to improve delay)
        force_prune = true;
    }

    // Recursively prune child nodes
    bool all_children_pruned = true;
    remove_child_if(rt_node, [&](auto& child) {
        vtr::optional<RouteTreeNode&> child_maybe = prune_x(child, connections_inf, force_prune, non_config_node_set_usage);

        if (child_maybe.has_value()) { // Not pruned
            all_children_pruned = false;
            return false;
        } else { // Pruned
            // After removing a child node, check if non_config_node_set_usage
            // needs an update.
            if (non_config_node_set_usage != nullptr && node_set != -1 && rr_graph.rr_switch_inf(child.parent_switch).configurable()) {
                (*non_config_node_set_usage)[node_set] -= 1;
                VTR_ASSERT((*non_config_node_set_usage)[node_set] >= 0);
            }
            return true;
        }
    });

    if (rr_graph.node_type(rt_node.inode) == SINK) {
        if (!force_prune) {
            //Valid path to sink

            //Record sink as reached
            _is_isink_reached[rt_node.net_pin_index] = true;
            return rt_node; // Not pruned
        } else {
            //Record as not reached
            _is_isink_reached[rt_node.net_pin_index] = false;
            return vtr::nullopt; // Pruned
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
        //      This avoids the creation of unused 'stubs'. (For example if
        //      we left the stubs in and they were subsequently not used
        //      they would uselessly consume routing resources).
        VTR_ASSERT(rt_node.is_leaf());

        bool reached_non_configurably = false;
        if (rt_node.parent()) {
            reached_non_configurably = !rr_graph.rr_switch_inf(rt_node.parent_switch).configurable();

            if (reached_non_configurably) {
                // Check if this non-configurable node set is in use.
                VTR_ASSERT(node_set != -1);
                if (non_config_node_set_usage != nullptr && (*non_config_node_set_usage)[node_set] == 0) {
                    force_prune = true;
                }
            }
        }

        if (reached_non_configurably && !force_prune) {
            return rt_node; //Not pruned
        } else {
            return vtr::nullopt; //Pruned
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
        if (non_config_node_set_usage != nullptr && node_set != -1 && rr_graph.rr_switch_inf(rt_node.parent_switch).configurable() && (*non_config_node_set_usage)[node_set] == 0) {
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
            return prune_x(rt_node, connections_inf, /*force_prune=*/false, non_config_node_set_usage);
        }

        //An unpruned intermediate node
        VTR_ASSERT(!force_prune);

        return rt_node; //Not pruned
    }
}

/** Remove all sinks and mark the remaining nodes as un-expandable.
 * This is used after routing a clock net.
 * TODO: is this function doing anything? Try running without it */
void RouteTree::freeze(void) {
    std::unique_lock<std::mutex> write_lock(_write_mutex);
    return freeze_x(*_root);
}

/** Helper function for freeze. */
void RouteTree::freeze_x(RouteTreeNode& rt_node) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    remove_child_if(rt_node, [&](RouteTreeNode& child) {
        if (rr_graph.node_type(child.inode) == SINK) {
            VTR_LOGV_DEBUG(f_router_debug,
                           "Removing sink %d from route tree\n", child.inode);
            return true;
        } else {
            rt_node.re_expand = false;
            VTR_LOGV_DEBUG(f_router_debug,
                           "unexpanding: %d in route tree\n", rt_node.inode);
            freeze_x(child);
            return false;
        }
    });
}

/* For each non-configurable node set, count the cases in the route tree where:
 * 1. the node is a member of a nonconf set
 * if not SINK:
 *   2. and there *is* an outgoing edge (we are not at the end of a stub)
 *   3. and that outgoing edge is a configurable switch
 * if SINK:
 *   2. and the incoming edge is a configurable switch
 *     (Note: the old code's comments mention that a "non-configurable edge"
 *      "to" a sink is a usage of the set, but the code used to check if the
 *      edge "from" the SINK, which shouldn't exist, was "configurable". This
 *      might be some faulty code / unnecessary check carried over.) */
std::vector<int> RouteTree::get_non_config_node_set_usage(void) const {
    auto& device_ctx = g_vpr_ctx.device();
    std::vector<int> usage(device_ctx.rr_non_config_node_sets.size(), 0);

    const auto& rr_to_nonconf = device_ctx.rr_node_to_non_config_node_set;

    for (auto& rt_node : all_nodes()) {
        auto it = rr_to_nonconf.find(rt_node.inode);
        if (it == rr_to_nonconf.end())
            continue;

        if (device_ctx.rr_graph.node_type(rt_node.inode) == SINK) {
            if (device_ctx.rr_graph.rr_switch_inf(rt_node.parent_switch).configurable()) {
                usage[it->second] += 1;
            }
            continue;
        }

        for (auto& child : rt_node.child_nodes()) {
            if (device_ctx.rr_graph.rr_switch_inf(child.parent_switch).configurable()) {
                usage[it->second] += 1;
            }
        }
    }

    return usage;
}
