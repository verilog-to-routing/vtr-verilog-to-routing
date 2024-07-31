#ifndef ROUTER_DELAY_PROFILING_H_
#define ROUTER_DELAY_PROFILING_H_

#include "vpr_types.h"
#include "binary_heap.h"
#include "four_ary_heap.h"
#include "connection_router.h"

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
     * @param source_node
     * @param sink_node
     * @param router_opts
     * @param net_delay
     * @param layer_num
     * @return
     */
    bool calculate_delay(RRNodeId source_node,
                         RRNodeId sink_node,
                         const t_router_opts& router_opts,
                         float* net_delay,
                         int layer_num);

    /**
     * @param physical_tile_type_idx
     * @param from_layer
     * @param to_layer
     * @param dx
     * @param dy
     * @return Return the minimum delay across all output pins (OPINs) on the physical tile identified by "physical_tile_idx" from an
     * instance of the physical type on the "from_layer" to an input pin (IPIN) that is dx and dy away at its location on "to_layer".
     */
    float get_min_delay(int physical_tile_type_idx, int from_layer, int to_layer, int dx, int dy) const;

  private:
    const Netlist<>& net_list_;
    RouterStats router_stats_;
    ConnectionRouter<FourAryHeap> router_;
    vtr::NdMatrix<float, 5> min_delays_; // [physical_type_idx][from_layer][to_layer][dx][dy]
    bool is_flat_;
};

vtr::vector<RRNodeId, float> calculate_all_path_delays_from_rr_node(RRNodeId src_rr_node,
                                                                    const t_router_opts& router_opts,
                                                                    bool is_flat);

void alloc_routing_structs(const t_chan_width& chan_width,
                           const t_router_opts& router_opts,
                           t_det_routing_arch* det_routing_arch,
                           std::vector<t_segment_inf>& segment_inf,
                           const t_direct_inf* directs,
                           const int num_directs,
                           bool is_flat);

void free_routing_structs();

#endif /* ROUTER_DELAY_PROFILING_H_ */
