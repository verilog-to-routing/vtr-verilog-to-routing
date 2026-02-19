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
