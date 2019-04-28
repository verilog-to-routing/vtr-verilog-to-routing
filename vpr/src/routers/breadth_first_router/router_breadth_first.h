#ifndef ROUTER_BREADTH_FIRST_H
#define ROUTER_BREADTH_FIRST_H

#include "router_common.h"

namespace router {

bool try_breadth_first_route(t_router_opts router_opts);
bool try_breadth_first_route_net(ClusterNetId net_id, float pres_fac,
		t_router_opts router_opts);

}

#endif
