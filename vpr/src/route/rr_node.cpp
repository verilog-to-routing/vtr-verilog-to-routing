#include "rr_node.h"
#include "rr_graph_storage.h"
#include "globals.h"
#include "vpr_error.h"
#include "rr_graph.h"

//Returns the max 'length' over the x or y direction
short t_rr_node::length() const {
    return std::max(
        storage_->node_yhigh(id_) - storage_->node_ylow(id_),
        storage_->node_xhigh(id_) - storage_->node_xlow(id_));
}

bool t_rr_node::edge_is_configurable(t_edge_size iedge) const {
    auto iswitch = edge_switch(iedge);

    auto& device_ctx = g_vpr_ctx.device();

    return device_ctx.rr_switch_inf[iswitch].configurable();
}

bool t_rr_node::validate() const {
    //Check internal assumptions about RR node are valid
    auto& device_ctx = g_vpr_ctx.device();

    const auto& rr_graph = device_ctx.rr_graph;

    t_edge_size iedge = 0;
    for (auto edge : rr_graph.edges(RRNodeId(id_))) {
        if (edge < rr_graph.num_configurable_edges(RRNodeId(id_))) {
            if (!edge_is_configurable(edge)) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "RR Node non-configurable edge found in configurable edge list");
            }
        } else {
            if (edge_is_configurable(edge)) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "RR Node configurable edge found in non-configurable edge list");
            }
        }
        ++iedge;
    }

    if (iedge != num_edges()) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "RR Node Edge iteration does not match edge size");
    }

    return true;
}
