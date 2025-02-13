#ifndef VPR_PLACE_AND_ROUTE_H
#define VPR_PLACE_AND_ROUTE_H

#define INFINITE -1

#define WNEED 1
#define WL 2

#include "vpr_types.h"
#include "timing_info.h"
#include "RoutingDelayCalculator.h"
#include "rr_graph.h"

class PlaceMacros;

struct t_fmap_cell {
    int fs;         ///<at this fs
    int fc;         ///<at this fc
    int wneed;      ///<need wneed to route
    int wirelength; ///<corresponding wirelength of successful routing at wneed
    t_fmap_cell* next;
};

int binary_search_place_and_route(const Netlist<>& placement_net_list,
                                  const Netlist<>& router_net_list,
                                  const t_placer_opts& placer_opts_ref,
                                  const t_router_opts& router_opts,
                                  const t_analysis_opts& analysis_opts,
                                  const t_noc_opts& noc_opts,
                                  const t_file_name_opts& filename_opts,
                                  const t_arch* arch,
                                  const PlaceMacros& place_macros,
                                  bool verify_binary_search,
                                  int min_chan_width_hint,
                                  t_det_routing_arch* det_routing_arch,
                                  std::vector<t_segment_inf>& segment_inf,
                                  NetPinsMatrix<float>& net_delay,
                                  const std::shared_ptr<SetupHoldTimingInfo>& timing_info,
                                  const std::shared_ptr<RoutingDelayCalculator>& delay_calc,
                                  bool is_flat);

t_chan_width setup_chan_width(const t_router_opts& router_opts,
                              t_chan_width_dist chan_width_dist);

t_chan_width init_chan(int cfactor,
                       const t_chan_width_dist& chan_width_dist,
                       t_graph_type graph_directionality);

void post_place_sync();

#endif
