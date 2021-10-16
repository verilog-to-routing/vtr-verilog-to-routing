/**
 * Copyright (c) 2021 Seyed Alireza Damghani (sdamghann@gmail.com)
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
 *
 * @file: This file provides utilities to modify the logical nodes 
 * to make them compatible with the Odin-II partial mapper.
 */

#include "LogicalOps.hpp"
#include "node_creation_library.h"
#include "odin_util.h"
#include "netlist_utils.h"
#include "vtr_memory.h"

/**
 * (function: prune_logical_node_outputs)
 * 
 * @brief prune output pins[1..n] by driving them 
 * using GND and keep only the first output pin
 * 
 * @param node pointing to a logical node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
void prune_logical_node_outputs(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes > 0);
    oassert(node->num_output_port_sizes == 1);

    /* no need to split. just return the node with equalized input ports size */
    if (node->num_output_pins == 1) {
        equalize_input_ports_size(node, traverse_mark_number, netlist);
        return;
    }

    int i, j;
    /* keep the recors of input widths */
    int width_a = node->input_port_sizes[0];
    int width_b = (node->num_input_port_sizes == 2) ? node->input_port_sizes[1] : -1;
    int max_width = std::max(width_a, width_b);

    /* making a new node with input port sizes equal to max_width and a single output pin */
    nnode_t* new_node = (node->num_input_port_sizes == 1)
                            ? make_1port_gate(node->type, max_width, 1, node, traverse_mark_number)
                            : make_2port_gate(node->type, max_width, max_width, 1, node, traverse_mark_number);

    /* copy attributes */
    copy_signedness(new_node->attributes, node->attributes);

    for (i = 0; i < max_width; i++) {
        /* hook the first input into new node */
        if (i < width_a) {
            remap_pin_to_new_node(node->input_pins[i], new_node, i);
        } else {
            add_input_pin_to_node(new_node,
                                  (node->attributes->port_a_signed) ? get_one_pin(netlist) : get_zero_pin(netlist),
                                  i);
        }
        /* hook the second input into new node if exist */
        if (node->num_input_port_sizes == 2) {
            if (i < width_b) {
                remap_pin_to_new_node(node->input_pins[width_a + i], new_node, max_width + i);
            } else {
                add_input_pin_to_node(new_node,
                                      (node->attributes->port_b_signed) ? get_one_pin(netlist) : get_zero_pin(netlist),
                                      max_width + i);
            }
        }
    }

    /* handle output pins */
    for (i = 0; i < node->num_output_pins; i++) {
        /* container for the output pin */
        npin_t* output_pin = node->output_pins[i];

        if (i == 0) {
            /* remap the first output (will show the logic node result) to the new node */
            remap_pin_to_new_node(output_pin, new_node, 0);
        } else {
            /* detach output pins [1..n] and drive them with GND */
            nnet_t* output_net = output_pin->net;

            // make GND the driver the output pins
            for (j = 0; j < output_net->num_fanout_pins; j++) {
                npin_t* fanout_pin = output_net->fanout_pins[j];
                add_fanout_pin_to_net(netlist->zero_net, fanout_pin);
            }
            // CLEAN UP per pin
            if (output_pin->mapping)
                vtr::free(output_pin->mapping);

            free_nnet(output_net);
        }
    }

    // CLEAN UP
    free_nnode(node);
}

/**
 * (function: split_in_single_bit_logic)
 * 
 * @brief splitting the logical node into single bit logical nodes
 * 
 * @param node pointing to a logical node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
void split_in_single_bit_logic(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes > 0);
    oassert(node->num_output_port_sizes == 1);

    /* no need to split. just return the node with equalized input ports size */
    if (node->num_output_pins == 1) {
        equalize_input_ports_size(node, traverse_mark_number, netlist);
        return;
    }

    int i;
    /* keep the records of input widths */
    int width_a = node->input_port_sizes[0];
    int width_b = (node->num_input_port_sizes == 2) ? node->input_port_sizes[1] : -1;
    int max_width = std::max(width_a, width_b);

    for (i = 0; i < max_width; i++) {
        /* making a new node with input port sizes equal to max_width and a single output pin */
        nnode_t* new_node = (node->num_input_port_sizes == 1)
                                ? make_1port_gate(node->type, 1, 1, node, traverse_mark_number)
                                : make_2port_gate(node->type, 1, 1, 1, node, traverse_mark_number);

        /* copy attributes */
        copy_signedness(new_node->attributes, node->attributes);

        /* hook the first input into new node */
        if (i < width_a) {
            remap_pin_to_new_node(node->input_pins[i], new_node, 0);
        } else {
            add_input_pin_to_node(new_node,
                                  (node->attributes->port_a_signed) ? get_one_pin(netlist) : get_zero_pin(netlist),
                                  0);
        }
        /* hook the second input into new node if exist */
        if (node->num_input_port_sizes == 2) {
            if (i < width_b) {
                remap_pin_to_new_node(node->input_pins[width_a + i], new_node, 1);
            } else {
                add_input_pin_to_node(new_node,
                                      (node->attributes->port_b_signed) ? get_one_pin(netlist) : get_zero_pin(netlist),
                                      1);
            }
        }

        /* handle output pins */
        if (i < node->num_output_pins) {
            npin_t* output_pin = node->output_pins[i];
            /* remap the first output (will show the logic node result) to the new node */
            remap_pin_to_new_node(output_pin, new_node, 0);
        } else {
            npin_t* new_pin1 = allocate_npin();
            npin_t* new_pin2 = allocate_npin();
            nnet_t* new_net = allocate_nnet();
            new_net->name = make_full_ref_name(NULL, NULL, NULL, new_node->name, 0);
            /* hook the output pin into the node */
            add_output_pin_to_node(new_node, new_pin1, 0);
            /* hook up new pin 1 into the new net */
            add_driver_pin_to_net(new_net, new_pin1);
            /* hook up the new pin 2 to this new net */
            add_fanout_pin_to_net(new_net, new_pin2);
        }
    }

    /* handle output pins indexed over input max width  */
    if (node->num_output_pins > max_width) {
        for (i = max_width; i < node->num_output_pins; i++) {
            nnode_t* buf_node = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
            /* hook a pin from PAD node into the buf node */
            add_input_pin_to_node(buf_node, get_pad_pin(netlist), 0);
            /* remap the extra output pin to buf node */
            remap_pin_to_new_node(node->output_pins[i], buf_node, 0);
        }
    }

    // CLEAN UP
    free_nnode(node);
}
