bool try_breadth_first_route(struct s_router_opts router_opts,
		vtr::t_ivec ** clb_opins_used_locally, int width_fac);
bool try_breadth_first_route_net(int inet, int itry, float pres_fac, 
		struct s_router_opts router_opts);
