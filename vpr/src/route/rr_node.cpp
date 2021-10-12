#include "rr_node.h"
#include "rr_graph_storage.h"
#include "globals.h"
#include "vpr_error.h"

/* Member function of "t_rr_node" used to retrieve a routing *
 * resource type string by its index, which is defined by           *
 * "t_rr_type type".												*/

const char* t_rr_node::type_string() const {
    // ESR API The contents of this function have been temporarily replaced
    // Once the RRGraphView API is complete, this function will be removed completely
    return rr_node_typename[g_vpr_ctx.device().rr_graph.node_type(id_)];
}

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
