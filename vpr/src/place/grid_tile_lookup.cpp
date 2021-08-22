#include "grid_tile_lookup.h"

void GridTileLookup::initialize_grid_tile_matrices() {
    auto& device_ctx = g_vpr_ctx.device();

    for (const auto& type : device_ctx.logical_block_types) {
        vtr::NdMatrix<grid_tile_info, 2> type_count({device_ctx.grid.width(), device_ctx.grid.height()});
        fill_type_matrix(&type, type_count);
        block_type_matrices.push_back(type_count);
    }
}

void GridTileLookup::fill_type_matrix(t_logical_block_type_ptr block_type, vtr::NdMatrix<grid_tile_info, 2>& type_count) {
    auto& device_ctx = g_vpr_ctx.device();

    int num_rows = device_ctx.grid.height();
    int num_cols = device_ctx.grid.width();

    for (int i_col = type_count.dim_size(0) - 1; i_col >= 0; i_col--) {
        for (int j_row = type_count.dim_size(1) - 1; j_row >= 0; j_row--) {
            auto& tile = device_ctx.grid[i_col][j_row].type;
            type_count[i_col][j_row].cumulative_total = 0;
            type_count[i_col][j_row].st_range.set(0, 0);

            if (is_tile_compatible(tile, block_type)) {
                for (const auto& sub_tile : tile->sub_tiles) {
                    if (is_sub_tile_compatible(tile, block_type, sub_tile.capacity.low)) {
                        type_count[i_col][j_row].st_range.set(sub_tile.capacity.low, sub_tile.capacity.high);
                        type_count[i_col][j_row].cumulative_total = 1;
                    }
                }
            }

            if (i_col < num_cols - 1) {
                type_count[i_col][j_row].cumulative_total += type_count[i_col + 1][j_row].cumulative_total;
            }
            if (j_row < num_rows - 1) {
                type_count[i_col][j_row].cumulative_total += type_count[i_col][j_row + 1].cumulative_total;
            }
            if (i_col < (num_cols - 1) && j_row < (num_rows - 1)) {
                type_count[i_col][j_row].cumulative_total -= type_count[i_col + 1][j_row + 1].cumulative_total;
            }
        }
    }
}

vtr::NdMatrix<grid_tile_info, 2>& GridTileLookup::get_type_grid(t_logical_block_type_ptr block_type) {
    return block_type_matrices[block_type->index];
}

void GridTileLookup::print_type_matrix(vtr::NdMatrix<grid_tile_info, 2>& type_count) {
    for (int i_col = type_count.dim_size(0) - 1; i_col >= 0; i_col--) {
        for (int j_row = type_count.dim_size(1) - 1; j_row >= 0; j_row--) {
            VTR_LOG("%d ", type_count[i_col][j_row].cumulative_total);
        }
        VTR_LOG("\n");
    }
}
