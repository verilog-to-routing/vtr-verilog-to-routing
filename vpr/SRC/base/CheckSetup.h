#ifndef CHECKSETUP_H
#define CHECKSETUP_H

void CheckSetup(const struct s_placer_opts PlacerOpts,
		const struct s_router_opts RouterOpts,
		const struct s_det_routing_arch RoutingArch,
		const t_segment_inf * Segments,
		const t_timing_inf Timing,
		const t_chan_width_dist Chans);

#endif
