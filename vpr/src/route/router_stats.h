#pragma once

#include "netlist_fwd.h"
#include "rr_graph_fwd.h"
#include "rr_node_types.h"
#include "vtr_assert.h"

// This struct instructs the router on how to route the given connection
struct ConnectionParameters {
    ConnectionParameters(ParentNetId net_id,
                         int target_pin_num,
                         bool has_choking_spot,
                         const std::unordered_map<RRNodeId, int>& connection_choking_spots)
        : net_id_(net_id)
        , target_pin_num_(target_pin_num)
        , has_choking_spot_(has_choking_spot)
        , connection_choking_spots_(connection_choking_spots) {}

    // Net id of the connection
    ParentNetId net_id_;
    // Net's pin number of the connection's SINK
    int target_pin_num_;

    // Show whether for the given connection, router should expect a choking point
    // If this is true, it would increase the routing time since the router has to
    // take some measures to solve the congestion
    bool has_choking_spot_;

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
    size_t inter_cluster_node_type_cnt_pushes[t_rr_type::NUM_RR_TYPES] = {0};
    size_t inter_cluster_node_type_cnt_pops[t_rr_type::NUM_RR_TYPES] = {0};
    size_t intra_cluster_node_type_cnt_pushes[t_rr_type::NUM_RR_TYPES] = {0};
    size_t intra_cluster_node_type_cnt_pops[t_rr_type::NUM_RR_TYPES] = {0};

    // For debugging purposes
    size_t rt_node_pushes[t_rr_type::NUM_RR_TYPES] = {0};
    size_t rt_node_high_fanout_pushes[t_rr_type::NUM_RR_TYPES] = {0};
    size_t rt_node_entire_tree_pushes[t_rr_type::NUM_RR_TYPES] = {0};

    size_t add_all_rt_from_high_fanout = 0;
    size_t add_high_fanout_rt = 0;
    size_t add_all_rt = 0;
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
