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
RRGraph convert_rr_graph() {
  auto& device_ctx = g_vpr_ctx.device();

  RRGraph rr_graph;

  /* The number of switches are in general small, 
   * reserve switches may not bring significant memory efficiency 
   * So, we just use create_switch to push_back each time 
   */
  //Create the switches
  std::map<int,RRSwitchId> old_to_new_rr_switch;
  for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); ++inode) {
    const auto& node = device_ctx.rr_nodes[inode];
    for (int iedge = 0; iedge < node.num_edges(); ++iedge) {
      short iswitch = node.edge_switch(iedge);

      if (!old_to_new_rr_switch.count(iswitch)) { //Only create the switch once
        RRSwitchId rr_switch = rr_graph.create_switch(device_ctx.rr_switch_inf[iswitch]);

        VTR_ASSERT(!old_to_new_rr_switch.count(iswitch));
        old_to_new_rr_switch[iswitch] = rr_switch;
      }
    }
  }

  /* The number of segments are in general small, reserve segments may not bring significant memory efficiency */
  //Create the segments
  std::map<int,RRSegmentId> old_to_new_rr_segment;
  for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); ++inode) {
    const auto& node = device_ctx.rr_nodes[inode];
    /* segment index stored in rr_indexed_data
     * we get the rr_index_data index from rr_node 
     * and then get the segment id 
     */
    short irc_data = node.cost_index();
    short iseg = device_ctx.rr_indexed_data[irc_data].seg_index;
    if (!old_to_new_rr_segment.count(iseg)) { //Only create the segment once
      RRSegmentId rr_segment = rr_graph.create_segment(device_ctx.arch->Segments[iseg]);

      VTR_ASSERT(!old_to_new_rr_segment.count(iseg));
      old_to_new_rr_segment[iseg] = rr_segment;
    }
  }

  /* Reserve list of nodes to be memory efficient */
  rr_graph.reserve_nodes(device_ctx.rr_nodes.size());

  //Create the nodes
  std::map<int,RRNodeId> old_to_new_rr_node;
  for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); ++inode) {
    auto& node = device_ctx.rr_nodes[inode];
    RRNodeId rr_node = rr_graph.create_node(node.type());

    rr_graph.set_node_xlow(rr_node, node.xlow());
    rr_graph.set_node_ylow(rr_node, node.ylow());
    rr_graph.set_node_xhigh(rr_node, node.xhigh());
    rr_graph.set_node_yhigh(rr_node, node.yhigh());

    rr_graph.set_node_capacity(rr_node, node.capacity());

    rr_graph.set_node_ptc_num(rr_node, node.ptc_num());

    rr_graph.set_node_cost_index(rr_node, node.cost_index());

    if (node.type() == CHANX || node.type() == CHANY) {
        rr_graph.set_node_direction(rr_node, node.direction());
    }
    if (node.type() == IPIN || node.type() == OPIN) {
        rr_graph.set_node_side(rr_node, node.side());
    }
    rr_graph.set_node_R(rr_node, node.R());
    rr_graph.set_node_C(rr_node, node.C());

    /* Set up segment id */
    short irc_data = node.cost_index();
    short iseg = device_ctx.rr_indexed_data[irc_data].seg_index;
    rr_graph.set_node_segment_id(rr_node, iseg);
    
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
    rr_graph.reserve_nodes(num_edges_to_reserve);
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
      VTR_ASSERT(old_to_new_rr_switch.count(iswitch));
      
      RREdgeId rr_edge = rr_graph.create_edge(old_to_new_rr_node[inode], 
                                              old_to_new_rr_node[isink_node], 
                                              old_to_new_rr_switch[iswitch]);

      auto key = std::make_pair(inode, iedge);
      VTR_ASSERT(!old_to_new_rr_edge.count(key));
      old_to_new_rr_edge[key] = rr_edge;
    }
  }

  /* TODO: Partition edges to be completed
   * See how the router will use the edges and determine the strategy
   * if we want to partition the edges first or depends on the routing needs  
   */
  rr_graph.partition_edges();

  return rr_graph;
}

