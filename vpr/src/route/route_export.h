#pragma once

/** @file Function prototypes for functions in route_common.cpp that
 * are used outside the router modules. */

#include <vector>
#include "clustered_netlist_fwd.h"
#include "netlist.h"
#include "route_tree.h"
#include "rr_graph_fwd.h"
#include "vtr_optional.h"
#include "vtr_vector.h"

std::vector<RRNodeId> collect_congested_rr_nodes();

vtr::vector<RRNodeId, std::set<ClusterNetId>> collect_rr_node_nets();

void free_route_structs();

void save_routing(vtr::vector<ParentNetId, vtr::optional<RouteTree>>& best_routing,
                  const t_clb_opins_used& clb_opins_used_locally,
                  t_clb_opins_used& saved_clb_opins_used_locally);

void restore_routing(const vtr::vector<ParentNetId, vtr::optional<RouteTree>>& best_routing,
                     t_clb_opins_used& clb_opins_used_locally,
                     const t_clb_opins_used& saved_clb_opins_used_locally);

void get_serial_num(const Netlist<>& net_list);
