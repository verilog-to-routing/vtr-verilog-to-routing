#pragma once

#include <string>

void read_router_lookahead(const std::string& file);
void write_router_lookahead(const std::string& file);

/* Computes the lookahead map to be used by the router. If a map was computed prior to this, a new one will not be computed again.
 * The rr graph must have been built before calling this function. */
void compute_router_lookahead(int num_segments);

/* queries the lookahead_map (should have been computed prior to routing) to get the expected cost
 * from the specified source to the specified target */
float get_lookahead_map_cost(int from_node_ind, int to_node_ind, float criticality_fac);
