#pragma once

/* Include header files which include data structures used by
 * the function declaration
 */
#include <vector>
#include "device_grid.h"
#include "rr_graph_view.h"

/* Get node-to-node switches in a RRGraph */
std::vector<RRSwitchId> find_rr_graph_switches(const RRGraphView& rr_graph,
                                               const RRNodeId& from_node,
                                               const RRNodeId& to_node);

std::vector<RRNodeId> find_rr_graph_nodes(const RRGraphView& rr_graph,
                                          const size_t& layer,
                                          const int& x,
                                          const int& y,
                                          const e_rr_type& rr_type,
                                          const int& ptc);

std::vector<RRNodeId> find_rr_graph_chan_nodes(const RRGraphView& rr_graph,
                                               const size_t& layer,
                                               const int& x,
                                               const int& y,
                                               const e_rr_type& rr_type);

std::vector<RRNodeId> find_rr_graph_grid_nodes(const RRGraphView& rr_graph,
                                               const DeviceGrid& device_grid,
                                               const size_t& layer,
                                               const int& x,
                                               const int& y,
                                               const e_rr_type& rr_type,
                                               const e_side& side,
                                               bool include_clock = false);
