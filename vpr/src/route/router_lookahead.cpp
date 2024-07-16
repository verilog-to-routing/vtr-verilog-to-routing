#include "router_lookahead.h"

#include "connection_router_interface.h"
#include "router_lookahead_map.h"
#include "router_lookahead_compressed_map.h"
#include "router_lookahead_extended_map.h"
#include "vpr_error.h"
#include "globals.h"

/**
 * Assuming inode is CHANX or CHANY, this function calculates the number of required wires of the same type as inode
 * to arrive at target_noe.
 * @param inode The source node from which the cost to the target node is obtained.
 * @param target_node The target node to which the cost is obtained.
 * @return std::pait<int, int> The first element is the number of required wires in the same direction as inode,
 * while the second element determines the number of wires in the direction orthogonal to inode.
 */
static std::pair<int, int> get_expected_segs_to_target(RRNodeId inode, RRNodeId target_node);

static int round_up(float x);

static std::unique_ptr<RouterLookahead> make_router_lookahead_object(const t_det_routing_arch& det_routing_arch,
                                                                     e_router_lookahead router_lookahead_type,
                                                                     bool is_flat) {
    if (router_lookahead_type == e_router_lookahead::CLASSIC) {
        return std::make_unique<ClassicLookahead>();
    } else if (router_lookahead_type == e_router_lookahead::MAP) {
        return std::make_unique<MapLookahead>(det_routing_arch, is_flat);
    } else if (router_lookahead_type == e_router_lookahead::COMPRESSED_MAP) {
        return std::make_unique<CompressedMapLookahead>(det_routing_arch, is_flat);
    } else if (router_lookahead_type == e_router_lookahead::EXTENDED_MAP) {
        return std::make_unique<ExtendedMapLookahead>(is_flat);
    } else if (router_lookahead_type == e_router_lookahead::NO_OP) {
        return std::make_unique<NoOpLookahead>();
    }

    VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Unrecognized router lookahead type");
    return nullptr;
}

std::unique_ptr<RouterLookahead> make_router_lookahead(const t_det_routing_arch& det_routing_arch,
                                                       e_router_lookahead router_lookahead_type,
                                                       const std::string& write_lookahead,
                                                       const std::string& read_lookahead,
                                                       const std::vector<t_segment_inf>& segment_inf,
                                                       bool is_flat) {
    std::unique_ptr<RouterLookahead> router_lookahead = make_router_lookahead_object(det_routing_arch,
                                                                                     router_lookahead_type,
                                                                                     is_flat);

    if (read_lookahead.empty()) {
        router_lookahead->compute(segment_inf);
    } else {
        router_lookahead->read(read_lookahead);
    }

    if (!write_lookahead.empty()) {
        router_lookahead->write(write_lookahead);
    }

    return router_lookahead;
}

float ClassicLookahead::get_expected_cost(RRNodeId current_node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const {
    auto [delay_cost, cong_cost] = get_expected_delay_and_cong(current_node, target_node, params, R_upstream);

    return delay_cost + cong_cost;
}

std::pair<float, float> ClassicLookahead::get_expected_delay_and_cong(RRNodeId node, RRNodeId target_node, const t_conn_cost_params& params, float R_upstream) const {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    t_rr_type rr_type = rr_graph.node_type(node);

    if (rr_type == CHANX || rr_type == CHANY) {
        auto [num_segs_same_dir, num_segs_ortho_dir] = get_expected_segs_to_target(node, target_node);

        auto cost_index = rr_graph.node_cost_index(node);
        int ortho_cost_index = device_ctx.rr_indexed_data[cost_index].ortho_cost_index;

        const auto& same_data = device_ctx.rr_indexed_data[cost_index];
        const auto& ortho_data = device_ctx.rr_indexed_data[RRIndexedDataId(ortho_cost_index)];
        const auto& ipin_data = device_ctx.rr_indexed_data[RRIndexedDataId(IPIN_COST_INDEX)];
        const auto& sink_data = device_ctx.rr_indexed_data[RRIndexedDataId(SINK_COST_INDEX)];

        float cong_cost = num_segs_same_dir * same_data.base_cost
                          + num_segs_ortho_dir * ortho_data.base_cost
                          + ipin_data.base_cost
                          + sink_data.base_cost;

        float Tdel = num_segs_same_dir * same_data.T_linear
                     + num_segs_ortho_dir * ortho_data.T_linear
                     + num_segs_same_dir * num_segs_same_dir * same_data.T_quadratic
                     + num_segs_ortho_dir * num_segs_ortho_dir * ortho_data.T_quadratic
                     + R_upstream * (num_segs_same_dir * same_data.C_load + num_segs_ortho_dir * ortho_data.C_load)
                     + ipin_data.T_linear;

        return std::make_pair(params.criticality * Tdel, (1 - params.criticality) * cong_cost);
    } else if (rr_type == IPIN) { /* Change if you're allowing route-throughs */
        return std::make_pair(0., device_ctx.rr_indexed_data[RRIndexedDataId(SINK_COST_INDEX)].base_cost);

    } else { /* Change this if you want to investigate route-throughs */
        return std::make_pair(0., 0.);
    }
}

float NoOpLookahead::get_expected_cost(RRNodeId /*current_node*/, RRNodeId /*target_node*/, const t_conn_cost_params& /*params*/, float /*R_upstream*/) const {
    return 0.;
}

std::pair<float, float> NoOpLookahead::get_expected_delay_and_cong(RRNodeId /*node*/, RRNodeId /*target_node*/, const t_conn_cost_params& /*params*/, float /*R_upstream*/) const {
    return std::make_pair(0., 0.);
}

/* Used below to ensure that fractions are rounded up, but floating   *
 * point values very close to an integer are rounded to that integer.       */
static int round_up(float x) {
    return std::ceil(x - 0.001);
}

static std::pair<int, int> get_expected_segs_to_target(RRNodeId inode, RRNodeId target_node) {
    /* Returns the number of segments the same type as inode that will be needed *
     * to reach target_node (not including inode) in each direction (the same    *
     * direction (horizontal or vertical) as inode and the orthogonal direction).*/
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    int num_segs_ortho_dir;
    int num_segs_same_dir;

    int no_need_to_pass_by_clb;
    float ylow, yhigh, xlow, xhigh;

    int target_x = rr_graph.node_xlow(target_node);
    int target_y = rr_graph.node_ylow(target_node);

    RRIndexedDataId cost_index = rr_graph.node_cost_index(inode);
    float inv_length = device_ctx.rr_indexed_data[cost_index].inv_length;
    int ortho_cost_index = device_ctx.rr_indexed_data[cost_index].ortho_cost_index;
    float ortho_inv_length = device_ctx.rr_indexed_data[RRIndexedDataId(ortho_cost_index)].inv_length;
    t_rr_type rr_type = rr_graph.node_type(inode);

    if (rr_type == CHANX) {
        ylow = rr_graph.node_ylow(inode);
        xhigh = rr_graph.node_xhigh(inode);
        xlow = rr_graph.node_xlow(inode);

        /* Count vertical (orthogonal to inode) segs first. */

        if (ylow > target_y) { /* Coming from a row above target? */
            num_segs_ortho_dir = round_up((ylow - target_y + 1.) * ortho_inv_length);
            no_need_to_pass_by_clb = 1;
        } else if (ylow < target_y - 1) { /* Below the CLB bottom? */
            num_segs_ortho_dir = round_up((target_y - ylow) * ortho_inv_length);
            no_need_to_pass_by_clb = 1;
        } else { /* In a row that passes by target CLB */
            num_segs_ortho_dir = 0;
            no_need_to_pass_by_clb = 0;
        }

        /* Now count horizontal (same dir. as inode) segs. */

        if (xlow > target_x + no_need_to_pass_by_clb) {
            num_segs_same_dir = round_up((xlow - no_need_to_pass_by_clb - target_x) * inv_length);
        } else if (xhigh < target_x - no_need_to_pass_by_clb) {
            num_segs_same_dir = round_up((target_x - no_need_to_pass_by_clb - xhigh) * inv_length);
        } else {
            num_segs_same_dir = 0;
        }
    } else { /* inode is a CHANY */
        ylow = rr_graph.node_ylow(inode);
        yhigh = rr_graph.node_yhigh(inode);
        xlow = rr_graph.node_xlow(inode);

        /* Count horizontal (orthogonal to inode) segs first. */

        if (xlow > target_x) { /* Coming from a column right of target? */
            num_segs_ortho_dir = round_up((xlow - target_x + 1.) * ortho_inv_length);
            no_need_to_pass_by_clb = 1;
        } else if (xlow < target_x - 1) { /* Left of and not adjacent to the CLB? */
            num_segs_ortho_dir = round_up((target_x - xlow) * ortho_inv_length);
            no_need_to_pass_by_clb = 1;
        } else { /* In a column that passes by target CLB */
            num_segs_ortho_dir = 0;
            no_need_to_pass_by_clb = 0;
        }

        /* Now count vertical (same dir. as inode) segs. */

        if (ylow > target_y + no_need_to_pass_by_clb) {
            num_segs_same_dir = round_up((ylow - no_need_to_pass_by_clb - target_y) * inv_length);
        } else if (yhigh < target_y - no_need_to_pass_by_clb) {
            num_segs_same_dir = round_up((target_y - no_need_to_pass_by_clb - yhigh) * inv_length);
        } else {
            num_segs_same_dir = 0;
        }
    }

    return {num_segs_same_dir, num_segs_ortho_dir};
}

void invalidate_router_lookahead_cache() {
    auto& router_ctx = g_vpr_ctx.mutable_routing();
    router_ctx.cached_router_lookahead_.clear();
}

const RouterLookahead* get_cached_router_lookahead(const t_det_routing_arch& det_routing_arch,
                                                   e_router_lookahead router_lookahead_type,
                                                   const std::string& write_lookahead,
                                                   const std::string& read_lookahead,
                                                   const std::vector<t_segment_inf>& segment_inf,
                                                   bool is_flat) {
    auto& router_ctx = g_vpr_ctx.routing();
    auto& mut_router_ctx = g_vpr_ctx.mutable_routing();

    auto cache_key = std::make_tuple(router_lookahead_type, read_lookahead, segment_inf);
    mut_router_ctx.router_lookahead_cache_key_ = cache_key;

    // Check if cache is valid.
    const RouterLookahead* router_lookahead = router_ctx.cached_router_lookahead_.get(cache_key);
    if (router_lookahead) {
        return router_lookahead;
    } else {
        return mut_router_ctx.cached_router_lookahead_.set(
            cache_key,
            make_router_lookahead(det_routing_arch,
                                  router_lookahead_type,
                                  write_lookahead,
                                  read_lookahead,
                                  segment_inf,
                                  is_flat));
    }
}
