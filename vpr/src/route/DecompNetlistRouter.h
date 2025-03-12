#pragma once

/** @file Parallel and net-decomposing case for NetlistRouter. Works like
 * \see ParallelNetlistRouter, but tries to "decompose" nets and assign them to
 * the next level of the partition tree where possible.
 * See "Parallel FPGA Routing with On-the-Fly Net Decomposition", FPT'24 */
#include "netlist_routers.h"

#include <tbb/task_group.h>

/** Maximum number of iterations for net decomposition
 * 5 is found experimentally: higher values get more speedup on initial iters but # of iters increases */
const int MAX_DECOMP_ITER = 5;

/** Maximum # of decomposition for a net: 2 means one net gets divided down to <4 virtual nets.
 * Higher values are more aggressive: better thread utilization but worse congestion resolving */
const int MAX_DECOMP_DEPTH = 2;

/** Minimum # of fanouts of a net to consider decomp. */
const int MIN_DECOMP_SINKS = 8;

/** Minimum # of fanouts of a virtual net to consider decomp. */
const int MIN_DECOMP_SINKS_VNET = 8;

template<typename HeapType>
class DecompNetlistRouter : public NetlistRouter {
  public:
    DecompNetlistRouter(
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
        : _routers_th(_make_router(router_lookahead, is_flat))
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
        , _is_flat(is_flat)
        , _net_known_samples(net_list.nets().size())
        , _is_decomp_disabled(net_list.nets().size()) {}
    ~DecompNetlistRouter() {}

    /** Run a single iteration of netlist routing for this->_net_list. This usually means calling
     * \ref route_net for each net, which will handle other global updates.
     * \return RouteIterResults for this iteration. */
    RouteIterResults route_netlist(int itry, float pres_fac, float worst_neg_slack);
    /** Inform the PartitionTree of the nets with updated bounding boxes */
    void handle_bb_updated_nets(const std::vector<ParentNetId>& nets);
    /** Set RCV enable flag for all routers managed by this netlist router.
     * Net decomposition does not work with RCV, so calling this fn with x=true is a fatal error. */
    void set_rcv_enabled(bool x);
    void set_timing_info(std::shared_ptr<SetupHoldTimingInfo> timing_info);

  private:
    /** Should we decompose this net? */
    bool should_decompose_net(ParentNetId net_id, const PartitionTreeNode& node);
    /** Get a bitset of sinks to route before net decomposition. Output bitset is 
     * [1..num_sinks] where the corresponding index is set to 1 if the sink needs to
     * be routed */
    vtr::dynamic_bitset<> get_decomposition_mask(ParentNetId net_id, const PartitionTreeNode& node);
    /** Get a bitset of sinks to route before virtual net decomposition. Output bitset is 
     * [1..num_sinks] where the corresponding index is set to 1 if the sink needs to
     * be routed */
    vtr::dynamic_bitset<> get_decomposition_mask_vnet(const VirtualNet& vnet, const PartitionTreeNode& node);
    /** Decompose and route a regular net. Output the resulting vnets to \p left and \p right.
     * \return Success status: true if routing is successful and left and right now contain valid virtual nets: false otherwise. */
    bool decompose_and_route_net(ParentNetId net_id, const PartitionTreeNode& node, VirtualNet& left, VirtualNet& right);
    /** Decompose and route a virtual net. Output the resulting vnets to \p left and \p right.
     * \return Success status: true if routing is successful and left and right now contain valid virtual nets: false otherwise. */
    bool decompose_and_route_vnet(VirtualNet& vnet, const PartitionTreeNode& node, VirtualNet& left, VirtualNet& right);
    /** A single task to route nets inside a PartitionTree node and add tasks for its child nodes to task group \p g. */
    void route_partition_tree_node(tbb::task_group& g, PartitionTreeNode& node);

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
    /** Per-thread storage for ConnectionRouters. */
    tbb::enumerable_thread_specific<ConnectionRouter<HeapType>> _routers_th;
    const Netlist<>& _net_list;
    const t_router_opts& _router_opts;
    CBRR& _connections_inf;
    /** Per-thread storage for RouteIterResults. */
    tbb::enumerable_thread_specific<RouteIterResults> _results_th;
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

    /** Sinks to be always sampled for decomposition for each net: [0.._net_list.size()-1]
     * (i.e. when routing fails after decomposition for a sink, sample it on next iteration) */
    vtr::vector<ParentNetId, vtr::dynamic_bitset<>> _net_known_samples;

    /** Is decomposition disabled for this net? [0.._net_list.size()-1] */
    vtr::vector<ParentNetId, bool> _is_decomp_disabled;
};

#include "DecompNetlistRouter.tpp"
