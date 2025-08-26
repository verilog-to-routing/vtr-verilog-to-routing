#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    March 2025
 * @brief   Utility methods for working with flat placements.
 */

#include <algorithm>
#include <cstdlib>
#include "device_grid.h"
#include "flat_placement_types.h"
#include "physical_types.h"

/**
 * @brief Returns the manhattan distance (L1 distance) between two flat
 *        placement locations.
 */
inline float get_manhattan_distance(const t_flat_pl_loc& loc_a,
                                    const t_flat_pl_loc& loc_b) {
    return std::abs(loc_a.x - loc_b.x) + std::abs(loc_a.y - loc_b.y) + std::abs(loc_a.layer - loc_b.layer);
}

/**
 * @brief Returns the L1 distance something at the given flat location would
 *        need to move to be within the bounds of a tile at the given tile loc.
 */
inline float get_manhattan_distance_to_tile(const t_flat_pl_loc& src_flat_loc,
                                            const t_physical_tile_loc& tile_loc,
                                            const DeviceGrid& device_grid) {
    // Get the bounds of the tile.
    // Note: The get_tile_bb function will not work in this case since it
    //       subtracts 1 from the width and height.
    auto tile_type = device_grid.get_physical_type(tile_loc);
    float tile_xmin = tile_loc.x - device_grid.get_width_offset(tile_loc);
    float tile_xmax = tile_xmin + tile_type->width;
    float tile_ymin = tile_loc.y - device_grid.get_height_offset(tile_loc);
    float tile_ymax = tile_ymin + tile_type->height;

    // Get the closest point in the bounding box (including the edges) to
    // the src_flat_loc. To do this, we project the point in L1 space.
    float proj_x = std::clamp(src_flat_loc.x, tile_xmin, tile_xmax);
    float proj_y = std::clamp(src_flat_loc.y, tile_ymin, tile_ymax);
    // Note: We assume that tiles do not cross layers, so the projected layer
    //       is just the layer that contains the tile.
    float proj_layer = tile_loc.layer_num;

    // Then compute the L1 distance from the src_flat_loc to the projected
    // position. This will be the minimum distance this point needs to move.
    float dx = std::abs(proj_x - src_flat_loc.x);
    float dy = std::abs(proj_y - src_flat_loc.y);
    float dlayer = std::abs(proj_layer - src_flat_loc.layer);
    return dx + dy + dlayer;
}
