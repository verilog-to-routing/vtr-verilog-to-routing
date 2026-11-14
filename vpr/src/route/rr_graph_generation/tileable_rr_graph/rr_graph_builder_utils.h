#pragma once

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <vector>

#include "device_grid.h"
#include "physical_types.h"
#include "rr_graph_type.h"
#include "vtr_geometry.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

size_t find_unidir_routing_channel_width(const size_t chan_width);

int get_grid_pin_class_index(const DeviceGrid& grids,
                             const size_t layer,
                             const size_t x,
                             const size_t y,
                             const int pin_index);

std::vector<e_side> find_grid_pin_sides(const DeviceGrid& grids,
                                        const size_t layer,
                                        const size_t x,
                                        const size_t y,
                                        const size_t pin_id);

std::vector<e_side> determine_io_grid_pin_side(const vtr::Point<size_t>& device_size,
                                               const vtr::Point<size_t>& grid_coordinate,
                                               const bool perimeter_cb);

/**
 * @brief Get a list of pin_index for a grid (either OPIN or IPIN)
 * For IO_TYPE, only one side will be used, we consider one side of pins 
 * For others, we consider all the sides
 */
std::vector<int> get_grid_side_pins(const DeviceGrid& grids,
                                    const size_t layer,
                                    const size_t x,
                                    const size_t y,
                                    const e_pin_type pin_type,
                                    const e_side pin_side,
                                    const int pin_width,
                                    const int pin_height);

size_t get_grid_num_pins(const DeviceGrid& grids,
                         const size_t layer,
                         const size_t x,
                         const size_t y,
                         const e_pin_type pin_type,
                         const std::vector<e_side>& io_side);

size_t get_grid_num_classes(const DeviceGrid& grids,
                            const size_t layer,
                            const size_t x,
                            const size_t y,
                            const e_pin_type pin_type);

bool is_chanx_exist(const DeviceGrid& grids,
                    const size_t layer,
                    const vtr::Point<size_t>& chanx_coord,
                    const bool perimeter_cb,
                    const bool through_channel = false);

bool is_chany_exist(const DeviceGrid& grids,
                    const size_t layer,
                    const vtr::Point<size_t>& chany_coord,
                    const bool perimeter_cb,
                    const bool through_channel = false);

bool is_chanx_right_to_multi_height_grid(const DeviceGrid& grids,
                                         const size_t layer,
                                         const vtr::Point<size_t>& chanx_coord,
                                         const bool perimeter_cb,
                                         const bool through_channel);

bool is_chanx_left_to_multi_height_grid(const DeviceGrid& grids,
                                        const size_t layer,
                                        const vtr::Point<size_t>& chanx_coord,
                                        const bool perimeter_cb,
                                        const bool through_channel);

bool is_chany_top_to_multi_width_grid(const DeviceGrid& grids,
                                      const size_t layer,
                                      const vtr::Point<size_t>& chany_coord,
                                      const bool perimeter_cb,
                                      const bool through_channel);

bool is_chany_bottom_to_multi_width_grid(const DeviceGrid& grids,
                                         const size_t layer,
                                         const vtr::Point<size_t>& chany_coord,
                                         const bool perimeter_cb,
                                         const bool through_channel);

/* A copy of the function from rr_graph2.cpp; This is keep tilable rr_graph builder self-contained */
int find_parallel_seg_index(const int abs_index,
                            const t_unified_to_parallel_seg_index& index_map,
                            const e_parallel_axis parallel_axis);
