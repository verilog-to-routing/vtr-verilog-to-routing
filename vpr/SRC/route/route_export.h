/******** Function prototypes for functions in route_common.c that ***********
 ******** are used outside the router modules.                     ***********/

void try_graph(int width_fac, struct s_router_opts router_opts,
		struct s_det_routing_arch det_routing_arch, t_segment_inf * segment_inf,
		t_timing_inf timing_inf, t_chan_width_dist chan_width_dist,
		t_direct_inf *directs, int num_directs);

boolean try_route(int width_fac, struct s_router_opts router_opts,
		struct s_det_routing_arch det_routing_arch, t_segment_inf * segment_inf,
		t_timing_inf timing_inf, float **net_delay, t_slack * slacks,
		t_chan_width_dist chan_width_dist, t_ivec ** clb_opins_used_locally,
		boolean * Fc_clipped, t_direct_inf *directs, int num_directs);

boolean feasible_routing(void);

t_ivec **alloc_route_structs(void);

void free_route_structs();

struct s_trace **alloc_saved_routing(t_ivec ** clb_opins_used_locally,
		t_ivec *** saved_clb_opins_used_locally_ptr);

void free_saved_routing(struct s_trace **best_routing,
		t_ivec ** saved_clb_opins_used_locally);

void save_routing(struct s_trace **best_routing,
		t_ivec ** clb_opins_used_locally,
		t_ivec ** saved_clb_opins_used_locally);

void restore_routing(struct s_trace **best_routing,
		t_ivec ** clb_opins_used_locally,
		t_ivec ** saved_clb_opins_used_locally);

void get_serial_num(void);

void print_route(char *name);
