#pragma once

/**
 * @file interposer_cut.h
 * @brief This file implements functions that:
 * (1) Marks all edges that cross an interposer cut for removal
 * (2) Makes the channel nodes that cross an interposer cut shorter to have them not cross the interposer anymore
 * Using these two functions and combined with 2D scatter-gather patterns, you can model and implement 2.5D FPGA RR Graphs.
 */

#include <vector>
#include "rr_graph_fwd.h"
#include "rr_graph_view.h"
#include "device_grid.h"

/**
 * @brief Goes through all edges in the RR Graph and returns a list of the edges that cross an interposer cut line.
 * 
 * @return std::vector<RREdgeId> List of all edges that cross an interposer cut line.
 */
std::vector<RREdgeId> mark_interposer_cut_edges_for_removal(const RRGraphView& rr_graph, const DeviceGrid& grid);

/**
 * @brief Shortens the channel nodes that cross an interposer cut line
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
