#pragma once

/** @file Templated implementations for SerialNetlistRouter */

#include "SerialNetlistRouter.h"
#include "connection_router_interface.h"
#include "globals.h"
#include "netlist_fwd.h"
#include "route_net.h"
#include "vpr_context.h"

// ENUM for the different net-level predictor types
// TODO: Make this a router option so it can be selected from the command line
#define ALWAYS_PARALLEL_PRED_TY 0
#define ALWAYS_SERIAL_PRED_TY 1
#define HARD_CODED_PRED_TY 2
// Which predictor should the hybrid router use.
#define HYBRID_ROUTER_PRED ALWAYS_PARALLEL_PRED_TY

template<typename HeapType>
inline bool SerialNetlistRouter<HeapType>::should_use_parallel_connection_router(const ParentNetId &net_id, int itry, float pres_fac, float worst_neg_slack) {
    (void)net_id;
    (void)itry;
    (void)pres_fac;
    (void)worst_neg_slack;
    // This method predicts whether the given net will parallelize well or not.
    // TODO: Maybe decide whether to route in parallel or not during the previous
    //       routing iteration. That way we do not need to store information.

#if HYBRID_ROUTER_PRED == ALWAYS_PARALLEL_PRED_TY
    // Always run the nets in parallel
    return true;
#elif HYBRID_ROUTER_PRED == ALWAYS_SERIAL_PRED_TY
    // Always run the nets serially
    return false;
#elif HYBRID_ROUTER_PRED == HARD_CODED_PRED_TY
    // Basic (ideal) predictor
    // If the net has a sink in a block that we know is problematic, route
    // in parallel.
    // NOTE: This is currently hard coded for the bwave-like circuit on the
    //       VTR Flagship Architecture.
    const AtomContext &atom_ctx = g_vpr_ctx.atom();
    for (ParentPinId sink_pin_id : _net_list.net_sinks(net_id)) {
        // Get the name of the block from the netlist
        ParentBlockId target_block_id = _net_list.pin_block(sink_pin_id);
        const std::string &block_name = _net_list.block_name(target_block_id);
        // Use the name to lookup the block in the atom netlist to get the model
        // name.
        // TODO: Find a more efficient way to lookup the atom block ID from the
        //       sink pin ID.
        AtomBlockId target_atom_block_id = atom_ctx.nlist.find_block(block_name);
        const t_model* block_model = atom_ctx.nlist.block_model(target_atom_block_id);
        // See if the model name matches the block types we know cause problems.
        if (std::strcmp(block_model->name, "dual_port_ram") == 0) {
            return true;
        } else if (std::strcmp(block_model->name, "multiply") == 0) {
            return true;
        }
    }
    return false;
#endif
}

template<typename HeapType>
inline RouteIterResults SerialNetlistRouter<HeapType>::route_netlist(int itry, float pres_fac, float worst_neg_slack) {
    auto& route_ctx = g_vpr_ctx.mutable_routing();
    RouteIterResults out;

    /* Sort so net with most sinks is routed first */
    auto sorted_nets = std::vector<ParentNetId>(_net_list.nets().begin(), _net_list.nets().end());
    std::stable_sort(sorted_nets.begin(), sorted_nets.end(), [&](ParentNetId id1, ParentNetId id2) -> bool {
        return _net_list.net_sinks(id1).size() > _net_list.net_sinks(id2).size();
    });

    for (size_t inet = 0; inet < sorted_nets.size(); inet++) {
        ParentNetId net_id = sorted_nets[inet];

        // Choose which router to use
        bool use_parallel_router = should_use_parallel_connection_router(net_id, itry, pres_fac, worst_neg_slack);
        ConnectionRouterInterface *router = use_parallel_router ? _parallel_router : _serial_router;

        NetResultFlags flags = route_net(
            router,
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
            /* Grow the BB and retry this net right away. */
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

    return out;
}

template<typename HeapType>
void SerialNetlistRouter<HeapType>::set_rcv_enabled(bool x) {
    _serial_router->set_rcv_enabled(x);
    _parallel_router->set_rcv_enabled(x);
}

template<typename HeapType>
void SerialNetlistRouter<HeapType>::set_timing_info(std::shared_ptr<SetupHoldTimingInfo> timing_info) {
    _timing_info = timing_info;
}
