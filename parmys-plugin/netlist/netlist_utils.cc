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
#include "odin_globals.h"
#include "odin_types.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "netlist_utils.h"
#include "node_utils.h"
#include "odin_util.h"
#include "vtr_memory.h"
#include "vtr_util.h"

/*---------------------------------------------------------------------------------------------
 * (function: allocate_nnode)
 *-------------------------------------------------------------------------------------------*/
nnode_t *allocate_nnode(loc_t loc)
{
    nnode_t *new_node = (nnode_t *)my_malloc_struct(sizeof(nnode_t));

    new_node->loc = loc;
    new_node->name = NULL;
    new_node->type = NO_OP;
    new_node->bit_width = 0;
    new_node->related_ast_node = NULL;
    new_node->traverse_visited = -1;

    new_node->input_pins = NULL;
    new_node->num_input_pins = 0;
    new_node->output_pins = NULL;
    new_node->num_output_pins = 0;

    new_node->input_port_sizes = NULL;
    new_node->num_input_port_sizes = 0;
    new_node->output_port_sizes = NULL;
    new_node->num_output_port_sizes = 0;

    new_node->node_data = NULL;
    new_node->unique_node_data_id = -1;

    new_node->forward_level = -1;
    new_node->backward_level = -1;
    new_node->sequential_level = -1;
    new_node->sequential_terminator = false;

    //    new_node->in_queue = false;

    //    new_node->undriven_pins = 0;
    //    new_node->num_undriven_pins = 0;

    //    new_node->ratio = 1;

    new_node->attributes = init_attribute();

    new_node->initial_value = init_value_e::undefined;

    return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: free_nnode)
 *-------------------------------------------------------------------------------------------*/
nnode_t *free_nnode(nnode_t *to_free)
{
    if (to_free) {
        /* need to free node_data */

        for (int i = 0; i < to_free->num_input_pins; i++) {
            if (to_free->input_pins[i] && to_free->input_pins[i]->name) {
                vtr::free(to_free->input_pins[i]->name);
                to_free->input_pins[i]->name = NULL;
            }
            to_free->input_pins[i] = (npin_t *)vtr::free(to_free->input_pins[i]);
        }

        to_free->input_pins = (npin_t **)vtr::free(to_free->input_pins);

        for (int i = 0; i < to_free->num_output_pins; i++) {
            if (to_free->output_pins[i] && to_free->output_pins[i]->name) {
                vtr::free(to_free->output_pins[i]->name);
                to_free->output_pins[i]->name = NULL;
            }
            to_free->output_pins[i] = (npin_t *)vtr::free(to_free->output_pins[i]);
        }

        to_free->output_pins = (npin_t **)vtr::free(to_free->output_pins);

        vtr::free(to_free->input_port_sizes);
        vtr::free(to_free->output_port_sizes);
        //        vtr::free(to_free->undriven_pins);

        free_attribute(to_free->attributes);

        if (to_free->name) {
            vtr::free(to_free->name);
            to_free->name = NULL;
        }

        /* now free the node */
    }
    return (nnode_t *)vtr::free(to_free);
}

/*-------------------------------------------------------------------------
 * (function: allocate_more_node_input_pins)
 * 	Makes more space in the node for pin connections ...
 *-----------------------------------------------------------------------*/
void allocate_more_input_pins(nnode_t *node, int width)
{
    if (width <= 0) {
        error_message(NETLIST, node->loc, "tried adding input pins for width %d <= 0 %s\n", width, node->name);
        return;
    }

    node->input_pins = (npin_t **)vtr::realloc(node->input_pins, sizeof(npin_t *) * (node->num_input_pins + width));
    for (int i = 0; i < width; i++) {
        node->input_pins[node->num_input_pins + i] = NULL;
    }
    node->num_input_pins += width;
}

/*-------------------------------------------------------------------------
 * (function: allocate_more_node_output_pins)
 * 	Makes more space in the node for pin connections ...
 *-----------------------------------------------------------------------*/
void allocate_more_output_pins(nnode_t *node, int width)
{
    if (width <= 0) {
        error_message(NETLIST, node->loc, "tried adding output pins for width %d <= 0 %s\n", width, node->name);
        return;
    }

    node->output_pins = (npin_t **)vtr::realloc(node->output_pins, sizeof(npin_t *) * (node->num_output_pins + width));
    for (int i = 0; i < width; i++) {
        node->output_pins[node->num_output_pins + i] = NULL;
    }
    node->num_output_pins += width;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_output_port_information)
 *-------------------------------------------------------------------------------------------*/
void add_output_port_information(nnode_t *node, int port_width)
{
    node->output_port_sizes = (int *)vtr::realloc(node->output_port_sizes, sizeof(int) * (node->num_output_port_sizes + 1));
    node->output_port_sizes[node->num_output_port_sizes] = port_width;
    node->num_output_port_sizes++;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_input_port_information)
 *-------------------------------------------------------------------------------------------*/
void add_input_port_information(nnode_t *node, int port_width)
{
    node->input_port_sizes = (int *)vtr::realloc(node->input_port_sizes, sizeof(int) * (node->num_input_port_sizes + 1));
    node->input_port_sizes[node->num_input_port_sizes] = port_width;
    node->num_input_port_sizes++;
}

/*---------------------------------------------------------------------------------------------
 * (function: allocate_npin)
 *-------------------------------------------------------------------------------------------*/
npin_t *allocate_npin()
{
    npin_t *new_pin;

    new_pin = (npin_t *)my_malloc_struct(sizeof(npin_t));

    new_pin->name = NULL;
    new_pin->type = NO_ID;
    new_pin->net = NULL;
    new_pin->pin_net_idx = -1;
    new_pin->node = NULL;
    new_pin->pin_node_idx = -1;
    new_pin->mapping = NULL;

    // new_pin->values = NULL;

    new_pin->coverage = 0;

    new_pin->is_default = false;
    new_pin->is_implied = false;

    return new_pin;
}

/*---------------------------------------------------------------------------------------------
 * (function: free_npin)
 *-------------------------------------------------------------------------------------------*/
npin_t *free_npin(npin_t *to_free)
{
    if (to_free) {
        if (to_free->name)
            vtr::free(to_free->name);

        to_free->name = NULL;

        if (to_free->mapping)
            vtr::free(to_free->mapping);

        to_free->mapping = NULL;

        /* now free the pin */
    }
    return (npin_t *)vtr::free(to_free);
}

/*-------------------------------------------------------------------------
 * (function: copy_npin)
 * 	Copies a pin
 *  Should only be called by the parent functions,
 *  copy_input_npin & copy_output_npin
 *-----------------------------------------------------------------------*/
static npin_t *copy_npin(npin_t *copy_pin)
{
    npin_t *new_pin = allocate_npin();
    new_pin->name = copy_pin->name ? vtr::strdup(copy_pin->name) : NULL;
    new_pin->type = copy_pin->type;
    new_pin->mapping = copy_pin->mapping ? vtr::strdup(copy_pin->mapping) : NULL;
    new_pin->is_default = copy_pin->is_default;
    new_pin->sensitivity = copy_pin->sensitivity;

    return new_pin;
}

/*-------------------------------------------------------------------------
 * (function: copy_output_npin)
 * 	Copies an output pin
 *-----------------------------------------------------------------------*/
npin_t *copy_output_npin(npin_t *copy_pin)
{
    npin_t *new_pin = copy_npin(copy_pin);
    oassert(copy_pin->type == OUTPUT);
    new_pin->net = copy_pin->net;

    return new_pin;
}

/*-------------------------------------------------------------------------
 * (function: copy_input_npin)
 * 	Copies an input pin and potentially adds to the net
 *-----------------------------------------------------------------------*/
npin_t *copy_input_npin(npin_t *copy_pin)
{
    npin_t *new_pin = copy_npin(copy_pin);
    oassert(copy_pin->type == INPUT);
    if (copy_pin->net != NULL) {
        add_fanout_pin_to_net(copy_pin->net, new_pin);
    }

    return new_pin;
}

/*---------------------------------------------------------------------------------------------
 * (function: allocate_nnet)
 *-------------------------------------------------------------------------------------------*/
nnet_t *allocate_nnet()
{
    nnet_t *new_net = (nnet_t *)my_malloc_struct(sizeof(nnet_t));

    new_net->name = NULL;
    new_net->driver_pins = NULL;
    new_net->num_driver_pins = 0;
    new_net->fanout_pins = NULL;
    new_net->num_fanout_pins = 0;
    new_net->combined = false;

    new_net->net_data = NULL;
    new_net->unique_net_data_id = -1;

    return new_net;
}

/*---------------------------------------------------------------------------------------------
 * (function: free_nnet)
 *-------------------------------------------------------------------------------------------*/
nnet_t *free_nnet(nnet_t *to_free)
{
    if (to_free) {
        to_free->fanout_pins = (npin_t **)vtr::free(to_free->fanout_pins);

        if (to_free->name)
            vtr::free(to_free->name);

        if (to_free->num_driver_pins)
            vtr::free(to_free->driver_pins);

        /* now free the net */
    }
    return (nnet_t *)vtr::free(to_free);
}

/*---------------------------------------------------------------------------
 * (function: move_a_output_pin)
 *-------------------------------------------------------------------------*/
void move_output_pin(nnode_t *node, int old_idx, int new_idx)
{
    npin_t *pin;

    oassert(node != NULL);
    oassert(((old_idx >= 0) && (old_idx < node->num_output_pins)));
    oassert(((new_idx >= 0) && (new_idx < node->num_output_pins)));
    /* assumes the pin spots have been allocated and the pin */
    pin = node->output_pins[old_idx];
    node->output_pins[new_idx] = node->output_pins[old_idx];
    node->output_pins[old_idx] = NULL;
    /* record the node and pin spot in the pin */
    pin->type = OUTPUT;
    pin->node = node;
    pin->pin_node_idx = new_idx;
}

/*---------------------------------------------------------------------------
 * (function: move_a_input_pin)
 *-------------------------------------------------------------------------*/
void move_input_pin(nnode_t *node, int old_idx, int new_idx)
{
    npin_t *pin;

    oassert(node != NULL);
    oassert(((old_idx >= 0) && (old_idx < node->num_input_pins)));
    oassert(((new_idx >= 0) && (new_idx < node->num_input_pins)));
    /* assumes the pin spots have been allocated and the pin */
    node->input_pins[new_idx] = node->input_pins[old_idx];
    pin = node->input_pins[old_idx];
    node->input_pins[old_idx] = NULL;
    /* record the node and pin spot in the pin */
    pin->type = INPUT;
    pin->node = node;
    pin->pin_node_idx = new_idx;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_a_input_pin_to_node_spot_idx)
 *-------------------------------------------------------------------------------------------*/
void add_input_pin_to_node(nnode_t *node, npin_t *pin, int pin_idx)
{
    oassert(node != NULL);
    oassert(pin != NULL);
    oassert(pin_idx < node->num_input_pins);
    /* assumes the pin spots have been allocated and the pin */
    node->input_pins[pin_idx] = pin;
    /* record the node and pin spot in the pin */
    pin->type = INPUT;
    pin->node = node;
    pin->pin_node_idx = pin_idx;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_a_input_pin_to_spot_idx)
 *-------------------------------------------------------------------------------------------*/
void add_fanout_pin_to_net(nnet_t *net, npin_t *pin)
{
    oassert(net != NULL);
    oassert(pin != NULL);
    oassert(pin->type != OUTPUT);
    /* assumes the pin spots have been allocated and the pin */
    net->fanout_pins = (npin_t **)vtr::realloc(net->fanout_pins, sizeof(npin_t *) * (net->num_fanout_pins + 1));
    net->fanout_pins[net->num_fanout_pins] = pin;
    net->num_fanout_pins++;
    /* record the node and pin spot in the pin */
    pin->net = net;
    pin->pin_net_idx = net->num_fanout_pins - 1;
    pin->type = INPUT;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_a_output_pin_to_node_spot_idx)
 *-------------------------------------------------------------------------------------------*/
void add_output_pin_to_node(nnode_t *node, npin_t *pin, int pin_idx)
{
    oassert(node != NULL);
    oassert(pin != NULL);
    oassert(pin_idx < node->num_output_pins);
    /* assumes the pin spots have been allocated and the pin */
    node->output_pins[pin_idx] = pin;
    /* record the node and pin spot in the pin */
    pin->type = OUTPUT;
    pin->node = node;
    pin->pin_node_idx = pin_idx;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_a_output_pin_to_spot_idx)
 *-------------------------------------------------------------------------------------------*/
void add_driver_pin_to_net(nnet_t *net, npin_t *pin)
{
    oassert(net != NULL);
    oassert(pin != NULL);
    oassert(pin->type != INPUT);
    /* assumes the pin spots have been allocated and the pin */
    net->num_driver_pins++;
    net->driver_pins = (npin_t **)vtr::realloc(net->driver_pins, net->num_driver_pins * sizeof(npin_t *));
    net->driver_pins[net->num_driver_pins - 1] = pin;
    /* record the node and pin spot in the pin */
    pin->net = net;
    pin->type = OUTPUT;
    pin->pin_net_idx = net->num_driver_pins - 1;
}

/*---------------------------------------------------------------------------------------------
 * (function: join_nets)
 * 	Copies the fanouts from input net into net
 * TODO: improve error message
 *-------------------------------------------------------------------------------------------*/
void join_nets(nnet_t *join_to_net, nnet_t *other_net)
{
    if (join_to_net == other_net) {
        for (int i = 0; i < join_to_net->num_driver_pins; i++) {
            const char *pin_name = join_to_net->driver_pins[i]->name ? join_to_net->driver_pins[i]->name : "unknown";
            if ((join_to_net->driver_pins[i]->node != NULL))
                warning_message(NETLIST, join_to_net->driver_pins[i]->node->loc, "%s %s\n", "Combinational loop with driver pin", pin_name);
            else
                warning_message(NETLIST, unknown_location, "%s %s\n", "Combinational loop with driver pin", pin_name);
        }
        for (int i = 0; i < join_to_net->num_fanout_pins; i++) {
            const char *pin_name = join_to_net->fanout_pins[i]->name ? join_to_net->fanout_pins[i]->name : "unknown";
            if ((join_to_net->fanout_pins[i] != NULL) && (join_to_net->fanout_pins[i]->node != NULL))
                warning_message(NETLIST, join_to_net->fanout_pins[i]->node->loc, "%s %s\n", "Combinational loop with fanout pin", pin_name);
            else
                warning_message(NETLIST, unknown_location, "%s %s\n", "Combinational loop with fanout pin", pin_name);
        }

        error_message(NETLIST, unknown_location, "%s", "Found a combinational loop");
    } else if (other_net->num_driver_pins > 1) {
        if (other_net->name && join_to_net->name)
            error_message(NETLIST, unknown_location, "Tried to join net %s to %s but this would lose %d drivers for net %s", other_net->name,
                          join_to_net->name, other_net->num_driver_pins - 1, other_net->name);
        else
            error_message(NETLIST, unknown_location, "Tried to join nets but this would lose %d drivers", other_net->num_driver_pins - 1);
    }

    /* copy the driver over to the new_net */
    for (int i = 0; i < other_net->num_fanout_pins; i++) {
        if (other_net->fanout_pins[i]) {
            add_fanout_pin_to_net(join_to_net, other_net->fanout_pins[i]);
        }
    }

    // CLEAN UP
    free_nnet(other_net);
}

/*---------------------------------------------------------------------------------------------
 * (function: remap_pin_to_new_net)
 *-------------------------------------------------------------------------------------------*/
void remap_pin_to_new_net(npin_t *pin, nnet_t *new_net)
{
    if (pin->type == INPUT) {
        /* clean out the entry in the old net */
        pin->net->fanout_pins[pin->pin_net_idx] = NULL;
        /* do the new addition */
        add_fanout_pin_to_net(new_net, pin);
    } else if (pin->type == OUTPUT) {
        /* clean out the entry in the old net */
        if (pin->net->num_driver_pins)
            vtr::free(pin->net->driver_pins);
        pin->net->num_driver_pins = 0;
        pin->net->driver_pins = NULL;
        /* do the new addition */
        add_driver_pin_to_net(new_net, pin);
    }
}

/*-------------------------------------------------------------------------
 * (function: remap_pin_to_new_node)
 *-----------------------------------------------------------------------*/
void remap_pin_to_new_node(npin_t *pin, nnode_t *new_node, int pin_idx)
{
    if (pin->type == INPUT) {
        /* clean out the entry in the old net */
        pin->node->input_pins[pin->pin_node_idx] = NULL;
        /* do the new addition */
        add_input_pin_to_node(new_node, pin, pin_idx);
    } else if (pin->type == OUTPUT) {
        /* clean out the entry in the old net */
        pin->node->output_pins[pin->pin_node_idx] = NULL;
        /* do the new addition */
        add_output_pin_to_node(new_node, pin, pin_idx);
    }
}

/*------------------------------------------------------------------------
 * (function: connect_nodes)
 * 	Connect one output node to the inputs of the input node
 *----------------------------------------------------------------------*/
void connect_nodes(nnode_t *out_node, int out_idx, nnode_t *in_node, int in_idx)
{
    npin_t *new_in_pin;

    oassert(out_node->num_output_pins > out_idx);
    oassert(in_node->num_input_pins > in_idx);

    new_in_pin = allocate_npin();

    /* create the pin that hooks up to the input */
    add_input_pin_to_node(in_node, new_in_pin, in_idx);

    if (out_node->output_pins[out_idx] == NULL) {
        /* IF - this node has no output net or pin */
        npin_t *new_out_pin;
        nnet_t *new_net;
        new_net = allocate_nnet();
        new_out_pin = allocate_npin();

        new_net->name = vtr::strdup(out_node->name);
        /* create the pin that hooks up to the input */
        add_output_pin_to_node(out_node, new_out_pin, out_idx);
        /* hook up in pin out of the new net */
        add_fanout_pin_to_net(new_net, new_in_pin);
        /* hook up the new pin 2 to this new net */
        add_driver_pin_to_net(new_net, new_out_pin);
    } else {
        /* ELSE - there is a net so we just add a fanout */
        /* hook up in pin out of the new net */
        add_fanout_pin_to_net(out_node->output_pins[out_idx]->net, new_in_pin);
    }
}

/**
 * -------------------------------------------------------------------------------------------
 * (function: init_attribute_structure)
 *
 * @brief Initializes the netlist node attributes
 * including edge sensitivies and reset value
 *-------------------------------------------------------------------------------------------*/
attr_t *init_attribute()
{
    attr_t *attribute;
    attribute = (attr_t *)vtr::malloc(sizeof(attr_t));

    attribute->clk_edge_type = UNDEFINED_SENSITIVITY;
    attribute->clr_polarity = UNDEFINED_SENSITIVITY;
    attribute->set_polarity = UNDEFINED_SENSITIVITY;
    attribute->areset_polarity = UNDEFINED_SENSITIVITY;
    attribute->sreset_polarity = UNDEFINED_SENSITIVITY;
    attribute->enable_polarity = UNDEFINED_SENSITIVITY;

    attribute->areset_value = 0;
    attribute->sreset_value = 0;

    attribute->port_a_signed = UNSIGNED;
    attribute->port_b_signed = UNSIGNED;

    /* memory node attributes */
    attribute->size = 0;
    attribute->offset = 0;
    attribute->memory_id = NULL;

    attribute->RD_CLK_ENABLE = UNDEFINED_SENSITIVITY;
    attribute->WR_CLK_ENABLE = UNDEFINED_SENSITIVITY;
    attribute->RD_CLK_POLARITY = UNDEFINED_SENSITIVITY;
    attribute->WR_CLK_POLARITY = UNDEFINED_SENSITIVITY;

    attribute->RD_PORTS = 0;
    attribute->WR_PORTS = 0;
    attribute->DBITS = 0;
    attribute->ABITS = 0;

    return (attribute);
}

/**
 * -------------------------------------------------------------------------------------------
 * (function: copy_attribute)
 *
 * @brief copy an attribute to another one. If the second
 * attribute is null, it will creates one
 *
 * @param to will copy to this attr
 * @param copy the attr that will be copied
 *-------------------------------------------------------------------------------------------*/
void copy_attribute(attr_t *to, attr_t *copy)
{
    if (to == NULL)
        to = init_attribute();

    to->clk_edge_type = copy->clk_edge_type;
    to->clr_polarity = copy->clr_polarity;
    to->set_polarity = copy->set_polarity;
    to->areset_polarity = copy->areset_polarity;
    to->sreset_polarity = copy->sreset_polarity;
    to->enable_polarity = copy->enable_polarity;

    to->areset_value = copy->areset_value;
    to->sreset_value = copy->sreset_value;

    to->port_a_signed = copy->port_a_signed;
    to->port_b_signed = copy->port_b_signed;

    /* memory node attributes */
    to->size = copy->size;
    to->offset = copy->offset;
    to->memory_id = vtr::strdup(copy->memory_id);

    to->RD_CLK_ENABLE = copy->RD_CLK_ENABLE;
    to->WR_CLK_ENABLE = copy->WR_CLK_ENABLE;
    to->RD_CLK_POLARITY = copy->RD_CLK_POLARITY;
    to->WR_CLK_POLARITY = copy->WR_CLK_POLARITY;

    to->RD_PORTS = copy->RD_PORTS;
    to->WR_PORTS = copy->WR_PORTS;
    to->DBITS = copy->DBITS;
    to->ABITS = copy->ABITS;
}

/**
 * -------------------------------------------------------------------------------------------
 * (function: copy_signedness)
 *
 * @brief copy the signedness variables of an attribute to another
 * one. If the second attribute is null, it will creates one
 *
 * @param to will copy to this attr
 * @param copy the attr that will be copied
 *-------------------------------------------------------------------------------------------*/
void copy_signedness(attr_t *to, attr_t *copy)
{
    if (to == NULL)
        to = init_attribute();

    to->port_a_signed = copy->port_a_signed;
    to->port_b_signed = copy->port_b_signed;
}

/*---------------------------------------------------------------------------------------------
 * (function: init_signal_list_structure)
 * 	Initializes the list structure which describes inputs and outputs of elements
 * 	as they coneect to other elements in the graph.
 *-------------------------------------------------------------------------------------------*/
signal_list_t *init_signal_list()
{
    signal_list_t *list;
    list = (signal_list_t *)vtr::malloc(sizeof(signal_list_t));

    list->count = 0;
    list->pins = NULL;
    list->is_memory = false;
    list->is_adder = false;

    return list;
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: is_constant_signal)
 *
 * @brief showing that a given signal list has a constant value or not
 *
 * @param signal list of pins
 * @param netlist pointer to the current netlist file
 *
 * @return is it constant or not
 *-------------------------------------------------------------------------------------------*/
bool is_constant_signal(signal_list_t *signal, netlist_t *netlist)
{
    bool is_constant = true;

    for (int i = 0; i < signal->count; i++) {
        nnet_t *net = signal->pins[i]->net;
        /* neither connected to GND nor VCC */
        if (strcmp(net->name, netlist->zero_net->name) && strcmp(net->name, netlist->one_net->name)) {
            is_constant = false;
            break;
        }
    }

    return (is_constant);
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: constant_signal_value)
 *
 * @brief calculating the value of a constant signal list
 *
 * @param signal list of pins
 * @param netlist pointer to the current netlist file
 *
 * @return the integer value of the constant signal
 *-------------------------------------------------------------------------------------------*/
long constant_signal_value(signal_list_t *signal, netlist_t *netlist)
{
    oassert(is_constant_signal(signal, netlist));

    long return_value = 0;

    for (int i = 0; i < signal->count; i++) {
        nnet_t *net = signal->pins[i]->net;
        /* if the pin is connected to VCC */
        if (!strcmp(net->name, netlist->one_net->name)) {
            return_value += shift_left_value_with_overflow_check(0X1, i, unknown_location);
        }
    }

    return (return_value);
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: create_constant_signal)
 *
 * @brief create the signal_list of the given constant value
 *
 * @param value a long value
 * @param desired_width the size of return signal list
 * @param netlist pointer to the netlist
 *-------------------------------------------------------------------------------------------*/
signal_list_t *create_constant_signal(const long long value, const int desired_width, netlist_t *netlist)
{
    signal_list_t *list = init_signal_list();

    std::string binary_value_str = string_of_radix_to_bitstring(std::to_string(value), 10);
    long width = binary_value_str.length();

    while (desired_width > width) {
        if (value < 0)
            binary_value_str = "1" + binary_value_str;
        else
            binary_value_str = "0" + binary_value_str;

        width = binary_value_str.length();
    }

    long start = width;
    long end = width - desired_width;
    bool extension = false;

    /* create vcc/gnd signal pins */
    for (long i = start; i > end; i--) {
        if (binary_value_str[i - 1] == '1') {
            add_pin_to_signal_list(list, get_one_pin(netlist));
        } else {
            add_pin_to_signal_list(list, (extension) ? get_pad_pin(netlist) : get_zero_pin(netlist));
        }
    }

    return (list);
}

/**
 * (function: prune_signal)
 *
 * @brief to prune extra pins. usually this happens when a memory
 * address (less than 32 bits) comes from an arithmetic operation
 * that makes it 32 bits (a lot useless pins).
 *
 * @param signalsvar signal list of pins (may include one signal or two signals)
 * @param signal_width the width of each signal in signalsvar
 * @param prune_size the desired size of signal list
 * @param num_of_signals showing the number of signals the signalsvar containing
 *
 * @return pruned signal list
 */
signal_list_t *prune_signal(signal_list_t *signalsvar, long signal_width, long prune_size, int num_of_signals)
{
    /* validation */
    oassert(prune_size);
    oassert(num_of_signals);
    oassert(signalsvar->count % signal_width == 0);

    /* no need to prune */
    if (prune_size >= signal_width)
        return (signalsvar);

    /* new signal list */
    signal_list_t *new_signals = NULL;
    signal_list_t **splitted_signals = split_signal_list(signalsvar, signal_width);

    /* iterating over signals to prune them */
    for (int i = 0; i < num_of_signals; i++) {
        /* init pruned signal list */
        signal_list_t *new_signal = init_signal_list();
        for (int j = 0; j < signal_width; j++) {
            npin_t *pin = splitted_signals[i]->pins[j];
            /* adding pin to new signal list */
            if (j < prune_size) {
                add_pin_to_signal_list(new_signal, pin);
            }
            /* pruning the extra pins */
            else {
                /* detach from the node, its net and free pin */
                pin->node->input_pins[pin->pin_node_idx] = NULL;
                pin->node = NULL;
                warning_message(NETLIST, unknown_location, "Input pin (%s) exceeds the size of its connected port, will be left unconnected",
                                pin->net->name);
            }
        }

        free_signal_list(splitted_signals[i]);
        splitted_signals[i] = new_signal;
    }

    /* combining pruned signals */
    new_signals = combine_lists(splitted_signals, num_of_signals);

    // CLEAN UP
    vtr::free(splitted_signals);

    return (new_signals);
}

/*---------------------------------------------------------------------------------------------
 * (function: add_inpin_to_signal_list)
 * 	Stores a pin in the signal list
 *-------------------------------------------------------------------------------------------*/
void add_pin_to_signal_list(signal_list_t *list, npin_t *pin)
{
    list->pins = (npin_t **)vtr::realloc(list->pins, sizeof(npin_t *) * (list->count + 1));
    list->pins[list->count] = pin;
    list->count++;
}

/*---------------------------------------------------------------------------------------------
 * (function: combine_lists)
 *-------------------------------------------------------------------------------------------*/
signal_list_t *combine_lists(signal_list_t **signal_lists, int num_signal_lists)
{
    for (int i = 1; i < num_signal_lists; i++) {
        if (signal_lists[i]) {
            for (int j = 0; j < signal_lists[i]->count; j++) {
                bool pin_already_added = false;
                for (int k = 0; k < signal_lists[0]->count; k++) {
                    if (!strcmp(signal_lists[0]->pins[k]->name, signal_lists[i]->pins[j]->name))
                        pin_already_added = true;
                }

                if (!pin_already_added)
                    add_pin_to_signal_list(signal_lists[0], signal_lists[i]->pins[j]);
            }

            free_signal_list(signal_lists[i]);
        }
    }

    return signal_lists[0];
}

/**
 * (function: split_list)
 *
 * @brief split signals list to a list of signal list with requested width
 *
 * @param signalsvar signal list of pins (may include one signal or two signals)
 * @param width the width of each signal in signalsvar
 *
 * @return splitted signal list
 */
signal_list_t **split_signal_list(signal_list_t *signalsvar, const int width)
{
    signal_list_t **splitted_signals = NULL;

    /* check if split is needed */
    if (signalsvar->count == width) {
        splitted_signals = (signal_list_t **)vtr::calloc(1, sizeof(signal_list_t *));
        splitted_signals[0] = signalsvar;
        return (splitted_signals);
    }

    /* validate signals list size */
    oassert(width != 0);
    oassert(signalsvar->count % width == 0);

    int offset = 0;
    int num_chunk = signalsvar->count / width;

    /* initialize splitted signals */
    splitted_signals = (signal_list_t **)vtr::calloc(num_chunk, sizeof(signal_list_t *));
    for (int i = 0; i < num_chunk; i++) {
        splitted_signals[i] = init_signal_list();
        for (int j = 0; j < width; j++) {
            npin_t *pin = signalsvar->pins[j + offset];
            /* add to splitted signals list */
            add_pin_to_signal_list(splitted_signals[i], pin);
        }
        offset += width;
    }

    return (splitted_signals);
}

/**
 * (function: sigcmp)
 *
 * @brief to check if sig is the same as be_checked
 *
 * @param sig first signals
 * @param be_checked second signals
 */
bool sigcmp(signal_list_t *sig, signal_list_t *be_checked)
{
    /* validate signal sizes */
    oassert(sig->count == be_checked->count);

    for (int i = 0; i < sig->count; i++) {
        /* checking their net */
        if (sig->pins[i]->net != be_checked->pins[i]->net) {
            return (false);
        }
    }
    return (true);
}

signal_list_t *copy_input_signals(signal_list_t *signalsvar)
{
    signal_list_t *duplicate_signals = init_signal_list();
    for (int i = 0; i < signalsvar->count; i++) {
        npin_t *pin = signalsvar->pins[i];
        pin = copy_input_npin(pin);
        add_pin_to_signal_list(duplicate_signals, pin);
    }
    return duplicate_signals;
}

/*---------------------------------------------------------------------------------------------
 * (function: make_output_pins_for_existing_node)
 * 	Looks at a node and extracts the output pins into a signal list so they can be accessed
 * 	in this form
 *-------------------------------------------------------------------------------------------*/
signal_list_t *make_output_pins_for_existing_node(nnode_t *node, int width)
{
    signal_list_t *return_list = init_signal_list();

    oassert(node->num_output_pins == width);

    for (int i = 0; i < width; i++) {
        npin_t *new_pin1;
        npin_t *new_pin2;
        nnet_t *new_net;
        new_pin1 = allocate_npin();
        new_pin2 = allocate_npin();
        new_net = allocate_nnet();
        new_net->name = node->name;
        /* hook the output pin into the node */
        add_output_pin_to_node(node, new_pin1, i);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net, new_pin1);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net, new_pin2);

        /* add the new_pin2 to the list of pins */
        add_pin_to_signal_list(return_list, new_pin2);
    }

    return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function: clean_signal_list_structure)
 *-------------------------------------------------------------------------------------------*/
void free_signal_list(signal_list_t *list)
{
    if (list) {
        vtr::free(list->pins);
        list->count = 0;
    }
    vtr::free(list);
    list = NULL;
}

/**
 * -------------------------------------------------------------------------------------------
 * (function: free_attribute)
 *
 * @brief clean the given attribute structure to avoid mem leaks
 *
 * @param attribute the given attribute structure
 *-------------------------------------------------------------------------------------------*/
void free_attribute(attr_t *attribute)
{
    if (attribute) {
        vtr::free(attribute->memory_id);
        attribute->memory_id = NULL;
    }

    vtr::free(attribute);
    attribute = NULL;
}

void depth_traverse_count(nnode_t *node, int *count, uintptr_t traverse_mark_number);

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traverse)
 *-------------------------------------------------------------------------------------------*/
void depth_traverse_count(nnode_t *node, int *count, uintptr_t traverse_mark_number)
{
    nnode_t *next_node;
    nnet_t *next_net;

    if (node->traverse_visited == traverse_mark_number) {
        return;
    } else {
        /* ELSE - this is a new node so depth visit it */
        (*count)++;

        node->traverse_visited = traverse_mark_number;

        for (int i = 0; i < node->num_output_pins; i++) {
            if (node->output_pins[i]->net == NULL)
                continue;

            next_net = node->output_pins[i]->net;
            for (int j = 0; j < next_net->num_fanout_pins; j++) {
                if (next_net->fanout_pins[j] == NULL)
                    continue;
                next_node = next_net->fanout_pins[j]->node;
                if (next_node == NULL)
                    continue;

                depth_traverse_count(next_node, count, traverse_mark_number);
            }
        }
    }
}

/*---------------------------------------------------------------------------------------------
 * (function:  allocate_netlist)
 *-------------------------------------------------------------------------------------------*/
netlist_t *allocate_netlist()
{
    netlist_t *new_netlist;

    new_netlist = (netlist_t *)my_malloc_struct(sizeof(netlist_t));

    new_netlist->gnd_node = NULL;
    new_netlist->vcc_node = NULL;
    new_netlist->pad_node = NULL;
    new_netlist->zero_net = NULL;
    new_netlist->one_net = NULL;
    new_netlist->pad_net = NULL;
    new_netlist->top_input_nodes = NULL;
    new_netlist->num_top_input_nodes = 0;
    new_netlist->top_output_nodes = NULL;
    new_netlist->num_top_output_nodes = 0;
    new_netlist->ff_nodes = NULL;
    new_netlist->num_ff_nodes = 0;
    new_netlist->internal_nodes = NULL;
    new_netlist->num_internal_nodes = 0;
    new_netlist->clocks = NULL;
    new_netlist->num_clocks = 0;

    new_netlist->forward_levels = NULL;
    new_netlist->num_forward_levels = 0;
    new_netlist->num_at_forward_level = NULL;
    new_netlist->backward_levels = NULL;
    new_netlist->num_backward_levels = 0;
    new_netlist->num_at_backward_level = NULL;
    new_netlist->sequential_level_nodes = NULL;
    new_netlist->num_sequential_levels = 0;
    new_netlist->num_at_sequential_level = NULL;
    new_netlist->sequential_level_combinational_termination_node = NULL;
    new_netlist->num_sequential_level_combinational_termination_nodes = 0;
    new_netlist->num_at_sequential_level_combinational_termination_node = NULL;

    /* initialize the string chaches */
    new_netlist->nets_sc = sc_new_string_cache();
    new_netlist->out_pins_sc = sc_new_string_cache();
    new_netlist->nodes_sc = sc_new_string_cache();

    return new_netlist;
}

/*---------------------------------------------------------------------------------------------
 * (function:  free_netlist)
 *-------------------------------------------------------------------------------------------*/
void free_netlist(netlist_t *to_free)
{
    if (!to_free)
        return;

    sc_free_string_cache(to_free->nets_sc);
    sc_free_string_cache(to_free->out_pins_sc);
    sc_free_string_cache(to_free->nodes_sc);
}

/*
 * Gets the index of the first output pin with the given mapping
 * on the given node.
 */
int get_output_pin_index_from_mapping(nnode_t *node, const char *name)
{
    for (int i = 0; i < node->num_output_pins; i++) {
        npin_t *pin = node->output_pins[i];
        if (!strcmp(pin->mapping, name))
            return i;
    }

    return -1;
}

/*
 * Gets the index of the first output port containing a pin with the given
 * mapping.
 */
int get_output_port_index_from_mapping(nnode_t *node, const char *name)
{
    int pin_number = 0;
    for (int i = 0; i < node->num_output_port_sizes; i++) {
        for (int j = 0; j < node->output_port_sizes[i]; j++, pin_number++) {
            npin_t *pin = node->output_pins[pin_number];
            if (!strcmp(pin->mapping, name))
                return i;
        }
    }
    return -1;
}

/*
 * Gets the index of the first pin with the given mapping.
 */
int get_input_pin_index_from_mapping(nnode_t *node, const char *name)
{
    for (int i = 0; i < node->num_input_pins; i++) {
        npin_t *pin = node->input_pins[i];
        if (!strcmp(pin->mapping, name))
            return i;
    }

    return -1;
}

/*
 * Gets the port index of the first port containing a pin with
 * the given mapping.
 */
int get_input_port_index_from_mapping(nnode_t *node, const char *name)
{
    int pin_number = 0;
    for (int i = 0; i < node->num_input_port_sizes; i++) {
        for (int j = 0; j < node->input_port_sizes[i]; j++, pin_number++) {
            npin_t *pin = node->input_pins[pin_number];
            if (!strcmp(pin->mapping, name))
                return i;
        }
    }
    return -1;
}

/**
 * (function: legalize_polarity)
 *
 * @brief legalize pin polarity to RE
 *
 * @param pin first pin
 * @param pin_polarity first pin polarity
 * @param node pointer to pins node for tracking purpose
 *
 * @return a new pin with RISING_EDGE_SENSITIVITY polarity
 */
npin_t *legalize_polarity(npin_t *pin, edge_type_e pin_polarity, nnode_t *node)
{
    /* validate pins */
    oassert(pin && node);
    oassert(pin->type == INPUT);

    /* pin and its polarity */
    npin_t *pin_out = pin;

    /* detach pin from its node */
    if (pin->node)
        pin->node->input_pins[pin->pin_node_idx] = NULL;

    if (pin_polarity == FALLING_EDGE_SENSITIVITY || pin_polarity == ACTIVE_LOW_SENSITIVITY) {
        /* create a not gate */
        nnode_t *not_node = make_1port_gate(LOGICAL_NOT, 1, 1, node, node->traverse_visited);
        /* hook the pin into not node */
        add_input_pin_to_node(not_node, pin, 0);
        /* create output pins */
        pin_out = allocate_npin();
        npin_t *not_out = allocate_npin();
        nnet_t *not_out_net = allocate_nnet();
        not_out_net->name = make_full_ref_name(NULL, NULL, NULL, not_node->name, 0);
        /* hook the output pin into the node */
        add_output_pin_to_node(not_node, not_out, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(not_out_net, not_out);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(not_out_net, pin_out);
    }

    /* set new pin polarity */
    pin_out->sensitivity = RISING_EDGE_SENSITIVITY;

    return (pin_out);
}

/**
 * (function: reduce_input_ports)
 *
 * @brief reduce the input ports size by removing extra pad pins
 *
 * @param node pointer to node
 * @param netlist pointer to the current netlist
 *
 * @return nothing, but set the node to a new node with reduced equalized
 * input port sizes (if more than one input port exist)
 */
void reduce_input_ports(nnode_t *&node, netlist_t *netlist)
{
    oassert(node->num_input_port_sizes == 1 || node->num_input_port_sizes == 2);

    int offset = 0;
    nnode_t *new_node;

    signal_list_t **input_ports = (signal_list_t **)vtr::calloc(node->num_input_port_sizes, sizeof(signal_list_t *));
    /* add pins to signals lists */
    for (int i = 0; i < node->num_input_port_sizes; i++) {
        /* initialize signal list */
        input_ports[i] = init_signal_list();
        for (int j = 0; j < node->input_port_sizes[i]; j++) {
            add_pin_to_signal_list(input_ports[i], node->input_pins[j + offset]);
        }
        offset += node->input_port_sizes[i];
    }

    /* reduce the first port */
    input_ports[0] = reduce_signal_list(input_ports[0], node->attributes->port_a_signed, netlist);
    /* reduce the second port if exist */
    if (node->num_input_port_sizes == 2) {
        input_ports[1] = reduce_signal_list(input_ports[1], node->attributes->port_b_signed, netlist);

        /* equalize port sizes */
        int max = std::max(input_ports[0]->count, input_ports[1]->count);

        while (input_ports[0]->count < max)
            add_pin_to_signal_list(input_ports[0], get_pad_pin(netlist));
        while (input_ports[1]->count < max)
            add_pin_to_signal_list(input_ports[1], get_pad_pin(netlist));
    }

    /* creating a new node */
    new_node = (node->num_input_port_sizes == 1)
                 ? make_1port_gate(node->type, input_ports[0]->count, node->num_output_pins, node, node->traverse_visited)
                 : make_2port_gate(node->type, input_ports[0]->count, input_ports[1]->count, node->num_output_pins, node, node->traverse_visited);

    /* copy attributes */
    copy_signedness(new_node->attributes, node->attributes);

    /* hook the input pins */
    for (int i = 0; i < input_ports[0]->count; i++) {
        npin_t *pin = input_ports[0]->pins[i];
        if (pin->node) {
            /* remap pins to new node */
            remap_pin_to_new_node(input_ports[0]->pins[i], new_node, i);
        } else {
            /* add pins to new node */
            add_input_pin_to_node(new_node, input_ports[0]->pins[i], i);
        }
    }
    offset = input_ports[0]->count;

    if (node->num_input_port_sizes == 2) {
        for (int i = 0; i < input_ports[1]->count; i++) {
            npin_t *pin = input_ports[1]->pins[i];
            if (pin->node) {
                /* remap pins to new node */
                remap_pin_to_new_node(input_ports[1]->pins[i], new_node, i + offset);
            } else {
                /* add pins to new node */
                add_input_pin_to_node(new_node, input_ports[1]->pins[i], i + offset);
            }
        }
    }

    /* hook the output pins */
    for (int i = 0; i < node->num_output_pins; i++) {
        remap_pin_to_new_node(node->output_pins[i], new_node, i);
    }

    // CLEAN UP
    for (int i = 0; i < node->num_input_port_sizes; i++) {
        free_signal_list(input_ports[i]);
    }
    vtr::free(input_ports);
    free_nnode(node);

    node = new_node;
}

/**
 * (function: reduce_signal_list)
 *
 * @brief reduce the size of signal list by removing extra pad pins
 *
 * @param signalvar list of signals
 * @param signedness the signedness of a port corresponding to the signal list
 * @param netlist pointer to the current netlist
 *
 * @return a pruned signal list
 */
signal_list_t *reduce_signal_list(signal_list_t *signalvar, operation_list signedness, netlist_t *netlist)
{
    /* validate signedness */
    oassert(signedness == operation_list::SIGNED || signedness == operation_list::UNSIGNED);

    signal_list_t *return_value = init_signal_list();
    /* specify the extension net */
    nnet_t *extended_net = (signedness == operation_list::SIGNED) ? netlist->one_net : netlist->zero_net;

    for (int i = signalvar->count - 1; i > -1; i--) {
        npin_t *pin = signalvar->pins[i];
        if (pin->net == extended_net) {
            delete_npin(pin);
            signalvar->pins[i] = NULL;
        } else {
            /* reaching to valuable pins, so break the pruning process */
            break;
        }
    }

    /* adding valuable pins to new signals list */
    for (int i = 0; i < signalvar->count; i++) {
        if (signalvar->pins[i]) {
            add_pin_to_signal_list(return_value, signalvar->pins[i]);
        }
    }

    // CLEAN UP
    free_signal_list(signalvar);

    return (return_value);
}

chain_information_t *allocate_chain_info()
{
    chain_information_t *new_node;

    new_node = (chain_information_t *)my_malloc_struct(sizeof(chain_information_t));

    new_node->name = NULL;
    new_node->count = 0;

    return new_node;
}

/**
 * (function: equalize_port_sizes)
 *
 * @brief equalizing the input and output ports for the given node
 *
 * NOTE: at max TWO input ports is supported
 *
 * @param node pointing to a shift node
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
void equalize_ports_size(nnode_t *&node, uintptr_t traverse_mark_number, netlist_t *netlist)
{
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes > 0 && node->num_input_port_sizes <= 2);

    /**
     * INPUTS
     *  A: (width_a)
     *  B: (width_b) [optional]
     * OUTPUT
     *  Y: width_y
     */
    /* removing extra pad pins based on the signedness of ports */
    reduce_input_ports(node, netlist);

    int port_a_size = node->input_port_sizes[0];
    int port_b_size = -1;
    if (node->num_input_port_sizes == 2) {
        port_b_size = node->input_port_sizes[1];
        /* validate inputport sizes */
        oassert(port_a_size == port_b_size);
    }

    int port_y_size = node->output_port_sizes[0];

    /* no change is needed */
    if (port_a_size == port_y_size)
        return;

    /* new port size */
    int new_out_size = port_a_size;

    /* creating the new node */
    nnode_t *new_node = (port_b_size == -1) ? make_1port_gate(node->type, port_a_size, new_out_size, node, traverse_mark_number)
                                            : make_2port_gate(node->type, port_a_size, port_b_size, new_out_size, node, traverse_mark_number);

    /* copy signedness attributes */
    copy_signedness(new_node->attributes, node->attributes);

    for (int i = 0; i < node->num_input_pins; i++) {
        /* remapping the a pins */
        remap_pin_to_new_node(node->input_pins[i], new_node, i);
    }

    /* Connecting output pins */
    for (int i = 0; i < new_out_size; i++) {
        if (i < port_y_size) {
            remap_pin_to_new_node(node->output_pins[i], new_node, i);
        } else {
            /* need create a new output pin */
            npin_t *new_pin1 = allocate_npin();
            npin_t *new_pin2 = allocate_npin();
            nnet_t *new_net = allocate_nnet();
            new_net->name = make_full_ref_name(NULL, NULL, NULL, new_node->name, i);
            /* hook the output pin into the node */
            add_output_pin_to_node(new_node, new_pin1, i);
            /* hook up new pin 1 into the new net */
            add_driver_pin_to_net(new_net, new_pin1);
            /* hook up the new pin 2 to this new net */
            add_fanout_pin_to_net(new_net, new_pin2);
        }
    }

    if (new_out_size < port_y_size) {
        for (int i = new_out_size; i < port_y_size; i++) {
            /* need to drive extra output pins with PAD */
            nnode_t *buf_node = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
            /* hook a pin from PAD node into the buf node */
            add_input_pin_to_node(buf_node, get_pad_pin(netlist), 0);
            /* remap the extra output pin to buf node */
            remap_pin_to_new_node(node->output_pins[i], buf_node, 0);
        }
    }

    // CLEAN UP
    free_nnode(node);

    node = new_node;
}

void remove_fanout_pins_from_net(nnet_t *net, npin_t * /*pin*/, int id)
{
    int i;
    for (i = id; i < net->num_fanout_pins - 1; i++) {
        net->fanout_pins[i] = net->fanout_pins[i + 1];
        if (net->fanout_pins[i] != NULL)
            net->fanout_pins[i]->pin_net_idx = i;
    }
    net->fanout_pins[i] = NULL;
    net->num_fanout_pins--;
}

void delete_npin(npin_t *pin)
{
    if (pin->type == INPUT) {
        /* detach from its node */
        pin->node->input_pins[pin->pin_node_idx] = NULL;
        /* detach from its net */
        pin->net->fanout_pins[pin->pin_net_idx] = NULL;
    } else if (pin->type == OUTPUT) {
        /* detach from its node */
        pin->node->output_pins[pin->pin_node_idx] = NULL;
        /* detach from its net */
        pin->net->driver_pins[pin->pin_net_idx] = NULL;
    }
    // CLEAN UP
    free_npin(pin);
}