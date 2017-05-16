#ifndef ROUTE_BREADTH_FIRST_H
#define ROUTE_BREADTH_FIRST_H

#include "route_common.h"

bool try_breadth_first_route(t_router_opts router_opts,
		t_clb_opins_used& clb_opins_used_locally);
bool try_breadth_first_route_net(int inet, float pres_fac, 
		t_router_opts router_opts);

#endif
