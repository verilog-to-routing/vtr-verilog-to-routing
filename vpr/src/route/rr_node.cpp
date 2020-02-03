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
    const auto& node = storage_->get(id_);
    return std::max(node.yhigh_ - node.ylow_, node.xhigh_ - node.xlow_);
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
    auto& node = storage_->get(id_);
    node.type_ = new_type;
}

/*
 * Pass in two coordinate variables describing location of node.
 * They do not have to be in any particular order.
 */
void t_rr_node::set_coordinates(short x1, short y1, short x2, short y2) {
    auto& node = storage_->get(id_);
    if (x1 < x2) {
        node.xlow_ = x1;
        node.xhigh_ = x2;
    } else {
        node.xlow_ = x2;
        node.xhigh_ = x1;
    }

    if (y1 < y2) {
        node.ylow_ = y1;
        node.yhigh_ = y2;
    } else {
        node.ylow_ = y2;
        node.yhigh_ = y1;
    }
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
    auto& node = storage_->get(id_);
    if (new_cost_index >= std::numeric_limits<decltype(node.cost_index_)>::max()) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Attempted to set cost_index_ %zu above cost_index storage max value.",
                        new_cost_index);
    }
    node.cost_index_ = new_cost_index;
}

void t_rr_node::set_rc_index(short new_rc_index) {
    auto& node = storage_->get(id_);
    node.rc_index_ = new_rc_index;
}

void t_rr_node::set_capacity(short new_capacity) {
    VTR_ASSERT(new_capacity >= 0);
    auto& node = storage_->get(id_);
    node.capacity_ = new_capacity;
}

void t_rr_node::set_direction(e_direction new_direction) {
    if (type() != CHANX && type() != CHANY) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Attempted to set RR node 'direction' for non-channel type '%s'", type_string());
    }
    auto& node = storage_->get(id_);
    node.dir_side_.direction = new_direction;
}

void t_rr_node::set_side(e_side new_side) {
    if (type() != IPIN && type() != OPIN) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Attempted to set RR node 'side' for non-channel type '%s'", type_string());
    }
    auto& node = storage_->get(id_);
    node.dir_side_.side = new_side;
}

t_rr_rc_data::t_rr_rc_data(float Rval, float Cval) noexcept
    : R(Rval)
    , C(Cval) {}

short find_create_rr_rc_data(const float R, const float C) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    auto match = [&](const t_rr_rc_data& val) {
        return val.R == R
               && val.C == C;
    };

    //Just a linear search for now
    auto itr = std::find_if(device_ctx.rr_rc_data.begin(),
                            device_ctx.rr_rc_data.end(),
                            match);

    if (itr == device_ctx.rr_rc_data.end()) {
        //Note found -> create it
        device_ctx.rr_rc_data.emplace_back(R, C);

        itr = --device_ctx.rr_rc_data.end(); //Iterator to inserted value
    }

    return std::distance(device_ctx.rr_rc_data.begin(), itr);
}
