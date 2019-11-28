#ifndef RR_GRAPH_UTIL_H
#define RR_GRAPH_UTIL_H

/* Include header files which include data structures used by
 * the function declaration
 */
#include <vector>
#include "rr_graph_fwd.h"

/* Get node-to-node switches in a RRGraph */
std::vector<RRSwitchId> find_rr_graph_switches(const RRGraph& rr_graph,
                                               const RRNodeId& from_node,
                                               const RRNodeId& to_node);

#endif
