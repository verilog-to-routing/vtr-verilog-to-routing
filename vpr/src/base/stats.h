#pragma once

#include <map>
#include <vector>
#include <string_view>

#include "netlist.h"
#include "rr_graph_type.h"

class DeviceGrid;

/**
 * @brief Prints out various statistics about the current routing.
 *
 * Both a routing and an rr_graph must exist when you call this routine.
 */
void routing_stats(const Netlist<>& net_list,
                   bool full_stats,
                   enum e_route_type route_type,
                   std::vector<t_segment_inf>& segment_inf,
                   float R_minW_nmos,
                   float R_minW_pmos,
                   float grid_logic_tile_area,
                   enum e_directionality directionality,
                   int wire_to_ipin_switch,
                   bool is_flat);

/**
 * @brief Calculates the routing channel width at each grid location.
 *
 * Iterates through all RR nodes and counts how many wires pass through each (layer, x, y) location
 * for both horizontal (CHANX) and vertical (CHANY) channels.
 *
 * @return A pair of 3D matrices:
 *         - First: CHANX width per [layer][x][y]
 *         - Second: CHANY width per [layer][x][y]
 */
std::pair<vtr::NdMatrix<int, 3>, vtr::NdMatrix<int, 3>> calculate_channel_width();

void print_wirelen_prob_dist(bool is_flat);

void print_lambda();

void get_num_bends_and_length(ParentNetId inet, int* bends, int* length, int* segments, bool* is_absorbed_ptr);

int count_netlist_clocks();

/**
 * @brief Calculate the device utilization
 *
 * Calculate the device utilization (i.e. fraction of used grid tiles)
 * for the specified grid and resource requirements
 */
float calculate_device_utilization(const DeviceGrid& grid, const std::map<t_logical_block_type_ptr, size_t>& instance_counts);

/**
 * @brief Prints the number of resources in the netlist and the number of available resources in the architecture.
 */
void print_resource_usage();

/**
 * @brief Prints the device utilization
 * @param target_device_utilization The target device utilization set by the user
 */
void print_device_utilization(const float target_device_utilization);

/**
 * @brief Writes channel occupancy data to a file.
 *
 * Each row contains:
 *   - (x, y) coordinate
 *   - Occupancy count
 *   - Occupancy percentage (occupancy / capacity)
 *   - Channel capacity
 *
 *   TODO: extend to 3D
 *
 * @param filename      Output file path.
 * @param occupancy     Matrix of occupancy counts.
 * @param capacity_list List of channel capacities (per y for chanx, per x for chany).
 */
void write_channel_occupancy_table(const std::string_view filename,
                                   const vtr::Matrix<int>& occupancy,
                                   const std::vector<int>& capacity_list);
