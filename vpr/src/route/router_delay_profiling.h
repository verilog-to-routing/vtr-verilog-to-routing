#ifndef ROUTER_DELAY_PROFILING_H_
#define ROUTER_DELAY_PROFILING_H_

#include "vpr_types.h"
#include "route_timing.h"
#include "binary_heap.h"
#include "connection_router.h"

#include <vector>

class RouterDelayProfiler {
  public:
    RouterDelayProfiler(const Netlist<>& net_list,
                        const RouterLookahead* lookahead,
                        bool is_flat);
    bool calculate_delay(RRNodeId source_node, RRNodeId sink_node, const t_router_opts& router_opts, float* net_delay);

  private:
    const Netlist<>& net_list_;
    RouterStats router_stats_;
    ConnectionRouter<BinaryHeap> router_;
    bool is_flat_;
};

vtr::vector<RRNodeId, float> calculate_all_path_delays_from_rr_node(RRNodeId src_rr_node,
                                                                    const t_router_opts& router_opts,
                                                                    bool is_flat);

void alloc_routing_structs(t_chan_width chan_width,
                           const t_router_opts& router_opts,
                           t_det_routing_arch* det_routing_arch,
                           std::vector<t_segment_inf>& segment_inf,
                           const t_direct_inf* directs,
                           const int num_directs,
                           bool is_flat);

void free_routing_structs();

#endif /* ROUTER_DELAY_PROFILING_H_ */
