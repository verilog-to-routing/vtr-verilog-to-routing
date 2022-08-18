#ifndef RR_GRAPH_UTILS_H
#define RR_GRAPH_UTILS_H

/* Include header files which include data structures used by
 * the function declaration
 */
#include <vector>
#include "rr_graph_fwd.h"
#include "rr_node_types.h"
#include "rr_graph_view.h"

/* Get node-to-node switches in a RRGraph */
std::vector<RRSwitchId> find_rr_graph_switches(const RRGraph& rr_graph,
                                               const RRNodeId& from_node,
                                               const RRNodeId& to_node);

// This function generates and returns a vector indexed by RRNodeId
// containing a list of fan-in edges for each node.
vtr::vector<RRNodeId, std::vector<RREdgeId>> get_fan_in_list(const RRGraphView& rr_graph);

int seg_index_of_cblock(const RRGraphView& rr_graph, t_rr_type from_rr_type, int to_node);
int seg_index_of_sblock(const RRGraphView& rr_graph, int from_node, int to_node);

#endif