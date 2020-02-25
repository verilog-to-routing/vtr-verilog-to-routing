#pragma once

#include <string>
#include "vtr_ndmatrix.h"
#include "router_lookahead.h"

class MapLookahead : public RouterLookahead {
  protected:
    float get_expected_cost(int node, int target_node, const t_conn_cost_params& params, float R_upstream) const override;
    void compute(const std::vector<t_segment_inf>& segment_inf) override;
    void read(const std::string& file) override;
    void write(const std::string& file) const override;
};

/* f_cost_map is an array of these cost entries that specifies delay/congestion estimates
 * to travel relative x/y distances */
class Cost_Entry {
  public:
    float delay;
    float congestion;

    Cost_Entry() {
        delay = -1.0;
        congestion = -1.0;
    }
    Cost_Entry(float set_delay, float set_congestion) {
        delay = set_delay;
        congestion = set_congestion;
    }
};

/* provides delay/congestion estimates to travel specified distances
 * in the x/y direction */
typedef vtr::NdMatrix<Cost_Entry, 4> t_wire_cost_map; //[0..1][[0..num_seg_types-1]0..device_ctx.grid.width()-1][0..device_ctx.grid.height()-1]
                                                      //[0..1] entry distinguish between CHANX/CHANY start nodes respectively

void read_router_lookahead(const std::string& file);
void write_router_lookahead(const std::string& file);

/* Computes the lookahead map to be used by the router. If a map was computed prior to this, a new one will not be computed again.
 * The rr graph must have been built before calling this function. */
void compute_router_lookahead(int num_segments);

/* queries the lookahead_map (should have been computed prior to routing) to get the expected cost
 * from the specified source to the specified target */
float get_lookahead_map_cost(int from_node_ind, int to_node_ind, float criticality_fac);
