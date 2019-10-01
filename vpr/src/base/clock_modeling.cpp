#include "clock_modeling.h"
#include "globals.h"
#include "vtr_assert.h"

void ClockModeling::treat_clock_pins_as_non_globals() {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    for (const auto& type : device_ctx.physical_tile_types) {
        if (!is_empty_type(&type)) {
            for (auto clock_pin_idx : type.get_clock_pins_indices()) {
                // clock pins should be originally considered as global when reading the architecture
                VTR_ASSERT(type.is_ignored_pin[clock_pin_idx]);

                type.is_ignored_pin[clock_pin_idx] = false;
            }
        }
    }
}
