#include "rr_node.h"
#include "globals.h"

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

/*
	Note: assuming that type -> width is 1, see assertion in rr_graph.c
*/
short t_rr_node::xhigh() const {
	if (type() == CHANX) {
		return xlow() + length();
	} else {
		return xlow();
	}
}

short t_rr_node::yhigh() const {
	if (type() != CHANX && type() != INTRA_CLUSTER_EDGE) {
		return ylow() + length();
	} else {
		return ylow();
	}
}

short t_rr_node::ptc_num() const {
	return ptc_num_;
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
	return direction_;
}

short t_rr_node::length() const {
	return length_;
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
	} else {
		xlow_ = x2;
	}

	if (y1 < y2) {
		ylow_ = y1;
	} else {
		ylow_ = y2;
	}
	
	if (y1 == y2) {
		length_ = abs(x2 - x1);
	} else {
		length_ = abs(y2 - y1);
	}
}

void t_rr_node::set_ptc_num(short new_ptc_num) {
	ptc_num_ = new_ptc_num;
}

void t_rr_node::set_cost_index(short new_cost_index) {
	cost_index_ = new_cost_index;
}

void t_rr_node::set_capacity(short new_capacity) {
	capacity_ = new_capacity;
}

void t_rr_node::set_fan_in(short new_fan_in) {
	fan_in_ = new_fan_in;
}

void t_rr_node::set_num_edges(short new_num_edges) {
    num_edges_ = new_num_edges;

    edge_sink_nodes_.reset(new int[num_edges_]);
    edge_switches_.reset(new short[num_edges_]);
}

void t_rr_node::set_direction(e_direction new_direction) {
	direction_ = new_direction;
}

void t_rr_node::set_R(float new_R) { 
    R_ = new_R; 
}

void t_rr_node::set_C(float new_C) { 
    C_ = new_C; 
}

void t_rr_node::set_edge_sink_node(short iedge, int sink_node) { 
    VTR_ASSERT(iedge < num_edges());
    edge_sink_nodes_[iedge] = sink_node; 
}

void t_rr_node::set_edge_switch(short iedge, short switch_index) { 
    VTR_ASSERT(iedge < num_edges());
    edge_switches_[iedge] = switch_index; 
}
