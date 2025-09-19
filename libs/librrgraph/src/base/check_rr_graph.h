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
 * @brief Validates the internal consistency of a single RR node in the routing resource graph.
 *
 * This function performs a series of checks on the specified RR node to ensure it conforms
 * to architectural and routing constraints. It verifies:
 *  - That the node's bounding box is valid and within the device grid bounds.
 *  - That the node's PTC number, capacity, and cost index are within legal limits.
 *  - That IPINs, OPINs, SOURCEs, and SINKs correspond to valid physical locations and types.
 *  - That CHANX, CHANY, and CHANZ nodes have legal coordinate bounds and track indices.
 *  - That electrical characteristics (resistance and capacitance) are appropriate for the node type.
 *
 * Errors or inconsistencies will cause fatal errors or logged messages, depending on severity.
 *
 * @param rr_graph The read-only view of the routing resource graph.
 * @param rr_indexed_data Indexed data for RR node cost metrics.
 * @param grid The device grid.
 * @param chan_width The channel widths for different channels
 * @param route_type The routing type (GLOBAL or DETAILED).
 * @param inode The index of the RR node to be checked.
 * @param is_flat Flag indicating if flat routing is enabled.
 */
void check_rr_node(const RRGraphView& rr_graph,
                   const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                   const DeviceGrid& grid,
                   const VibDeviceGrid& vib_grid,
                   const t_chan_width& chan_width,
                   const e_route_type route_type,
                   const int inode,
                   bool is_flat);
