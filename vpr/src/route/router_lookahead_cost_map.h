#ifndef ROUTER_LOOKAHEAD_COST_MAP_H_
#define ROUTER_LOOKAHEAD_COST_MAP_H_

#include <vector>
#include "router_lookahead_map_utils.h"
#include "vtr_geometry.h"

// Dense cost maps per source segment and destination chan types
class CostMap {
  public:
    void set_counts(size_t seg_count);
    void build_segment_map();
    int node_to_segment(int from_node_ind) const;
    util::Cost_Entry find_cost(int from_seg_index, int delta_x, int delta_y) const;
    void set_cost_map(const util::RoutingCosts& delay_costs, const util::RoutingCosts& base_costs);
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
};

#endif
