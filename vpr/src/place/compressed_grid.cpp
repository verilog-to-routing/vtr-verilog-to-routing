#include "compressed_grid.h"
#include "arch_util.h"
#include "globals.h"

std::vector<t_compressed_block_grid> create_compressed_block_grids() {
    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

    //Collect the set of x/y locations for each instace of a block type
    std::vector<std::vector<vtr::Point<int>>> block_locations(device_ctx.logical_block_types.size());
    for (size_t x = 0; x < grid.width(); ++x) {
        for (size_t y = 0; y < grid.height(); ++y) {
            const t_grid_tile& tile = grid[x][y];
            if (tile.width_offset == 0 && tile.height_offset == 0) {
                auto equivalent_sites = get_equivalent_sites_set(tile.type);

                for (auto& block : equivalent_sites) {
                    //Only record at block root location
                    block_locations[block->index].emplace_back(x, y);
                }
            }
        }
    }

    std::vector<t_compressed_block_grid> compressed_type_grids(device_ctx.logical_block_types.size());
    for (const auto& logical_block : device_ctx.logical_block_types) {
        auto compressed_block_grid = create_compressed_block_grid(block_locations[logical_block.index]);

        for (const auto& physical_tile : logical_block.equivalent_tiles) {
            std::vector<int> compatible_sub_tiles;

            // Build a vector to store all the sub tile indices that are compatible with the
            // (physical_tile, logical_block) pair.
            //
            // Given that a logical block can potentially be compatible with only a subset of the
            // sub tiles of a physical tile, we need to ensure which sub tile locations are part of
            // this subset.
            for (int isub_tile = 0; isub_tile < physical_tile->capacity; isub_tile++) {
                if (is_sub_tile_compatible(physical_tile, &logical_block, isub_tile)) {
                    compatible_sub_tiles.push_back(isub_tile);
                }
            }

            // For each of physical tiles compatible with the current logical block, create add an entry
            // to the compatible sub tiles map with the physical tile index and the above generated vector.
            compressed_block_grid.compatible_sub_tiles_for_tile.insert({physical_tile->index, compatible_sub_tiles});
        }

        compressed_type_grids[logical_block.index] = compressed_block_grid;
    }

    return compressed_type_grids;
}

//Given a set of locations, returns a 2D matrix in a compressed space
t_compressed_block_grid create_compressed_block_grid(const std::vector<vtr::Point<int>>& locations) {
    t_compressed_block_grid compressed_grid;

    if (locations.empty()) {
        return compressed_grid;
    }

    {
        std::vector<int> x_locs;
        std::vector<int> y_locs;

        //Record all the x/y locations seperately
        for (auto point : locations) {
            x_locs.emplace_back(point.x());
            y_locs.emplace_back(point.y());
        }

        //Uniquify x/y locations
        std::sort(x_locs.begin(), x_locs.end());
        x_locs.erase(unique(x_locs.begin(), x_locs.end()), x_locs.end());

        std::sort(y_locs.begin(), y_locs.end());
        y_locs.erase(unique(y_locs.begin(), y_locs.end()), y_locs.end());

        //The index of an x-position in x_locs corresponds to it's compressed
        //x-coordinate (similarly for y)
        compressed_grid.compressed_to_grid_x = x_locs;
        compressed_grid.compressed_to_grid_y = y_locs;
    }

    //
    //Build the compressed grid
    //

    //Create a full/dense x-dimension (since there must be at least one
    //block per x location)
    compressed_grid.grid.resize(compressed_grid.compressed_to_grid_x.size());

    //Fill-in the y-dimensions
    //
    //Note that we build the y-dimension sparsely (using a flat map), since
    //there may not be full columns of blocks at each x location, this makes
    //it efficient to find the non-empty blocks in the y dimension
    for (auto point : locations) {
        //Determine the compressed indices in the x & y dimensions
        auto x_itr = std::lower_bound(compressed_grid.compressed_to_grid_x.begin(), compressed_grid.compressed_to_grid_x.end(), point.x());
        int cx = std::distance(compressed_grid.compressed_to_grid_x.begin(), x_itr);

        auto y_itr = std::lower_bound(compressed_grid.compressed_to_grid_y.begin(), compressed_grid.compressed_to_grid_y.end(), point.y());
        int cy = std::distance(compressed_grid.compressed_to_grid_y.begin(), y_itr);

        VTR_ASSERT(cx >= 0 && cx < (int)compressed_grid.compressed_to_grid_x.size());
        VTR_ASSERT(cy >= 0 && cy < (int)compressed_grid.compressed_to_grid_y.size());

        VTR_ASSERT(compressed_grid.compressed_to_grid_x[cx] == point.x());
        VTR_ASSERT(compressed_grid.compressed_to_grid_y[cy] == point.y());

        auto result = compressed_grid.grid[cx].insert(std::make_pair(cy, t_type_loc(point.x(), point.y())));

        VTR_ASSERT_MSG(result.second, "Duplicates should not exist in compressed grid space");
    }

    return compressed_grid;
}

int grid_to_compressed(const std::vector<int>& coords, int point) {
    auto itr = std::lower_bound(coords.begin(), coords.end(), point);
    VTR_ASSERT(*itr == point);

    return std::distance(coords.begin(), itr);
}

/**
 * @brief  find the nearest location in the compressed grid.
 *
 * Useful when the point is of a different block type from coords.
 * 
 *   @param point represents a coordinate in one dimension of the point
 *   @param coords represents vector of coordinate values of a single type only
 *
 * Hence, the exact point coordinate will not be found in coords if they are of different block types. In this case the function will return 
 * the nearest compressed location to point by rounding it down 
 */
int grid_to_compressed_approx(const std::vector<int>& coords, int point) {
    auto itr = std::lower_bound(coords.begin(), coords.end(), point);
    if (itr == coords.end())
        return std::distance(coords.begin(), itr - 1);
    return std::distance(coords.begin(), itr);
}

/*Print the contents of the compressed grids to an echo file*/
void echo_compressed_grids(char* filename, const std::vector<t_compressed_block_grid>& comp_grids) {
    FILE* fp;
    fp = vtr::fopen(filename, "w");

    auto& device_ctx = g_vpr_ctx.device();

    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "Compressed Grids: \n");
    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "\n");

    for (int i = 0; i < (int)comp_grids.size(); i++) {
        fprintf(fp, "\n\nGrid type: %s \n", device_ctx.logical_block_types[i].name);

        fprintf(fp, "X coordinates: \n");
        for (int j = 0; j < (int)comp_grids[i].compressed_to_grid_x.size(); j++) {
            fprintf(fp, "%d ", comp_grids[i].compressed_to_grid_x[j]);
        }
        fprintf(fp, "\n");

        fprintf(fp, "Y coordinates: \n");
        for (int k = 0; k < (int)comp_grids[i].compressed_to_grid_y.size(); k++) {
            fprintf(fp, "%d ", comp_grids[i].compressed_to_grid_y[k]);
        }
        fprintf(fp, "\n");

        fprintf(fp, "Subtiles: \n");
        for (int s = 0; s < (int)comp_grids[i].compatible_sub_tiles_for_tile.size(); s++) {
            fprintf(fp, "%d ", comp_grids[i].compressed_to_grid_y[s]);
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
}
