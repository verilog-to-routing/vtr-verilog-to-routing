
#include "simple_delay_model.h"


void SimpleDelayModel::compute(RouterDelayProfiler& route_profiler,
                               const t_placer_opts& /*placer_opts*/,
                               const t_router_opts& /*router_opts*/,
                               int /*longest_length*/) {
    const auto& grid = g_vpr_ctx.device().grid;
    const size_t num_physical_tile_types = g_vpr_ctx.device().physical_tile_types.size();
    const size_t num_layers = grid.get_num_layers();

    // Initializing the delay matrix to [num_physical_types][num_layers][num_layers][width][height]
    // The second index related to the layer that the source location is on and the third index is for the sink layer
    delays_ = vtr::NdMatrix<float, 5>({num_physical_tile_types,
                                       num_layers,
                                       num_layers,
                                       grid.width(),
                                       grid.height()});

    for (size_t physical_tile_type_idx = 0; physical_tile_type_idx < num_physical_tile_types; ++physical_tile_type_idx) {
        for (size_t from_layer = 0; from_layer < num_layers; ++from_layer) {
            for (size_t to_layer = 0; to_layer < num_layers; ++to_layer) {
                for (size_t dx = 0; dx < grid.width(); ++dx) {
                    for (size_t dy = 0; dy < grid.height(); ++dy) {
                        float min_delay = route_profiler.get_min_delay(physical_tile_type_idx,
                                                                       from_layer,
                                                                       to_layer,
                                                                       dx,
                                                                       dy);
                        delays_[physical_tile_type_idx][from_layer][to_layer][dx][dy] = min_delay;
                    }
                }
            }
        }
    }
}

float SimpleDelayModel::delay(const t_physical_tile_loc& from_loc, int /*from_pin*/, const t_physical_tile_loc& to_loc, int /*to_pin*/) const {
    int delta_x = std::abs(from_loc.x - to_loc.x);
    int delta_y = std::abs(from_loc.y - to_loc.y);

    int from_tile_idx = g_vpr_ctx.device().grid.get_physical_type(from_loc)->index;
    return delays_[from_tile_idx][from_loc.layer_num][to_loc.layer_num][delta_x][delta_y];
}