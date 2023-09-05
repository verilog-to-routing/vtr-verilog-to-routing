#ifndef VPR_ROUTE_UTIL_H
#define VPR_ROUTE_UTIL_H
#include "vpr_types.h"

vtr::Matrix<float> calculate_routing_avail(t_rr_type rr_type);

/**
* @brief Calculates routing usage over entire grid
*
* Collects all in-use nodes and records number of used resources
* in each x/y channel. Takes into consideration visible layers for
* multi-layered architectures. 
*/
vtr::Matrix<float> calculate_routing_usage(t_rr_type rr_type, bool is_flat);
float routing_util(float used, float avail);

#endif
