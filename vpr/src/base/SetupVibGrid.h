#pragma once

#include <vector>
#include "physical_types.h"

/**
 * @brief Create a VIB device grid
 * @param layout_name The name of the VIB device grid layout
 * @param vib_grid_layouts The list of VIB device grid layouts
 * @return The VIB device grid
 */
VibDeviceGrid create_vib_device_grid(std::string layout_name, const std::vector<t_vib_grid_def>& vib_grid_layouts);
