#ifndef CHECK_RR_GRAPH_H
#define CHECK_RR_GRAPH_H

#include "device_grid.h"
#include "rr_graph_view.h"
#include "rr_graph_type.h"

void check_rr_graph(const RRGraphView& rr_graph,
                    const std::vector<t_physical_tile_type>& types,
                    const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                    const DeviceGrid& grid,
                    const t_chan_width& chan_width,
                    const t_graph_type graph_type,
                    const int virtual_clock_network_root_idx,
                    bool is_flat);

void check_rr_node(const RRGraphView& rr_graph,
                   const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                   const DeviceGrid& grid,
                   const t_chan_width& chan_width,
                   const enum e_route_type route_type,
                   const int inode,
                   bool is_flat);

#endif
