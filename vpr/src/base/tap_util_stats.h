/**
 * @file
 * @brief Tap utilization statistics for routed wire segments.
 *
 * This module computes and prints per-tap fanout statistics for CHANX/CHANY
 * wires in a routed design.  A "tap" is a grid point along a wire segment,
 * numbered from 0 at the driving MUX location toward the far end of the wire.
 */

#pragma once

#include <vector>

#include "netlist.h"
#include "physical_types.h"

/**
 * @brief Prints tap utilization statistics for all routed wire segments.
 *
 * Assumes a unidirectional architecture; aborts with a warning if a BIDIR
 * wire is encountered.
 *
 * @param net_list    The netlist.
 * @param segment_inf Segment type information from the architecture.
 */
void print_tap_utilization(const Netlist<>& net_list,
                           const std::vector<t_segment_inf>& segment_inf);
