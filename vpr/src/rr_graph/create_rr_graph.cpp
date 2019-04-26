/* Standard header files required go first */
#include <map>

/* EXTERNAL library header files go second*/
#include "vtr_assert.h"

/* VPR header files go second*/
#include "vpr_types.h"
#include "rr_graph_obj.h"
#include "create_rr_graph.h"

/* Finally we include global variables */
#include "globals.h"


/* TODO: remove when this conversion (from traditional to new data structure) is no longer needed
 * This function will convert an existing rr_graph in device_ctx to the RRGraph object
 * This function is used to test our RRGraph if it is acceptable in downstream routers  
 */
void convert_rr_graph() {
  auto& device_ctx = g_vpr_ctx.mutable_device();

  /* TODO: make sure we have a clean empty rr_graph */
  /* We need an empty() function ! 
   */
  device_ctx.rr_graph.clear(); 

  /* The number of switches are in general small, 
   * reserve switches may not bring significant memory efficiency 
   * So, we just use create_switch to push_back each time 
   */
  device_ctx.rr_graph.reserve_switches(device_ctx.rr_switch_inf.size());
  //Create the switches
  for (size_t iswitch = 0; iswitch < device_ctx.rr_switch_inf.size(); ++iswitch) {
    device_ctx.rr_graph.create_switch(device_ctx.rr_switch_inf[iswitch]);
  }

  /* The number of segments are in general small, reserve segments may not bring significant memory efficiency */
  device_ctx.rr_graph.reserve_segments(device_ctx.arch->Segments.size());
  //Create the segments
  for (size_t iseg = 0; iseg < device_ctx.arch->Segments.size(); ++iseg) {
    device_ctx.rr_graph.create_segment(device_ctx.arch->Segments[iseg]);
  }

  /* Reserve list of nodes to be memory efficient */
  device_ctx.rr_graph.reserve_nodes(device_ctx.rr_nodes.size());

  //Create the nodes
  std::map<int,RRNodeId> old_to_new_rr_node;
  for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); ++inode) {
    auto& node = device_ctx.rr_nodes[inode];
    RRNodeId rr_node = device_ctx.rr_graph.create_node(node.type());

    device_ctx.rr_graph.set_node_xlow(rr_node, node.xlow());
    device_ctx.rr_graph.set_node_ylow(rr_node, node.ylow());
    device_ctx.rr_graph.set_node_xhigh(rr_node, node.xhigh());
    device_ctx.rr_graph.set_node_yhigh(rr_node, node.yhigh());

    device_ctx.rr_graph.set_node_capacity(rr_node, node.capacity());

    device_ctx.rr_graph.set_node_ptc_num(rr_node, node.ptc_num());

    device_ctx.rr_graph.set_node_cost_index(rr_node, node.cost_index());

    if ( CHANX == node.type() || CHANY == node.type() ) {
      device_ctx.rr_graph.set_node_direction(rr_node, node.direction());
    }
    if ( IPIN == node.type() || OPIN == node.type() ) {
      device_ctx.rr_graph.set_node_side(rr_node, node.side());
    }
    device_ctx.rr_graph.set_node_R(rr_node, node.R());
    device_ctx.rr_graph.set_node_C(rr_node, node.C());

    /* Set up segment id */
    short irc_data = node.cost_index();
    short iseg = device_ctx.rr_indexed_data[irc_data].seg_index;
    device_ctx.rr_graph.set_node_segment_id(rr_node, iseg);
    
    VTR_ASSERT(!old_to_new_rr_node.count(inode));
    old_to_new_rr_node[inode] = rr_node;
  }

  /* Reserve list of edges to be memory efficient */
  {
    int num_edges_to_reserve = 0; 
    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); ++inode) {
        const auto& node = device_ctx.rr_nodes[inode];
        num_edges_to_reserve += node.num_edges();
    }
    device_ctx.rr_graph.reserve_edges(num_edges_to_reserve);
  }

  //Create the edges
  std::map<std::pair<int,int>,RREdgeId> old_to_new_rr_edge; //Key: {inode,iedge}
  for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); ++inode) {
    const auto& node = device_ctx.rr_nodes[inode];
    for (int iedge = 0; iedge < node.num_edges(); ++iedge) {
    
      int isink_node = node.edge_sink_node(iedge);
      int iswitch = node.edge_switch(iedge);

      VTR_ASSERT(old_to_new_rr_node.count(inode));
      VTR_ASSERT(old_to_new_rr_node.count(isink_node));
      
      RREdgeId rr_edge = device_ctx.rr_graph.create_edge(old_to_new_rr_node[inode], 
                                                         old_to_new_rr_node[isink_node], 
                                                         RRSwitchId(iswitch));

      auto key = std::make_pair(inode, iedge);
      VTR_ASSERT(!old_to_new_rr_edge.count(key));
      old_to_new_rr_edge[key] = rr_edge;
    }
  }

  /* Partition edges to be two class: configurable (1st part) and non-configurable (2nd part)
   * See how the router will use the edges and determine the strategy
   * if we want to partition the edges first or depends on the routing needs  
   */
  device_ctx.rr_graph.partition_edges();

  return;
}

