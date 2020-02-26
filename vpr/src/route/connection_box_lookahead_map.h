#ifndef CONNECTION_BOX_LOOKAHEAD_H_
#define CONNECTION_BOX_LOOKAHEAD_H_

#include <vector>
#include "physical_types.h"
#include "router_lookahead.h"
#include "router_lookahead_map_utils.h"
#include "connection_box.h"
#include "vtr_geometry.h"

// Keys in the RoutingCosts map
struct RoutingCostKey {
    // segment type index
    int seg_index;

    // type of the destination connection box
    ConnectionBoxId box_id;

    // offset of the destination connection box from the starting segment
    vtr::Point<int> delta;

    RoutingCostKey()
        : seg_index(-1)
        , delta(0, 0) {}
    RoutingCostKey(int seg_index_arg, ConnectionBoxId box_id_arg, vtr::Point<int> delta_arg)
        : seg_index(seg_index_arg)
        , box_id(box_id_arg)
        , delta(delta_arg) {}

    bool operator==(const RoutingCostKey& other) const {
        return seg_index == other.seg_index && box_id == other.box_id && delta == other.delta;
    }
};

// hash implementation for RoutingCostKey
struct HashRoutingCostKey {
    std::size_t operator()(RoutingCostKey const& key) const noexcept {
        std::size_t hash = std::hash<int>{}(key.seg_index);
        vtr::hash_combine(hash, key.box_id);
        vtr::hash_combine(hash, key.delta.x());
        vtr::hash_combine(hash, key.delta.y());
        return hash;
    }
};

// Map used to store intermediate routing costs
typedef std::unordered_map<RoutingCostKey, float, HashRoutingCostKey> RoutingCosts;

// Dense cost maps per source segment and destination connection box types
class CostMap {
  public:
    void set_counts(size_t seg_count, size_t box_count);
    void build_segment_map();
    int node_to_segment(int from_node_ind) const;
    util::Cost_Entry find_cost(int from_seg_index, ConnectionBoxId box_id, int delta_x, int delta_y) const;
    void set_cost_map(const RoutingCosts& delay_costs, const RoutingCosts& base_costs);
    std::pair<util::Cost_Entry, int> get_nearby_cost_entry(const vtr::NdMatrix<util::Cost_Entry, 2>& matrix, int cx, int cy, const vtr::Rect<int>& bounds);
    void read(const std::string& file);
    void write(const std::string& file) const;
    void print(int iseg) const;
    std::vector<std::pair<int, int>> list_empty() const;

  private:
    vtr::Matrix<vtr::Matrix<util::Cost_Entry>> cost_map_;
    vtr::Matrix<std::pair<int, int>> offset_;
    vtr::Matrix<float> penalty_;
    std::vector<int> segment_map_;
    size_t seg_count_;
    size_t box_count_;
};

// Implementation of RouterLookahead based on source segment and destination connection box types
class ConnectionBoxMapLookahead : public RouterLookahead {
  public:
    float get_expected_cost(int node, int target_node, const t_conn_cost_params& params, float R_upstream) const override;
    float get_map_cost(int from_node_ind, int to_node_ind, float criticality_fac) const;
    void compute(const std::vector<t_segment_inf>& segment_inf) override;

    void read(const std::string& file) override;
    void write(const std::string& file) const override;

    CostMap cost_map_;
};

#endif
