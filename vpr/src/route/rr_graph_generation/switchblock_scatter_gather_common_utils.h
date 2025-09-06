
#pragma once

#include "device_grid.h"
#include "rr_types.h"

#include <vector>

/**
 * @brief check whether a switch block exists in a specified coordinate within the device grid
 *
 *   @param grid device grid
 *   @param inter_cluster_rr used to check whether inter-cluster programmable routing resources exist in the current layer
 *   @param loc Coordinates of the given location to be evaluated.
 *   @param sb switchblock information specified in the architecture file
 *
 * @return true if a switch block exists at the specified location, false otherwise.
 */
bool sb_not_here(const DeviceGrid& grid,
                 const std::vector<bool>& inter_cluster_rr,
                 const t_physical_tile_loc& loc,
                 e_sb_location sb_location,
                 const t_specified_loc& specified_loc = t_specified_loc());

const t_chan_details& index_into_correct_chan(const t_physical_tile_loc& sb_loc,
                                              e_side src_side,
                                              const t_chan_details& chan_details_x,
                                              const t_chan_details& chan_details_y,
                                              t_physical_tile_loc& chan_loc,
                                              e_rr_type& chan_type);

bool chan_coords_out_of_bounds(const t_physical_tile_loc& loc, e_rr_type chan_type);