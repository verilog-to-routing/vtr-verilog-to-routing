#include "types.h"

nnode_t *make_not_gate_with_input(npin_t *input_pin, nnode_t *node, short mark);
nnode_t *make_1port_logic_gate_with_inputs(operation_list type, int width, signal_list_t *pin_list, nnode_t *node, short mark);
nnode_t *make_2port_logic_gates_with_inputs(operation_list type, int width_port1, signal_list_t *pin_list1, int width_port2, signal_list_t *pin_list2, nnode_t *node, short mark);

nnode_t *make_not_gate(nnode_t *node, short mark);
nnode_t *make_1port_logic_gate(operation_list type, int width, nnode_t *node, short mark);

nnode_t *make_1port_gate(operation_list type, int width_input, int width_output, nnode_t *node, short mark);
nnode_t *make_2port_gate(operation_list type, int width_port1, int width_port2, int width_output, nnode_t *node, short mark);
nnode_t *make_3port_gate(operation_list type, int width_port1, int width_port2, int width_port3, int width_output, nnode_t *node, short mark);

npin_t *get_zero_pin();
npin_t *get_one_pin();


char *node_name_based_on_op(nnode_t *node);
char *node_name(nnode_t *node, char *instance_prefix_name);
char *hard_node_name(nnode_t *node, char *instance_name_prefix, char *hb_name, char *hb_inst);
nnode_t *make_mult_block(nnode_t *node, short mark);
