#ifndef RR_GRAPH_OBJ_UTIL_H
#define RR_GRAPH_OBJ_UTIL_H

/* Include header files which include data structures used by
 * the function declaration
 */
#include <vector>
#include "rr_graph_obj.h"
#include "device_grid.h"

/* Get node-to-node switches in a RRGraph */
std::vector<RRSwitchId> find_rr_graph_switches(const RRGraph& rr_graph,
                                               const RRNodeId& from_node,
                                               const RRNodeId& to_node);

std::vector<RRNodeId> find_rr_graph_nodes(const RRGraph& rr_graph,
                                          const int& x,
                                          const int& y,
                                          const t_rr_type& rr_type,
                                          const int& ptc);

std::vector<RRNodeId> find_rr_graph_chan_nodes(const RRGraph& rr_graph,
                                               const int& x,
                                               const int& y,
                                               const t_rr_type& rr_type);

std::vector<RRNodeId> find_rr_graph_grid_nodes(const RRGraph& rr_graph,
                                               const DeviceGrid& device_grid,
                                               const int& x,
                                               const int& y,
                                               const t_rr_type& rr_type,
                                               const e_side& side);


#endif
