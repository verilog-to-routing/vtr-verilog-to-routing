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
#ifndef _NODE_UTILS_H_
#define _NODE_UTILS_H_

#include "odin_types.h"

nnode_t *make_not_gate_with_input(npin_t *input_pin, nnode_t *node, short mark);

nnode_t *make_not_gate(nnode_t *node, short mark);
nnode_t *make_inverter(npin_t *pin, nnode_t *node, short mark);
nnode_t *make_1port_logic_gate(operation_list type, int width, nnode_t *node, short mark);

nnode_t *make_1port_gate(operation_list type, int width_input, int width_output, nnode_t *node, short mark);
nnode_t *make_2port_gate(operation_list type, int width_port1, int width_port2, int width_output, nnode_t *node, short mark);
nnode_t *make_3port_gate(operation_list type, int width_port1, int width_port2, int width_port3, int width_output, nnode_t *node, short mark);
nnode_t *make_nport_gate(operation_list type, int port_sizes, int width, int width_output, nnode_t *node, short mark);

char *node_name(nnode_t *node, char *instance_prefix_name);
char *op_node_name(operation_list op, char *instance_prefix_name);

const char *edge_type_blif_str(edge_type_e edge_type, loc_t loc);

extern nnode_t *make_multiport_smux(signal_list_t **inputs, signal_list_t *selector, int num_muxed_inputs, signal_list_t *outs, nnode_t *node,
                                    netlist_t *netlist);
#endif // _NODE_UTILS_H_
