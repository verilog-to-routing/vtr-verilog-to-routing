#pragma once

/** @file Impls for ParallelNetlistRouter */

#include "netlist_routers.h"
#include "route_net.h"
#include "vtr_time.h"

template<typename HeapType>
inline RouteIterResults ParallelNetlistRouter<HeapType>::route_netlist(int itry, float pres_fac, float worst_neg_slack) {
    /* Reset results for each thread */
    for (auto& results : _results_th) {
        results = RouteIterResults();
    }

    /* Set the routing parameters: they won't change until the next call and that saves us the trouble of passing them around */
    _itry = itry;
    _pres_fac = pres_fac;
    _worst_neg_slack = worst_neg_slack;

    /* Organize netlist into a PartitionTree.
     * Nets in a given level of nodes are guaranteed to not have any overlapping bounding boxes, so they can be routed in parallel. */
    PartitionTree tree(_net_list);

    /* Put the root node on the task queue, which will add its child nodes when it's finished. Wait until the entire tree gets routed. */
    tbb::task_group g;
    route_partition_tree_node(g, tree.root());
    g.wait();

    /* Combine results from threads */
    RouteIterResults out;
    for (auto& results : _results_th) {
        out.stats.combine(results.stats);
        out.rerouted_nets.insert(out.rerouted_nets.end(), results.rerouted_nets.begin(), results.rerouted_nets.end());
        out.is_routable &= results.is_routable;
    }
    return out;
}

template<typename HeapType>
void ParallelNetlistRouter<HeapType>::route_partition_tree_node(tbb::task_group& g, PartitionTreeNode& node) {
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    /* Sort so net with most sinks is routed first. */
    std::stable_sort(node.nets.begin(), node.nets.end(), [&](ParentNetId id1, ParentNetId id2) -> bool {
        return _net_list.net_sinks(id1).size() > _net_list.net_sinks(id2).size();
    });

    vtr::Timer t;
    for (auto net_id : node.nets) {
        auto flags = route_net(
            &_routers_th.local(),
            _net_list,
            net_id,
            _itry,
            _pres_fac,
            _router_opts,
            _connections_inf,
            _results_th.local().stats,
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
            _results_th.local().is_routable = false;
            return;
        }
        if (flags.retry_with_full_bb) {
            /* ConnectionRouter thinks we should grow the BB. Do that and leave this net unrouted for now */
            route_ctx.route_bb[net_id] = full_device_bb();
            continue;
        }
        if (flags.was_rerouted) {
            _results_th.local().rerouted_nets.push_back(net_id);
        }
    }
    PartitionTreeDebug::log("Node with " + std::to_string(node.nets.size()) + " nets routed in " + std::to_string(t.elapsed_sec()) + " s");

    /* This node is finished: add left & right branches to the task queue */
    if (node.left && node.right) {
        g.run([&]() {
            route_partition_tree_node(g, *node.left);
        });
        g.run([&]() {
            route_partition_tree_node(g, *node.right);
        });
    } else {
        VTR_ASSERT(!node.left && !node.right); // there shouldn't be a node with a single branch
    }
}

template<typename HeapType>
void ParallelNetlistRouter<HeapType>::set_rcv_enabled(bool x) {
    for (auto& router : _routers_th) {
        router.set_rcv_enabled(x);
    }
}

template<typename HeapType>
void ParallelNetlistRouter<HeapType>::set_timing_info(std::shared_ptr<SetupHoldTimingInfo> timing_info) {
    _timing_info = timing_info;
}
