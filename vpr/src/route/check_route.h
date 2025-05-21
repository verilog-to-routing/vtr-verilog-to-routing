#ifndef VPR_CHECK_ROUTE_H
#define VPR_CHECK_ROUTE_H

#include "netlist.h"
#include "vpr_types.h"

void check_route(const Netlist<>& net_list,
                 enum e_route_type route_type,
                 e_check_route_option check_route_option,
                 bool is_flat);

void recompute_occupancy_from_scratch(const Netlist<>& net_list, bool is_flat);

#endif
