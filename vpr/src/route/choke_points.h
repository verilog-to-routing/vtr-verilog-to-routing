#pragma once

/**
 * @file
 * @brief Find choke points for flat routing.
 *
 * Some architectures route several sinks in a cluster through a few shared input pins.
 * The connection router lowers congestion costs at these choke points to help routing
 * converge (see ConnectionParameters).
 */

#include <unordered_map>
#include <vector>
#include "netlist.h"
#include "rr_graph_fwd.h"
#include "vtr_vector.h"

/**
 * @brief Find choke points and reachable sink counts for each net sink.
 *
 * Returns empty maps when flat routing or choke point optimization is disabled.
 * Global nets always have empty maps.
 *
 * @param net_list Netlist to analyze.
 * @param router_opt_choke_points Whether choke point optimization is enabled.
 * @param is_flat Whether flat routing is enabled.
 * @return [net ID][net pin index] -> {choke point RR node ID: reachable group sink count}.
 */
vtr::vector<ParentNetId, std::vector<std::unordered_map<RRNodeId, int>>> set_nets_choking_spots(const Netlist<>& net_list,
                                                                                                bool router_opt_choke_points,
                                                                                                bool is_flat);
