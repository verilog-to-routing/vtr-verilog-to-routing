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
 * @brief Choke points for each sink of a net.
 *
 * Each sink's map records how many sinks in its group each IPIN can reach.
 */
struct t_net_choke_points {
    std::vector<std::unordered_map<RRNodeId, int>> by_pin; ///< Maps by net pin index; empty if the net has no choke points.

    /**
     * @brief Return choke points for pin_index, or an empty map if none exist.
     */
    const std::unordered_map<RRNodeId, int>& sink_choke_points(int pin_index) const;
};

/**
 * @brief Find choke points and reachable sink counts for each net sink.
 *
 * Returns no choke points for global nets or when either option is disabled.
 *
 * @param net_list Netlist to analyze.
 * @param router_opt_choke_points Whether choke point optimization is enabled.
 * @param is_flat Whether flat routing is enabled.
 * @return Choke points indexed by net ID.
 */
vtr::vector<ParentNetId, t_net_choke_points> set_nets_choking_spots(const Netlist<>& net_list,
                                                                    bool router_opt_choke_points,
                                                                    bool is_flat);
