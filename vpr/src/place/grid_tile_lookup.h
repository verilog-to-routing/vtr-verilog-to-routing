#ifndef VPR_SRC_PLACE_GRID_TILE_LOOKUP_H_
#define VPR_SRC_PLACE_GRID_TILE_LOOKUP_H_

#include "place_util.h"
#include "globals.h"

/**
* @class GridTileLookup
* @brief This class is used to store a grid for each logical block type that stores the cumulative number of subtiles
* for that type available at each location in the grid.
*
* The cumulative number of subtiles is the subtiles at the location plus the subtiles available at the grid locations
* above and to the right of the locations. Having these grids allows for O(1) lookups about the number of subtiles
* available for a given type of block in a rectangular region.
* This lookup class is used during initial placement when sorting blocks by the size of their floorplan constraint regions.
*/
class GridTileLookup {
 public:
   /**
    * @brief Constructs a new GridTileLookup object.
    *
    * Creates a grid for each logical type and fills it with the cumulative number
    * of subtiles of that type.
    */
   GridTileLookup();

   /**
    * @brief Returns the number of subtiles available in the specified region for the given block type.
    *
    * This routine uses pre-computed values from the grids for each block type to get the number of grid tiles
    * covered by a region.
    * For a region with no subtiles specified, the number of grid tiles can be calculated by adding
    * and subtracting four values from within/at the edge of the region.
    * The region with subtile case is taken care of by a helper routine, region_with_subtile_count().
    *
    * @param reg The region to be queried.
    * @param block_type The type of logical block.
    * @return int The number of available subtiles.
    */
   int region_tile_count(const Region& reg, t_logical_block_type_ptr block_type) const;

   /**
    * @brief Returns the number of subtiles that can be placed in the specified region for the given block type.
    *
    * This routine is for the subtile specified case; an O(region_size) scan needs to be done to check whether each grid
    * location in the region is compatible for the block at the subtile specified.
    *
    * @param reg The region to be queried.
    * @param block_type The type of logical block.
    * @return int The number of subtiles with placement.
    */
   int region_with_subtile_count(const Region& reg, t_logical_block_type_ptr block_type) const;

   /**
    * @brief Returns the total number of tiles available for the specified block type.
    *
    * @param block_type The type of logical block.
    * @return int The total number of available tiles.
    */
   int total_type_tiles(t_logical_block_type_ptr block_type) const;

 private:
   /**
    * @brief Fills the type matrix with cumulative subtiles count for the given block type.
    *
    * @param block_type The type of logical block.
    * @param type_count The matrix to be filled with cumulative subtiles count.
    */
   void fill_type_matrix(t_logical_block_type_ptr block_type, vtr::NdMatrix<int, 3>& type_count);

   /**
    * @brief Stores the cumulative total of subtiles available at each (x, y) location in each layer
    * for all block types.
    *
    * Therefore, the length of the vector will be the number of logical block types. To access the cumulative
    * number of subtiles at a location in a specific layer, you would use block_type_matrices[iblock_type][layer][x][y].
    * This would give the number of placement locations that are at, or above (larger y) and to the right of the given [x,y] for
    * the given block type in the given layer.
    */
   std::vector<vtr::NdMatrix<int, 3>> block_type_matrices;

   /**
    * @brief Stores the total number of placement locations (i.e. compatible subtiles) for each block type.
    *
    * To access the max_placement locations for a particular block type, use max_placement_locations[block_type->index]
    */
   std::vector<int> max_placement_locations;
};

#endif /* VPR_SRC_PLACE_GRID_TILE_LOOKUP_H_ */