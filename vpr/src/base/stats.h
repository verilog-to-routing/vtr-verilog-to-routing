#pragma once
#include <vector>
#include <limits>
#include <algorithm>
#include "vpr_types.h"

void routing_stats(bool full_stats, enum e_route_type route_type, std::vector<t_segment_inf>& segment_inf, float R_minW_nmos, float R_minW_pmos, float grid_logic_tile_area, enum e_directionality directionality, int wire_to_ipin_switch);

void print_wirelen_prob_dist();

void print_lambda();

void get_num_bends_and_length(ClusterNetId inet, int* bends, int* length, int* segments);

int count_netlist_clocks();

// template functions must be defined in header, or explicitely instantiated in definition file (defeats the point of template)
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
