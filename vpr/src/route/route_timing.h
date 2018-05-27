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

int get_max_pins_per_net();

bool try_timing_driven_route(t_router_opts router_opts,
		vtr::vector_map<ClusterNetId, float *> &net_delay,
        const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
        std::shared_ptr<SetupHoldTimingInfo> timing_info,
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
		t_rt_node** rt_node_of_sink, vtr::vector_map<ClusterNetId, float *> &net_delay,
        const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
        std::shared_ptr<SetupTimingInfo> timing_info, route_budgets &budgeting_inf);

bool timing_driven_route_net(ClusterNetId net_id, int itry, float pres_fac, float max_criticality,
		float criticality_exp, float astar_fac, float bend_cost,
		CBRR& connections_inf,
        RouterStats& connections_routed,
		float *pin_criticality, int min_incremental_reroute_fanout, t_rt_node ** rt_node_of_sink,
		float *net_delay,
        const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
        std::shared_ptr<const SetupTimingInfo> timing_info, route_budgets &budgeting_inf);


void alloc_timing_driven_route_structs(float **pin_criticality_ptr,
        int **sink_order_ptr, t_rt_node *** rt_node_of_sink_ptr);
void free_timing_driven_route_structs(float *pin_criticality, int *sink_order,
        t_rt_node ** rt_node_of_sink);

t_heap * timing_driven_route_connection(int source_node, int sink_node, float target_criticality,
        float astar_fac, float bend_cost, t_rt_node* rt_root, t_bb bounding_box, int num_sinks,
        route_budgets &budgeting_inf, float max_delay, float min_delay, float target_delay, float short_path_crit,
        std::vector<int>& modified_rr_node_inf, RouterStats& router_stats);

struct timing_driven_route_structs {
    // data while timing driven route is active
    float* pin_criticality; /* [1..max_pins_per_net-1] */
    int* sink_order; /* [1..max_pins_per_net-1] */
    t_rt_node** rt_node_of_sink; /* [1..max_pins_per_net-1] */

    timing_driven_route_structs();
    ~timing_driven_route_structs();
};

void update_rr_base_costs(int fanout);
