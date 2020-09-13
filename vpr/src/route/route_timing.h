#pragma once
#include <unordered_map>
#include <vector>
#include "connection_based_routing.h"
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

int get_max_pins_per_net();

bool try_timing_driven_route(const t_router_opts& router_opts,
                             const t_analysis_opts& analysis_opts,
                             const std::vector<t_segment_inf>& segment_inf,
                             ClbNetPinsMatrix<float>& net_delay,
                             const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                             std::shared_ptr<SetupHoldTimingInfo> timing_info,
                             std::shared_ptr<RoutingDelayCalculator> delay_calc,
                             ScreenUpdatePriority first_iteration_priority);

template<typename ConnectionRouter>
bool try_timing_driven_route_net(ConnectionRouter& router,
                                 ClusterNetId net_id,
                                 int itry,
                                 float pres_fac,
                                 const t_router_opts& router_opts,
                                 CBRR& connections_inf,
                                 RouterStats& connections_routed,
                                 float* pin_criticality,
                                 t_rt_node** rt_node_of_sink,
                                 ClbNetPinsMatrix<float>& net_delay,
                                 const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                 std::shared_ptr<SetupHoldTimingInfo> timing_info,
                                 ClusteredPinTimingInvalidator* pin_timing_invalidator,
                                 route_budgets& budgeting_inf,
                                 bool& was_rerouted,
                                 float worst_neg_slack,
                                 const RoutingPredictor& routing_predictor);

template<typename ConnectionRouter>
bool timing_driven_route_net(ConnectionRouter& router,
                             ClusterNetId net_id,
                             int itry,
                             float pres_fac,
                             const t_router_opts& router_opts,
                             CBRR& connections_inf,
                             RouterStats& connections_routed,
                             float* pin_criticality,
                             t_rt_node** rt_node_of_sink,
                             float* net_delay,
                             const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                             std::shared_ptr<const SetupHoldTimingInfo> timing_info,
                             ClusteredPinTimingInvalidator* pin_timing_invalidator,
                             route_budgets& budgeting_inf,
                             float worst_neg_slack,
                             const RoutingPredictor& routing_predictor);

void alloc_timing_driven_route_structs(float** pin_criticality_ptr,
                                       int** sink_order_ptr,
                                       t_rt_node*** rt_node_of_sink_ptr);
void free_timing_driven_route_structs(float* pin_criticality, int* sink_order, t_rt_node** rt_node_of_sink);

void enable_router_debug(const t_router_opts& router_opts, ClusterNetId net, int sink_rr, int router_iteration, ConnectionRouterInterface* router);

bool is_iteration_complete(bool routing_is_feasible, const t_router_opts& router_opts, int itry, std::shared_ptr<const SetupHoldTimingInfo> timing_info, bool rcv_finished);

bool should_setup_lower_bound_connection_delays(int itry, const t_router_opts& router_opts);

std::vector<t_heap> timing_driven_find_all_shortest_paths_from_route_tree(t_rt_node* rt_root,
                                                                          const t_conn_cost_params cost_params,
                                                                          t_bb bounding_box,
                                                                          std::vector<int>& modified_rr_node_inf,
                                                                          RouterStats& router_stats);

struct timing_driven_route_structs {
    // data while timing driven route is active
    float* pin_criticality;      /* [1..max_pins_per_net-1] */
    int* sink_order;             /* [1..max_pins_per_net-1] */
    t_rt_node** rt_node_of_sink; /* [1..max_pins_per_net-1] */

    timing_driven_route_structs();
    ~timing_driven_route_structs();
};

void update_rr_base_costs(int fanout);
