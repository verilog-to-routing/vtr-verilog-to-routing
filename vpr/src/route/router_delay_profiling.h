#pragma once

#include "d_ary_heap.h"
#include "netlist.h"
#include "router_lookahead.h"
#include "router_stats.h"
#include "serial_connection_router.h"

#include <vector>

class RouterDelayProfiler {
  public:
    RouterDelayProfiler(const Netlist<>& net_list,
                        const RouterLookahead* lookahead,
                        bool is_flat);

    /**
     * @brief Returns true as long as found some way to hook up this net, even if that
     * way resulted in overuse of resources (congestion).  If there is no way
     * to route this net, even ignoring congestion, it returns false.  In this
     * case the rr_graph is disconnected and you can give up.
     * @return
     */
    bool calculate_delay(RRNodeId source_node,
                         RRNodeId sink_node,
                         const t_router_opts& router_opts,
                         float* net_delay);

    /**
     * @return Return the minimum delay across all output pins (OPINs) on the physical tile identified by "physical_tile_idx" from an
     * instance of the physical type on the "from_layer" to an input pin (IPIN) that is dx and dy away at its location on "to_layer".
     */
    float get_min_delay(int physical_tile_type_idx, int from_layer, int to_layer, int dx, int dy) const;

  private:
    const Netlist<>& net_list_;
    RouterStats router_stats_;
    SerialConnectionRouter<FourAryHeap> router_;
    vtr::NdMatrix<float, 5> min_delays_; // [physical_type_idx][from_layer][to_layer][dx][dy]
    bool is_flat_;
};

vtr::vector<RRNodeId, float> calculate_all_path_delays_from_rr_node(RRNodeId src_rr_node,
                                                                    const t_router_opts& router_opts,
                                                                    bool is_flat);

void alloc_routing_structs(const t_chan_width& chan_width,
                           const t_router_opts& router_opts,
                           t_det_routing_arch& det_routing_arch,
                           const std::vector<t_segment_inf>& segment_inf,
                           const std::vector<t_direct_inf>& directs,
                           bool is_flat);

void free_routing_structs();
