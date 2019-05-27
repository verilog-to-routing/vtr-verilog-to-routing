#include "route_util.h"
#include "globals.h"

vtr::Matrix<float> calculate_routing_usage(t_rr_type rr_type) {
    VTR_ASSERT(rr_type == CHANX || rr_type == CHANY);

    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

    vtr::Matrix<float> usage({{device_ctx.grid.width(), device_ctx.grid.height()}}, 0.);

    //Collect all the in-use RR nodes
    std::set<int> rr_nodes;
    for (auto net : cluster_ctx.clb_nlist.nets()) {
        t_trace* tptr = route_ctx.trace[net].head;
        while (tptr != nullptr) {
            int inode = tptr->index;

            if (device_ctx.rr_nodes[inode].type() == rr_type) {
                rr_nodes.insert(inode);
            }
            tptr = tptr->next;
        }
    }

    //Record number of used resources in each x/y channel
    for (int inode : rr_nodes) {
        auto& rr_node = device_ctx.rr_nodes[inode];

        if (rr_type == CHANX) {
            VTR_ASSERT(rr_node.type() == CHANX);
            VTR_ASSERT(rr_node.ylow() == rr_node.yhigh());

            int y = rr_node.ylow();
            for (int x = rr_node.xlow(); x <= rr_node.xhigh(); ++x) {
                usage[x][y] += route_ctx.rr_node_route_inf[inode].occ();
            }
        } else {
            VTR_ASSERT(rr_type == CHANY);
            VTR_ASSERT(rr_node.type() == CHANY);
            VTR_ASSERT(rr_node.xlow() == rr_node.xhigh());

            int x = rr_node.xlow();
            for (int y = rr_node.ylow(); y <= rr_node.yhigh(); ++y) {
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

    vtr::Matrix<float> avail({{device_ctx.grid.width(), device_ctx.grid.height()}}, 0.);
    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); ++inode) {
        auto& rr_node = device_ctx.rr_nodes[inode];

        if (rr_node.type() == CHANX && rr_type == CHANX) {
            VTR_ASSERT(rr_node.type() == CHANX);
            VTR_ASSERT(rr_node.ylow() == rr_node.yhigh());

            int y = rr_node.ylow();
            for (int x = rr_node.xlow(); x <= rr_node.xhigh(); ++x) {
                avail[x][y] += rr_node.capacity();
            }
        } else if (rr_node.type() == CHANY && rr_type == CHANY) {
            VTR_ASSERT(rr_node.type() == CHANY);
            VTR_ASSERT(rr_node.xlow() == rr_node.xhigh());

            int x = rr_node.xlow();
            for (int y = rr_node.ylow(); y <= rr_node.yhigh(); ++y) {
                avail[x][y] += rr_node.capacity();
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
