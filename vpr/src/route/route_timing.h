#pragma once
#include <unordered_map>
#include <vector>
#include "connection_based_routing.h"
#include "vpr_types.h"

#include "vpr_utils.h"
#include "timing_info_fwd.h"
#include "route_budgets.h"
#include "route_common.h"
#include "router_stats.h"
#include "router_lookahead.h"

int get_max_pins_per_net();

bool try_timing_driven_route(const t_router_opts& router_opts,
                             const t_analysis_opts& analysis_opts,
                             vtr::vector<ClusterNetId, float*>& net_delay,
                             const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                             std::shared_ptr<SetupHoldTimingInfo> timing_info,
                             std::shared_ptr<RoutingDelayCalculator> delay_calc,
                             ScreenUpdatePriority first_iteration_priority);

bool try_timing_driven_route_net(ClusterNetId net_id, int itry, float pres_fac, t_router_opts router_opts, CBRR& connections_inf, RouterStats& connections_routed, float* pin_criticality, t_rt_node** rt_node_of_sink, vtr::vector<ClusterNetId, float*>& net_delay, const RouterLookahead& router_lookahead, const ClusteredPinAtomPinsLookup& netlist_pin_lookup, std::shared_ptr<SetupTimingInfo> timing_info, route_budgets& budgeting_inf);

bool timing_driven_route_net(ClusterNetId net_id, int itry, float pres_fac, const t_router_opts& router_opts, CBRR& connections_inf, RouterStats& connections_routed, float* pin_criticality, t_rt_node** rt_node_of_sink, float* net_delay, const RouterLookahead& router_lookahead, const ClusteredPinAtomPinsLookup& netlist_pin_lookup, std::shared_ptr<const SetupTimingInfo> timing_info, route_budgets& budgeting_inf);

void alloc_timing_driven_route_structs(float** pin_criticality_ptr,
                                       int** sink_order_ptr,
                                       t_rt_node*** rt_node_of_sink_ptr);
void free_timing_driven_route_structs(float* pin_criticality, int* sink_order, t_rt_node** rt_node_of_sink);

//Delay budget information for a specific connection
struct t_conn_delay_budget {
    float short_path_criticality; //Hold criticality

    float min_delay;    //Minimum legal connection delay
    float target_delay; //Target/goal connection delay
    float max_delay;    //Maximum legal connection delay
};

struct t_conn_cost_params {
    float criticality = 1.;
    float astar_fac = 1.2;
    float bend_cost = 1.;
    const t_conn_delay_budget* delay_budget = nullptr;

    //TODO: Eventually once delay budgets are working, t_conn_delay_budget
    //should be factoured out, and the delay budget parameters integrated
    //into this struct instead. For now left as a pointer to control whether
    //budgets are enabled.
};

t_heap* timing_driven_route_connection_from_route_tree(t_rt_node* rt_root, int sink_node, const t_conn_cost_params cost_params, t_bb bounding_box, const RouterLookahead& router_lookahead, std::vector<int>& modified_rr_node_inf, RouterStats& router_stats);

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
