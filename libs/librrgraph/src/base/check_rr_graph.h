#pragma once

#include "device_grid.h"
#include "rr_graph_view.h"
#include "rr_graph_type.h"

void check_rr_graph(const RRGraphView& rr_graph,
                    const std::vector<t_physical_tile_type>& types,
                    const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                    const DeviceGrid& grid,
                    const VibDeviceGrid& vib_grid,
                    const t_chan_width& chan_width,
                    const e_graph_type graph_type,
                    bool is_flat);

/**
 * @brief Checks the validity of the given node in the RR graph.
 * 
 * @note This function checks for the following:
 *       - The node's location is within the legal range.
 *       - The node's capacity and other properties are consistent with the node's type.
 *       - The node's type is valid.
 *       - The node's capacity is consistent with the node's type.
 *       - The node's capacitance and resistance are non-negative.
 * 
 * @note vib_grid is added and used only of VIB architecture is used. This data strucutre
 *       is used to check for MUX node type PTC number. Ideally, this should be removed and
 *       ** grid ** should be used instead.
 * 
 */
void check_rr_node(const RRGraphView& rr_graph,
                   const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                   const DeviceGrid& grid,
                   const VibDeviceGrid& vib_grid,
                   const t_chan_width& chan_width,
                   const enum e_route_type route_type,
                   const int inode,
                   bool is_flat);
