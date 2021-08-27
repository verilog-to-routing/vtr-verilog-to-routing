/*
 * This class is used to store a grid for each logical block type that stores the cumulative number of subtiles
 * for that type available at each location in the grid. The cumulative number of subtiles is the subtiles at the
 * location plus the subtiles available at the grid locations above and to the left of the locations.
 * Having these grids allows for O(1) lookups about the number of subtiles available for a given type of block
 * in a rectangular region.
 * This lookup class is used during initial placement when sorting blocks by the size of their floorplan constraint
 * regions.
 */
#ifndef VPR_SRC_PLACE_GRID_TILE_LOOKUP_H_
#define VPR_SRC_PLACE_GRID_TILE_LOOKUP_H_

#include "place_util.h"
#include "globals.h"

class GridTileLookup {
  public:
    vtr::NdMatrix<int, 2>& get_type_grid(t_logical_block_type_ptr block_type);

    void initialize_grid_tile_matrices();

    void fill_type_matrix(t_logical_block_type_ptr block_type, vtr::NdMatrix<int, 2>& type_count);

    void print_type_matrix(vtr::NdMatrix<int, 2>& type_count);

    int region_tile_count(const Region& reg, t_logical_block_type_ptr block_type);

    int region_with_subtile_count(const Region& reg, t_logical_block_type_ptr block_type);

    int total_type_tiles(t_logical_block_type_ptr block_type);

  private:
    std::vector<vtr::NdMatrix<int, 2>> block_type_matrices;

    std::vector<int> max_tile_counts;
};

#endif /* VPR_SRC_PLACE_GRID_TILE_LOOKUP_H_ */
