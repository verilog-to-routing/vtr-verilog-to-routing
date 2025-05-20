#ifndef VPR_ROUTER_LOOKAHEAD_H
#define VPR_ROUTER_LOOKAHEAD_H
#include <memory>
#include "vpr_types.h"
#include "vpr_error.h"

struct t_conn_cost_params; //Forward declaration

/**
 * @brief The base class to define router lookahead.
 *
 * This class is used by different parts of VPR to get an estimation of the cost, in terms of delay and congestion, to get from one point to the other.
 */
class RouterLookahead {
  public:
    /**
     * @brief Get expected cost from node to target_node.
     * @attention Either compute or read methods must be invoked before invoking get_expected_cost.
     * @param node The source node from which the cost to the target node is obtained.
     * @param target_node The target node to which the cost is obtained.
     * @param params Contain the router parameter such as connection criticality, etc. Used to calculate the cost based on the delay and congestion costs.
     * @param R_upstream Upstream resistance to get to the "node".
     * @return
     */
    virtual float get_expected_cost(RRNodeId node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const = 0;
    virtual std::pair<float, float> get_expected_delay_and_cong(RRNodeId node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const = 0;

    /**
     * @brief Compute router lookahead (if needed)
     * @param segment_inf
     */
    virtual void compute(const std::vector<t_segment_inf>& segment_inf) = 0;

    /**
     * @brief Initialize the data structures which store the information about intra-cluster resources.
     * @attention This function is called in the routing stage.
     */
    virtual void compute_intra_tile() = 0;

    /**
     * @brief Read router lookahead data (if any) from specified file.
     * @attention May be unimplemented, in which case method should throw an exception.
     * @param file Name of the file that stores the router lookahead.
     */
    virtual void read(const std::string& file) = 0;

    /**
     * @brief Read intra-cluster router lookahead data (if any) from specified file.
     * @attention May be unimplemented, in which case method should throw an exception.
     * @param file Name of the file that stores the intra-cluster router lookahead.
     */
    virtual void read_intra_cluster(const std::string& file) = 0;

    /**
     * @brief Write router lookahead data (if any) to specified file.
     * @attention May be unimplemented, in which case method should throw an exception.
     * @param file Name of the file to write the router lookahead.
     */
    virtual void write(const std::string& file) const = 0;

    /**
     * @brief Write intra-cluster router lookahead data (if any) to specified file.
     * @attention May be unimplemented, in which case method should throw an exception.
     * @param file Name of the file to write the intra-cluster router lookahead.
     */
    virtual void write_intra_cluster(const std::string& file) const = 0;

    /**
     * @brief Retrieve the minimum delay to a point on the "to_layer," which is dx and dy away, across all the OPINs on the physical tile identified by "physical_tile_idx."
     * @param physical_tile_idx The index of the physical tile from which the cost is calculated
     * @param from_layer The layer that the tile is located on
     * @param to_layer The layer on which the destination is located
     * @param dx Horizontal distance to the destination
     * @param dy Vertical distance to the destination
     * @return Minimum delay to a point which is dx and dy away from a point on the die number "from_layer" to a point on the die number "to_layer".
     */
    virtual float get_opin_distance_min_delay(int physical_tile_idx, int from_layer, int to_layer, int dx, int dy) const = 0;

    virtual ~RouterLookahead() {}
};

/**
 * @brief Force creation of lookahead object.
 * @attention This may involve recomputing the lookahead, so only use if lookahead cache cannot be used.
 * @param det_routing_arch
 * @param router_lookahead_type
 * @param write_lookahead
 * @param read_lookahead
 * @param segment_inf
 * @param is_flat
 * @return Return a unique pointer that points to the router lookahead object
 */
std::unique_ptr<RouterLookahead> make_router_lookahead(const t_det_routing_arch& det_routing_arch,
                                                       e_router_lookahead router_lookahead_type,
                                                       const std::string& write_lookahead,
                                                       const std::string& read_lookahead,
                                                       const std::vector<t_segment_inf>& segment_inf,
                                                       bool is_flat);

/**
 * @brief Clear router lookahead cache (e.g. when changing or free rrgraph).
 */
void invalidate_router_lookahead_cache();

/**
 * @brief Returns lookahead for given rr graph.
 * @attention Object is cached in RouterContext, but access to cached object should performed via this function.
 * @param det_routing_arch
 * @param router_lookahead_type
 * @param write_lookahead
 * @param read_lookahead
 * @param segment_inf
 * @param is_flat
 * @return
 */
const RouterLookahead* get_cached_router_lookahead(const t_det_routing_arch& det_routing_arch,
                                                   e_router_lookahead router_lookahead_type,
                                                   const std::string& write_lookahead,
                                                   const std::string& read_lookahead,
                                                   const std::vector<t_segment_inf>& segment_inf,
                                                   bool is_flat);

class ClassicLookahead : public RouterLookahead {
  public:
    float get_expected_cost(RRNodeId node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const override;
    std::pair<float, float> get_expected_delay_and_cong(RRNodeId node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const override;

    void compute(const std::vector<t_segment_inf>& /*segment_inf*/) override {
    }

    void compute_intra_tile() override {
        VPR_THROW(VPR_ERROR_ROUTE, "ClassicLookahead::compute_intra_time unimplemented");
    }

    void read(const std::string& /*file*/) override {
        VPR_THROW(VPR_ERROR_ROUTE, "ClassicLookahead::read unimplemented");
    }

    void read_intra_cluster(const std::string& /*file*/) override {
        VPR_THROW(VPR_ERROR_ROUTE, "ClassicLookahead::read_intra_cluster unimplemented");
    }

    void write(const std::string& /*file*/) const override {
        VPR_THROW(VPR_ERROR_ROUTE, "ClassicLookahead::write unimplemented");
    }

    void write_intra_cluster(const std::string& /*file*/) const override {
        VPR_THROW(VPR_ERROR_ROUTE, "ClassicLookahead::write_intra_cluster unimplemented");
    }

    float get_opin_distance_min_delay(int /*physical_tile_idx*/, int /*from_layer*/, int /*to_layer*/, int /*dx*/, int /*dy*/) const override {
        return -1.;
    }

  private:
    float classic_wire_lookahead_cost(RRNodeId node, RRNodeId target_node, float criticality, float R_upstream) const;
};

class NoOpLookahead : public RouterLookahead {
  protected:
    float get_expected_cost(RRNodeId node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const override;
    std::pair<float, float> get_expected_delay_and_cong(RRNodeId node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const override;

    void compute(const std::vector<t_segment_inf>& /*segment_inf*/) override {
    }

    void compute_intra_tile() override {
        VPR_THROW(VPR_ERROR_ROUTE, "ClassicLookahead::compute_intra_time unimplemented");
    }

    void read(const std::string& /*file*/) override {
        VPR_THROW(VPR_ERROR_ROUTE, "Read not supported for NoOpLookahead");
    }

    void read_intra_cluster(const std::string& /*file*/) override {
        VPR_THROW(VPR_ERROR_ROUTE, "read_intra_cluster not supported for NoOpLookahead");
    }

    void write(const std::string& /*file*/) const override {
        VPR_THROW(VPR_ERROR_ROUTE, "Write not supported for NoOpLookahead");
    }

    void write_intra_cluster(const std::string& /*file*/) const override {
        VPR_THROW(VPR_ERROR_ROUTE, "write_intra_cluster not supported for NoOpLookahead");
    }

    float get_opin_distance_min_delay(int /*physical_tile_idx*/, int /*from_layer*/, int /*to_layer*/, int /*dx*/, int /*dy*/) const override {
        return -1.;
    }
};

#endif
