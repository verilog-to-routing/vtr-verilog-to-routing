#include "place_util.h"
#include "globals.h"

static vtr::Matrix<t_grid_blocks> init_grid_blocks();

void init_placement_context() {
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    place_ctx.block_locs.clear();
    place_ctx.block_locs.resize(cluster_ctx.clb_nlist.blocks().size());

    place_ctx.grid_blocks = init_grid_blocks();
}

static vtr::Matrix<t_grid_blocks> init_grid_blocks() {
    auto& device_ctx = g_vpr_ctx.device();

    auto grid_blocks = vtr::Matrix<t_grid_blocks>({device_ctx.grid.width(), device_ctx.grid.height()});
    for (size_t x = 0; x < device_ctx.grid.width(); ++x) {
        for (size_t y = 0; y < device_ctx.grid.height(); ++y) {
            auto type = device_ctx.grid[x][y].type;

            int capacity = type->capacity;

            grid_blocks[x][y].blocks.resize(capacity, EMPTY_BLOCK_ID);
        }
    }

    return grid_blocks;
}
