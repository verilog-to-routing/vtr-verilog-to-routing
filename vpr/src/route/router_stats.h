#pragma once

struct RouterStats {
    size_t connections_routed = 0;
    size_t nets_routed = 0;
    size_t heap_pushes = 0;
    size_t heap_pops = 0;
    size_t inter_cluster_node_pushes = 0;
    size_t inter_cluster_node_pops = 0;
    size_t intra_cluster_node_pushes = 0;
    size_t intra_cluster_node_pops = 0;
    size_t inter_cluster_node_type_cnt_pushes[t_rr_type::NUM_RR_TYPES];
    size_t inter_cluster_node_type_cnt_pops[t_rr_type::NUM_RR_TYPES];
    size_t intra_cluster_node_type_cnt_pushes[t_rr_type::NUM_RR_TYPES];
    size_t intra_cluster_node_type_cnt_pops[t_rr_type::NUM_RR_TYPES];

    // For debugging purposes
    size_t rt_node_pushes[t_rr_type::NUM_RR_TYPES];
    size_t rt_node_high_fanout_pushes[t_rr_type::NUM_RR_TYPES];
    size_t rt_node_entire_tree_pushes[t_rr_type::NUM_RR_TYPES];

    size_t add_all_rt_from_high_fanout;
    size_t add_high_fanout_rt;
    size_t add_all_rt;

    ParentNetId net_id;
    int target_pin_num;
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
