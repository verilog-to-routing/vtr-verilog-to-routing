#pragma once

/** @file Nested parallel case for NetlistRouter */
#include "netlist_routers.h"
#include "vtr_optional.h"
#include "vtr_thread_pool.h"
#include <unordered_map>

/* Add cmd line option for this later */
constexpr int MAX_THREADS = 4;

/** Nested parallel impl for NetlistRouter.
 *
 * Calls a parallel ConnectionRouter for route_net to extract even more parallelism.
 * The main reason why this is a different router instead of templating NetlistRouter
 * on ConnectionRouter is this router does not use TBB. The scheduling performance is
 * worse, but it can wait in individual tasks now (which is not possible with TBB).
 *
 * Holds enough context members to glue together ConnectionRouter and net routing functions,
 * such as \ref route_net. Keeps the members in thread-local storage where needed,
 * i.e. ConnectionRouters and RouteIterResults-es.
 * See \ref route_net. */
template<typename HeapType>
class NestedNetlistRouter : public NetlistRouter {
  public:
    NestedNetlistRouter(
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
        : _net_list(net_list)
        , _router_lookahead(router_lookahead)
        , _router_opts(router_opts)
        , _connections_inf(connections_inf)
        , _net_delay(net_delay)
        , _netlist_pin_lookup(netlist_pin_lookup)
        , _timing_info(timing_info)
        , _pin_timing_invalidator(pin_timing_invalidator)
        , _budgeting_inf(budgeting_inf)
        , _routing_predictor(routing_predictor)
        , _choking_spots(choking_spots)
        , _is_flat(is_flat)
        , _thread_pool(MAX_THREADS) {}
    ~NestedNetlistRouter() {}

    /** Run a single iteration of netlist routing for this->_net_list. This usually means calling
     * \ref route_net for each net, which will handle other global updates.
     * \return RouteIterResults for this iteration. */
    RouteIterResults route_netlist(int itry, float pres_fac, float worst_neg_slack);
    /** Inform the PartitionTree of the nets with updated bounding boxes */
    void handle_bb_updated_nets(const std::vector<ParentNetId>& nets);

    /** Set rcv_enabled for each ConnectionRouter this is managing */
    void set_rcv_enabled(bool x);
    /** Set timing_info for each ConnectionRouter this is managing */
    void set_timing_info(std::shared_ptr<SetupHoldTimingInfo> timing_info);

  private:
    /** Route all nets in a PartitionTree node and add its children to the task queue. */
    void route_partition_tree_node(PartitionTreeNode& node);

    ConnectionRouter<HeapType> _make_router(const RouterLookahead* router_lookahead, bool is_flat) {
        auto& device_ctx = g_vpr_ctx.device();
        auto& route_ctx = g_vpr_ctx.mutable_routing();

        return ConnectionRouter<HeapType>(
            device_ctx.grid,
            *router_lookahead,
            device_ctx.rr_graph.rr_nodes(),
            &device_ctx.rr_graph,
            device_ctx.rr_rc_data,
            device_ctx.rr_graph.rr_switch(),
            route_ctx.rr_node_route_inf,
            is_flat);
    }

    /* Context fields. Most of them will be forwarded to route_net (see route_net.tpp) */
    const Netlist<>& _net_list;
    const RouterLookahead* _router_lookahead;
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

    /** Cached routing parameters for current iteration (inputs to \see route_netlist()) */
    int _itry;
    float _pres_fac;
    float _worst_neg_slack;

    /** The partition tree. Holds the groups of nets for each partition */
    vtr::optional<PartitionTree> _tree;

    /** Thread pool for parallel routing. See vtr_thread_pool.h for implementation */
    vtr::thread_pool _thread_pool;

    /* Thread-local storage.
     * These are maps because thread::id is a random integer instead of 1, 2, ... */
    std::unordered_map<std::thread::id, ConnectionRouter<HeapType>> _routers_th;
    std::unordered_map<std::thread::id, RouteIterResults> _results_th;
    std::mutex _storage_mutex;

    /** Get a thread-local ConnectionRouter. We lock the id->router lookup, but this is
     * accessed once per partition so the overhead should be small */
    ConnectionRouter<HeapType>& get_thread_router() {
        auto id = std::this_thread::get_id();
        std::lock_guard<std::mutex> lock(_storage_mutex);
        if (!_routers_th.count(id)) {
            _routers_th.emplace(id, _make_router(_router_lookahead, _is_flat));
        }
        return _routers_th.at(id);
    }

    RouteIterResults& get_thread_results() {
        auto id = std::this_thread::get_id();
        std::lock_guard<std::mutex> lock(_storage_mutex);
        return _results_th[id];
    }
};

#include "NestedNetlistRouter.tpp"
