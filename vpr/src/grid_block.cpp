
#include "grid_block.h"

#include "globals.h"

void GridBlock::zero_initialize() {
    auto& device_ctx = g_vpr_ctx.device();

    /* Initialize all occupancy to zero. */
    for (int layer_num = 0; layer_num < (int)device_ctx.grid.get_num_layers(); layer_num++) {
        for (int i = 0; i < (int)device_ctx.grid.width(); i++) {
            for (int j = 0; j < (int)device_ctx.grid.height(); j++) {
                set_usage({i, j, layer_num}, 0);
                auto tile = device_ctx.grid.get_physical_type({i, j, layer_num});

                for (const auto& sub_tile : tile->sub_tiles) {
                    auto capacity = sub_tile.capacity;

                    for (int k = 0; k < capacity.total(); k++) {
                        set_block_at_location({i, j, k + capacity.low, layer_num}, ClusterBlockId::INVALID());
                    }
                }
            }
        }
    }
}

int GridBlock::increment_usage(const t_physical_tile_loc& loc) {
    int curr_usage = get_usage(loc);
    int updated_usage = set_usage(loc, curr_usage + 1);

    return updated_usage;
}

int GridBlock::decrement_usage(const t_physical_tile_loc& loc) {
    int curr_usage = get_usage(loc);
    int updated_usage = set_usage(loc, curr_usage - 1);

    return updated_usage;
}
