#pragma once

#include "router_lookahead_map_utils.h"
#include "rr_graph_fwd.h"

struct t_bb;

/**
 * @brief runs Dijkstra's algorithm from specified node until all nodes
 * have been visited. Each time a SOURCE/IPIN is visited, the delay/congestion
 * information from start_node to that pin is passed on to found_sink_callback,
 * which is passed on by lookaheads. Typically a lookahead would then save this
 * information in an internal table/map.
 */
void run_dijkstra(RRNodeId start_node,
                  util::t_dijkstra_data& data,
                  const t_bb& bb,
                  std::function<void(util::PQ_Entry)> found_sink_callback);
