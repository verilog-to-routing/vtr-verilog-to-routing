#pragma once
/**
 * Jason Luu
 * April 15, 2011
 * pb_type_graph_annotations loads statistical information onto the different nodes/edges of a pb_type_graph.  These statistical information include delays, capacitance, etc.
 */

class t_pb_graph_node;

void load_pb_graph_pin_to_pin_annotations(t_pb_graph_node* pb_graph_node);
