/*
 * Copyright 2023 CAS—Atlantic (University of New Brunswick, CASA)
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef NETLIST_UTILS_H_FUNCTIONS
#define NETLIST_UTILS_H_FUNCTIONS

#include "odin_types.h"

// PROTOTYPES
//
// how to void remap_input_pin_to_new_node(nnode_t *from_node, int from_node_pin, nnode_t *to_node, int to_node_pin)
//	remap_int_to_new_node(from_node->input_pins[from_node_pin], to_node, to_node_pin);
//
nnode_t* allocate_nnode(loc_t loc);
npin_t* allocate_npin();
nnet_t* allocate_nnet();

nnode_t* free_nnode(nnode_t* to_free);
npin_t* free_npin(npin_t* to_free);
nnet_t* free_nnet(nnet_t* to_free);

npin_t* get_zero_pin(netlist_t* netlist);
npin_t* get_pad_pin(netlist_t* netlist);
npin_t* get_one_pin(netlist_t* netlist);
npin_t* copy_input_npin(npin_t* copy_pin);
npin_t* copy_output_npin(npin_t* copy_pin);

void allocate_more_input_pins(nnode_t* node, int width);
void allocate_more_output_pins(nnode_t* node, int width);

void move_input_pin(nnode_t* node, int old_idx, int new_idx);
void move_output_pin(nnode_t* node, int old_idx, int new_idx);
void add_input_pin_to_node(nnode_t* node, npin_t* pin, int pin_idx);
void add_fanout_pin_to_net(nnet_t* net, npin_t* pin);
void add_output_pin_to_node(nnode_t* node, npin_t* pin, int pin_idx);
void add_driver_pin_to_net(nnet_t* net, npin_t* pin);
void add_output_port_information(nnode_t* node, int port_width);
void add_input_port_information(nnode_t* node, int port_width);

void combine_nets(nnet_t* output_net, nnet_t* input_net, netlist_t* netlist);
void combine_nets_with_spot_copy(nnet_t* output_net, nnet_t* input_net, long sc_spot_output, netlist_t* netlist);
void join_nets(nnet_t* net, nnet_t* input_net);
void integrate_nets(char* alias_name, char* full_name, nnet_t* input_signal_net);

void remap_pin_to_new_node(npin_t* pin, nnode_t* new_node, int pin_idx);

attr_t* init_attribute();
void free_attribute(attr_t* attribute);

signal_list_t* init_signal_list();
extern bool is_constant_signal(signal_list_t* signal, netlist_t* netlist);
extern long constant_signal_value(signal_list_t* signal, netlist_t* netlist);
extern signal_list_t** split_signal_list(signal_list_t* signalsvar, const int width);
extern bool sigcmp(signal_list_t* sig, signal_list_t* be_checked);
void add_pin_to_signal_list(signal_list_t* list, npin_t* pin);
void sort_signal_list_alphabetically(signal_list_t* list);
signal_list_t* combine_lists(signal_list_t** signal_lists, int num_signal_lists);
signal_list_t* combine_lists_without_freeing_originals(signal_list_t** signal_lists, int num_signal_lists);
signal_list_t* copy_input_signals(signal_list_t* signalsvar);
signal_list_t* copy_output_signals(signal_list_t* signalsvar);
void free_signal_list(signal_list_t* list);

void hookup_hb_input_pins_from_signal_list(nnode_t* node,
                                           int n_start_idx,
                                           signal_list_t* input_list,
                                           int il_start_idx,
                                           int width,
                                           netlist_t* netlist);
void hookup_input_pins_from_signal_list(nnode_t* node,
                                        int n_start_idx,
                                        signal_list_t* input_list,
                                        int il_start_idx,
                                        int width,
                                        netlist_t* netlist);
void hookup_output_pins_from_signal_list(nnode_t* node,
                                         int n_start_idx,
                                         signal_list_t* output_list,
                                         int ol_start_idx,
                                         int width);

signal_list_t* make_output_pins_for_existing_node(nnode_t* node, int width);
void connect_nodes(nnode_t* out_node, int out_idx, nnode_t* in_node, int in_idx);

netlist_t* allocate_netlist();
void free_netlist(netlist_t* to_free);

int get_output_pin_index_from_mapping(nnode_t* node, const char* name);
int get_output_port_index_from_mapping(nnode_t* node, const char* name);
int get_input_pin_index_from_mapping(nnode_t* node, const char* name);
int get_input_port_index_from_mapping(nnode_t* node, const char* name);
chain_information_t* allocate_chain_info();
void remove_fanout_pins_from_net(nnet_t* net, npin_t* pin, int id);

extern void delete_npin(npin_t* pin);

#endif
