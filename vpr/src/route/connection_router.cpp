#include "connection_router.h"

#include <algorithm>
#include "rr_graph.h"
#include "rr_graph_fwd.h"

/** Used for the flat router. The node isn't relevant to the target if
 * it is an intra-block node outside of our target block */
static bool relevant_node_to_target(const RRGraphView* rr_graph,
                                    RRNodeId node_to_add,
                                    RRNodeId target_node);

static void update_router_stats(RouterStats* router_stats,
                                bool is_push,
                                RRNodeId rr_node_id,
                                const RRGraphView* rr_graph);

/** return tuple <found_path, retry_with_full_bb, cheapest> */
template<typename Heap>
std::tuple<bool, bool, RTExploredNode> ConnectionRouter<Heap>::timing_driven_route_connection_from_route_tree(
    const RouteTreeNode& rt_root,
    RRNodeId sink_node,
    const t_conn_cost_params& cost_params,
    const t_bb& bounding_box,
    RouterStats& router_stats,
    const ConnectionParameters& conn_params) {
    router_stats_ = &router_stats;
    conn_params_ = &conn_params;

    bool retry = false;
    retry = timing_driven_route_connection_common_setup(rt_root, sink_node, cost_params, bounding_box);

    if (!std::isinf(rr_node_route_inf_[sink_node].path_cost)) {
        // Only the `index`, `prev_edge`, and `rcv_path_backward_delay` fields of `out`
        // are used after this function returns.
        RTExploredNode out;
        out.index = sink_node;
        out.prev_edge = rr_node_route_inf_[sink_node].prev_edge;
        if (rcv_path_manager.is_enabled()) {
            out.rcv_path_backward_delay = rcv_path_data[sink_node]->backward_delay;
            rcv_path_manager.update_route_tree_set(rcv_path_data[sink_node]);
            rcv_path_manager.empty_heap();
        }
        heap_.empty_heap();
        return std::make_tuple(true, /*retry=*/false, out);
    } else {
        reset_path_costs();
        clear_modified_rr_node_info();
        heap_.empty_heap();
        rcv_path_manager.empty_heap();
        return std::make_tuple(false, retry, RTExploredNode());
    }
}

/** Return whether to retry with full bb */
template<typename Heap>
bool ConnectionRouter<Heap>::timing_driven_route_connection_common_setup(
    const RouteTreeNode& rt_root,
    RRNodeId sink_node,
    const t_conn_cost_params& cost_params,
    const t_bb& bounding_box) {
    //Re-add route nodes from the existing route tree to the heap.
    //They need to be repushed onto the heap since each node's cost is target specific.

    add_route_tree_to_heap(rt_root, sink_node, cost_params, bounding_box);
    heap_.build_heap(); // via sifting down everything

    RRNodeId source_node = rt_root.inode;

    if (heap_.is_empty_heap()) {
        VTR_LOG("No source in route tree: %s\n", describe_unrouteable_connection(source_node, sink_node, is_flat_).c_str());
        return false;
    }

    VTR_LOGV_DEBUG(router_debug_, "  Routing to %d as normal net (BB: %d,%d,%d x %d,%d,%d)\n", sink_node,
                   bounding_box.layer_min, bounding_box.xmin, bounding_box.ymin,
                   bounding_box.layer_max, bounding_box.xmax, bounding_box.ymax);

    timing_driven_route_connection_from_heap(sink_node,
                                             cost_params,
                                             bounding_box);

    if (std::isinf(rr_node_route_inf_[sink_node].path_cost)) {
        // No path found within the current bounding box.
        //
        // If the bounding box is already max size, just fail
        if (bounding_box.xmin == 0
            && bounding_box.ymin == 0
            && bounding_box.xmax == (int)(grid_.width() - 1)
            && bounding_box.ymax == (int)(grid_.height() - 1)
            && bounding_box.layer_min == 0
            && bounding_box.layer_max == (int)(grid_.get_num_layers() - 1)) {
            VTR_LOG("%s\n", describe_unrouteable_connection(source_node, sink_node, is_flat_).c_str());
            return false;
        }

        // Otherwise, leave unrouted and bubble up a signal to retry this net with a full-device bounding box
        VTR_LOG_WARN("No routing path for connection to sink_rr %d, leaving unrouted to retry later\n", sink_node);
        return true;
    }

    return false;
}

// Finds a path from the route tree rooted at rt_root to sink_node for a high fanout net.
//
// Unlike timing_driven_route_connection_from_route_tree(), only part of the route tree
// which is spatially close to the sink is added to the heap.
// Returns a  tuple of <found_path?, retry_with_full_bb?, cheapest> */
template<typename Heap>
std::tuple<bool, bool, RTExploredNode> ConnectionRouter<Heap>::timing_driven_route_connection_from_route_tree_high_fanout(
    const RouteTreeNode& rt_root,
    RRNodeId sink_node,
    const t_conn_cost_params& cost_params,
    const t_bb& net_bounding_box,
    const SpatialRouteTreeLookup& spatial_rt_lookup,
    RouterStats& router_stats,
    const ConnectionParameters& conn_params) {
    router_stats_ = &router_stats;
    conn_params_ = &conn_params;

    // re-explore route tree from root to add any new nodes (buildheap afterwards)
    // route tree needs to be repushed onto the heap since each node's cost is target specific
    t_bb high_fanout_bb = add_high_fanout_route_tree_to_heap(rt_root, sink_node, cost_params, spatial_rt_lookup, net_bounding_box);
    heap_.build_heap();

    RRNodeId source_node = rt_root.inode;

    if (heap_.is_empty_heap()) {
        VTR_LOG("No source in route tree: %s\n", describe_unrouteable_connection(source_node, sink_node, is_flat_).c_str());
        return std::make_tuple(false, false, RTExploredNode());
    }

    VTR_LOGV_DEBUG(router_debug_, "  Routing to %d as high fanout net (BB: %d,%d,%d x %d,%d,%d)\n", sink_node,
                   high_fanout_bb.layer_min, high_fanout_bb.xmin, high_fanout_bb.ymin,
                   high_fanout_bb.layer_max, high_fanout_bb.xmax, high_fanout_bb.ymax);

    bool retry_with_full_bb = false;
    timing_driven_route_connection_from_heap(sink_node,
                                             cost_params,
                                             high_fanout_bb);

    if (std::isinf(rr_node_route_inf_[sink_node].path_cost)) {
        //Found no path, that may be due to an unlucky choice of existing route tree sub-set,
        //try again with the full route tree to be sure this is not an artifact of high-fanout routing
        VTR_LOG_WARN("No routing path found in high-fanout mode for net %zu connection (to sink_rr %d), retrying with full route tree\n", size_t(conn_params.net_id_), sink_node);

        //Reset any previously recorded node costs so timing_driven_route_connection()
        //starts over from scratch.
        reset_path_costs();
        clear_modified_rr_node_info();

        retry_with_full_bb = timing_driven_route_connection_common_setup(rt_root,
                                                                         sink_node,
                                                                         cost_params,
                                                                         net_bounding_box);
    }

    if (std::isinf(rr_node_route_inf_[sink_node].path_cost)) {
        VTR_LOG("%s\n", describe_unrouteable_connection(source_node, sink_node, is_flat_).c_str());

        heap_.empty_heap();
        rcv_path_manager.empty_heap();
        return std::make_tuple(false, retry_with_full_bb, RTExploredNode());
    }

    RTExploredNode out;
    out.index = sink_node;
    out.prev_edge = rr_node_route_inf_[sink_node].prev_edge;
    if (rcv_path_manager.is_enabled()) {
        out.rcv_path_backward_delay = rcv_path_data[sink_node]->backward_delay;
        rcv_path_manager.update_route_tree_set(rcv_path_data[sink_node]);
        rcv_path_manager.empty_heap();
    }
    heap_.empty_heap();

    return std::make_tuple(true, retry_with_full_bb, out);
}

// Finds a path to sink_node, starting from the elements currently in the heap.
// This is the core maze routing routine.
template<typename Heap>
void ConnectionRouter<Heap>::timing_driven_route_connection_from_heap(RRNodeId sink_node,
                                                                      const t_conn_cost_params& cost_params,
                                                                      const t_bb& bounding_box) {
    VTR_ASSERT_SAFE(heap_.is_valid());

    if (heap_.is_empty_heap()) { //No source
        VTR_LOGV_DEBUG(router_debug_, "  Initial heap empty (no source)\n");
    }

    const auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    // Get bounding box for sink node used in timing_driven_expand_neighbour
    VTR_ASSERT_SAFE(sink_node != RRNodeId::INVALID());

    t_bb target_bb;
    if (rr_graph_->node_type(sink_node) == SINK) { // We need to get a bounding box for the sink's entire tile
        vtr::Rect<int> tile_bb = grid_.get_tile_bb({rr_graph_->node_xlow(sink_node),
                                                    rr_graph_->node_ylow(sink_node),
                                                    rr_graph_->node_layer(sink_node)});

        target_bb.xmin = tile_bb.xmin();
        target_bb.ymin = tile_bb.ymin();
        target_bb.xmax = tile_bb.xmax();
        target_bb.ymax = tile_bb.ymax();
    } else {
        target_bb.xmin = rr_graph_->node_xlow(sink_node);
        target_bb.ymin = rr_graph_->node_ylow(sink_node);
        target_bb.xmax = rr_graph_->node_xhigh(sink_node);
        target_bb.ymax = rr_graph_->node_yhigh(sink_node);
    }

    target_bb.layer_min = rr_graph_->node_layer(RRNodeId(sink_node));
    target_bb.layer_max = rr_graph_->node_layer(RRNodeId(sink_node));

    // Start measuring path search time
    std::chrono::steady_clock::time_point begin_time = std::chrono::steady_clock::now();

    HeapNode cheapest;
    while (heap_.try_pop(cheapest)) {
        // inode with the cheapest total cost in current route tree to be expanded on
        const auto& [ new_total_cost, inode ] = cheapest;
        update_router_stats(router_stats_,
                            /*is_push=*/false,
                            inode,
                            rr_graph_);

        VTR_LOGV_DEBUG(router_debug_, "  Popping node %d (cost: %g)\n",
                       inode, new_total_cost);

        // Have we found the target?
        if (inode == sink_node) {
            // If we're running RCV, the path will be stored in the path_data->path_rr vector
            // This is then placed into the traceback so that the correct path is returned
            // TODO: This can be eliminated by modifying the actual traceback function in route_timing
            if (rcv_path_manager.is_enabled()) {
                rcv_path_manager.insert_backwards_path_into_traceback(rcv_path_data[inode],
                                                                      rr_node_route_inf_[inode].path_cost,
                                                                      rr_node_route_inf_[inode].backward_path_cost,
                                                                      route_ctx);
            }
            VTR_LOGV_DEBUG(router_debug_, "  Found target %8d (%s)\n", inode, describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, inode, is_flat_).c_str());
            break;
        }

        // If not, keep searching
        timing_driven_expand_cheapest(inode,
                                      new_total_cost,
                                      sink_node,
                                      cost_params,
                                      bounding_box,
                                      target_bb);
    }

    // Stop measuring path search time
    std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();
    path_search_cumulative_time += std::chrono::duration_cast<std::chrono::microseconds>(end_time - begin_time);
}

// Find shortest paths from specified route tree to all nodes in the RR graph
template<typename Heap>
vtr::vector<RRNodeId, RTExploredNode> ConnectionRouter<Heap>::timing_driven_find_all_shortest_paths_from_route_tree(
    const RouteTreeNode& rt_root,
    const t_conn_cost_params& cost_params,
    const t_bb& bounding_box,
    RouterStats& router_stats,
    const ConnectionParameters& conn_params) {
    router_stats_ = &router_stats;
    conn_params_ = &conn_params;

    // Add the route tree to the heap with no specific target node
    RRNodeId target_node = RRNodeId::INVALID();
    add_route_tree_to_heap(rt_root, target_node, cost_params, bounding_box);
    heap_.build_heap(); // via sifting down everything

    auto res = timing_driven_find_all_shortest_paths_from_heap(cost_params, bounding_box);
    heap_.empty_heap();

    return res;
}

// Find shortest paths from current heap to all nodes in the RR graph
//
// Since there is no single *target* node this uses Dijkstra's algorithm
// with a modified exit condition (runs until heap is empty).
template<typename Heap>
vtr::vector<RRNodeId, RTExploredNode> ConnectionRouter<Heap>::timing_driven_find_all_shortest_paths_from_heap(
    const t_conn_cost_params& cost_params,
    const t_bb& bounding_box) {
    vtr::vector<RRNodeId, RTExploredNode> cheapest_paths(rr_nodes_.size());

    VTR_ASSERT_SAFE(heap_.is_valid());

    if (heap_.is_empty_heap()) { // No source
        VTR_LOGV_DEBUG(router_debug_, "  Initial heap empty (no source)\n");
    }

    // Start measuring path search time
    std::chrono::steady_clock::time_point begin_time = std::chrono::steady_clock::now();

    HeapNode cheapest;
    while (heap_.try_pop(cheapest)) {
        // inode with the cheapest total cost in current route tree to be expanded on
        const auto& [ new_total_cost, inode ] = cheapest;
        update_router_stats(router_stats_,
                            /*is_push=*/false,
                            inode,
                            rr_graph_);

        VTR_LOGV_DEBUG(router_debug_, "  Popping node %d (cost: %g)\n",
                       inode, new_total_cost);

        // Since we want to find shortest paths to all nodes in the graph
        // we do not specify a target node.
        //
        // By setting the target_node to INVALID in combination with the NoOp router
        // lookahead we can re-use the node exploration code from the regular router
        RRNodeId target_node = RRNodeId::INVALID();

        timing_driven_expand_cheapest(inode,
                                      new_total_cost,
                                      target_node,
                                      cost_params,
                                      bounding_box,
                                      t_bb());

        if (cheapest_paths[inode].index == RRNodeId::INVALID() || cheapest_paths[inode].total_cost >= new_total_cost) {
            VTR_LOGV_DEBUG(router_debug_, "  Better cost to node %d: %g (was %g)\n", inode, new_total_cost, cheapest_paths[inode].total_cost);
            // Only the `index` and `prev_edge` fields of `cheapest_paths[inode]` are used after this function returns
            cheapest_paths[inode].index = inode;
            cheapest_paths[inode].prev_edge = rr_node_route_inf_[inode].prev_edge;
        } else {
            VTR_LOGV_DEBUG(router_debug_, "  Worse cost to node %d: %g (better %g)\n", inode, new_total_cost, cheapest_paths[inode].total_cost);
        }
    }

    // Stop measuring path search time
    std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();
    path_search_cumulative_time += std::chrono::duration_cast<std::chrono::microseconds>(end_time - begin_time);

    return cheapest_paths;
}

template<typename Heap>
void ConnectionRouter<Heap>::timing_driven_expand_cheapest(RRNodeId from_node,
                                                           float new_total_cost,
                                                           RRNodeId target_node,
                                                           const t_conn_cost_params& cost_params,
                                                           const t_bb& bounding_box,
                                                           const t_bb& target_bb) {
    float best_total_cost = rr_node_route_inf_[from_node].path_cost;
    if (best_total_cost == new_total_cost) {
        // Explore from this node, since its total cost is exactly the same as
        // the best total cost ever seen for this node. Otherwise, prune this node
        // to reduce redundant work (i.e., unnecessary neighbor exploration).
        // `new_total_cost` is used here as an identifier to detect if the pair
        // (from_node or inode, new_total_cost) was the most recently pushed
        // element for the corresponding node.
        //
        // Note: For RCV, it often isn't searching for a shortest path; it is
        // searching for a path in the target delay range. So it might find a
        // path to node n that has a higher `backward_path_cost` but the `total_cost`
        // (including expected delay to sink, going through a cost function that
        // checks that against the target delay) might be lower than the previously
        // stored value. In that case we want to re-expand the node so long as
        // it doesn't create a loop. That `rcv_path_manager` should store enough
        // info for us to avoid loops.
        RTExploredNode current;
        current.index = from_node;
        current.backward_path_cost = rr_node_route_inf_[from_node].backward_path_cost;
        current.prev_edge = rr_node_route_inf_[from_node].prev_edge;
        current.R_upstream = rr_node_route_inf_[from_node].R_upstream;

        VTR_LOGV_DEBUG(router_debug_, "    Better cost to %d\n", from_node);
        VTR_LOGV_DEBUG(router_debug_, "    New total cost: %g\n", new_total_cost);
        VTR_LOGV_DEBUG(router_debug_ && (current.prev_edge != RREdgeId::INVALID()),
                       "      Setting path costs for associated node %d (from %d edge %zu)\n",
                       from_node,
                       static_cast<size_t>(rr_graph_->edge_src_node(current.prev_edge)),
                       static_cast<size_t>(current.prev_edge));

        timing_driven_expand_neighbours(current, cost_params, bounding_box, target_node, target_bb);
    } else {
        // Post-heap prune, do not re-explore from the current/new partial path as it
        // has worse cost than the best partial path to this node found so far
        VTR_LOGV_DEBUG(router_debug_, "    Worse cost to %d\n", from_node);
        VTR_LOGV_DEBUG(router_debug_, "    Old total cost: %g\n", best_total_cost);
        VTR_LOGV_DEBUG(router_debug_, "    New total cost: %g\n", new_total_cost);
    }
}

template<typename Heap>
void ConnectionRouter<Heap>::timing_driven_expand_neighbours(const RTExploredNode& current,
                                                             const t_conn_cost_params& cost_params,
                                                             const t_bb& bounding_box,
                                                             RRNodeId target_node,
                                                             const t_bb& target_bb) {
    /* Puts all the rr_nodes adjacent to current on the heap. */

    // For each node associated with the current heap element, expand all of it's neighbors
    auto edges = rr_nodes_.edge_range(current.index);

    // This is a simple prefetch that prefetches:
    //  - RR node data reachable from this node
    //  - rr switch data to reach those nodes from this node.
    //
    // This code will be a NOP on compiler targets that do not have a
    // builtin to emit prefetch instructions.
    //
    // This code will be a NOP on CPU targets that lack prefetch instructions.
    // All modern x86 and ARM64 platforms provide prefetch instructions.
    //
    // This code delivers ~6-8% reduction in wallclock time when running Titan
    // benchmarks, and was specifically measured against the gsm_switch and
    // directrf vtr_reg_weekly running in high effort.
    //
    //  - directrf_stratixiv_arch_timing.blif
    //  - gsm_switch_stratixiv_arch_timing.blif
    //
    for (RREdgeId from_edge : edges) {
        RRNodeId to_node = rr_nodes_.edge_sink_node(from_edge);
        rr_nodes_.prefetch_node(to_node);

        int switch_idx = rr_nodes_.edge_switch(from_edge);
        VTR_PREFETCH(&rr_switch_inf_[switch_idx], 0, 0);
    }

    for (RREdgeId from_edge : edges) {
        RRNodeId to_node = rr_nodes_.edge_sink_node(from_edge);
        timing_driven_expand_neighbour(current,
                                       from_edge,
                                       to_node,
                                       cost_params,
                                       bounding_box,
                                       target_node,
                                       target_bb);
    }
}

// Conditionally adds to_node to the router heap (via path from from_node via from_edge).
// RR nodes outside the expanded bounding box specified in bounding_box are not added
// to the heap.
template<typename Heap>
void ConnectionRouter<Heap>::timing_driven_expand_neighbour(const RTExploredNode& current,
                                                            RREdgeId from_edge,
                                                            RRNodeId to_node,
                                                            const t_conn_cost_params& cost_params,
                                                            const t_bb& bounding_box,
                                                            RRNodeId target_node,
                                                            const t_bb& target_bb) {
    VTR_ASSERT(bounding_box.layer_max < g_vpr_ctx.device().grid.get_num_layers());

    const RRNodeId& from_node = current.index;

    // BB-pruning
    // Disable BB-pruning if RCV is enabled, as this can make it harder for circuits with high negative hold slack to resolve this
    // TODO: Only disable pruning if the net has negative hold slack, maybe go off budgets
    if (!inside_bb(to_node, bounding_box)
        && !rcv_path_manager.is_enabled()) {
        VTR_LOGV_DEBUG(router_debug_,
                       "      Pruned expansion of node %d edge %zu -> %d"
                       " (to node location %d,%d,%d x %d,%d,%d outside of expanded"
                       " net bounding box %d,%d,%d x %d,%d,%d)\n",
                       from_node, size_t(from_edge), size_t(to_node),
                       rr_graph_->node_xlow(to_node), rr_graph_->node_ylow(to_node), rr_graph_->node_layer(to_node),
                       rr_graph_->node_xhigh(to_node), rr_graph_->node_yhigh(to_node), rr_graph_->node_layer(to_node),
                       bounding_box.xmin, bounding_box.ymin, bounding_box.layer_min,
                       bounding_box.xmax, bounding_box.ymax, bounding_box.layer_max);
        return; /* Node is outside (expanded) bounding box. */
    }

    /* Prune away IPINs that lead to blocks other than the target one.  Avoids  *
     * the issue of how to cost them properly so they don't get expanded before *
     * more promising routes, but makes route-through (via CLBs) impossible.   *
     * Change this if you want to investigate route-throughs.                   */
    if (target_node != RRNodeId::INVALID()) {
        t_rr_type to_type = rr_graph_->node_type(to_node);
        if (to_type == IPIN) {
            // Check if this IPIN leads to the target block
            // IPIN's of the target block should be contained within it's bounding box
            int to_xlow = rr_graph_->node_xlow(to_node);
            int to_ylow = rr_graph_->node_ylow(to_node);
            int to_layer = rr_graph_->node_layer(to_node);
            int to_xhigh = rr_graph_->node_xhigh(to_node);
            int to_yhigh = rr_graph_->node_yhigh(to_node);
            if (to_xlow < target_bb.xmin
                || to_ylow < target_bb.ymin
                || to_xhigh > target_bb.xmax
                || to_yhigh > target_bb.ymax
                || to_layer < target_bb.layer_min
                || to_layer > target_bb.layer_max) {
                VTR_LOGV_DEBUG(router_debug_,
                               "      Pruned expansion of node %d edge %zu -> %d"
                               " (to node is IPIN at %d,%d,%d x %d,%d,%d which does not"
                               " lead to target block %d,%d,%d x %d,%d,%d)\n",
                               from_node, size_t(from_edge), size_t(to_node),
                               to_xlow, to_ylow, to_layer,
                               to_xhigh, to_yhigh, to_layer,
                               target_bb.xmin, target_bb.ymin, target_bb.layer_min,
                               target_bb.xmax, target_bb.ymax, target_bb.layer_max);
                return;
            }
        }
    }

    VTR_LOGV_DEBUG(router_debug_, "      Expanding node %d edge %zu -> %d\n",
                   from_node, size_t(from_edge), size_t(to_node));

    // Check if the node exists in the route tree when RCV is enabled
    // Other pruning methods have been disabled when RCV is on, so this method is required to prevent "loops" from being created
    bool node_exists = false;
    if (rcv_path_manager.is_enabled()) {
        node_exists = rcv_path_manager.node_exists_in_tree(rcv_path_data[from_node],
                                                           to_node);
    }

    if (!node_exists || !rcv_path_manager.is_enabled()) {
        timing_driven_add_to_heap(cost_params,
                                  current,
                                  to_node,
                                  from_edge,
                                  target_node);
    }
}

// Add to_node to the heap, and also add any nodes which are connected by non-configurable edges
template<typename Heap>
void ConnectionRouter<Heap>::timing_driven_add_to_heap(const t_conn_cost_params& cost_params,
                                                       const RTExploredNode& current,
                                                       RRNodeId to_node,
                                                       const RREdgeId from_edge,
                                                       RRNodeId target_node) {
    const auto& device_ctx = g_vpr_ctx.device();
    const RRNodeId& from_node = current.index;

    // Initialized to current
    RTExploredNode next;
    next.R_upstream = current.R_upstream;
    next.index = to_node;
    next.prev_edge = from_edge;
    next.total_cost = std::numeric_limits<float>::infinity(); // Not used directly
    next.backward_path_cost = current.backward_path_cost;

    // Initalize RCV data struct if needed, otherwise it's set to nullptr
    rcv_path_manager.alloc_path_struct(next.path_data);
    // path_data variables are initialized to current values
    if (rcv_path_manager.is_enabled() && rcv_path_data[from_node]) {
        next.path_data->backward_cong = rcv_path_data[from_node]->backward_cong;
        next.path_data->backward_delay = rcv_path_data[from_node]->backward_delay;
    }

    evaluate_timing_driven_node_costs(&next,
                                      cost_params,
                                      from_node,
                                      target_node);

    float best_total_cost = rr_node_route_inf_[to_node].path_cost;
    float best_back_cost = rr_node_route_inf_[to_node].backward_path_cost;

    float new_total_cost = next.total_cost;
    float new_back_cost = next.backward_path_cost;

    // We need to only expand this node if it is a better path. And we need to
    // update its `rr_node_route_inf` data as we put it into the heap; there may
    // be other (previously explored) paths to this node in the heap already,
    // but they will be pruned when we pop those heap nodes later as we'll see
    // they have inferior costs to what is in the `rr_node_route_inf` data for
    // this node.
    // FIXME: Adding a link to the FPT paper when it is public
    //
    // When RCV is enabled, prune based on the RCV-specific total path cost (see
    // in `compute_node_cost_using_rcv` in `evaluate_timing_driven_node_costs`)
    // to allow detours to get better QoR.
    if ((!rcv_path_manager.is_enabled() && best_back_cost > new_back_cost) ||
        (rcv_path_manager.is_enabled() && best_total_cost > new_total_cost)) {
        VTR_LOGV_DEBUG(router_debug_, "      Expanding to node %d (%s)\n", to_node,
                       describe_rr_node(device_ctx.rr_graph,
                                        device_ctx.grid,
                                        device_ctx.rr_indexed_data,
                                        to_node,
                                        is_flat_)
                           .c_str());
        VTR_LOGV_DEBUG(router_debug_, "        New Total Cost %g New back Cost %g\n", new_total_cost, new_back_cost);
        //Add node to the heap only if the cost via the current partial path is less than the
        //best known cost, since there is no reason for the router to expand more expensive paths.
        //
        //Pre-heap prune to keep the heap small, by not putting paths which are known to be
        //sub-optimal (at this point in time) into the heap.

        update_cheapest(next, from_node);

        heap_.add_to_heap({new_total_cost, to_node});
        update_router_stats(router_stats_,
                            /*is_push=*/true,
                            to_node,
                            rr_graph_);

    } else {
        VTR_LOGV_DEBUG(router_debug_, "      Didn't expand to %d (%s)\n", to_node, describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, to_node, is_flat_).c_str());
        VTR_LOGV_DEBUG(router_debug_, "        Prev Total Cost %g Prev back Cost %g \n", best_total_cost, best_back_cost);
        VTR_LOGV_DEBUG(router_debug_, "        New Total Cost %g New back Cost %g \n", new_total_cost, new_back_cost);
    }

    if (rcv_path_manager.is_enabled() && next.path_data != nullptr) {
        rcv_path_manager.free_path_struct(next.path_data);
    }
}

#ifdef VTR_ASSERT_SAFE_ENABLED

//Returns true if both nodes are part of the same non-configurable edge set
static bool same_non_config_node_set(RRNodeId from_node, RRNodeId to_node) {
    auto& device_ctx = g_vpr_ctx.device();

    auto from_itr = device_ctx.rr_node_to_non_config_node_set.find(from_node);
    auto to_itr = device_ctx.rr_node_to_non_config_node_set.find(to_node);

    if (from_itr == device_ctx.rr_node_to_non_config_node_set.end()
        || to_itr == device_ctx.rr_node_to_non_config_node_set.end()) {
        return false; //Not part of a non-config node set
    }

    return from_itr->second == to_itr->second; //Check for same non-config set IDs
}

#endif

template<typename Heap>
float ConnectionRouter<Heap>::compute_node_cost_using_rcv(const t_conn_cost_params cost_params,
                                                          RRNodeId to_node,
                                                          RRNodeId target_node,
                                                          float backwards_delay,
                                                          float backwards_cong,
                                                          float R_upstream) {
    float expected_delay;
    float expected_cong;

    const t_conn_delay_budget* delay_budget = cost_params.delay_budget;
    // TODO: This function is not tested for is_flat == true
    VTR_ASSERT(is_flat_ != true);
    std::tie(expected_delay, expected_cong) = router_lookahead_.get_expected_delay_and_cong(to_node, target_node, cost_params, R_upstream);

    float expected_total_delay_cost;
    float expected_total_cong_cost;

    float expected_total_cong = expected_cong + backwards_cong;
    float expected_total_delay = expected_delay + backwards_delay;

    //If budgets specified calculate cost as described by RCV paper:
    //    R. Fung, V. Betz and W. Chow, "Slack Allocation and Routing to Improve FPGA Timing While
    //     Repairing Short-Path Violations," in IEEE Transactions on Computer-Aided Design of
    //     Integrated Circuits and Systems, vol. 27, no. 4, pp. 686-697, April 2008.

    // Normalization constant defined in RCV paper cited above
    constexpr float NORMALIZATION_CONSTANT = 100e-12;

    expected_total_delay_cost = expected_total_delay;
    expected_total_delay_cost += (delay_budget->short_path_criticality + cost_params.criticality) * std::max(0.f, delay_budget->target_delay - expected_total_delay);
    // expected_total_delay_cost += std::pow(std::max(0.f, expected_total_delay - delay_budget->max_delay), 2) / NORMALIZATION_CONSTANT;
    expected_total_delay_cost += std::pow(std::max(0.f, delay_budget->min_delay - expected_total_delay), 2) / NORMALIZATION_CONSTANT;
    expected_total_cong_cost = expected_total_cong;

    float total_cost = expected_total_delay_cost + expected_total_cong_cost;

    return total_cost;
}

// Empty the route tree set node, use this after each net is routed
template<typename Heap>
void ConnectionRouter<Heap>::empty_rcv_route_tree_set() {
    rcv_path_manager.empty_route_tree_nodes();
}

// Enable or disable RCV
template<typename Heap>
void ConnectionRouter<Heap>::set_rcv_enabled(bool enable) {
    rcv_path_manager.set_enabled(enable);
    if (enable) {
        rcv_path_data.resize(rr_node_route_inf_.size());
    }
}

//Calculates the cost of reaching to_node (i.e., to->index)
template<typename Heap>
void ConnectionRouter<Heap>::evaluate_timing_driven_node_costs(RTExploredNode* to,
                                                               const t_conn_cost_params& cost_params,
                                                               RRNodeId from_node,
                                                               RRNodeId target_node) {
    /* new_costs.backward_cost: is the "known" part of the cost to this node -- the
     * congestion cost of all the routing resources back to the existing route
     * plus the known delay of the total path back to the source.
     *
     * new_costs.total_cost: is this "known" backward cost + an expected cost to get to the target.
     *
     * new_costs.R_upstream: is the upstream resistance at the end of this node
     */

    //Info for the switch connecting from_node to_node (i.e., to->index)
    int iswitch = rr_nodes_.edge_switch(to->prev_edge);
    bool switch_buffered = rr_switch_inf_[iswitch].buffered();
    bool reached_configurably = rr_switch_inf_[iswitch].configurable();
    float switch_R = rr_switch_inf_[iswitch].R;
    float switch_Tdel = rr_switch_inf_[iswitch].Tdel;
    float switch_Cinternal = rr_switch_inf_[iswitch].Cinternal;

    //To node info
    auto rc_index = rr_graph_->node_rc_index(to->index);
    float node_C = rr_rc_data_[rc_index].C;
    float node_R = rr_rc_data_[rc_index].R;

    //From node info
    float from_node_R = rr_rc_data_[rr_graph_->node_rc_index(from_node)].R;

    //Update R_upstream
    if (switch_buffered) {
        to->R_upstream = 0.; //No upstream resistance
    } else {
        //R_Upstream already initialized
    }

    to->R_upstream += switch_R; //Switch resistance
    to->R_upstream += node_R;   //Node resistance

    //Calculate delay
    float Rdel = to->R_upstream - 0.5 * node_R; //Only consider half node's resistance for delay
    float Tdel = switch_Tdel + Rdel * node_C;

    //Depending on the switch used, the Tdel of the upstream node (from_node) may change due to
    //increased loading from the switch's internal capacitance.
    //
    //Even though this delay physically affects from_node, we make the adjustment (now) on the to_node,
    //since only once we've reached to to_node do we know the connection used (and the switch enabled).
    //
    //To adjust for the time delay, we compute the product of the Rdel associated with from_node and
    //the internal capacitance of the switch.
    //
    //First, we will calculate Rdel_adjust (just like in the computation for Rdel, we consider only
    //half of from_node's resistance).
    float Rdel_adjust = to->R_upstream - 0.5 * from_node_R;

    //Second, we adjust the Tdel to account for the delay caused by the internal capacitance.
    Tdel += Rdel_adjust * switch_Cinternal;

    float cong_cost = 0.;
    if (reached_configurably) {
        cong_cost = get_rr_cong_cost(to->index, cost_params.pres_fac);
    } else {
        //Reached by a non-configurable edge.
        //Therefore the from_node and to_node are part of the same non-configurable node set.
#ifdef VTR_ASSERT_SAFE_ENABLED
        VTR_ASSERT_SAFE_MSG(same_non_config_node_set(from_node, to->index),
                            "Non-configurably connected edges should be part of the same node set");
#endif

        //The congestion cost of all nodes in the set has already been accounted for (when
        //the current path first expanded a node in the set). Therefore do *not* re-add the congestion
        //cost.
        cong_cost = 0.;
    }
    if (conn_params_->router_opt_choke_points_ && is_flat_ && rr_graph_->node_type(to->index) == IPIN) {
        auto find_res = conn_params_->connection_choking_spots_.find(to->index);
        if (find_res != conn_params_->connection_choking_spots_.end()) {
            cong_cost = cong_cost / pow(2, (float)find_res->second);
        }
    }

    //Update the backward cost (upstream already included)
    to->backward_path_cost += (1. - cost_params.criticality) * cong_cost; //Congestion cost
    to->backward_path_cost += cost_params.criticality * Tdel;             //Delay cost

    if (cost_params.bend_cost != 0.) {
        t_rr_type from_type = rr_graph_->node_type(from_node);
        t_rr_type to_type = rr_graph_->node_type(to->index);
        if ((from_type == CHANX && to_type == CHANY) || (from_type == CHANY && to_type == CHANX)) {
            to->backward_path_cost += cost_params.bend_cost; //Bend cost
        }
    }

    float total_cost = 0.;

    if (rcv_path_manager.is_enabled() && to->path_data != nullptr) {
        to->path_data->backward_delay += cost_params.criticality * Tdel;
        to->path_data->backward_cong += (1. - cost_params.criticality) * get_rr_cong_cost(to->index, cost_params.pres_fac);

        total_cost = compute_node_cost_using_rcv(cost_params, to->index, target_node, to->path_data->backward_delay, to->path_data->backward_cong, to->R_upstream);
    } else {
        const auto& device_ctx = g_vpr_ctx.device();
        //Update total cost
        float expected_cost = router_lookahead_.get_expected_cost(to->index, target_node, cost_params, to->R_upstream);
        VTR_LOGV_DEBUG(router_debug_ && !std::isfinite(expected_cost),
                        "        Lookahead from %s (%s) to %s (%s) is non-finite, expected_cost = %f, to->R_upstream = %f\n",
                        rr_node_arch_name(to->index, is_flat_).c_str(),
                        describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, to->index, is_flat_).c_str(),
                        rr_node_arch_name(target_node, is_flat_).c_str(),
                        describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, target_node, is_flat_).c_str(),
                        expected_cost, to->R_upstream);
        total_cost += to->backward_path_cost + cost_params.astar_fac * std::max(0.f, expected_cost - cost_params.astar_offset);
    }
    to->total_cost = total_cost;
}

//Adds the route tree rooted at rt_node to the heap, preparing it to be
//used as branch-points for further routing.
template<typename Heap>
void ConnectionRouter<Heap>::add_route_tree_to_heap(
    const RouteTreeNode& rt_node,
    RRNodeId target_node,
    const t_conn_cost_params& cost_params,
    const t_bb& net_bb) {
    /* Puts the entire partial routing below and including rt_node onto the heap *
     * (except for those parts marked as not to be expanded) by calling itself   *
     * recursively.                                                              */

    /* Pre-order depth-first traversal */
    // IPINs and SINKS are not re_expanded
    if (rt_node.re_expand) {
        add_route_tree_node_to_heap(rt_node,
                                    target_node,
                                    cost_params,
                                    net_bb);
    }

    for (const RouteTreeNode& child_node : rt_node.child_nodes()) {
        if (is_flat_) {
            if (relevant_node_to_target(rr_graph_,
                                        child_node.inode,
                                        target_node)) {
                add_route_tree_to_heap(child_node,
                                       target_node,
                                       cost_params,
                                       net_bb);
            }
        } else {
            add_route_tree_to_heap(child_node,
                                   target_node,
                                   cost_params,
                                   net_bb);
        }
    }
}

//Unconditionally adds rt_node to the heap
//
//Note that if you want to respect rt_node.re_expand that is the caller's
//responsibility.
template<typename Heap>
void ConnectionRouter<Heap>::add_route_tree_node_to_heap(
    const RouteTreeNode& rt_node,
    RRNodeId target_node,
    const t_conn_cost_params& cost_params,
    const t_bb& net_bb) {
    const auto& device_ctx = g_vpr_ctx.device();
    const RRNodeId inode = rt_node.inode;
    float backward_path_cost = cost_params.criticality * rt_node.Tdel;
    float R_upstream = rt_node.R_upstream;

    /* Don't push to heap if not in bounding box: no-op for serial router, important for parallel router */
    if (!inside_bb(rt_node.inode, net_bb))
        return;

    // after budgets are loaded, calculate delay cost as described by RCV paper
    /* R. Fung, V. Betz and W. Chow, "Slack Allocation and Routing to Improve FPGA Timing While
     * Repairing Short-Path Violations," in IEEE Transactions on Computer-Aided Design of
     * Integrated Circuits and Systems, vol. 27, no. 4, pp. 686-697, April 2008.*/
    // float expected_cost = router_lookahead_.get_expected_cost(inode, target_node, cost_params, R_upstream);

    if (!rcv_path_manager.is_enabled()) {
        // tot_cost = backward_path_cost + cost_params.astar_fac * expected_cost;
        float expected_cost = router_lookahead_.get_expected_cost(inode, target_node, cost_params, R_upstream);
        float tot_cost = backward_path_cost + cost_params.astar_fac * std::max(0.f, expected_cost - cost_params.astar_offset);
        VTR_LOGV_DEBUG(router_debug_, "  Adding node %8d to heap from init route tree with cost %g (%s)\n",
                       inode,
                       tot_cost,
                       describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, inode, is_flat_).c_str());

        if (tot_cost > rr_node_route_inf_[inode].path_cost) {
            return ;
        }
        add_to_mod_list(inode);
        rr_node_route_inf_[inode].path_cost = tot_cost;
        rr_node_route_inf_[inode].prev_edge = RREdgeId::INVALID();
        rr_node_route_inf_[inode].backward_path_cost = backward_path_cost;
        rr_node_route_inf_[inode].R_upstream = R_upstream;
        heap_.push_back({tot_cost, inode});

        // push_back_node(&heap_, rr_node_route_inf_,
        //                inode, tot_cost, RREdgeId::INVALID(),
        //                backward_path_cost, R_upstream);
    } else {
        float expected_total_cost = compute_node_cost_using_rcv(cost_params, inode, target_node, rt_node.Tdel, 0, R_upstream);

        add_to_mod_list(inode);
        rr_node_route_inf_[inode].path_cost = expected_total_cost;
        rr_node_route_inf_[inode].prev_edge = RREdgeId::INVALID();
        rr_node_route_inf_[inode].backward_path_cost = backward_path_cost;
        rr_node_route_inf_[inode].R_upstream = R_upstream;

        rcv_path_manager.alloc_path_struct(rcv_path_data[inode]);
        rcv_path_data[inode]->backward_delay = rt_node.Tdel;

        heap_.push_back({expected_total_cost, inode});

        // push_back_node_with_info(&heap_, inode, expected_total_cost,
        //                          backward_path_cost, R_upstream, rt_node.Tdel, &rcv_path_manager);
    }

    update_router_stats(router_stats_,
                        /*is_push=*/true,
                        inode,
                        rr_graph_);

    if constexpr (VTR_ENABLE_DEBUG_LOGGING_CONST_EXPR) {
        router_stats_->rt_node_pushes[rr_graph_->node_type(inode)]++;
    }
}

/* Expand bb by inode's extents and clip against net_bb */
inline void expand_highfanout_bounding_box(t_bb& bb, const t_bb& net_bb, RRNodeId inode, const RRGraphView* rr_graph) {
    bb.xmin = std::max<int>(net_bb.xmin, std::min<int>(bb.xmin, rr_graph->node_xlow(inode)));
    bb.ymin = std::max<int>(net_bb.ymin, std::min<int>(bb.ymin, rr_graph->node_ylow(inode)));
    bb.xmax = std::min<int>(net_bb.xmax, std::max<int>(bb.xmax, rr_graph->node_xhigh(inode)));
    bb.ymax = std::min<int>(net_bb.ymax, std::max<int>(bb.ymax, rr_graph->node_yhigh(inode)));
    bb.layer_min = std::min<int>(bb.layer_min, rr_graph->node_layer(inode));
    bb.layer_max = std::max<int>(bb.layer_max, rr_graph->node_layer(inode));
}

/* Expand bb by HIGH_FANOUT_BB_FAC and clip against net_bb */
inline void adjust_highfanout_bounding_box(t_bb& bb, const t_bb& net_bb) {
    constexpr int HIGH_FANOUT_BB_FAC = 3;

    bb.xmin = std::max<int>(net_bb.xmin, bb.xmin - HIGH_FANOUT_BB_FAC);
    bb.ymin = std::max<int>(net_bb.ymin, bb.ymin - HIGH_FANOUT_BB_FAC);
    bb.xmax = std::min<int>(net_bb.xmax, bb.xmax + HIGH_FANOUT_BB_FAC);
    bb.ymax = std::min<int>(net_bb.ymax, bb.ymax + HIGH_FANOUT_BB_FAC);
    bb.layer_min = std::min<int>(net_bb.layer_min, bb.layer_min);
    bb.layer_max = std::max<int>(net_bb.layer_max, bb.layer_max);
}

template<typename Heap>
t_bb ConnectionRouter<Heap>::add_high_fanout_route_tree_to_heap(
    const RouteTreeNode& rt_root,
    RRNodeId target_node,
    const t_conn_cost_params& cost_params,
    const SpatialRouteTreeLookup& spatial_rt_lookup,
    const t_bb& net_bounding_box) {
    //For high fanout nets we only add those route tree nodes which are spatially close
    //to the sink.
    //
    //Based on:
    //  J. Swartz, V. Betz, J. Rose, "A Fast Routability-Driven Router for FPGAs", FPGA, 1998
    //
    //We rely on a grid-based spatial look-up which is maintained for high fanout nets by
    //update_route_tree(), which allows us to add spatially close route tree nodes without traversing
    //the entire route tree (which is likely large for a high fanout net).

    //Determine which bin the target node is located in

    int target_bin_x = grid_to_bin_x(rr_graph_->node_xlow(target_node), spatial_rt_lookup);
    int target_bin_y = grid_to_bin_y(rr_graph_->node_ylow(target_node), spatial_rt_lookup);

    auto target_layer = rr_graph_->node_layer(target_node);

    int chan_nodes_added = 0;

    t_bb highfanout_bb;
    highfanout_bb.xmin = rr_graph_->node_xlow(target_node);
    highfanout_bb.xmax = rr_graph_->node_xhigh(target_node);
    highfanout_bb.ymin = rr_graph_->node_ylow(target_node);
    highfanout_bb.ymax = rr_graph_->node_yhigh(target_node);
    highfanout_bb.layer_min = target_layer;
    highfanout_bb.layer_max = target_layer;

    //Add existing routing starting from the target bin.
    //If the target's bin has insufficient existing routing add from the surrounding bins
    constexpr int SINGLE_BIN_MIN_NODES = 2;
    bool done = false;
    bool found_node_on_same_layer = false;
    for (int dx : {0, -1, +1}) {
        size_t bin_x = target_bin_x + dx;

        if (bin_x > spatial_rt_lookup.dim_size(0) - 1) continue; //Out of range

        for (int dy : {0, -1, +1}) {
            size_t bin_y = target_bin_y + dy;

            if (bin_y > spatial_rt_lookup.dim_size(1) - 1) continue; //Out of range

            for (const RouteTreeNode& rt_node : spatial_rt_lookup[bin_x][bin_y]) {
                if (!rt_node.re_expand) // Some nodes (like IPINs) shouldn't be re-expanded
                    continue;
                RRNodeId rr_node_to_add = rt_node.inode;

                /* Flat router: don't go into clusters other than the target one */
                if (is_flat_) {
                    if (!relevant_node_to_target(rr_graph_, rr_node_to_add, target_node))
                        continue;
                }

                /* In case of the parallel router, we may be dealing with a virtual net
                 * so prune the nodes from the HF lookup against the bounding box just in case */
                if (!inside_bb(rr_node_to_add, net_bounding_box))
                    continue;

                auto rt_node_layer_num = rr_graph_->node_layer(rr_node_to_add);
                if (rt_node_layer_num == target_layer)
                    found_node_on_same_layer = true;

                // Put the node onto the heap
                add_route_tree_node_to_heap(rt_node, target_node, cost_params, net_bounding_box);

                // Expand HF BB to include the node (clip by original BB)
                expand_highfanout_bounding_box(highfanout_bb, net_bounding_box, rr_node_to_add, rr_graph_);

                if (rr_graph_->node_type(rr_node_to_add) == CHANY || rr_graph_->node_type(rr_node_to_add) == CHANX) {
                    chan_nodes_added++;
                }
            }

            if (dx == 0 && dy == 0 && chan_nodes_added > SINGLE_BIN_MIN_NODES && found_node_on_same_layer) {
                //Target bin contained at least minimum amount of routing
                //
                //We require at least SINGLE_BIN_MIN_NODES to be added.
                //This helps ensure we don't end up with, for example, a single
                //routing wire running in the wrong direction which may not be
                //able to reach the target within the bounding box.
                done = true;
                break;
            }
        }
        if (done) break;
    }
    /* If we didn't find enough nodes to branch off near the target
     * or they are on the wrong grid layer, just add the full route tree */
    if (chan_nodes_added <= SINGLE_BIN_MIN_NODES || !found_node_on_same_layer) {
        add_route_tree_to_heap(rt_root, target_node, cost_params, net_bounding_box);
        return net_bounding_box;
    } else {
        //We found nearby routing, replace original bounding box to be localized around that routing
        adjust_highfanout_bounding_box(highfanout_bb, net_bounding_box);
        return highfanout_bb;
    }
}

static inline bool relevant_node_to_target(const RRGraphView* rr_graph,
                                           RRNodeId node_to_add,
                                           RRNodeId target_node) {
    VTR_ASSERT_SAFE(rr_graph->node_type(target_node) == t_rr_type::SINK);
    auto node_to_add_type = rr_graph->node_type(node_to_add);
    return node_to_add_type != t_rr_type::IPIN || node_in_same_physical_tile(node_to_add, target_node);
}

static inline void update_router_stats(RouterStats* router_stats,
                                       bool is_push,
                                       RRNodeId rr_node_id,
                                       const RRGraphView* rr_graph) {
    if (is_push) {
        router_stats->heap_pushes++;
    } else {
        router_stats->heap_pops++;
    }

    if constexpr (VTR_ENABLE_DEBUG_LOGGING_CONST_EXPR) {
        auto node_type = rr_graph->node_type(rr_node_id);
        VTR_ASSERT(node_type != NUM_RR_TYPES);

        if (is_inter_cluster_node(*rr_graph, rr_node_id)) {
            if (is_push) {
                router_stats->inter_cluster_node_pushes++;
                router_stats->inter_cluster_node_type_cnt_pushes[node_type]++;
            } else {
                router_stats->inter_cluster_node_pops++;
                router_stats->inter_cluster_node_type_cnt_pops[node_type]++;
            }
        } else {
            if (is_push) {
                router_stats->intra_cluster_node_pushes++;
                router_stats->intra_cluster_node_type_cnt_pushes[node_type]++;
            } else {
                router_stats->intra_cluster_node_pops++;
                router_stats->intra_cluster_node_type_cnt_pops[node_type]++;
            }
        }
    }
}

std::unique_ptr<ConnectionRouterInterface> make_connection_router(e_heap_type heap_type,
                                                                  const DeviceGrid& grid,
                                                                  const RouterLookahead& router_lookahead,
                                                                  const t_rr_graph_storage& rr_nodes,
                                                                  const RRGraphView* rr_graph,
                                                                  const std::vector<t_rr_rc_data>& rr_rc_data,
                                                                  const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switch_inf,
                                                                  vtr::vector<RRNodeId, t_rr_node_route_inf>& rr_node_route_inf,
                                                                  bool is_flat) {
    switch (heap_type) {
        case e_heap_type::BINARY_HEAP:
            return std::make_unique<ConnectionRouter<BinaryHeap>>(
                grid,
                router_lookahead,
                rr_nodes,
                rr_graph,
                rr_rc_data,
                rr_switch_inf,
                rr_node_route_inf,
                is_flat);
        case e_heap_type::FOUR_ARY_HEAP:
            return std::make_unique<ConnectionRouter<FourAryHeap>>(
                grid,
                router_lookahead,
                rr_nodes,
                rr_graph,
                rr_rc_data,
                rr_switch_inf,
                rr_node_route_inf,
                is_flat);
        default:
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Unknown heap_type %d",
                            heap_type);
    }
}
