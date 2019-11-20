void place_and_route (enum e_operation operation, struct s_placer_opts
   placer_opts, char *place_file, char *net_file, char *arch_file,
   char *route_file, boolean full_stats, boolean verify_binary_search,
   struct s_annealing_sched annealing_sched, struct s_router_opts router_opts,
   struct s_det_routing_arch det_routing_arch, t_segment_inf *segment_inf,
   t_timing_inf timing_inf, t_subblock_data *subblock_data_ptr, 
   t_chan_width_dist chan_width_dist);

void init_chan (int cfactor, t_chan_width_dist chan_width_dist);
