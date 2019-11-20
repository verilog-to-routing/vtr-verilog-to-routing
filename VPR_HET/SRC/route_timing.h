boolean try_timing_driven_route(struct s_router_opts router_opts,
				float **net_slack,
				float **net_delay,
				t_ivec ** fb_opins_used_locally);
boolean timing_driven_route_net(int inet,
				float pres_fac,
				float max_criticality,
				float criticality_exp,
				float astar_fac,
				float bend_cost,
				float *net_slack,
				float *pin_criticality,
				int *sink_order,
				t_rt_node ** rt_node_of_sink,
				float T_crit,
				float *net_delay);
void alloc_timing_driven_route_structs(float **pin_criticality_ptr,
				       int **sink_order_ptr,
				       t_rt_node *** rt_node_of_sink_ptr);
void free_timing_driven_route_structs(float *pin_criticality,
				      int *sink_order,
				      t_rt_node ** rt_node_of_sink);
