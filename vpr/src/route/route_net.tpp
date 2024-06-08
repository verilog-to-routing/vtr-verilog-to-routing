#pragma once

/** @file Header implementations for templated net routing fns. */

#include "route_net.h"

#include <tuple>

#include "connection_router_interface.h"
#include "describe_rr_node.h"
#include "draw.h"
#include "route_common.h"
#include "route_debug.h"
#include "route_profiling.h"
#include "rr_graph_fwd.h"
#include "vtr_dynamic_bitset.h"

/** Attempt to route a single net.
 *
 * @param router The ConnectionRouter instance 
 * @param net_list Input netlist
 * @param net_id
 * @param itry # of iteration
 * @param pres_fac
 * @param router_opts
 * @param connections_inf
 * @param router_stats
 * @param pin_criticality
 * @param rt_node_of_sink Lookup from target_pin-like indices (indicating SINK nodes) to RouteTreeNodes
 * @param net_delay
 * @param netlist_pin_lookup
 * @param timing_info
 * @param pin_timing_invalidator
 * @param budgeting_inf
 * @param worst_neg_slack
 * @param routing_predictor
 * @param choking_spots
 * @param is_flat
 * @param net_bb Bounding box for the net (Routing resources outside net_bb will not be used)
 * @param should_setup Should we reset/prune the existing route tree first?
 * @param sink_mask Which sinks to route? Assumed all sinks if nullopt, otherwise a mask of [1..num_sinks+1] where set bits request the sink to be routed
 * @return NetResultFlags for this net */
template<typename ConnectionRouter>
inline NetResultFlags route_net(ConnectionRouter *router,
                                const Netlist<>& net_list,
                                const ParentNetId& net_id,
                                int itry,
                                float pres_fac,
                                const t_router_opts& router_opts,
                                CBRR& connections_inf,
                                RouterStats& router_stats,
                                NetPinsMatrix<float>& net_delays,
                                const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                SetupHoldTimingInfo* timing_info,
                                NetPinTimingInvalidator* pin_timing_invalidator,
                                route_budgets& budgeting_inf,
                                float worst_negative_slack,
                                const RoutingPredictor& routing_predictor,
                                const std::vector<std::unordered_map<RRNodeId, int>>& choking_spots,
                                bool is_flat,
                                const t_bb& net_bb,
                                bool should_setup = true,
                                vtr::optional<const vtr::dynamic_bitset<>&> sink_mask = vtr::nullopt) {
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    NetResultFlags flags;
    flags.success = true;

    if (!should_route_net(net_list, net_id, connections_inf, budgeting_inf, worst_negative_slack, true))
        return flags;

    // track time spent vs fanout
    profiling::net_fanout_start();

    flags.was_rerouted = true; // Flag to record whether routing was actually changed

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    unsigned int num_sinks = net_list.net_sinks(net_id).size();

    VTR_LOGV_DEBUG(f_router_debug, "Routing Net %zu (%zu sinks)\n", size_t(net_id), num_sinks);

    /* Prune or rip-up existing routing for the net */
    if (should_setup) {
        setup_net(
            itry,
            net_id,
            net_list,
            connections_inf,
            router_opts,
            worst_negative_slack);
    }

    VTR_ASSERT(route_ctx.route_trees[net_id]);
    RouteTree& tree = route_ctx.route_trees[net_id].value();

    bool high_fanout = is_high_fanout(num_sinks, router_opts.high_fanout_threshold);

    SpatialRouteTreeLookup spatial_route_tree_lookup;
    if (high_fanout) {
        spatial_route_tree_lookup = build_route_tree_spatial_lookup(net_list,
                                                                    route_ctx.route_bb,
                                                                    net_id,
                                                                    tree.root());
    }

    /* 1-indexed! */
    std::vector<float> pin_criticality(tree.num_sinks() + 1, 0);

    // after this point the route tree is correct
    // remaining_targets from this point on are the **pin indices** that have yet to be routed
    auto remaining_targets_mask = ~tree.get_is_isink_reached();
    if (sink_mask)
        remaining_targets_mask &= sink_mask.value();

    auto remaining_targets = sink_mask_to_vector(remaining_targets_mask, num_sinks);

    // calculate criticality of remaining target pins
    for (int ipin : remaining_targets) {
        auto pin = net_list.net_pin(net_id, ipin);
        pin_criticality[ipin] = get_net_pin_criticality(timing_info,
                                                        netlist_pin_lookup,
                                                        router_opts.max_criticality,
                                                        router_opts.criticality_exp,
                                                        net_id,
                                                        pin,
                                                        is_flat);
    }

    // compare the criticality of different sink nodes
    sort(begin(remaining_targets), end(remaining_targets), [&](int a, int b) {
        return pin_criticality[a] > pin_criticality[b];
    });

    /* Update base costs according to fanout and criticality rules */
    update_rr_base_costs(num_sinks);

    t_conn_delay_budget conn_delay_budget;
    t_conn_cost_params cost_params;
    cost_params.astar_fac = router_opts.astar_fac;
    cost_params.astar_offset = router_opts.astar_offset;
    cost_params.post_target_prune_fac = router_opts.post_target_prune_fac;
    cost_params.post_target_prune_offset = router_opts.post_target_prune_offset;
    cost_params.bend_cost = router_opts.bend_cost;
    cost_params.pres_fac = pres_fac;
    cost_params.delay_budget = ((budgeting_inf.if_set()) ? &conn_delay_budget : nullptr);

    // Pre-route to clock source for clock nets (marked as global nets)
    if (net_list.net_is_global(net_id) && router_opts.two_stage_clock_routing) {
        //VTR_ASSERT(router_opts.clock_modeling == DEDICATED_NETWORK);
        RRNodeId sink_node(device_ctx.virtual_clock_network_root_idx);

        enable_router_debug(router_opts, net_id, sink_node, itry, router);

        VTR_LOGV_DEBUG(f_router_debug, "Pre-routing global net %zu\n", size_t(net_id));

        // Set to the max timing criticality which should intern minimize clock insertion
        // delay by selecting a direct route from the clock source to the virtual sink
        cost_params.criticality = router_opts.max_criticality;

        return pre_route_to_clock_root(router,
                                       net_id,
                                       net_list,
                                       sink_node,
                                       cost_params,
                                       router_opts.high_fanout_threshold,
                                       tree,
                                       spatial_route_tree_lookup,
                                       router_stats,
                                       is_flat);
    }

    if (budgeting_inf.if_set()) {
        budgeting_inf.set_should_reroute(net_id, false);
    }

    // explore in order of decreasing criticality (no longer need sink_order array)
    for (unsigned itarget = 0; itarget < remaining_targets.size(); ++itarget) {
        int target_pin = remaining_targets[itarget];

        RRNodeId sink_rr = route_ctx.net_rr_terminals[net_id][target_pin];

        enable_router_debug(router_opts, net_id, sink_rr, itry, router);

        cost_params.criticality = pin_criticality[target_pin];

        if (budgeting_inf.if_set()) {
            conn_delay_budget.max_delay = budgeting_inf.get_max_delay_budget(net_id, target_pin);
            conn_delay_budget.target_delay = budgeting_inf.get_delay_target(net_id, target_pin);
            conn_delay_budget.min_delay = budgeting_inf.get_min_delay_budget(net_id, target_pin);
            conn_delay_budget.short_path_criticality = budgeting_inf.get_crit_short_path(net_id, target_pin);
            conn_delay_budget.routing_budgets_algorithm = router_opts.routing_budgets_algorithm;
        }

        profiling::conn_start();

        // build a branch in the route tree to the target
        auto sink_flags = route_sink(router,
                                     net_list,
                                     net_id,
                                     itarget,
                                     target_pin,
                                     cost_params,
                                     router_opts,
                                     tree,
                                     spatial_route_tree_lookup,
                                     router_stats,
                                     budgeting_inf,
                                     routing_predictor,
                                     choking_spots,
                                     is_flat,
                                     net_bb);

        flags.retry_with_full_bb |= sink_flags.retry_with_full_bb;

        if (!sink_flags.success) {
            flags.success = false;
            VTR_LOG("Routing failed for sink %d of net %d\n", target_pin, net_id);
            return flags;
        }

        profiling::conn_finish(size_t(route_ctx.net_rr_terminals[net_id][0]),
                               size_t(sink_rr),
                               pin_criticality[target_pin]);

        ++router_stats.connections_routed;
    } // finished all sinks

    ++router_stats.nets_routed;
    profiling::net_finish();

    /* For later timing analysis. */

    float* net_delay = net_delays[net_id].data();

    // may have to update timing delay of the previously legally reached sinks since downstream capacitance could be changed
    update_net_delays_from_route_tree(net_delay,
                                      net_list,
                                      net_id,
                                      timing_info,
                                      pin_timing_invalidator);

    if (router_opts.update_lower_bound_delays) {
        for (int ipin : remaining_targets) {
            connections_inf.update_lower_bound_connection_delay(net_id, ipin, net_delay[ipin]);
        }
    }

    VTR_ASSERT_MSG(g_vpr_ctx.routing().rr_node_route_inf[tree.root().inode].occ() <= rr_graph.node_capacity(tree.root().inode), "SOURCE should never be congested");
    VTR_LOGV_DEBUG(f_router_debug, "Routed Net %zu (%zu sinks)\n", size_t(net_id), num_sinks);

    router->empty_rcv_route_tree_set(); // ?

    profiling::net_fanout_end(net_list.net_sinks(net_id).size());

    route_ctx.net_status.set_is_routed(net_id, true);
    return flags;
}

/** Route to a "virtual sink" in the netlist which corresponds to the start point
 * of the global clock network. */
template<typename ConnectionRouter>
inline NetResultFlags pre_route_to_clock_root(ConnectionRouter *router,
                                              ParentNetId net_id,
                                              const Netlist<>& net_list,
                                              RRNodeId sink_node,
                                              const t_conn_cost_params cost_params,
                                              int high_fanout_threshold,
                                              RouteTree& tree,
                                              SpatialRouteTreeLookup& spatial_rt_lookup,
                                              RouterStats& router_stats,
                                              bool is_flat) {
    const auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.mutable_routing();
    auto& m_route_ctx = g_vpr_ctx.mutable_routing();

    NetResultFlags out;

    bool high_fanout = is_high_fanout(net_list.net_sinks(net_id).size(), high_fanout_threshold);

    VTR_LOGV_DEBUG(f_router_debug, "Net %zu pre-route to (%s)\n", size_t(net_id), describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, sink_node, is_flat).c_str());
    profiling::sink_criticality_start();

    t_bb bounding_box = route_ctx.route_bb[net_id];

    router->clear_modified_rr_node_info();

    bool found_path, retry_with_full_bb;
    t_heap cheapest;
    ConnectionParameters conn_params(net_id,
                                     -1,
                                     false,
                                     std::unordered_map<RRNodeId, int>());

    std::tie(found_path, retry_with_full_bb, cheapest) = router->timing_driven_route_connection_from_route_tree(
        tree.root(),
        sink_node,
        cost_params,
        bounding_box,
        router_stats,
        conn_params);

    // TODO: Parts of the rest of this function are repetitive to code in route_sink. Should refactor.
    if (!found_path) {
        ParentBlockId src_block = net_list.net_driver_block(net_id);
        VTR_LOG("Failed to route connection from '%s' to '%s' for net '%s' (#%zu)\n",
                net_list.block_name(src_block).c_str(),
                describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, sink_node, is_flat).c_str(),
                net_list.net_name(net_id).c_str(),
                size_t(net_id));
        if (f_router_debug) {
            update_screen(ScreenUpdatePriority::MAJOR, "Unable to route connection.", ROUTING, nullptr);
        }
        router->reset_path_costs();
        out.success = false;
        out.retry_with_full_bb = retry_with_full_bb;
        return out;
    }

    profiling::sink_criticality_end(cost_params.criticality);

    /* This is a special pre-route to a sink that does not correspond to any    *
     * netlist pin, but which can be reached from the global clock root drive   *
     * points. Therefore, we can set the net pin index of the sink node to      *
     * OPEN (meaning illegal) as it is not meaningful for this sink.            */
    vtr::optional<const RouteTreeNode&> new_branch, new_sink;
    std::tie(new_branch, new_sink) = tree.update_from_heap(&cheapest, OPEN, ((high_fanout) ? &spatial_rt_lookup : nullptr), is_flat);

    VTR_ASSERT_DEBUG(!high_fanout || validate_route_tree_spatial_lookup(tree.root(), spatial_rt_lookup));

    if (f_router_debug) {
        std::string msg = vtr::string_fmt("Routed Net %zu connection to RR node %d successfully", size_t(net_id), sink_node);
        update_screen(ScreenUpdatePriority::MAJOR, msg.c_str(), ROUTING, nullptr);
    }

    if (new_branch)
        pathfinder_update_cost_from_route_tree(new_branch.value(), 1);

    // need to guarantee ALL nodes' path costs are HUGE_POSITIVE_FLOAT at the start of routing to a sink
    // do this by resetting all the path_costs that have been touched while routing to the current sink
    router->reset_path_costs();

    // Post route cleanup:
    // - remove sink from route tree and fix routing for all nodes leading to the sink ("freeze")
    // - free up virtual sink occupancy
    tree.freeze();
    m_route_ctx.rr_node_route_inf[sink_node].set_occ(0);

    // routed to a sink successfully
    out.success = true;
    return out;
}

/** Attempt to route a single sink (target_pin) in a net.
 * In the process, update global pathfinder costs, rr_node_route_inf and extend the global RouteTree
 * for this net.
 *
 * @param router The ConnectionRouter instance 
 * @param net_list Input netlist
 * @param net_id
 * @param itarget # of this connection in the net (only used for debug output)
 * @param target_pin # of this sink in the net (TODO: is it the same thing as itarget?)
 * @param cost_params
 * @param router_opts
 * @param[in, out] tree RouteTree describing the current routing state
 * @param rt_node_of_sink Lookup from target_pin-like indices (indicating SINK nodes) to RouteTreeNodes
 * @param spatial_rt_lookup
 * @param router_stats
 * @param budgeting_inf
 * @param routing_predictor
 * @param choking_spots
 * @param is_flat
 * @param net_bb Bounding box for the net (Routing resources outside net_bb will not be used)
 * @return NetResultFlags for this sink to be bubbled up through route_net */
template<typename ConnectionRouter>
inline NetResultFlags route_sink(ConnectionRouter *router,
                                 const Netlist<>& net_list,
                                 ParentNetId net_id,
                                 unsigned itarget,
                                 int target_pin,
                                 const t_conn_cost_params cost_params,
                                 const t_router_opts& router_opts,
                                 RouteTree& tree,
                                 SpatialRouteTreeLookup& spatial_rt_lookup,
                                 RouterStats& router_stats,
                                 route_budgets& budgeting_inf,
                                 const RoutingPredictor& routing_predictor,
                                 const std::vector<std::unordered_map<RRNodeId, int>>& choking_spots,
                                 bool is_flat,
                                 const t_bb& net_bb) {
    const auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    NetResultFlags flags;

    profiling::sink_criticality_start();

    RRNodeId sink_node = route_ctx.net_rr_terminals[net_id][target_pin];
    VTR_LOGV_DEBUG(f_router_debug, "Net %zu Target %d (%s)\n", size_t(net_id), itarget, describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, sink_node, is_flat).c_str());

    router->clear_modified_rr_node_info();

    bool found_path;
    t_heap cheapest;

    bool net_is_global = net_list.net_is_global(net_id);
    bool high_fanout = is_high_fanout(net_list.net_sinks(net_id).size(), router_opts.high_fanout_threshold);
    constexpr float HIGH_FANOUT_CRITICALITY_THRESHOLD = 0.9;
    bool sink_critical = (cost_params.criticality > HIGH_FANOUT_CRITICALITY_THRESHOLD);
    bool net_is_clock = route_ctx.is_clock_net[net_id] != 0;

    bool has_choking_spot = ((int)choking_spots[target_pin].size() != 0) && router_opts.has_choking_spot;
    ConnectionParameters conn_params(net_id, target_pin, has_choking_spot, choking_spots[target_pin]);

    //We normally route high fanout nets by only adding spatially close-by routing to the heap (reduces run-time).
    //However, if the current sink is 'critical' from a timing perspective, we put the entire route tree back onto
    //the heap to ensure it has more flexibility to find the best path.
    if (high_fanout && !sink_critical && !net_is_global && !net_is_clock && -routing_predictor.get_slope() > router_opts.high_fanout_max_slope) {
        std::tie(found_path, flags.retry_with_full_bb, cheapest) = router->timing_driven_route_connection_from_route_tree_high_fanout(tree.root(),
                                                                                                                                     sink_node,
                                                                                                                                     cost_params,
                                                                                                                                     net_bb,
                                                                                                                                     spatial_rt_lookup,
                                                                                                                                     router_stats,
                                                                                                                                     conn_params);
    } else {
        std::tie(found_path, flags.retry_with_full_bb, cheapest) = router->timing_driven_route_connection_from_route_tree(tree.root(),
                                                                                                                         sink_node,
                                                                                                                         cost_params,
                                                                                                                         net_bb,
                                                                                                                         router_stats,
                                                                                                                         conn_params);
    }

    if (!found_path) {
        ParentBlockId src_block = net_list.net_driver_block(net_id);
        ParentBlockId sink_block = net_list.pin_block(*(net_list.net_pins(net_id).begin() + target_pin));
        VTR_LOG("Failed to route connection from '%s' to '%s' for net '%s' (#%zu)\n",
                net_list.block_name(src_block).c_str(),
                net_list.block_name(sink_block).c_str(),
                net_list.net_name(net_id).c_str(),
                size_t(net_id));
        if (f_router_debug) {
            update_screen(ScreenUpdatePriority::MAJOR, "Unable to route connection.", ROUTING, nullptr);
        }
        flags.success = false;
        router->reset_path_costs();
        return flags;
    }

    profiling::sink_criticality_end(cost_params.criticality);

    vtr::optional<const RouteTreeNode&> new_branch, new_sink;
    std::tie(new_branch, new_sink) = tree.update_from_heap(&cheapest, target_pin, ((high_fanout) ? &spatial_rt_lookup : nullptr), is_flat);

    VTR_ASSERT_DEBUG(!high_fanout || validate_route_tree_spatial_lookup(tree.root(), spatial_rt_lookup));

    if (f_router_debug) {
        std::string msg = vtr::string_fmt("Routed Net %zu connection %d to RR node %d successfully", size_t(net_id), itarget, sink_node);
        update_screen(ScreenUpdatePriority::MAJOR, msg.c_str(), ROUTING, nullptr);
    }

    if (budgeting_inf.if_set() && cheapest.path_data != nullptr && cost_params.delay_budget) {
        if (cheapest.path_data->backward_delay < cost_params.delay_budget->min_delay) {
            budgeting_inf.set_should_reroute(net_id, true);
        }
    }

    /* update global occupancy from the new branch */
    if (new_branch)
        pathfinder_update_cost_from_route_tree(new_branch.value(), 1);

    // need to guarantee ALL nodes' path costs are HUGE_POSITIVE_FLOAT at the start of routing to a sink
    // do this by resetting all the path_costs that have been touched while routing to the current sink
    router->reset_path_costs();

    // routed to a sink successfully
    flags.success = true;
    return flags;
}
