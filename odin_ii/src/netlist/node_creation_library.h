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

#ifndef NODE_CREATION_LIBRARY_H
#define NODE_CREATION_LIBRARY_H

#include "odin_types.h"

nnode_t* make_not_gate_with_input(npin_t* input_pin, nnode_t* node, short mark);
nnode_t* make_1port_logic_gate_with_inputs(operation_list type,
                                           int width,
                                           signal_list_t* pin_list,
                                           nnode_t* node,
                                           short mark);

nnode_t* make_not_gate(nnode_t* node, short mark);
nnode_t* make_inverter(npin_t* pin, nnode_t* node, short mark);
nnode_t* make_1port_logic_gate(operation_list type, int width, nnode_t* node, short mark);

nnode_t* make_1port_gate(operation_list type, int width_input, int width_output, nnode_t* node, short mark);
nnode_t* make_2port_gate(operation_list type,
                         int width_port1,
                         int width_port2,
                         int width_output,
                         nnode_t* node,
                         short mark);
nnode_t* make_3port_gate(operation_list type,
                         int width_port1,
                         int width_port2,
                         int width_port3,
                         int width_output,
                         nnode_t* node,
                         short mark);
nnode_t* make_nport_gate(operation_list type, int port_sizes, int width, int width_output, nnode_t* node, short mark);

npin_t* get_zero_pin();

char* node_name(nnode_t* node, char* instance_prefix_name);
char* op_node_name(operation_list op, char* instance_prefix_name);
char* hard_node_name(nnode_t* node, char* instance_name_prefix, char* hb_name, char* hb_inst);

edge_type_e edge_type_blif_enum(std::string edge_kind_str, loc_t loc);
const char* edge_type_blif_str(edge_type_e edge_type, loc_t loc);

extern nnode_t* make_multiport_smux(signal_list_t** inputs,
                                    signal_list_t* selector,
                                    int num_muxed_inputs,
                                    signal_list_t* outs,
                                    nnode_t* node,
                                    netlist_t* netlist);
#endif
