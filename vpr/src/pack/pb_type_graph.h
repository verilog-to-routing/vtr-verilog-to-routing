#ifndef PB_TYPE_GRAPH_H
#define PB_TYPE_GRAPH_H

struct t_pb_graph_edge_comparator {
    int input_pin_id_in_cluster;
    int output_pin_id_in_cluster;
    t_pb_graph_pin* input_pin;
    t_pb_graph_pin* output_pin;
    t_pb_graph_edge* parent_edge;
};

// Find the edge between driver_pin and pin.
//
// Returns nullptr on invalid input or error.
const t_pb_graph_edge* get_edge_between_pins(
    const t_pb_graph_pin* driver_pin,
    const t_pb_graph_pin* pin);

void alloc_and_load_all_pb_graphs(bool load_power_structures);
void echo_pb_graph(char* filename);
void free_pb_graph_edges();
t_pb_graph_pin*** alloc_and_load_port_pin_ptrs_from_string(const int line_num,
                                                           const t_pb_graph_node* pb_graph_parent_node,
                                                           t_pb_graph_node** pb_graph_children_nodes,
                                                           const char* port_string,
                                                           int** num_ptrs,
                                                           int* num_sets,
                                                           const bool is_input_to_interc,
                                                           const bool interconnect_error_check);
#endif
