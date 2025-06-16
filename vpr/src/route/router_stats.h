#pragma once

#include "netlist_fwd.h"
#include "rr_graph_fwd.h"
#include "rr_node_types.h"
#include "vtr_assert.h"
#include "vtr_array.h"

// This struct instructs the router on how to route the given connection
struct ConnectionParameters {
    ConnectionParameters(ParentNetId net_id,
                         int target_pin_num,
                         bool router_opt_choke_points,
                         const std::unordered_map<RRNodeId, int>& connection_choking_spots)
        : net_id_(net_id)
        , target_pin_num_(target_pin_num)
        , router_opt_choke_points_(router_opt_choke_points)
        , connection_choking_spots_(connection_choking_spots) {}

    // Net id of the connection
    ParentNetId net_id_;
    // Net's pin number of the connection's SINK
    int target_pin_num_;

    // Show whether for the given connection, router should expect a choking point
    // If this is true, it would increase the routing time since the router has to
    // take some measures to solve the congestion
    bool router_opt_choke_points_;

    const std::unordered_map<RRNodeId, int>& connection_choking_spots_;
};

struct RouterStats {
    size_t connections_routed = 0;
    size_t nets_routed = 0;
    size_t heap_pushes = 0;
    size_t heap_pops = 0;
    size_t inter_cluster_node_pushes = 0;
    size_t inter_cluster_node_pops = 0;
    size_t intra_cluster_node_pushes = 0;
    size_t intra_cluster_node_pops = 0;
    vtr::array<e_rr_type, size_t, (size_t)e_rr_type::NUM_RR_TYPES> inter_cluster_node_type_cnt_pushes{0};
    vtr::array<e_rr_type, size_t, (size_t)e_rr_type::NUM_RR_TYPES> inter_cluster_node_type_cnt_pops{0};
    vtr::array<e_rr_type, size_t, (size_t)e_rr_type::NUM_RR_TYPES> intra_cluster_node_type_cnt_pushes{0};
    vtr::array<e_rr_type, size_t, (size_t)e_rr_type::NUM_RR_TYPES> intra_cluster_node_type_cnt_pops{0};

    // For debugging purposes
    vtr::array<e_rr_type, size_t, (size_t)e_rr_type::NUM_RR_TYPES> rt_node_pushes{0};

    /** Add rhs's stats to mine */
    void combine(RouterStats& rhs) {
        connections_routed += rhs.connections_routed;
        nets_routed += rhs.nets_routed;
        heap_pushes += rhs.heap_pushes;
        inter_cluster_node_pushes += rhs.inter_cluster_node_pushes;
        intra_cluster_node_pushes += rhs.intra_cluster_node_pushes;
        heap_pops += rhs.heap_pops;
        inter_cluster_node_pops += rhs.inter_cluster_node_pops;
        intra_cluster_node_pops += rhs.intra_cluster_node_pops;
        for (e_rr_type rr_type : RR_TYPES) {
            inter_cluster_node_type_cnt_pushes[rr_type] += rhs.inter_cluster_node_type_cnt_pushes[rr_type];
            inter_cluster_node_type_cnt_pops[rr_type] += rhs.inter_cluster_node_type_cnt_pops[rr_type];
            intra_cluster_node_type_cnt_pushes[rr_type] += rhs.intra_cluster_node_type_cnt_pushes[rr_type];
            intra_cluster_node_type_cnt_pops[rr_type] += rhs.intra_cluster_node_type_cnt_pops[rr_type];
            rt_node_pushes[rr_type] += rhs.rt_node_pushes[rr_type];
        }
    }
};

class WirelengthInfo {
  public:
    WirelengthInfo(size_t available = 0u, size_t used = 0u)
        : available_wirelength_(available)
        , used_wirelength_(used) {
    }

    size_t available_wirelength() const {
        return available_wirelength_;
    }

    size_t used_wirelength() const {
        return used_wirelength_;
    }

    float used_wirelength_ratio() const {
        if (available_wirelength() > 0) {
            return float(used_wirelength()) / available_wirelength();
        } else {
            VTR_ASSERT(used_wirelength() == 0);
            return 0.;
        }
    }

  private:
    size_t available_wirelength_;
    size_t used_wirelength_;
};

struct OveruseInfo {
    OveruseInfo(size_t total_nodes_ = 0u) {
        total_nodes = total_nodes_;
    }

    size_t total_nodes = 0u;
    size_t overused_nodes = 0u;
    size_t total_overuse = 0u;
    size_t worst_overuse = 0u;

    float overused_node_ratio() const {
        if (total_nodes > 0) {
            return float(overused_nodes) / total_nodes;
        } else {
            VTR_ASSERT(overused_nodes == 0);
            return 0.;
        }
    }
};
