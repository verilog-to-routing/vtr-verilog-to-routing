#pragma once

#include "routing_netlist_fwd.h"
#include "vpr_types.h"

void check_route(const RoutingNetlist& net_list,
                 enum e_route_type route_type,
                 e_check_route_option check_route_option);

void recompute_occupancy_from_scratch(const RoutingNetlist& net_list);
