#include "grid_tile_lookup.h"

void GridTileLookup::fill_type_matrix(t_logical_block_type_ptr block_type, vtr::NdMatrix<int, 2>& type_count) {
    auto& device_ctx = g_vpr_ctx.device();

    int num_rows = device_ctx.grid.height();
    int num_cols = device_ctx.grid.width();

    /*
     * Iterating through every location on the grid to store the number of subtiles of
     * the correct type at each location. For each location, we store the cumulative
     * number of tiles of the type up to that location - meaning we store the number of
     * subtiles at the location, plus the number of subtiles at the locations above and to
     * the right of it.
     */
    for (int i_col = type_count.dim_size(0) - 1; i_col >= 0; i_col--) {
        for (int j_row = type_count.dim_size(1) - 1; j_row >= 0; j_row--) {
            auto& tile = device_ctx.grid[i_col][j_row].type;
            const t_grid_tile& grid_tile = device_ctx.grid[i_col][j_row];
            type_count[i_col][j_row] = 0;

            if (is_tile_compatible(tile, block_type) && grid_tile.height_offset == 0 && grid_tile.width_offset == 0) {
                for (const auto& sub_tile : tile->sub_tiles) {
                    if (is_sub_tile_compatible(tile, block_type, sub_tile.capacity.low)) {
                        type_count[i_col][j_row] = sub_tile.capacity.total();
                    }
                }
            }

            if (i_col < num_cols - 1) {
                type_count[i_col][j_row] += type_count[i_col + 1][j_row];
            }
            if (j_row < num_rows - 1) {
                type_count[i_col][j_row] += type_count[i_col][j_row + 1];
            }
            if (i_col < (num_cols - 1) && j_row < (num_rows - 1)) {
                type_count[i_col][j_row] -= type_count[i_col + 1][j_row + 1];
            }
        }
    }

    //The total number of subtiles for the block type will be at [0][0]
    max_placement_locations[block_type->index] = type_count[0][0];
}

vtr::NdMatrix<int, 2>& GridTileLookup::get_type_grid(t_logical_block_type_ptr block_type) {
    return block_type_matrices[block_type->index];
}

int GridTileLookup::total_type_tiles(t_logical_block_type_ptr block_type) {
    return max_placement_locations[block_type->index];
}

/*
 * This routine uses pre-computed values from the grids for each block type to get the number of grid tiles
 * covered by a region.
 * For a region with no subtiles specified, the number of grid tiles can be calculated by adding
 * and subtracting four values from within/at the edge of the region.
 * The region with subtile case is taken care of by a helper routine, region_with_subtile_count().
 */
int GridTileLookup::region_tile_count(const Region& reg, t_logical_block_type_ptr block_type) {
    auto& device_ctx = g_vpr_ctx.device();
    int subtile = reg.get_sub_tile();

    /*Intersect the region with the grid, in case the region passed in goes out of bounds
     * By intersecting with the grid, we ensure that we are only counting tiles for the part of the
     * region that fits on the grid.*/
    Region grid_reg;
    grid_reg.set_region_rect(0, 0, device_ctx.grid.width() - 1, device_ctx.grid.height() - 1);
    Region intersect_reg;
    intersect_reg = intersection(reg, grid_reg);

    vtr::Rect<int> intersect_rect = intersect_reg.get_region_rect();

    int xmin = intersect_rect.xmin();
    int ymin = intersect_rect.ymin();
    int xmax = intersect_rect.xmax();
    int ymax = intersect_rect.ymax();
    auto& type_grid = block_type_matrices[block_type->index];

    int xdim = type_grid.dim_size(0);
    int ydim = type_grid.dim_size(1);

    int num_tiles = 0;

    if (subtile == NO_SUBTILE) {
        num_tiles = type_grid[xmin][ymin];

        if ((ymax + 1) < ydim) {
            num_tiles -= type_grid[xmin][ymax + 1];
        }

        if ((xmax + 1) < xdim) {
            num_tiles -= type_grid[xmax + 1][ymin];
        }

        if ((xmax + 1) < xdim && (ymax + 1) < ydim) {
            num_tiles += type_grid[xmax + 1][ymax + 1];
        }
    } else {
        num_tiles = region_with_subtile_count(reg, block_type);
    }

    return num_tiles;
}

/*
 * This routine is for the subtile specified case; an O(region_size) scan needs to be done to check whether each grid
 * location in the region is compatible for the block at the subtile specified.
 */
int GridTileLookup::region_with_subtile_count(const Region& reg, t_logical_block_type_ptr block_type) {
    auto& device_ctx = g_vpr_ctx.device();
    int num_sub_tiles = 0;
    vtr::Rect<int> reg_rect = reg.get_region_rect();
    int subtile = reg.get_sub_tile();

    int xmin = reg_rect.xmin();
    int ymin = reg_rect.ymin();
    int xmax = reg_rect.xmax();
    int ymax = reg_rect.ymax();

    for (int i = xmax; i >= xmin; i--) {
        for (int j = ymax; j >= ymin; j--) {
            auto& tile = device_ctx.grid[i][j].type;
            if (is_sub_tile_compatible(tile, block_type, subtile)) {
                num_sub_tiles++;
            }
        }
    }

    return num_sub_tiles;
}
