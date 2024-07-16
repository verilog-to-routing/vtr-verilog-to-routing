/*
 * Copyright 2022 CAS—Atlantic (University of New Brunswick, CASA)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _NETLIST_UTILS_H_
#define _NETLIST_UTILS_H_

#include "odin_types.h"

nnode_t *allocate_nnode(loc_t loc);
npin_t *allocate_npin();
nnet_t *allocate_nnet();

nnode_t *free_nnode(nnode_t *to_free);
npin_t *free_npin(npin_t *to_free);
nnet_t *free_nnet(nnet_t *to_free);

npin_t *get_zero_pin(netlist_t *netlist);
npin_t *get_pad_pin(netlist_t *netlist);
npin_t *get_one_pin(netlist_t *netlist);
npin_t *copy_input_npin(npin_t *copy_pin);
npin_t *copy_output_npin(npin_t *copy_pin);

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

void join_nets(nnet_t *net, nnet_t *input_net);

void remap_pin_to_new_net(npin_t *pin, nnet_t *new_net);
void remap_pin_to_new_node(npin_t *pin, nnode_t *new_node, int pin_idx);

attr_t *init_attribute();
void copy_attribute(attr_t *to, attr_t *copy);
void copy_signedness(attr_t *to, attr_t *copy);
void free_attribute(attr_t *attribute);

signal_list_t *init_signal_list();
extern bool is_constant_signal(signal_list_t *signal, netlist_t *netlist);
extern long constant_signal_value(signal_list_t *signal, netlist_t *netlist);
extern signal_list_t *create_constant_signal(const long long value, const int desired_width, netlist_t *netlist);
extern signal_list_t *prune_signal(signal_list_t *signalsvar, long signal_width, long prune_size, int num_of_signals);
extern signal_list_t **split_signal_list(signal_list_t *signalsvar, const int width);
extern bool sigcmp(signal_list_t *sig, signal_list_t *be_checked);
void add_pin_to_signal_list(signal_list_t *list, npin_t *pin);
signal_list_t *combine_lists(signal_list_t **signal_lists, int num_signal_lists);
signal_list_t *copy_input_signals(signal_list_t *signalsvar);
void free_signal_list(signal_list_t *list);

signal_list_t *make_output_pins_for_existing_node(nnode_t *node, int width);
void connect_nodes(nnode_t *out_node, int out_idx, nnode_t *in_node, int in_idx);

netlist_t *allocate_netlist();
void free_netlist(netlist_t *to_free);

int get_output_pin_index_from_mapping(nnode_t *node, const char *name);
int get_output_port_index_from_mapping(nnode_t *node, const char *name);
int get_input_pin_index_from_mapping(nnode_t *node, const char *name);
int get_input_port_index_from_mapping(nnode_t *node, const char *name);
extern npin_t *legalize_polarity(npin_t *pin, edge_type_e pin_polarity, nnode_t *node);
extern void reduce_input_ports(nnode_t *&node, netlist_t *netlist);
extern signal_list_t *reduce_signal_list(signal_list_t *signalvar, operation_list signedness, netlist_t *netlist);
chain_information_t *allocate_chain_info();
void remove_fanout_pins_from_net(nnet_t *net, npin_t *pin, int id);

extern void equalize_ports_size(nnode_t *&node, uintptr_t traverse_mark_number, netlist_t *netlist);
extern void delete_npin(npin_t *pin);

#endif // _NETLIST_UTILS_H_
