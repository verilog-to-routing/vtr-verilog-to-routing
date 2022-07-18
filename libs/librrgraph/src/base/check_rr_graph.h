#ifndef CHECK_RR_GRAPH_H
#define CHECK_RR_GRAPH_H

#include "physical_types.h"
//#include "vpr_context.h"

#include "graph_type.h"
#include "device_grid.h"
#include "route_type.h"
#include "rr_graph_view.h"
#include "chan_width.h"


void check_rr_graph(const t_graph_type graph_type,
                    const DeviceGrid& grid,
                    const std::vector<t_physical_tile_type>& types,
                    const RRGraphView& rr_graph,
                    const vtr::vector<RRIndexedDataId, t_rr_indexed_data> rr_indexed_data,
                    const t_chan_width& chan_width,
                    int virtual_clock_network_root_idx);

void check_rr_node(int inode, 
                   enum e_route_type route_type, 
                   const RRGraphView& rr_graph,
                   const DeviceGrid& grid,
                   const vtr::vector<RRIndexedDataId, t_rr_indexed_data> rr_indexed_data,
                   const t_chan_width& chan_width);

#endif
