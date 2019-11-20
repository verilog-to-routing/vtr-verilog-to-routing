boolean IsTimingEnabled(IN t_options Options);

void SetupVPR(IN t_options Options,
	      IN boolean TimingEnabled,
	      OUT t_arch * Arch,
	      OUT enum e_operation *Operation,
	      OUT struct s_placer_opts *PlacerOpts,
	      OUT struct s_annealing_sched *AnnealSched,
	      OUT struct s_router_opts *RouterOpts,
	      OUT struct s_det_routing_arch *RoutingArch,
	      OUT t_segment_inf ** Segments,
	      OUT t_timing_inf * Timing,
	      OUT t_subblock_data * Subblocks,
	      OUT boolean * ShowGraphics,
	      OUT int *GraphPause);

void CheckSetup(IN enum e_operation Operation,
		IN struct s_placer_opts PlacerOpts,
		IN struct s_annealing_sched AnnealSched,
		IN struct s_router_opts RouterOpts,
		IN struct s_det_routing_arch RoutingArch,
		IN t_segment_inf * Segments,
		IN t_timing_inf Timing,
		IN t_subblock_data Subblocks,
		IN t_chan_width_dist Chans);

void CheckArch(IN t_arch Arch,
	       IN boolean TimingEnabled);

void CheckOptions(IN t_options Options,
		  IN boolean TimingEnabled);

void ShowSetup(IN t_options Options,
	       IN t_arch Arch,
	       IN boolean TimingEnabled,
	       IN enum e_operation Operation,
	       IN struct s_placer_opts PlacerOpts,
	       IN struct s_annealing_sched AnnealSched,
	       IN struct s_router_opts RouterOpts,
	       IN struct s_det_routing_arch RoutingArch,
	       IN t_segment_inf * Segments,
	       IN t_timing_inf Timing,
	       IN t_subblock_data Subblocks);
