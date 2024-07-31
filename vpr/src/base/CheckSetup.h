#ifndef CHECKSETUP_H
#define CHECKSETUP_H
#include "vpr_types.h"

const int DYMANIC_PORT_RANGE_MIN = 49152;
const int DYNAMIC_PORT_RANGE_MAX = 65535;

void CheckSetup(const t_packer_opts& PackerOpts,
                const t_placer_opts& PlacerOpts,
                const t_router_opts& RouterOpts,
                const t_server_opts& ServerOpts,
                const t_det_routing_arch& RoutingArch,
                const std::vector<t_segment_inf>& Segments,
                const t_timing_inf& Timing,
                const t_chan_width_dist Chans);

#endif
