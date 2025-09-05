
#pragma once

#include "device_grid.h"

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