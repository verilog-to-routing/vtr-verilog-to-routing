#ifndef VPR_ROUTE_UTIL_H
#define VPR_ROUTE_UTIL_H
#include "vpr_types.h"
#include "draw_types.h"
#include "draw_global.h"

vtr::Matrix<float> calculate_routing_avail(t_rr_type rr_type);
vtr::Matrix<float> calculate_routing_usage(t_rr_type rr_type, bool is_flat);
float routing_util(float used, float avail);

#endif
