#pragma once

#include <optional>
#include <string>
#include "vtr_ndmatrix.h"
#include "router_lookahead.h"
#include "router_lookahead_map_utils.h"
#include "router_lookahead_interposer.h"

/**
 * @brief Current VPR RouterLookahead implementation.
 */
class MapLookahead : public RouterLookahead {
  public:
    explicit MapLookahead(const t_det_routing_arch& det_routing_arch, bool is_flat, int route_verbosity, bool device_model_warnings, float interposer_base_cost_multiplier);

    /**
     * @brief Get the delay and congestion cost estimated by the lookahead map for a node that
     *        is (delta_x, delta_y) away from from_node, without reference to an actual target node.
     * @attention This does not account for effects that depend on an actual target node (e.g. the
     *            interposer lookahead, or the target's layer when it differs from from_node's layer).
     *            It is intended for profiling/reporting purposes (e.g. checking how separable the
     *            lookahead's delay estimate is in x and y), not for use during routing.
     * @param from_node The source node from which the cost is estimated.
     * @param delta_x Horizontal distance (in tiles) to the (hypothetical) target node.
     * @param delta_y Vertical distance (in tiles) to the (hypothetical) target node.
     * @param params Contains the router parameters such as connection criticality, etc.
     * @return A pair of (delay cost, congestion cost).
     */
    std::pair<float, float> get_expected_delay_and_cong_from_deltas(RRNodeId from_node, int delta_x, int delta_y, const t_conn_cost_params& params) const;

    /**
     * @brief Returns the lookahead map's cost to travel (delta_x, delta_y) from a wire of the given
     *        channel type and segment type on "from_layer_num", to a point on "to_layer_num".
     * @attention This exposes a raw lookahead map entry, without any of the adjustments made when
     *            costing an actual connection. It is intended for reporting, and for other lookaheads
     *            that want to fall back on this one's estimates.
     */
    util::Cost_Entry get_wire_cost(e_rr_type rr_type, int seg_index, int from_layer_num, int delta_x, int delta_y, int to_layer_num) const;

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
    std::optional<InterposerLookahead> interposer_lookahead_;

    const t_det_routing_arch& det_routing_arch_;
    bool is_flat_;
    int route_verbosity_;
    bool device_model_warnings_;
    bool has_interposer_cuts_;
    const float interposer_base_cost_multiplier_;

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
