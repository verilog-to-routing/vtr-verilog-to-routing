#pragma once

#include <map>
#include <rr_graph_fwd.h>
#include <vector>
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
                   e_route_type route_type,
                   std::vector<t_segment_inf>& segment_inf,
                   float R_minW_nmos,
                   float R_minW_pmos,
                   float grid_logic_tile_area,
                   e_directionality directionality,
                   RRSwitchId wire_to_ipin_switch,
                   bool is_flat);

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
