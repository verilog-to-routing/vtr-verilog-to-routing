boolean try_timing_driven_route(struct s_router_opts router_opts,
		float **net_delay, t_slack * slacks, t_ivec ** clb_opins_used_locally,
		boolean timing_analysis_enabled);
boolean try_timing_driven_route_net(int inet, int itry, float pres_fac, 
		struct s_router_opts router_opts,
		float* pin_criticality, int* sink_order,
		t_rt_node** rt_node_of_sink, float** net_delay, t_slack* slacks);

boolean timing_driven_route_net(int inet, int itry, float pres_fac, float max_criticality,
		float criticality_exp, float astar_fac, float bend_cost,
		float *pin_criticality, int *sink_order, t_rt_node ** rt_node_of_sink, 
		float *net_delay, t_slack * slacks);
void alloc_timing_driven_route_structs(float **pin_criticality_ptr,
		int **sink_order_ptr, t_rt_node *** rt_node_of_sink_ptr);
void free_timing_driven_route_structs(float *pin_criticality, int *sink_order,
		t_rt_node ** rt_node_of_sink);
