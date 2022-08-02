#include "route_util.h"
#include "globals.h"

vtr::Matrix<float> calculate_routing_usage(t_rr_type rr_type, bool is_flat) {
    VTR_ASSERT(rr_type == CHANX || rr_type == CHANY);

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

    vtr::Matrix<float> usage({{device_ctx.grid.width(), device_ctx.grid.height()}}, 0.);

    //Collect all the in-use RR nodes
    std::set<int> rr_nodes;
    for (auto net : cluster_ctx.clb_nlist.nets()) {
        auto par_net_id = get_cluster_net_parent_id(g_vpr_ctx.atom().lookup, net, is_flat);
        t_trace* tptr = route_ctx.trace[par_net_id].head;
        while (tptr != nullptr) {
            int inode = tptr->index;

            if (rr_graph.node_type(RRNodeId(inode)) == rr_type) {
                rr_nodes.insert(inode);
            }
            tptr = tptr->next;
        }
    }

    //Record number of used resources in each x/y channel
    for (int inode : rr_nodes) {
        RRNodeId rr_node = RRNodeId(inode);

        if (rr_type == CHANX) {
            VTR_ASSERT(rr_graph.node_type(rr_node) == CHANX);
            VTR_ASSERT(rr_graph.node_ylow(rr_node) == rr_graph.node_yhigh(rr_node));

            int y = rr_graph.node_ylow(rr_node);
            for (int x = rr_graph.node_xlow(rr_node); x <= rr_graph.node_xhigh(rr_node); ++x) {
                usage[x][y] += route_ctx.rr_node_route_inf[inode].occ();
            }
        } else {
            VTR_ASSERT(rr_type == CHANY);
            VTR_ASSERT(rr_graph.node_type(rr_node) == CHANY);
            VTR_ASSERT(rr_graph.node_xlow(rr_node) == rr_graph.node_xhigh(rr_node));

            int x = rr_graph.node_xlow(rr_node);
            for (int y = rr_graph.node_ylow(rr_node); y <= rr_graph.node_yhigh(rr_node); ++y) {
                usage[x][y] += route_ctx.rr_node_route_inf[inode].occ();
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

bool node_in_same_physical_tile(RRNodeId node_first, RRNodeId node_second) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto first_rr_type = rr_graph.node_type(node_first);
    auto second_rr_type = rr_graph.node_type(node_second);

    if(first_rr_type == t_rr_type::CHANX || first_rr_type == t_rr_type::CHANY ||
        second_rr_type == t_rr_type::CHANX || second_rr_type == t_rr_type::CHANY) {
        return false;
    } else {
        VTR_ASSERT(first_rr_type == t_rr_type::IPIN || first_rr_type == t_rr_type::OPIN ||
                   first_rr_type == t_rr_type::SINK || first_rr_type == t_rr_type::SOURCE);
        VTR_ASSERT(second_rr_type == t_rr_type::IPIN || second_rr_type == t_rr_type::OPIN ||
                   second_rr_type == t_rr_type::SINK || second_rr_type == t_rr_type::SOURCE);
        int first_x = rr_graph.node_xlow(node_first);
        int first_y = rr_graph.node_ylow(node_first);
        int sec_x = rr_graph.node_xlow(node_second);
        int sec_y = rr_graph.node_ylow(node_second);

        int first_root_x = first_x - device_ctx.grid[first_x][first_y].width_offset;
        int first_root_y = first_y - device_ctx.grid[first_x][first_y].height_offset;

        int sec_root_x = sec_x - device_ctx.grid[sec_x][sec_y].width_offset;
        int sec_root_y = sec_y - device_ctx.grid[sec_x][sec_y].height_offset;

        if(first_root_x == sec_root_x && first_root_y == sec_root_y)
            return true;
        else
            return false;


    }

}
