#pragma once

/**
 * @file interposer_cut.h
 * @brief This file implements functions that:
 *
 * (1) Get a list of all edges that cross an interposer cut for removal. Edges whose source and sink nodes start on opposite
 * sides of an interposer cut line are considered to cross the cut line and will be removed.
 *
 * (2) Makes the channel nodes that cross an interposer cut shorter to have them not cross the interposer anymore
 *
 * Using these two functions and combined with 2D scatter-gather patterns, you can model and implement 2.5D FPGA RR Graphs.
 *
 * Below you can see a diagram of a device with a vertical interposer cut at x = 2 (Shown with X) and a horizontal interposer cut at y = 2 (shown with =)
 * X(x,y) denotes the ChanX at location (x,y) and Y(x,y) denotes the ChanX at location (x,y). Consistent with VPR's coordinate system, each tile "owns"
 * the channels above and to the right of it, therefore interposer cut lines will leave the channels on the side of the tile it belongs to.
 *
 *
 *     X(0,3)           X(1,3)        X  X(2,3)           X(3,3)           X(4,3)
 *    _________        _________      X _________        _________        _________
 *    |       |        |       |      X |       |        |       |        |       |
 *    |       | Y(0,3) |       |Y(1,3)X |       | Y(2,3) |       | Y(3,3) |       | Y(4,3)
 *    |       |        |       |      X |       |        |       |        |       |
 *    ---------        ---------      X ---------        ---------        ---------
 *                                    X
 *     X(0,2)           X(1,2)        X  X(2,2)           X(3,2)           X(4,2)
 *                                    X
 *    _________        _________      X _________        _________        _________
 *    |       |        |       |      X |       |        |       |        |       |
 *    |       | Y(0,2) |       |Y(1,2)X |       | Y(2,2) |       | Y(3,2) |       | Y(4,2)
 *    |       |        |       |      X |       |        |       |        |       |
 *    ---------        ---------      X ---------        ---------        ---------
 * ===================================X=================================================
 *     X(0,1)           X(1,1)        X  X(2,1)           X(3,1)           X(4,1)
 *                                    X
 *    _________        _________      X _________        _________        _________
 *    |       |        |       |      X |       |        |       |        |       |
 *    |       | Y(0,1) |       |Y(1,1)X |       | Y(2,1) |       | Y(3,1) |       | Y(4,1)
 *    |       |        |       |      X |       |        |       |        |       |
 *    ---------        ---------      X ---------        ---------        ---------
 *                                    X
 *     X(0,0)           X(1,0)        X  X(2,0)           X(3,0)           X(4,0)
 *                                    X
 *    _________        _________      X _________        _________        _________
 *    |       |        |       |      X |       |        |       |        |       |
 *    |       | Y(0,0) |       |Y(1,0)X |       | Y(2,0) |       | Y(3,0) |       | Y(4,0)
 *    |       |        |       |      X |       |        |       |        |       |
 *    ---------        ---------      X ---------        ---------        ---------
 *                                    X
 *                                    X
 *
 *
 */

#include <vector>
#include "rr_graph_fwd.h"
#include "rr_graph_view.h"
#include "device_grid.h"

/**
 * @brief Goes through all edges in the RR Graph and returns a list of the edges that cross an interposer cut line.
 * Edges whose source and sink nodes start on opposite sides of an interposer cut line are considered to cross the
 * cut line and will be removed.
 * 
 * @return std::vector<RREdgeId> List of all edges that cross an interposer cut line.
 */
std::vector<RREdgeId> get_interposer_cut_edges_for_removal(const RRGraphView& rr_graph, const DeviceGrid& grid);

/**
 * @brief Shortens the channel nodes that cross an interposer cut line. 
 * 
 * @details When a wire crosses an interposer cut line, it is shortened to instead end at the interposed cut line
 * unless it is in the list of sg_node_indices. The wire's drive point and the wire up to the interposer cut is
 * retained and will not be modified. "Shortening a wire" in this context means modifying the information in the
 * RR Graph itself (Changing the bounding box of the corresponding RR Graph node) and the spatial lookup
 * (Making sure there is no reference to the shortened wire in locations it doesn't exist at anymore.)
 *
 * @param rr_graph RRGraphView, used to read the RR Graph.
 * @param grid Device grid, used to access interposer cut locations.
 * @param rr_graph_builder RRGraphBuilder, to modify the RRGraph.
 * @param sg_node_indices list of scatter-gather node IDs. We do not want to cut these nodes as they're allowed to cross an interposer cut line.
 */
void update_interposer_crossing_nodes_in_spatial_lookup_and_rr_graph_storage(const RRGraphView& rr_graph,
                                                                             const DeviceGrid& grid,
                                                                             RRGraphBuilder& rr_graph_builder,
                                                                             const std::vector<std::pair<RRNodeId, int>>& sg_node_indices);
