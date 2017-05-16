#ifndef CHECKSETUP_H
#define CHECKSETUP_H

void CheckSetup(const t_placer_opts PlacerOpts,
		const t_router_opts RouterOpts,
		const t_det_routing_arch RoutingArch,
		const t_segment_inf * Segments,
		const t_timing_inf Timing,
		const t_chan_width_dist Chans);

#endif
