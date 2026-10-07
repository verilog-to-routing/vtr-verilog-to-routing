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
#include <string>
#include <vector>

#include "clustered_netlist_fwd.h"
#include "physical_types.h"
#include "rr_graph_fwd.h"
#include "vtr_util.h"

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

    /// @brief Describe the mux and its owner, e.g. "'a2a' in one_mult_27x27[0]".
    std::string describe() const {
        return vtr::string_fmt("'%s' in %s[%d]",
                               interconnect->name.c_str(),
                               owner->pb_type->name,
                               owner->placement_index);
    }
};

/// @brief The mux and input set an rr edge belongs to, see find_bus_mux_edge().
struct t_bus_mux_edge {
    /// @brief Index into DeviceContext::rr_bus_muxes.
    int mux_idx;
    /// @brief Input set (data line) selected by the edge.
    int set;
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

/// @brief Routing state of one bus-based mux instance.
struct t_bus_mux_route_inf {
    /// @brief Routed bit counts indexed by input set [0..num_sets-1].
    std::vector<int> set_occ;

    /// @brief Count input sets driving at least one bit.
    int num_sets_in_use() const {
        int num_in_use = 0;
        for (int occ : set_occ) {
            if (occ > 0) {
                num_in_use++;
            }
        }
        return num_in_use;
    }

    /// @brief Whether the mux uses more than one input set.
    bool is_control_congested() const {
        return num_sets_in_use() > 1;
    }

    /// @brief Count bits that must change inputs if the mux selects set.
    int bits_on_other_sets(int set) const {
        int num_bits = 0;
        for (size_t other_set = 0; other_set < set_occ.size(); other_set++) {
            if ((int)other_set != set) {
                num_bits += set_occ[other_set];
            }
        }
        return num_bits;
    }
};
