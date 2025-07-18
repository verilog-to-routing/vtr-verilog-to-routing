
#include "pin_density_manager.h"

#include "globals.h"

PinDensityManager::PinDensityManager(const PlacerState& placer_state)
    : placer_state_(placer_state) {
    const auto& grid = g_vpr_ctx.device().grid;

    const size_t grid_width = grid.width();
    const size_t grid_height = grid.height();

    chanx_input_pin_count_.resize({grid_width, grid_height}, 0);
    chany_input_pin_count_.resize({grid_width, grid_height}, 0);
    chanx_output_pin_count_.resize({grid_width, grid_height}, 0);
    chany_output_pin_count_.resize({grid_width, grid_height}, 0);
}

void PinDensityManager::find_affected_channels_and_update_costs() {
}

double PinDensityManager::compute_cost() {
}

void PinDensityManager::update_move_channels() {
}

void PinDensityManager::reset_move_channels() {
}
