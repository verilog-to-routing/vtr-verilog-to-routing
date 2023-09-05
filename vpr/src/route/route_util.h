#ifndef VPR_ROUTE_UTIL_H
#define VPR_ROUTE_UTIL_H
#include "vpr_types.h"
#include "draw_types.h"
#include "draw_global.h"

vtr::Matrix<float> calculate_routing_avail(t_rr_type rr_type);

/**
 * @brief Calculates routing usage over entire grid
 * Collects all in-use nodes and records number of used resources
 * in each x/y channel. Takes into consideration visible layers
 * for multi-layered architectures. Also takes into consideration
 * if it is being printed, if so, layer visibility is ignored.
 */
vtr::Matrix<float> calculate_routing_usage(t_rr_type rr_type, bool is_flat, bool is_print);
float routing_util(float used, float avail);

#endif
