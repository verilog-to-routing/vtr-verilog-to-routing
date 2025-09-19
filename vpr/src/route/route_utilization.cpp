#include "route_utilization.h"
#include "draw_global.h"
#include "draw_types.h"
#include "globals.h"
#include "vpr_utils.h"
#include "route_common.h"

RoutingChanUtilEstimator::RoutingChanUtilEstimator(const BlkLocRegistry& blk_loc_registry, bool cube_bb) {
    placer_state_ = std::make_unique<PlacerState>(/*placement_is_timing_driven=*/false);
    placer_state_->mutable_blk_loc_registry() = blk_loc_registry;

    placer_opts_.place_algorithm = e_place_algorithm::BOUNDING_BOX_PLACE;
    net_cost_handler_ = std::make_unique<NetCostHandler>(placer_opts_, *placer_state_, cube_bb);
}

std::pair<vtr::NdMatrix<double, 3>, vtr::NdMatrix<double, 3>> RoutingChanUtilEstimator::estimate_routing_chan_util() {
    const auto& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    const auto& block_locs = placer_state_->block_locs();

    bool placement_is_done = (block_locs.size() == clb_nlist.blocks().size());

    if (placement_is_done) {
        // Compute net bounding boxes
        net_cost_handler_->comp_bb_cost(e_cost_methods::NORMAL);

        // Estimate routing channel utilization using
        return net_cost_handler_->estimate_routing_chan_util();
    } else {
        const auto& device_ctx = g_vpr_ctx.device();

        auto chanx_util = vtr::NdMatrix<double, 3>({{(size_t)device_ctx.grid.get_num_layers(),
                                                     device_ctx.grid.width(),
                                                     device_ctx.grid.height()}},
                                                   0);

        auto chany_util = vtr::NdMatrix<double, 3>({{(size_t)device_ctx.grid.get_num_layers(),
                                                     device_ctx.grid.width(),
                                                     device_ctx.grid.height()}},
                                                   0);

        return {chanx_util, chany_util};
    }
}

vtr::Matrix<float> calculate_routing_usage(e_rr_type rr_type, bool is_flat, bool is_print) {
    VTR_ASSERT(rr_type == e_rr_type::CHANX || rr_type == e_rr_type::CHANY);

    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& route_ctx = g_vpr_ctx.routing();

    vtr::Matrix<float> usage({{device_ctx.grid.width(), device_ctx.grid.height()}}, 0.);

    // Collect all the in-use RR nodes
    std::set<RRNodeId> rr_nodes;
    for (ClusterNetId net : cluster_ctx.clb_nlist.nets()) {
        ParentNetId parent_id = get_cluster_net_parent_id(g_vpr_ctx.atom().lookup(), net, is_flat);

        if (!route_ctx.route_trees[parent_id])
            continue;
        for (const RouteTreeNode& rt_node : route_ctx.route_trees[parent_id].value().all_nodes()) {
            if (rr_graph.node_type(rt_node.inode) == rr_type) {
                rr_nodes.insert(rt_node.inode);
            }
        }
    }

    // Record number of used resources in each x/y channel
    for (RRNodeId rr_node : rr_nodes) {
#ifndef NO_GRAPHICS
        if (!is_print) {
            t_draw_state* draw_state = get_draw_state_vars();
            int layer_num = rr_graph.node_layer_low(rr_node);
            if (!draw_state->draw_layer_display[layer_num].visible)
                continue; // don't count usage if layer is not visible
        }
#else
        // Cast to void to avoid warning.
        (void)is_print;
#endif

        if (rr_type == e_rr_type::CHANX) {
            VTR_ASSERT(rr_graph.node_type(rr_node) == e_rr_type::CHANX);
            VTR_ASSERT(rr_graph.node_ylow(rr_node) == rr_graph.node_yhigh(rr_node));

            int y = rr_graph.node_ylow(rr_node);
            for (int x = rr_graph.node_xlow(rr_node); x <= rr_graph.node_xhigh(rr_node); ++x) {
                usage[x][y] += route_ctx.rr_node_route_inf[rr_node].occ();
            }
        } else {
            VTR_ASSERT(rr_type == e_rr_type::CHANY);
            VTR_ASSERT(rr_graph.node_type(rr_node) == e_rr_type::CHANY);
            VTR_ASSERT(rr_graph.node_xlow(rr_node) == rr_graph.node_xhigh(rr_node));

            int x = rr_graph.node_xlow(rr_node);
            for (int y = rr_graph.node_ylow(rr_node); y <= rr_graph.node_yhigh(rr_node); ++y) {
                usage[x][y] += route_ctx.rr_node_route_inf[rr_node].occ();
            }
        }
    }
    return usage;
}

vtr::Matrix<float> calculate_routing_avail(e_rr_type rr_type) {
    // Calculate the number of available resources in each x/y channel
    VTR_ASSERT(rr_type == e_rr_type::CHANX || rr_type == e_rr_type::CHANY);

    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    vtr::Matrix<float> avail({{device_ctx.grid.width(), device_ctx.grid.height()}}, 0.);
    for (const RRNodeId& rr_node : rr_graph.nodes()) {
        const short& rr_node_capacity = rr_graph.node_capacity(rr_node);

        if (rr_graph.node_type(rr_node) == e_rr_type::CHANX && rr_type == e_rr_type::CHANX) {
            VTR_ASSERT(rr_graph.node_type(rr_node) == e_rr_type::CHANX);
            VTR_ASSERT(rr_graph.node_ylow(rr_node) == rr_graph.node_yhigh(rr_node));

            int y = rr_graph.node_ylow(rr_node);
            for (int x = rr_graph.node_xlow(rr_node); x <= rr_graph.node_xhigh(rr_node); ++x) {
                avail[x][y] += rr_node_capacity;
            }
        } else if (rr_graph.node_type(rr_node) == e_rr_type::CHANY && rr_type == e_rr_type::CHANY) {
            VTR_ASSERT(rr_graph.node_type(rr_node) == e_rr_type::CHANY);
            VTR_ASSERT(rr_graph.node_xlow(rr_node) == rr_graph.node_xhigh(rr_node));

            int x = rr_graph.node_xlow(rr_node);
            for (int y = rr_graph.node_ylow(rr_node); y <= rr_graph.node_yhigh(rr_node); ++y) {
                avail[x][y] += rr_node_capacity;
            }
        }
    }
    return avail;
}

float routing_util(float used, float avail) {
    if (used > 0.) {
        VTR_ASSERT(avail > 0.);
        return used / avail;
    }
    return 0.;
}
