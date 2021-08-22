/*
 * This class is used to stores a grid for each logical block type. The grid stores the number of cumulative
 * tiles that are available for this block type at each grid location. At each grid location, it also stores
 * the range of compatible subtiles for the block type.
 * This lookup class is used during initial placement when sorting blocks by the size of their floorplan constraint
 * regions.
 */
#ifndef VPR_SRC_PLACE_GRID_TILE_LOOKUP_H_
#define VPR_SRC_PLACE_GRID_TILE_LOOKUP_H_

#include "place_util.h"
#include "globals.h"

struct grid_tile_info {
    t_capacity_range st_range;
    int cumulative_total;
};

class GridTileLookup {
  public:
    vtr::NdMatrix<grid_tile_info, 2>& get_type_grid(t_logical_block_type_ptr block_type);

    void initialize_grid_tile_matrices();

    void fill_type_matrix(t_logical_block_type_ptr block_type, vtr::NdMatrix<grid_tile_info, 2>& type_count);

    void print_type_matrix(vtr::NdMatrix<grid_tile_info, 2>& type_count);

  private:
    std::vector<vtr::NdMatrix<grid_tile_info, 2>> block_type_matrices;
};

#endif /* VPR_SRC_PLACE_GRID_TILE_LOOKUP_H_ */
