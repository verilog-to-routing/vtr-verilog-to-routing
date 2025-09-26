#include "serial_connection_router.h"

#include <algorithm>
#include "d_ary_heap.h"
#include "rr_graph_fwd.h"

/** Used to update router statistics for serial connection router */
inline void update_serial_router_stats(RouterStats* router_stats,
                                       bool is_push,
                                       RRNodeId rr_node_id,
                                       const RRGraphView* rr_graph);

template<typename Heap>
void SerialConnectionRouter<Heap>::timing_driven_find_single_shortest_path_from_heap(RRNodeId sink_node,
                                                                                     const t_conn_cost_params& cost_params,
                                                                                     const t_bb& bounding_box,
                                                                                     const t_bb& target_bb) {
    const auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    HeapNode cheapest;
    while (this->heap_.try_pop(cheapest)) {
        // Pop a new inode with the cheapest total cost in current route tree to be expanded on
        const auto& [new_total_cost, inode] = cheapest;
        update_serial_router_stats(this->router_stats_,
                                   /*is_push=*/false,
                                   inode,
                                   this->rr_graph_);

        VTR_LOGV_DEBUG(this->router_debug_, "  Popping node %d (cost: %g)\n",
                       inode, new_total_cost);

        // Have we found the target?
        if (inode == sink_node) {
            // If we're running RCV, the path will be stored in the path_data->path_rr vector
            // This is then placed into the traceback so that the correct path is returned
            // TODO: This can be eliminated by modifying the actual traceback function in route_timing
            if (this->rcv_path_manager.is_enabled()) {
                this->rcv_path_manager.insert_backwards_path_into_traceback(this->rcv_path_data[inode],
                                                                            this->rr_node_route_inf_[inode].path_cost,
                                                                            this->rr_node_route_inf_[inode].backward_path_cost,
                                                                            route_ctx);
            }
            VTR_LOGV_DEBUG(this->router_debug_, "  Found target %8d (%s)\n", inode, describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, inode, this->is_flat_).c_str());
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
}

template<typename Heap>
vtr::vector<RRNodeId, RTExploredNode> SerialConnectionRouter<Heap>::timing_driven_find_all_shortest_paths_from_route_tree(
    const RouteTreeNode& rt_root,
    const t_conn_cost_params& cost_params,
    const t_bb& bounding_box,
    RouterStats& router_stats,
    const ConnectionParameters& conn_params) {
    this->router_stats_ = &router_stats;
    this->conn_params_ = &conn_params;

    // Add the route tree to the heap with no specific target node
    RRNodeId target_node = RRNodeId::INVALID();
    this->add_route_tree_to_heap(rt_root, target_node, cost_params, bounding_box);
    this->heap_.build_heap(); // via sifting down everything

    auto res = timing_driven_find_all_shortest_paths_from_heap(cost_params, bounding_box);
    this->heap_.empty_heap();

    return res;
}

template<typename Heap>
vtr::vector<RRNodeId, RTExploredNode> SerialConnectionRouter<Heap>::timing_driven_find_all_shortest_paths_from_heap(
    const t_conn_cost_params& cost_params,
    const t_bb& bounding_box) {
    // Since there is no single *target* node this uses Dijkstra's algorithm
    // with a modified exit condition (runs until heap is empty).

    vtr::vector<RRNodeId, RTExploredNode> cheapest_paths(this->rr_nodes_.size());

    VTR_ASSERT_SAFE(this->heap_.is_valid());

    if (this->heap_.is_empty_heap()) { // No source
        VTR_LOGV_DEBUG(this->router_debug_, "  Initial heap empty (no source)\n");
    }

    // Start measuring path search time
    std::chrono::steady_clock::time_point begin_time = std::chrono::steady_clock::now();

    HeapNode cheapest;
    while (this->heap_.try_pop(cheapest)) {
        // Pop a new inode with the cheapest total cost in current route tree to be expanded on
        const auto& [new_total_cost, inode] = cheapest;
        update_serial_router_stats(this->router_stats_,
                                   /*is_push=*/false,
                                   inode,
                                   this->rr_graph_);

        VTR_LOGV_DEBUG(this->router_debug_, "  Popping node %d (cost: %g)\n",
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
            VTR_LOGV_DEBUG(this->router_debug_, "  Better cost to node %d: %g (was %g)\n", inode, new_total_cost, cheapest_paths[inode].total_cost);
            // Only the `index` and `prev_edge` fields of `cheapest_paths[inode]` are used after this function returns
            cheapest_paths[inode].index = inode;
            cheapest_paths[inode].prev_edge = this->rr_node_route_inf_[inode].prev_edge;
        } else {
            VTR_LOGV_DEBUG(this->router_debug_, "  Worse cost to node %d: %g (better %g)\n", inode, new_total_cost, cheapest_paths[inode].total_cost);
        }
    }

    // Stop measuring path search time
    std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();
    this->path_search_cumulative_time += std::chrono::duration_cast<std::chrono::microseconds>(end_time - begin_time);

    return cheapest_paths;
}

template<typename Heap>
void SerialConnectionRouter<Heap>::timing_driven_expand_cheapest(RRNodeId from_node,
                                                                 float new_total_cost,
                                                                 RRNodeId target_node,
                                                                 const t_conn_cost_params& cost_params,
                                                                 const t_bb& bounding_box,
                                                                 const t_bb& target_bb) {
    float best_total_cost = this->rr_node_route_inf_[from_node].path_cost;
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
        // it doesn't create a loop. That `this->rcv_path_manager` should store enough
        // info for us to avoid loops.
        RTExploredNode current;
        current.index = from_node;
        current.backward_path_cost = this->rr_node_route_inf_[from_node].backward_path_cost;
        current.prev_edge = this->rr_node_route_inf_[from_node].prev_edge;
        current.R_upstream = this->rr_node_route_inf_[from_node].R_upstream;

        VTR_LOGV_DEBUG(this->router_debug_, "    Better cost to %d\n", from_node);
        VTR_LOGV_DEBUG(this->router_debug_, "    New total cost: %g\n", new_total_cost);
        VTR_LOGV_DEBUG(this->router_debug_ && (current.prev_edge != RREdgeId::INVALID()),
                       "      Setting path costs for associated node %d (from %d edge %zu)\n",
                       from_node,
                       static_cast<size_t>(this->rr_graph_->edge_src_node(current.prev_edge)),
                       static_cast<size_t>(current.prev_edge));

        timing_driven_expand_neighbours(current, cost_params, bounding_box, target_node, target_bb);
    } else {
        // Post-heap prune, do not re-explore from the current/new partial path as it
        // has worse cost than the best partial path to this node found so far
        VTR_LOGV_DEBUG(this->router_debug_, "    Worse cost to %d\n", from_node);
        VTR_LOGV_DEBUG(this->router_debug_, "    Old total cost: %g\n", best_total_cost);
        VTR_LOGV_DEBUG(this->router_debug_, "    New total cost: %g\n", new_total_cost);
    }
}

template<typename Heap>
void SerialConnectionRouter<Heap>::timing_driven_expand_neighbours(const RTExploredNode& current,
                                                                   const t_conn_cost_params& cost_params,
                                                                   const t_bb& bounding_box,
                                                                   RRNodeId target_node,
                                                                   const t_bb& target_bb) {
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
                                       target_bb);
    }
}

template<typename Heap>
void SerialConnectionRouter<Heap>::timing_driven_expand_neighbour(const RTExploredNode& current,
                                                                  RREdgeId from_edge,
                                                                  RRNodeId to_node,
                                                                  const t_conn_cost_params& cost_params,
                                                                  const t_bb& bounding_box,
                                                                  RRNodeId target_node,
                                                                  const t_bb& target_bb) {
    VTR_ASSERT(bounding_box.layer_max < (int)g_vpr_ctx.device().grid.get_num_layers());

    const RRNodeId& from_node = current.index;

    // BB-pruning
    // Disable BB-pruning if RCV is enabled, as this can make it harder for circuits with high negative hold slack to resolve this
    // TODO: Only disable pruning if the net has negative hold slack, maybe go off budgets
    if (!inside_bb(to_node, bounding_box)
        && !this->rcv_path_manager.is_enabled()) {
        VTR_LOGV_DEBUG(this->router_debug_,
                       "      Pruned expansion of node %d edge %zu -> %d"
                       " (to node location %d,%d,%d x %d,%d,%d outside of expanded"
                       " net bounding box %d,%d,%d x %d,%d,%d)\n",
                       from_node, size_t(from_edge), size_t(to_node),
                       this->rr_graph_->node_xlow(to_node), this->rr_graph_->node_ylow(to_node), this->rr_graph_->node_layer_low(to_node),
                       this->rr_graph_->node_xhigh(to_node), this->rr_graph_->node_yhigh(to_node), this->rr_graph_->node_layer_low(to_node),
                       bounding_box.xmin, bounding_box.ymin, bounding_box.layer_min,
                       bounding_box.xmax, bounding_box.ymax, bounding_box.layer_max);
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
                VTR_LOGV_DEBUG(this->router_debug_,
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

    VTR_LOGV_DEBUG(this->router_debug_, "      Expanding node %d edge %zu -> %d\n",
                   from_node, size_t(from_edge), size_t(to_node));

    // Check if the node exists in the route tree when RCV is enabled
    // Other pruning methods have been disabled when RCV is on, so this method is required to prevent "loops" from being created
    bool node_exists = false;
    if (this->rcv_path_manager.is_enabled()) {
        node_exists = this->rcv_path_manager.node_exists_in_tree(this->rcv_path_data[from_node],
                                                                 to_node);
    }

    if (!node_exists || !this->rcv_path_manager.is_enabled()) {
        timing_driven_add_to_heap(cost_params,
                                  current,
                                  to_node,
                                  from_edge,
                                  target_node);
    }
}

template<typename Heap>
void SerialConnectionRouter<Heap>::timing_driven_add_to_heap(const t_conn_cost_params& cost_params,
                                                             const RTExploredNode& current,
                                                             RRNodeId to_node,
                                                             const RREdgeId from_edge,
                                                             RRNodeId target_node) {
    const auto& device_ctx = g_vpr_ctx.device();
    const RRNodeId& from_node = current.index;

    // Initialize the neighbor RTExploredNode
    RTExploredNode next;
    next.R_upstream = current.R_upstream;
    next.index = to_node;
    next.prev_edge = from_edge;
    next.total_cost = std::numeric_limits<float>::infinity(); // Not used directly
    next.backward_path_cost = current.backward_path_cost;

    // Initialize RCV data struct if needed, otherwise it's set to nullptr
    this->rcv_path_manager.alloc_path_struct(next.path_data);
    // path_data variables are initialized to current values
    if (this->rcv_path_manager.is_enabled() && this->rcv_path_data[from_node]) {
        next.path_data->backward_cong = this->rcv_path_data[from_node]->backward_cong;
        next.path_data->backward_delay = this->rcv_path_data[from_node]->backward_delay;
    }

    this->evaluate_timing_driven_node_costs(&next, cost_params, from_node, target_node);

    float best_total_cost = this->rr_node_route_inf_[to_node].path_cost;
    float best_back_cost = this->rr_node_route_inf_[to_node].backward_path_cost;

    float new_total_cost = next.total_cost;
    float new_back_cost = next.backward_path_cost;

    // We need to only expand this node if it is a better path. And we need to
    // update its `rr_node_route_inf` data as we put it into the heap; there may
    // be other (previously explored) paths to this node in the heap already,
    // but they will be pruned when we pop those heap nodes later as we'll see
    // they have inferior costs to what is in the `rr_node_route_inf` data for
    // this node. More details can be found from the FPT'24 parallel connection
    // router paper.
    //
    // When RCV is enabled, prune based on the RCV-specific total path cost (see
    // in `compute_node_cost_using_rcv` in `evaluate_timing_driven_node_costs`)
    // to allow detours to get better QoR.
    if ((!this->rcv_path_manager.is_enabled() && best_back_cost > new_back_cost) || (this->rcv_path_manager.is_enabled() && best_total_cost > new_total_cost)) {
        VTR_LOGV_DEBUG(this->router_debug_, "      Expanding to node %d (%s)\n", to_node,
                       describe_rr_node(device_ctx.rr_graph,
                                        device_ctx.grid,
                                        device_ctx.rr_indexed_data,
                                        to_node,
                                        this->is_flat_)
                           .c_str());
        VTR_LOGV_DEBUG(this->router_debug_, "        New Total Cost %g New back Cost %g\n", new_total_cost, new_back_cost);
        //Add node to the heap only if the cost via the current partial path is less than the
        //best known cost, since there is no reason for the router to expand more expensive paths.
        //
        //Pre-heap prune to keep the heap small, by not putting paths which are known to be
        //sub-optimal (at this point in time) into the heap.

        update_cheapest(next, from_node);

        this->heap_.add_to_heap({new_total_cost, to_node});
        update_serial_router_stats(this->router_stats_,
                                   /*is_push=*/true,
                                   to_node,
                                   this->rr_graph_);

    } else {
        VTR_LOGV_DEBUG(this->router_debug_, "      Didn't expand to %d (%s)\n", to_node, describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, to_node, this->is_flat_).c_str());
        VTR_LOGV_DEBUG(this->router_debug_, "        Prev Total Cost %g Prev back Cost %g \n", best_total_cost, best_back_cost);
        VTR_LOGV_DEBUG(this->router_debug_, "        New Total Cost %g New back Cost %g \n", new_total_cost, new_back_cost);
    }

    if (this->rcv_path_manager.is_enabled() && next.path_data != nullptr) {
        this->rcv_path_manager.free_path_struct(next.path_data);
    }
}

template<typename Heap>
void SerialConnectionRouter<Heap>::add_route_tree_node_to_heap(
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
    // float expected_cost = router_lookahead_.get_expected_cost(inode, target_node, cost_params, R_upstream);

    if (!this->rcv_path_manager.is_enabled()) {
        float expected_cost = this->router_lookahead_.get_expected_cost(inode, target_node, cost_params, R_upstream);
        float tot_cost = backward_path_cost + cost_params.astar_fac * std::max(0.f, expected_cost - cost_params.astar_offset);
        VTR_LOGV_DEBUG(this->router_debug_, "  Adding node %8d to heap from init route tree with cost %g (%s)\n",
                       inode,
                       tot_cost,
                       describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, inode, this->is_flat_).c_str());

        if (tot_cost > this->rr_node_route_inf_[inode].path_cost) {
            return;
        }
        add_to_mod_list(inode);
        this->rr_node_route_inf_[inode].path_cost = tot_cost;
        this->rr_node_route_inf_[inode].prev_edge = RREdgeId::INVALID();
        this->rr_node_route_inf_[inode].backward_path_cost = backward_path_cost;
        this->rr_node_route_inf_[inode].R_upstream = R_upstream;
        this->heap_.push_back({tot_cost, inode});
    } else {
        float expected_total_cost = this->compute_node_cost_using_rcv(cost_params, inode, target_node, rt_node.Tdel, 0, R_upstream);

        add_to_mod_list(inode);
        this->rr_node_route_inf_[inode].path_cost = expected_total_cost;
        this->rr_node_route_inf_[inode].prev_edge = RREdgeId::INVALID();
        this->rr_node_route_inf_[inode].backward_path_cost = backward_path_cost;
        this->rr_node_route_inf_[inode].R_upstream = R_upstream;

        this->rcv_path_manager.alloc_path_struct(this->rcv_path_data[inode]);
        this->rcv_path_data[inode]->backward_delay = rt_node.Tdel;

        this->heap_.push_back({expected_total_cost, inode});
    }

    update_serial_router_stats(this->router_stats_,
                               /*is_push=*/true,
                               inode,
                               this->rr_graph_);

    if constexpr (VTR_ENABLE_DEBUG_LOGGING_CONST_EXPR) {
        this->router_stats_->rt_node_pushes[this->rr_graph_->node_type(inode)]++;
    }
}

std::unique_ptr<ConnectionRouterInterface> make_serial_connection_router(e_heap_type heap_type,
                                                                         const DeviceGrid& grid,
                                                                         const RouterLookahead& router_lookahead,
                                                                         const t_rr_graph_storage& rr_nodes,
                                                                         const RRGraphView* rr_graph,
                                                                         const std::vector<t_rr_rc_data>& rr_rc_data,
                                                                         const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switch_inf,
                                                                         vtr::vector<RRNodeId, t_rr_node_route_inf>& rr_node_route_inf,
                                                                         bool is_flat,
                                                                         int route_verbosity) {
    switch (heap_type) {
        case e_heap_type::BINARY_HEAP:
            return std::make_unique<SerialConnectionRouter<BinaryHeap>>(
                grid,
                router_lookahead,
                rr_nodes,
                rr_graph,
                rr_rc_data,
                rr_switch_inf,
                rr_node_route_inf,
                is_flat,
                route_verbosity);
        case e_heap_type::FOUR_ARY_HEAP:
            return std::make_unique<SerialConnectionRouter<FourAryHeap>>(
                grid,
                router_lookahead,
                rr_nodes,
                rr_graph,
                rr_rc_data,
                rr_switch_inf,
                rr_node_route_inf,
                is_flat,
                route_verbosity);
        default:
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Unknown heap_type %d",
                            heap_type);
    }
}

/** This function is only used for the serial connection router since some
 * statistic variables in router_stats are not thread-safe for the parallel
 * connection router. To update router_stats (more precisely heap_pushes/pops)
 * for parallel connection router, we use the MultiQueue internal statistics
 * method instead. */
inline void update_serial_router_stats(RouterStats* router_stats,
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
        VTR_ASSERT(node_type != e_rr_type::NUM_RR_TYPES);

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
