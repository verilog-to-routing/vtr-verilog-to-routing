
#include "separable_delay_model.h"
#include "device_grid.h"
#include "globals.h"
#include "router_lookahead_separable.h"
#include "vpr_context.h"
#include "vpr_error.h"

void SeparableDelayModel::compute(RouterDelayProfiler& route_profiler,
                                  const t_placer_opts& /*placer_opts*/,
                                  const t_router_opts& /*router_opts*/,
                                  int /*longest_length*/) {
    const DeviceContext& device = g_vpr_ctx.device();
    const DeviceGrid& grid = device.grid;
    const size_t num_physical_tile_types = device.physical_tile_types.size();
    const size_t num_layers = grid.get_num_layers();

    // This model reads the separable lookahead's per-axis, absolute-coordinate cost maps directly, so
    // it cannot be built on top of any other lookahead.
    const auto* separable_lookahead = dynamic_cast<const SeparableLookahead*>(route_profiler.get_lookahead());
    if (separable_lookahead == nullptr) {
        VPR_FATAL_ERROR(VPR_ERROR_PLACE,
                        "The separable placement delay model requires the separable router lookahead "
                        "(--router_lookahead separable).");
    }

    // One table per axis, indexed by the absolute source and sink coordinate along that axis.
    // The second index is the layer the source is on and the third is the sink layer.
    x_delays_ = vtr::NdMatrix<float, 5>({num_physical_tile_types,
                                        num_layers,
                                        num_layers,
                                        grid.width(),
                                        grid.width()});

    y_delays_ = vtr::NdMatrix<float, 5>({num_physical_tile_types,
                                        num_layers,
                                        num_layers,
                                        grid.height(),
                                        grid.height()});

    // As in the simple delay model, the delay is the minimum across all the OPINs of the tile type,
    // taken from the router lookahead rather than by running the router. Here each axis is queried
    // separately, by absolute coordinate rather than by distance.
    for (size_t physical_tile_type_idx = 0; physical_tile_type_idx < num_physical_tile_types; ++physical_tile_type_idx) {
        for (size_t from_layer = 0; from_layer < num_layers; ++from_layer) {
            for (size_t to_layer = 0; to_layer < num_layers; ++to_layer) {
                for (size_t from_x = 0; from_x < grid.width(); ++from_x) {
                    for (size_t to_x = 0; to_x < grid.width(); ++to_x) {
                        x_delays_[physical_tile_type_idx][from_layer][to_layer][from_x][to_x] = separable_lookahead->get_opin_min_delay_x(physical_tile_type_idx,
                                                                                                                                         from_layer, to_layer,
                                                                                                                                         from_x, to_x);
                    }
                }

                for (size_t from_y = 0; from_y < grid.height(); ++from_y) {
                    for (size_t to_y = 0; to_y < grid.height(); ++to_y) {
                        y_delays_[physical_tile_type_idx][from_layer][to_layer][from_y][to_y] = separable_lookahead->get_opin_min_delay_y(physical_tile_type_idx,
                                                                                                                                         from_layer, to_layer,
                                                                                                                                         from_y, to_y);
                    }
                }
            }
        }
    }
}

float SeparableDelayModel::delay(const t_physical_tile_loc& from_loc, int /*from_pin*/, const t_physical_tile_loc& to_loc, int /*to_pin*/) const {
    const DeviceGrid& grid = g_vpr_ctx.device().grid;

    int from_tile_idx = grid.get_physical_type(from_loc)->index;

    float x_delay = x_delays_[from_tile_idx][from_loc.layer_num][to_loc.layer_num][from_loc.x][to_loc.x];
    float y_delay = y_delays_[from_tile_idx][from_loc.layer_num][to_loc.layer_num][from_loc.y][to_loc.y];

    return x_delay + y_delay;
}

void SeparableDelayModel::read(const std::string& /*file*/) {
    VPR_THROW(VPR_ERROR_PLACE, "SeparableDelayModel::read is not implemented.");
}

void SeparableDelayModel::write(const std::string& /*file*/) const {
    VPR_THROW(VPR_ERROR_PLACE, "SeparableDelayModel::write is not implemented.");
}
