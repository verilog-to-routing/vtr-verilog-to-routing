#ifndef VPR_COMPRESSED_GRID_H
#define VPR_COMPRESSED_GRID_H

#include "physical_types.h"

#include "vtr_geometry.h"
#include "vtr_flat_map.h"

struct t_compressed_block_grid {
    // The compressed grid of a block type stores only the coordinates that are occupied by that particular block type.
    // For instance, if a DSP block exists only in the 2nd, 3rd, and 5th columns, the compressed grid of X axis will solely store the values 2, 3, and 5.
    // Consequently, the compressed to_grid_x will contain only three members. The same approach is applicable to other compressed directions.
    // This compressed data structure helps to move blocks in a more efficient way. For instance, if I need to move a DSP block to the next compatible column, I can simply get
    // the next compatible column number by accessing the next element in the compressed grid instead of iterating over all columns to find the next compatible column.
    //If 'cx' is an index in the compressed grid space, then
    //'compressed_to_grid_x[cx]' is the corresponding location in the
    //full (uncompressed) device grid.
    std::vector<std::vector<int>> compressed_to_grid_x; // [0...num_layers-1][0...num_columns-1] -> uncompressed x
    std::vector<std::vector<int>> compressed_to_grid_y; // [0...num_layers-1][0...num_rows-1] -> uncompressed y
    std::vector<int> compressed_to_grid_layer;          // [0...num_layers-1] -> uncompressed layer

    //The grid is stored with a full/dense x-dimension (since only
    //x values which exist are considered), while the y-dimension is
    //stored sparsely, since we may not have full columns of blocks.
    //This makes it easy to check whether there exist
    std::vector<std::vector<vtr::flat_map2<int, t_physical_tile_loc>>> grid;

    //The sub type compatibility for a given physical tile and a compressed block grid
    //corresponding to the possible placement location for a given logical block
    //  - key: physical tile index
    //  - value: vector of compatible sub tiles for the physical tile/logical block pair
    std::unordered_map<int, std::vector<int>> compatible_sub_tiles_for_tile;

    inline size_t get_num_columns(int layer_num) const {
        return compressed_to_grid_x[layer_num].size();
    }

    inline size_t get_num_rows(int layer_num) const {
        return compressed_to_grid_y[layer_num].size();
    }

    inline t_physical_tile_loc grid_loc_to_compressed_loc(t_physical_tile_loc grid_loc) const {
        int cx = OPEN;
        int cy = OPEN;
        int layer_num = grid_loc.layer_num;

        auto itr_x = std::lower_bound(compressed_to_grid_x[layer_num].begin(), compressed_to_grid_x[layer_num].end(), grid_loc.x);
        VTR_ASSERT(*itr_x == grid_loc.x);
        cx = std::distance(compressed_to_grid_x[layer_num].begin(), itr_x);

        auto itr_y = std::lower_bound(compressed_to_grid_y[layer_num].begin(), compressed_to_grid_y[layer_num].end(), grid_loc.y);
        VTR_ASSERT(*itr_y == grid_loc.y);
        cy = std::distance(compressed_to_grid_y[layer_num].begin(), itr_y);

        return {cx, cy, layer_num};
    }

    /**
     * @brief Converts a grid location to its corresponding compressed location, rounding up where necessary.
     *
     * This function takes a physical tile location in the grid and converts it to the corresponding
     * compressed location. The conversion approximates by rounding up to the nearest valid compressed location.
     *
     * @param grid_loc The physical tile location in the grid.
     * @return The corresponding compressed location with the same layer number.
     */
    inline t_physical_tile_loc grid_loc_to_compressed_loc_approx_round_up(t_physical_tile_loc grid_loc) const {
        auto find_compressed_index = [](const std::vector<int>& compressed, int value) -> int {
            auto itr = std::upper_bound(compressed.begin(), compressed.end(), value);
            if (itr == compressed.begin())
                return 0;
            if (itr == compressed.end() || *(itr - 1) == value)
                return (int)std::distance(compressed.begin(), itr - 1);
            return (int)std::distance(compressed.begin(), itr);
        };

        int layer_num = grid_loc.layer_num;
        int cx = find_compressed_index(compressed_to_grid_x[layer_num], grid_loc.x);
        int cy = find_compressed_index(compressed_to_grid_y[layer_num], grid_loc.y);

        return {cx, cy, layer_num};
    }

    /**
     * @brief Converts a grid location to its corresponding compressed location, rounding down where necessary.
     *
     * This function takes a physical tile location in the grid and converts it to the corresponding
     * compressed location. The conversion approximates by rounding down to the nearest valid compressed location.
     *
     * @param grid_loc The physical tile location in the grid.
     * @return The corresponding compressed location with the same layer number.
     */
    inline t_physical_tile_loc grid_loc_to_compressed_loc_approx_round_down(t_physical_tile_loc grid_loc) const {
        auto find_compressed_index = [](const std::vector<int>& compressed, int value) -> int {
            auto itr = std::lower_bound(compressed.begin(), compressed.end(), value);
            if (itr == compressed.end()) {
                return (int)std::distance(compressed.begin(), itr - 1);
            }
            return (int)std::distance(compressed.begin(), itr);
        };

        int layer_num = grid_loc.layer_num;
        int cx = find_compressed_index(compressed_to_grid_x[layer_num], grid_loc.x);
        int cy = find_compressed_index(compressed_to_grid_y[layer_num], grid_loc.y);

        return {cx, cy, layer_num};
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
    inline t_physical_tile_loc grid_loc_to_compressed_loc_approx(t_physical_tile_loc grid_loc) const {
        auto find_closest_compressed_point = [](int loc, const std::vector<int>& compressed_grid_dim) -> int {
            auto itr = std::lower_bound(compressed_grid_dim.begin(), compressed_grid_dim.end(), loc);
            int cx;
            if (itr < compressed_grid_dim.end() - 1) {
                int dist_prev = abs(loc - *itr);
                int dist_next = abs(loc - *(itr+1));
                if (dist_prev < dist_next) {
                    cx = std::distance(compressed_grid_dim.begin(), itr);
                } else {
                    cx = std::distance(compressed_grid_dim.begin(), itr + 1);
                }
            } else if (itr == compressed_grid_dim.end()) {
                cx = std::distance(compressed_grid_dim.begin(), itr - 1);
            } else {
                cx = std::distance(compressed_grid_dim.begin(), itr);
            }

            return cx;
        };

        const int layer_num = grid_loc.layer_num;
        const int cx = find_closest_compressed_point(grid_loc.x, compressed_to_grid_x[layer_num]);
        const int cy = find_closest_compressed_point(grid_loc.y, compressed_to_grid_y[layer_num]);

        return {cx, cy, layer_num};
    }

    inline t_physical_tile_loc compressed_loc_to_grid_loc(t_physical_tile_loc compressed_loc) const {
        int layer_num = compressed_loc.layer_num;
        return {compressed_to_grid_x[layer_num][compressed_loc.x], compressed_to_grid_y[layer_num][compressed_loc.y], layer_num};
    }

    inline const std::vector<int>& compatible_sub_tile_num(int physical_type_index) const {
        return compatible_sub_tiles_for_tile.at(physical_type_index);
    }

    inline const vtr::flat_map2<int, t_physical_tile_loc>& get_column_block_map(int cx, int layer_num) const {
        return grid[layer_num][cx];
    }

    inline const std::vector<int>& get_layer_nums() const {
        return compressed_to_grid_layer;
    }
};

//Compressed grid space for each block type
//Used to efficiently find logically 'adjacent' blocks of the same block type even though
//the may be physically far apart
typedef std::vector<t_compressed_block_grid> t_compressed_block_grids;

std::vector<t_compressed_block_grid> create_compressed_block_grids();

/**
 * @brief  print the contents of the compressed grids to an echo file
 *
 *   @param filename the name of the file to print to
 *   @param comp_grids the compressed grids that are used during placement
 *
 */
void echo_compressed_grids(const char* filename, const std::vector<t_compressed_block_grid>& comp_grids);

#endif
