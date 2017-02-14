#ifndef TIMING_PLACE
#define TIMING_PLACE

#include "timing_info_fwd.h"

t_slack * alloc_lookups_and_criticalities(t_chan_width_dist chan_width_dist,
		struct s_router_opts router_opts,
		struct s_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
		t_timing_inf timing_inf, const t_direct_inf *directs, 
		const int num_directs);

void free_lookups_and_criticalities(t_slack * slacks);

void print_sink_delays(const char *fname);

void load_criticalities(SetupTimingInfo& timing_info, float crit_exponent, const IntraLbPbPinLookup& pb_gpin_lookup);

extern float **timing_place_crit;

#endif
