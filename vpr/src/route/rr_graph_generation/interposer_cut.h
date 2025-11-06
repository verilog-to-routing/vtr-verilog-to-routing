#pragma once

#include <vector>
#include "rr_graph_fwd.h"
#include "rr_graph_view.h"
#include "device_grid.h"

std::vector<RREdgeId> mark_interposer_cut_edges_for_removal(const RRGraphView& rr_graph, const DeviceGrid& grid);

void update_interposer_crossing_nodes_in_spatial_lookup_and_rr_graph_storage(const RRGraphView& rr_graph, const DeviceGrid& grid, RRGraphBuilder& rr_graph_builder, const std::vector<std::pair<RRNodeId, int>>& sg_node_indices);