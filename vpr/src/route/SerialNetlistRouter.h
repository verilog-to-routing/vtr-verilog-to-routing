#pragma once

/** @file Serial case for \ref NetlistRouter: just loop through nets */

#include "netlist_routers.h"
#include "parallel_connection_router.h"

template<typename HeapType>
class SerialNetlistRouter : public NetlistRouter {
  public:
    SerialNetlistRouter(
        const Netlist<>& net_list,
        const RouterLookahead* router_lookahead,
        const t_router_opts& router_opts,
        CBRR& connections_inf,
        NetPinsMatrix<float>& net_delay,
        const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
        std::shared_ptr<SetupHoldTimingInfo> timing_info,
        NetPinTimingInvalidator* pin_timing_invalidator,
        route_budgets& budgeting_inf,
        const RoutingPredictor& routing_predictor,
        const vtr::vector<ParentNetId, std::vector<std::unordered_map<RRNodeId, int>>>& choking_spots,
        bool is_flat)
        : _serial_router(_make_router(router_lookahead, router_opts, is_flat, false))
        , _parallel_router(_make_router(router_lookahead, router_opts, is_flat, true))
        , _net_list(net_list)
        , _router_opts(router_opts)
        , _connections_inf(connections_inf)
        , _net_delay(net_delay)
        , _netlist_pin_lookup(netlist_pin_lookup)
        , _timing_info(timing_info)
        , _pin_timing_invalidator(pin_timing_invalidator)
        , _budgeting_inf(budgeting_inf)
        , _routing_predictor(routing_predictor)
        , _choking_spots(choking_spots)
        , _is_flat(is_flat) {}
    ~SerialNetlistRouter() {
      delete _serial_router;
      delete _parallel_router;
    }

    RouteIterResults route_netlist(int itry, float pres_fac, float worst_neg_slack);
    void set_rcv_enabled(bool x);
    void set_timing_info(std::shared_ptr<SetupHoldTimingInfo> timing_info);

  private:
    bool should_use_parallel_connection_router(const ParentNetId &net_id, int itry, float pres_fac, float worst_neg_slack);

    ConnectionRouterInterface *_make_router(const RouterLookahead* router_lookahead,
                                            const t_router_opts& router_opts,
                                            bool is_flat, bool is_parallel) {
        auto& device_ctx = g_vpr_ctx.device();
        auto& route_ctx = g_vpr_ctx.mutable_routing();

        if (!is_parallel) {
        // Serial Connection Router
            return new ConnectionRouter<HeapType>(
                device_ctx.grid,
                *router_lookahead,
                device_ctx.rr_graph.rr_nodes(),
                &device_ctx.rr_graph,
                device_ctx.rr_rc_data,
                device_ctx.rr_graph.rr_switch(),
                route_ctx.rr_node_route_inf,
                is_flat);
        } else {
            // Parallel Connection Router
            return new ParallelConnectionRouter(
                device_ctx.grid,
                *router_lookahead,
                device_ctx.rr_graph.rr_nodes(),
                &device_ctx.rr_graph,
                device_ctx.rr_rc_data,
                device_ctx.rr_graph.rr_switch(),
                route_ctx.rr_node_route_inf,
                is_flat,
                router_opts.multi_queue_num_threads,
                router_opts.multi_queue_num_queues,
                router_opts.multi_queue_direct_draining,
                router_opts.thread_affinity);
            }
    }
    /* Context fields */
    ConnectionRouterInterface *_serial_router;
    ConnectionRouterInterface *_parallel_router;
    const Netlist<>& _net_list;
    const t_router_opts& _router_opts;
    CBRR& _connections_inf;
    NetPinsMatrix<float>& _net_delay;
    const ClusteredPinAtomPinsLookup& _netlist_pin_lookup;
    std::shared_ptr<SetupHoldTimingInfo> _timing_info;
    NetPinTimingInvalidator* _pin_timing_invalidator;
    route_budgets& _budgeting_inf;
    const RoutingPredictor& _routing_predictor;
    const vtr::vector<ParentNetId, std::vector<std::unordered_map<RRNodeId, int>>>& _choking_spots;
    bool _is_flat;
};

#include "SerialNetlistRouter.tpp"
