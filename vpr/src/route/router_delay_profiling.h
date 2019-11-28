#ifndef ROUTER_DELAY_PROFILING_H_
#define ROUTER_DELAY_PROFILING_H_

#include "vpr_types.h"
#include "router_lookahead.h"

#include <vector>

class RouterDelayProfiler {
  public:
    RouterDelayProfiler(const RouterLookahead* lookahead);
    bool calculate_delay(int source_node, int sink_node, const t_router_opts& router_opts, float* net_delay) const;

  private:
    const RouterLookahead* router_lookahead_;
};

std::vector<float> calculate_all_path_delays_from_rr_node(int src_rr_node, const t_router_opts& router_opts);

void alloc_routing_structs(t_chan_width chan_width,
                           const t_router_opts& router_opts,
                           t_det_routing_arch* det_routing_arch,
                           std::vector<t_segment_inf>& segment_inf,
                           const t_direct_inf* directs,
                           const int num_directs);

void free_routing_structs();

#endif /* ROUTER_DELAY_PROFILING_H_ */
