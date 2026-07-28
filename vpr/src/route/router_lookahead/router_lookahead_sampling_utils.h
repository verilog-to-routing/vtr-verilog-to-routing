#pragma once

#include "router_lookahead_map_utils.h"
#include "rr_graph_fwd.h"

/* runs Dijkstra's algorithm from specified node until all nodes have been visited. Each time a pin is visited, the delay/congestion information
 * to that pin is stored is added to an entry in the routing_cost_map */
void run_dijkstra(RRNodeId start_node,
                  util::t_dijkstra_data& data,
                  const t_bb& bb,
                  std::function<void(util::PQ_Entry)> found_sink_callback);
