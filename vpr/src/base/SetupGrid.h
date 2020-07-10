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

///@brief Find the device satisfying the specified minimum resources
DeviceGrid create_device_grid(std::string layout_name,
                              const std::vector<t_grid_def>& grid_layouts,
                              const std::map<t_logical_block_type_ptr, size_t>& minimum_instance_counts,
                              float target_device_utilization);

///@brief Find the device close in size to the specified dimensions
DeviceGrid create_device_grid(std::string layout_name, const std::vector<t_grid_def>& grid_layouts, size_t min_width, size_t min_height);

/**
 * @brief Calculate the device utilization
 *
 * Calculate the device utilization (i.e. fraction of used grid tiles)
 * foor the specified grid and resource requirements
 */
float calculate_device_utilization(const DeviceGrid& grid, std::map<t_logical_block_type_ptr, size_t> instance_counts);

/**
 * @brief Returns the effective size of the device
 *        (size of the bounding box of non-empty grid tiles)
 */
size_t count_grid_tiles(const DeviceGrid& grid);

#endif
