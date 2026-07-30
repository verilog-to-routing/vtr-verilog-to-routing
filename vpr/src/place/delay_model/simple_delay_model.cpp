
#include "simple_delay_model.h"
#include <tuple>
#include "device_grid.h"
#include "globals.h"
#include "router_lookahead_interposer.h"
#include "vpr_context.h"
#include "vtr_assert.h"

void SimpleDelayModel::compute(RouterDelayProfiler& /*route_profiler*/,
                               const t_placer_opts& /*placer_opts*/,
                               const t_router_opts& /*router_opts*/,
                               int /*longest_length*/) {
    const DeviceContext& device = g_vpr_ctx.device();
    const DeviceGrid& grid = device.grid;

    if (grid.has_interposer_cuts()) {
        // We don't use the base cost in the simple delay model, so we set the multiplier to 1.
        interposer_lookahead_.emplace(device.rr_graph, grid, device, /*interposer_cut_base_cost_multiplier*/ 1);
    }
}

float SimpleDelayModel::delay(const t_physical_tile_loc& from_loc, int /*from_pin*/, const t_physical_tile_loc& to_loc, int /*to_pin*/) const {
    const DeviceGrid& grid = g_vpr_ctx.device().grid;
    int delta_x = std::abs(from_loc.x - to_loc.x);
    int delta_y = std::abs(from_loc.y - to_loc.y);

    int from_tile_idx = grid.get_physical_type(from_loc)->index;

    float interposer_delay = 0.f;
    if (interposer_lookahead_) {
        VTR_ASSERT_SAFE(grid.has_interposer_cuts());
        std::tie(interposer_delay, std::ignore) = interposer_lookahead_->get_interposer_lookahead_cost(from_loc, to_loc);
    }

    float min_delay = router_lookahead_.get_opin_distance_min_delay(from_tile_idx,
                                                                    from_loc.layer_num, to_loc.layer_num,
                                                                    delta_x, delta_y);
    return min_delay + interposer_delay;
}

void SimpleDelayModel::read(const std::string& /*file*/) {
    VPR_THROW(VPR_ERROR_PLACE,
              "SimpleDelayModel does not support reading a placement delay lookup: "
              "it queries the router lookahead directly instead of storing a delay matrix.");
}

void SimpleDelayModel::write(const std::string& /*file*/) const {
    VPR_THROW(VPR_ERROR_PLACE,
              "SimpleDelayModel does not support writing a placement delay lookup: "
              "it queries the router lookahead directly instead of storing a delay matrix.");
}
