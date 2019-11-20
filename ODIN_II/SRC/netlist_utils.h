#include "types.h"

// PROTOTYPES
//
// how to void remap_input_pin_to_new_node(nnode_t *from_node, int from_node_pin, nnode_t *to_node, int to_node_pin)
//	remap_int_to_new_node(from_node->input_pins[from_node_pin], to_node, to_node_pin);
//
nnode_t* allocate_nnode();
npin_t* allocate_npin();
void free_npin(npin_t *to_free);
npin_t *get_zero_pin(netlist_t *netlist);
npin_t *get_pad_pin(netlist_t *netlist);
npin_t *get_one_pin(netlist_t *netlist);
npin_t* copy_input_npin(npin_t* copy_pin);
npin_t* copy_output_npin(npin_t* copy_pin);
nnet_t* allocate_nnet();
void free_nnode(nnode_t *to_free);
void free_npin(npin_t *to_free);
void free_nnet(nnet_t *to_free);

void allocate_more_input_pins(nnode_t *node, int width);
void allocate_more_output_pins(nnode_t *node, int width);

void move_input_pin(nnode_t *node, int old_idx, int new_idx);
void move_output_pin(nnode_t *node, int old_idx, int new_idx);
void add_input_pin_to_node(nnode_t *node, npin_t *pin, int pin_idx);
void add_fanout_pin_to_net(nnet_t *net, npin_t *pin);
void add_output_pin_to_node(nnode_t *node, npin_t *pin, int pin_idx);
void add_driver_pin_to_net(nnet_t *net, npin_t *pin);
void add_output_port_information(nnode_t *node, int port_width);
void add_input_port_information(nnode_t *node, int port_width);

void combine_nets(nnet_t *output_net, nnet_t* input_net, netlist_t *netlist);
void join_nets(nnet_t *net, nnet_t* input_net);

void remap_pin_to_new_net(npin_t *pin, nnet_t *new_net);
void remap_pin_to_new_node(npin_t *pin, nnode_t *new_node, int pin_idx);

signal_list_t *init_signal_list();
void add_pin_to_signal_list(signal_list_t *list, npin_t* pin);
void sort_signal_list_alphabetically(signal_list_t *list);
signal_list_t *combine_lists(signal_list_t **signal_lists, int num_signal_lists);
signal_list_t *combine_lists_without_freeing_originals(signal_list_t **signal_lists, int num_signal_lists);
signal_list_t *copy_input_signals(signal_list_t *signalsvar);
signal_list_t *copy_output_signals(signal_list_t *signalsvar);
void free_signal_list(signal_list_t *list);

void hookup_hb_input_pins_from_signal_list(nnode_t *node, int n_start_idx, signal_list_t* input_list, int il_start_idx, int width, netlist_t *netlist) ;
void hookup_input_pins_from_signal_list(nnode_t *node, int n_start_idx, signal_list_t* input_list, int il_start_idx, int width, netlist_t *netlist) ;
void hookup_output_pins_from_signal_list(nnode_t *node, int n_start_idx, signal_list_t* output_list, int ol_start_idx, int width);

signal_list_t *make_output_pins_for_existing_node(nnode_t* node, int width);
void connect_nodes(nnode_t *out_node, int out_idx, nnode_t *in_node, int in_idx);

int count_nodes_in_netlist(netlist_t *netlist);

netlist_t* allocate_netlist();
void free_netlist(netlist_t *to_free);
void add_node_to_netlist(netlist_t *netlist, nnode_t *node, short special_node);
void mark_clock_node ( netlist_t *netlist, char *clock_name);

int get_output_pin_index_from_mapping(nnode_t *node, char *name);
int get_output_port_index_from_mapping(nnode_t *node, char *name);
int get_input_pin_index_from_mapping(nnode_t *node, char *name);
int get_input_port_index_from_mapping(nnode_t *node, char *name);
chain_information_t* allocate_chain_info();
