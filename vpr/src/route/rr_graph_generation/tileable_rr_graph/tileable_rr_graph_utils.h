#pragma once

/**
 * @file tileable_rr_graph_utils.cpp
 * @brief This file includes most utilized functions for the rr_graph
 * data structure in the OpenFPGA context
 */

#include "vtr_geometry.h"

/* Headers from vpr library */
#include "rr_graph_obj.h"
#include "rr_graph_view.h"

/**
 * @brief Get the coordinator of a starting point of a routing track 
 * For routing tracks in INC_DIRECTION
 * (xlow, ylow) should be the starting point 
 * For routing tracks in DEC_DIRECTION
 * (xhigh, yhigh) should be the starting point 
 */
vtr::Point<size_t> get_track_rr_node_start_coordinate(const RRGraphView& rr_graph,
                                                      const RRNodeId& track_rr_node);

/**
 * @brief Get the coordinator of a end point of a routing track 
 * For routing tracks in INC_DIRECTION
 * (xhigh, yhigh) should be the starting point 
 * For routing tracks in DEC_DIRECTION
 * (xlow, ylow) should be the starting point 
 */
vtr::Point<size_t> get_track_rr_node_end_coordinate(const RRGraphView& rr_graph,
                                                    const RRNodeId& track_rr_node);

/**
 * @brief Find the driver switches for a node in the rr_graph
 * This function only return unique driver switches
 */
std::vector<RRSwitchId> get_rr_graph_driver_switches(const RRGraphView& rr_graph,
                                                     const RRNodeId node);

/**
 * @brief Find the driver nodes for a node in the rr_graph
 */
std::vector<RRNodeId> get_rr_graph_driver_nodes(const RRGraphView& rr_graph,
                                                const RRNodeId node);

/**
 * @brief Find the configurable driver nodes for a node in the rr_graph
 */
std::vector<RRNodeId> get_rr_graph_configurable_driver_nodes(const RRGraphView& rr_graph,
                                                             const RRNodeId node);

/**
 * @brief Find the configurable driver nodes for a node in the rr_graph
 */
std::vector<RRNodeId> get_rr_graph_non_configurable_driver_nodes(const RRGraphView& rr_graph,
                                                                 const RRNodeId node);

/**
 * @brief Check if an OPIN of a rr_graph is directly driving an IPIN
 * To meet this requirement, the OPIN must:
 *   - Have only 1 fan-out
 *   - The only fan-out is an IPIN 
 */
bool is_opin_direct_connected_ipin(const RRGraphView& rr_graph,
                                   const RRNodeId node);

/**
 * @brief Check if an IPIN of a rr_graph is directly connected to an OPIN
 * To meet this requirement, the IPIN must:
 *   - Have only 1 fan-in
 *   - The only fan-in is an OPIN 
 */
bool is_ipin_direct_connected_opin(const RRGraphView& rr_graph,
                                   const RRNodeId node);

/**
 * @brief Get a side of a given node in a routing resource graph.
 * Note that this function expect one valid side to be got. Otherwise, it will fail!
 */
e_side get_rr_graph_single_node_side(const RRGraphView& rr_graph,
                                     const RRNodeId node);
