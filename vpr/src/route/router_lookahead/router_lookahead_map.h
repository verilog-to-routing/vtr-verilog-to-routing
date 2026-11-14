#pragma once

#include <cstdlib>
#include <optional>
#include <string>
#include <tuple>
#include <vector>
#include "vtr_assert.h"
#include "vtr_ndmatrix.h"
#include "router_lookahead.h"
#include "router_lookahead_map_utils.h"
#include "router_lookahead_interposer.h"

/**
 * @brief Current VPR RouterLookahead implementation. This lookahead uses a table
 * indexed by (delta_x, delta_y) and other things like wire type.
 *
 * The lookahead table/map is constructed using data reduction of sample routes.
 * It first picks sample points near the bottom left corner of the device and then
 * finds the shortest paths to the entire RR Graph starting from these sample points.
 * For all sample routes with a specific (delta_x, delta_y), the minimum path cost
 * is inserted in the table.
 */
class MapLookahead final : public RouterLookahead {
  public:
    explicit MapLookahead(const t_det_routing_arch& det_routing_arch, bool is_flat, int route_verbosity, bool device_model_warnings, float interposer_base_cost_multiplier);

  private:
    float get_expected_cost_flat_router(RRNodeId current_node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const;
    //Same as get_expected_cost_flat_router(), but returns the delay and congestion costs separately
    //instead of their sum (RCV requires this). It is called by get_expected_delay_and_cong() when is_flat_ is true.
    std::pair<float, float> get_expected_delay_and_cong_flat_router(RRNodeId current_node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const;
    //The original non-flat routing delay/congestion lookup.
    //Called directly by get_expected_delay_and_cong_flat_router() and by get_expected_delay_and_cong() when !is_flat_.
    std::pair<float, float> get_expected_delay_and_cong_global(RRNodeId from_node, RRNodeId to_node, const t_conn_cost_params& params, float R_upstream) const;
    /**
     * @brief Add the intra-cluster cost from the target tile's root-level IPINs to the target SINK to a
     *        (delay, congestion) cost that only reaches the root-level pins.
     */
    std::pair<float, float> add_target_sink_cost(std::pair<float, float> root_pin_cost, RRNodeId target_node, const t_conn_cost_params& params) const;
    /**
     * @brief Cost from a node inside a cluster to a SINK in another tile: the cheapest wire cost over the
     *        (dx, dy) distance plus the intra-cluster cost into the target. The cost of leaving the current
     *        cluster is unknown and not included.
     */
    std::pair<float, float> cost_to_sink_in_other_tile(RRNodeId current_node, RRNodeId target_node, const t_conn_cost_params& params) const;
    //Look-up table from SOURCE/OPIN to CHANX/CHANY of various types
    util::t_src_opin_delays src_opin_delays;
    /// Intra-cluster cost from each pin of the tile (root-level and intra-cluster) to each primitive sink
    /// reachable from it: [physical tile type index][pin ptc] -> {sink ptc -> cost}. Built only for flat
    /// routing.
    std::vector<util::t_ipin_primitive_sink_delays> intra_tile_pin_primitive_pin_delay;
    /// Minimum cost from a tile's root-level IPINs to each primitive sink:
    /// [physical tile type index][sink ptc] -> cost.
    std::vector<std::vector<util::Cost_Entry>> tile_min_cost;
    // Lookup table to store the minimum cost for each dx and dy
    vtr::NdMatrix<util::Cost_Entry, 4> chann_distance_based_min_cost; // [from_layer_num][to_layer_num][dx][dy] -> cost
    vtr::NdMatrix<util::Cost_Entry, 5> opin_distance_based_min_cost;  // [physical_tile_idx][from_layer_num][to_layer_num][dx][dy] -> cost
    std::optional<InterposerLookahead> interposer_lookahead_;

    const t_det_routing_arch& det_routing_arch_;
    bool is_flat_;
    int route_verbosity_;
    bool device_model_warnings_;
    bool has_interposer_cuts_;
    /// True when the device has a single layer, so every RR node is on layer 0.
    bool is_single_layer_;
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

  public:
    // Public so SimpleDelayModel can call it through the
    // concrete type statically, without using dynamic dispatch.
    inline float get_opin_distance_min_delay(int physical_tile_idx, const t_physical_tile_loc& from_loc, const t_physical_tile_loc& to_loc) const override {
        float min_delay = opin_distance_based_min_cost[physical_tile_idx]
                                                      [from_loc.layer_num]
                                                      [to_loc.layer_num]
                                                      [std::abs(from_loc.x - to_loc.x)]
                                                      [std::abs(from_loc.y - to_loc.y)]
                                                          .delay;

        // Add the delay of crossing interposer cuts, used in 2.5D architectures.
        if (has_interposer_cuts_) {
            VTR_ASSERT_SAFE(interposer_lookahead_);
            float interposer_delay;
            std::tie(interposer_delay, std::ignore) = interposer_lookahead_->get_interposer_lookahead_cost(from_loc, to_loc);
            min_delay += interposer_delay;
        }

        return min_delay;
    }
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
