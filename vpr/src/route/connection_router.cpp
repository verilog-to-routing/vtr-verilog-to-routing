#include "connection_router.h"
#include "rr_graph.h"

#include "binary_heap.h"
#include "bucket.h"
#include "rr_graph_fwd.h"

static inline bool relevant_node_to_target(const RRGraphView* rr_graph,
                                           RRNodeId node_to_add,
                                           RRNodeId target_node) {
    VTR_ASSERT(rr_graph->node_type(target_node) == t_rr_type::SINK);
    auto node_to_add_type = rr_graph->node_type(node_to_add);
    if (node_to_add_type == t_rr_type::OPIN || node_to_add_type == t_rr_type::SOURCE || node_to_add_type == t_rr_type::CHANX || node_to_add_type == t_rr_type::CHANY || node_to_add_type == SINK) {
        return true;
    } else if (node_in_same_physical_tile(node_to_add, target_node)) {
        VTR_ASSERT(node_to_add_type == IPIN);
        return true;
    }
    return false;
}

inline void update_router_stats(const DeviceContext& device_ctx,
                                const RRGraphView* rr_graph,
                                RouterStats* router_stats,
                                RRNodeId rr_node_id,
                                bool is_push) {
    if (is_push) {
        router_stats->heap_pushes++;
    } else {
        router_stats->heap_pops++;
    }

    auto node_type = rr_graph->node_type(rr_node_id);
    VTR_ASSERT(node_type != NUM_RR_TYPES);
    t_physical_tile_type_ptr physical_type = device_ctx.grid.get_physical_type({rr_graph->node_xlow(rr_node_id),
                                                                                rr_graph->node_ylow(rr_node_id),
                                                                                rr_graph->node_layer(rr_node_id)});

    if (is_inter_cluster_node(physical_type,
                              node_type,
                              rr_graph->node_ptc_num(rr_node_id))) {
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

/** return tuple <found_path, retry_with_full_bb, cheapest> */
template<typename Heap>
std::tuple<bool, bool, t_heap> ConnectionRouter<Heap>::timing_driven_route_connection_from_route_tree(
    const RouteTreeNode& rt_root,
    RRNodeId sink_node,
    const t_conn_cost_params cost_params,
    t_bb bounding_box,
    RouterStats& router_stats,
    const ConnectionParameters& conn_params,
    bool can_grow_bb) {
    router_stats_ = &router_stats;
    conn_params_ = &conn_params;

    bool retry = false;
    t_heap* cheapest;
    std::tie(retry, cheapest) = timing_driven_route_connection_common_setup(rt_root, sink_node, cost_params, bounding_box, can_grow_bb);

    if (cheapest != nullptr) {
        rcv_path_manager.update_route_tree_set(cheapest->path_data);
        update_cheapest(cheapest);
        t_heap out = *cheapest;
        heap_.free(cheapest);
        heap_.empty_heap();
        rcv_path_manager.empty_heap();
        return std::make_tuple(true, /*retry=*/false, out);
    } else {
        reset_path_costs();
        modified_rr_node_inf_.clear();
        heap_.empty_heap();
        return std::make_tuple(false, retry, t_heap());
    }
}

/** Return <retry with full bb?, cheapest> */
template<typename Heap>
std::tuple<bool, t_heap*> ConnectionRouter<Heap>::timing_driven_route_connection_common_setup(
    const RouteTreeNode& rt_root,
    RRNodeId sink_node,
    const t_conn_cost_params cost_params,
    t_bb bounding_box,
    bool can_grow_bb) {
    //Re-add route nodes from the existing route tree to the heap.
    //They need to be repushed onto the heap since each node's cost is target specific.

    add_route_tree_to_heap(rt_root, sink_node, cost_params, false);
    heap_.build_heap(); // via sifting down everything

    RRNodeId source_node = rt_root.inode;

    if (heap_.is_empty_heap()) {
        VTR_LOG("No source in route tree: %s\n", describe_unrouteable_connection(source_node, sink_node, is_flat_).c_str());
        return std::make_tuple(false, nullptr);
    }

    VTR_LOGV_DEBUG(router_debug_, "  Routing to %d as normal net (BB: %d,%d x %d,%d)\n", sink_node,
                   bounding_box.xmin, bounding_box.ymin,
                   bounding_box.xmax, bounding_box.ymax);

    t_heap* cheapest = timing_driven_route_connection_from_heap(sink_node,
                                                                cost_params,
                                                                bounding_box);

    if (cheapest == nullptr) {
        // No path found within the current bounding box.
        //
        // If the bounding box is already max size, just fail
        if (bounding_box.xmin == 0
            && bounding_box.ymin == 0
            && bounding_box.xmax == (int)(grid_.width() - 1)
            && bounding_box.ymax == (int)(grid_.height() - 1)) {
            VTR_LOG("%s\n", describe_unrouteable_connection(source_node, sink_node, is_flat_).c_str());
            return std::make_tuple(false, nullptr);
        }

        // If we cannot grow the bounding box, leave unrouted and bubble up a signal
        // to retry this net with a full-device bounding box. If we are already at full device extents,
        // just fail
        if (!can_grow_bb) {
            VTR_LOG_WARN("No routing path for connection to sink_rr %d, leaving unrouted to retry on next iteration\n", sink_node);
            return std::make_tuple(true, nullptr);
        }

        // Otherwise, try again with full-device bounding box.
        //
        // Note that the additional run-time overhead of re-trying only occurs
        // when we were otherwise going to give up -- the typical case (route
        // found with the bounding box) remains fast and never re-tries .
        VTR_LOG_WARN("No routing path for connection to sink_rr %d, retrying with full device bounding box\n", sink_node);

        t_bb full_device_bounding_box;
        full_device_bounding_box.xmin = 0;
        full_device_bounding_box.ymin = 0;
        full_device_bounding_box.xmax = grid_.width() - 1;
        full_device_bounding_box.ymax = grid_.height() - 1;

        //
        //TODO: potential future optimization
        //      We have already explored the RR nodes accessible within the regular
        //      BB (which are stored in modified_rr_node_inf), and so already know
        //      their cost from the source. Instead of re-starting the path search
        //      from scratch (i.e. from the previous route tree as we do below), we
        //      could just re-add all the explored nodes to the heap and continue
        //      expanding.
        //

        //Reset any previously recorded node costs so that when we call
        //add_route_tree_to_heap() the nodes in the route tree actually
        //make it back into the heap.
        reset_path_costs();
        modified_rr_node_inf_.clear();
        heap_.empty_heap();

        //Re-initialize the heap since it was emptied by the previous call to
        //timing_driven_route_connection_from_heap()
        add_route_tree_to_heap(rt_root, sink_node, cost_params, false);
        heap_.build_heap(); // via sifting down everything

        //Try finding the path again with the relaxed bounding box
        cheapest = timing_driven_route_connection_from_heap(sink_node,
                                                            cost_params,
                                                            full_device_bounding_box);
    }

    if (cheapest == nullptr) {
        VTR_LOG("%s\n", describe_unrouteable_connection(source_node, sink_node, is_flat_).c_str());
        return std::make_tuple(false, nullptr);
    }

    return std::make_tuple(false, cheapest);
}

// Finds a path from the route tree rooted at rt_root to sink_node for a high fanout net.
//
// Unlike timing_driven_route_connection_from_route_tree(), only part of the route tree
// which is spatially close to the sink is added to the heap.
// Returns a  tuple of <found_path?, retry_with_full_bb?, cheapest> */
template<typename Heap>
std::tuple<bool, bool, t_heap> ConnectionRouter<Heap>::timing_driven_route_connection_from_route_tree_high_fanout(
    const RouteTreeNode& rt_root,
    RRNodeId sink_node,
    const t_conn_cost_params cost_params,
    t_bb net_bounding_box,
    const SpatialRouteTreeLookup& spatial_rt_lookup,
    RouterStats& router_stats,
    const ConnectionParameters& conn_params,
    bool can_grow_bb) {
    router_stats_ = &router_stats;
    conn_params_ = &conn_params;

    // re-explore route tree from root to add any new nodes (buildheap afterwards)
    // route tree needs to be repushed onto the heap since each node's cost is target specific
    router_stats_->add_high_fanout_rt++;
    t_bb high_fanout_bb = add_high_fanout_route_tree_to_heap(rt_root, sink_node, cost_params, spatial_rt_lookup, net_bounding_box);
    heap_.build_heap();

    RRNodeId source_node = rt_root.inode;

    if (heap_.is_empty_heap()) {
        VTR_LOG("No source in route tree: %s\n", describe_unrouteable_connection(source_node, sink_node, is_flat_).c_str());
        return std::make_tuple(false, false, t_heap());
    }

    VTR_LOGV_DEBUG(router_debug_, "  Routing to %d as high fanout net (BB: %d,%d x %d,%d)\n", sink_node,
                   high_fanout_bb.xmin, high_fanout_bb.ymin,
                   high_fanout_bb.xmax, high_fanout_bb.ymax);

    bool retry_with_full_bb = false;
    t_heap* cheapest;
    cheapest = timing_driven_route_connection_from_heap(sink_node,
                                                        cost_params,
                                                        high_fanout_bb);

    if (cheapest == nullptr) {
        //Found no path, that may be due to an unlucky choice of existing route tree sub-set,
        //try again with the full route tree to be sure this is not an artifact of high-fanout routing
        VTR_LOG_WARN("No routing path found in high-fanout mode for net connection (to sink_rr %d), retrying with full route tree\n", sink_node);

        //Reset any previously recorded node costs so timing_driven_route_connection()
        //starts over from scratch.
        reset_path_costs();
        modified_rr_node_inf_.clear();

        std::tie(retry_with_full_bb, cheapest) = timing_driven_route_connection_common_setup(rt_root,
                                                                                             sink_node,
                                                                                             cost_params,
                                                                                             net_bounding_box,
                                                                                             can_grow_bb);
    }

    if (cheapest == nullptr) {
        VTR_LOG("%s\n", describe_unrouteable_connection(source_node, sink_node, is_flat_).c_str());

        heap_.empty_heap();
        rcv_path_manager.empty_heap();
        return std::make_tuple(false, retry_with_full_bb, t_heap());
    }

    rcv_path_manager.update_route_tree_set(cheapest->path_data);
    update_cheapest(cheapest);

    t_heap out = *cheapest;
    heap_.free(cheapest);
    heap_.empty_heap();
    rcv_path_manager.empty_heap();

    return std::make_tuple(true, retry_with_full_bb, out);
}

//Finds a path to sink_node, starting from the elements currently in the heap.
//
// This is the core maze routing routine.
//
// Returns either the last element of the path, or nullptr if no path is found
template<typename Heap>
t_heap* ConnectionRouter<Heap>::timing_driven_route_connection_from_heap(RRNodeId sink_node,
                                                                         const t_conn_cost_params cost_params,
                                                                         t_bb bounding_box) {
    VTR_ASSERT_SAFE(heap_.is_valid());
    //std::cout << "using this: " << (void *)this << "\n";
    //std::cout << "using heap: " << heap_.get_ptr() << "\n";

    if (heap_.is_empty_heap()) { //No source
        VTR_LOGV_DEBUG(router_debug_, "  Initial heap empty (no source)\n");
    }

    const auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    t_heap* cheapest = nullptr;
    while (!heap_.is_empty_heap()) {
        // cheapest t_heap in current route tree to be expanded on
        cheapest = heap_.get_heap_head();
        update_router_stats(device_ctx,
                            rr_graph_,
                            router_stats_,
                            cheapest->index,
                            false);

        RRNodeId inode = cheapest->index;
        VTR_LOGV_DEBUG(router_debug_, "  Popping node %d (cost: %g)\n",
                       inode, cheapest->cost);

        // Have we found the target?
        if (inode == sink_node) {
            // If we're running RCV, the path will be stored in the path_data->path_rr vector
            // This is then placed into the traceback so that the correct path is returned
            // TODO: This can be eliminated by modifying the actual traceback function in route_timing
            if (rcv_path_manager.is_enabled()) {
                rcv_path_manager.insert_backwards_path_into_traceback(cheapest->path_data, cheapest->cost, cheapest->backward_path_cost, route_ctx);
            }
            VTR_LOGV_DEBUG(router_debug_, "  Found target %8d (%s)\n", inode, describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, inode, is_flat_).c_str());
            break;
        }

        // If not, keep searching
        timing_driven_expand_cheapest(cheapest,
                                      sink_node,
                                      cost_params,
                                      bounding_box);

        rcv_path_manager.free_path_struct(cheapest->path_data);
        heap_.free(cheapest);
        cheapest = nullptr;
    }

    if (router_debug_) {
        //Update known path costs for nodes pushed but not popped, useful for debugging
        empty_heap_annotating_node_route_inf();
    }

    if (cheapest == nullptr) { /* Impossible routing.  No path for net. */
        VTR_LOGV_DEBUG(router_debug_, "  Empty heap (no path found)\n");
        return nullptr;
    }

    return cheapest;
}

// Find shortest paths from specified route tree to all nodes in the RR graph
template<typename Heap>
vtr::vector<RRNodeId, t_heap> ConnectionRouter<Heap>::timing_driven_find_all_shortest_paths_from_route_tree(
    const RouteTreeNode& rt_root,
    const t_conn_cost_params cost_params,
    t_bb bounding_box,
    RouterStats& router_stats,
    const ConnectionParameters& conn_params) {
    router_stats_ = &router_stats;
    conn_params_ = &conn_params;

    // Add the route tree to the heap with no specific target node
    RRNodeId target_node = RRNodeId::INVALID();
    add_route_tree_to_heap(rt_root, target_node, cost_params, false);
    heap_.build_heap(); // via sifting down everything

    auto res = timing_driven_find_all_shortest_paths_from_heap(cost_params, bounding_box);
    heap_.empty_heap();

    return res;
}

// Find shortest paths from current heap to all nodes in the RR graph
//
// Since there is no single *target* node this uses Dijkstra's algorithm
// with a modified exit condition (runs until heap is empty).
//
// Note that to re-use code used for the regular A*-based router we use a
// no-operation lookahead which always returns zero.
template<typename Heap>
vtr::vector<RRNodeId, t_heap> ConnectionRouter<Heap>::timing_driven_find_all_shortest_paths_from_heap(
    const t_conn_cost_params cost_params,
    t_bb bounding_box) {
    vtr::vector<RRNodeId, t_heap> cheapest_paths(rr_nodes_.size());

    VTR_ASSERT_SAFE(heap_.is_valid());

    if (heap_.is_empty_heap()) { // No source
        VTR_LOGV_DEBUG(router_debug_, "  Initial heap empty (no source)\n");
    }

    while (!heap_.is_empty_heap()) {
        // cheapest t_heap in current route tree to be expanded on
        t_heap* cheapest = heap_.get_heap_head();
        update_router_stats(g_vpr_ctx.device(),
                            rr_graph_,
                            router_stats_,
                            cheapest->index,
                            false);

        RRNodeId inode = cheapest->index;
        VTR_LOGV_DEBUG(router_debug_, "  Popping node %d (cost: %g)\n",
                       inode, cheapest->cost);

        // Since we want to find shortest paths to all nodes in the graph
        // we do not specify a target node.
        //
        // By setting the target_node to INVALID in combination with the NoOp router
        // lookahead we can re-use the node exploration code from the regular router
        RRNodeId target_node = RRNodeId::INVALID();

        timing_driven_expand_cheapest(cheapest,
                                      target_node,
                                      cost_params,
                                      bounding_box);

        if (cheapest_paths[inode].index == RRNodeId::INVALID() || cheapest_paths[inode].cost >= cheapest->cost) {
            VTR_LOGV_DEBUG(router_debug_, "  Better cost to node %d: %g (was %g)\n", inode, cheapest->cost, cheapest_paths[inode].cost);
            cheapest_paths[inode] = *cheapest;
        } else {
            VTR_LOGV_DEBUG(router_debug_, "  Worse cost to node %d: %g (better %g)\n", inode, cheapest->cost, cheapest_paths[inode].cost);
        }

        rcv_path_manager.free_path_struct(cheapest->path_data);
        heap_.free(cheapest);
    }

    return cheapest_paths;
}

template<typename Heap>
void ConnectionRouter<Heap>::timing_driven_expand_cheapest(t_heap* cheapest,
                                                           RRNodeId target_node,
                                                           const t_conn_cost_params cost_params,
                                                           t_bb bounding_box) {
    RRNodeId inode = cheapest->index;

    t_rr_node_route_inf* route_inf = &rr_node_route_inf_[inode];
    float best_total_cost = route_inf->path_cost;
    float best_back_cost = route_inf->backward_path_cost;

    float new_total_cost = cheapest->cost;
    float new_back_cost = cheapest->backward_path_cost;

    /* I only re-expand a node if both the "known" backward cost is lower  *
     * in the new expansion (this is necessary to prevent loops from       *
     * forming in the routing and causing havoc) *and* the expected total  *
     * cost to the sink is lower than the old value.  Different R_upstream *
     * values could make a path with lower back_path_cost less desirable   *
     * than one with higher cost.  Test whether or not I should disallow   *
     * re-expansion based on a higher total cost.                          */

    if (best_total_cost > new_total_cost && ((rcv_path_manager.is_enabled()) || best_back_cost > new_back_cost)) {
        // Explore from this node, since the current/new partial path has the best cost
        // found so far
        VTR_LOGV_DEBUG(router_debug_, "    Better cost to %d\n", inode);
        VTR_LOGV_DEBUG(router_debug_, "    New total cost: %g\n", new_total_cost);
        VTR_LOGV_DEBUG(router_debug_, "    New back cost: %g\n", new_back_cost);
        VTR_LOGV_DEBUG(router_debug_, "      Setting path costs for associated node %d (from %d edge %zu)\n",
                       cheapest->index,
                       cheapest->prev_node(),
                       size_t(cheapest->prev_edge()));

        update_cheapest(cheapest, route_inf);

        timing_driven_expand_neighbours(cheapest, cost_params, bounding_box,
                                        target_node);
    } else {
        // Post-heap prune, do not re-explore from the current/new partial path as it
        // has worse cost than the best partial path to this node found so far
        VTR_LOGV_DEBUG(router_debug_, "    Worse cost to %d\n", inode);
        VTR_LOGV_DEBUG(router_debug_, "    Old total cost: %g\n", best_total_cost);
        VTR_LOGV_DEBUG(router_debug_, "    Old back cost: %g\n", best_back_cost);
        VTR_LOGV_DEBUG(router_debug_, "    New total cost: %g\n", new_total_cost);
        VTR_LOGV_DEBUG(router_debug_, "    New back cost: %g\n", new_back_cost);
    }
}

template<typename Heap>
void ConnectionRouter<Heap>::timing_driven_expand_neighbours(t_heap* current,
                                                             const t_conn_cost_params cost_params,
                                                             t_bb bounding_box,
                                                             RRNodeId target_node) {
    /* Puts all the rr_nodes adjacent to current on the heap. */

    t_bb target_bb;
    if (target_node != RRNodeId::INVALID()) {
        target_bb.xmin = rr_graph_->node_xlow(target_node);
        target_bb.ymin = rr_graph_->node_ylow(target_node);
        target_bb.xmax = rr_graph_->node_xhigh(target_node);
        target_bb.ymax = rr_graph_->node_yhigh(target_node);
    }

    // For each node associated with the current heap element, expand all of it's neighbors
    RRNodeId from_node = current->index;
    auto edges = rr_nodes_.edge_range(from_node);

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
                                       from_node,
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
void ConnectionRouter<Heap>::timing_driven_expand_neighbour(t_heap* current,
                                                            RRNodeId from_node,
                                                            RREdgeId from_edge,
                                                            RRNodeId to_node,
                                                            const t_conn_cost_params cost_params,
                                                            const t_bb bounding_box,
                                                            RRNodeId target_node,
                                                            const t_bb target_bb) {
    int to_xlow = rr_graph_->node_xlow(to_node);
    int to_ylow = rr_graph_->node_ylow(to_node);
    int to_xhigh = rr_graph_->node_xhigh(to_node);
    int to_yhigh = rr_graph_->node_yhigh(to_node);

    // BB-pruning
    // Disable BB-pruning if RCV is enabled, as this can make it harder for circuits with high negative hold slack to resolve this
    // TODO: Only disable pruning if the net has negative hold slack, maybe go off budgets
    if ((to_xhigh < bounding_box.xmin    // Strictly left of BB left-edge
         || to_xlow > bounding_box.xmax  // Strictly right of BB right-edge
         || to_yhigh < bounding_box.ymin // Strictly below BB bottom-edge
         || to_ylow > bounding_box.ymax) // Strictly above BB top-edge
        && !rcv_path_manager.is_enabled()) {
        VTR_LOGV_DEBUG(router_debug_,
                       "      Pruned expansion of node %d edge %zu -> %d"
                       " (to node location %d,%dx%d,%d outside of expanded"
                       " net bounding box %d,%dx%d,%d)\n",
                       from_node, size_t(from_edge), size_t(to_node),
                       to_xlow, to_ylow, to_xhigh, to_yhigh,
                       bounding_box.xmin, bounding_box.ymin, bounding_box.xmax, bounding_box.ymax);
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
            if (to_xlow < target_bb.xmin
                || to_ylow < target_bb.ymin
                || to_xhigh > target_bb.xmax
                || to_yhigh > target_bb.ymax) {
                VTR_LOGV_DEBUG(router_debug_,
                               "      Pruned expansion of node %d edge %zu -> %d"
                               " (to node is IPIN at %d,%dx%d,%d which does not"
                               " lead to target block %d,%dx%d,%d)\n",
                               from_node, size_t(from_edge), size_t(to_node),
                               to_xlow, to_ylow, to_xhigh, to_yhigh,
                               target_bb.xmin, target_bb.ymin, target_bb.xmax, target_bb.ymax);
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
        node_exists = rcv_path_manager.node_exists_in_tree(current->path_data,
                                                           to_node);
    }

    if (!node_exists || !rcv_path_manager.is_enabled()) {
        timing_driven_add_to_heap(cost_params,
                                  current,
                                  from_node,
                                  to_node,
                                  from_edge,
                                  target_node);
    }
}

// Add to_node to the heap, and also add any nodes which are connected by non-configurable edges
template<typename Heap>
void ConnectionRouter<Heap>::timing_driven_add_to_heap(const t_conn_cost_params cost_params,
                                                       const t_heap* current,
                                                       RRNodeId from_node,
                                                       RRNodeId to_node,
                                                       const RREdgeId from_edge,
                                                       RRNodeId target_node) {
    const auto& device_ctx = g_vpr_ctx.device();
    t_heap next;

    // Initalize RCV data struct if needed, otherwise it's set to nullptr
    rcv_path_manager.alloc_path_struct(next.path_data);

    // Costs initialized to current
    next.cost = std::numeric_limits<float>::infinity(); //Not used directly
    next.backward_path_cost = current->backward_path_cost;

    // path_data variables are initialized to current values
    if (rcv_path_manager.is_enabled() && current->path_data) {
        next.path_data->backward_cong = current->path_data->backward_cong;
        next.path_data->backward_delay = current->path_data->backward_delay;
    }

    next.R_upstream = current->R_upstream;

    evaluate_timing_driven_node_costs(&next,
                                      cost_params,
                                      from_node,
                                      to_node,
                                      from_edge,
                                      target_node);

    float best_total_cost = rr_node_route_inf_[to_node].path_cost;
    float best_back_cost = rr_node_route_inf_[to_node].backward_path_cost;

    float new_total_cost = next.cost;
    float new_back_cost = next.backward_path_cost;

    if (new_total_cost < best_total_cost && ((rcv_path_manager.is_enabled()) || (new_back_cost < best_back_cost))) {
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
        t_heap* next_ptr = heap_.alloc();

        // Use the already created next path structure pointer when RCV is enabled
        if (rcv_path_manager.is_enabled()) rcv_path_manager.move(next_ptr->path_data, next.path_data);

        //Record how we reached this node
        next_ptr->cost = next.cost;
        next_ptr->R_upstream = next.R_upstream;
        next_ptr->backward_path_cost = next.backward_path_cost;
        next_ptr->index = to_node;
        next_ptr->set_prev_edge(from_edge);
        next_ptr->set_prev_node(from_node);

        if (rcv_path_manager.is_enabled() && current->path_data) {
            next_ptr->path_data->path_rr = current->path_data->path_rr;
            next_ptr->path_data->edge = current->path_data->edge;
            next_ptr->path_data->path_rr.emplace_back(from_node);
            next_ptr->path_data->edge.emplace_back(from_edge);
        }

        heap_.add_to_heap(next_ptr);
        update_router_stats(device_ctx,
                            rr_graph_,
                            router_stats_,
                            to_node,
                            true);

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

    float expected_total_cong = cost_params.astar_fac * expected_cong + backwards_cong;
    float expected_total_delay = cost_params.astar_fac * expected_delay + backwards_delay;

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
}

//Calculates the cost of reaching to_node
template<typename Heap>
void ConnectionRouter<Heap>::evaluate_timing_driven_node_costs(t_heap* to,
                                                               const t_conn_cost_params cost_params,
                                                               RRNodeId from_node,
                                                               RRNodeId to_node,
                                                               RREdgeId from_edge,
                                                               RRNodeId target_node) {
    /* new_costs.backward_cost: is the "known" part of the cost to this node -- the
     * congestion cost of all the routing resources back to the existing route
     * plus the known delay of the total path back to the source.
     *
     * new_costs.total_cost: is this "known" backward cost + an expected cost to get to the target.
     *
     * new_costs.R_upstream: is the upstream resistance at the end of this node
     */

    //Info for the switch connecting from_node to_node
    int iswitch = rr_nodes_.edge_switch(from_edge);
    bool switch_buffered = rr_switch_inf_[iswitch].buffered();
    bool reached_configurably = rr_switch_inf_[iswitch].configurable();
    float switch_R = rr_switch_inf_[iswitch].R;
    float switch_Tdel = rr_switch_inf_[iswitch].Tdel;
    float switch_Cinternal = rr_switch_inf_[iswitch].Cinternal;

    //To node info
    auto rc_index = rr_graph_->node_rc_index(to_node);
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
        cong_cost = get_rr_cong_cost(to_node, cost_params.pres_fac);
    } else {
        //Reached by a non-configurable edge.
        //Therefore the from_node and to_node are part of the same non-configurable node set.
#ifdef VTR_ASSERT_SAFE_ENABLED
        VTR_ASSERT_SAFE_MSG(same_non_config_node_set(from_node, to_node),
                            "Non-configurably connected edges should be part of the same node set");
#endif

        //The congestion cost of all nodes in the set has already been accounted for (when
        //the current path first expanded a node in the set). Therefore do *not* re-add the congestion
        //cost.
        cong_cost = 0.;
    }
    if (conn_params_->has_choking_spot_ && is_flat_ && rr_graph_->node_type(to_node) == IPIN) {
        auto find_res = conn_params_->connection_choking_spots_.find(to_node);
        if (find_res != conn_params_->connection_choking_spots_.end()) {
            cong_cost = cong_cost / pow(2, (float)find_res->second);
        }
    }

    //Update the backward cost (upstream already included)
    to->backward_path_cost += (1. - cost_params.criticality) * cong_cost; //Congestion cost
    to->backward_path_cost += cost_params.criticality * Tdel;             //Delay cost

    if (cost_params.bend_cost != 0.) {
        t_rr_type from_type = rr_graph_->node_type(from_node);
        t_rr_type to_type = rr_graph_->node_type(to_node);
        if ((from_type == CHANX && to_type == CHANY) || (from_type == CHANY && to_type == CHANX)) {
            to->backward_path_cost += cost_params.bend_cost; //Bend cost
        }
    }

    float total_cost = 0.;

    if (rcv_path_manager.is_enabled() && to->path_data != nullptr) {
        to->path_data->backward_delay += cost_params.criticality * Tdel;
        to->path_data->backward_cong += (1. - cost_params.criticality) * get_rr_cong_cost(to_node, cost_params.pres_fac);

        total_cost = compute_node_cost_using_rcv(cost_params, to_node, target_node, to->path_data->backward_delay, to->path_data->backward_cong, to->R_upstream);
    } else {
        const auto& device_ctx = g_vpr_ctx.device();
        //Update total cost
        float expected_cost = router_lookahead_.get_expected_cost(to_node,
                                                                  target_node,
                                                                  cost_params,
                                                                  to->R_upstream);
        VTR_LOGV_DEBUG(router_debug_ && !std::isfinite(expected_cost),
                       "        Lookahead from %s (%s) to %s (%s) is non-finite, expected_cost = %f, to->R_upstream = %f\n",
                       rr_node_arch_name(to_node, is_flat_).c_str(),
                       describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, to_node, is_flat_).c_str(),
                       rr_node_arch_name(target_node, is_flat_).c_str(),
                       describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, target_node, is_flat_).c_str(),
                       expected_cost, to->R_upstream);
        total_cost += to->backward_path_cost + cost_params.astar_fac * expected_cost;
    }
    to->cost = total_cost;
}

template<typename Heap>
void ConnectionRouter<Heap>::empty_heap_annotating_node_route_inf() {
    //Pop any remaining nodes in the heap and annotate their costs
    //
    //Useful for visualizing router expansion in graphics, as it shows
    //the cost of all nodes considered by the router (e.g. nodes never
    //expanded, such as parts of the initial route tree far from the
    //target).
    while (!heap_.is_empty_heap()) {
        t_heap* tmp = heap_.get_heap_head();

        rr_node_route_inf_[tmp->index].path_cost = tmp->cost;
        rr_node_route_inf_[tmp->index].backward_path_cost = tmp->backward_path_cost;
        modified_rr_node_inf_.push_back(tmp->index);

        rcv_path_manager.free_path_struct(tmp->path_data);
        heap_.free(tmp);
    }
}

//Adds the route tree rooted at rt_node to the heap, preparing it to be
//used as branch-points for further routing.
template<typename Heap>
void ConnectionRouter<Heap>::add_route_tree_to_heap(
    const RouteTreeNode& rt_node,
    RRNodeId target_node,
    const t_conn_cost_params cost_params,
    bool from_high_fanout) {
    /* Puts the entire partial routing below and including rt_node onto the heap *
     * (except for those parts marked as not to be expanded) by calling itself   *
     * recursively.                                                              */

    if (from_high_fanout) {
        router_stats_->add_all_rt_from_high_fanout++;
    } else {
        router_stats_->add_all_rt++;
    }

    /* Pre-order depth-first traversal */
    // IPINs and SINKS are not re_expanded
    if (rt_node.re_expand) {
        add_route_tree_node_to_heap(rt_node,
                                    target_node,
                                    cost_params,
                                    false);
    }

    for (const RouteTreeNode& child_node : rt_node.child_nodes()) {
        if (is_flat_) {
            if (relevant_node_to_target(rr_graph_,
                                        child_node.inode,
                                        target_node)) {
                add_route_tree_to_heap(child_node,
                                       target_node,
                                       cost_params,
                                       from_high_fanout);
            }
        } else {
            add_route_tree_to_heap(child_node,
                                   target_node,
                                   cost_params,
                                   from_high_fanout);
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
    const t_conn_cost_params cost_params,
    bool is_high_fanout) {
    const auto& device_ctx = g_vpr_ctx.device();
    const RRNodeId inode = rt_node.inode;
    float backward_path_cost = cost_params.criticality * rt_node.Tdel;
    float R_upstream = rt_node.R_upstream;

    // after budgets are loaded, calculate delay cost as described by RCV paper
    /* R. Fung, V. Betz and W. Chow, "Slack Allocation and Routing to Improve FPGA Timing While
     * Repairing Short-Path Violations," in IEEE Transactions on Computer-Aided Design of
     * Integrated Circuits and Systems, vol. 27, no. 4, pp. 686-697, April 2008.*/
    // float expected_cost = router_lookahead_.get_expected_cost(inode, target_node, cost_params, R_upstream);

    if (!rcv_path_manager.is_enabled()) {
        // tot_cost = backward_path_cost + cost_params.astar_fac * expected_cost;
        float tot_cost = backward_path_cost
                         + cost_params.astar_fac
                               * router_lookahead_.get_expected_cost(inode,
                                                                     target_node,
                                                                     cost_params,
                                                                     R_upstream);
        VTR_LOGV_DEBUG(router_debug_, "  Adding node %8d to heap from init route tree with cost %g (%s)\n",
                       inode,
                       tot_cost,
                       describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, inode, is_flat_).c_str());

        push_back_node(&heap_, rr_node_route_inf_,
                       inode, tot_cost, RRNodeId::INVALID(), RREdgeId::INVALID(),
                       backward_path_cost, R_upstream);
    } else {
        float expected_total_cost = compute_node_cost_using_rcv(cost_params, inode, target_node, rt_node.Tdel, 0, R_upstream);

        push_back_node_with_info(&heap_, inode, expected_total_cost,
                                 backward_path_cost, R_upstream, rt_node.Tdel, &rcv_path_manager);
    }

    update_router_stats(device_ctx,
                        rr_graph_,
                        router_stats_,
                        inode,
                        true);

    router_stats_->rt_node_pushes[rr_graph_->node_type(inode)]++;
    if (is_high_fanout) {
        router_stats_->rt_node_high_fanout_pushes[rr_graph_->node_type(inode)]++;
    } else {
        router_stats_->rt_node_entire_tree_pushes[rr_graph_->node_type(inode)]++;
    }
}

static t_bb adjust_highfanout_bounding_box(t_bb highfanout_bb) {
    t_bb bb = highfanout_bb;

    constexpr int HIGH_FANOUT_BB_FAC = 3;
    bb.xmin -= HIGH_FANOUT_BB_FAC;
    bb.ymin -= HIGH_FANOUT_BB_FAC;
    bb.xmax += HIGH_FANOUT_BB_FAC;
    bb.ymax += HIGH_FANOUT_BB_FAC;

    return bb;
}

template<typename Heap>
t_bb ConnectionRouter<Heap>::add_high_fanout_route_tree_to_heap(
    const RouteTreeNode& rt_root,
    RRNodeId target_node,
    const t_conn_cost_params cost_params,
    const SpatialRouteTreeLookup& spatial_rt_lookup,
    t_bb net_bounding_box) {
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

    int nodes_added = 0;
    int chan_nodes_added = 0;

    t_bb highfanout_bb;
    highfanout_bb.xmin = rr_graph_->node_xlow(target_node);
    highfanout_bb.xmax = rr_graph_->node_xhigh(target_node);
    highfanout_bb.ymin = rr_graph_->node_ylow(target_node);
    highfanout_bb.ymax = rr_graph_->node_yhigh(target_node);

    //Add existing routing starting from the target bin.
    //If the target's bin has insufficient existing routing add from the surrounding bins
    bool done = false;
    for (int dx : {0, -1, +1}) {
        size_t bin_x = target_bin_x + dx;

        if (bin_x > spatial_rt_lookup.dim_size(0) - 1) continue; //Out of range

        for (int dy : {0, -1, +1}) {
            size_t bin_y = target_bin_y + dy;

            if (bin_y > spatial_rt_lookup.dim_size(1) - 1) continue; //Out of range

            for (const RouteTreeNode& rt_node : spatial_rt_lookup[bin_x][bin_y]) {
                if (!rt_node.re_expand) continue; //Some nodes (like IPINs) shouldn't be re-expanded
                RRNodeId rr_node_to_add = rt_node.inode;

                if (is_flat_) {
                    if (!relevant_node_to_target(rr_graph_, rr_node_to_add, target_node))
                        continue;
                }

                // Put the node onto the heap
                add_route_tree_node_to_heap(rt_node, target_node, cost_params, true);

                // Update Bounding Box
                highfanout_bb.xmin = std::min<int>(highfanout_bb.xmin, rr_graph_->node_xlow(rr_node_to_add));
                highfanout_bb.ymin = std::min<int>(highfanout_bb.ymin, rr_graph_->node_ylow(rr_node_to_add));
                highfanout_bb.xmax = std::max<int>(highfanout_bb.xmax, rr_graph_->node_xhigh(rr_node_to_add));
                highfanout_bb.ymax = std::max<int>(highfanout_bb.ymax, rr_graph_->node_yhigh(rr_node_to_add));
                if (is_flat_) {
                    if (rr_graph_->node_type(rr_node_to_add) == CHANY || rr_graph_->node_type(rr_node_to_add) == CHANX) {
                        chan_nodes_added++;
                    }
                } else {
                    chan_nodes_added++;
                }
                nodes_added++;
            }

            constexpr int SINGLE_BIN_MIN_NODES = 2;
            if (dx == 0 && dy == 0 && chan_nodes_added > SINGLE_BIN_MIN_NODES) {
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

    t_bb bounding_box = net_bounding_box;
    if (nodes_added == 0) { //If the target bin, and it's surrounding bins were empty, just add the full route tree
        add_route_tree_to_heap(rt_root, target_node, cost_params, true);
    } else {
        //We found nearby routing, replace original bounding box to be localized around that routing
        bounding_box = adjust_highfanout_bounding_box(highfanout_bb);
    }

    return bounding_box;
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
        case e_heap_type::BUCKET_HEAP_APPROXIMATION:
            return std::make_unique<ConnectionRouter<Bucket>>(
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