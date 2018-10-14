#ifndef VPR_CHECK_ROUTE_H
#define VPR_CHECK_ROUTE_H
#include "physical_types.h"
#include "route_common.h"

void check_route(enum e_route_type route_type);

void recompute_occupancy_from_scratch();

#endif
