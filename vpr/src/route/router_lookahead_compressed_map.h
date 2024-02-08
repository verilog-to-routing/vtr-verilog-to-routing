//
// Created by amin on 11/27/23.
//

#ifndef VTR_ROUTER_LOOKAHEAD_COMPRESSED_MAP_H
#define VTR_ROUTER_LOOKAHEAD_COMPRESSED_MAP_H

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

    void read(const std::string& /*file*/) override {
        VPR_THROW(VPR_ERROR_ROUTE, "CompressedMapLookahead::read unimplemented");
    }

    void read_intra_cluster(const std::string& /*file*/) {
        VPR_THROW(VPR_ERROR_ROUTE, "CompressedMapLookahead::read_intra_cluster unimplemented");
    }

    void write(const std::string& file_name) const override;

    void write_intra_cluster(const std::string& /*file*/) const override {
        VPR_THROW(VPR_ERROR_ROUTE, "CompressedMapLookahead::write_intra_cluster unimplemented");
    }

    float get_opin_distance_min_delay(int /*physical_tile_idx*/, int /*from_layer*/, int /*to_layer*/, int /*dx*/, int /*dy*/) const override {
        return -1.;
    }
};

// This is a 5D array that stores estimates of the cost to reach a location at a particular distance away from the current location.
// The router look-ahead is built under the assumption of translation-invariance, meaning the current location (in terms of x-y coordinates) is not crucial.
// The indices of this array are as follows:
//   from_layer: The layer number that the node under consideration is on.
//   Chan type: The type of channel (x/y) that the node under consideration belongs to.
//   Seg type: The type of segment (listed under "segmentlist" tag in the architecture file) that the node under consideration belongs to.
//   to_layer: The layer number that the target node is on.
//   compressed index: In this type of router look-ahead, we do not sample every x and y. Another data structure maps every x and y to
//   an index. That index should be used here.

typedef vtr::NdMatrix<util::Cost_Entry, 5> t_compressed_wire_cost_map; //[0..num_layers][0..1][[0..num_seg_types-1][0..num_layers][compressed_idx]
                                                                       //[0..1] entry distinguish between CHANX/CHANY start nodes respectively
                                                                       // The first index is the layer number that the node under consideration is on, and the forth index
                                                                       // is the layer number that the target node is on.

#endif //VTR_ROUTER_LOOKAHEAD_COMPRESSED_MAP_H
