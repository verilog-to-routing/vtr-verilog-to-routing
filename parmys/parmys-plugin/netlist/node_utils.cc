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
#include "node_utils.h"
#include "netlist_utils.h"
#include "odin_globals.h"
#include "odin_types.h"
#include "odin_util.h"
#include "vtr_memory.h"
#include "vtr_util.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

long unique_node_name_id = 0;

/*-----------------------------------------------------------------------
 * (function: get_a_pad_pin)
 * 	this allows us to attach to the constant netlist driving hb_pad
 *---------------------------------------------------------------------*/
npin_t *get_pad_pin(netlist_t *netlist)
{
    npin_t *pad_fanout_pin = allocate_npin();
    pad_fanout_pin->name = vtr::strdup(pad_string);
    add_fanout_pin_to_net(netlist->pad_net, pad_fanout_pin);
    return pad_fanout_pin;
}

/*-----------------------------------------------------------------------
 * (function: get_a_zero_pin)
 * 	this allows us to attach to the constant netlist driving zero
 *---------------------------------------------------------------------*/
npin_t *get_zero_pin(netlist_t *netlist)
{
    npin_t *zero_fanout_pin = allocate_npin();
    zero_fanout_pin->name = vtr::strdup(zero_string);
    add_fanout_pin_to_net(netlist->zero_net, zero_fanout_pin);
    return zero_fanout_pin;
}

/*---------------------------------------------------------------------------------------------
 * (function: get_a_one_pin)
 * 	this allows us to attach to the constant netlist driving one
 *-------------------------------------------------------------------------------------------*/
npin_t *get_one_pin(netlist_t *netlist)
{
    npin_t *one_fanout_pin = allocate_npin();
    one_fanout_pin->name = vtr::strdup(one_string);
    add_fanout_pin_to_net(netlist->one_net, one_fanout_pin);
    return one_fanout_pin;
}

/*---------------------------------------------------------------------------------------------
 * (function: make_not_gate_with_input)
 * 	Creates a not gate and attaches it to the inputs
 *-------------------------------------------------------------------------------------------*/
nnode_t *make_not_gate_with_input(npin_t *input_pin, nnode_t *node, short mark)
{
    nnode_t *logic_node;

    logic_node = make_not_gate(node, mark);

    /* add the input ports as needed */
    add_input_pin_to_node(logic_node, input_pin, 0);

    return logic_node;
}

/*-------------------------------------------------------------------------
 * (function: make_not_gate)
 * 	Just make a not gate
 *-----------------------------------------------------------------------*/
nnode_t *make_not_gate(nnode_t *node, short mark)
{
    nnode_t *logic_node;

    logic_node = allocate_nnode(node->loc);
    logic_node->traverse_visited = mark;
    logic_node->type = LOGICAL_NOT;
    logic_node->name = node_name(logic_node, node->name);
    logic_node->related_ast_node = node->related_ast_node;

    allocate_more_input_pins(logic_node, 1);
    allocate_more_output_pins(logic_node, 1);

    return logic_node;
}

/**
 * -------------------------------------------------------------------------
 * (function: make_not_gate)
 * @brief Making a not gate with the given pin as input
 * and a new pin allocated as the output
 *
 * @param pin input pin
 * @param node related netlist node
 * @param mark netlist traversal number
 *
 * @return not node
 *-----------------------------------------------------------------------*/
nnode_t *make_inverter(npin_t *pin, nnode_t *node, short mark)
{
    /* validate the input pin */
    oassert(pin->type == INPUT);

    nnode_t *logic_node;

    logic_node = allocate_nnode(node->loc);
    logic_node->traverse_visited = mark;
    logic_node->type = LOGICAL_NOT;
    logic_node->name = node_name(logic_node, node->name);
    logic_node->related_ast_node = node->related_ast_node;

    allocate_more_input_pins(logic_node, 1);
    allocate_more_output_pins(logic_node, 1);

    if (pin->node)
        pin->node->input_pins[pin->pin_node_idx] = NULL;

    /* hook the pin into the not node */
    add_input_pin_to_node(logic_node, pin, 0);

    /* connecting the not_node output pin */
    npin_t *new_pin1 = allocate_npin();
    npin_t *new_pin2 = allocate_npin();
    nnet_t *new_net = allocate_nnet();
    new_net->name = make_full_ref_name(NULL, NULL, NULL, logic_node->name, 0);
    /* hook the output pin into the node */
    add_output_pin_to_node(logic_node, new_pin1, 0);
    /* hook up new pin 1 into the new net */
    add_driver_pin_to_net(new_net, new_pin1);
    /* hook up the new pin 2 to this new net */
    add_fanout_pin_to_net(new_net, new_pin2);

    return logic_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: make_1port_gate)
 * 	Make a 1 port gate with variable sizes
 *-------------------------------------------------------------------------------------------*/
nnode_t *make_1port_gate(operation_list type, int width_input, int width_output, nnode_t *node, short mark)
{
    nnode_t *logic_node;

    logic_node = allocate_nnode(node->loc);
    logic_node->traverse_visited = mark;
    logic_node->type = type;
    logic_node->name = node_name(logic_node, node->name);
    logic_node->related_ast_node = node->related_ast_node;

    /* add the input ports as needed */
    allocate_more_input_pins(logic_node, width_input);
    add_input_port_information(logic_node, width_input);
    /* add output */
    allocate_more_output_pins(logic_node, width_output);
    add_output_port_information(logic_node, width_output);

    return logic_node;
}
/*---------------------------------------------------------------------------------------------
 * (function: make_1port_logic_gate)
 * 	Make a gate with variable sized inputs and 1 output
 *-------------------------------------------------------------------------------------------*/
nnode_t *make_1port_logic_gate(operation_list type, int width, nnode_t *node, short mark)
{
    nnode_t *logic_node = make_1port_gate(type, width, 1, node, mark);

    return logic_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: make_3port_logic_gates)
 * 	Make a 3 port gate all variable port widths.
 *-------------------------------------------------------------------------------------------*/
nnode_t *make_3port_gate(operation_list type, int width_port1, int width_port2, int width_port3, int width_output, nnode_t *node, short mark)
{
    nnode_t *logic_node = allocate_nnode(node->loc);
    logic_node->traverse_visited = mark;
    logic_node->type = type;
    logic_node->name = node_name(logic_node, node->name);
    logic_node->related_ast_node = node->related_ast_node;

    /* add the input ports as needed */
    allocate_more_input_pins(logic_node, width_port1);
    add_input_port_information(logic_node, width_port1);
    allocate_more_input_pins(logic_node, width_port2);
    add_input_port_information(logic_node, width_port2);
    allocate_more_input_pins(logic_node, width_port3);
    add_input_port_information(logic_node, width_port3);
    /* add output */
    allocate_more_output_pins(logic_node, width_output);
    add_output_port_information(logic_node, width_output);

    return logic_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: make_2port_logic_gates)
 * 	Make a 2 port gate with variable sizes.  The first port will be input_pins index 0..width_port1.
 *-------------------------------------------------------------------------------------------*/
nnode_t *make_2port_gate(operation_list type, int width_port1, int width_port2, int width_output, nnode_t *node, short mark)
{
    nnode_t *logic_node = allocate_nnode(node->loc);
    logic_node->traverse_visited = mark;
    logic_node->type = type;
    logic_node->name = node_name(logic_node, node->name);
    logic_node->related_ast_node = node->related_ast_node;

    /* add the input ports as needed */
    allocate_more_input_pins(logic_node, width_port1);
    add_input_port_information(logic_node, width_port1);
    allocate_more_input_pins(logic_node, width_port2);
    add_input_port_information(logic_node, width_port2);
    /* add output */
    allocate_more_output_pins(logic_node, width_output);
    add_output_port_information(logic_node, width_output);

    return logic_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: make_nport_logic_gates)
 * 	Make a n port gate with variable sizes.  The first port will be input_pins index 0..width_port1.
 *-------------------------------------------------------------------------------------------*/
nnode_t *make_nport_gate(operation_list type, int port_sizes, int width, int width_output, nnode_t *node, short mark)
{
    nnode_t *logic_node = allocate_nnode(node->loc);
    logic_node->traverse_visited = mark;
    logic_node->type = type;
    logic_node->name = node_name(logic_node, node->name);
    logic_node->related_ast_node = node->related_ast_node;

    /* add the input ports as needed */
    for (int i = 0; i < port_sizes; i++) {
        allocate_more_input_pins(logic_node, width);
        add_input_port_information(logic_node, width);
    }
    // allocate_more_input_pins(logic_node, width_port2);
    // add_input_port_information(logic_node, width_port2);
    /* add output */
    allocate_more_output_pins(logic_node, width_output);
    add_output_port_information(logic_node, width_output);

    return logic_node;
}

const char *edge_type_blif_str(edge_type_e edge_type, loc_t loc)
{
    switch (edge_type) {
    case FALLING_EDGE_SENSITIVITY:
        return "fe";
    case RISING_EDGE_SENSITIVITY:
        return "re";
    case ACTIVE_HIGH_SENSITIVITY:
        return "ah";
    case ACTIVE_LOW_SENSITIVITY:
        return "al";
    case ASYNCHRONOUS_SENSITIVITY:
        return "as";
    default:
        error_message(NETLIST, loc, "undefined sensitivity kind for flip flop %s", edge_type_e_STR[edge_type]);

        return NULL;
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: node_name)
 * 	This creates the unique node name
 *-------------------------------------------------------------------------------------------*/
char *node_name(nnode_t *node, char *instance_name_prefix) { return op_node_name(node->type, instance_name_prefix); }

/*---------------------------------------------------------------------------------------------
 * (function: node_name)
 * 	This creates the unique node name
 *-------------------------------------------------------------------------------------------*/
char *op_node_name(operation_list op, char *instance_prefix_name)
{
    char *return_node_name;

    /* create the unique name for this node */
    return_node_name = make_full_ref_name(instance_prefix_name, NULL, NULL, name_based_on_op(op), unique_node_name_id);

    unique_node_name_id++;

    return return_node_name;
}

/**
 * (function: make_multiport_mux)
 *
 * @brief make a multiport mux with given selector and signals lists
 *
 * @param inputs list of input signal lists
 * @param selector signal list of selector pins
 * @param num_muxed_inputs num of inputs to be muxxed
 * @param outs list of outputs signals
 * @param node pointing to the src node
 * @param netlist pointer to the netlist
 *
 * @return mux node
 */
nnode_t *make_multiport_smux(signal_list_t **inputs, signal_list_t *selector, int num_muxed_inputs, signal_list_t *outs, nnode_t *node,
                             netlist_t *netlist)
{
    /* validation */
    int valid_num_mux_inputs = shift_left_value_with_overflow_check(0X1, selector->count, node->loc);
    oassert(valid_num_mux_inputs >= num_muxed_inputs);

    int offset = 0;

    nnode_t *mux = allocate_nnode(node->loc);
    mux->type = MULTIPORT_nBIT_SMUX;
    mux->name = node_name(mux, node->name);
    mux->traverse_visited = node->traverse_visited;

    /* add selector signal */
    add_input_port_information(mux, selector->count);
    allocate_more_input_pins(mux, selector->count);
    for (int i = 0; i < selector->count; i++) {
        npin_t *sel = selector->pins[i];
        /* hook selector into mux node as first port */
        if (sel->node)
            remap_pin_to_new_node(sel, mux, i);
        else
            add_input_pin_to_node(mux, sel, i);
    }
    offset += selector->count;

    int max_width = 0;
    for (int i = 0; i < num_muxed_inputs; i++) {
        /* keep the size of max input to allocate equal output */
        if (inputs[i]->count > max_width)
            max_width = inputs[i]->count;
    }

    for (int i = 0; i < num_muxed_inputs; i++) {
        /* add input port data */
        add_input_port_information(mux, max_width);
        allocate_more_input_pins(mux, max_width);

        for (int j = 0; j < inputs[i]->count; j++) {
            npin_t *pin = inputs[i]->pins[j];
            /* hook inputs into mux node */
            if (j < max_width) {
                if (pin->node)
                    remap_pin_to_new_node(pin, mux, j + offset);
                else
                    add_input_pin_to_node(mux, pin, j + offset);
            } else {
                /* pad with PAD node */
                add_input_pin_to_node(mux, get_pad_pin(netlist), j + offset);
            }
        }
        /* record offset to have correct idx for adding pins */
        offset += max_width;
    }

    /* add output info */
    add_output_port_information(mux, max_width);
    allocate_more_output_pins(mux, max_width);

    // specify output pin
    if (outs != NULL) {
        for (int i = 0; i < outs->count; i++) {
            npin_t *output_pin;
            if (i < max_width) {
                output_pin = outs->pins[i];
                if (output_pin->node)
                    remap_pin_to_new_node(output_pin, mux, i);
                else
                    add_output_pin_to_node(mux, output_pin, i);
            } else {
                output_pin = allocate_npin();
                nnet_t *output_net = allocate_nnet();
                /* add output driver to the output net */
                add_driver_pin_to_net(output_net, output_pin);
                /* add output pin to the mux node */
                add_output_pin_to_node(mux, output_pin, i);
            }
        }
    } else {
        signal_list_t *outputs = make_output_pins_for_existing_node(mux, max_width);
        // CLEAN UP
        free_signal_list(outputs);
    }

    return (mux);
}
