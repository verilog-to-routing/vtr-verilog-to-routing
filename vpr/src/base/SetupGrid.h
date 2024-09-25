#ifndef SETUPGRID_H
#define SETUPGRID_H

/**
 * @file
 * @author  Jason Luu
 * @date    October 8, 2008
 *
 * @brief   Initializes and allocates the physical logic block grid for VPR.
 */

#include <vector>
#include "physical_types.h"

class DeviceGrid;

///@brief Find the device satisfying the specified minimum resources
/// minimum_instance_counts and target_device_utilization are not required when specifying a fixed layout
DeviceGrid create_device_grid(const std::string& layout_name,
                              const std::vector<t_grid_def>& grid_layouts,
                              const std::map<t_logical_block_type_ptr, size_t>& minimum_instance_counts = {},
                              float target_device_utilization = 0.0);

///@brief Find the device close in size to the specified dimensions
DeviceGrid create_device_grid(const std::string& layout_name,
                              const std::vector<t_grid_def>& grid_layouts,
                              size_t min_width,
                              size_t min_height);

/**
 * @brief Calculate the device utilization
 *
 * Calculate the device utilization (i.e. fraction of used grid tiles)
 * foor the specified grid and resource requirements
 */
float calculate_device_utilization(const DeviceGrid& grid, const std::map<t_logical_block_type_ptr, size_t>& instance_counts);

/**
 * @brief Returns the effective size of the device
 *        (size of the bounding box of non-empty grid tiles)
 */
size_t count_grid_tiles(const DeviceGrid& grid);

#endif
