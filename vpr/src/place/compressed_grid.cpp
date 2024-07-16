#include "compressed_grid.h"
#include "arch_util.h"
#include "globals.h"

/**
 * @brief Creates a compressed grid from the given locations.
 *
 *
 * @param locations The location of logical blocks of a specific type.
 * [0...layer-1][0...num_instances_on_layer-1] --> (x, y)
 * @param num_layers The number of dice.
 * @return t_compressed_block_grid The compressed grid created from the given locations.
 */
static t_compressed_block_grid create_compressed_block_grid(const std::vector<std::vector<vtr::Point<int>>>& locations,
                                                            int num_layers);


std::vector<t_compressed_block_grid> create_compressed_block_grids() {
    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;
    const int num_layers = grid.get_num_layers();

    //Collect the set of x/y locations for each instance of a block type
    // [logical_block_type][layer_num][0...num_instance_on_layer] -> (x, y)
    std::vector<std::vector<std::vector<vtr::Point<int>>>> block_locations(device_ctx.logical_block_types.size());

    for (const auto& block_type : device_ctx.logical_block_types) {
        block_locations[block_type.index].resize(num_layers);
    }

    for (int layer_num = 0; layer_num < num_layers; layer_num++) {
        for (int x = 0; x < (int)grid.width(); ++x) {
            for (int y = 0; y < (int)grid.height(); ++y) {
                int width_offset = grid.get_width_offset({x, y, layer_num});
                int height_offset = grid.get_height_offset(t_physical_tile_loc(x, y, layer_num));

                if (width_offset == 0 && height_offset == 0) {  // the bottom left corner of a tile
                    const auto& type = grid.get_physical_type({x, y, layer_num});
                    auto equivalent_sites = get_equivalent_sites_set(type);

                    for (t_logical_block_type_ptr block_type : equivalent_sites) {
                        //Only record at block root location
                        block_locations[block_type->index][layer_num].emplace_back(x, y);
                    }
                }
            }
        }
    }

    // each logical block type has its own compressed grid
    std::vector<t_compressed_block_grid> compressed_type_grids(device_ctx.logical_block_types.size());

    for (const auto& logical_block : device_ctx.logical_block_types) {
        auto compressed_block_grid = create_compressed_block_grid(block_locations[logical_block.index], num_layers);

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
static t_compressed_block_grid create_compressed_block_grid(const std::vector<std::vector<vtr::Point<int>>>& locations,
                                                            int num_layers) {
    t_compressed_block_grid compressed_grid;

    if (locations.empty()) {
        return compressed_grid;
    }

    {
        compressed_grid.compressed_to_grid_x.resize(num_layers);
        compressed_grid.compressed_to_grid_y.resize(num_layers);

        for (int layer_num = 0; layer_num < num_layers; layer_num++) {
            std::vector<int> layer_x_locs;
            std::vector<int> layer_y_locs;
            //Record all the x/y locations separately
            for (auto point : locations[layer_num]) {
                layer_x_locs.emplace_back(point.x());
                layer_y_locs.emplace_back(point.y());
            }

            //Uniquify x/y locations
            std::stable_sort(layer_x_locs.begin(), layer_x_locs.end());
            layer_x_locs.erase(unique(layer_x_locs.begin(), layer_x_locs.end()), layer_x_locs.end());

            std::stable_sort(layer_y_locs.begin(), layer_y_locs.end());
            layer_y_locs.erase(unique(layer_y_locs.begin(), layer_y_locs.end()), layer_y_locs.end());

            //The index of an x-position in x_locs corresponds to it's compressed
            //x-coordinate (similarly for y)
            if (!layer_x_locs.empty()) {
                compressed_grid.compressed_to_grid_layer.push_back(layer_num);
            }
            compressed_grid.compressed_to_grid_x[layer_num] = std::move(layer_x_locs);
            compressed_grid.compressed_to_grid_y[layer_num] = std::move(layer_y_locs);

            layer_x_locs.clear();
            layer_y_locs.clear();
        }
    }

    compressed_grid.grid.resize(num_layers);
    for (int layer_num = 0; layer_num < num_layers; layer_num++) {
        auto& layer_compressed_grid = compressed_grid.grid[layer_num];
        const auto& layer_compressed_x_locs = compressed_grid.compressed_to_grid_x[layer_num];
        const auto& layer_compressed_y_locs = compressed_grid.compressed_to_grid_y[layer_num];
        //
        //Build the compressed grid
        //

        //Create a full/dense x-dimension (since there must be at least one
        //block per x location)
        layer_compressed_grid.resize(layer_compressed_x_locs.size());

        //Fill-in the y-dimensions
        //
        //Note that we build the y-dimension sparsely (using a flat map), since
        //there may not be full columns of blocks at each x location, this makes
        //it efficient to find the non-empty blocks in the y dimension
        for (auto point : locations[layer_num]) {
            //Determine the compressed indices in the x & y dimensions
            auto x_itr = std::lower_bound(layer_compressed_x_locs.begin(), layer_compressed_x_locs.end(), point.x());
            int cx = std::distance(layer_compressed_x_locs.begin(), x_itr);

            auto y_itr = std::lower_bound(layer_compressed_y_locs.begin(), layer_compressed_y_locs.end(), point.y());
            int cy = std::distance(layer_compressed_y_locs.begin(), y_itr);

            VTR_ASSERT(cx >= 0 && cx < (int)layer_compressed_x_locs.size());
            VTR_ASSERT(cy >= 0 && cy < (int)layer_compressed_y_locs.size());

            VTR_ASSERT(layer_compressed_x_locs[cx] == point.x());
            VTR_ASSERT(layer_compressed_y_locs[cy] == point.y());

            auto result = layer_compressed_grid[cx].insert(std::make_pair(cy, t_physical_tile_loc(point.x(), point.y(), layer_num)));

            VTR_ASSERT_MSG(result.second, "Duplicates should not exist in compressed grid space");
        }
    }

    return compressed_grid;
}

/*Print the contents of the compressed grids to an echo file*/
void echo_compressed_grids(const char* filename, const std::vector<t_compressed_block_grid>& comp_grids) {
    FILE* fp;
    fp = vtr::fopen(filename, "w");

    auto& device_ctx = g_vpr_ctx.device();
    int num_layers = device_ctx.grid.get_num_layers();

    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "Compressed Grids: \n");
    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "\n");
    for (int layer_num = 0; layer_num < num_layers; layer_num++) {
        fprintf(fp, "Layer Num: %d \n", layer_num);
        fprintf(fp, "--------------------------------------------------------------\n");
        fprintf(fp, "\n");
        for (int i = 0; i < (int)comp_grids.size(); i++) {
            fprintf(fp, "\n\nGrid type: %s \n", device_ctx.logical_block_types[i].name);

            fprintf(fp, "X coordinates: \n");
            for (int j = 0; j < (int)comp_grids[i].compressed_to_grid_x[layer_num].size(); j++) {
                auto grid_loc = comp_grids[i].compressed_loc_to_grid_loc({j, 0, layer_num});
                fprintf(fp, "%d ", grid_loc.x);
            }
            fprintf(fp, "\n");

            fprintf(fp, "Y coordinates: \n");
            for (int k = 0; k < (int)comp_grids[i].compressed_to_grid_y[layer_num].size(); k++) {
                auto grid_loc = comp_grids[i].compressed_loc_to_grid_loc({0, k, layer_num});
                fprintf(fp, "%d ", grid_loc.y);
            }
            fprintf(fp, "\n");
            //TODO: Print the compatible sub-tiles for a logical block type
        }
    }

    fclose(fp);
}
