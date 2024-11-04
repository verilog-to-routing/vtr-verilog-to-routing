#ifndef SETUPVIBGRID_H
#define SETUPVIBGRID_H

/**
 * @file
 * @author  Jason Luu
 * @date    October 8, 2008
 *
 * @brief   Initializes and allocates the physical logic block grid for VPR.
 */

#include <vector>
#include "physical_types.h"

VibDeviceGrid create_vib_device_grid(std::string layout_name, const std::vector<t_vib_grid_def>& vib_grid_layouts);

#endif
