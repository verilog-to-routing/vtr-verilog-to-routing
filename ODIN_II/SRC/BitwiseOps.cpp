/**
 * Copyright (c) 2022 Seyed Alireza Damghani (sdamghann@gmail.com)
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
 * @file: This file provides utilities to modify the bitwise nodes 
 * to make them compatible with the Odin-II partial mapper.
 */

#include <cmath> // log2
#include "BitwiseOps.hpp"
#include "node_creation_library.h"
#include "odin_util.h"
#include "netlist_utils.h"
#include "vtr_memory.h"

/**
 * (function: decode_bitwise_nodes)
 * 
 * @brief decoding bitwise nodes into 2 inputs - 1 output nodes
 * 
 * @param node pointing to a bitwise node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
void decode_bitwise_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* /* netlist */) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes < 3);
    oassert(node->num_output_port_sizes == 1);
    oassert(node->num_output_pins == 1);

    // already satisfied the partial mapper requirement
    if (node->num_input_pins < 3 || node->num_input_port_sizes == 2)
        return;

    int i, j;
    /* keep the record of primary node info */
    int width = node->input_port_sizes[0];
    int levels = std::ceil(std::log2(width));

    /**
     *************************************************************************************************
     *                                                                                               *
     *                                              |\                                               *
     *                                       N bit  | \    1 bit                                     *
     *                                    in --'--> |  | ----'---->                                  *
     *                                              | /                                              *
     *                                              |/                                               *
     *                                                                                               *
     ***************************************** <decoded nodes> ***************************************
     *                                                                                               *
     *                                                                                               *
     *                                |\                                                             *
     *                         2 bits | \                                                            *
     *                       in --'-- |  | --                                                        *
     *                                | /               |\                                           *
     *                                |/         2 bits | \                                          *
     *                                            --'-- |  | --                                      *
     *                                |\                | /                                          *
     *                         2 bits | \               |/                                           *
     *                       in --'-- |  | --                                                        *
     *                                | /                                |\                          *
     *                                |/                          2 bits | \                         *
     *                                                             --'-- |  | --                     *
     *                                |\                                 | /                         *
     *                         2 bits | \                                |/                          *
     *                       in --'-- |  | --                                                        *
     *                                | /              |\                                            *
     *                                |/        2 bits | \                                           *
     *                                           --'-- |  | --                                       *
     *                                |\               | /                                           *
     *                         2 bits | \              |/                                            *
     *                       in --'-- |  | --                                                        *
     *                                | /                                                            *
     *                                |/                                                             *
     *                                                                                               *
     *                                ...              ...              ...                          *
     *                                                                                               *
     *                                ...              ...              ...                          *
     *                                                                                               *
     *                                ...              ...              ...                          *
     *************************************************************************************************/

    nnode_t*** decoded_nodes = (nnode_t***)vtr::calloc(levels, sizeof(nnode_t**));
    /* to keep the internal input signals for future usage */
    signal_list_t** input_signals = (signal_list_t**)vtr::calloc(levels + 1, sizeof(signal_list_t*));

    /* init the first input signals with the primary node input signals */
    input_signals[0] = init_signal_list();
    for (i = 0; i < width; ++i)
        add_pin_to_signal_list(input_signals[0], node->input_pins[i]);

    /* creating multiple stages to decode bitwise nodes into 2-1 nodes */
    for (i = 0; i < levels; ++i) {
        /* num of nodes in each stage */
        int num_of_nodes = width / 2;
        decoded_nodes[i] = (nnode_t**)vtr::calloc(num_of_nodes, sizeof(nnode_t*));
        input_signals[i + 1] = init_signal_list();

        /* iterating over each decoded node to connect inputs */
        for (j = 0; j < num_of_nodes; ++j) {
            /*****************************************************************************************
             * <Bitwise Node>                                                                        *
             *                                                                                       *
             *  Port 1                                                                               *
             *      0: in1                                   |\                                      *
             *      1: in2                            1 bit  | \                                     *
             *                                    in1 --'--> |  |  1 bit                             *
             *                                               |  |---'--->                            *
             *                                    in2 --'--> |  |                                    *
             *                                               | /                                     *
             *                                               |/                                      *
             ****************************************************************************************/
            decoded_nodes[i][j] = make_1port_gate(node->type, 2, 1, node, traverse_mark_number);

            if (input_signals[i]->pins[2 * j]->node)
                remap_pin_to_new_node(input_signals[i]->pins[2 * j], decoded_nodes[i][j], 0);
            else
                add_input_pin_to_node(decoded_nodes[i][j], input_signals[i]->pins[2 * j], 0);

            if (input_signals[i]->pins[2 * j + 1]->node)
                remap_pin_to_new_node(input_signals[i]->pins[2 * j + 1], decoded_nodes[i][j], 1);
            else
                add_input_pin_to_node(decoded_nodes[i][j], input_signals[i]->pins[2 * j + 1], 1);

            // Connect output pin to related input pin
            if (i != levels - 1) {
                npin_t* new_pin1 = allocate_npin();
                npin_t* new_pin2 = allocate_npin();
                nnet_t* new_net = allocate_nnet();
                new_net->name = make_full_ref_name(NULL, NULL, NULL, decoded_nodes[i][j]->name, -1);
                /* hook the output pin into the node */
                add_output_pin_to_node(decoded_nodes[i][j], new_pin1, 0);
                /* hook up new pin 1 into the new net */
                add_driver_pin_to_net(new_net, new_pin1);
                /* hook up the new pin 2 to this new net */
                add_fanout_pin_to_net(new_net, new_pin2);

                // Storing the output pins of the current node as the input of the next one
                add_pin_to_signal_list(input_signals[i + 1], new_pin2);

            } else {
                remap_pin_to_new_node(node->output_pins[0], decoded_nodes[i][j], 0);
            }
        }
        /* in case we have odd width, move the last pin to next level */
        if (width - 2 * j > 0) {
            oassert(width - 2 * j < 2);
            add_pin_to_signal_list(input_signals[i + 1], input_signals[i]->pins[width - 1]);
        }
        width = input_signals[i + 1]->count;
    }

    // CLEAN UP
    for (i = 0; i < levels; i++) {
        vtr::free(decoded_nodes[i]);
    }
    vtr::free(decoded_nodes);

    for (i = 0; i < levels + 1; i++) {
        free_signal_list(input_signals[i]);
    }
    vtr::free(input_signals);

    // to free primary node
    free_nnode(node);
}