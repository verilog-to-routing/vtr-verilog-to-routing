#pragma once
/**
 * @file
 * @brief Bus-based muxes (<mux bus="true">) in the routing resource graph and
 *        their state during routing.
 *
 * All bits of a bus-based mux share one select, so every bit routed through the
 * mux must be driven from the same input set (data line). The pb graph lowers
 * such a mux to one single-pin edge per (input set, bit), and the flat router
 * sees them as ordinary rr edges. The router keeps the bits together the way
 * pathfinder resolves node congestion: a bit driven from an input set that the
 * other routed bits of the mux do not use is "control congested", and its
 * present cost grows with the number of bits it would force onto another set.
 *
 * Negotiating a shared mux select as a second kind of congestion comes from
 * Friedman et al., "SPR: An Architecture-Adaptive CGRA Mapping Tool", FPGA 2009,
 * section 7.1. Their setting is time-multiplexed, so virtual per-phase copies of
 * one mux contend for a single output wire and they need two congestion types on
 * that one resource. Here every mux is static and the bits are distinct wires
 * with their own capacity, so ordinary node congestion already covers everything
 * except agreement on the select.
 *
 * There is no history term. SPR needs one because their present control cost is a
 * mux-level scalar, identical for every input, so nothing tells a signal which
 * input to prefer and the symmetry has to be broken by history. The cost here is
 * per input set and already makes the minority set the expensive one to sit on.
 * A history term added later would have to penalize the set in use as well as the
 * alternatives, or it becomes a ratchet that locks in whichever set leads early.
 */
#include <unordered_map>
#include <vector>

#include "clustered_netlist_fwd.h"
#include "rr_graph_fwd.h"

struct t_interconnect;
class t_pb_graph_node;

/// @brief One bus-based mux instance in the routing resource graph (one per mux per cluster).
struct t_rr_bus_mux {
    /// @brief Cluster the mux belongs to (for messages).
    ClusterBlockId cluster;
    /// @brief The <mux> tag of the architecture.
    const t_interconnect* interconnect = nullptr;
    /// @brief pb_graph_node whose mode contains the mux.
    const t_pb_graph_node* owner = nullptr;
    /// @brief Number of input sets (data lines).
    int num_sets = 0;
};

/// @brief An rr edge implementing one bit of a bus-based mux, seen from the mux output node.
struct t_rr_bus_mux_in_edge {
    RRNodeId from_node;
    int set;
};

/// @brief The bus-mux edges ending at one output bit of a bus-based mux.
struct t_rr_bus_mux_out_node {
    /// @brief Index into RoutingContext::rr_bus_muxes.
    int mux_idx = -1;
    /// @brief The edges driving this output bit, one per input set present in the rr graph.
    std::vector<t_rr_bus_mux_in_edge> in_edges;
};
