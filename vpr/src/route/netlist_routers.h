#pragma once

/** @file Interface for a netlist router.
 *
 * A NetlistRouter manages the required bits of state to complete the netlist routing process,
 * which requires finding a path for every connection in the netlist using a ConnectionRouter.
 * This needs to be an interface because there may be different netlist routing schedules,
 * i.e. parallel or net-decomposing routers.
 *
 * Includes derived classes of NetlistRouter and a fn to provide the correct NetlistRouter
 * for given router options.
 *
 * NetlistRouter impls are typically templated by HeapType, since the single implementation
 * of ConnectionRouterInterface is templated by a heap type at the moment. Any templated
 * NetlistRouter-derived class is still a NetlistRouter, so that is transparent to the user
 * of this interface. */

#include "NetPinTimingInvalidator.h"
#include "binary_heap.h"
#include "four_ary_heap.h"
#include "bucket.h"
#include "clustered_netlist_utils.h"
#include "connection_based_routing_fwd.h"
#include "connection_router.h"
#include "globals.h"
#include "heap_type.h"
#include "netlist_fwd.h"
#include "partition_tree.h"
#include "routing_predictor.h"
#include "route_budgets.h"
#include "route_utils.h"
#include "router_stats.h"
#include "timing_info.h"
#include "vpr_net_pins_matrix.h"
#include "vpr_types.h"

/** Results for a single netlist routing run inside a routing iteration. */
struct RouteIterResults {
    /** Are there any connections impossible to route due to a disconnected rr_graph? */
    bool is_routable = true;
    /** Net IDs with changed routing */
    std::vector<ParentNetId> rerouted_nets;
    /** RouterStats for this iteration */
    RouterStats stats;
};

/** Route a given netlist. Takes a big context and passes it around to net & sink routing fns.
 * route_netlist only needs to call the functions in route_net.h/tpp: they handle the global
 * bookkeeping. */
class NetlistRouter {
  public:
    virtual ~NetlistRouter() {}

    /** Run a single iteration of netlist routing for this->_net_list. This usually means calling
     * route_net for each net, which will handle other global updates.
     * \return RouteIterResults for this iteration. */
    virtual RouteIterResults route_netlist(int itry, float pres_fac, float worst_neg_slack) = 0;

    /** Enable RCV for each of the ConnectionRouters this NetlistRouter manages.*/
    virtual void set_rcv_enabled(bool x) = 0;

    /** Set this NetlistRouter's timing_info ptr. We sometimes change timing_info
     * throughout iterations, but not frequently enough to make it a public member. */
    virtual void set_timing_info(std::shared_ptr<SetupHoldTimingInfo> timing_info) = 0;
};

/* Include the derived classes here to get the HeapType-templated impls */
#include "SerialNetlistRouter.h"
#ifdef VPR_USE_TBB
#    include "ParallelNetlistRouter.h"
#    include "DecompNetlistRouter.h"
#endif

template<typename HeapType>
inline std::unique_ptr<NetlistRouter> make_netlist_router_with_heap(
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
    bool is_flat) {
    if (router_opts.router_algorithm == e_router_algorithm::TIMING_DRIVEN) {
        return std::make_unique<SerialNetlistRouter<HeapType>>(
            net_list,
            router_lookahead,
            router_opts,
            connections_inf,
            net_delay,
            netlist_pin_lookup,
            timing_info,
            pin_timing_invalidator,
            budgeting_inf,
            routing_predictor,
            choking_spots,
            is_flat);
    } else if (router_opts.router_algorithm == e_router_algorithm::PARALLEL) {
#ifdef VPR_USE_TBB
        return std::make_unique<ParallelNetlistRouter<HeapType>>(
            net_list,
            router_lookahead,
            router_opts,
            connections_inf,
            net_delay,
            netlist_pin_lookup,
            timing_info,
            pin_timing_invalidator,
            budgeting_inf,
            routing_predictor,
            choking_spots,
            is_flat);
#else
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "VPR isn't compiled with TBB support required for parallel routing");
#endif
    } else if (router_opts.router_algorithm == e_router_algorithm::PARALLEL_DECOMP) {
#ifdef VPR_USE_TBB
        return std::make_unique<DecompNetlistRouter<HeapType>>(
            net_list,
            router_lookahead,
            router_opts,
            connections_inf,
            net_delay,
            netlist_pin_lookup,
            timing_info,
            pin_timing_invalidator,
            budgeting_inf,
            routing_predictor,
            choking_spots,
            is_flat);
#else
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "VPR isn't compiled with TBB support required for parallel routing");
#endif
    } else {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Unknown router algorithm %d", router_opts.router_algorithm);
    }
}

/** Make a NetlistRouter depending on router_algorithm and router_heap in \p router_opts. */
inline std::unique_ptr<NetlistRouter> make_netlist_router(
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
    bool is_flat) {
    if (router_opts.router_heap == e_heap_type::BINARY_HEAP) {
        return make_netlist_router_with_heap<BinaryHeap>(
            net_list,
            router_lookahead,
            router_opts,
            connections_inf,
            net_delay,
            netlist_pin_lookup,
            timing_info,
            pin_timing_invalidator,
            budgeting_inf,
            routing_predictor,
            choking_spots,
            is_flat);
    } else if (router_opts.router_heap == e_heap_type::FOUR_ARY_HEAP) {
        return make_netlist_router_with_heap<FourAryHeap>(
            net_list,
            router_lookahead,
            router_opts,
            connections_inf,
            net_delay,
            netlist_pin_lookup,
            timing_info,
            pin_timing_invalidator,
            budgeting_inf,
            routing_predictor,
            choking_spots,
            is_flat);
    } else if (router_opts.router_heap == e_heap_type::BUCKET_HEAP_APPROXIMATION) {
        return make_netlist_router_with_heap<Bucket>(
            net_list,
            router_lookahead,
            router_opts,
            connections_inf,
            net_delay,
            netlist_pin_lookup,
            timing_info,
            pin_timing_invalidator,
            budgeting_inf,
            routing_predictor,
            choking_spots,
            is_flat);
    } else {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Unknown heap type %d", router_opts.router_heap);
    }
}
