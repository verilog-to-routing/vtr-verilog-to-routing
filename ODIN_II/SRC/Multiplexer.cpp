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
 * @file: this file provides utils to modify the multiplexer node 
 * to make it compatible with the Odin-II partial mapper. Moreover,
 * it includes the circuitry implementation of pmux, a cell designed
 * for parallel cases
 */

#include "Multiplexer.hpp"
#include "Shift.hpp"
#include "node_creation_library.h"
#include "odin_util.h"
#include "netlist_utils.h"
#include "vtr_memory.h"

/**
 * (function: make_selector_as_first_port)
 * 
 * @brief reorder the input signals of the mux in a way 
 * that the selector would come as the first signal
 * 
 * @param node pointing to a mux node 
 */
void make_selector_as_first_port(nnode_t* node) {
    long num_input_pins = node->num_input_pins;
    npin_t** input_pins = (npin_t**)vtr::malloc(num_input_pins * sizeof(npin_t*)); // the input pins

    int num_input_port_sizes = node->num_input_port_sizes;
    int* input_port_sizes = (int*)vtr::malloc(num_input_port_sizes * sizeof(int)); // info about the input ports

    int i, j;
    int acc_port_sizes = 0;
    i = num_input_port_sizes - 1;
    while (i > 0) {
        acc_port_sizes += node->input_port_sizes[i - 1];
        i--;
    }

    for (j = 0; j < node->input_port_sizes[num_input_port_sizes - 1]; j++) {
        input_pins[j] = node->input_pins[j + acc_port_sizes];
        input_pins[j]->pin_node_idx = j;
    }

    acc_port_sizes = 0;
    int offset = input_port_sizes[0] = node->input_port_sizes[num_input_port_sizes - 1];
    for (i = 1; i < num_input_port_sizes; i++) {
        input_port_sizes[i] = node->input_port_sizes[i - 1];

        for (j = 0; j < input_port_sizes[i]; j++) {
            input_pins[offset] = node->input_pins[j + acc_port_sizes];
            input_pins[offset]->pin_node_idx = offset;
            offset++;
        }
        acc_port_sizes += input_port_sizes[i];
    }

    // CLEAN UP
    vtr::free(node->input_pins);
    vtr::free(node->input_port_sizes);

    node->input_pins = input_pins;
    node->input_port_sizes = input_port_sizes;
}

/**
 * (function: resolve_pmux_node)
 * 
 * @brief resolving the pmux node including many input
 * multiplexing using a one-hot select signal
 * 
 * @param node pointing to a pmux node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */

void resolve_pmux_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);

    /**
     * input_pin[0]:  S (S_WIDTH)
     * input_pin[1]:  A (WIDTH)
     * input_pin[2]:  B (WIDTH*S_WIDTH)
     * output_pin[0]: Y (WIDTH)
     */
    int width = node->num_output_pins;
    int selector_width = node->input_port_sizes[0];

    int i, j;
    nnode_t** level_muxes = (nnode_t**)vtr::calloc(selector_width, sizeof(nnode_t*));
    signal_list_t** level_muxes_out_signals = (signal_list_t**)vtr::calloc(selector_width, sizeof(signal_list_t*));

    nnode_t** ternary_muxes = (nnode_t**)vtr::calloc(selector_width - 1, sizeof(nnode_t*));
    signal_list_t** ternary_muxes_out_signals = (signal_list_t**)vtr::calloc(selector_width - 1, sizeof(signal_list_t*));

    nnode_t** active_bit_muxes = (nnode_t**)vtr::calloc(selector_width - 1, sizeof(nnode_t*));
    signal_list_t* active_bit_muxes_out_signals = (signal_list_t*)vtr::calloc(selector_width - 1, sizeof(signal_list_t));

    /* Keeping signal B pins for future usage */
    signal_list_t* signal_B = init_signal_list();
    /* S_WIDTH + A_WIDTH */
    int offset = selector_width + width;
    for (j = 0; j < node->input_port_sizes[2]; j++) {
        add_pin_to_signal_list(signal_B, node->input_pins[offset + j]);
    }

    for (i = 0; i < selector_width; i++) {
        /**                                                                                                                                                                                         
         *          <pmux internal>                                                                          *
         *                                                                                                   *
         *                       S[0]                                                                        *
         *                        |                 S[1]                                                     *
         *                      |\|                  |                                S[2]                   *
         *                      | \                |\|                                 |                     *
         *               A ---  |0:|               | \                               |\|                     *
         *                      |  | ------------->|0:|                              | \                     *
         *               B ---  |1:|               |  | ---------------------------->|0:|                    *
         *          [width-1:0] | /          ----->|1:|                              |  | -->       ...      *
         *                      |/           |     | /                     --------->|1:|                    *
         *                (level_mux_0)      |     |/                      |         | /                     *
         *                                   |(level_mux_1)                |         |/                      *
         *                                   |                             |    (level_mux_1)                *
         *                       S[0]        |                             |                                 *
         *                        |          |                 S[1]        |                                 *
         *                      |\|          |                  |          |                                 *
         *                      | \          |                |\|          |                                 *
         *                0 --->|0:|         |                | \          |                                 *
         *                      |  |---------0--------------->|0:|         |                                 *
         *                1 --->|1:|   |     |                |  |---------0---->         ...                *
         *                      | /    |     |          1 --->|1:|   |     |                                 *
         *                      |/     |     |                | /    |     |                                 *
         *                 (active_bit |     |                |/     |     |                                 *
         *                   _mux_0)   |     |           (active_bit |     |                                 *
         *                             |     |              _mux_1)  |     |                                 *
         *                             |     |                       |     |                                 *
         *                           |\|     |                       |     |                                 *
         *                           | \     |                       |     |                                 *
         *              B >> 2^1 --->|0:|    |                     |\|     |                                 *
         *                           |  |-----                     | \     |                                 *
         *               'hxxx   --->|1:|             B >> 2^2 --->|0:|    |                                 *
         *                           | /                           |  |-----                                 *
         *                           |/                'hxxx   --->|1:|                                      *
         *                     (ternay_mux_0)                      | /                                       *
         *                                                         |/                                        *
         *                                                   (ternay_mux_1)                                  *
         *                                                                                                       
         */
        /**
         * (Level_muxes)
         * S: 1 bit
         * 0: width 
         * 1: width 
         * Out: width 
         */
        level_muxes[i] = make_3port_gate(MULTIPORT_nBIT_SMUX, 1, width, width, width, node, traverse_mark_number);
        level_muxes_out_signals[i] = (signal_list_t*)vtr::calloc(width, sizeof(signal_list_t));

        // Remapping the selector bit to level_mux
        remap_pin_to_new_node(node->input_pins[i],
                              level_muxes[i],
                              0);

        /* Assigning multiplexing inputs 0: and 1: */
        if (i == 0) {
            /* For the first stage the inputs are (0: A, 1: B >> 2^i) */
            for (j = 0; j < width; j++) {
                /* 0: A */
                // offset +1 is for the selector pin assgined above
                remap_pin_to_new_node(node->input_pins[j + selector_width],
                                      level_muxes[i],
                                      j + 1);

                /* 1: B */
                // will be assinged at the end of this function to handle mem leaks

                /* Specifying the level_muxes outputs */
                // Connect output pin to related input pin
                npin_t* new_pin1 = allocate_npin();
                npin_t* new_pin2 = allocate_npin();
                nnet_t* new_net = allocate_nnet();
                new_net->name = make_full_ref_name(NULL, NULL, NULL, level_muxes[i]->name, j);
                /* hook the output pin into the node */
                add_output_pin_to_node(level_muxes[i], new_pin1, j);
                /* hook up new pin 1 into the new net */
                add_driver_pin_to_net(new_net, new_pin1);
                /* hook up the new pin 2 to this new net */
                add_fanout_pin_to_net(new_net, new_pin2);

                // Storing the output pins of the current mux stage as the input of the next one
                add_pin_to_signal_list(level_muxes_out_signals[i], new_pin2);
            }

        } else {
            /*****************************************************************************************/
            /********************************** ACTIVE_BIT_MUXES *************************************/
            /*****************************************************************************************/
            /**
             * Creating the active_bit multiplexer which will
             * be connected as selector to ternary muxes 
             * (Active_bit_muxes)
             * S: 1 bit
             * 0: 1 bit
             * 1: 1 bit
             * Out: 1 bit
             */
            int active_idx = i - 1;
            active_bit_muxes[active_idx] = make_3port_gate(MULTIPORT_nBIT_SMUX, 1, 1, 1, 1, node, traverse_mark_number);

            // Remapping the selector bit to new multiplexer
            add_input_pin_to_node(active_bit_muxes[active_idx],
                                  copy_input_npin(level_muxes[i - 1]->input_pins[0]),
                                  0);

            if (active_idx == 0) {
                /* active_bit_mux[0] ->0: is 0 */
                add_input_pin_to_node(active_bit_muxes[active_idx],
                                      get_zero_pin(netlist),
                                      1);
            } else {
                /* active_bit_mux[1...] ->0: is connected to the previous active_bit_mux */
                add_input_pin_to_node(active_bit_muxes[active_idx],
                                      active_bit_muxes_out_signals->pins[active_idx - 1],
                                      1);
            }
            /* active_bit_mux[1...] ->1: is always 1 */
            add_input_pin_to_node(active_bit_muxes[active_idx],
                                  get_one_pin(netlist),
                                  2);

            // Connect output pin to related input pin
            npin_t* new_pin1 = allocate_npin();
            npin_t* new_pin2 = allocate_npin();
            nnet_t* new_net = allocate_nnet();
            new_net->name = make_full_ref_name(NULL, NULL, NULL, active_bit_muxes[active_idx]->name, 0);
            /* hook the output pin into the node */
            add_output_pin_to_node(active_bit_muxes[active_idx], new_pin1, 0);
            /* hook up new pin 1 into the new net */
            add_driver_pin_to_net(new_net, new_pin1);
            /* hook up the new pin 2 to this new net */
            add_fanout_pin_to_net(new_net, new_pin2);

            // Storing the output pins of the current mux stage as the input of the next one
            add_pin_to_signal_list(active_bit_muxes_out_signals, new_pin2);

            /*****************************************************************************************/
            /************************************* TERNARY_MUXES *************************************/
            /*****************************************************************************************/

            /**
             * Creating the ternary multiplexer which will
             * be connected to 1: port of level_muxes for i >= 1
             * (ternary_muxes)
             * S: 1 bit
             * 0: width
             * 1: width
             * Out: width
             */
            int ternary_idx = i - 1;
            ternary_muxes[ternary_idx] = make_3port_gate(MULTIPORT_nBIT_SMUX, 1, width, width, width, node, traverse_mark_number);
            ternary_muxes_out_signals[ternary_idx] = (signal_list_t*)vtr::calloc(width, sizeof(signal_list_t));

            // Remapping the selector bit to new multiplexer
            add_input_pin_to_node(ternary_muxes[ternary_idx],
                                  copy_input_npin(active_bit_muxes_out_signals->pins[ternary_idx]),
                                  0);

            /* Creating signals B >> 2^i */
            signal_list_t* signal_B_to_shift = init_signal_list();

            for (j = 0; j < signal_B->count; j++) {
                // choosing between node and level_muxes[0] is because we remap the B signal in i = 0
                add_pin_to_signal_list(signal_B_to_shift, copy_input_npin(signal_B->pins[j]));
            }
            signal_list_t* shifted_B = constant_shift(signal_B_to_shift, i, SR, width, UNSIGNED, netlist);

            /* Connecting multiplexing inputs of ternary_muxes, 0: B>>2^i, 1:'hxxx */
            for (j = 0; j < width; j++) {
                /* 0: B >> 2^i */
                add_input_pin_to_node(ternary_muxes[ternary_idx],
                                      shifted_B->pins[j],
                                      j + 1);

                /* 1: 'hxxxx' */
                add_input_pin_to_node(ternary_muxes[ternary_idx],
                                      get_pad_pin(netlist),
                                      j + width + 1);

                /* Specifying the ternary_muxes outputs */
                // Connect output pin to related input pin
                new_pin1 = allocate_npin();
                new_pin2 = allocate_npin();
                new_net = allocate_nnet();
                new_net->name = make_full_ref_name(NULL, NULL, NULL, ternary_muxes[ternary_idx]->name, j);
                /* hook the output pin into the node */
                add_output_pin_to_node(ternary_muxes[ternary_idx], new_pin1, j);
                /* hook up new pin 1 into the new net */
                add_driver_pin_to_net(new_net, new_pin1);
                /* hook up the new pin 2 to this new net */
                add_fanout_pin_to_net(new_net, new_pin2);

                // Storing the output pins of the current mux stage as the input of the next one
                add_pin_to_signal_list(ternary_muxes_out_signals[ternary_idx], new_pin2);
            }

            free_signal_list(shifted_B);

            /*****************************************************************************************/
            /************************************** LEVEL_MUXES **************************************/
            /*****************************************************************************************/

            for (j = 0; j < width; j++) {
                /* 0: previous level_mux outputs */
                add_input_pin_to_node(level_muxes[i],
                                      level_muxes_out_signals[i - 1]->pins[j],
                                      j + 1);

                /* 1: related ternary_mux */
                add_input_pin_to_node(level_muxes[i],
                                      ternary_muxes_out_signals[i - 1]->pins[j],
                                      j + width + 1);

                /* Specifying the level_muxes outputs */
                // Connect output pin to related input pin
                if (i != selector_width - 1) {
                    new_pin1 = allocate_npin();
                    new_pin2 = allocate_npin();
                    new_net = allocate_nnet();
                    new_net->name = make_full_ref_name(NULL, NULL, NULL, level_muxes[i]->name, j);
                    /* hook the output pin into the node */
                    add_output_pin_to_node(level_muxes[i], new_pin1, j);
                    /* hook up new pin 1 into the new net */
                    add_driver_pin_to_net(new_net, new_pin1);
                    /* hook up the new pin 2 to this new net */
                    add_fanout_pin_to_net(new_net, new_pin2);

                    // Storing the output pins of the current mux stage as the input of the next one
                    add_pin_to_signal_list(level_muxes_out_signals[i], new_pin2);

                } else {
                    remap_pin_to_new_node(node->output_pins[j], level_muxes[i], j);
                }
            }
        }
    }

    /* Remapping singal B to the first level_mux */
    for (j = 0; j < signal_B->count; j++) {
        /* 1: B */
        if (j < width) {
            remap_pin_to_new_node(signal_B->pins[j],
                                  level_muxes[0],
                                  j + width + 1);
        } else {
            remove_fanout_pins_from_net(signal_B->pins[j]->net,
                                        signal_B->pins[j],
                                        signal_B->pins[j]->pin_net_idx);

            if (signal_B->pins[j]->mapping)
                vtr::free(signal_B->pins[j]->mapping);
        }
    }

    // CLEAN_UP
    for (i = 0; i < selector_width; i++) {
        free_signal_list(level_muxes_out_signals[i]);

        if (i < selector_width - 1) {
            free_signal_list(ternary_muxes_out_signals[i]);
        }
    }
    free_signal_list(signal_B);
    free_signal_list(active_bit_muxes_out_signals);

    free_nnode(node);
    vtr::free(level_muxes);
    vtr::free(ternary_muxes);
    vtr::free(active_bit_muxes);

    vtr::free(level_muxes_out_signals);
    vtr::free(ternary_muxes_out_signals);
}

/**
 * (function: transform_to_single_bit_mux_nodes)
 * 
 * @brief split the mux node read from yosys blif to
 * the same type nodes with input/output width one
 * 
 * @param node pointing to the mux node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
nnode_t** transform_to_single_bit_mux_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* /* netlist */) {
    oassert(node->traverse_visited == traverse_mark_number);

    int i, j;
    /* to check all mux inputs have the same width(except [0] which is selector) */
    for (i = 2; i < node->num_input_port_sizes; i++) {
        oassert(node->input_port_sizes[i] == node->input_port_sizes[1]);
    }

    int selector_width = node->input_port_sizes[0];
    int num_input_ports = node->num_input_port_sizes;
    int num_mux_nodes = node->num_output_pins;

    nnode_t** mux_node = (nnode_t**)vtr::calloc(num_mux_nodes, sizeof(nnode_t*));

    /**
     * input_port[0] -> SEL
     * input_pin[SEL_WIDTH..n] -> MUX inputs
     * output_pin[0..n-1] -> MUX outputs
     */
    for (i = 0; i < num_mux_nodes; i++) {
        mux_node[i] = allocate_nnode(node->loc);

        mux_node[i]->type = node->type;
        mux_node[i]->traverse_visited = traverse_mark_number;

        /* Name the mux based on the name of its output pin */
        // const char* mux_base_name = node_name_based_on_op(mux_node[i]);
        // mux_node[i]->name = (char*)vtr::malloc(sizeof(char) * (strlen(node->output_pins[i]->name) + strlen(mux_base_name) + 2));
        // odin_sprintf(mux_node[i]->name, "%s_%s", node->output_pins[i]->name, mux_base_name);
        mux_node[i]->name = node_name(mux_node[i], node->name);

        add_input_port_information(mux_node[i], selector_width);
        allocate_more_input_pins(mux_node[i], selector_width);

        if (i == num_mux_nodes - 1) {
            /**
             * remap the SEL pins from the mux node 
             * to the last splitted mux node  
             */
            for (j = 0; j < selector_width; j++) {
                remap_pin_to_new_node(node->input_pins[j], mux_node[i], j);
            }

        } else {
            /* add a copy of SEL pins from the mux node to the splitted mux nodes */
            for (j = 0; j < selector_width; j++) {
                add_input_pin_to_node(mux_node[i], copy_input_npin(node->input_pins[j]), j);
            }
        }

        /**
         * remap the input_pin[i+1]/output_pin[i] from the mux node to the 
         * last splitted ff node since we do not need it in dff node anymore 
         **/
        int acc_port_sizes = selector_width;
        for (j = 1; j < num_input_ports; j++) {
            add_input_port_information(mux_node[i], 1);
            allocate_more_input_pins(mux_node[i], 1);

            remap_pin_to_new_node(node->input_pins[i + acc_port_sizes], mux_node[i], selector_width + j - 1);
            acc_port_sizes += node->input_port_sizes[j];
        }

        /* output */
        add_output_port_information(mux_node[i], 1);
        allocate_more_output_pins(mux_node[i], 1);

        remap_pin_to_new_node(node->output_pins[i], mux_node[i], 0);
    }

    // CLEAN UP
    free_nnode(node);

    return mux_node;
}
