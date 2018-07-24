#include "rr_node.h"
#include "globals.h"
#include "vpr_error.h"

/* Member function of "t_rr_node" used to retrieve a routing *
 * resource type string by its index, which is defined by           *
 * "t_rr_type type".												*/

const char *t_rr_node::type_string() const {
	return rr_node_typename[type()];
}

short t_rr_node::xlow() const {
	return xlow_;
}

short t_rr_node::ylow() const {
	return ylow_;
}

short t_rr_node::xhigh() const {
    return xhigh_;
}

short t_rr_node::yhigh() const {
    return yhigh_;
}

short t_rr_node::ptc_num() const {
	return ptc_.pin_num; //TODO eventually remove
}

short t_rr_node::pin_num() const {
    if (type() != IPIN && type() != OPIN) {
        VPR_THROW(VPR_ERROR_ROUTE, "Attempted to access RR node 'pin_num' for non-IPIN/OPIN type '%s'", type_string());
    }
    return ptc_.pin_num;
}

short t_rr_node::track_num() const {
    if (type() != CHANX && type() != CHANY) {
        VPR_THROW(VPR_ERROR_ROUTE, "Attempted to access RR node 'track_num' for non-CHANX/CHANY type '%s'", type_string());
    }
    return ptc_.track_num;
}

short t_rr_node::class_num() const {
    if (type() != SOURCE && type() != SINK) {
        VPR_THROW(VPR_ERROR_ROUTE, "Attempted to access RR node 'class_num' for non-SOURCE/SINK type '%s'", type_string());
    }
    return ptc_.class_num;
}

short t_rr_node::cost_index() const {
	return cost_index_;
}

short t_rr_node::rc_index() const {
	return rc_index_;
}

short t_rr_node::capacity() const {
	return capacity_;
}

short t_rr_node::fan_in() const {
	return fan_in_;
}

e_direction t_rr_node::direction() const {
    if (type() != CHANX && type() != CHANY) {
        VPR_THROW(VPR_ERROR_ROUTE, "Attempted to access RR node 'direction' for non-channel type '%s'", type_string());
    }
	return dir_side_.direction;
}

const char* t_rr_node::direction_string() const{
	if (direction() == INC_DIRECTION){
		return "INC_DIR";
	} else if (direction() == DEC_DIRECTION){
		return "DEC_DIR";
	} else if (direction() == BI_DIRECTION){
		return "BI_DIR";
    }

	VTR_ASSERT(direction() == NO_DIRECTION);
    return "NO_DIR";
}

e_side t_rr_node::side() const {
    if (type() != IPIN && type() != OPIN) {
        VPR_THROW(VPR_ERROR_ROUTE, "Attempted to access RR node 'side' for non-IPIN/OPIN type '%s'", type_string());
    }
	return dir_side_.side;
}

const char* t_rr_node::side_string() const {
    return SIDE_STRING[side()];
}


//Returns the max 'length' over the x or y direction
short t_rr_node::length() const {
	return std::max(yhigh_ - ylow_, xhigh_ - xlow_);
}

bool t_rr_node::found_at(int x, int y) const {
    switch (type_) {
    case CHANX: {
        VTR_ASSERT(ylow_ == yhigh_);

        return ((ylow_ == y) && ((x >= xlow_) && (x <= xhigh_)));
    }
    case CHANY: {
        VTR_ASSERT(xlow_ == xhigh_);

        return ((xlow_ == x) && ((y >= ylow_) && (y <= yhigh_)));
    }
    default:
        return ((x == xlow_) && (y == ylow_));
    }
}

static std::pair<int, int> overlap(short low_a, short high_a, short low_b, short high_b) {
    auto overlap = std::make_pair(std::max(low_a, low_b), std::min(high_a, high_b));
    if (overlap.first <= overlap.second) {
        return overlap;
    } else {
        return std::make_pair(-1, -1);
    }
}

std::pair<t_offset, t_offset> t_rr_node::overlap(t_rr_node& other) const {
    auto overlap_x = ::overlap(xlow(), xhigh(), other.xlow(), other.xhigh());
    auto overlap_y = ::overlap(ylow(), yhigh(), other.ylow(), other.yhigh());

    t_offset low;
    t_offset high;
    low.x = overlap_x.first;
    low.y = overlap_y.first;
    high.x = overlap_x.second;
    high.y = overlap_y.second;
    return std::make_pair(low, high);
}

short t_rr_node::get_iedge(int sink_node, short switch_id) const {
    for (auto e : edges()) {
        if (sink_node == edges_[e].sink_node && switch_id == edges_[e].switch_id) {
            return e;
        }
    }
    VTR_ASSERT(false);
    return -1;
}

bool t_rr_node::edge_is_configurable(short iedge) const {
    auto iswitch =  edge_switch(iedge);

    auto& device_ctx = g_vpr_ctx.device();

    return device_ctx.rr_switch_inf[iswitch].configurable();
}

t_metadata_as* t_rr_node::metadata(std::string key) const {
    return metadata(std::make_pair(t_offset(), key));
}
t_metadata_as* t_rr_node::metadata(t_offset o, std::string key) const {
    return metadata(std::make_pair(o, key));
}
t_metadata_as* t_rr_node::metadata(std::pair<t_offset, std::string> ok) const {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    if (device_ctx.rr_node_metadata.size() == 0 || device_ctx.rr_node_metadata.count(this) == 0) {
        return nullptr;
    }
    auto& data = device_ctx.rr_node_metadata.at(this);
    return data.one(ok);
}

void t_rr_node::add_metadata(std::string key, std::string value) {
    add_metadata(std::make_pair(t_offset(), key), value);
}
void t_rr_node::add_metadata(t_offset o, std::string key, std::string value) {
    add_metadata(std::make_pair(o, key), value);
}
void t_rr_node::add_metadata(std::pair<t_offset, std::string> ok, std::string value) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    if (device_ctx.rr_node_metadata.count(this) == 0) {
        device_ctx.rr_node_metadata.emplace(this, t_metadata_dict());
    }
    auto& data = device_ctx.rr_node_metadata.at(this);
    data.add(ok, value);
}

t_metadata_as* t_rr_node::edge_metadata(int sink_id, short switch_id, std::string key) const {
    return edge_metadata(sink_id, switch_id, std::make_pair(t_offset(), key));
}
t_metadata_as* t_rr_node::edge_metadata(int sink_id, short switch_id, t_offset o, std::string key) const {
    return edge_metadata(sink_id, switch_id, std::make_pair(o, key));
}
t_metadata_as* t_rr_node::edge_metadata(int sink_id, short switch_id, std::pair<t_offset, std::string> ok) const {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    std::pair<int, short> rr_edge = std::make_pair(sink_id, switch_id);

    if (device_ctx.rr_edge_metadata.count(this) == 0) {
        return nullptr;
    }
    auto& edata = device_ctx.rr_edge_metadata.at(this);

    if (edata.count(rr_edge) == 0) {
        return nullptr;
    }
    auto& data = edata[rr_edge];
    return data.one(ok);
}
void t_rr_node::add_edge_metadata(int sink_id, short switch_id, std::string key, std::string value) {
    return add_edge_metadata(sink_id, switch_id, std::make_pair(t_offset(), key), value);
}
void t_rr_node::add_edge_metadata(int sink_id, short switch_id, t_offset o, std::string key, std::string value) {
    return add_edge_metadata(sink_id, switch_id, std::make_pair(o, key), value);
}
void t_rr_node::add_edge_metadata(int sink_id, short switch_id, std::pair<t_offset, std::string> ok, std::string value) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    std::pair<int, short> rr_edge = std::make_pair(sink_id, switch_id);

    if (device_ctx.rr_edge_metadata.count(this) == 0) {
      device_ctx.rr_edge_metadata.emplace(this, std::map<std::pair<int, short>, t_metadata_dict>());
    }
    auto& edata = device_ctx.rr_edge_metadata.at(this);
    if (edata.count(rr_edge) == 0) {
        edata.emplace(rr_edge, t_metadata_dict());
    }
    auto& data = edata[rr_edge];
    data.add(ok, value);
}

float t_rr_node::R() const {
    auto& device_ctx = g_vpr_ctx.device();
    return device_ctx.rr_rc_data[rc_index()].R;
}

float t_rr_node::C() const {
    auto& device_ctx = g_vpr_ctx.device();
    return device_ctx.rr_rc_data[rc_index()].C;
}

void t_rr_node::set_type(t_rr_type new_type) {
    type_ = new_type;
}

/*
	Pass in two coordinate variables describing location of node.
	They do not have to be in any particular order.
*/
void t_rr_node::set_coordinates(short x1, short y1, short x2, short y2) {
	if (x1 < x2) {
		xlow_ = x1;
        xhigh_ = x2;
	} else {
		xlow_ = x2;
        xhigh_ = x1;
	}

	if (y1 < y2) {
		ylow_ = y1;
        yhigh_ = y2;
	} else {
		ylow_ = y2;
        yhigh_ = y1;
	}
}

void t_rr_node::set_ptc_num(short new_ptc_num) {
	ptc_.pin_num = new_ptc_num; //TODO: eventually remove
}

void t_rr_node::set_pin_num(short new_pin_num) {
    if (type() != IPIN && type() != OPIN) {
        VPR_THROW(VPR_ERROR_ROUTE, "Attempted to set RR node 'pin_num' for non-IPIN/OPIN type '%s'", type_string());
    }
    ptc_.pin_num = new_pin_num;
}

void t_rr_node::set_track_num(short new_track_num) {
    if (type() != CHANX && type() != CHANY) {
        VPR_THROW(VPR_ERROR_ROUTE, "Attempted to set RR node 'track_num' for non-CHANX/CHANY type '%s'", type_string());
    }
    ptc_.track_num = new_track_num;
}

void t_rr_node::set_class_num(short new_class_num) {
    if (type() != SOURCE && type() != SINK) {
        VPR_THROW(VPR_ERROR_ROUTE, "Attempted to set RR node 'class_num' for non-SOURCE/SINK type '%s'", type_string());
    }
    ptc_.class_num = new_class_num;
}

void t_rr_node::set_cost_index(short new_cost_index) {
	cost_index_ = new_cost_index;
}
void t_rr_node::set_rc_index(short new_rc_index) {
	rc_index_ = new_rc_index;
}

void t_rr_node::set_capacity(short new_capacity) {
    VTR_ASSERT(new_capacity >= 0);
	capacity_ = new_capacity;
}

void t_rr_node::set_fan_in(short new_fan_in) {
    VTR_ASSERT(new_fan_in >= 0);
	fan_in_ = new_fan_in;
}

short t_rr_node::add_edge(int sink_node, int iswitch) {
    auto new_edges = std::make_unique<t_rr_edge[]>(num_edges_ + 1);
    std::copy_n(edges_.get(), num_edges_, new_edges.get());

    new_edges[num_edges_].sink_node = sink_node;
    new_edges[num_edges_].switch_id = iswitch;

    edges_ = std::move(new_edges);
    ++num_edges_;

    return num_edges_;
}

void t_rr_node::partition_edges() {
    auto& device_ctx = g_vpr_ctx.device();
    auto is_configurable = [&](const t_rr_edge& edge) {
        auto iswitch =  edge.switch_id;
        return device_ctx.rr_switch_inf[iswitch].configurable();
    };

    //Partition the edges so the first set of edges are all configurable, and the later are not
    auto first_non_config_edge = std::partition(edges_.get(), edges_.get() + num_edges_, is_configurable);

    num_configurable_edges_ = std::distance(edges_.get(), first_non_config_edge);
}

void t_rr_node::set_num_edges(short new_num_edges) {
    VTR_ASSERT(new_num_edges >= 0);
    num_edges_ = new_num_edges;

    edges_ = std::make_unique<t_rr_edge[]>(num_edges_);
}

void t_rr_node::set_direction(e_direction new_direction) {
    if (type() != CHANX && type() != CHANY) {
        VPR_THROW(VPR_ERROR_ROUTE, "Attempted to set RR node 'direction' for non-channel type '%s'", type_string());
    }
	dir_side_.direction = new_direction;
}

void t_rr_node::set_side(e_side new_side) {
    if (type() != IPIN && type() != OPIN) {
        VPR_THROW(VPR_ERROR_ROUTE, "Attempted to set RR node 'side' for non-channel type '%s'", type_string());
    }
	dir_side_.side = new_side;
}

void t_rr_node::set_edge_sink_node(short iedge, int sink_node) {
    VTR_ASSERT(iedge < num_edges());
    VTR_ASSERT(sink_node >= 0);
    edges_[iedge].sink_node = sink_node;
}

void t_rr_node::set_edge_switch(short iedge, short switch_index) {
    VTR_ASSERT(iedge < num_edges());
    VTR_ASSERT(switch_index >= 0);
    edges_[iedge].switch_id = switch_index;
}

t_rr_rc_data::t_rr_rc_data(float Rval, float Cval)
    : R(Rval)
    , C(Cval)
    {}


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
