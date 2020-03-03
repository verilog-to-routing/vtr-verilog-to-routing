#include "rr_node.h"
#include "rr_graph_storage.h"
#include "globals.h"
#include "vpr_error.h"

/* Member function of "t_rr_node" used to retrieve a routing *
 * resource type string by its index, which is defined by           *
 * "t_rr_type type".												*/

const char* t_rr_node::type_string() const {
    return rr_node_typename[type()];
}

const char* t_rr_node::direction_string() const {
    if (direction() == INC_DIRECTION) {
        return "INC_DIR";
    } else if (direction() == DEC_DIRECTION) {
        return "DEC_DIR";
    } else if (direction() == BI_DIRECTION) {
        return "BI_DIR";
    }

    VTR_ASSERT(direction() == NO_DIRECTION);
    return "NO_DIR";
}

const char* t_rr_node::side_string() const {
    return SIDE_STRING[side()];
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

float t_rr_node::R() const {
    auto& device_ctx = g_vpr_ctx.device();
    return device_ctx.rr_rc_data[rc_index()].R;
}

float t_rr_node::C() const {
    auto& device_ctx = g_vpr_ctx.device();
    VTR_ASSERT(rc_index() < (short)device_ctx.rr_rc_data.size());
    return device_ctx.rr_rc_data[rc_index()].C;
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

void t_rr_node::set_type(t_rr_type new_type) {
    storage_->set_node_type(id_, new_type);
}

/*
 * Pass in two coordinate variables describing location of node.
 * They do not have to be in any particular order.
 */
void t_rr_node::set_coordinates(short x1, short y1, short x2, short y2) {
    storage_->set_node_coordinates(id_, x1, y1, x2, y2);
}

void t_rr_node::set_ptc_num(short new_ptc_num) {
    storage_->set_node_ptc_num(id_, new_ptc_num);
}

void t_rr_node::set_pin_num(short new_pin_num) {
    storage_->set_node_pin_num(id_, new_pin_num);
}

void t_rr_node::set_track_num(short new_track_num) {
    storage_->set_node_track_num(id_, new_track_num);
}

void t_rr_node::set_class_num(short new_class_num) {
    storage_->set_node_class_num(id_, new_class_num);
}

void t_rr_node::set_cost_index(size_t new_cost_index) {
    storage_->set_node_cost_index(id_, new_cost_index);
}

void t_rr_node::set_rc_index(short new_rc_index) {
    storage_->set_node_rc_index(id_, new_rc_index);
}

void t_rr_node::set_capacity(short new_capacity) {
    storage_->set_node_capacity(id_, new_capacity);
}

void t_rr_node::set_direction(e_direction new_direction) {
    storage_->set_node_direction(id_, new_direction);
}

void t_rr_node::set_side(e_side new_side) {
    storage_->set_node_side(id_, new_side);
}
