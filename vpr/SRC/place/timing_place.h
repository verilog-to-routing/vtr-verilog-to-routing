#ifndef TIMING_PLACE
#define TIMING_PLACE

#include "timing_info_fwd.h"

void alloc_lookups_and_criticalities(t_chan_width_dist chan_width_dist,
		t_router_opts router_opts,
		t_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
		const t_direct_inf *directs, 
		const int num_directs);

void free_lookups_and_criticalities();

void print_sink_delays(const char *fname);

void load_criticalities(SetupTimingInfo& timing_info, float crit_exponent, const IntraLbPbPinLookup& pb_gpin_lookup);


float get_timing_place_crit(int inet, int ipin);
void set_timing_place_crit(int inet, int ipin, float val);

#endif
