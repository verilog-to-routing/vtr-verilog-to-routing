/*
 * grid_tile_lookup.h
 *
 *  Created on: Aug. 19, 2021
 *      Author: khalid88
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
