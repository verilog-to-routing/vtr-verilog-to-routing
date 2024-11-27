#pragma once

/** @file Templated implementations for SerialNetlistRouter */

#include "SerialNetlistRouter.h"
#include "route_net.h"
#include "vtr_time.h"

template<typename HeapType>
inline RouteIterResults SerialNetlistRouter<HeapType>::route_netlist(int itry, float pres_fac, float worst_neg_slack) {
    auto& route_ctx = g_vpr_ctx.mutable_routing();
    RouteIterResults out;

    vtr::Timer timer;

    /* Sort so net with most sinks is routed first */
    auto sorted_nets = std::vector<ParentNetId>(_net_list.nets().begin(), _net_list.nets().end());
    std::stable_sort(sorted_nets.begin(), sorted_nets.end(), [&](ParentNetId id1, ParentNetId id2) -> bool {
        return _net_list.net_sinks(id1).size() > _net_list.net_sinks(id2).size();
    });

    for (size_t inet = 0; inet < sorted_nets.size(); inet++) {
        ParentNetId net_id = sorted_nets[inet];
        NetResultFlags flags = route_net(
            _router,
            _net_list,
            net_id,
            itry,
            pres_fac,
            _router_opts,
            _connections_inf,
            out.stats,
            _net_delay,
            _netlist_pin_lookup,
            _timing_info.get(),
            _pin_timing_invalidator,
            _budgeting_inf,
            worst_neg_slack,
            _routing_predictor,
            _choking_spots[net_id],
            _is_flat,
            route_ctx.route_bb[net_id]);

        if (!flags.success && !flags.retry_with_full_bb) {
            /* Disconnected RRG and ConnectionRouter doesn't think growing the BB will work */
            out.is_routable = false;
            return out;
        }

        if (flags.retry_with_full_bb) {
            /* Grow the BB and retry this net right away.
             * We don't populate out.bb_updated_nets for the serial router, since
             * there is no partition tree to update. */
            route_ctx.route_bb[net_id] = full_device_bb();
            inet--;
            continue;
        }

        if (flags.was_rerouted) {
            out.rerouted_nets.push_back(net_id);
#ifndef NO_GRAPHICS
            update_router_info_and_check_bp(BP_NET_ID, size_t(net_id));
#endif
        }
    }

    PartitionTreeDebug::log("Routing all nets took " + std::to_string(timer.elapsed_sec()) + " s");
    return out;
}

template<typename HeapType>
void SerialNetlistRouter<HeapType>::handle_bb_updated_nets(const std::vector<ParentNetId>& /* nets */) {
}

template<typename HeapType>
void SerialNetlistRouter<HeapType>::set_rcv_enabled(bool x) {
    _router.set_rcv_enabled(x);
}

template<typename HeapType>
void SerialNetlistRouter<HeapType>::set_timing_info(std::shared_ptr<SetupHoldTimingInfo> timing_info) {
    _timing_info = timing_info;
}
