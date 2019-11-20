void alloc_lookups_and_criticalities(t_chan_width_dist chan_width_dist,
				     struct s_router_opts router_opts,
				     struct s_det_routing_arch
				     det_routing_arch,
				     t_segment_inf * segment_inf,
				     t_timing_inf timing_inf,
				     t_subblock_data subblock_data,
				     float ***net_delay,
				     float ***net_slack);

void load_criticalities(struct s_placer_opts placer_opts,
			float **net_slack,
			float d_max,
			float crit_exponent);

void free_lookups_and_criticalities(float ***net_delay,
				    float ***net_slack);

/*void print_sink_delays(char *fname);*/
extern float **timing_place_crit;
