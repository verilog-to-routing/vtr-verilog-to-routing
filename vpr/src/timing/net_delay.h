#pragma once

#include "netlist.h"
#include "vpr_net_pins_matrix.h"

/**
 * @brief Loads the max routing delay of every net connection from the route trees.
 *
 * This is the delay the router uses (RouteTreeNode::Tdel). net_delay must already be allocated.
 */
void load_net_delay_from_routing(const Netlist<>& net_list, NetPinsMatrix<float>& net_delay);

/**
 * @brief Loads the min and max routing delay of every net connection from the route trees.
 *
 * The max delay is the one the router computed (RouteTreeNode::Tdel). The min delay is
 * computed by reloading the timing of a copy of each route tree with DelayType::MIN (see
 * RouteTree::reload_timing()), so it uses the same delay calculation as the router.
 * Both matrices must already be allocated.
 */
void load_net_delay_from_routing(const Netlist<>& net_list,
                                 NetPinsMatrix<float>& net_delay_min,
                                 NetPinsMatrix<float>& net_delay_max);
