#pragma once
/**
 * @file
 * @brief Bus-based muxes (<mux bus="true">) and their routing state.
 *
 * All bits of a bus mux share one select, so they must all use the same input
 * set. The pb graph lowers the mux into independent single-pin edges, so the
 * flat router needs extra state to preserve this constraint.
 *
 * Similar to Pathfinder congestion, a bit using a different input set from the
 * other routed bits is treated as "control congested". Its present cost grows
 * with the number of bits it would move onto another input set.
 *
 * No history term is needed because the cost is tracked per input set, which
 * already makes the minority choice more expensive.
 */
#include <unordered_map>
#include <vector>

#include "clustered_netlist_fwd.h"
#include "rr_graph_fwd.h"

struct t_interconnect;
class t_pb_graph_node;

/// @brief Identity of one bus-based mux instance: its <mux> tag and the pb instance that owns it.
struct t_bus_mux_key {
    const t_interconnect* interconnect = nullptr;
    const t_pb_graph_node* owner = nullptr;

    bool operator==(const t_bus_mux_key& other) const {
        return interconnect == other.interconnect && owner == other.owner;
    }
};

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
    /// @brief Index into DeviceContext::rr_bus_muxes.
    int mux_idx = -1;
    /// @brief The edges driving this output bit, one per input set present in the rr graph.
    std::vector<t_rr_bus_mux_in_edge> in_edges;
};
