#ifndef ROUTE_BREADTH_FIRST_H
#define ROUTE_BREADTH_FIRST_H

#include "route_common.h"
#include "binary_heap.h"

bool try_breadth_first_route(const t_router_opts& router_opts);
bool try_breadth_first_route_net(BinaryHeap& heap, ClusterNetId net_id, float pres_fac, const t_router_opts& router_opts);

#endif
