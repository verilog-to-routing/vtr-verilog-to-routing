#pragma once

/** @file Function prototypes for functions in route_common.cpp that
 * are used outside the router modules. */

#include <memory>

#include "route_common.h"
#include "timing_info_fwd.h"
#include "vpr_types.h"

#include "RoutingDelayCalculator.h"

std::vector<RRNodeId> collect_congested_rr_nodes();

vtr::vector<RRNodeId, std::set<ClusterNetId>> collect_rr_node_nets();

void free_route_structs();

void save_routing(vtr::vector<ParentNetId, vtr::optional<RouteTree>>& best_routing,
                  const t_clb_opins_used& clb_opins_used_locally,
                  t_clb_opins_used& saved_clb_opins_used_locally);

void restore_routing(vtr::vector<ParentNetId, vtr::optional<RouteTree>>& best_routing,
                     t_clb_opins_used& clb_opins_used_locally,
                     const t_clb_opins_used& saved_clb_opins_used_locally);

void get_serial_num(const Netlist<>& net_list);
