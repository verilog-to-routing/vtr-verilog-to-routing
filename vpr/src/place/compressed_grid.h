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
};

//Compressed grid space for each block type
//Used to efficiently find logically 'adjacent' blocks of the same block type even though
//the may be physically far apart
typedef std::vector<t_compressed_block_grid> t_compressed_block_grids;

std::vector<t_compressed_block_grid> create_compressed_block_grids();

t_compressed_block_grid create_compressed_block_grid(const std::vector<vtr::Point<int>>& locations);

int grid_to_compressed(const std::vector<int>& coords, int point);
#endif
