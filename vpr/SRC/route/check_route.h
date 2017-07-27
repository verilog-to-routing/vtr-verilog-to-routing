#ifndef VPR_CHECK_ROUTE_H
#define VPR_CHECK_ROUTE_H
#include "physical_types.h"
#include "route_common.h"

void check_route(enum e_route_type route_type, int num_switches,
		const t_clb_opins_used& clb_opins_used_locally, const t_segment_inf* segment_inf);

void recompute_occupancy_from_scratch(const t_clb_opins_used& clb_opins_used_locally);

#endif
