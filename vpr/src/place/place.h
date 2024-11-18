#ifndef VPR_PLACE_H
#define VPR_PLACE_H

#include "vpr_types.h"

void try_place(const Netlist<>& net_list,
               const t_placer_opts& placer_opts,
               const t_router_opts& router_opts,
               const t_analysis_opts& analysis_opts,
               const t_noc_opts& noc_opts,
               t_chan_width_dist chan_width_dist,
               t_det_routing_arch* det_routing_arch,
               std::vector<t_segment_inf>& segment_inf,
               const std::vector<t_direct_inf>& directs,
               bool is_flat);

#endif
