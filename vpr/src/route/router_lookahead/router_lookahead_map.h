#pragma once

#include <string>
#include "vtr_ndmatrix.h"
#include "router_lookahead.h"
#include "router_lookahead_map_utils.h"

/**
 * @brief Current VPR RouterLookahead implementation.
 */
class MapLookahead : public RouterLookahead {
  public:
    explicit MapLookahead(const t_det_routing_arch& det_routing_arch, bool is_flat, int route_verbosity);

  private:
    float get_expected_cost_flat_router(RRNodeId current_node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const;
    //Look-up table from SOURCE/OPIN to CHANX/CHANY of various types
    util::t_src_opin_delays src_opin_delays;
    // Lookup table from a tile pins to the primitive classes inside that tile
    std::unordered_map<int, util::t_ipin_primitive_sink_delays> intra_tile_pin_primitive_pin_delay; // [physical_tile_type][from_pin_physical_num][sink_physical_num] -> cost
    // Lookup table to store the minimum cost to reach to a primitive pin from the root-level IPINs
    std::unordered_map<int, std::unordered_map<int, util::Cost_Entry>> tile_min_cost; // [physical_tile_type][sink_physical_num] -> cost
    // Lookup table to store the minimum cost for each dx and dy
    vtr::NdMatrix<util::Cost_Entry, 4> chann_distance_based_min_cost; // [from_layer_num][to_layer_num][dx][dy] -> cost
    vtr::NdMatrix<util::Cost_Entry, 5> opin_distance_based_min_cost;  // [physical_tile_idx][from_layer_num][to_layer_num][dx][dy] -> cost

    const t_det_routing_arch& det_routing_arch_;
    bool is_flat_;
    int route_verbosity_;

  protected:
    float get_expected_cost(RRNodeId current_node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const override;
    std::pair<float, float> get_expected_delay_and_cong(RRNodeId from_node, RRNodeId to_node, const t_conn_cost_params& params, float R_upstream) const override;

    void compute(const std::vector<t_segment_inf>& segment_inf) override;
    void compute_intra_tile() override;
    void read(const std::string& file) override;
    void read_intra_cluster(const std::string& file) override;
    void write(const std::string& file_name) const override;
    void write_intra_cluster(const std::string& file) const override;
    float get_opin_distance_min_delay(int physical_tile_idx, int from_layer, int to_layer, int dx, int dy) const override;
};

/**
 * @brief Provides delay/congestion estimates to travel specified distances
 * in the x/y direction
 *
 * This is a 6D array storing the cost to travel from a node of type CHANX/CHANY/CHANZ
 * to a point that is dx, dy further, and is on the "layer_num" layer.
 *
 * To store this information:
 * - The first index is the layer number that the node under consideration is on.
 * - The second index is the layer number of the target node.
 * - The third index represents the type of channel (X/Y/Z) that the node under consideration belongs to.
 * - The forth is the segment type (specified in the architecture file under the "segmentlist" tag).
 * - The fifth is dx.
 * - The last one is dy.
 *
 * @note [0..num_layers][0..num_layers][0..2][0..num_seg_types-1][0..device_ctx.grid.width()-1][0..device_ctx.grid.height()-1]
 * - [0..2] entry distinguish between CHANX/CHANY/CHANZ start nodes respectively
 * - The first index is the layer number that the node under consideration is on.
 * - The second index is the layer number that the target node is on.
 */
typedef vtr::NdMatrix<util::Cost_Entry, 6> t_wire_cost_map;

void read_router_lookahead(const std::string& file);
void write_router_lookahead(const std::string& file);
