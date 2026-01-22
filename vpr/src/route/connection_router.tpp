#pragma once

#include "connection_router.h"

#include <algorithm>
#include "describe_rr_node.h"
#include "route_common.h"
#include "rr_graph_fwd.h"
#include "vpr_utils.h"

/** Used for the flat router. The node isn't relevant to the target if
 * it is an intra-block node outside of our target block */
inline bool relevant_node_to_target(const RRGraphView* rr_graph,
                                    RRNodeId node_to_add,
                                    RRNodeId target_node);

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

    bool retry = timing_driven_route_connection_common_setup(rt_root, sink_node, cost_params, bounding_box);

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
    timing_driven_route_connection_from_heap(sink_node, cost_params, high_fanout_bb);

    if (std::isinf(rr_node_route_inf_[sink_node].path_cost)) {
        //Found no path, that may be due to an unlucky choice of existing route tree sub-set,
        //try again with the full route tree to be sure this is not an artifact of high-fanout routing
        VTR_LOGV(route_verbosity_ > 1, "No routing path found in high-fanout mode for net %zu connection (to sink_rr %d), retrying with full route tree\n", size_t(conn_params.net_id_), sink_node);

        //Reset any previously recorded node costs so timing_driven_route_connection()
        //starts over from scratch.
        reset_path_costs();
        clear_modified_rr_node_info();

        retry_with_full_bb = timing_driven_route_connection_common_setup(rt_root, sink_node, cost_params, net_bounding_box);
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

    timing_driven_route_connection_from_heap(sink_node, cost_params, bounding_box);

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
        VTR_LOGV(route_verbosity_ > 1, "No routing path for connection to sink_rr %d, leaving unrouted to retry later\n", sink_node);
        return true;
    }

    return false;
}

template<typename Heap>
void ConnectionRouter<Heap>::timing_driven_route_connection_from_heap(RRNodeId sink_node,
                                                                      const t_conn_cost_params& cost_params,
                                                                      const t_bb& bounding_box) {
    VTR_ASSERT_SAFE(heap_.is_valid());

    if (heap_.is_empty_heap()) { //No source
        VTR_LOGV_DEBUG(router_debug_, "  Initial heap empty (no source)\n");
    }

    // Get bounding box for sink node used in timing_driven_expand_neighbour
    VTR_ASSERT_SAFE(sink_node != RRNodeId::INVALID());

    t_bb target_bb;
    if (rr_graph_->node_type(sink_node) == e_rr_type::SINK) { // We need to get a bounding box for the sink's entire tile
        vtr::Rect<int> tile_bb = grid_.get_tile_bb({rr_graph_->node_xlow(sink_node),
                                                    rr_graph_->node_ylow(sink_node),
                                                    rr_graph_->node_layer_low(sink_node)});

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

    target_bb.layer_min = rr_graph_->node_layer_low(RRNodeId(sink_node));
    target_bb.layer_max = rr_graph_->node_layer_high(RRNodeId(sink_node));

    // Start measuring path search time
    std::chrono::steady_clock::time_point begin_time = std::chrono::steady_clock::now();

    timing_driven_find_single_shortest_path_from_heap(sink_node, cost_params, bounding_box, target_bb);

    // Stop measuring path search time
    std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();
    path_search_cumulative_time += std::chrono::duration_cast<std::chrono::microseconds>(end_time - begin_time);
}

#ifdef VTR_ASSERT_SAFE_ENABLED

//Returns true if both nodes are part of the same non-configurable edge set
inline bool same_non_config_node_set(RRNodeId from_node, RRNodeId to_node) {
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
    const t_conn_delay_budget* delay_budget = cost_params.delay_budget;
    // TODO: This function is not tested for is_flat == true
    VTR_ASSERT(is_flat_ != true);
    const auto [expected_delay, expected_cong] = router_lookahead_.get_expected_delay_and_cong(to_node, target_node, cost_params, R_upstream);

    float expected_total_cong = expected_cong + backwards_cong;
    float expected_total_delay = expected_delay + backwards_delay;

    //If budgets specified calculate cost as described by RCV paper:
    //    R. Fung, V. Betz and W. Chow, "Slack Allocation and Routing to Improve FPGA Timing While
    //     Repairing Short-Path Violations," in IEEE Transactions on Computer-Aided Design of
    //     Integrated Circuits and Systems, vol. 27, no. 4, pp. 686-697, April 2008.

    // Normalization constant defined in RCV paper cited above
    constexpr float NORMALIZATION_CONSTANT = 100e-12;

    float expected_total_delay_cost = expected_total_delay;
    expected_total_delay_cost += (delay_budget->short_path_criticality + cost_params.criticality) * std::max(0.f, delay_budget->target_delay - expected_total_delay);
    // expected_total_delay_cost += std::pow(std::max(0.f, expected_total_delay - delay_budget->max_delay), 2) / NORMALIZATION_CONSTANT;
    expected_total_delay_cost += std::pow(std::max(0.f, delay_budget->min_delay - expected_total_delay), 2) / NORMALIZATION_CONSTANT;
    float expected_total_cong_cost = expected_total_cong;

    float total_cost = expected_total_delay_cost + expected_total_cong_cost;

    return total_cost;
}

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
    if (conn_params_->router_opt_choke_points_ && is_flat_ && rr_graph_->node_type(to->index) == e_rr_type::IPIN) {
        auto find_res = conn_params_->connection_choking_spots_.find(to->index);
        if (find_res != conn_params_->connection_choking_spots_.end()) {
            cong_cost = cong_cost / pow(2, (float)find_res->second);
        }
    }

    //Update the backward cost (upstream already included)
    to->backward_path_cost += (1. - cost_params.criticality) * cong_cost; //Congestion cost
    to->backward_path_cost += cost_params.criticality * Tdel;             //Delay cost

    if (cost_params.bend_cost != 0.) {
        e_rr_type from_type = rr_graph_->node_type(from_node);
        e_rr_type to_type = rr_graph_->node_type(to->index);
        if ((from_type == e_rr_type::CHANX && to_type == e_rr_type::CHANY) || (from_type == e_rr_type::CHANY && to_type == e_rr_type::CHANX)) {
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
        add_route_tree_node_to_heap(rt_node, target_node, cost_params, net_bb);
    }

    for (const RouteTreeNode& child_node : rt_node.child_nodes()) {
        if (is_flat_) {
            if (relevant_node_to_target(rr_graph_, child_node.inode, target_node)) {
                add_route_tree_to_heap(child_node, target_node, cost_params, net_bb);
            }
        } else {
            add_route_tree_to_heap(child_node, target_node, cost_params, net_bb);
        }
    }
}

/* Expand bb by inode's extents and clip against net_bb */
inline void expand_highfanout_bounding_box(t_bb& bb, const t_bb& net_bb, RRNodeId inode, const RRGraphView* rr_graph) {
    bb.xmin = std::max<int>(net_bb.xmin, std::min<int>(bb.xmin, rr_graph->node_xlow(inode)));
    bb.ymin = std::max<int>(net_bb.ymin, std::min<int>(bb.ymin, rr_graph->node_ylow(inode)));
    bb.xmax = std::min<int>(net_bb.xmax, std::max<int>(bb.xmax, rr_graph->node_xhigh(inode)));
    bb.ymax = std::min<int>(net_bb.ymax, std::max<int>(bb.ymax, rr_graph->node_yhigh(inode)));
    bb.layer_min = std::min<int>(bb.layer_min, rr_graph->node_layer_low(inode));
    bb.layer_max = std::max<int>(bb.layer_max, rr_graph->node_layer_high(inode));
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

    short target_layer = rr_graph_->node_layer_low(target_node);

    int chan_nodes_added = 0;

    t_bb highfanout_bb;
    highfanout_bb.xmin = rr_graph_->node_xlow(target_node);
    highfanout_bb.xmax = rr_graph_->node_xhigh(target_node);
    highfanout_bb.ymin = rr_graph_->node_ylow(target_node);
    highfanout_bb.ymax = rr_graph_->node_yhigh(target_node);
    highfanout_bb.layer_min = rr_graph_->node_layer_low(target_node);
    highfanout_bb.layer_max = rr_graph_->node_layer_high(target_node);

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

                auto rt_node_layer_num = rr_graph_->node_layer_low(rr_node_to_add);
                if (rt_node_layer_num == target_layer)
                    found_node_on_same_layer = true;

                // Put the node onto the heap
                add_route_tree_node_to_heap(rt_node, target_node, cost_params, net_bounding_box);

                // Expand HF BB to include the node (clip by original BB)
                expand_highfanout_bounding_box(highfanout_bb, net_bounding_box, rr_node_to_add, rr_graph_);

                if (rr_graph_->node_type(rr_node_to_add) == e_rr_type::CHANY || rr_graph_->node_type(rr_node_to_add) == e_rr_type::CHANX) {
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

/** Used for the flat router. The node isn't relevant to the target if
 * it is an intra-block node outside of our target block */
inline bool relevant_node_to_target(const RRGraphView* rr_graph,
                                    RRNodeId node_to_add,
                                    RRNodeId target_node) {
    VTR_ASSERT_SAFE(rr_graph->node_type(target_node) == e_rr_type::SINK);
    auto node_to_add_type = rr_graph->node_type(node_to_add);
    return node_to_add_type != e_rr_type::IPIN || node_in_same_physical_tile(node_to_add, target_node);
}
