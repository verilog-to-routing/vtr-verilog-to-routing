#pragma once

#include <string>
#include <vector>
#include "physical_types.h"

/// @brief Parse a .grid file and return a t_grid_def suitable for registration on t_arch.
/// @param grid_filepath Path to the .grid file.
/// @param physical_tile_types Tile types used to resolve tile names in the file.
t_grid_def read_grid_file(const std::string& grid_filepath,
                          const std::vector<t_physical_tile_type>& physical_tile_types);
