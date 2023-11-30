//
// Created by amin on 11/27/23.
//

#ifndef VTR_ROUTER_LOOKAHEAD_SPARSE_MAP_H
#define VTR_ROUTER_LOOKAHEAD_SPARSE_MAP_H

#include <string>
#include <limits>
#include "vtr_ndmatrix.h"
#include "router_lookahead.h"
#include "router_lookahead_map_utils.h"

class CompressedMapLookahead : public RouterLookahead {
  public:
    explicit CompressedMapLookahead(const t_det_routing_arch& det_routing_arch, bool is_flat);

  private:
    //Look-up table from SOURCE/OPIN to CHANX/CHANY of various types
    util::t_src_opin_delays src_opin_delays;
    // Lookup table to store the minimum cost for each dx and dy
    vtr::NdMatrix<util::Cost_Entry, 3> distance_based_min_cost; // [layer_num][dx][dy] -> cost

    const t_det_routing_arch& det_routing_arch_;
    bool is_flat_;

  protected:
    float get_expected_cost(RRNodeId node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const override;

    std::pair<float, float> get_expected_delay_and_cong(RRNodeId from_node,
                                                        RRNodeId to_node,
                                                        const t_conn_cost_params& params,
                                                        float R_upstream) const override;

    void compute(const std::vector<t_segment_inf>& segment_inf) override;

    void compute_intra_tile() override {
        VPR_THROW(VPR_ERROR_ROUTE, "CompressedMapLookahead::compute_intra_time unimplemented");
    }

    void read(const std::string& /*file*/) override;

    void read_intra_cluster(const std::string& /*file*/) {
        VPR_THROW(VPR_ERROR_ROUTE, "CompressedMapLookahead::read_intra_cluster unimplemented");
    }

    void write(const std::string& /*file*/) const override {
        VPR_THROW(VPR_ERROR_ROUTE, "CompressedMapLookahead::write unimplemented");
    }

    void write_intra_cluster(const std::string& /*file*/) const override {
        VPR_THROW(VPR_ERROR_ROUTE, "CompressedMapLookahead::write_intra_cluster unimplemented");
    }
};

class Cost_Entry {
  public:
    float delay;
    float congestion;

    Cost_Entry()
        : Cost_Entry(std::numeric_limits<float>::quiet_NaN(),
                     std::numeric_limits<float>::quiet_NaN()) {
    }

    Cost_Entry(float set_delay, float set_congestion) {
        delay = set_delay;
        congestion = set_congestion;
    }
};

#endif //VTR_ROUTER_LOOKAHEAD_SPARSE_MAP_H
