#include "clock_modeling.h"
#include "globals.h"
#include "vtr_assert.h"

void ClockModeling::treat_clock_pins_as_non_globals() {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    for (int type_idx = 0; type_idx < device_ctx.num_block_types; type_idx++) {
        auto type = &device_ctx.physical_tile_types[type_idx];
        if (logical_block_type(type)->pb_type) {
            for (auto clock_pin_idx : type->get_clock_pins_indices(logical_block_type(type))) {
                // clock pins should be originally considered as global when reading the architecture
                VTR_ASSERT(type->is_ignored_pin[clock_pin_idx]);

                type->is_ignored_pin[clock_pin_idx] = false;
            }
        }
    }
}
