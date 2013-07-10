#ifndef PB_TYPE_GRAPH_H
#define PB_TYPE_GRAPH_H

typedef struct s_pb_graph_edge_comparator{
	int input_pin_id_in_cluster;
	int output_pin_id_in_cluster;
	s_pb_graph_pin* input_pin;
	s_pb_graph_pin* output_pin;
	s_pb_graph_edge* parent_edge;
}t_pb_graph_edge_comparator;

void alloc_and_load_all_pb_graphs(boolean load_power_structures);
void echo_pb_graph(char * filename);
void free_all_pb_graph_nodes(void);
t_pb_graph_pin *** alloc_and_load_port_pin_ptrs_from_string(INP int line_num,
		INP const t_pb_graph_node *pb_graph_parent_node,
		INP t_pb_graph_node **pb_graph_children_nodes,
		INP const char * port_string,
		OUTP int ** num_ptrs,
		OUTP int * num_sets,
		INP boolean is_input_to_interc,
		INP boolean interconnect_error_check);
#endif

