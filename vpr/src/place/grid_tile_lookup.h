/*
 * This class is used to store a grid for each logical block type that stores the cumulative number of subtiles
 * for that type available at each location in the grid. The cumulative number of subtiles is the subtiles at the
 * location plus the subtiles available at the grid locations above and to the right of the locations.
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
    /*
     * Stores the cumulative total of subtiles available at each location in the grid for each block type.
     * Therefore, the length of the vector will be the number of logical block types. To access the cumulative
     * number of subtiles at a location, you would use block_type_matrices[iblock_type][x][y] - this would
     * give the number of placement locations that are at, or above and to the right of the given [x,y] for
     * the given block type.
     */
    std::vector<vtr::NdMatrix<int, 2>> block_type_matrices;

    /*
     * Stores the total number of placement locations (i.e. compatible subtiles) for each block type.
     * To access the max_placement locations for a particular block type, use max_placement_locations[block_type->index]
     */
    std::vector<int> max_placement_locations;
};

#endif /* VPR_SRC_PLACE_GRID_TILE_LOOKUP_H_ */
