void try_place(struct s_placer_opts placer_opts,
	       struct s_annealing_sched annealing_sched,
	       t_chan_width_dist chan_width_dist,
	       struct s_router_opts router_opts,
	       struct s_det_routing_arch det_routing_arch,
	       t_segment_inf * segment_inf,
	       t_timing_inf timing_inf,
	       t_subblock_data * subblock_data_ptr,
	       t_mst_edge *** mst);

#if 0
/* Moved to read_place file */
void read_place(char *place_file,
		char *net_file,
		char *arch_file,
		struct s_placer_opts placer_opts,
		struct s_router_opts router_opts,
		t_chan_width_dist chan_width_dist,
		struct s_det_routing_arch det_routing_arch,
		t_segment_inf * segment_inf,
		t_timing_inf timing_inf,
		t_subblock_data * subblock_data_ptr,
		t_mst_edge *** mst);
#endif
