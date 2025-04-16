#pragma once

/** @file Impls for ParallelNetlistRouter */

#include <string>
#include "netlist_routers.h"
#include "route_net.h"
#include "vtr_time.h"

template<typename HeapType>
inline RouteIterResults NestedNetlistRouter<HeapType>::route_netlist(int itry, float pres_fac, float worst_neg_slack) {
    /* Reset results for each thread */
    for (auto& [_, results] : _results_th) {
        results = RouteIterResults();
    }

    /* Set the routing parameters: they won't change until the next call and that saves us the trouble of passing them around */
    _itry = itry;
    _pres_fac = pres_fac;
    _worst_neg_slack = worst_neg_slack;

    /* Organize netlist into a PartitionTree.
     * Nets in a given level of nodes are guaranteed to not have any overlapping bounding boxes, so they can be routed in parallel. */
    vtr::Timer timer;
    if (!_tree) {
        _tree = PartitionTree(_net_list);
        PartitionTreeDebug::log("Iteration " + std::to_string(itry) + ": built partition tree in " + std::to_string(timer.elapsed_sec()) + " s");
    }

    /* Push a single route_partition_tree_node task to the thread pool,
     * which will recursively schedule the rest of the tree */
    _thread_pool.schedule_work([this]() {
        route_partition_tree_node(_tree->root());
    });

    /* Wait for all tasks in the thread pool to complete */
    _thread_pool.wait_for_all();

    PartitionTreeDebug::log("Routing all nets took " + std::to_string(timer.elapsed_sec()) + " s");

    /* Combine results from threads */
    RouteIterResults out;
    for (auto& [_, results] : _results_th) {
        out.stats.combine(results.stats);
        out.rerouted_nets.insert(out.rerouted_nets.end(), results.rerouted_nets.begin(), results.rerouted_nets.end());
        out.bb_updated_nets.insert(out.bb_updated_nets.end(), results.bb_updated_nets.begin(), results.bb_updated_nets.end());
        out.is_routable &= results.is_routable;
    }
    return out;
}

template<typename HeapType>
void NestedNetlistRouter<HeapType>::route_partition_tree_node(PartitionTreeNode& node) {
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    /* node.nets is an unordered set, copy into vector to sort */
    std::vector<ParentNetId> nets(node.nets.begin(), node.nets.end());

    /* Sort so net with most sinks is routed first. */
    std::stable_sort(nets.begin(), nets.end(), [&](ParentNetId id1, ParentNetId id2) -> bool {
        return _net_list.net_sinks(id1).size() > _net_list.net_sinks(id2).size();
    });

    vtr::Timer timer;

    /* Route all nets in this node serially */
    for (auto net_id : nets) {
        auto& results = get_thread_results();
        auto& router = get_thread_router();

        auto flags = route_net(
            router,
            _net_list,
            net_id,
            _itry,
            _pres_fac,
            _router_opts,
            _connections_inf,
            results.stats,
            _net_delay,
            _netlist_pin_lookup,
            _timing_info.get(),
            _pin_timing_invalidator,
            _budgeting_inf,
            _worst_neg_slack,
            _routing_predictor,
            _choking_spots[net_id],
            _is_flat,
            route_ctx.route_bb[net_id]);

        if (!flags.success && !flags.retry_with_full_bb) {
            /* Disconnected RRG and ConnectionRouter doesn't think growing the BB will work */
            results.is_routable = false;
            return;
        }
        if (flags.retry_with_full_bb) {
            /* ConnectionRouter thinks we should grow the BB. Do that and leave this net unrouted for now */
            route_ctx.route_bb[net_id] = full_device_bb();
            results.bb_updated_nets.push_back(net_id);
            continue;
        }
        if (flags.was_rerouted) {
            results.rerouted_nets.push_back(net_id);
        }
    }

    PartitionTreeDebug::log("Node with " + std::to_string(node.nets.size())
                            + " nets and " + std::to_string(node.vnets.size())
                            + " virtual nets routed in " + std::to_string(timer.elapsed_sec())
                            + " s");

    /* Schedule child nodes as new tasks */
    if (node.left && node.right) {
        _thread_pool.schedule_work([this, left = node.left.get()]() {
            route_partition_tree_node(*left);
        });
        _thread_pool.schedule_work([this, right = node.right.get()]() {
            route_partition_tree_node(*right);
        });
    } else {
        VTR_ASSERT(!node.left && !node.right); // there shouldn't be a node with a single branch
    }
}

template<typename HeapType>
void NestedNetlistRouter<HeapType>::handle_bb_updated_nets(const std::vector<ParentNetId>& nets) {
    VTR_ASSERT(_tree);
    _tree->update_nets(nets);
}

template<typename HeapType>
void NestedNetlistRouter<HeapType>::set_rcv_enabled(bool x) {
    for (auto& [_, router] : _routers_th) {
        router.set_rcv_enabled(x);
    }
}

template<typename HeapType>
void NestedNetlistRouter<HeapType>::set_timing_info(std::shared_ptr<SetupHoldTimingInfo> timing_info) {
    _timing_info = timing_info;
}
