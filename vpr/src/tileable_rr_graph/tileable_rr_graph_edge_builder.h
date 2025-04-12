#ifndef TILEABLE_RR_GRAPH_EDGE_BUILDER_H
#define TILEABLE_RR_GRAPH_EDGE_BUILDER_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <vector>

/* Headers from vtrutil library */
#include "vtr_ndmatrix.h"
#include "vtr_geometry.h"

#include "physical_types.h"
#include "device_grid.h"
#include "rr_graph_obj.h"
#include "rr_graph_type.h"
#include "rr_graph_view.h"
#include "rr_graph.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

void build_rr_graph_edges(const RRGraphView& rr_graph,
                          RRGraphBuilder& rr_graph_builder,
                          const vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches,
                          const DeviceGrid& grids,
                          const size_t& layer,
                          const vtr::Point<size_t>& device_chan_width,
                          const std::vector<t_segment_inf>& segment_inf,
                          const std::vector<t_segment_inf>& segment_inf_x,
                          const std::vector<t_segment_inf>& segment_inf_y,
                          const std::vector<vtr::Matrix<int>>& Fc_in,
                          const std::vector<vtr::Matrix<int>>& Fc_out,
                          const e_switch_block_type& sb_type,
                          const int& Fs,
                          const e_switch_block_type& sb_subtype,
                          const int& subFs,
                          const bool& wire_opposite_side);

void build_rr_graph_direct_connections(const RRGraphView& rr_graph,
                                       RRGraphBuilder& rr_graph_builder,
                                       const DeviceGrid& grids,
                                       const size_t& layer,
                                       const RRSwitchId& delayless_switch,
                                       const std::vector<t_direct_inf>& directs,
                                       const std::vector<t_clb_to_clb_directs>& clb_to_clb_directs);

void build_rr_graph_edges_for_source_nodes(const RRGraphView& rr_graph,
                                           RRGraphBuilder& rr_graph_builder,
                                           const vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches,
                                           const DeviceGrid& grids,
                                           const size_t& layer,
                                           size_t& num_edges_to_create);

void build_rr_graph_edges_for_sink_nodes(const RRGraphView& rr_graph,
                                         RRGraphBuilder& rr_graph_builder,
                                         const vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches,
                                         const DeviceGrid& grids,
                                         const size_t& layer,
                                         size_t& num_edges_to_create);

#endif
