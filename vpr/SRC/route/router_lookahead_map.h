#pragma once


/* Computes the lookahead map to be used by the router. If a map was computed prior to this, it will be cleared
   and a new one will be allocated. The rr graph must have been built before calling this function. */
void compute_router_lookahead(int num_segments, int wire_to_ipin_rr_switch_ind);

/* queries the lookahead_map (should have been computed prior to routing) to get the expected cost
   from the specified source to the specified target */
float get_lookahead_map_cost(int from_node_ind, int to_node_ind, float criticality_fac, float &delay, float &cong);


