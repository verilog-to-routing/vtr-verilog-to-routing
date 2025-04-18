
#pragma once

#include <memory>
#include <vector>

#include "netlist.h"

class PlaceDelayModel;
struct t_placer_opts;
struct t_router_opts;
struct t_det_routing_arch;
struct t_segment_inf;
struct t_chan_width_dist;
struct t_direct_inf;

class PlacementDelayModelCreator {
  public:
    // nothing to do in the constructor
    PlacementDelayModelCreator() = delete;

    static std::unique_ptr<PlaceDelayModel> create_delay_model(const t_placer_opts& placer_opts,
                                                               const t_router_opts& router_opts,
                                                               const Netlist<>& net_list,
                                                               t_det_routing_arch* det_routing_arch,
                                                               std::vector<t_segment_inf>& segment_inf,
                                                               t_chan_width_dist chan_width_dist,
                                                               const std::vector<t_direct_inf>& directs,
                                                               bool is_flat);
};
