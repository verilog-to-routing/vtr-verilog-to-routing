/* Standard header files required go first */

/* EXTERNAL library header files go second*/

/* VPR header files go second*/
#include "timing_driven_router_lookahead_map.h"
#include "vpr_error.h"
#include "timing_driven_route_timing.h"
#include "timing_driven_router_lookahead.h"
#include "rr_graph_fwd.h"

/* Finally we include global variables */
#include "globals.h"


/* All the routers should be placed in the namespace of router
 * A specific router may have it own namespace under the router namespace
 * To call a function in a router, function need a prefix of router::<your_router_namespace>::
 * This will ease the development in modularization.
 * The concern is to minimize/avoid conflicts between data type names as much as possible
 * Also, this will keep function names as short as possible.
 */
/* Categorize all the functions in the specific name space of this router */
namespace router {

namespace timing_driven {

static int get_expected_segs_to_target(RRNodeId inode_id, RRNodeId target_node_id, int *num_segs_ortho_dir_ptr);
static int round_up(float x);

std::unique_ptr<RouterLookahead> make_router_lookahead(e_router_lookahead router_lookahead_type) {
    if (router_lookahead_type == e_router_lookahead::CLASSIC) {
        return std::make_unique<ClassicLookahead>();
    } else if (router_lookahead_type == e_router_lookahead::MAP) {
        return std::make_unique<MapLookahead>();
    } else if (router_lookahead_type == e_router_lookahead::NO_OP) {
        return std::make_unique<NoOpLookahead>();
    }

    VPR_THROW(VPR_ERROR_ROUTE, "Unrecognized router lookahead type");
    return nullptr;
}


float ClassicLookahead::get_expected_cost(RRNodeId current_node_id, RRNodeId target_node_id, 
                                          const t_conn_cost_params& params, float R_upstream) const {
    auto& device_ctx = g_vpr_ctx.device();

    t_rr_type rr_type = device_ctx.rr_graph.node_type(current_node_id);

    if (rr_type == CHANX || rr_type == CHANY) {
        return classic_wire_lookahead_cost(current_node_id, target_node_id, params.criticality, R_upstream);
    } else if (rr_type == IPIN) { /* Change if you're allowing route-throughs */
        return (device_ctx.rr_indexed_data[SINK_COST_INDEX].base_cost);
    } else { /* Change this if you want to investigate route-throughs */
        return (0.);
    }
}

float ClassicLookahead::classic_wire_lookahead_cost(RRNodeId inode_id, RRNodeId target_node_id,
                                                    float criticality, float R_upstream) const {
    auto& device_ctx = g_vpr_ctx.device();

    VTR_ASSERT_SAFE(CHANX == device_ctx.rr_graph.node_type(inode_id) 
                 || CHANY == device_ctx.rr_graph.node_type(inode_id));

    int num_segs_ortho_dir = 0;
    int num_segs_same_dir = get_expected_segs_to_target(inode_id, target_node_id, &num_segs_ortho_dir);

    int cost_index = device_ctx.rr_graph.node_cost_index(inode_id);
    int ortho_cost_index = device_ctx.rr_indexed_data[cost_index].ortho_cost_index;

    const auto& same_data = device_ctx.rr_indexed_data[cost_index];
    const auto& ortho_data = device_ctx.rr_indexed_data[ortho_cost_index];
    const auto& ipin_data = device_ctx.rr_indexed_data[IPIN_COST_INDEX];
    const auto& sink_data = device_ctx.rr_indexed_data[SINK_COST_INDEX];

    float cong_cost =   num_segs_same_dir * same_data.base_cost
                      + num_segs_ortho_dir * ortho_data.base_cost
                      + ipin_data.base_cost
                      + sink_data.base_cost;

    float Tdel =   num_segs_same_dir * same_data.T_linear
                 + num_segs_ortho_dir * ortho_data.T_linear
                 + num_segs_same_dir * num_segs_same_dir * same_data.T_quadratic
                 + num_segs_ortho_dir * num_segs_ortho_dir * ortho_data.T_quadratic
                 + R_upstream * (  num_segs_same_dir * same_data.C_load
                                 + num_segs_ortho_dir * ortho_data.C_load)
                 + ipin_data.T_linear;

    float expected_cost = criticality * Tdel + (1. - criticality) * cong_cost;
    return (expected_cost);
}

float MapLookahead::get_expected_cost(RRNodeId current_node_id, RRNodeId target_node_id, 
                                      const t_conn_cost_params& params, float /*R_upstream*/) const {
    auto& device_ctx = g_vpr_ctx.device();

    t_rr_type rr_type = device_ctx.rr_graph.node_type(current_node_id);

    if (rr_type == CHANX || rr_type == CHANY) {
        return get_lookahead_map_cost(current_node_id, target_node_id, params.criticality);
    } else if (rr_type == IPIN) { /* Change if you're allowing route-throughs */
        return (device_ctx.rr_indexed_data[SINK_COST_INDEX].base_cost);
    } else { /* Change this if you want to investigate route-throughs */
        return (0.);
    }
}

float NoOpLookahead::get_expected_cost(RRNodeId /*current_node*/, RRNodeId /*target_node*/, 
                                       const t_conn_cost_params& /*params*/, float /*R_upstream*/) const {
    return 0.;
}


/* Used below to ensure that fractions are rounded up, but floating   *
 * point values very close to an integer are rounded to that integer.       */
static int round_up(float x) {
    return std::ceil(x - 0.001);
}

static int get_expected_segs_to_target(RRNodeId inode_id, RRNodeId target_node_id,
                                       int *num_segs_ortho_dir_ptr) {

    /* Returns the number of segments the same type as inode that will be needed *
     * to reach target_node (not including inode) in each direction (the same    *
     * direction (horizontal or vertical) as inode and the orthogonal direction).*/

    auto& device_ctx = g_vpr_ctx.device();

    t_rr_type rr_type;
    int target_x, target_y, num_segs_same_dir, cost_index, ortho_cost_index;
    int no_need_to_pass_by_clb;
    float inv_length, ortho_inv_length, ylow, yhigh, xlow, xhigh;

    target_x = device_ctx.rr_graph.node_xlow(target_node_id);
    target_y = device_ctx.rr_graph.node_ylow(target_node_id);
    cost_index = device_ctx.rr_graph.node_cost_index(inode_id);
    inv_length = device_ctx.rr_indexed_data[cost_index].inv_length;
    ortho_cost_index = device_ctx.rr_indexed_data[cost_index].ortho_cost_index;
    ortho_inv_length = device_ctx.rr_indexed_data[ortho_cost_index].inv_length;
    rr_type = device_ctx.rr_graph.node_type(inode_id);

    if (rr_type == CHANX) {
        ylow = device_ctx.rr_graph.node_ylow(inode_id);
        xhigh = device_ctx.rr_graph.node_xhigh(inode_id);
        xlow = device_ctx.rr_graph.node_xlow(inode_id);

        /* Count vertical (orthogonal to inode) segs first. */

        if (ylow > target_y) { /* Coming from a row above target? */
            *num_segs_ortho_dir_ptr = round_up((ylow - target_y + 1.) * ortho_inv_length);
            no_need_to_pass_by_clb = 1;
        } else if (ylow < target_y - 1) { /* Below the CLB bottom? */
            *num_segs_ortho_dir_ptr = round_up((target_y - ylow) * ortho_inv_length);
            no_need_to_pass_by_clb = 1;
        } else { /* In a row that passes by target CLB */
            *num_segs_ortho_dir_ptr = 0;
            no_need_to_pass_by_clb = 0;
        }


        /* Now count horizontal (same dir. as inode) segs. */

        if (xlow > target_x + no_need_to_pass_by_clb) {
            num_segs_same_dir = round_up((xlow - no_need_to_pass_by_clb -
                    target_x) * inv_length);
        } else if (xhigh < target_x - no_need_to_pass_by_clb) {
            num_segs_same_dir = round_up((target_x - no_need_to_pass_by_clb -
                    xhigh) * inv_length);
        } else {
            num_segs_same_dir = 0;
        }
    } else { /* inode is a CHANY */
        ylow = device_ctx.rr_graph.node_ylow(inode_id);
        yhigh = device_ctx.rr_graph.node_yhigh(inode_id);
        xlow = device_ctx.rr_graph.node_xlow(inode_id);

        /* Count horizontal (orthogonal to inode) segs first. */

        if (xlow > target_x) { /* Coming from a column right of target? */
            *num_segs_ortho_dir_ptr = round_up((xlow - target_x + 1.) * ortho_inv_length);
            no_need_to_pass_by_clb = 1;
        } else if (xlow < target_x - 1) { /* Left of and not adjacent to the CLB? */
            *num_segs_ortho_dir_ptr = round_up((target_x - xlow) *
                    ortho_inv_length);
            no_need_to_pass_by_clb = 1;
        } else { /* In a column that passes by target CLB */
            *num_segs_ortho_dir_ptr = 0;
            no_need_to_pass_by_clb = 0;
        }

        /* Now count vertical (same dir. as inode) segs. */

        if (ylow > target_y + no_need_to_pass_by_clb) {
            num_segs_same_dir = round_up((ylow - no_need_to_pass_by_clb -
                    target_y) * inv_length);
        } else if (yhigh < target_y - no_need_to_pass_by_clb) {
            num_segs_same_dir = round_up((target_y - no_need_to_pass_by_clb -
                    yhigh) * inv_length);
        } else {
            num_segs_same_dir = 0;
        }

    }

    return (num_segs_same_dir);
}

}// end namespace timing_driven

}// end namespace router
