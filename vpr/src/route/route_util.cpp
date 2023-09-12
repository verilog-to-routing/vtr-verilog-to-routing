#include "route_util.h"
#include "globals.h"
#include "draw_types.h"
#include "draw_global.h"

vtr::Matrix<float> calculate_routing_usage(t_rr_type rr_type, bool is_flat, bool is_print) {
    VTR_ASSERT(rr_type == CHANX || rr_type == CHANY);

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

    vtr::Matrix<float> usage({{device_ctx.grid.width(), device_ctx.grid.height()}}, 0.);

    //Collect all the in-use RR nodes
    std::set<RRNodeId> rr_nodes;
    for (auto net : cluster_ctx.clb_nlist.nets()) {
        auto parent_id = get_cluster_net_parent_id(g_vpr_ctx.atom().lookup, net, is_flat);

        if (!route_ctx.route_trees[parent_id])
            continue;
        for (auto& rt_node : route_ctx.route_trees[parent_id].value().all_nodes()) {
            if (rr_graph.node_type(rt_node.inode) == rr_type) {
                rr_nodes.insert(rt_node.inode);
            }
        }
    }

    //Record number of used resources in each x/y channel
    for (RRNodeId rr_node : rr_nodes) {
#ifndef NO_GRAPHICS
        if (!is_print) {
            t_draw_state* draw_state = get_draw_state_vars();
            int layer_num = rr_graph.node_layer(rr_node);
            if (!draw_state->draw_layer_display[layer_num].visible)
                continue; // don't count usage if layer is not visible
        }
#endif

        if (rr_type == CHANX) {
            VTR_ASSERT(rr_graph.node_type(rr_node) == CHANX);
            VTR_ASSERT(rr_graph.node_ylow(rr_node) == rr_graph.node_yhigh(rr_node));

            int y = rr_graph.node_ylow(rr_node);
            for (int x = rr_graph.node_xlow(rr_node); x <= rr_graph.node_xhigh(rr_node); ++x) {
                usage[x][y] += route_ctx.rr_node_route_inf[rr_node].occ();
            }
        } else {
            VTR_ASSERT(rr_type == CHANY);
            VTR_ASSERT(rr_graph.node_type(rr_node) == CHANY);
            VTR_ASSERT(rr_graph.node_xlow(rr_node) == rr_graph.node_xhigh(rr_node));

            int x = rr_graph.node_xlow(rr_node);
            for (int y = rr_graph.node_ylow(rr_node); y <= rr_graph.node_yhigh(rr_node); ++y) {
                usage[x][y] += route_ctx.rr_node_route_inf[rr_node].occ();
            }
        }
    }
    return usage;
}

vtr::Matrix<float> calculate_routing_avail(t_rr_type rr_type) {
    //Calculate the number of available resources in each x/y channel
    VTR_ASSERT(rr_type == CHANX || rr_type == CHANY);

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    vtr::Matrix<float> avail({{device_ctx.grid.width(), device_ctx.grid.height()}}, 0.);
    for (const RRNodeId& rr_node : rr_graph.nodes()) {
        const short& rr_node_capacity = rr_graph.node_capacity(rr_node);

        if (rr_graph.node_type(rr_node) == CHANX && rr_type == CHANX) {
            VTR_ASSERT(rr_graph.node_type(rr_node) == CHANX);
            VTR_ASSERT(rr_graph.node_ylow(rr_node) == rr_graph.node_yhigh(rr_node));

            int y = rr_graph.node_ylow(rr_node);
            for (int x = rr_graph.node_xlow(rr_node); x <= rr_graph.node_xhigh(rr_node); ++x) {
                avail[x][y] += rr_node_capacity;
            }
        } else if (rr_graph.node_type(rr_node) == CHANY && rr_type == CHANY) {
            VTR_ASSERT(rr_graph.node_type(rr_node) == CHANY);
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
