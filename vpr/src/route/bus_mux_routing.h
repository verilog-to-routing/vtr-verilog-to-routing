#pragma once
/**
 * @file
 * @brief Flat-router support for bus-based muxes (<mux bus="true">).
 *
 * The router keeps, per bus mux, how many routed bits use each input set (see
 * bus_mux_route_types.h). These helpers maintain those counts as nets are ripped up and
 * committed, turn them into the control congestion cost added to a bus-mux edge, and
 * report which muxes still use more than one input set.
 */
#include <cstddef>

#include "bus_mux_route_types.h"
#include "route_tree_fwd.h"
#include "rr_graph_fwd.h"
#include "vtr_vector.h"

/// @brief Give every bus mux in rr_bus_muxes a zeroed per-set bit count in bus_mux_route_inf.
void reset_bus_mux_route_inf(const vtr::vector<BusMuxId, t_rr_bus_mux>& rr_bus_muxes,
                             vtr::vector<BusMuxId, t_bus_mux_route_inf>& bus_mux_route_inf);

/**
 * @brief Return the control congestion cost of the edge from_node -> to_node.
 *
 * Non-zero only when the edge implements one bit of a bus-based mux: each routed bit of
 * that mux on another input set counts as one unit of overuse on to_node.
 */
float get_bus_mux_cong_cost(RRNodeId from_node, RRNodeId to_node, float pres_fac);

/**
 * @brief Add add_or_sub (+1 or -1) to the bit count of the input set used by the edge into rt_node.
 *
 * No effect if rt_node has no parent or the edge is not part of a bus mux.
 */
void pathfinder_update_bus_mux_occupancy(vtr::vector<BusMuxId, t_bus_mux_route_inf>& bus_mux_route_inf,
                                         const RouteTreeNode& rt_node,
                                         int add_or_sub);

/// @brief Whether the edge into rt_node belongs to a bus mux using more than one input set.
bool is_bus_mux_edge_control_congested(const RouteTreeNode& rt_node);

/// @brief Count bus muxes using more than one input set.
size_t count_control_congested_bus_muxes(const vtr::vector<BusMuxId, t_bus_mux_route_inf>& bus_mux_route_inf);
