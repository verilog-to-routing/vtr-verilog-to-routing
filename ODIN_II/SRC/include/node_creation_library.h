#ifndef NODE_CREATION_LIBRARY_H
#define NODE_CREATION_LIBRARY_H

#include "odin_types.h"

nnode_t* make_not_gate_with_input(npin_t* input_pin, nnode_t* node, short mark);
nnode_t* make_1port_logic_gate_with_inputs(operation_list type, int width, signal_list_t* pin_list, nnode_t* node, short mark);
nnode_t* make_2port_logic_gates_with_inputs(operation_list type, int width_port1, signal_list_t* pin_list1, int width_port2, signal_list_t* pin_list2, nnode_t* node, short mark);

nnode_t* make_not_gate(nnode_t* node, short mark);
nnode_t* make_inverter(npin_t* pin, nnode_t* node, short mark);
nnode_t* make_1port_logic_gate(operation_list type, int width, nnode_t* node, short mark);

nnode_t* make_1port_gate(operation_list type, int width_input, int width_output, nnode_t* node, short mark);
nnode_t* make_2port_gate(operation_list type, int width_port1, int width_port2, int width_output, nnode_t* node, short mark);
nnode_t* make_3port_gate(operation_list type, int width_port1, int width_port2, int width_port3, int width_output, nnode_t* node, short mark);
nnode_t* make_nport_gate(operation_list type, int port_sizes, int width, int width_output, nnode_t* node, short mark);

npin_t* get_zero_pin();
npin_t* get_one_pin();

signal_list_t* make_chain(operation_list type, signal_list_t* inputs, nnode_t* node);

char* node_name(nnode_t* node, char* instance_prefix_name);
char* op_node_name(operation_list op, char* instance_prefix_name);
char* hard_node_name(nnode_t* node, char* instance_name_prefix, char* hb_name, char* hb_inst);
nnode_t* make_mult_block(nnode_t* node, short mark);

edge_type_e edge_type_blif_enum(std::string edge_kind_str, loc_t loc);
const char* edge_type_blif_str(edge_type_e edge_type, loc_t loc);

extern nnode_t* make_ff_node(npin_t* D, npin_t* clk, npin_t* Q, nnode_t* node, netlist_t* netlist);
extern nnode_t* smux_with_sel_polarity(npin_t* pin1, npin_t* pin2, npin_t* sel, nnode_t* node);
extern nnode_t* make_multiport_smux(signal_list_t** inputs, signal_list_t* selector, int num_muxed_inputs, signal_list_t* outs, nnode_t* node, netlist_t* netlist);
#endif
