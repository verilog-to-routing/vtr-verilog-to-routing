#pragma once

// Experimental separable lookahead.
// TODO: Remove the dependency on the map lookahead for filling in unsampled elements in the table
// TODO: Clean up the OPIN/SOURCE lookahead code
// TODO: Use the separable lookahead in the get_opin_distance_min_delay function
// TODO: Fix the sampling strip logic to not hardcode the position
// TODO: Add flat router support
// TODO: Add writing/reading lookahead support
// TODO: Make it more scalable in Z dimension

#include <memory>
#include <string>
#include "vtr_ndmatrix.h"
#include "router_lookahead.h"
#include "router_lookahead_map_utils.h"

/**
 * @brief Wire delay and congestion indexed by source layer, target layer, channel type
 * (CHANX/CHANY/CHANZ), segment type, direction (INC/DEC/BIDIR), source x and target x.
 * [0..num_layers][0..num_layers][0..2][0..num_seg_types-1][0..2][0..device_ctx.grid.width()-1][0..device_ctx.grid.width()-1]
 */
using t_x_wire_cost_map = vtr::NdMatrix<util::Cost_Entry, 7>;

/** @brief The y-coordinate counterpart of t_x_wire_cost_map.
 * [0..num_layers][0..num_layers][0..2][0..num_seg_types-1][0..2][0..device_ctx.grid.height()-1][0..device_ctx.grid.height()-1]
 */
using t_y_wire_cost_map = vtr::NdMatrix<util::Cost_Entry, 7>;

/**
 * @brief RouterLookahead implementation which estimates delay/congestion by
 *        treating the x and y components of a route independently (i.e. the
 *        cost is separable in x and y), rather than as a joint function of
 *        (delta_x, delta_y) as done by MapLookahead.
 */
class SeparableLookahead : public RouterLookahead {
  public:
    /// @brief Configure profiling and the map lookahead used for fallback estimates.
    explicit SeparableLookahead(const t_det_routing_arch& det_routing_arch, bool is_flat, int route_verbosity, bool device_model_warnings, float interposer_base_cost_multiplier);

  private:
    // MapLookahead overrides are protected, so access them through the base interface.
    std::unique_ptr<RouterLookahead> map_lookahead_; ///< SOURCE/OPIN estimates and fallback for unsampled wire costs.
    t_x_wire_cost_map x_wire_cost_map_;              ///< Profiled wire costs along x.
    t_y_wire_cost_map y_wire_cost_map_;              ///< Profiled wire costs along y.

    bool is_flat_; ///< Whether flat routing is enabled.

  protected:
    /// @brief Return the combined delay and congestion estimate; flat routing is unsupported.
    float get_expected_cost(RRNodeId current_node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const override;
    /// @brief Return criticality-weighted delay and congestion estimates.
    std::pair<float, float> get_expected_delay_and_cong(RRNodeId from_node, RRNodeId to_node, const t_conn_cost_params& params, float R_upstream) const override;

    /// @brief Profile wire costs and compute the map lookahead used for SOURCE/OPIN estimates and fallback.
    void compute(const std::vector<t_segment_inf>& segment_inf) override;
    /// @brief Report that intra-tile profiling is unsupported.
    void compute_intra_tile() override;
    /// @brief Report that lookahead deserialization is unsupported.
    void read(const std::string& file) override;
    /// @brief Report that intra-cluster deserialization is unsupported.
    void read_intra_cluster(const std::string& file) override;
    /// @brief Report that lookahead serialization is unsupported.
    void write(const std::string& file_name) const override;
    /// @brief Report that intra-cluster serialization is unsupported.
    void write_intra_cluster(const std::string& file) const override;

  public:
    /// @brief Delegate relative-coordinate OPIN delay queries to the map lookahead.
    float get_opin_distance_min_delay(int physical_tile_idx, int from_layer, int to_layer, int dx, int dy) const override;
};
