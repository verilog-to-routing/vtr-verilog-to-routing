#ifndef VPR_CHECK_ROUTE_H
#define VPR_CHECK_ROUTE_H
#include "physical_types.h"

void check_route(enum e_route_type route_type, int num_switches,
		vtr::t_ivec ** clb_opins_used_locally, const t_segment_inf* segment_inf);

#endif
