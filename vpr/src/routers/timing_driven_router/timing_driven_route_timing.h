/* IMPORTANT:
 * The following preprocessing flags are added to 
 * avoid compilation error when this headers are included in more than 1 times 
 */
#ifndef TIMING_DRIVEN_ROUTE_TIMING_H
#define TIMING_DRIVEN_ROUTE_TIMING_H

/*
 * Notes in include header files in a head file 
 * Only include the neccessary header files 
 * that is required by the data types in the function/class declarations!
 */
/* Header files should be included in a sequence */
/* Standard header files required go first */

#include <unordered_map>
#include <vector>

#include "timing_driven_connection_based_routing.h"
#include "vpr_types.h"

#include "vpr_utils.h"
#include "timing_info_fwd.h"
#include "timing_driven_route_budgets.h"
#include "router_common.h"
#include "router_stats.h"
#include "timing_driven_router_lookahead.h"

/* Namespace declaration */
/* All the routers should be placed in the namespace of router
 * A specific router may have it own namespace under the router namespace
 * To call a function in a router, function need a prefix of router::<your_router_namespace>::
 * This will ease the development in modularization.
 * The concern is to minimize/avoid conflicts between data type names as much as possible
 * Also, this will keep function names as short as possible.
 */
namespace router { 

namespace timing_driven { 

int get_max_pins_per_net();

bool try_timing_driven_route(
        const t_router_opts& router_opts,
        const t_analysis_opts& analysis_opts,
		vtr::vector<ClusterNetId, float *> &net_delay,
        const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
        std::shared_ptr<SetupHoldTimingInfo> timing_info,
        std::shared_ptr<RoutingDelayCalculator> delay_calc, 
#ifdef ENABLE_CLASSIC_VPR_STA
        t_slack * slacks,
        const t_timing_inf &timing_inf,
#endif
        ScreenUpdatePriority first_iteration_priority
        );
bool try_timing_driven_route_net(ClusterNetId net_id, int itry, float pres_fac,
		t_router_opts router_opts,
		CBRR& connections_inf,
        RouterStats& connections_routed,
		float* pin_criticality,
		t_rt_node** rt_node_of_sink, vtr::vector<ClusterNetId, float *> &net_delay,
        const RouterLookahead& router_lookahead,
        const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
        std::shared_ptr<SetupTimingInfo> timing_info, route_budgets &budgeting_inf);

bool timing_driven_route_net(ClusterNetId net_id, int itry, float pres_fac,
        const t_router_opts& router_opts,
		CBRR& connections_inf,
        RouterStats& connections_routed,
		float *pin_criticality,
        t_rt_node ** rt_node_of_sink,
		float *net_delay,
        const RouterLookahead& router_lookahead,
        const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
        std::shared_ptr<const SetupTimingInfo> timing_info, route_budgets &budgeting_inf);


void alloc_timing_driven_route_structs(float **pin_criticality_ptr,
        int **sink_order_ptr, t_rt_node *** rt_node_of_sink_ptr);
void free_timing_driven_route_structs(float *pin_criticality, int *sink_order,
        t_rt_node ** rt_node_of_sink);


//Delay budget information for a specific connection
struct t_conn_delay_budget {
    float short_path_criticality; //Hold criticality

    float min_delay; //Minimum legal connection delay
    float target_delay; //Target/goal connection delay
    float max_delay; //Maximum legal connection delay
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


t_heap* timing_driven_route_connection_from_route_tree(t_rt_node* rt_root, int sink_node,
        const t_conn_cost_params cost_params,
        t_bb bounding_box,
        const RouterLookahead& router_lookahead,
        std::vector<int>& modified_rr_node_inf,
        RouterStats& router_stats);

std::vector<t_heap> timing_driven_find_all_shortest_paths_from_route_tree(
        t_rt_node* rt_root,
        const t_conn_cost_params cost_params,
        t_bb bounding_box,
        std::vector<int>& modified_rr_node_inf,
        RouterStats& router_stats);

struct timing_driven_route_structs {
    // data while timing driven route is active
    float* pin_criticality; /* [1..max_pins_per_net-1] */
    int* sink_order; /* [1..max_pins_per_net-1] */
    t_rt_node** rt_node_of_sink; /* [1..max_pins_per_net-1] */

    timing_driven_route_structs();
    ~timing_driven_route_structs();
};

void update_rr_base_costs(int fanout);

} /* end of namespace timing_driven */

} /* end of namespace router */

#endif
