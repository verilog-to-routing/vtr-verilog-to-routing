#pragma once
#include <unordered_map>
#include <vector>
#include "connection_based_routing.h"
#include "netlist.h"
#include "vpr_types.h"

#include "vpr_utils.h"
#include "timing_info_fwd.h"
#include "route_budgets.h"
#include "router_stats.h"
#include "router_lookahead.h"
#include "spatial_route_tree_lookup.h"
#include "connection_router_interface.h"
#include "heap_type.h"
#include "routing_predictor.h"

extern bool f_router_debug;

int get_max_pins_per_net(const Netlist<>& net_list);

bool try_timing_driven_route(const Netlist<>& net_list,
                             const t_det_routing_arch& det_routing_arch,
                             const t_router_opts& router_opts,
                             const t_analysis_opts& analysis_opts,
                             const std::vector<t_segment_inf>& segment_inf,
                             NetPinsMatrix<float>& net_delay,
                             const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                             std::shared_ptr<SetupHoldTimingInfo> timing_info,
                             std::shared_ptr<RoutingDelayCalculator> delay_calc,
                             ScreenUpdatePriority first_iteration_priority,
                             bool is_flat);

template<typename ConnectionRouter>
bool try_timing_driven_route_net(ConnectionRouter& router,
                                 const Netlist<>& net_list,
                                 const ParentNetId& net_id,
                                 int itry,
                                 float pres_fac,
                                 const t_router_opts& router_opts,
                                 CBRR& connections_inf,
                                 RouterStats& router_stats,
                                 std::vector<float>& pin_criticality,
                                 std::vector<vtr::optional<const RouteTreeNode&>>& rt_node_of_sink,
                                 ClbNetPinsMatrix<float>& net_delay,
                                 const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                 std::shared_ptr<SetupHoldTimingInfo> timing_info,
                                 NetPinTimingInvalidator* pin_timing_invalidator,
                                 route_budgets& budgeting_inf,
                                 bool& was_rerouted,
                                 float worst_negative_slack,
                                 const RoutingPredictor& routing_predictor,
                                 const std::vector<std::unordered_map<RRNodeId, int>>& choking_spots,
                                 bool is_flat);

template<typename ConnectionRouter>
bool timing_driven_route_net(ConnectionRouter& router,
                             const Netlist<>& net_list,
                             ParentNetId net_id,
                             int itry,
                             float pres_fac,
                             const t_router_opts& router_opts,
                             CBRR& connections_inf,
                             RouterStats& router_stats,
                             std::vector<float>& pin_criticality,
                             std::vector<vtr::optional<const RouteTreeNode&>>& rt_node_of_sink,
                             float* net_delay,
                             const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                             std::shared_ptr<SetupHoldTimingInfo> timing_info,
                             NetPinTimingInvalidator* pin_timing_invalidator,
                             route_budgets& budgeting_inf,
                             float worst_neg_slack,
                             const RoutingPredictor& routing_predictor,
                             const std::vector<std::unordered_map<RRNodeId, int>>& choking_spots,
                             bool is_flat);

void enable_router_debug(const t_router_opts& router_opts, ParentNetId net, int sink_rr, int router_iteration, ConnectionRouterInterface* router);

bool is_iteration_complete(bool routing_is_feasible, const t_router_opts& router_opts, int itry, std::shared_ptr<const SetupHoldTimingInfo> timing_info, bool rcv_finished);

bool should_setup_lower_bound_connection_delays(int itry, const t_router_opts& router_opts);

void update_rr_base_costs(int fanout);

/* Data while timing driven route is active */
class timing_driven_route_structs {
  public:
    std::vector<float> pin_criticality;                               /* [1..max_pins_per_net-1] */
    std::vector<int> sink_order;                                      /* [1..max_pins_per_net-1] */
    std::vector<vtr::optional<const RouteTreeNode&>> rt_node_of_sink; /* [1..max_pins_per_net-1] */

    timing_driven_route_structs(const Netlist<>& net_list) {
        int max_sinks = std::max(get_max_pins_per_net(net_list) - 1, 0);
        pin_criticality.resize(max_sinks + 1);
        sink_order.resize(max_sinks + 1);
        rt_node_of_sink.resize(max_sinks + 1);

        /* Set element 0 to invalid values */
        pin_criticality[0] = std::numeric_limits<float>::quiet_NaN();
        sink_order[0] = -1;
        rt_node_of_sink[0] = vtr::nullopt;
    }
};
