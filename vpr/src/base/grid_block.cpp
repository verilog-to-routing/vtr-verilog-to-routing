
#include "grid_block.h"

#include "device_grid.h"
#include "globals.h"
#include "physical_types.h"

void GridBlock::init_grid_blocks(const DeviceGrid& device_grid) {
    size_t grid_width = device_grid.width();
    size_t grid_height = device_grid.height();
    size_t num_layers = device_grid.get_num_layers();

    /* Structure should have the same dimensions as the grid. */
    grid_blocks_.resize({num_layers, grid_width, grid_height});

    for (size_t layer_num = 0; layer_num < num_layers; layer_num++) {
        for (size_t x = 0; x < grid_width; x++) {
            for (size_t y = 0; y < grid_height; y++) {
                const t_physical_tile_loc tile_loc({(int)x, (int)y, (int)layer_num});
                auto type = device_grid.get_physical_type(tile_loc);
                initialized_grid_block_at_location(tile_loc, type->capacity);
            }
        }
    }
}

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

void GridBlock::load_from_block_locs(const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();

    zero_initialize();

    for (ClusterBlockId blk_id : cluster_ctx.clb_nlist.blocks()) {
        t_pl_loc location = block_locs[blk_id].loc;

        VTR_ASSERT(location.x < (int)device_ctx.grid.width());
        VTR_ASSERT(location.y < (int)device_ctx.grid.height());

        set_block_at_location(location, blk_id);
        increment_usage({location.x, location.y, location.layer});
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


