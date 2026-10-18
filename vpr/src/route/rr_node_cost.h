#pragma once

/// @file Per rr node routing state and the delay and
/// congestion cost of a single rr node.

#include "globals.h"
#include "rr_graph_fwd.h"
#include "rr_graph_view.h"
#include "rr_switch.h"
#include "vtr_assert.h"

/// @brief Extra information about each rr_node needed only during routing
/// (i.e. during the maze expansion).
struct t_rr_node_route_inf {
    /// ID of the edge (globally unique edge ID in the RR Graph)
    /// that was used to reach this node from the previous node.
    /// If there is no predecessor, prev_edge = NO_PREVIOUS.
    RREdgeId prev_edge;

    /// Accumulated cost term from previous Pathfinder iterations.
    float acc_cost;

    /// Total cost of the path up to and including this node +
    /// the expected cost to the target if the timing_driven router
    /// is being used.
    float path_cost;

    /// Total cost of the path up to and including this node.
    float backward_path_cost;

    /// Upstream resistance to ground from this node in the current
    /// path search (connection routing), including the resistance
    /// of the node itself (device_ctx.rr_nodes[index].R).
    float R_upstream;

  public: // Accessors
    /// @return The current occupancy of the associated rr node.
    short occ() const { return occ_; }

    /// @return True when the associated rr node belongs to a non-configurable node set.
    bool in_non_config_node_set() const { return in_non_config_node_set_; }

  public: // Mutators
    /// @param new_occ The new occupancy to set.
    void set_occ(int new_occ) { occ_ = new_occ; }

    /// @param value Whether the associated rr node belongs to a non-configurable
    /// node set.
    void set_in_non_config_node_set(bool value) { in_non_config_node_set_ = value; }

  private: // Data
    /// The current occupancy of the associated rr node.
    short occ_ = 0;

    /// Whether the associated rr node belongs to a non-configurable node set.
    /// Set once when the routing structures are allocated and fixed for the rest
    /// of routing.
    bool in_non_config_node_set_ = false;
};

/// @brief The fields of a t_rr_switch_inf that the neighbour cost evaluation reads.
/// A t_rr_switch_inf holds a std::string and spans two cache lines. This copy holds only
/// the per-switch values read for every expanded edge and the two switch type tests.
struct t_rr_switch_cost_inf {
    t_rr_switch_cost_inf() = default;

    explicit t_rr_switch_cost_inf(const t_rr_switch_inf& switch_inf)
        : R(switch_inf.R)
        , Cinternal(switch_inf.Cinternal)
        , Tdel(switch_inf.Tdel)
        , buffered(switch_inf.buffered())
        , configurable(switch_inf.configurable()) {}

    /// Equivalent resistance of the switch.
    float R = 0.;
    /// Internal capacitance of the switch.
    float Cinternal = 0.;
    /// Intrinsic delay of the switch.
    float Tdel = 0.;
    /// True when the switch isolates its input from its output.
    bool buffered = false;
    /// True when the switch can be turned on or off.
    bool configurable = true;
};

/// @brief Delay of an rr node driven through the given switch, ignoring upstream
/// and downstream effects. Only half of the node's own resistance is charged
/// against its capacitance.
inline float compute_rr_node_delay(const t_rr_switch_cost_inf& switch_inf,
                                   t_rr_rc_data node_rc) {
    float R_node_only = switch_inf.R + node_rc.R;
    float Rdel = R_node_only - 0.5 * node_rc.R;
    return switch_inf.Tdel + Rdel * node_rc.C;
}

/// @brief Delay of a single rr node when driven through connecting_edge.
/// Ignores all upstream and downstream effects.
inline float get_rr_node_delay_cost(RRNodeId to_node, RREdgeId connecting_edge) {
    const RRGraphView& rr_graph = g_vpr_ctx.device().rr_graph;
    const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switch_inf = rr_graph.rr_switch();

    // Info for the switch connecting from_node to_node (i.e., to->index)
    RRSwitchId iswitch = (RRSwitchId)rr_graph.edge_switch(connecting_edge);
    t_rr_switch_cost_inf switch_inf(rr_switch_inf[iswitch]);

    // To node info
    t_rr_rc_data node_rc(rr_graph.node_R(to_node), rr_graph.node_C(to_node));

    return compute_rr_node_delay(switch_inf, node_rc);
}

/// @brief Returns the base cost of using this rr_node.
inline float get_single_rr_cong_base_cost(RRNodeId inode) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    RRIndexedDataId cost_index = device_ctx.rr_graph.node_cost_index(inode);

    return device_ctx.rr_indexed_data[cost_index].base_cost;
}

/// @brief Returns the accumulated congestion cost of using this rr_node.
inline float get_single_rr_cong_acc_cost(RRNodeId inode) {
    const RoutingContext& route_ctx = g_vpr_ctx.routing();

    return route_ctx.rr_node_route_inf[inode].acc_cost;
}

/// @brief Returns the present congestion cost of using this rr_node.
inline float get_single_rr_cong_pres_cost(RRNodeId inode, float pres_fac) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;
    const RoutingContext& route_ctx = g_vpr_ctx.routing();

    int occ = route_ctx.rr_node_route_inf[inode].occ();
    int capacity = rr_graph.node_capacity(inode);

    if (occ >= capacity) {
        return (1. + pres_fac * (occ + 1 - capacity));
    } else {
        return 1.;
    }
}

/// @brief Returns the congestion cost of using this rr_node,
/// *ignoring* non-configurable edges.
inline float get_single_rr_cong_cost(RRNodeId inode, float pres_fac) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;
    const RoutingContext& route_ctx = g_vpr_ctx.routing();

    float pres_cost;
    int occ = route_ctx.rr_node_route_inf[inode].occ();
    int overuse = occ - rr_graph.node_capacity(inode);

    if (overuse >= 0) {
        pres_cost = (1. + pres_fac * (overuse + 1));
    } else {
        pres_cost = 1.;
    }

    RRIndexedDataId cost_index = rr_graph.node_cost_index(inode);

    float base_cost = device_ctx.rr_indexed_data[cost_index].base_cost;
    float acc_cost = route_ctx.rr_node_route_inf[inode].acc_cost;
    float cost = base_cost * acc_cost * pres_cost;

    VTR_ASSERT_DEBUG_MSG(cost == get_single_rr_cong_base_cost(inode)
                                     * get_single_rr_cong_acc_cost(inode)
                                     * get_single_rr_cong_pres_cost(inode, pres_fac),
                         "Single rr node congestion cost is inaccurate");

    return cost;
}

/// @brief Congestion cost of using this rr node plus that of any non-configurably
/// connected rr nodes that must be used when it is used.
inline float get_rr_cong_cost(RRNodeId inode, float pres_fac) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RoutingContext& route_ctx = g_vpr_ctx.routing();

    float cost = get_single_rr_cong_cost(inode, pres_fac);

    if (route_ctx.rr_node_route_inf[inode].in_non_config_node_set()) {
        // Access unordered_map only when the node is part of a non-configurable set
        auto itr = device_ctx.rr_node_to_non_config_node_set.find(inode);
        if (itr != device_ctx.rr_node_to_non_config_node_set.end()) {
            for (RRNodeId node : device_ctx.rr_non_config_node_sets[itr->second]) {
                if (node == inode) {
                    continue; // Already included above
                }

                cost += get_single_rr_cong_cost(node, pres_fac);
            }
        }
    }
    return (cost);
}
