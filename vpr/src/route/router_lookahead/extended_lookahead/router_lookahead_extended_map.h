#pragma once

#include <vector>
#include "physical_types.h"
#include "router_lookahead.h"
#include "router_lookahead_map_utils.h"
#include "router_lookahead_cost_map.h"

// Implementation of RouterLookahead based on source segment and destination connection box types
class ExtendedMapLookahead final : public RouterLookahead {
  public:
    ExtendedMapLookahead(bool is_flat, int route_verbosity, bool device_model_warnings)
        : is_flat_(is_flat)
        , route_verbosity_(route_verbosity)
        , device_model_warnings_(device_model_warnings) {}

  private:
    bool is_flat_;
    int route_verbosity_;
    bool device_model_warnings_;
    ///<Look-up table from SOURCE/OPIN to CHANX/CHANY of various types
    util::t_src_opin_delays src_opin_delays;

    ///<Look-up table from CHANX/CHANY to SINK/IPIN of various types
    util::t_chan_ipins_delays chan_ipins_delays;

    /**
     * @brief Returns the cost to get to a input delta_x/y location when starting from a SOURCE or OPIN node
     * @param from_node starting node
     * @param delta_x x delta value that is the distance between the current node to the target node
     * @param delta_y y delta value that is the distance between the current node to the target node
     * @param criticality_fac criticality of the current connection between 0 (all congestion) and 1 (all timing)
     * @return expected cost to get to the destination
     */
    std::pair<float, float> get_src_opin_cost(RRNodeId from_node, int delta_x, int delta_y, int to_layer_num, const t_conn_cost_params& params) const;

    CostMap cost_map_; ///<Cost map containing all data to extract the entry cost when querying the lookahead.
  public:
    /**
     * @brief Returns the expected cost to get to a destination node
     */
    float get_expected_cost(RRNodeId node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const override;

    /**
     * @brief Returns a pair of expected delay and congestion
     */
    std::pair<float, float> get_expected_delay_and_cong(RRNodeId inode, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const override;

    /**
     * @brief Computes the extended lookahead map
     */
    void compute(const std::vector<t_segment_inf>& segment_inf) override;

    void compute_intra_tile() override {
        VPR_THROW(VPR_ERROR_ROUTE, "ClassicLookahead::compute_intra_time unimplemented");
    }

    /**
     * @brief Reads the extended lookahead map
     */
    void read(const std::string& file) override;

    /**
     * @brief Read the extended intra-cluster lookahead map
     */
    void read_intra_cluster(const std::string& /*file*/) override {
        VPR_THROW(VPR_ERROR_ROUTE, "ExtendedMapLookahead::read_intra_cluster unimplemented");
    }

    /**
     * @brief Writes the extended lookahead map
     */
    void write(const std::string& file) const override;

    /**
     * @brief Writes the extended intra-cluster lookahead map
     */
    void write_intra_cluster(const std::string& /*file*/) const override {
        VPR_THROW(VPR_ERROR_ROUTE, "ExtendedMapLookahead::write_intra_cluster unimplemented");
    }

    float get_opin_distance_min_delay(int /*physical_tile_idx*/, const t_physical_tile_loc& /*from_loc*/, const t_physical_tile_loc& /*to_loc*/) const override {
        VPR_THROW(VPR_ERROR_ROUTE, "ExtendedMapLookahead::get_opin_distance_min_delay unimplemented");
    }
};
