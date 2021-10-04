#ifndef VPR_COMPRESSED_GRID_H
#define VPR_COMPRESSED_GRID_H

#include "physical_types.h"

#include "vtr_geometry.h"
#include "vtr_flat_map.h"

struct t_type_loc {
    int x = OPEN;
    int y = OPEN;

    t_type_loc(int x_val, int y_val)
        : x(x_val)
        , y(y_val) {}

    //Returns true if this type location has valid x/y values
    operator bool() const {
        return !(x == OPEN || y == OPEN);
    }
};

struct t_compressed_block_grid {
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
