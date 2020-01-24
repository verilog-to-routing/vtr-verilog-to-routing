#include "rr_node.h"
#include "rr_node_storage.h"
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
    auto& node = storage_->get(id_);

    if (node.num_edges_ > node.edges_capacity_) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "RR Node number of edges exceeded edge capacity");
    }

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
    auto& node = storage_->get(id_);
    node.ptc_.pin_num = new_ptc_num; //TODO: eventually remove
}

void t_rr_node::set_pin_num(short new_pin_num) {
    if (type() != IPIN && type() != OPIN) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Attempted to set RR node 'pin_num' for non-IPIN/OPIN type '%s'", type_string());
    }
    auto& node = storage_->get(id_);
    node.ptc_.pin_num = new_pin_num;
}

void t_rr_node::set_track_num(short new_track_num) {
    if (type() != CHANX && type() != CHANY) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Attempted to set RR node 'track_num' for non-CHANX/CHANY type '%s'", type_string());
    }
    auto& node = storage_->get(id_);
    node.ptc_.track_num = new_track_num;
}

void t_rr_node::set_class_num(short new_class_num) {
    if (type() != SOURCE && type() != SINK) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Attempted to set RR node 'class_num' for non-SOURCE/SINK type '%s'", type_string());
    }
    auto& node = storage_->get(id_);
    node.ptc_.class_num = new_class_num;
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

void t_rr_node::set_fan_in(t_edge_size new_fan_in) {
    auto& node = storage_->get(id_);
    node.fan_in_ = new_fan_in;
}

t_edge_size t_rr_node::add_edge(int sink_node, int iswitch) {
    auto& node = storage_->get(id_);
    if (node.edges_capacity_ == node.num_edges_) {
        constexpr size_t MAX_EDGE_COUNT = std::numeric_limits<decltype(t_rr_node_data::edges_capacity_)>::max();
        if (node.edges_capacity_ == MAX_EDGE_COUNT) {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Maximum RR Node out-edge count (%zu) exceeded", MAX_EDGE_COUNT);
        }

        //Grow
        size_t new_edges_capacity = std::max<size_t>(1, 2 * node.edges_capacity_);
        new_edges_capacity = std::min(new_edges_capacity, MAX_EDGE_COUNT); //Clip to maximum count
        auto new_edges = std::make_unique<t_rr_node_data::t_rr_edge[]>(new_edges_capacity);

        //Copy
        std::copy_n(node.edges_.get(), node.num_edges_, new_edges.get());

        //Replace
        node.edges_ = std::move(new_edges);
        node.edges_capacity_ = new_edges_capacity;
    }

    VTR_ASSERT(node.num_edges_ < node.edges_capacity_);

    node.edges_[node.num_edges_].sink_node = sink_node;
    node.edges_[node.num_edges_].switch_id = iswitch;

    ++node.num_edges_;

    return node.num_edges_;
}

void t_rr_node::shrink_to_fit() {
    //Shrink
    auto& node = storage_->get(id_);
    auto new_edges = std::make_unique<t_rr_node_data::t_rr_edge[]>(node.num_edges_);

    //Copy
    std::copy_n(node.edges_.get(), node.num_edges_, new_edges.get());

    //Replace
    node.edges_ = std::move(new_edges);
    node.edges_capacity_ = node.num_edges_;
}

void t_rr_node::partition_edges() {
    auto& device_ctx = g_vpr_ctx.device();
    auto is_configurable = [&](const t_rr_node_data::t_rr_edge& edge) {
        auto iswitch = edge.switch_id;
        return device_ctx.rr_switch_inf[iswitch].configurable();
    };

    //Partition the edges so the first set of edges are all configurable, and the later are not
    auto& node = storage_->get(id_);
    auto first_non_config_edge = std::partition(node.edges_.get(), node.edges_.get() + node.num_edges_, is_configurable);

    size_t num_conf_edges = std::distance(node.edges_.get(), first_non_config_edge);
    size_t num_non_conf_edges = num_edges() - num_conf_edges; //Note we calculate using the size_t to get full range

    //Check that within allowable range (no overflow when stored as num_non_configurable_edges_
    if (num_non_conf_edges > std::numeric_limits<decltype(node.num_non_configurable_edges_)>::max()) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Exceeded RR node maximum number of non-configurable edges");
    }
    node.num_non_configurable_edges_ = num_non_conf_edges; //Narrowing
}

void t_rr_node::set_num_edges(size_t new_num_edges) {
    auto& node = storage_->get(id_);
    VTR_ASSERT(new_num_edges <= std::numeric_limits<t_edge_size>::max());
    node.num_edges_ = new_num_edges;
    node.edges_capacity_ = new_num_edges;

    node.edges_ = std::make_unique<t_rr_node_data::t_rr_edge[]>(node.num_edges_);
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

void t_rr_node::set_edge_sink_node(t_edge_size iedge, int sink_node) {
    auto& node = storage_->get(id_);
    VTR_ASSERT(iedge < num_edges());
    VTR_ASSERT(sink_node >= 0);
    node.edges_[iedge].sink_node = sink_node;
}

void t_rr_node::set_edge_switch(t_edge_size iedge, short switch_index) {
    auto& node = storage_->get(id_);
    VTR_ASSERT(iedge < num_edges());
    VTR_ASSERT(switch_index >= 0);
    node.edges_[iedge].switch_id = switch_index;
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
