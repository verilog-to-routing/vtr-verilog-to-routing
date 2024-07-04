#ifndef RR_GRAPH_BUILDER_UTILS_H
#define RR_GRAPH_BUILDER_UTILS_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include "device_grid.h"
#include "rr_graph_obj.h"
#include "rr_graph_type.h"
#include "vtr_geometry.h"

/********************************************************************
 * Function declaration
 *******************************************************************/

size_t find_unidir_routing_channel_width(const size_t& chan_width);

int get_grid_pin_class_index(const DeviceGrid& grids,
                             const size_t& layer,
                             const size_t& x,
                             const size_t& y,
                             const int pin_index);

std::vector<e_side> find_grid_pin_sides(const DeviceGrid& grids,
                                        const size_t& layer,
                                        const size_t& x,
                                        const size_t& y,
                                        const size_t& pin_id);

std::vector<e_side> determine_io_grid_pin_side(const vtr::Point<size_t>& device_size,
                                  const vtr::Point<size_t>& grid_coordinate,
                                  const bool& perimeter_cb);

std::vector<int> get_grid_side_pins(const DeviceGrid& grids,
                                    const size_t& layer,
                                    const size_t& x,
                                    const size_t& y,
                                    const e_pin_type& pin_type,
                                    const e_side& pin_side,
                                    const int& pin_width,
                                    const int& pin_height);

size_t get_grid_num_pins(const DeviceGrid& grids,
                         const size_t& layer,
                         const size_t& x,
                         const size_t& y,
                         const e_pin_type& pin_type,
                         const std::vector<e_side>& io_side);

size_t get_grid_num_classes(const DeviceGrid& grids,
                            const size_t& layer,
                            const size_t& x,
                            const size_t& y,
                            const e_pin_type& pin_type);

bool is_chanx_exist(const DeviceGrid& grids,
                    const size_t& layer,
                    const vtr::Point<size_t>& chanx_coord,
                    const bool& perimeter_cb,
                    const bool& through_channel = false);

bool is_chany_exist(const DeviceGrid& grids,
                    const size_t& layer,
                    const vtr::Point<size_t>& chany_coord,
                    const bool& perimeter_cb,
                    const bool& through_channel = false);

bool is_chanx_right_to_multi_height_grid(const DeviceGrid& grids,
                                         const size_t& layer,
                                         const vtr::Point<size_t>& chanx_coord,
                                         const bool& perimeter_cb,
                                         const bool& through_channel);

bool is_chanx_left_to_multi_height_grid(const DeviceGrid& grids,
                                        const size_t& layer,
                                        const vtr::Point<size_t>& chanx_coord,
                                        const bool& perimeter_cb,
                                        const bool& through_channel);

bool is_chany_top_to_multi_width_grid(const DeviceGrid& grids,
                                      const size_t& layer,
                                      const vtr::Point<size_t>& chany_coord,
                                      const bool& perimeter_cb,
                                      const bool& through_channel);

bool is_chany_bottom_to_multi_width_grid(const DeviceGrid& grids,
                                         const size_t& layer,
                                         const vtr::Point<size_t>& chany_coord,
                                         const bool& perimeter_cb,
                                         const bool& through_channel);

short get_rr_node_actual_track_id(const RRGraph& rr_graph,
                                  const RRNodeId& track_rr_node,
                                  const vtr::Point<size_t>& coord,
                                  const vtr::vector<RRNodeId, std::vector<size_t>>& tileable_rr_graph_node_track_ids);

vtr::Point<size_t> get_track_rr_node_start_coordinator(const RRGraph& rr_graph,
                                                       const RRNodeId& track_rr_node);

vtr::Point<size_t> get_track_rr_node_end_coordinator(const RRGraph& rr_graph,
                                                     const RRNodeId& track_rr_node);

short get_track_rr_node_end_track_id(const RRGraph& rr_graph,
                                     const RRNodeId& track_rr_node,
                                     const vtr::vector<RRNodeId, std::vector<size_t>>& tileable_rr_graph_node_track_ids);

short find_rr_graph_num_nodes(const RRGraph& rr_graph,
                              const std::vector<t_rr_type>& node_types);

short find_rr_graph_max_fan_in(const RRGraph& rr_graph,
                               const std::vector<t_rr_type>& node_types);

short find_rr_graph_min_fan_in(const RRGraph& rr_graph,
                               const std::vector<t_rr_type>& node_types);

short find_rr_graph_average_fan_in(const RRGraph& rr_graph,
                                   const std::vector<t_rr_type>& node_types);

void print_rr_graph_mux_stats(const RRGraph& rr_graph);

/* A copy of the function from rr_graph2.cpp; This is keep tilable rr_graph builder self-contained */
int find_parallel_seg_index(const int abs_index,
                            const t_unified_to_parallel_seg_index& index_map,
                            const e_parallel_axis parallel_axis);

#endif
