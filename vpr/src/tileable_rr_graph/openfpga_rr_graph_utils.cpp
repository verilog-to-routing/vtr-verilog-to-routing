/********************************************************************
 * This file includes most utilized functions for the rr_graph
 * data structure in the OpenFPGA context
 *******************************************************************/
#include <algorithm>

/* Headers from vtrutil library */
#include "vtr_assert.h"
#include "vtr_log.h"

#include "openfpga_rr_graph_utils.h"
#include "rr_graph_types.h"

/* begin namespace openfpga */
namespace openfpga {

/************************************************************************
 * Get the coordinator of a starting point of a routing track 
 * For routing tracks in INC_DIRECTION
 * (xlow, ylow) should be the starting point 
 *
 * For routing tracks in DEC_DIRECTION
 * (xhigh, yhigh) should be the starting point 
 ***********************************************************************/
vtr::Point<size_t> get_track_rr_node_start_coordinate(const RRGraph& rr_graph,
                                                      const RRNodeId& track_rr_node) {
  /* Make sure we have CHANX or CHANY */
  VTR_ASSERT( (CHANX == rr_graph.node_type(track_rr_node))
           || (CHANY == rr_graph.node_type(track_rr_node)) );
 
  vtr::Point<size_t> start_coordinator;

  if (Direction::INC == rr_graph.node_direction(track_rr_node)) {
    start_coordinator.set(rr_graph.node_xlow(track_rr_node), rr_graph.node_ylow(track_rr_node));
  } else {
    VTR_ASSERT(Direction::DEC == rr_graph.node_direction(track_rr_node));
    start_coordinator.set(rr_graph.node_xhigh(track_rr_node), rr_graph.node_yhigh(track_rr_node));
  }

  return start_coordinator;
}

/************************************************************************
 * Get the coordinator of a end point of a routing track 
 * For routing tracks in INC_DIRECTION
 * (xhigh, yhigh) should be the starting point 
 *
 * For routing tracks in DEC_DIRECTION
 * (xlow, ylow) should be the starting point 
 ***********************************************************************/
vtr::Point<size_t> get_track_rr_node_end_coordinate(const RRGraph& rr_graph,
                                                    const RRNodeId& track_rr_node) {
  /* Make sure we have CHANX or CHANY */
  VTR_ASSERT( (CHANX == rr_graph.node_type(track_rr_node))
           || (CHANY == rr_graph.node_type(track_rr_node)) );
 
  vtr::Point<size_t> end_coordinator;

  if (Direction::INC == rr_graph.node_direction(track_rr_node)) {
    end_coordinator.set(rr_graph.node_xhigh(track_rr_node), rr_graph.node_yhigh(track_rr_node));
  } else {
    VTR_ASSERT(Direction::DEC == rr_graph.node_direction(track_rr_node));
    end_coordinator.set(rr_graph.node_xlow(track_rr_node), rr_graph.node_ylow(track_rr_node));
  }

  return end_coordinator;
}

/************************************************************************
 * Find the driver switches for a node in the rr_graph
 * This function only return unique driver switches
 ***********************************************************************/
std::vector<RRSwitchId> get_rr_graph_driver_switches(const RRGraph& rr_graph,
                                                     const RRNodeId& node) {
  std::vector<RRSwitchId> driver_switches;
  for (const RREdgeId& edge : rr_graph.node_in_edges(node)) {
    if (driver_switches.end() == std::find(driver_switches.begin(), driver_switches.end(), rr_graph.edge_switch(edge))) {
      driver_switches.push_back(rr_graph.edge_switch(edge));
    }
  }

  return driver_switches;
}

/************************************************************************
 * Find the driver nodes for a node in the rr_graph
 ***********************************************************************/
std::vector<RRNodeId> get_rr_graph_driver_nodes(const RRGraph& rr_graph,
                                                const RRNodeId& node) {
  std::vector<RRNodeId> driver_nodes;
  for (const RREdgeId& edge : rr_graph.node_in_edges(node)) {
    driver_nodes.push_back(rr_graph.edge_src_node(edge));
  }

  return driver_nodes;
}

/************************************************************************
 * Find the configurable driver nodes for a node in the rr_graph
 ***********************************************************************/
std::vector<RRNodeId> get_rr_graph_configurable_driver_nodes(const RRGraph& rr_graph,
                                                             const RRNodeId& node) {
  std::vector<RRNodeId> driver_nodes;
  for (const RREdgeId& edge : rr_graph.node_in_edges(node)) {
    /* Bypass non-configurable edges */
    if (false == rr_graph.edge_is_configurable(edge)) {
      continue;
    } 
    driver_nodes.push_back(rr_graph.edge_src_node(edge));
  }

  return driver_nodes;
}

/************************************************************************
 * Find the configurable driver nodes for a node in the rr_graph
 ***********************************************************************/
std::vector<RRNodeId> get_rr_graph_non_configurable_driver_nodes(const RRGraph& rr_graph,
                                                                 const RRNodeId& node) {
  std::vector<RRNodeId> driver_nodes;
  for (const RREdgeId& edge : rr_graph.node_in_edges(node)) {
    /* Bypass configurable edges */
    if (true == rr_graph.edge_is_configurable(edge)) {
      continue;
    } 
    driver_nodes.push_back(rr_graph.edge_src_node(edge));
  }

  return driver_nodes;
}

/************************************************************************
 * Check if an OPIN of a rr_graph is directly driving an IPIN
 * To meet this requirement, the OPIN must:
 *   - Have only 1 fan-out
 *   - The only fan-out is an IPIN
 ***********************************************************************/
bool is_opin_direct_connected_ipin(const RRGraph& rr_graph,
                                   const RRNodeId& node) {
  /* We only accept OPIN */
  VTR_ASSERT(OPIN == rr_graph.node_type(node));

  if (1 != rr_graph.node_out_edges(node).size()) {
    return false;
  }

  VTR_ASSERT(1 == rr_graph.node_out_edges(node).size());
  for (const RREdgeId& edge: rr_graph.node_out_edges(node)) {
    const RRNodeId& sink_node = rr_graph.edge_sink_node(edge);
    if (IPIN != rr_graph.node_type(sink_node)) {
      return false;
    }
  }
  
  return true;
}

/************************************************************************
 * Check if an IPIN of a rr_graph is directly connected to an OPIN
 * To meet this requirement, the IPIN must:
 *   - Have only 1 fan-in
 *   - The only fan-in is an OPIN
 ***********************************************************************/
bool is_ipin_direct_connected_opin(const RRGraph& rr_graph,
                                   const RRNodeId& node) {
  /* We only accept IPIN */
  VTR_ASSERT(IPIN == rr_graph.node_type(node));

  if (1 != rr_graph.node_in_edges(node).size()) {
    return false;
  }

  VTR_ASSERT(1 == rr_graph.node_in_edges(node).size());
  for (const RREdgeId& edge: rr_graph.node_in_edges(node)) {
    const RRNodeId& src_node = rr_graph.edge_src_node(edge);
    if (OPIN != rr_graph.node_type(src_node)) {
      return false;
    }
  }
  
  return true;
}

} /* end namespace openfpga */
