#ifndef EXTENDED_MAP_LOOKAHEAD_H_
#define EXTENDED_MAP_LOOKAHEAD_H_

#include <vector>
#include "physical_types.h"
#include "router_lookahead.h"
#include "router_lookahead_map_utils.h"
#include "router_lookahead_cost_map.h"
#include "vtr_geometry.h"

// Implementation of RouterLookahead based on source segment and destination connection box types
class ExtendedMapLookahead : public RouterLookahead {
  private:
    //Look-up table from SOURCE/OPIN to CHANX/CHANY of various types
    util::t_src_opin_delays src_opin_delays;

    //Look-up table from CHANX/CHANY to SINK/IPIN of various types
    util::t_chan_ipins_delays chan_ipins_delays;

    std::pair<float, float> get_src_opin_delays(RRNodeId from_node, int delta_x, int delta_y, float criticality_fac) const;
    float get_chan_ipin_delays(RRNodeId to_node) const;

    template<typename Entry>
    bool add_paths(int start_node_ind,
                   Entry current,
                   const std::vector<util::Search_Path>& paths,
                   util::RoutingCosts* routing_costs);

    template<typename Entry>
    std::pair<float, int> run_dijkstra(int start_node_ind,
                                       std::vector<bool>* node_expanded,
                                       std::vector<util::Search_Path>* paths,
                                       util::RoutingCosts* routing_costs);

  public:
    float get_expected_cost(int node, int target_node, const t_conn_cost_params& params, float R_upstream) const override;
    float get_map_cost(int from_node_ind, int to_node_ind, float criticality_fac) const;
    void compute(const std::vector<t_segment_inf>& segment_inf) override;

    void read(const std::string& file) override;
    void write(const std::string& file) const override;

    CostMap cost_map_;
};

#endif
