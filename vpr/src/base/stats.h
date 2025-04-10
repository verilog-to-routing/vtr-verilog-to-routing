#pragma once
#include <vector>
#include <limits>
#include <algorithm>
#include "vpr_types.h"
#include "netlist.h"

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
 * @brief template functions must be defined in header, or explicitely
 *        instantiated in definition file (defeats the point of template)
 */
template<typename T>
double linear_regression_vector(const std::vector<T>& vals, size_t start_x = 0) {
    // returns slope; index is x, val is y
    size_t n{vals.size() - start_x};

    double x_avg{0}, y_avg{0};
    for (size_t x = start_x; x < vals.size(); ++x) {
        x_avg += x;
        y_avg += vals[x];
    }
    x_avg /= (double)n;
    y_avg /= (double)n;

    double numerator = 0, denominator = 0;
    for (size_t x = start_x; x < vals.size(); ++x) {
        numerator += (x - x_avg) * (vals[x] - y_avg);
        denominator += (x - x_avg) * (x - x_avg);
    }

    if (denominator == 0) return std::numeric_limits<double>::max();
    return numerator / denominator;
}
