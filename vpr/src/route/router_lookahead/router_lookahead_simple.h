#pragma once

#include "vtr_ndmatrix.h"
#include "router_lookahead.h"
#include "router_lookahead_map_utils.h"

/**
 * @brief A lookahead that can only read a cost map from a file and query it based on distance.
 *
 * SimpleLookahead cannot compute a cost map, instead it can only read a single cost map from an external file.
 * This cost map is similar to that of MapLookahead, and it only refers to channel segments. There is no additional
 * cost map for sources/opins or intra-cluster estimates.
 */
class SimpleLookahead : public RouterLookahead {
  protected:
    float get_expected_cost(RRNodeId node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const override;
    std::pair<float, float> get_expected_delay_and_cong(RRNodeId from_node, RRNodeId to_node, const t_conn_cost_params& params, float R_upstream) const override;

    void compute(const std::vector<t_segment_inf>& /*segment_inf*/) override {
        VPR_THROW(VPR_ERROR_ROUTE, "SimpleLookahead::compute unsupported, try to load lookahead from a file.");
    }

    void compute_intra_tile() override {
        VPR_THROW(VPR_ERROR_ROUTE, "SimpleLookahead::compute_intra_tile unimplemented");
    }

    void read(const std::string& file) override;

    void read_intra_cluster(const std::string& /*file*/) override {
        VPR_THROW(VPR_ERROR_ROUTE, "SimpleLookahead::read_intra_cluster unimplemented");
    }

    void write(const std::string& file) const override;

    void write_intra_cluster(const std::string& /*file*/) const override {
        VPR_THROW(VPR_ERROR_ROUTE, "SimpleLookahead::write_intra_cluster unimplemented");
    }

    float get_opin_distance_min_delay(int /*physical_tile_idx*/, int /*from_layer*/, int /*to_layer*/, int /*dx*/, int /*dy*/) const override {
        return -1;
    }
};

/* provides delay/congestion estimates to travel specified distances
 * in the x/y direction */
// This is a 6D array storing the cost to travel from a node of type CHANX/CHANY to a point that is dx, dy further, and is on the "layer_num" layer.
// To store this information, the first index is the layer number that the node under consideration is on, the second index is the layer number of the target node, the third index represents the type of channel (X/Y)
// that the node under consideration belongs to, the forth is the segment type (specified in the architecture file under the "segmentlist" tag), the fourth is the
// target "layer_num" mentioned above, the fifth is dx, and the last one is dy.
typedef vtr::NdMatrix<util::Cost_Entry, 6> t_simple_cost_map; //[0..num_layers][0..num_layers][0..1][[0..num_seg_types-1][0..device_ctx.grid.width()-1][0..device_ctx.grid.height()-1]
                                                              //[0..1] entry distinguish between CHANX/CHANY start nodes respectively
                                                              // The first index is the layer number that the node under consideration is on, and the second index
                                                              // is the layer number that the target node is on.
