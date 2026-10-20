#pragma once

/**
 * @file Choke points of flat routing.
 *
 * On some architectures several sinks of a net in the same cluster can only be reached through a few
 * intra-cluster input pins, the choke points. To help the router converge on such architectures
 * (t_router_opts::has_choke_point), the choke points of every sink are found before routing and the
 * connection router lowers their congestion cost (see ConnectionParameters).
 */

#include <unordered_map>
#include <vector>
#include "netlist.h"
#include "rr_graph_fwd.h"
#include "vtr_vector.h"

/** If flat_routing and router_opt_choke_points are true, there are some choke points inside the cluster which would increase the convergence time of routing.
 * To address this issue, the congestion cost of those choke points needs to decrease. This function identify those choke points for each net,
 * and since the amount of congestion reduction is dependent on the number sinks reachable from that choke point, it also store the number of reachable sinks
 * for each choke point.
 * @param net_list
 * @param router_opt_choke_points is true if the given architecture has choking spots inside the cluster
 * @param is_flat is true if flat_routing is enabled
 * @return [Net_id][pin_id] -> [choke_point_rr_node_id, number of sinks reachable by this choke point] */
vtr::vector<ParentNetId, std::vector<std::unordered_map<RRNodeId, int>>> set_nets_choking_spots(const Netlist<>& net_list,
                                                                                                bool router_opt_choke_points,
                                                                                                bool is_flat);
