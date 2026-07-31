#pragma once

#include <optional>
#include <tuple>

#include "device_grid.h"
#include "globals.h"
#include "place_delay_model.h"
#include "router_lookahead.h"
#include "router_lookahead_interposer.h"
#include "vpr_context.h"
#include "vpr_error.h"
#include "vtr_assert.h"

/**
 * @class SimpleDelayModel
 * @brief A simple delay model based on the information stored in router lookahead
 * This is in contrast to other placement delay models that get the cost of getting from one location to another by running the router
 *
 * The model is templated on the concrete lookahead type so that the per-query lookahead
 * call binds statically and can be inlined into delay(). This lookup is in the placer's
 * inner hot loop and therefore needs this optimization.
 */
template<typename LookaheadT>
class SimpleDelayModel final : public PlaceDelayModel {
  public:
    explicit SimpleDelayModel(const LookaheadT& router_lookahead)
        : router_lookahead_(router_lookahead) {}

    /// @brief Set up any auxiliary data needed to query the router lookahead (e.g. interposer delay information)
    void compute(RouterDelayProfiler& /*route_profiler*/,
                 const t_placer_opts& /*placer_opts*/,
                 const t_router_opts& /*router_opts*/,
                 int /*longest_length*/) override {
        const DeviceContext& device = g_vpr_ctx.device();
        const DeviceGrid& grid = device.grid;

        if (grid.has_interposer_cuts()) {
            // We don't use the base cost in the simple delay model, so we set the multiplier to 1.
            interposer_lookahead_.emplace(device.rr_graph, grid, device, /*interposer_cut_base_cost_multiplier*/ 1);
        }
    }

    float delay(const t_physical_tile_loc& from_loc, int /*from_pin*/, const t_physical_tile_loc& to_loc, int /*to_pin*/) const override {
        const DeviceGrid& grid = g_vpr_ctx.device().grid;
        int delta_x = std::abs(from_loc.x - to_loc.x);
        int delta_y = std::abs(from_loc.y - to_loc.y);

        int from_tile_idx = grid.get_physical_type(from_loc)->index;

        float interposer_delay = 0.f;
        if (interposer_lookahead_) {
            VTR_ASSERT_SAFE(grid.has_interposer_cuts());
            std::tie(interposer_delay, std::ignore) = interposer_lookahead_->get_interposer_lookahead_cost(from_loc, to_loc);
        }

        // The cost is used for placement, so the source is on OPINs; thus, we need the minimum delay
        // starting from OPINs, not from channels.
        float min_delay = router_lookahead_.get_opin_distance_min_delay(from_tile_idx,
                                                                        from_loc.layer_num, to_loc.layer_num,
                                                                        delta_x, delta_y);
        return min_delay + interposer_delay;
    }

    void dump_echo(std::string /*filepath*/) const override {}

    void read(const std::string& /*file*/) override {
        VPR_THROW(VPR_ERROR_PLACE,
                  "SimpleDelayModel does not support reading a placement delay lookup: "
                  "it queries the router lookahead directly instead of storing a delay matrix.");
    }

    void write(const std::string& /*file*/) const override {
        VPR_THROW(VPR_ERROR_PLACE,
                  "SimpleDelayModel does not support writing a placement delay lookup: "
                  "it queries the router lookahead directly instead of storing a delay matrix.");
    }

  private:
    /// @brief The router lookahead queried for the minimum delay between locations.
    const LookaheadT& router_lookahead_;

    /// @brief Contains delay information of crossing a die, used in 2.5D architectures.
    std::optional<InterposerLookahead> interposer_lookahead_;
};
