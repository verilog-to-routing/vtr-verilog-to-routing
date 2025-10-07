#include "parallel_connection_router.h"

#include <algorithm>
#include "d_ary_heap.h"
#include "route_tree.h"
#include "rr_graph_fwd.h"

/** Post-target pruning: Prune a given node (do not explore it) if the cost of
 * the best possible path from the source, through the node, to the target is
 * higher than the cost of the best path found to the target so far. Cited from
 * the FPT'24 conference paper (more details can also be found there). */
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

    // NOTE: in the check below, the multiplication factor is used to account for floating point errors.
    if ((new_back_cost + new_expected_cost) * 0.999f > best_back_cost_to_target)
        return true;
    // NOTE: we do NOT check for equality here. Equality does not matter for
    //       determinism when draining the queues (may just lead to a bit more work).
    return false;
}

/** Pre-push pruning: when iterating over the neighbors of u, this function
 * determines whether a path through u to its neighbor node v has a better
 * backward cost than the best path to v found so far (breaking ties if needed).
 * Cited from the FPT'24 conference paper (more details can also be found there).
 */
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

/** Post-pop pruning: After node u is popped from the queue, this function
 * decides whether to explore the neighbors of u or to prune. Initially, it
 * performs Post-Target Pruning based on the stopping criterion. Then, the
 * current total estimated cost of the path through node u (f_u) is compared
 * to the best total cost so far (most recently pushed) for that node and,
 * if the two are different, the node u is pruned. During the wave expansion,
 * u may be pushed to the queue multiple times. For example, node u may be
 * pushed to the queue and then, before u is popped from the queue, a better
 * path to u may be found and pushed to the queue. Here we are using f_u as
 * an optimistic identifier to check if the pair (u, f_u) is the most recently
 * pushed element for node u. This reduces redundant work.
 * Cited from the FPT'24 conference paper (more details can also be found there).
 */
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

template<typename Heap>
void ParallelConnectionRouter<Heap>::timing_driven_find_single_shortest_path_from_heap(RRNodeId sink_node,
                                                                                       const t_conn_cost_params& cost_params,
                                                                                       const t_bb& bounding_box,
                                                                                       const t_bb& target_bb) {
    // Assign the thread task function parameters to atomic variables
    this->sink_node_ = &sink_node;
    this->cost_params_ = const_cast<t_conn_cost_params*>(&cost_params);
    this->bounding_box_ = const_cast<t_bb*>(&bounding_box);
    this->target_bb_ = const_cast<t_bb*>(&target_bb);

    // Synchronize at the barrier before executing a new thread task
    this->thread_barrier_.wait();

    // Main thread executes a new thread task (helper threads are doing the same in the background)
    this->timing_driven_find_single_shortest_path_from_heap_thread_func(*this->sink_node_,
                                                                        *this->cost_params_,
                                                                        *this->bounding_box_,
                                                                        *this->target_bb_, 0);

    // Synchronize at the barrier before resetting the heap
    this->thread_barrier_.wait();

    // Collect the number of heap pushes and pops
    this->router_stats_->heap_pushes += this->heap_.get_num_pushes();
    this->router_stats_->heap_pops += this->heap_.get_num_pops();

    // Reset the heap for the next connection
    this->heap_.reset();
}

template<typename Heap>
void ParallelConnectionRouter<Heap>::timing_driven_find_single_shortest_path_from_heap_sub_thread_wrapper(const size_t thread_idx) {
    this->thread_barrier_.init();
    while (true) {
        this->thread_barrier_.wait();
        if (this->is_router_destroying_ == true) {
            return;
        } else {
            timing_driven_find_single_shortest_path_from_heap_thread_func(*this->sink_node_,
                                                                          *this->cost_params_,
                                                                          *this->bounding_box_,
                                                                          *this->target_bb_,
                                                                          thread_idx);
        }
        this->thread_barrier_.wait();
    }
}

template<typename Heap>
void ParallelConnectionRouter<Heap>::timing_driven_find_single_shortest_path_from_heap_thread_func(RRNodeId sink_node,
                                                                                                   const t_conn_cost_params& cost_params,
                                                                                                   const t_bb& bounding_box,
                                                                                                   const t_bb& target_bb,
                                                                                                   const size_t thread_idx) {
    HeapNode cheapest;
    while (this->heap_.try_pop(cheapest)) {
        // Pop a new inode with the cheapest total cost in current route tree to be expanded on
        const auto& [new_total_cost, inode] = cheapest;

        // Check if we should explore the neighbors of this node
        if (should_not_explore_neighbors(inode, new_total_cost, this->rr_node_route_inf_[inode].backward_path_cost, sink_node, this->rr_node_route_inf_, cost_params)) {
            continue;
        }

        // Get the current RR node info within a critical section to prevent data races
        obtainSpinLock(inode);

        RTExploredNode current;
        current.index = inode;
        current.backward_path_cost = this->rr_node_route_inf_[inode].backward_path_cost;
        current.prev_edge = this->rr_node_route_inf_[inode].prev_edge;
        current.R_upstream = this->rr_node_route_inf_[inode].R_upstream;

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
        if (should_not_explore_neighbors(inode, new_total_cost, current.backward_path_cost, sink_node, this->rr_node_route_inf_, cost_params)) {
            continue;
        }

        // Adding nodes to heap
        timing_driven_expand_neighbours(current, cost_params, bounding_box, sink_node, target_bb, thread_idx);
    }
}

template<typename Heap>
void ParallelConnectionRouter<Heap>::timing_driven_expand_neighbours(const RTExploredNode& current,
                                                                     const t_conn_cost_params& cost_params,
                                                                     const t_bb& bounding_box,
                                                                     RRNodeId target_node,
                                                                     const t_bb& target_bb,
                                                                     size_t thread_idx) {
    /* Puts all the rr_nodes adjacent to current on the heap. */

    // For each node associated with the current heap element, expand all of it's neighbors
    auto edges = this->rr_nodes_.edge_range(current.index);

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
        RRNodeId to_node = this->rr_nodes_.edge_sink_node(from_edge);
        this->rr_nodes_.prefetch_node(to_node);

        int switch_idx = this->rr_nodes_.edge_switch(from_edge);
        VTR_PREFETCH(&this->rr_switch_inf_[switch_idx], 0, 0);
    }

    for (RREdgeId from_edge : edges) {
        RRNodeId to_node = this->rr_nodes_.edge_sink_node(from_edge);
        timing_driven_expand_neighbour(current,
                                       from_edge,
                                       to_node,
                                       cost_params,
                                       bounding_box,
                                       target_node,
                                       target_bb,
                                       thread_idx);
    }
}

template<typename Heap>
void ParallelConnectionRouter<Heap>::timing_driven_expand_neighbour(const RTExploredNode& current,
                                                                    RREdgeId from_edge,
                                                                    RRNodeId to_node,
                                                                    const t_conn_cost_params& cost_params,
                                                                    const t_bb& bounding_box,
                                                                    RRNodeId target_node,
                                                                    const t_bb& target_bb,
                                                                    size_t thread_idx) {
    // BB-pruning
    // Disable BB-pruning if RCV is enabled, as this can make it harder for circuits with high negative hold slack to resolve this
    // TODO: Only disable pruning if the net has negative hold slack, maybe go off budgets
    if (!inside_bb(to_node, bounding_box)) {
        // Note: Logging are disabled for parallel connection router
        return; /* Node is outside (expanded) bounding box. */
    }

    /* Prune away IPINs that lead to blocks other than the target one.  Avoids  *
     * the issue of how to cost them properly so they don't get expanded before *
     * more promising routes, but makes route-through (via CLBs) impossible.   *
     * Change this if you want to investigate route-throughs.                   */
    if (target_node != RRNodeId::INVALID()) {
        e_rr_type to_type = this->rr_graph_->node_type(to_node);
        if (to_type == e_rr_type::IPIN) {
            // Check if this IPIN leads to the target block
            // IPIN's of the target block should be contained within it's bounding box
            int to_xlow = this->rr_graph_->node_xlow(to_node);
            int to_ylow = this->rr_graph_->node_ylow(to_node);
            int to_layer = this->rr_graph_->node_layer_low(to_node);
            int to_xhigh = this->rr_graph_->node_xhigh(to_node);
            int to_yhigh = this->rr_graph_->node_yhigh(to_node);
            if (to_xlow < target_bb.xmin
                || to_ylow < target_bb.ymin
                || to_xhigh > target_bb.xmax
                || to_yhigh > target_bb.ymax
                || to_layer < target_bb.layer_min
                || to_layer > target_bb.layer_max) {
                // Note: Logging are disabled for parallel connection router
                return;
            }
        }
    }
    // Note: Logging are disabled for parallel connection router

    timing_driven_add_to_heap(cost_params,
                              current,
                              to_node,
                              from_edge,
                              target_node,
                              thread_idx);
}

template<typename Heap>
void ParallelConnectionRouter<Heap>::timing_driven_add_to_heap(const t_conn_cost_params& cost_params,
                                                               const RTExploredNode& current,
                                                               RRNodeId to_node,
                                                               const RREdgeId from_edge,
                                                               RRNodeId target_node,
                                                               size_t thread_idx) {
    const RRNodeId& from_node = current.index;

    // Initialize the neighbor RTExploredNode
    RTExploredNode next;
    next.R_upstream = current.R_upstream;
    next.index = to_node;
    next.prev_edge = from_edge;
    next.total_cost = std::numeric_limits<float>::infinity(); // Not used directly
    next.backward_path_cost = current.backward_path_cost;

    this->evaluate_timing_driven_node_costs(&next, cost_params, from_node, target_node);

    float new_total_cost = next.total_cost;
    float new_back_cost = next.backward_path_cost;

    // To further reduce lock contention, we add a cheap read-only check before acquiring the lock, motivated by Shun et al.
    if (prune_node(to_node, new_total_cost, new_back_cost, from_edge, target_node, this->rr_node_route_inf_, cost_params)) {
        return;
    }

    obtainSpinLock(to_node);

    if (prune_node(to_node, new_total_cost, new_back_cost, from_edge, target_node, this->rr_node_route_inf_, cost_params)) {
        releaseLock(to_node);
        return;
    }

    update_cheapest(next, thread_idx);

    releaseLock(to_node);

    if (to_node == target_node) {
#ifdef MQ_IO_ENABLE_CLEAR_FOR_POP
        if (multi_queue_direct_draining_) {
            this->heap_.set_min_priority_for_pop(new_total_cost);
        }
#endif
        return;
    }
    this->heap_.add_to_heap({new_total_cost, to_node});
}

template<typename Heap>
void ParallelConnectionRouter<Heap>::add_route_tree_node_to_heap(
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

    // After budgets are loaded, calculate delay cost as described by RCV paper
    /* R. Fung, V. Betz and W. Chow, "Slack Allocation and Routing to Improve FPGA Timing While
     * Repairing Short-Path Violations," in IEEE Transactions on Computer-Aided Design of
     * Integrated Circuits and Systems, vol. 27, no. 4, pp. 686-697, April 2008.*/

    if (!this->rcv_path_manager.is_enabled()) {
        float expected_cost = this->router_lookahead_.get_expected_cost(inode, target_node, cost_params, R_upstream);
        float tot_cost = backward_path_cost + cost_params.astar_fac * std::max(0.f, expected_cost - cost_params.astar_offset);
        VTR_LOGV_DEBUG(this->router_debug_, "  Adding node %8d to heap from init route tree with cost %g (%s)\n",
                       inode,
                       tot_cost,
                       describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, inode, this->is_flat_).c_str());

        if (prune_node(inode, tot_cost, backward_path_cost, RREdgeId::INVALID(), target_node, this->rr_node_route_inf_, cost_params)) {
            return;
        }
        add_to_mod_list(inode, 0 /*main thread*/);
        this->rr_node_route_inf_[inode].path_cost = tot_cost;
        this->rr_node_route_inf_[inode].prev_edge = RREdgeId::INVALID();
        this->rr_node_route_inf_[inode].backward_path_cost = backward_path_cost;
        this->rr_node_route_inf_[inode].R_upstream = R_upstream;
        this->heap_.push_back({tot_cost, inode});
    }
    // Note: RCV is not supported by parallel connection router
}

std::unique_ptr<ConnectionRouterInterface> make_parallel_connection_router(e_heap_type heap_type,
                                                                           const DeviceGrid& grid,
                                                                           const RouterLookahead& router_lookahead,
                                                                           const t_rr_graph_storage& rr_nodes,
                                                                           const RRGraphView* rr_graph,
                                                                           const std::vector<t_rr_rc_data>& rr_rc_data,
                                                                           const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switch_inf,
                                                                           vtr::vector<RRNodeId, t_rr_node_route_inf>& rr_node_route_inf,
                                                                           bool is_flat,
                                                                           int route_verbosity,
                                                                           int multi_queue_num_threads,
                                                                           int multi_queue_num_queues,
                                                                           bool multi_queue_direct_draining) {
    switch (heap_type) {
        case e_heap_type::BINARY_HEAP:
            return std::make_unique<ParallelConnectionRouter<BinaryHeap>>(
                grid,
                router_lookahead,
                rr_nodes,
                rr_graph,
                rr_rc_data,
                rr_switch_inf,
                rr_node_route_inf,
                is_flat,
                route_verbosity,
                multi_queue_num_threads,
                multi_queue_num_queues,
                multi_queue_direct_draining);
        case e_heap_type::FOUR_ARY_HEAP:
            return std::make_unique<ParallelConnectionRouter<FourAryHeap>>(
                grid,
                router_lookahead,
                rr_nodes,
                rr_graph,
                rr_rc_data,
                rr_switch_inf,
                rr_node_route_inf,
                is_flat,
                route_verbosity,
                multi_queue_num_threads,
                multi_queue_num_queues,
                multi_queue_direct_draining);
        default:
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Unknown heap_type %d",
                            heap_type);
    }
}
