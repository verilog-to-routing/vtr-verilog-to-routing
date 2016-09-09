#pragma once
#include <unordered_map>
#include <vector>
#include "connection_based_routing.h"

int get_max_pins_per_net(void);

bool try_timing_driven_route(struct s_router_opts router_opts,
		float **net_delay, t_slack * slacks, vtr::t_ivec ** clb_opins_used_locally,
        bool timing_analysis_enabled, const t_timing_inf &timing_inf);
bool try_timing_driven_route_net(int inet, int itry, float pres_fac, 
		struct s_router_opts router_opts,
		CBRR& connections_inf,
		float* pin_criticality, 
		t_rt_node** rt_node_of_sink, float** net_delay, t_slack* slacks);

bool timing_driven_route_net(int inet, int itry, float pres_fac, float max_criticality,
		float criticality_exp, float astar_fac, float bend_cost,
		CBRR& connections_inf,
		float *pin_criticality, int min_incremental_reroute_fanout, t_rt_node ** rt_node_of_sink, 
		float *net_delay, t_slack * slacks);


void alloc_timing_driven_route_structs(float **pin_criticality_ptr,
		int **sink_order_ptr, t_rt_node *** rt_node_of_sink_ptr);
void free_timing_driven_route_structs(float *pin_criticality, int *sink_order,
		t_rt_node ** rt_node_of_sink);

struct timing_driven_route_structs {
	// data while timing driven route is active 
	float* pin_criticality; /* [1..max_pins_per_net-1] */
	int* sink_order; /* [1..max_pins_per_net-1] */
	t_rt_node** rt_node_of_sink; /* [1..max_pins_per_net-1] */

	timing_driven_route_structs();
	~timing_driven_route_structs();
};
