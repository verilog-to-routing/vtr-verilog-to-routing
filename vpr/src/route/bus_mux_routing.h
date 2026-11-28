#pragma once
/**
 * @file
 * @brief Flat-router support for bus-based muxes (<mux bus="true">).
 *
 * These helpers maintain per-set and per-edge occupancy, compute control congestion
 * costs, update edge history costs, and identify muxes using multiple input sets.
 */
#include <cstddef>

#include "bus_mux_route_types.h"
#include "route_tree_fwd.h"
#include "rr_graph_fwd.h"
#include "vtr_vector.h"

/// @brief Reset each bus mux's occupancy to zero and edge history costs to 1.
void reset_bus_mux_route_inf(const vtr::vector<BusMuxId, t_rr_bus_mux>& rr_bus_muxes,
                             vtr::vector<BusMuxId, t_bus_mux_route_inf>& bus_mux_route_inf);

/**
 * @brief Return the control congestion cost of the edge from_node -> to_node.
 *
 * For a bus-mux edge, scale the number of routed bits on other input sets by pres_fac,
 * to_node's base and accumulated costs, and the edge's history cost. Return zero for
 * other edges.
 */
float get_bus_mux_cong_cost(RRNodeId from_node, RRNodeId to_node, float pres_fac);

/**
 * @brief Add add_or_sub (+1 or -1) to the use counts of the bus-mux edge into rt_node and of its input set.
 *
 * No effect if rt_node has no parent or the edge is not part of a bus mux.
 */
void pathfinder_update_bus_mux_occupancy(vtr::vector<BusMuxId, t_bus_mux_route_inf>& bus_mux_route_inf,
                                         const RouteTreeNode& rt_node,
                                         int add_or_sub);

/**
 * @brief Update the history cost of the bus-mux edges after a routing iteration.
 *
 * Add acc_fac to each occupied edge's history cost in muxes using multiple input sets.
 * For muxes using at most one input set, halve every edge's history cost above 1.
 */
void pathfinder_update_bus_mux_acc_cost(vtr::vector<BusMuxId, t_bus_mux_route_inf>& bus_mux_route_inf,
                                        float acc_fac);

/// @brief Whether the edge into rt_node belongs to a bus mux using more than one input set.
bool is_bus_mux_edge_control_congested(const RouteTreeNode& rt_node);

/// @brief Count bus muxes using more than one input set.
size_t count_control_congested_bus_muxes(const vtr::vector<BusMuxId, t_bus_mux_route_inf>& bus_mux_route_inf);
