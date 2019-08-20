#ifndef TIMING_PLACE_LOOKUP_H
#define TIMING_PLACE_LOOKUP_H
#include "place_delay_model.h"

std::unique_ptr<PlaceDelayModel> compute_place_delay_model(const t_placer_opts& placer_opts,
                                                           const t_router_opts& router_opts,
                                                           t_det_routing_arch* det_routing_arch,
                                                           std::vector<t_segment_inf>& segment_inf,
                                                           t_chan_width_dist chan_width_dist,
                                                           const t_direct_inf* directs,
                                                           const int num_directs);

std::vector<int> get_best_classes(enum e_pin_type pintype, t_physical_tile_type_ptr type);
bool directconnect_exists(int src_rr_node, int sink_rr_node);

#endif
