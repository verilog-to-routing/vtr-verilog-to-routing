/* IMPORTANT:
 * The following preprocessing flags are added to 
 * avoid compilation error when this headers are included in more than 1 times 
 */
#ifndef TIMING_DRIVEN_ROUTER_LOOKAHEAD_MAP_H
#define TIMING_DRIVEN_ROUTER_LOOKAHEAD_MAP_H

/*
 * Notes in include header files in a head file 
 * Only include the neccessary header files 
 * that is required by the data types in the function/class declarations!
 */
/* Header files should be included in a sequence */
/* Standard header files required go first */
#include "rr_graph_fwd.h"

/* Computes the lookahead map to be used by the router. If a map was computed prior to this, a new one will not be computed again.
   The rr graph must have been built before calling this function. */
void compute_router_lookahead(size_t num_segments);

/* queries the lookahead_map (should have been computed prior to routing) to get the expected cost
   from the specified source to the specified target */
float get_lookahead_map_cost(RRNodeId from_node_ind, RRNodeId to_node_ind, float criticality_fac);

#endif


