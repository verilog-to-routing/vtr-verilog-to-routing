#include "parallel_connection_router.h"
#include <algorithm>
#include <chrono>
#include <tuple>
#include "router_lookahead.h"
#include "rr_graph.h"

#include "binary_heap.h"
#include "bucket.h"
#include "rr_graph_fwd.h"

// #define NON_DETERMINISTIC_PRUNING

/**
 * @brief This function is relevant when the architecture is 3D. If inter-layer connections are only from OPINs (determine by is_inter_layer_opin_connection),
 * then nodes (other that OPINs) which are on the other layer than sink's layer, don't need to be pushed back to the heap.
 * @param rr_nodes
 * @param rr_graph
 * @param from_node
 * @param sink_node
 * @param is_inter_layer_opin_connection It is true if the architecture is 3D and inter-layer connections are only from OPINs.
 * @return
 */
static bool has_path_to_sink(const t_rr_graph_view& rr_nodes,
                             const RRGraphView* rr_graph,
                             RRNodeId from_node,
                             RRNodeId sink_node,
                             bool is_inter_layer_opin_connection);

static bool relevant_node_to_target(const RRGraphView* rr_graph,
                                    RRNodeId node_to_add,
                                    RRNodeId target_node);

#ifdef VTR_ENABLE_DEBUG_LOGGING
static void update_router_stats(RouterStats* router_stats,
                                bool is_push,
                                RRNodeId rr_node_id,
                                const RRGraphView* rr_graph);
#else
static void update_router_stats(RouterStats* router_stats,
                                bool is_push,
                                RRNodeId /*rr_node_id*/,
                                const RRGraphView* /*rr_graph*/);
#endif

/** return tuple <found_path, retry_with_full_bb, cheapest> */
std::tuple<bool, bool, t_heap> ParallelConnectionRouter::timing_driven_route_connection_from_route_tree(
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
        t_heap out;
        out.index = sink_node;
        out.set_prev_edge(rr_node_route_inf_[sink_node].prev_edge);
        out.path_data = nullptr;
        heap_.empty_heap();
        return std::make_tuple(true, /*retry=*/false, out);
    } else {
        reset_path_costs();
        clear_modified_rr_node_info();
        heap_.empty_heap();
        return std::make_tuple(false, retry, t_heap());
    }
}

/** Return <retry with full bb?, cheapest> */
bool ParallelConnectionRouter::timing_driven_route_connection_common_setup(
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

    if (std::isinf(rr_node_route_inf_[sink_node].path_cost)) {
        VTR_LOG("%s\n", describe_unrouteable_connection(source_node, sink_node, is_flat_).c_str());
        return false;
    }

    return false;
}

// Finds a path from the route tree rooted at rt_root to sink_node for a high fanout net.
//
// Unlike timing_driven_route_connection_from_route_tree(), only part of the route tree
// which is spatially close to the sink is added to the heap.
// Returns a  tuple of <found_path?, retry_with_full_bb?, cheapest> */
std::tuple<bool, bool, t_heap> ParallelConnectionRouter::timing_driven_route_connection_from_route_tree_high_fanout(
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
        return std::make_tuple(false, false, t_heap());
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
        VTR_LOG_WARN("No routing path found in high-fanout mode for net connection (to sink_rr %d), retrying with full route tree\n", sink_node);

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
        return std::make_tuple(false, retry_with_full_bb, t_heap());
    }

    t_heap out;
    out.index = sink_node;
    out.set_prev_edge(rr_node_route_inf_[sink_node].prev_edge);
    out.path_data = nullptr;
    heap_.empty_heap();

    return std::make_tuple(true, retry_with_full_bb, out);
}

static inline bool post_target_prune_node(float new_total_cost,
                                          float new_back_cost,
                                          float best_back_cost_to_target,
                                          const t_conn_cost_params& params) {
    // Divide out the astar_fac, then multiply to get determinism
    // This is a correction factor to the forward cost to make the total
    // cost an under-estimate.
    // TODO: Should investigate creating a heuristic function that is
    //       gaurenteed to be an under-estimate.
    // NOTE: Found experimentally that using the original heuristic to order
    //       the nodes in the queue and then post-target pruning based on the
    //       under-estimating heuristic has better runtime.
    float expected_cost = new_total_cost - new_back_cost;
    float new_expected_cost = expected_cost;
    // h1 = (h - offset) * fac
    // Protection for division by zero
    if (params.astar_fac > 0.001)
        // To save time, does not recompute the heuristic, just divideds out
        // the astar_fac.
        new_expected_cost /= params.astar_fac;
    new_expected_cost = new_expected_cost - params.post_target_prune_offset;
    // Max function to prevent the heuristic from going negative
    new_expected_cost = std::max(0.f, new_expected_cost);
    new_expected_cost *= params.post_target_prune_fac;
    if ((new_back_cost + new_expected_cost) > best_back_cost_to_target)
        return true;
    // NOTE: we do NOT check for equality here. Equality does not matter for
    //       determinism when draining the queues (may just lead to a bit more work).
    return false;
}

// TODO: Once we have a heap node struct, clean this up!
static inline bool prune_node(RRNodeId inode,
                              float new_total_cost,
                              float new_back_cost,
                              RREdgeId new_prev_edge,
                              RRNodeId target_node,
                              vtr::vector<RRNodeId, t_rr_node_route_inf>& rr_node_route_inf_,
                              const t_conn_cost_params& params) {
    // Post-target pruning: After the target is reached the first time, should
    // use the heuristic to help drain the queues.
    if (inode != target_node) {
        t_rr_node_route_inf* target_route_inf = &rr_node_route_inf_[target_node];
        float best_back_cost_to_target = target_route_inf->backward_path_cost;
        if (post_target_prune_node(new_total_cost, new_back_cost, best_back_cost_to_target, params))
            return true;
    }

    // Backwards Pruning
    // NOTE: When going to the target, we only want to prune on the truth.
    //       The queues handle using the heuristic to explore nodes faster.
    t_rr_node_route_inf* route_inf = &rr_node_route_inf_[inode];
    float best_back_cost = route_inf->backward_path_cost;
    if (new_back_cost > best_back_cost)
        return true;
    // In the case of a tie, need to be picky about whether to prune or not in
    // order to get determinism.
    // FIXME: This may not be thread safe. If the best node changes while this
    //        function is being called, we may have the new_back_cost and best
    //        prev_edge's being from different heap nodes!
    // TODO: Move this to within the lock (the rest can stay for performance).
    if (new_back_cost == best_back_cost) {
#ifndef NON_DETERMINISTIC_PRUNING
        // With deterministic pruning, cannot always prune on ties.
        // In the case of a true tie, just prune, no need to explore neightbors
        RREdgeId best_prev_edge = route_inf->prev_edge;
        if (new_prev_edge == best_prev_edge)
            return true;
        // When it comes to invalid edge IDs, in the case of a tied back cost,
        // always try to keep the invalid edge ID (likely the start node).
        // TODO: Verify this.
        // If the best previous edge is invalid, prune
        if (!best_prev_edge.is_valid())
            return true;
        // If the new previous edge is invalid (assuming the best is not), accept
        if (!new_prev_edge.is_valid())
            return false;
        // Finally, if this node is not coming from a preferred edge, prune
        // Deterministic version prefers a given EdgeID, so a unique path is returned since,
        // in the case of a tie, a determinstic path wins.
        // Is first preferred over second?
        auto is_preferred_edge = [](RREdgeId first, RREdgeId second) {
            return first < second;
        };
        if (!is_preferred_edge(new_prev_edge, best_prev_edge))
            return true;
#else
        std::ignore = new_prev_edge;
        // When we do not care about determinism, always prune on equality.
        return true;
#endif
    }

    // If all above passes, do not prune.
    return false;
}

static inline bool should_not_explore_neighbors(RRNodeId inode,
                                float new_total_cost,
                                float new_back_cost,
                                RRNodeId target_node,
                                vtr::vector<RRNodeId, t_rr_node_route_inf>& rr_node_route_inf_,
                                const t_conn_cost_params& params) {
#ifndef NON_DETERMINISTIC_PRUNING
    // For deterministic pruning, cannot enforce anything on the total cost since
    // traversal order is not gaurenteed. However, since total cost is used as a
    // "key" to signify that this node is the last node that was pushed, we can
    // just check for equality. There is a chance this may cause some duplicates
    // for the deterministic case, but thats ok they will be handled.
    // TODO: Maybe consider having the non-deterministic version do this too.
    if (new_total_cost != rr_node_route_inf_[inode].path_cost)
        return true;
#else
    // For non-deterministic pruning, can greadily just ignore nodes with higher
    // total cost.
    if (new_total_cost > rr_node_route_inf_[inode].path_cost)
        return true;
#endif
    // Perform post-target pruning. If this is not done, there is a chance that
    // several duplicates of a node is in the queue that will never reach the
    // target better than what we found and they will explore all of their
    // neighbors which is not good. This is done before obtaining the lock to
    // prevent lock contention where possible.
    if (inode != target_node) {
        float best_back_cost_to_target = rr_node_route_inf_[target_node].backward_path_cost;
        if (post_target_prune_node(new_total_cost, new_back_cost, best_back_cost_to_target, params))
            return true;
    }
    return false;
}

//Finds a path to sink_node, starting from the elements currently in the heap.
//
// This is the core maze routing routine.
//
// Returns either the last element of the path, or nullptr if no path is found
void ParallelConnectionRouter::timing_driven_route_connection_from_heap(RRNodeId sink_node,
                                                                         const t_conn_cost_params& cost_params,
                                                                         const t_bb& bounding_box) {
    this->sink_node_ = &sink_node;
    this->cost_params_ = const_cast<t_conn_cost_params*>(&cost_params);
    this->bounding_box_ = const_cast<t_bb*>(&bounding_box);

    // Start measuring time before the barrier
    std::chrono::steady_clock::time_point begin_time = std::chrono::steady_clock::now();

    thread_barrier_.wait();
    this->timing_driven_route_connection_from_heap_thread_func(*this->sink_node_, *this->cost_params_, *this->bounding_box_, 0);
    thread_barrier_.wait();

    // Collect the number of heap pushes and pops
    router_stats_->heap_pushes += heap_.getNumPushes();
    router_stats_->heap_pops += heap_.getNumPops();
    // Reset the heap for the next connection
    heap_.reset();

    // Stop measuring time after the barrier and the heap is reset.
    std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();
    this->sssp_total_time += std::chrono::duration_cast<std::chrono::microseconds>(end_time - begin_time);

    // if (router_debug_) {
    //     //Update known path costs for nodes pushed but not popped, useful for debugging
    //     empty_heap_annotating_node_route_inf();
    // }
}

void ParallelConnectionRouter::timing_driven_route_connection_from_heap_sub_thread_wrapper(const size_t thread_idx) {
    thread_barrier_.init();
    while (true) {
        thread_barrier_.wait();
        if (is_router_destroying_ == true) {
            return;
        } else {
            timing_driven_route_connection_from_heap_thread_func(*this->sink_node_, *this->cost_params_, *this->bounding_box_, thread_idx);
        }
        thread_barrier_.wait();
    }
}

void ParallelConnectionRouter::timing_driven_route_connection_from_heap_thread_func(RRNodeId sink_node,
                                                                         const t_conn_cost_params& cost_params,
                                                                         const t_bb& bounding_box,
                                                                         const size_t thread_idx) {
    // cheapest t_heap in current route tree to be expanded on
    float new_total_cost;
    RRNodeId inode;
#ifdef PROFILE_HEAP_OCCUPANCY
    unsigned count = 0;
    if (thread_idx == 0) {
        heap_occ_profile_ << size_t(sink_node) << "\n";
    }
#endif
    // While the heap is not empty do
    while (heap_.try_pop(new_total_cost, inode)) {
#ifdef PROFILE_HEAP_OCCUPANCY
        if (thread_idx == 0) {
            if (count % (1000 / mq_num_threads) == 0) {
                heap_occ_profile_ << count << " " << heap_.getHeapOccupancy() << "\n";
            }
            count ++;
        }
#endif
        // update_router_stats(router_stats_,
        //                     false,
        //                     cheapest->index,
        //                     rr_graph_);

        // Should we explore the neighbors of this node?

        if (inode == sink_node) {
            heap_.setMinPrio(new_total_cost);
            continue;
        }

        if (should_not_explore_neighbors(inode, new_total_cost, rr_node_route_inf_[inode].backward_path_cost, sink_node, rr_node_route_inf_, cost_params)) {
            continue;
        }

        obtainSpinLock(inode);

        node_t cheapest;
        cheapest.backward_path_cost = rr_node_route_inf_[inode].backward_path_cost;
        cheapest.R_upstream = rr_node_route_inf_[inode].R_upstream;
        cheapest.prev_edge = rr_node_route_inf_[inode].prev_edge;

        releaseLock(inode);

        // Double check now just to be sure that we should still explore neighbors
        // NOTE: A good question is what happened to the uniqueness pruning. The idea
        //       is that at this point it does not matter. Basically any duplicates
        //       will act like they were the last one pushed in. This may create some
        //       duplicates, but it is a simple way of handling this situation.
        //       It may be worth investigating a better way to do this in the future.
        // TODO: This is still doing post-target pruning. May want to investigate
        //       if this is worth doing.
        // TODO: should try testing without the pruning below and see if anything changes.
        if (should_not_explore_neighbors(inode, new_total_cost, cheapest.backward_path_cost, sink_node, rr_node_route_inf_, cost_params)) {
            continue;
        }

        // Adding nodes to heap
        timing_driven_expand_neighbours(cheapest, inode, cost_params, bounding_box, sink_node, thread_idx);

    }
}

// Find shortest paths from specified route tree to all nodes in the RR graph
vtr::vector<RRNodeId, t_heap> ParallelConnectionRouter::timing_driven_find_all_shortest_paths_from_route_tree(
    const RouteTreeNode& rt_root,
    const t_conn_cost_params& cost_params,
    const t_bb& bounding_box,
    RouterStats& router_stats,
    const ConnectionParameters& conn_params) {

    (void)rt_root;
    (void)cost_params;
    (void)bounding_box;
    (void)router_stats;
    (void)conn_params;
    VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "SSMDSP not yet implemented (nor is the focus of this project). Not expected to be called.");
}

void ParallelConnectionRouter::timing_driven_expand_neighbours(const node_t& current,
                                                             RRNodeId from_node,
                                                             const t_conn_cost_params& cost_params,
                                                             const t_bb& bounding_box,
                                                             RRNodeId target_node,
                                                             size_t thread_idx) {
    /* Puts all the rr_nodes adjacent to current on the heap. */

    t_bb target_bb;
    if (target_node != RRNodeId::INVALID()) {
        target_bb.xmin = rr_graph_->node_xlow(RRNodeId(target_node));
        target_bb.ymin = rr_graph_->node_ylow(RRNodeId(target_node));
        target_bb.xmax = rr_graph_->node_xhigh(RRNodeId(target_node));
        target_bb.ymax = rr_graph_->node_yhigh(RRNodeId(target_node));
        target_bb.layer_min = rr_graph_->node_layer(RRNodeId(target_node));
        target_bb.layer_max = rr_graph_->node_layer(RRNodeId(target_node));
    }

    // For each node associated with the current heap element, expand all of it's neighbors
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
                                       target_bb,
                                       thread_idx);
    }
}

// Conditionally adds to_node to the router heap (via path from from_node via from_edge).
// RR nodes outside the expanded bounding box specified in bounding_box are not added
// to the heap.
void ParallelConnectionRouter::timing_driven_expand_neighbour(const node_t& current,
                                                            RRNodeId from_node,
                                                            RREdgeId from_edge,
                                                            RRNodeId to_node,
                                                            const t_conn_cost_params& cost_params,
                                                            const t_bb& bounding_box,
                                                            RRNodeId target_node,
                                                            const t_bb& target_bb,
                                                            size_t thread_idx) {
    // VTR_ASSERT(bounding_box.layer_max < g_vpr_ctx.device().grid.get_num_layers());

    // BB-pruning
    // Disable BB-pruning if RCV is enabled, as this can make it harder for circuits with high negative hold slack to resolve this
    // TODO: Only disable pruning if the net has negative hold slack, maybe go off budgets
    if (!inside_bb(to_node, bounding_box)) {
        // VTR_LOGV_DEBUG(router_debug_,
        //                "      Pruned expansion of node %d edge %zu -> %d"
        //                " (to node location %d,%d,%d x %d,%d,%d outside of expanded"
        //                " net bounding box %d,%d,%d x %d,%d,%d)\n",
        //                from_node, size_t(from_edge), size_t(to_node),
        //                rr_graph_->node_xlow(to_node), rr_graph_->node_ylow(to_node), rr_graph_->node_layer(to_node),
        //                rr_graph_->node_xhigh(to_node), rr_graph_->node_yhigh(to_node), rr_graph_->node_layer(to_node),
        //                bounding_box.xmin, bounding_box.ymin, bounding_box.layer_min,
        //                bounding_box.xmax, bounding_box.ymax, bounding_box.layer_max);
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
                // VTR_LOGV_DEBUG(router_debug_,
                //                "      Pruned expansion of node %d edge %zu -> %d"
                //                " (to node is IPIN at %d,%d,%d x %d,%d,%d which does not"
                //                " lead to target block %d,%d,%d x %d,%d,%d)\n",
                //                from_node, size_t(from_edge), size_t(to_node),
                //                to_xlow, to_ylow, to_layer,
                //                to_xhigh, to_yhigh, to_layer,
                //                target_bb.xmin, target_bb.ymin, target_bb.layer_min,
                //                target_bb.xmax, target_bb.ymax, target_bb.layer_max);
                return;
            }
        }
    }

    // VTR_LOGV_DEBUG(router_debug_, "      Expanding node %d edge %zu -> %d\n",
    //                from_node, size_t(from_edge), size_t(to_node));

    timing_driven_add_to_heap(cost_params,
                              current,
                              from_node,
                              to_node,
                              from_edge,
                              target_node,
                              thread_idx);
}

// Add to_node to the heap, and also add any nodes which are connected by non-configurable edges
void ParallelConnectionRouter::timing_driven_add_to_heap(const t_conn_cost_params& cost_params,
                                                       const node_t& current,
                                                       RRNodeId from_node,
                                                       RRNodeId to_node,
                                                       const RREdgeId from_edge,
                                                       RRNodeId target_node,
                                                       size_t thread_idx) {
    // const auto& device_ctx = g_vpr_ctx.device();
    // Initialized to current
    node_t next;
    next.total_cost = std::numeric_limits<float>::infinity(); // Not used directly
    next.backward_path_cost = current.backward_path_cost;
    next.R_upstream = current.R_upstream;
    next.prev_edge = from_edge;

    evaluate_timing_driven_node_costs(&next,
                                      cost_params,
                                      from_node,
                                      to_node,
                                      from_edge,
                                      target_node);

    float new_total_cost = next.total_cost;
    float new_back_cost = next.backward_path_cost;

    if (prune_node(to_node, new_total_cost, new_back_cost, from_edge, target_node, rr_node_route_inf_, cost_params))
        return;

    obtainSpinLock(to_node);

    if (prune_node(to_node, new_total_cost, new_back_cost, from_edge, target_node, rr_node_route_inf_, cost_params)) {
        releaseLock(to_node);
        return;
    }

    update_cheapest(next, to_node, thread_idx);

    releaseLock(to_node);

    heap_.add_to_heap(next.total_cost, to_node);

    // update_router_stats(router_stats_,
    //                     true,
    //                     to_node,
    //                     rr_graph_);
}

// #ifdef VTR_ASSERT_SAFE_ENABLED

// //Returns true if both nodes are part of the same non-configurable edge set
// static bool same_non_config_node_set(RRNodeId from_node, RRNodeId to_node) {
//     auto& device_ctx = g_vpr_ctx.device();

//     auto from_itr = device_ctx.rr_node_to_non_config_node_set.find(from_node);
//     auto to_itr = device_ctx.rr_node_to_non_config_node_set.find(to_node);

//     if (from_itr == device_ctx.rr_node_to_non_config_node_set.end()
//         || to_itr == device_ctx.rr_node_to_non_config_node_set.end()) {
//         return false; //Not part of a non-config node set
//     }

//     return from_itr->second == to_itr->second; //Check for same non-config set IDs
// }

// #endif

// Empty the route tree set node, use this after each net is routed
void ParallelConnectionRouter::empty_rcv_route_tree_set() {
    // rcv_path_manager.empty_route_tree_nodes();
    // Do nothing.
}

// Enable or disable RCV
void ParallelConnectionRouter::set_rcv_enabled(bool enable) {
    // rcv_path_manager.set_enabled(enable);
    (void)enable;
    // Do nothing.
}

//Calculates the cost of reaching to_node
void ParallelConnectionRouter::evaluate_timing_driven_node_costs(node_t* to,
                                                               const t_conn_cost_params& cost_params,
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
// #ifdef VTR_ASSERT_SAFE_ENABLED
//         VTR_ASSERT_SAFE_MSG(same_non_config_node_set(from_node, to_node),
//                             "Non-configurably connected edges should be part of the same node set");
// #endif

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

    // const auto& device_ctx = g_vpr_ctx.device();
    //Update total cost
    float expected_cost = router_lookahead_.get_expected_cost(to_node,
                                                              target_node,
                                                              cost_params,
                                                              to->R_upstream);
    total_cost += to->backward_path_cost + cost_params.astar_fac * expected_cost;

    // if (rcv_path_manager.is_enabled() && to->path_data != nullptr) {
    //     to->path_data->backward_delay += cost_params.criticality * Tdel;
    //     to->path_data->backward_cong += (1. - cost_params.criticality) * get_rr_cong_cost(to_node, cost_params.pres_fac);

    //     total_cost = compute_node_cost_using_rcv(cost_params, to_node, target_node, to->path_data->backward_delay, to->path_data->backward_cong, to->R_upstream);
    // } else {
    //     const auto& device_ctx = g_vpr_ctx.device();
    //     //Update total cost
    //     float expected_cost = router_lookahead_.get_expected_cost(to_node,
    //                                                               target_node,
    //                                                               cost_params,
    //                                                               to->R_upstream);
    //     VTR_LOGV_DEBUG(router_debug_ && !std::isfinite(expected_cost),
    //                    "        Lookahead from %s (%s) to %s (%s) is non-finite, expected_cost = %f, to->R_upstream = %f\n",
    //                    rr_node_arch_name(to_node, is_flat_).c_str(),
    //                    describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, to_node, is_flat_).c_str(),
    //                    rr_node_arch_name(target_node, is_flat_).c_str(),
    //                    describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, target_node, is_flat_).c_str(),
    //                    expected_cost, to->R_upstream);
    //     total_cost += to->backward_path_cost + cost_params.astar_fac * expected_cost;
    // }

    to->total_cost = total_cost;
}

void ParallelConnectionRouter::empty_heap_annotating_node_route_inf() {
    VTR_LOG_ERROR("ParallelConnectionRouter::empty_heap_annotating_node_route_inf is not supported");
    VTR_ASSERT(0);
    //Pop any remaining nodes in the heap and annotate their costs
    //
    //Useful for visualizing router expansion in graphics, as it shows
    //the cost of all nodes considered by the router (e.g. nodes never
    //expanded, such as parts of the initial route tree far from the
    //target).
    // pq_node_t tmp;
    // while (heap_.try_pop(tmp)) {
    //     rr_node_route_inf_[tmp.index].path_cost = tmp.cost;
    //     rr_node_route_inf_[tmp.index].backward_path_cost = tmp.backward_path_cost;
    //     // Push back serially at this point
    //     modified_rr_node_inf_[modified_rr_node_inf_.size()-1].push_back(tmp.index);
    // }
}

//Adds the route tree rooted at rt_node to the heap, preparing it to be
//used as branch-points for further routing.
void ParallelConnectionRouter::add_route_tree_to_heap(
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
        if (target_node.is_valid() && !has_path_to_sink(rr_nodes_, rr_graph_, RRNodeId(rt_node.inode), RRNodeId(target_node), only_opin_inter_layer)) {
            return;
        }
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
void ParallelConnectionRouter::add_route_tree_node_to_heap(
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

    // if (!rcv_path_manager.is_enabled()) {
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


        if (prune_node(inode, tot_cost, backward_path_cost, RREdgeId::INVALID(), target_node, rr_node_route_inf_, cost_params))
            return ;
        add_to_mod_list(inode, 0/*main thread*/);
        rr_node_route_inf_[inode].path_cost = tot_cost;
        rr_node_route_inf_[inode].prev_edge = RREdgeId::INVALID();
        rr_node_route_inf_[inode].backward_path_cost = backward_path_cost;
        rr_node_route_inf_[inode].R_upstream = R_upstream;
        heap_.push_back(tot_cost, inode);

    // } else {
    //     float expected_total_cost = compute_node_cost_using_rcv(cost_params, inode, target_node, rt_node.Tdel, 0, R_upstream);

    //     push_back_node_with_info(&heap_, inode, expected_total_cost,
    //                              backward_path_cost, R_upstream, rt_node.Tdel, &rcv_path_manager);
    // }

//     update_router_stats(router_stats_,
//                         true,
//                         inode,
//                         rr_graph_);

// #ifdef VTR_ENABLE_DEBUG_LOGGING
//     router_stats_->rt_node_pushes[rr_graph_->node_type(inode)]++;
// #endif
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

t_bb ParallelConnectionRouter::add_high_fanout_route_tree_to_heap(
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

    int nodes_added = 0;
    int chan_nodes_added = 0;

    t_bb highfanout_bb;
    highfanout_bb.xmin = rr_graph_->node_xlow(target_node);
    highfanout_bb.xmax = rr_graph_->node_xhigh(target_node);
    highfanout_bb.ymin = rr_graph_->node_ylow(target_node);
    highfanout_bb.ymax = rr_graph_->node_yhigh(target_node);
    highfanout_bb.layer_min = rr_graph_->node_layer(target_node);
    highfanout_bb.layer_max = rr_graph_->node_layer(target_node);

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
                if (!rt_node.re_expand) // Some nodes (like IPINs) shouldn't be re-expanded
                    continue;
                RRNodeId rr_node_to_add = rt_node.inode;

                if (is_flat_) {
                    if (!relevant_node_to_target(rr_graph_, rr_node_to_add, target_node))
                        continue;
                }

                /* In case of the parallel router, we may be dealing with a virtual net
                 * so prune the nodes from the HF lookup against the bounding box just in case */
                if (!inside_bb(rr_node_to_add, net_bounding_box))
                    continue;

                if (!has_path_to_sink(rr_nodes_, rr_graph_, RRNodeId(rt_node.inode), target_node, only_opin_inter_layer)) {
                    continue;
                }
                // Put the node onto the heap
                add_route_tree_node_to_heap(rt_node, target_node, cost_params, net_bounding_box);

                // Expand HF BB to include the node (clip by original BB)
                expand_highfanout_bounding_box(highfanout_bb, net_bounding_box, rr_node_to_add, rr_graph_);

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

    if (nodes_added == 0) { //If the target bin, and it's surrounding bins were empty, just add the full route tree
        add_route_tree_to_heap(rt_root, target_node, cost_params, net_bounding_box);
        return net_bounding_box;
    } else {
        //We found nearby routing, replace original bounding box to be localized around that routing
        adjust_highfanout_bounding_box(highfanout_bb, net_bounding_box);
        return highfanout_bb;
    }
}

static inline bool has_path_to_sink(const t_rr_graph_view& rr_nodes,
                                    const RRGraphView* rr_graph,
                                    RRNodeId from_node,
                                    RRNodeId sink_node,
                                    bool is_inter_layer_opin_connection) {
    int sink_layer = rr_graph->node_layer(sink_node);

    if (rr_graph->node_layer(from_node) == sink_layer || rr_graph->node_type(from_node) == SOURCE || !is_inter_layer_opin_connection) {
        return true;
    } else if (rr_graph->node_type(from_node) == CHANX || rr_graph->node_type(from_node) == CHANY || rr_graph->node_type(from_node) == IPIN) {
        return false;
    } else {
        VTR_ASSERT(rr_graph->node_type(from_node) == OPIN && is_inter_layer_opin_connection);
        auto edges = rr_nodes.edge_range(from_node);

        for (RREdgeId from_edge : edges) {
            RRNodeId to_node = rr_nodes.edge_sink_node(from_edge);
            if (rr_graph->node_layer(to_node) == sink_layer) {
                return true;
            }
        }
        return false;
    }
}

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

#ifdef VTR_ENABLE_DEBUG_LOGGING
static inline void update_router_stats(RouterStats* router_stats,
                                       bool is_push,
                                       RRNodeId rr_node_id,
                                       const RRGraphView* rr_graph) {
#else
static inline void update_router_stats(RouterStats* router_stats,
                                       bool is_push,
                                       RRNodeId /*rr_node_id*/,
                                       const RRGraphView* /*rr_graph*/) {
#endif
    if (is_push) {
        router_stats->heap_pushes++;
    } else {
        router_stats->heap_pops++;
    }

#ifdef VTR_ENABLE_DEBUG_LOGGING
    const auto& device_ctx = g_vpr_ctx.device();
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
#endif
}

