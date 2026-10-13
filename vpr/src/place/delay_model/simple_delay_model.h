#pragma once

#include "device_grid.h"
#include "globals.h"
#include "place_delay_model.h"
#include "router_lookahead.h"
#include "vpr_context.h"
#include "vpr_error.h"

/**
 * @class SimpleDelayModel
 * @brief A simple delay model based on the information stored in router lookahead
 * This is in contrast to other placement delay models that get the cost of getting from one location to another by running the router
 * Since this class depends on the lookahead, its lifetime should not exceed the cached lookahead
 * and must be rebuilt in case of an RR Graph rebuild.
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

    /// @brief Nothing to compute: all delay information is queried from the router lookahead.
    void compute(RouterDelayProfiler& /*route_profiler*/,
                 const t_placer_opts& /*placer_opts*/,
                 const t_router_opts& /*router_opts*/,
                 int /*longest_length*/) override {}

    float delay(const t_physical_tile_loc& from_loc, int /*from_pin*/, const t_physical_tile_loc& to_loc, int /*to_pin*/) const override {
        const DeviceGrid& grid = g_vpr_ctx.device().grid;
        int from_tile_idx = grid.get_physical_type(from_loc)->index;

        // The cost is used for placement, so the source is on OPINs; thus, we need the minimum delay
        // starting from OPINs, not from channels.
        return router_lookahead_.get_opin_distance_min_delay(from_tile_idx, from_loc, to_loc);
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
};
