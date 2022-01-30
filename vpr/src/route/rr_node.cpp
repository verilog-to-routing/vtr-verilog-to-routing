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
/* TODO: This API should be reworked once rr_switch APIs are in RRGraphView. */
bool t_rr_node::edge_is_configurable(t_edge_size iedge) const {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto iswitch = rr_graph.edge_switch(id_, iedge);
    return rr_graph.rr_switch_inf(RRSwitchId(iswitch)).configurable();
}

bool t_rr_node::validate() const {
    //Check internal assumptions about RR node are valid

    t_edge_size iedge = 0;
    for (auto edge : edges()) {
        if (edge < num_configurable_edges()) {
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

void t_rr_node::set_cost_index(RRIndexedDataId new_cost_index) {
    storage_->set_node_cost_index(id_, new_cost_index);
}

void t_rr_node::set_rc_index(short new_rc_index) {
    storage_->set_node_rc_index(id_, new_rc_index);
}

void t_rr_node::add_side(e_side new_side) {
    storage_->add_node_side(id_, new_side);
}

void t_rr_node::set_node_crosstalk_add_n_node(RRNodeId neighbour, float v){
    storage_->set_node_crosstalk_add_n_node(id_,neighbour,v);
}

std::map<RRNodeId,float> t_rr_node::get_node_crosstalk_n() const {
    return storage_->get_node_crosstalk_n(id_);
}
