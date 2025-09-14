#pragma once

/**
 * @file tileable_rr_graph_node_builder.h
 * @brief This file contains functions that are used to allocate nodes
 * for the tileable routing resource graph builder
 */

#include "vtr_geometry.h"

#include "physical_types.h"

#include "device_grid.h"
#include "device_grid_annotation.h"
#include "rr_node_types.h"
#include "rr_graph_type.h"
#include "rr_graph_view.h"
#include "rr_graph_builder.h"

/**
 * @brief Allocate rr_nodes to a rr_graph object
 * @details This function just allocate the memory and ensure its efficiency
 *          It will NOT fill detailed information for each node!!!
 *          Note: ensure that there are NO nodes in the rr_graph
 */
void alloc_tileable_rr_graph_nodes(RRGraphBuilder& rr_graph_builder,
                                   vtr::vector<RRNodeId, RRSwitchId>& driver_switches,
                                   const DeviceGrid& grids,
                                   const VibDeviceGrid& vib_grid,
                                   const size_t& layer,
                                   const vtr::Point<size_t>& chan_width,
                                   const std::vector<t_segment_inf>& segment_inf_x,
                                   const std::vector<t_segment_inf>& segment_inf_y,
                                   const DeviceGridAnnotation& device_grid_annotation,
                                   const bool shrink_boundary,
                                   const bool perimeter_cb,
                                   const bool through_channel);

void create_tileable_rr_graph_nodes(const RRGraphView& rr_graph,
                                    RRGraphBuilder& rr_graph_builder,
                                    vtr::vector<RRNodeId, RRSwitchId>& rr_node_driver_switches,
                                    std::map<RRNodeId, std::vector<size_t>>& rr_node_track_ids,
                                    std::vector<t_rr_rc_data>& rr_rc_data,
                                    const DeviceGrid& grids,
                                    const VibDeviceGrid& vib_grid,
                                    const size_t& layer,
                                    const vtr::Point<size_t>& chan_width,
                                    const std::vector<t_segment_inf>& segment_inf_x,
                                    const std::vector<t_segment_inf>& segment_inf_y,
                                    const t_unified_to_parallel_seg_index& segment_index_map,
                                    const RRSwitchId& wire_to_ipin_switch,
                                    const RRSwitchId& delayless_switch,
                                    const DeviceGridAnnotation& device_grid_annotation,
                                    const bool shrink_boundary,
                                    const bool perimeter_cb,
                                    const bool through_channel);
