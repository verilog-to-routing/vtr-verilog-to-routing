#ifndef TIMING_PLACE
#define TIMING_PLACE

#include "timing_info_fwd.h"
#include "clustered_netlist_utils.h"
#include "place_delay_model.h"

std::unique_ptr<PlaceDelayModel> alloc_lookups_and_criticalities(t_chan_width_dist chan_width_dist,
                                                                 const t_placer_opts& place_opts,
                                                                 const t_router_opts& router_opts,
                                                                 t_det_routing_arch* det_routing_arch,
                                                                 std::vector<t_segment_inf>& segment_inf,
                                                                 const t_direct_inf* directs,
                                                                 const int num_directs);

void free_lookups_and_criticalities();

void load_criticalities(SetupTimingInfo& timing_info, float crit_exponent, const ClusteredPinAtomPinsLookup& pin_lookup);

float get_timing_place_crit(ClusterNetId net_id, int ipin);
void set_timing_place_crit(ClusterNetId net_id, int ipin, float val);

#endif
