#pragma once
/**
 * @file
 * @brief Bus-based muxes (<mux bus="true">) and their routing state.
 *
 * All bits share one select and must use the same input set. The pb graph
 * represents each bit separately, so the flat router tracks this constraint.
 *
 * A mux using multiple input sets has "control congestion". An edge's cost
 * grows with the number of routed bits using other input sets.
 *
 * The cost has no history term; sets with fewer routed bits cost more to use.
 */
#include <string>
#include <vector>

#include "clustered_netlist_fwd.h"
#include "physical_types.h"
#include "rr_graph_fwd.h"
#include "vtr_util.h"

/** @brief Identifies a bus mux by its architecture tag and owning pb instance. */
struct t_bus_mux_key {
    const t_interconnect* interconnect = nullptr; ///< Architecture mux definition.
    const t_pb_graph_node* owner = nullptr;       ///< pb instance containing the mux.

    bool operator==(const t_bus_mux_key& other) const {
        return interconnect == other.interconnect && owner == other.owner;
    }
};

/** @brief A bus mux instance in the routing resource graph. */
struct t_rr_bus_mux {
    ClusterBlockId cluster;                       ///< Cluster containing the mux, used in messages.
    const t_interconnect* interconnect = nullptr; ///< Architecture mux definition.
    const t_pb_graph_node* owner = nullptr;       ///< pb instance whose mode contains the mux.
    int num_sets = 0;                             ///< Number of input sets (data lines).

    /** @brief Describe the mux and its owner, e.g. "'a2a' in one_mult_27x27[0]". */
    std::string describe() const {
        return vtr::string_fmt("'%s' in %s[%d]",
                               interconnect->name.c_str(),
                               owner->pb_type->name,
                               owner->placement_index);
    }
};

/**
 * @brief Mux and input set identified by find_bus_mux_edge().
 */
struct t_bus_mux_edge {
    int mux_idx; ///< Index into DeviceContext::rr_bus_muxes.
    int set;     ///< Input set (data line) selected by the edge.
};

/** @brief An incoming RR edge for one bus mux output bit. */
struct t_rr_bus_mux_in_edge {
    RRNodeId from_node; ///< Node driving the output bit.
    int set;            ///< Input set selected by this edge.
};

/** @brief Incoming edges for one bus mux output bit. */
struct t_rr_bus_mux_out_node {
    int mux_idx = -1;                           ///< Index into DeviceContext::rr_bus_muxes.
    std::vector<t_rr_bus_mux_in_edge> in_edges; ///< One edge per input set present in the RR graph.
};

/** @brief Routing state of one bus mux instance. */
struct t_bus_mux_route_inf {
    std::vector<int> set_occ; ///< Routed bit counts indexed by input set [0..num_sets-1].

    /** @brief Count input sets driving at least one bit. */
    int num_sets_in_use() const {
        int num_in_use = 0;
        for (int occ : set_occ) {
            if (occ > 0) {
                num_in_use++;
            }
        }
        return num_in_use;
    }

    /** @brief Whether the mux uses more than one input set. */
    bool is_control_congested() const {
        return num_sets_in_use() > 1;
    }

    /** @brief Count bits that must change inputs if the mux selects set. */
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
