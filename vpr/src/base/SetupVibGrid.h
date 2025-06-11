#ifndef SETUPVIBGRID_H
#define SETUPVIBGRID_H

#include <vector>
#include "physical_types.h"

VibDeviceGrid create_vib_device_grid(std::string layout_name, const std::vector<t_vib_grid_def>& vib_grid_layouts);

#endif
