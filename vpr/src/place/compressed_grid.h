#ifndef VPR_COMPRESSED_GRID_H
#define VPR_COMPRESSED_GRID_H

#include "physical_types.h"

#include "vtr_geometry.h"
#include "vtr_flat_map.h"

struct t_type_loc {
    int x = OPEN;
    int y = OPEN;
    int layer_num = 0;

    t_type_loc() = default;

    t_type_loc(int x_val, int y_val, int layer_num_val = 0)
        : x(x_val)
        , y(y_val)
        , layer_num(layer_num_val){}

    //Returns true if this type location has valid x/y values
    operator bool() const {
        return !(x == OPEN || y == OPEN || layer_num == OPEN);
    }
};

struct t_compressed_block_grid {
    //TODO: compressed_to_grid_x/y should become a 2d vector
    //If 'cx' is an index in the compressed grid space, then
    //'compressed_to_grid_x[cx]' is the corresponding location in the
    //full (uncompressed) device grid.
    std::vector<int> compressed_to_grid_x;
    std::vector<int> compressed_to_grid_y;

    //The grid is stored with a full/dense x-dimension (since only
    //x values which exist are considered), while the y-dimension is
    //stored sparsely, since we may not have full columns of blocks.
    //This makes it easy to check whether there exist
    std::vector<vtr::flat_map2<int, t_type_loc>> grid;

    //The sub type compatibility for a given physical tile and a compressed block grid
    //corresponding to the possible placement location for a given logical block
    //  - key: physical tile index
    //  - value: vector of compatible sub tiles for the physical tile/logical block pair
    std::unordered_map<int, std::vector<int>> compatible_sub_tiles_for_tile;

    inline size_t get_num_columns(int /*layer_num*/) const {
        return compressed_to_grid_x.size();
    }

    inline size_t get_num_rows(int /*layer_num*/) const {
        return compressed_to_grid_y.size();
    }

    inline t_type_loc grid_loc_to_compressed_loc(t_type_loc grid_loc) const {
        int cx = OPEN;
        int cy = OPEN;

        auto itr_x = std::lower_bound(compressed_to_grid_x.begin(), compressed_to_grid_x.end(), grid_loc.x);
        VTR_ASSERT(*itr_x == grid_loc.x);
        cx = std::distance(compressed_to_grid_x.begin(), itr_x);

        auto itr_y = std::lower_bound(compressed_to_grid_y.begin(), compressed_to_grid_y.end(), grid_loc.y);
        VTR_ASSERT(*itr_y == grid_loc.y);
        cy = std::distance(compressed_to_grid_y.begin(), itr_y);

        return {cx, cy};
    }

    inline t_type_loc grid_loc_to_compressed_loc_approx(t_type_loc grid_loc) const {
        int cx = OPEN;
        int cy = OPEN;

        auto itr_x = std::lower_bound(compressed_to_grid_x.begin(), compressed_to_grid_x.end(), grid_loc.x);
        if (itr_x == compressed_to_grid_x.end())
            cx = std::distance(compressed_to_grid_x.begin(), itr_x - 1);
        else
            cx = std::distance(compressed_to_grid_x.begin(), itr_x);

        auto itr_y = std::lower_bound(compressed_to_grid_y.begin(), compressed_to_grid_y.end(), grid_loc.y);
        if (itr_y == compressed_to_grid_y.end())
            cy = std::distance(compressed_to_grid_y.begin(), itr_y - 1);
        else
            cy = std::distance(compressed_to_grid_y.begin(), itr_y);

        return {cx, cy};
    }

    inline t_type_loc compressed_loc_to_grid_loc(t_type_loc compressed_loc) const {
        return {compressed_to_grid_x[compressed_loc.x], compressed_to_grid_y[compressed_loc.y], compressed_loc.layer_num};
    }

    inline const std::vector<int>& compatible_sub_tile_num(int physical_type_index) const {
        return compatible_sub_tiles_for_tile.at(physical_type_index);
    }


};

//Compressed grid space for each block type
//Used to efficiently find logically 'adjacent' blocks of the same block type even though
//the may be physically far apart
typedef std::vector<t_compressed_block_grid> t_compressed_block_grids;

std::vector<t_compressed_block_grid> create_compressed_block_grids();

t_compressed_block_grid create_compressed_block_grid(const std::vector<vtr::Point<int>>& locations);

int grid_to_compressed(const std::vector<int>& coords, int point);

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
int grid_to_compressed_approx(const std::vector<int>& coords, int point);

/**
 * @brief  print the contents of the compressed grids to an echo file
 *
 *   @param filename the name of the file to print to
 *   @param comp_grids the compressed grids that are used during placement
 *
 */
void echo_compressed_grids(char* filename, const std::vector<t_compressed_block_grid>& comp_grids);

#endif
