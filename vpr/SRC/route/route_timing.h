#pragma once
#include <unordered_map>
#include <vector>

bool try_timing_driven_route(struct s_router_opts router_opts,
		float **net_delay, t_slack * slacks, t_ivec ** clb_opins_used_locally,
        bool timing_analysis_enabled, const t_timing_inf &timing_inf);
bool try_timing_driven_route_net(int inet, int itry, float pres_fac, 
		struct s_router_opts router_opts,
		float* pin_criticality, 
		t_rt_node** rt_node_of_sink, float** net_delay, t_slack* slacks);

bool timing_driven_route_net(int inet, int itry, float pres_fac, float max_criticality,
		float criticality_exp, float astar_fac, float bend_cost,
		float *pin_criticality, int min_incremental_reroute_fanout, t_rt_node ** rt_node_of_sink, 
		float *net_delay, t_slack * slacks);

/*
 * NOTE:
 * Suggest using a timing_driven_route_structs struct (below) . Memory is managed for you.
 */
void alloc_timing_driven_route_structs(float **pin_criticality_ptr,
		int **sink_order_ptr, t_rt_node *** rt_node_of_sink_ptr);
void free_timing_driven_route_structs(float *pin_criticality, int *sink_order,
		t_rt_node ** rt_node_of_sink);

struct timing_driven_route_structs {
	float* pin_criticality;
	int* sink_order;
	t_rt_node** rt_node_of_sink;

	timing_driven_route_structs();
	~timing_driven_route_structs();
};

// connection based functions
// const alias on the lookup table as my testing showed twice as fast reads compared to using global
void convert_sink_node_to_pins_of_net(const std::unordered_map<int,int>& node_to_pin_mapping, std::vector<int>& sink_nodes);
void put_sink_rt_nodes_to_pins_lookup(const std::unordered_map<int, int>& node_to_pin_mapping, const std::vector<s_rt_node*>& sink_rt_node,
	 t_rt_node** rt_node_of_sink);

// profiling functions
#ifdef PROFILE
void time_on_fanout_analysis();
#endif