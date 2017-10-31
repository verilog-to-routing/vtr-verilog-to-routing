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

void t_rr_node::set_capacity(short new_capacity) {
    VTR_ASSERT(new_capacity >= 0);
	capacity_ = new_capacity;
}

void t_rr_node::set_fan_in(short new_fan_in) {
    VTR_ASSERT(new_fan_in >= 0);
	fan_in_ = new_fan_in;
}

void t_rr_node::set_num_edges(short new_num_edges) {
    VTR_ASSERT(new_num_edges >= 0);
    num_edges_ = new_num_edges;

    edge_sink_nodes_ = std::make_unique<int[]>(num_edges_);
    edge_switches_ = std::make_unique<short[]>(num_edges_);
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

void t_rr_node::set_R(float new_R) { 
    R_ = new_R; 
}

void t_rr_node::set_C(float new_C) { 
    C_ = new_C; 
}

void t_rr_node::set_edge_sink_node(short iedge, int sink_node) { 
    VTR_ASSERT(iedge < num_edges());
    VTR_ASSERT(sink_node >= 0);
    edge_sink_nodes_[iedge] = sink_node; 
}

void t_rr_node::set_edge_switch(short iedge, short switch_index) { 
    VTR_ASSERT(iedge < num_edges());
    VTR_ASSERT(switch_index >= 0);
    edge_switches_[iedge] = switch_index; 
}
