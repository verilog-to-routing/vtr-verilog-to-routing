#ifndef CONNECTION_BOX_LOOKAHEAD_H_
#define CONNECTION_BOX_LOOKAHEAD_H_

#include <vector>
#include <list>
#include "physical_types.h"
#include "router_lookahead.h"
#include "router_lookahead_map_utils.h"
#include "connection_box.h"
#include "vtr_geometry.h"

struct RoutingCostKey {
    vtr::Point<int> delta;
    ConnectionBoxId box_id;
    bool operator==(const RoutingCostKey& other) const {
        return delta == other.delta && box_id == other.box_id;
    }
};
struct RoutingCost {
    int from_node, to_node;
    util::Cost_Entry cost_entry;
};

// specialization of std::hash for RoutingCostKey
namespace std {
template<>
struct hash<RoutingCostKey> {
    typedef RoutingCostKey argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& s) const noexcept {
        result_type const h1(std::hash<int>{}(s.delta.x()));
        result_type const h2(std::hash<int>{}(s.delta.y()));
        result_type const h3(std::hash<size_t>{}(size_t(s.box_id)));
        return h1 ^ ((h2 ^ (h3 << 1)) << 1);
    }
};
} // namespace std

typedef std::unordered_map<RoutingCostKey, RoutingCost> RoutingCosts;

class CostMap {
  public:
    void set_counts(size_t seg_count, size_t box_count);
    int node_to_segment(int from_node_ind) const;
    util::Cost_Entry find_cost(int from_seg_index, ConnectionBoxId box_id, int delta_x, int delta_y) const;
    void set_cost_map(int from_seg_index,
                      const RoutingCosts& costs,
                      util::e_representative_entry_method method);
    void set_cost_map(int from_seg_index, ConnectionBoxId box_id, const RoutingCosts& costs, util::e_representative_entry_method method);
    util::Cost_Entry get_nearby_cost_entry(const vtr::NdMatrix<util::Cost_Entry, 2>& matrix, int cx, int cy, const vtr::Rect<int>& bounds);
    void read(const std::string& file);
    void write(const std::string& file) const;
    void print(int iseg) const;
    std::list<std::pair<int, int>> list_empty() const;

  private:
    vtr::NdMatrix<vtr::NdMatrix<util::Cost_Entry, 2>, 2> cost_map_;
    vtr::NdMatrix<std::pair<int, int>, 2> offset_;
    std::vector<int> segment_map_;
};

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
