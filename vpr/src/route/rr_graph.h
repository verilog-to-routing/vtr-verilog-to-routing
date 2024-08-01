#ifndef RR_GRAPH_H
#define RR_GRAPH_H

/* Include track buffers or not. Track buffers isolate the tracks from the
 * input connection block. However, they are difficult to lay out in practice,
 * and so are not currently used in commercial architectures. */
#define INCLUDE_TRACK_BUFFERS false

#include "device_grid.h"
#include "vpr_types.h"
#include "rr_graph_type.h"
#include "describe_rr_node.h"

/* Warnings about the routing graph that can be returned.
 * This is to avoid output messages during a value sweep */
enum {
    RR_GRAPH_NO_WARN = 0x00,
    RR_GRAPH_WARN_FC_CLIPPED = 0x01,
    RR_GRAPH_WARN_CHAN_X_WIDTH_CHANGED = 0x02,
    RR_GRAPH_WARN_CHAN_Y_WIDTH_CHANGED = 0x03
};

void create_rr_graph(const t_graph_type graph_type,
                     const std::vector<t_physical_tile_type>& block_types,
                     const DeviceGrid& grid,
                     const t_chan_width& nodes_per_chan,
                     t_det_routing_arch* det_routing_arch,
                     const std::vector<t_segment_inf>& segment_inf,
                     const t_router_opts& router_opts,
                     const t_direct_inf* directs,
                     const int num_directs,
                     int* Warnings,
                     bool is_flat);

// Build a complete RR graph, including all modes, for the given tile. This is used by router lookahead when
// flat-routing is enabled. It allows it to store the cost from the border of a tile to a sink inside of it
void build_tile_rr_graph(RRGraphBuilder& rr_graph_builder,
                         const t_det_routing_arch& det_routing_arch,
                         t_physical_tile_type_ptr physical_tile,
                         int layer,
                         int x,
                         int y,
                         const int delayless_switch);

void free_rr_graph();

t_rr_switch_inf create_rr_switch_from_arch_switch(const t_arch_switch_inf& arch_sw_inf,
                                                  const float R_minW_nmos,
                                                  const float R_minW_pmos);
// Sets the spec for the rr_switch based on the arch switch
void load_rr_switch_from_arch_switch(RRGraphBuilder& rr_graph_builder,
                                     const std::map<int, t_arch_switch_inf>& arch_sw_inf,
                                     int arch_switch_idx,
                                     int rr_switch_idx,
                                     int fanin,
                                     const float R_minW_nmos,
                                     const float R_minW_pmos);

t_non_configurable_rr_sets identify_non_configurable_rr_sets();

bool pins_connected(t_block_loc cluster_loc,
                    t_physical_tile_type_ptr physical_type,
                    t_logical_block_type_ptr logical_block,
                    int from_pin_logical_num,
                    int to_pin_logical_num);

#endif
