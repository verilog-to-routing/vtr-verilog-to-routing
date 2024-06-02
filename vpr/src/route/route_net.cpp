/** @file Impls for non-templated net routing fns & utils */

#include "route_net.h"
#include "stats.h"

bool check_hold(const t_router_opts& router_opts, float worst_neg_slack) {
    if (router_opts.routing_budgets_algorithm != YOYO) {
        return false;
    } else if (worst_neg_slack != 0) {
        return true;
    }
    return false;
}

void setup_net(int itry,
               ParentNetId net_id,
               const Netlist<>& net_list,
               CBRR& connections_inf,
               const t_router_opts& router_opts,
               float worst_neg_slack) {
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    /* "tree" points to this net's spot in the global context here, so re-initializing it etc. changes the global state */
    vtr::optional<RouteTree>& tree = route_ctx.route_trees[net_id];

    bool ripup_high_fanout_nets = check_hold(router_opts, worst_neg_slack);
    int num_sinks = net_list.net_sinks(net_id).size();

    // for nets below a certain size (min_incremental_reroute_fanout), rip up any old routing
    // otherwise, we incrementally reroute by reusing legal parts of the previous iteration
    if (num_sinks < router_opts.min_incremental_reroute_fanout || itry == 1 || ripup_high_fanout_nets) {
        profiling::net_rerouted();

        /* rip up the whole net */
        if (tree)
            pathfinder_update_cost_from_route_tree(tree.value().root(), -1);
        tree = vtr::nullopt;

        /* re-initialize net */
        tree = RouteTree(net_id);
        pathfinder_update_cost_from_route_tree(tree.value().root(), 1);

        // since all connections will be rerouted for this net, clear all of net's forced reroute flags
        connections_inf.clear_force_reroute_for_net(net_id);
    } else {
        profiling::net_rebuild_start();

        if (!tree) {
            tree = RouteTree(net_id);
            pathfinder_update_cost_from_route_tree(tree.value().root(), 1);
        }

        /* copy the existing routing
         * prune() depends on global occ, so we can't subtract before pruning
         * OPT: to skip this copy, return a "diff" from RouteTree::prune */
        RouteTree tree2 = tree.value();

        // Prune the copy (using congestion data before subtraction)
        vtr::optional<RouteTree&> pruned_tree2 = tree2.prune(connections_inf);

        // Subtract congestion using the non-pruned original
        pathfinder_update_cost_from_route_tree(tree->root(), -1);

        if (pruned_tree2) { //Partially pruned
            profiling::route_tree_preserved();

            // Add back congestion for the pruned route tree
            pathfinder_update_cost_from_route_tree(pruned_tree2->root(), 1);
            // pruned_tree2 is no longer required -> we can move rather than copy
            tree = std::move(pruned_tree2.value());
        } else { // Fully destroyed
            profiling::route_tree_pruned();

            // Initialize only to source
            tree = RouteTree(net_id);
            pathfinder_update_cost_from_route_tree(tree->root(), 1);
        }

        profiling::net_rebuild_end(num_sinks, tree->get_remaining_isinks().size());

        // still need to calculate the tree's time delay
        tree->reload_timing();

        // check for R_upstream C_downstream and edge correctness
        VTR_ASSERT_SAFE(tree->is_valid());

        // congestion should've been pruned away
        VTR_ASSERT_SAFE(tree->is_uncongested());

        // mark the lookup (rr_node_route_inf) for existing tree elements as NO_PREVIOUS so add_to_path stops when it reaches one of them
        update_rr_route_inf_from_tree(tree->root());
    }

    // completed constructing the partial route tree and updated all other data structures to match
}

void update_rr_base_costs(int fanout) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    float factor;
    size_t index;

    /* Other reasonable values for factor include fanout and 1 */
    factor = sqrt(fanout);

    for (index = CHANX_COST_INDEX_START; index < device_ctx.rr_indexed_data.size(); index++) {
        if (device_ctx.rr_indexed_data[RRIndexedDataId(index)].T_quadratic > 0.) { /* pass transistor */
            device_ctx.rr_indexed_data[RRIndexedDataId(index)].base_cost = device_ctx.rr_indexed_data[RRIndexedDataId(index)].saved_base_cost * factor;
        } else {
            device_ctx.rr_indexed_data[RRIndexedDataId(index)].base_cost = device_ctx.rr_indexed_data[RRIndexedDataId(index)].saved_base_cost;
        }
    }
}

void update_rr_route_inf_from_tree(const RouteTreeNode& rt_node) {
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    for (auto& node : rt_node.all_nodes()) {
        RRNodeId inode = node.inode;
        route_ctx.rr_node_route_inf[inode].prev_edge = RREdgeId::INVALID();

        // path cost should be unset
        VTR_ASSERT(std::isinf(route_ctx.rr_node_route_inf[inode].path_cost));
        VTR_ASSERT(std::isinf(route_ctx.rr_node_route_inf[inode].backward_path_cost));
    }
}

bool should_route_net(const Netlist<>& net_list,
                      ParentNetId net_id,
                      CBRR& connections_inf,
                      route_budgets& budgeting_inf,
                      float worst_negative_slack,
                      bool if_force_reroute) {
    auto& route_ctx = g_vpr_ctx.routing();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    if (route_ctx.net_status.is_fixed(net_id)) /* Skip pre-routed nets */
        return false;
    if (net_list.net_is_ignored(net_id)) /* Skip ignored nets */
        return false;

    if (!route_ctx.route_trees[net_id]) /* No routing yet */
        return true;
    if (worst_negative_slack != 0 && budgeting_inf.if_set() && budgeting_inf.get_should_reroute(net_id)) /* Reroute for hold */
        return true;

    const RouteTree& tree = route_ctx.route_trees[net_id].value();

    /* Walk over all rt_nodes in the net */
    for (auto& rt_node : tree.all_nodes()) {
        RRNodeId inode = rt_node.inode;
        int occ = route_ctx.rr_node_route_inf[inode].occ();
        int capacity = rr_graph.node_capacity(inode);

        if (occ > capacity) {
            return true; /* overuse detected */
        }

        if (rt_node.is_leaf()) { //End of a branch
            // even if net is fully routed, not complete if parts of it should get ripped up (EXPERIMENTAL)
            if (if_force_reroute) {
                if (connections_inf.should_force_reroute_connection(net_id, inode)) {
                    return true;
                }
            }
        }
    }

    /* If all sinks have been routed to without overuse, no need to route this */
    if (tree.get_remaining_isinks().empty())
        return false;

    return true;
}

bool early_exit_heuristic(const t_router_opts& router_opts, const WirelengthInfo& wirelength_info) {
    if (wirelength_info.used_wirelength_ratio() > router_opts.init_wirelength_abort_threshold) {
        VTR_LOG("Wire length usage ratio %g exceeds limit of %g, fail routing.\n",
                wirelength_info.used_wirelength_ratio(),
                router_opts.init_wirelength_abort_threshold);
        return true;
    }
    return false;
}

float get_net_pin_criticality(const SetupHoldTimingInfo* timing_info,
                              const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                              float max_criticality,
                              float criticality_exp,
                              ParentNetId net_id,
                              ParentPinId pin_id,
                              bool is_flat) {
    float pin_criticality = 0.0;
    const auto& route_ctx = g_vpr_ctx.routing();

    if (route_ctx.is_clock_net[net_id]) {
        pin_criticality = max_criticality;
    } else {
        pin_criticality = calculate_clb_net_pin_criticality(*timing_info,
                                                            netlist_pin_lookup,
                                                            pin_id,
                                                            is_flat);
    }

    /* Pin criticality is between 0 and 1.
     * Shift it downwards by 1 - max_criticality (max_criticality is 0.99 by default,
     * so shift down by 0.01) and cut off at 0.  This means that all pins with small
     * criticalities (<0.01) get criticality 0 and are ignored entirely, and everything
     * else becomes a bit less critical. This effect becomes more pronounced if
     * max_criticality is set lower. */
    // VTR_ASSERT(pin_criticality[ipin] > -0.01 && pin_criticality[ipin] < 1.01);
    pin_criticality = std::max(pin_criticality - (1.0 - max_criticality), 0.0);

    /* Take pin criticality to some power (1 by default). */
    pin_criticality = std::pow(pin_criticality, criticality_exp);

    /* Cut off pin criticality at max_criticality. */
    pin_criticality = std::min(pin_criticality, max_criticality);

    return pin_criticality;
}

size_t calculate_wirelength_available() {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    size_t available_wirelength = 0;
    // But really what's happening is that this for loop iterates over every node and determines the available wirelength
    for (const RRNodeId& rr_id : device_ctx.rr_graph.nodes()) {
        const t_rr_type channel_type = rr_graph.node_type(rr_id);
        if (channel_type == CHANX || channel_type == CHANY) {
            available_wirelength += rr_graph.node_capacity(rr_id) * rr_graph.node_length(rr_id);
        }
    }
    return available_wirelength;
}

WirelengthInfo calculate_wirelength_info(const Netlist<>& net_list, size_t available_wirelength) {
    size_t used_wirelength = 0;
    VTR_ASSERT(available_wirelength > 0);

    auto& route_ctx = g_vpr_ctx.routing();

#ifdef VPR_USE_TBB
    tbb::combinable<size_t> thread_used_wirelength(0);

    tbb::parallel_for_each(net_list.nets().begin(), net_list.nets().end(), [&](ParentNetId net_id) {
        if (!net_list.net_is_ignored(net_id)
            && net_list.net_sinks(net_id).size() != 0 /* Globals don't count. */
            && route_ctx.route_trees[net_id]) {
            int bends, wirelength, segments;
            bool is_absorbed;
            get_num_bends_and_length(net_id, &bends, &wirelength, &segments, &is_absorbed);

            thread_used_wirelength.local() += wirelength;
        }
    });

    used_wirelength = thread_used_wirelength.combine(std::plus<size_t>());
#else
    for (auto net_id : net_list.nets()) {
        if (!net_list.net_is_ignored(net_id)
            && net_list.net_sinks(net_id).size() != 0 /* Globals don't count. */
            && route_ctx.route_trees[net_id]) {
            int bends = 0, wirelength = 0, segments = 0;
            bool is_absorbed;
            get_num_bends_and_length(net_id, &bends, &wirelength, &segments, &is_absorbed);

            used_wirelength += wirelength;
        }
    }
#endif

    return WirelengthInfo(available_wirelength, used_wirelength);
}

t_bb calc_current_bb(const RouteTree& tree) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& grid = device_ctx.grid;

    t_bb bb;
    bb.xmin = grid.width() - 1;
    bb.ymin = grid.height() - 1;
    bb.layer_min = grid.get_num_layers() - 1;
    bb.xmax = 0;
    bb.ymax = 0;
    bb.layer_max = 0;

    for (auto& rt_node : tree.all_nodes()) {
        //The router interprets RR nodes which cross the boundary as being
        //'within' of the BB. Only those which are *strictly* out side the
        //box are excluded, hence we use the nodes xhigh/yhigh for xmin/xmax,
        //and xlow/ylow for xmax/ymax calculations
        bb.xmin = std::min<int>(bb.xmin, rr_graph.node_xhigh(rt_node.inode));
        bb.ymin = std::min<int>(bb.ymin, rr_graph.node_yhigh(rt_node.inode));
        bb.layer_min = std::min<int>(bb.layer_min, rr_graph.node_layer(rt_node.inode));
        bb.xmax = std::max<int>(bb.xmax, rr_graph.node_xlow(rt_node.inode));
        bb.ymax = std::max<int>(bb.ymax, rr_graph.node_ylow(rt_node.inode));
        bb.layer_max = std::max<int>(bb.layer_max, rr_graph.node_layer(rt_node.inode));
    }

    VTR_ASSERT(bb.xmin <= bb.xmax);
    VTR_ASSERT(bb.ymin <= bb.ymax);

    return bb;
}

// Initializes net_delay based on best-case delay estimates from the router lookahead
void init_net_delay_from_lookahead(const RouterLookahead& router_lookahead,
                                   const Netlist<>& net_list,
                                   const vtr::vector<ParentNetId, std::vector<RRNodeId>>& net_rr_terminals,
                                   NetPinsMatrix<float>& net_delay,
                                   const RRGraphView& rr_graph,
                                   bool is_flat) {
    t_conn_cost_params cost_params;
    cost_params.criticality = 1.; // Ensures lookahead returns delay value

    for (auto net_id : net_list.nets()) {
        if (net_list.net_is_ignored(net_id)) continue;

        RRNodeId source_rr = net_rr_terminals[net_id][0];

        for (size_t ipin = 1; ipin < net_list.net_pins(net_id).size(); ++ipin) {
            RRNodeId sink_rr = net_rr_terminals[net_id][ipin];

            float est_delay = get_cost_from_lookahead(router_lookahead,
                                                      rr_graph,
                                                      source_rr,
                                                      sink_rr,
                                                      0.,
                                                      cost_params,
                                                      is_flat);
            VTR_ASSERT(std::isfinite(est_delay) && est_delay < std::numeric_limits<float>::max());

            net_delay[net_id][ipin] = est_delay;
        }
    }
}

void update_net_delays_from_route_tree(float* net_delay,
                                       const Netlist<>& net_list,
                                       ParentNetId inet,
                                       TimingInfo* timing_info,
                                       NetPinTimingInvalidator* pin_timing_invalidator) {
    auto& route_ctx = g_vpr_ctx.routing();
    const RouteTree& tree = route_ctx.route_trees[inet].value();
    auto& is_isink_reached = tree.get_is_isink_reached();

    for (unsigned int isink = 1; isink < net_list.net_pins(inet).size(); isink++) {
        if (!is_isink_reached.get(isink))
            continue;
        update_net_delay_from_isink(net_delay, tree, isink, net_list, inet, timing_info, pin_timing_invalidator);
    }
}
