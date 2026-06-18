#pragma once
/**
 * @file
 * @author  Jason Luu
 * @date    October 8, 2008
 *
 * @brief   Initializes and allocates the physical logic block grid for VPR.
 */

#include <vector>
#include <optional>
#include "physical_types.h"

class DeviceGrid;
struct t_vpr_setup;

/// @brief Returns aspect ratio from the single auto_layout, or std::nullopt if none
std::optional<float> get_auto_layout_aspect_ratio(const std::vector<t_grid_def>& grid_layouts);

/// @brief Compute height for a fixed auto-layout width from the architecture aspect ratio
size_t compute_auto_layout_height(const std::vector<t_grid_def>& grid_layouts, size_t width);

/// @brief True when device size is fixed (named layout OR auto + device_width > 0)
bool has_fixed_device_size(const t_vpr_setup& vpr_setup);

/// @brief Find the device satisfying the specified minimum resources
/// minimum_instance_counts and target_device_utilization are not required when specifying a fixed layout
DeviceGrid create_device_grid(const std::string& layout_name,
                              const std::vector<t_grid_def>& grid_layouts,
                              const std::map<t_logical_block_type_ptr, size_t>& minimum_instance_counts = {},
                              float target_device_utilization = 0.0,
                              size_t fixed_device_width = 0);

///@brief Find the device close in size to the specified dimensions
DeviceGrid create_device_grid(const std::string& layout_name,
                              const std::vector<t_grid_def>& grid_layouts,
                              size_t min_width,
                              size_t min_height);

/**
 * @brief Returns the effective size of the device
 *        (size of the bounding box of non-empty grid tiles)
 */
size_t count_grid_tiles(const DeviceGrid& grid);

/**
 * @brief Logs a one-line summary of the device grid dimensions and tile count.
 *
 * The format of this log line is parsed by VTR's task parser to track device
 * size as a QoR metric across regression runs. Do not change the format without
 * also updating the task parser's regex patterns.
 */
void report_device_grid_stats(const DeviceGrid& grid);
