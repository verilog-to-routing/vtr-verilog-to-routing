#include "grid_tile_lookup.h"

GridTileLookup::GridTileLookup() {
    const auto& device_ctx = g_vpr_ctx.device();
    const int num_layers = device_ctx.grid.get_num_layers();

    //Will store the max number of tile locations for each logical block type
    max_placement_locations.resize(device_ctx.logical_block_types.size());

    for (const auto& type : device_ctx.logical_block_types) {
        vtr::NdMatrix<int, 3> type_count({static_cast<unsigned long>(num_layers), device_ctx.grid.width(), device_ctx.grid.height()});
        fill_type_matrix(&type, type_count);
        block_type_matrices.push_back(type_count);
    }
}

void GridTileLookup::fill_type_matrix(t_logical_block_type_ptr block_type, vtr::NdMatrix<int, 3>& type_count) {
    auto& device_ctx = g_vpr_ctx.device();

    int num_layers = device_ctx.grid.get_num_layers();
    int width = (int)device_ctx.grid.width();
    int height = (int)device_ctx.grid.height();
    /*
     * Iterating through every location on the grid to store the number of subtiles of
     * the correct type at each location. For each location, we store the cumulative
     * number of tiles of the type up to that location - meaning we store the number of
     * subtiles at the location, plus the number of subtiles at the locations above and to
     * the right of it.
     */
    std::vector<int> layer_acc_type_count(num_layers, 0);
    for (int layer_num = num_layers - 1; layer_num >= 0; layer_num--) {
        int num_rows = (int)device_ctx.grid.height();
        int num_cols = (int)device_ctx.grid.width();

        for (int i_col = width - 1; i_col >= 0; i_col--) {
            for (int j_row = height - 1; j_row >= 0; j_row--) {
                const auto& tile = device_ctx.grid.get_physical_type({i_col, j_row, layer_num});
                int height_offset = device_ctx.grid.get_height_offset({i_col, j_row, layer_num});
                int width_offset = device_ctx.grid.get_width_offset({i_col, j_row, layer_num});
                type_count[layer_num][i_col][j_row] = 0;

                if (is_tile_compatible(tile, block_type) && height_offset == 0 && width_offset == 0) {
                    for (const auto& sub_tile : tile->sub_tiles) {
                        if (is_sub_tile_compatible(tile, block_type, sub_tile.capacity.low)) {
                            type_count[layer_num][i_col][j_row] = sub_tile.capacity.total();
                            layer_acc_type_count[layer_num] += sub_tile.capacity.total();
                        }
                    }
                }

                if (i_col < num_cols - 1) {
                    type_count[layer_num][i_col][j_row] += type_count[layer_num][i_col + 1][j_row];
                }
                if (j_row < num_rows - 1) {
                    type_count[layer_num][i_col][j_row] += type_count[layer_num][i_col][j_row + 1];
                }
                if (i_col < (num_cols - 1) && j_row < (num_rows - 1)) {
                    type_count[layer_num][i_col][j_row] -= type_count[layer_num][i_col + 1][j_row + 1];
                }
                if (layer_num < num_layers - 1) {
                    type_count[layer_num][i_col][j_row] += layer_acc_type_count[layer_num + 1];
                }
            }
        }
    }

    //The total number of subtiles for the block type will be at [0][0]
    max_placement_locations[block_type->index] = type_count[0][0][0];
}

int GridTileLookup::total_type_tiles(t_logical_block_type_ptr block_type) const {
    return max_placement_locations[block_type->index];
}

int GridTileLookup::region_tile_count(const Region& reg, t_logical_block_type_ptr block_type) const {
    auto& device_ctx = g_vpr_ctx.device();
    const int n_layers = device_ctx.grid.get_num_layers();
    int subtile = reg.get_sub_tile();

    /*Intersect the region with the grid, in case the region passed in goes out of bounds
     * By intersecting with the grid, we ensure that we are only counting tiles for the part of the
     * region that fits on the grid.*/
    Region grid_reg(0, 0,
                    (int)device_ctx.grid.width() - 1, (int)device_ctx.grid.height() - 1,
                    0, n_layers - 1);
    Region intersect_reg = intersection(reg, grid_reg);

//    VTR_ASSERT(intersect_coord.layer_num == layer_num);

    const auto [xmin, ymin, xmax, ymax] = intersect_reg.get_rect().coordinates();
    const auto [layer_low, layer_high] = intersect_reg.get_layer_range();
    auto& layer_type_grid = block_type_matrices[block_type->index];

    int xdim = (int)layer_type_grid.dim_size(1);
    int ydim = (int)layer_type_grid.dim_size(2);

    int num_tiles = 0;

    if (subtile == NO_SUBTILE) {
        for (int l = layer_low; l <= layer_high; l++) {
            int num_tiles_in_layer = layer_type_grid[l][xmin][ymin];

            if ((ymax + 1) < ydim) {
                num_tiles_in_layer -= layer_type_grid[l][xmin][ymax + 1];
            }

            if ((xmax + 1) < xdim) {
                num_tiles_in_layer -= layer_type_grid[l][xmax + 1][ymin];
            }

            if ((xmax + 1) < xdim && (ymax + 1) < ydim) {
                num_tiles_in_layer += layer_type_grid[l][xmax + 1][ymax + 1];
            }

            num_tiles += num_tiles_in_layer;
        }
    } else {
        num_tiles = region_with_subtile_count(reg, block_type);
    }

    return num_tiles;
}

int GridTileLookup::region_with_subtile_count(const Region& reg, t_logical_block_type_ptr block_type) const {
    auto& device_ctx = g_vpr_ctx.device();
    int num_sub_tiles = 0;

    const vtr::Rect<int>& reg_rect = reg.get_rect();
    const auto [xmin, ymin, xmax, ymax] = reg_rect.coordinates();
    const auto [layer_low, layer_high] = reg.get_layer_range();
    int subtile = reg.get_sub_tile();

    for (int i = xmax; i >= xmin; i--) {
        for (int j = ymax; j >= ymin; j--) {
            for (int l = layer_low; l <= layer_high; l++) {
                const t_physical_tile_type_ptr tile_type = device_ctx.grid.get_physical_type({i, j, l});
                if (is_sub_tile_compatible(tile_type, block_type, subtile)) {
                    num_sub_tiles++;
                }
            }
        }
    }

    return num_sub_tiles;
}
