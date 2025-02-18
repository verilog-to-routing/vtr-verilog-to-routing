#ifndef CHECKSETUP_H
#define CHECKSETUP_H

#include "vpr_types.h"

void CheckSetup(const t_packer_opts& packer_opts,
                const t_placer_opts& placer_opts,
                const t_ap_opts& ap_opts,
                const t_router_opts& router_opts,
                const t_server_opts& server_opts,
                const t_det_routing_arch& routing_arch,
                const std::vector<t_segment_inf>& segments,
                const t_timing_inf& timing,
                const t_chan_width_dist& chans);

#endif
