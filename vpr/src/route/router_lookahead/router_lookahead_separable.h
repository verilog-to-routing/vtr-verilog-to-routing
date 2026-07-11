#pragma once

#include <optional>
#include <string>
#include "vtr_ndmatrix.h"
#include "router_lookahead.h"
#include "router_lookahead_map_utils.h"
#include "router_lookahead_interposer.h"

/**
 * @brief Provides delay/congestion estimates to travel to a point with a given
 * x-coordinate, considering only the x component of the route.
 *
 * This is a 7D array storing the cost to travel from a node of type CHANX/CHANY/CHANZ
 * whose x-coordinate is x1, to a point whose x-coordinate is x2, on the "layer_num" layer.
 *
 * To store this information:
 * - The first index is the layer number that the node under consideration is on.
 * - The second index is the layer number of the target node.
 * - The third index represents the type of channel (X/Y/Z) that the node under consideration belongs to.
 * - The forth is the segment type (specified in the architecture file under the "segmentlist" tag).
 * - The fifth is the wire direction (INC/DEC/BIDIR) of the node under consideration.
 * - The sixth is x1.
 * - The last one is x2.
 *
 * @note [0..num_layers][0..num_layers][0..2][0..num_seg_types-1][0..2][0..device_ctx.grid.width()-1][0..device_ctx.grid.width()-1]
 * - [0..2] (3rd index) entry distinguish between CHANX/CHANY/CHANZ start nodes respectively
 * - [0..2] (5th index) entry distinguish between INC/DEC/BIDIR wire directions respectively (see Direction in rr_node_types.h)
 * - The first index is the layer number that the node under consideration is on.
 * - The second index is the layer number that the target node is on.
 */
typedef vtr::NdMatrix<util::Cost_Entry, 7> t_x_wire_cost_map;

/**
 * @brief Provides delay/congestion estimates to travel to a point with a given
 * y-coordinate, considering only the y component of the route.
 *
 * This is a 7D array storing the cost to travel from a node of type CHANX/CHANY/CHANZ
 * whose y-coordinate is y1, to a point whose y-coordinate is y2, on the "layer_num" layer.
 *
 * To store this information:
 * - The first index is the layer number that the node under consideration is on.
 * - The second index is the layer number of the target node.
 * - The third index represents the type of channel (X/Y/Z) that the node under consideration belongs to.
 * - The forth is the segment type (specified in the architecture file under the "segmentlist" tag).
 * - The fifth is the wire direction (INC/DEC/BIDIR) of the node under consideration.
 * - The sixth is y1.
 * - The last one is y2.
 *
 * @note [0..num_layers][0..num_layers][0..2][0..num_seg_types-1][0..2][0..device_ctx.grid.height()-1][0..device_ctx.grid.height()-1]
 * - [0..2] (3rd index) entry distinguish between CHANX/CHANY/CHANZ start nodes respectively
 * - [0..2] (5th index) entry distinguish between INC/DEC/BIDIR wire directions respectively (see Direction in rr_node_types.h)
 * - The first index is the layer number that the node under consideration is on.
 * - The second index is the layer number that the target node is on.
 */
typedef vtr::NdMatrix<util::Cost_Entry, 7> t_y_wire_cost_map;

/**
 * @brief RouterLookahead implementation which estimates delay/congestion by
 *        treating the x and y components of a route independently (i.e. the
 *        cost is separable in x and y), rather than as a joint function of
 *        (delta_x, delta_y) as done by MapLookahead.
 */
class SeparableLookahead : public RouterLookahead {
  public:
    explicit SeparableLookahead(const t_det_routing_arch& det_routing_arch, bool is_flat, int route_verbosity, bool device_model_warnings, float interposer_base_cost_multiplier);

  private:
    float get_expected_cost_flat_router(RRNodeId current_node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const;

    //Look-up table from SOURCE/OPIN to CHANX/CHANY of various types
    util::t_src_opin_delays src_opin_delays;
    // Lookup table from a tile pins to the primitive classes inside that tile
    std::unordered_map<int, util::t_ipin_primitive_sink_delays> intra_tile_pin_primitive_pin_delay; // [physical_tile_type][from_pin_physical_num][sink_physical_num] -> cost
    // Lookup table to store the minimum cost to reach to a primitive pin from the root-level IPINs
    std::unordered_map<int, std::unordered_map<int, util::Cost_Entry>> tile_min_cost; // [physical_tile_type][sink_physical_num] -> cost

    // A MapLookahead used to answer SOURCE/OPIN distance queries (get_opin_distance_min_delay, and the
    // SOURCE/OPIN case of get_expected_delay_and_cong). The separable lookahead only overrides the
    // wire-to-target (CHAN) cost estimation; OPIN/SOURCE handling is delegated to this object.
    std::unique_ptr<RouterLookahead> map_lookahead_;
    std::optional<InterposerLookahead> interposer_lookahead_;

    t_x_wire_cost_map x_wire_cost_map_;
    t_y_wire_cost_map y_wire_cost_map_;

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

void read_router_lookahead_separable(const std::string& file);
void write_router_lookahead_separable(const std::string& file);
