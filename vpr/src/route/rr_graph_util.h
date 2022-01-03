#ifndef RR_GRAPH_UTIL_H
#define RR_GRAPH_UTIL_H

#include "vpr_types.h"

int seg_index_of_cblock(t_rr_type from_rr_type, int to_node);

int seg_index_of_sblock(int from_node, int to_node);

// This function generates and returns a vector indexed by RRNodeId
// containing a list of fan-in edges for each node.
vtr::vector<RRNodeId, std::vector<RREdgeId>> get_fan_in_list();

#endif
