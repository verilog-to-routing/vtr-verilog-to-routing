#pragma once

#include <string>
#include <limits>
#include "vtr_ndmatrix.h"
#include "router_lookahead.h"
#include "router_lookahead_map_utils.h"

class MapLookahead : public RouterLookahead {
  public:
    explicit MapLookahead(const t_det_routing_arch& det_routing_arch, bool is_flat)
        : det_routing_arch_(det_routing_arch)
        , is_flat_(is_flat) {}

  private:
    //Look-up table from SOURCE/OPIN to CHANX/CHANY of various types
    util::t_src_opin_delays src_opin_delays;
    // Lookup table from a tile pins to the primitive classes inside that tile
    std::unordered_map<int, util::t_ipin_primitive_sink_delays> inter_tile_pin_primitive_pin_delay; // [physical_tile_type][from_pin_physical_num][sink_physical_num] -> cost
    // Lookup table to store the minimum cost to reach to a primitive pin from the root-level IPINs
    std::unordered_map<int, std::unordered_map<int, util::Cost_Entry>> tile_min_cost; // [physical_tile_type][sink_physical_num] -> cost
    // Lookup table to store the minimum cost for each dx and dy
    vtr::NdMatrix<util::Cost_Entry, 3> distance_based_min_cost; // [layer_num][dx][dy] -> cost
    const t_det_routing_arch& det_routing_arch_;
    bool is_flat_;

  protected:
    float get_expected_cost(RRNodeId node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const override;
    std::pair<float, float> get_expected_delay_and_cong(RRNodeId from_node, RRNodeId to_node, const t_conn_cost_params& params, float R_upstream) const override;

    void compute(const std::vector<t_segment_inf>& segment_inf) override;
    void compute_intra_tile() override;
    void read(const std::string& file) override;
    void read_intra_cluster(const std::string& file) override;
    void write(const std::string& file) const override;
    void write_intra_cluster(const std::string& file) const override;
};

/* f_cost_map is an array of these cost entries that specifies delay/congestion estimates
 * to travel relative x/y distances */
class Cost_Entry {
  public:
    float delay;
    float congestion;

    Cost_Entry()
        : Cost_Entry(std::numeric_limits<float>::quiet_NaN(),
                     std::numeric_limits<float>::quiet_NaN()) {
    }

    Cost_Entry(float set_delay, float set_congestion) {
        delay = set_delay;
        congestion = set_congestion;
    }
};

/* provides delay/congestion estimates to travel specified distances
 * in the x/y direction */
typedef vtr::NdMatrix<Cost_Entry, 5> t_wire_cost_map; //[0..num_layers][0..1][[0..num_seg_types-1]0..device_ctx.grid.width()-1][0..device_ctx.grid.height()-1]
                                                      //[0..1] entry distinguish between CHANX/CHANY start nodes respectively

void read_router_lookahead(const std::string& file);
void write_router_lookahead(const std::string& file);
