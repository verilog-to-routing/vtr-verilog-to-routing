#pragma once

/**
 * @file setup_vib_grid.h
 * @brief This file contains the functions to create a grid for device where
 * Versatile Interconnect Block (VIB) is used. For further details, please refer to
 * the following paper:
 * https://doi.org/10.1109/ICFPT59805.2023.00014
 * 
 */

#include <vector>
#include <string_view>

#include "physical_types.h"

/**
 * @brief Create a VIB device grid. This function is called when the 
 * architecture described in the architecture file use versatile interconnect block (VIB).
 * 
 * @param layout_name The name of the VIB device grid layout
 * @param vib_grid_layouts The list of VIB device grid layouts
 * @return The VIB device grid
 */
VibDeviceGrid create_vib_device_grid(std::string_view layout_name, const std::vector<t_vib_grid_def>& vib_grid_layouts);
