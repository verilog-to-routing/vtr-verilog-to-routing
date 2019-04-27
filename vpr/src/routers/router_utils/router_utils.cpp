
/* Standard header files required go first */

/* EXTERNAL library header files go second*/

/* VPR header files go then */
#include "router_utils.h"
#include "rr_graph_fwd.h"

/* Finally we include global variables */
#include "globals.h"

namespace router { /* We put utility functions in an independent namespace than routers*/

vtr::Matrix<float> calculate_routing_usage(t_rr_type rr_type) {
  VTR_ASSERT(rr_type == CHANX || rr_type == CHANY);

  auto& device_ctx = g_vpr_ctx.device();
  auto& cluster_ctx = g_vpr_ctx.clustering();
  auto& route_ctx = g_vpr_ctx.routing();

  vtr::Matrix<float> usage({{device_ctx.grid.width(), device_ctx.grid.height()}}, 0.);

  //Collect all the in-use RR nodes
  std::set<int> rr_nodes;
  for (auto net : cluster_ctx.clb_nlist.nets()) {
    t_trace* tptr = route_ctx.trace[net].head;
    while (tptr != nullptr) {
      int inode = tptr->index; 
      RRNodeId inode_id = RRNodeId(inode); 

      if (device_ctx.rr_graph.node_type(inode_id) == rr_type) {
        rr_nodes.insert(inode);
      }
      tptr = tptr->next;
    }
  }

  //Record number of used resources in each x/y channel
  for (int inode : rr_nodes) {
    RRNodeId inode_id = RRNodeId(inode); 

    if (rr_type == CHANX) {
      VTR_ASSERT(CHANX == device_ctx.rr_graph.node_type(inode_id));
      VTR_ASSERT(device_ctx.rr_graph.node_ylow(inode_id) == device_ctx.rr_graph.node_yhigh(inode_id));

      int y = device_ctx.rr_graph.node_ylow(inode_id);
      for (int x = device_ctx.rr_graph.node_xlow(inode_id); 
               x <= device_ctx.rr_graph.node_xhigh(inode_id); 
               ++x) {
        usage[x][y] += route_ctx.rr_node_route_inf[inode].occ();
      }
    } else {
      VTR_ASSERT(rr_type == CHANY);
      VTR_ASSERT(CHANY == device_ctx.rr_graph.node_type(inode_id));
      VTR_ASSERT(device_ctx.rr_graph.node_xlow(inode_id) == device_ctx.rr_graph.node_xhigh(inode_id));

      int x = device_ctx.rr_graph.node_xlow(inode_id);
      for (int y = device_ctx.rr_graph.node_ylow(inode_id); 
               y <= device_ctx.rr_graph.node_yhigh(inode_id); 
               ++y) {
        usage[x][y] += route_ctx.rr_node_route_inf[inode].occ();
      }
    }
  }
  return usage;
}

vtr::Matrix<float> calculate_routing_avail(t_rr_type rr_type) {
  //Calculate the number of available resources in each x/y channel
  VTR_ASSERT(rr_type == CHANX || rr_type == CHANY);

  auto& device_ctx = g_vpr_ctx.device();

  vtr::Matrix<float> avail({{device_ctx.grid.width(), device_ctx.grid.height()}}, 0.);
  for (auto inode : device_ctx.rr_graph.nodes()) {

    if (CHANX == device_ctx.rr_graph.node_type(inode) && CHANX == rr_type) {
      VTR_ASSERT(device_ctx.rr_graph.node_ylow(inode) == device_ctx.rr_graph.node_yhigh(inode));

      int y = device_ctx.rr_graph.node_ylow(inode);
      for (int x = device_ctx.rr_graph.node_xlow(inode); 
               x <= device_ctx.rr_graph.node_xhigh(inode); 
               ++x) {
        avail[x][y] += device_ctx.rr_graph.node_capacity(inode);
      }
    } else if (CHANY == device_ctx.rr_graph.node_type(inode) && CHANY == rr_type) {
      VTR_ASSERT(device_ctx.rr_graph.node_xlow(inode) == device_ctx.rr_graph.node_xhigh(inode));

      int x = device_ctx.rr_graph.node_xlow(inode);
      for (int y = device_ctx.rr_graph.node_ylow(inode); 
               y <= device_ctx.rr_graph.node_yhigh(inode); 
               ++y) {
        avail[x][y] += device_ctx.rr_graph.node_capacity(inode);
      }
    }
  }
  return avail;
}

float routing_util(float used, float avail) {
  if (used > 0.) {
    VTR_ASSERT(avail > 0.);
    return used / avail;
  }
  return 0.;
}

} /* end of router namespace */
