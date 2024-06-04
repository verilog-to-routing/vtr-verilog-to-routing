#ifndef VPR_ROUTE_UTIL_H
#define VPR_ROUTE_UTIL_H
#include "vpr_types.h"
#include "draw_types.h"
#include "draw_global.h"

vtr::Matrix<float> calculate_routing_avail(t_rr_type rr_type);

/**
 * @brief: Calculates and returns the usage over the entire grid for the specified
 * type of rr_node to the usage array. The usage is recorded at each (x,y) location.
 *
 * @param rr_type: Type of rr_node that we are calculating the usage of; can be CHANX or CHANY
 * @param is_flat: Is the flat router being used or not?
 * @param only_visible: If true, only record the usage of rr_nodes on layers that are visible according to the current
 * drawing settings.
 */
vtr::Matrix<float> calculate_routing_usage(t_rr_type rr_type, bool is_flat, bool is_print);
float routing_util(float used, float avail);

#endif
