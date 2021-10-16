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
 * @file: this file proposes the combinational instantiation of constant
 * and variable exponentiation operations. Both implementations utilize 
 * the multiplication operations; however, the constant exponentiation is
 * designed to use fewer multiply nodes to achieve area and speed efficiency.
 */

#include "Power.hpp"
#include "node_creation_library.h"
#include "odin_util.h"
#include "netlist_utils.h"
#include "multipliers.h"
#include "vtr_memory.h"

static void implement_constant_exponentiation(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void implement_non_constant_exponentiation(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);

/**
 *-------------------------------------------------------------------------------------------
 * (function: resolve_power_node )
 * 
 * @brief if the base is power of two this function will perform a shift
 * operation as the exponentation. Otherwise it will cascade multipications 
 * nodes to implement the exponeniation. 
 * 
 * @param node pointing to the pow node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 *-----------------------------------------------------------------------------------------*/
void resolve_power_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes == 2);

    /**
     * POW ports:
     * 
     * A: input port [0]
     * B: input port [1]
     * 
     * Y: output port [0]
     */

    int i;
    int BASE_width = node->input_port_sizes[0];
    int EXPO_width = node->input_port_sizes[1];
    signal_list_t* base = init_signal_list();
    signal_list_t* exponent = init_signal_list();

    /* creating a list of base pins */
    for (i = 0; i < BASE_width; i++) {
        add_pin_to_signal_list(base, node->input_pins[i]);
    }

    /* creating a list of exponent pins */
    for (i = 0; i < EXPO_width; i++) {
        add_pin_to_signal_list(exponent, node->input_pins[BASE_width + i]);
    }

    /* check if the base is constant */
    if (is_constant_signal(base, netlist)) {
        /* check if the exponentiation is constant */
        if (is_constant_signal(exponent, netlist)) {
            pure_const_biops(node, netlist);
        } else {
            /* swap base and expo */
            swap_ports(node, 0, 1);
            implement_constant_exponentiation(node, traverse_mark_number, netlist);
        }
    } else if (is_constant_signal(exponent, netlist)) {
        /* check if the exponentiation is constant */
        implement_constant_exponentiation(node, traverse_mark_number, netlist);
    } else {
        implement_non_constant_exponentiation(node, traverse_mark_number, netlist);
    }

    // CLEAN UP
    free_signal_list(base);
    free_signal_list(exponent);
}

/**
 *-------------------------------------------------------------------------------------------
 * (function: implement_constant_exponentiation )
 * 
 * @brief instantiating multipication nodes as much 
 * as needed for exponention operation 
 * 
 * @param node pointing to the pow node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 *-----------------------------------------------------------------------------------------*/
static void implement_constant_exponentiation(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes == 2);

    /**                                                                                                                                                                                         
     *  <POW internal>                                                                                             *
     *   BASE: m bits                                                                                              *
     *   EXPO: constant                                                                                            *
     *                                                                                                             *
     *                              _____                                                                          *
     *                    BASE --- |     |                                                                         *
     *                  (PADDED)   | MUL |      _____                                                              *
     *                             |  1  | ----|     |      _____                                                  *
     *                    BASE --- |_____|     | MUL |     |     |                                                 *
     *                  (PADDED)               |  2  | ----| MUL |    ..    _____                                  *
     *                                BASE --- |_____|     |  3  | -- .. --|     |                                 *
     *                              (PADDED)      BASE --- |_____|    ..   | MUL | ----> OUTPUT                    *
     *                                          (PADDED)                   |exp-1|                                 *
     *                                                            BASE --- |_____|                                 *
     *                                                          (PADDED)                                           *
     *                                                                                                             *
     */
    /**
     * POW ports:
     * 
     * A: input port [0]
     * B: input port [1] (constant)
     * 
     * Y: output port [0]
     */

    int i, j;
    int BASE_width = node->input_port_sizes[0];
    int EXPO_width = node->input_port_sizes[1];
    int OUT_width = node->output_port_sizes[0];
    signal_list_t* base = init_signal_list();
    signal_list_t* exponent = init_signal_list();

    /* creating a list of exponent pins */
    for (i = 0; i < EXPO_width; i++) {
        add_pin_to_signal_list(exponent, node->input_pins[BASE_width + i]);
    }
    int pad_pin = BASE_width - 1;
    for (i = 0; i < OUT_width; i++) {
        /* here BASE (act as BASE^1) signal should be assigned to interanl output [1] */
        if (i < BASE_width) {
            /* adding pin BASE[i] */
            add_pin_to_signal_list(base, node->input_pins[i]);
        } else {
            /* for extra pins, padding with zero */
            add_pin_to_signal_list(base, (node->attributes->port_a_signed == operation_list::SIGNED)
                                             ? copy_input_npin(node->input_pins[pad_pin])
                                             : get_zero_pin(netlist));
        }
    }

    long exponent_value = constant_signal_value(exponent, netlist);
    nnode_t** mul_nodes = (nnode_t**)vtr::calloc((exponent_value - 1), sizeof(nnode_t*));
    signal_list_t** internal_outputs = (signal_list_t**)vtr::calloc(exponent_value - 1, sizeof(signal_list_t*));

    /* creating the mutipication chain */
    for (i = 0; i < exponent_value - 1; i++) {
        /**
         * here multipication output [i] should be assigned to interanl output [i] 
         * (BASE^2 ... BASE^(2^n-1)) 
         */
        mul_nodes[i] = make_2port_gate(MULTIPLY, OUT_width, OUT_width, OUT_width, node, traverse_mark_number);
        /* copy signedness */
        if (node->attributes->port_a_signed == operation_list::SIGNED)
            mul_nodes[i]->attributes->port_a_signed = SIGNED;
        /* Adding to mult_list for future checking on hard blocks */
        mult_list = insert_in_vptr_list(mult_list, mul_nodes[i]);
        /* initialize the inernal output signal list */
        internal_outputs[i] = init_signal_list();

        /* hook the inputs and output */
        for (j = 0; j < OUT_width; j++) {
            /* MUL inputs */
            if (i == 0) {
                /* first multipication is fed by two BASE signal */
                add_input_pin_to_node(mul_nodes[i], copy_input_npin(base->pins[j]), j);
                add_input_pin_to_node(mul_nodes[i], copy_input_npin(base->pins[j]), OUT_width + j);
            } else {
                add_input_pin_to_node(mul_nodes[i], internal_outputs[i - 1]->pins[j], j);
                add_input_pin_to_node(mul_nodes[i], copy_input_npin(base->pins[j]), OUT_width + j);
            }

            /* mul output */
            // specify not gate output pin
            if (i != exponent_value - 2) {
                npin_t* new_pin1 = allocate_npin();
                npin_t* new_pin2 = allocate_npin();
                nnet_t* new_net = allocate_nnet();
                new_net->name = make_full_ref_name(NULL, NULL, NULL, mul_nodes[i]->name, j);
                /* hook the output pin into the node */
                add_output_pin_to_node(mul_nodes[i], new_pin1, j);
                /* hook up new pin 1 into the new net */
                add_driver_pin_to_net(new_net, new_pin1);
                /* hook up the new pin 2 to this new net */
                add_fanout_pin_to_net(new_net, new_pin2);

                /* add the output pin to internal outputs */
                add_pin_to_signal_list(internal_outputs[i], new_pin2);
            } else {
                remap_pin_to_new_node(node->output_pins[j], mul_nodes[i], j);
            }
        }
    }

    // CLEAN UP
    /* detach pins from the pow node */
    for (i = 0; i < node->num_input_pins; i++) {
        /* container for input pin */
        npin_t* input_pin = node->input_pins[i];

        /* detach from pow node */
        node->input_pins[i] = NULL;
        /* detach from its net */
        remove_fanout_pins_from_net(input_pin->net, input_pin, input_pin->pin_net_idx);
        /* free extra pins */
        input_pin->node = NULL;
        free_npin(input_pin);
    }
    for (i = 0; i < exponent_value - 1; i++) {
        /* free signal list */
        free_signal_list(internal_outputs[i]);
    }
    vtr::free(internal_outputs);
    vtr::free(mul_nodes);

    free_signal_list(base);
    free_signal_list(exponent);
    free_nnode(node);
}

/**
 *-------------------------------------------------------------------------------------------
 * (function: implement_non_constant_exponentiation )
 * 
 * @brief implements a chain of multipliers to calculate different 
 * possible exponentiation values and output the one which is needed
 * using multiplexer with the exponent port as the selector
 * 
 * @param node pointing to the pow node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 *-----------------------------------------------------------------------------------------*/
static void implement_non_constant_exponentiation(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes == 2);

    /**                                                                                                                                                                                         
     *  <POW internal>                                                                                             *
     *   BASE: m bits                                                                                              *
     *   EXPO: n bits                                                                                              *
     *                                                                                                  EXPO       *
     *                                                                                                   |         *
     *     1 ---BASE^0---------------------------------------------------------------------------------|\|         *
     *       ---BASE^1---------------------------------------------------------------------------------| \         *
     *       |            ----BASE^2-------------------------------------------------------------------|  |        *
     *       |           |            ---BASE^3--------------------------------------------------------|  |        *
     *       |           |           |           --BASE^4----------------------------------------------|  | OUT    *
     *       |           |           |           |                                            ..       |  |-----   *
     *       |           |           |           |     ..                                     ..       |  |        *
     *       |           |           |           |     ..                                     ..       |  |        *
     *       |           |           |           |     ..      ----BASE^(2^n-3)------------------------|  |        *
     *       |           |           |           |             |           ---BASE^(2^n-2)-------------| /         *
     *       |           |           |           |             |           |           --BASE^(2^n-1)--|/          *
     *       |           |           |           |     ...     |           |           |                           *
     *       |           |           |           |             |           |           |          Multiport MUX    *
     *       |           |           |           |             |           |           |                           *
     *       |           |           |           |     ...     |           |           |                           *
     *       |           |           |           |             |           |           |                           *
     *       |           |           |           |             |           |           |                           *
     *       |   _____   |           |           |     ...     |           |           |                           *
     * BASE --- |     |  |   _____   |           |             |           |           |                           *
     *          | MUL | ----|     |  |   _____   |             |           |           |                           *
     * BASE --- |_____|     | MUL | ----|     |  |     _____   |           |           |                           *
     * (PADDED)    BASE --- |_____|     | MUL | -- .. -|     | |    _____  |           |                           *
     *            (PADDED)     BASE --- |_____|    ..  | MUL | ----|     | |    _____  |                           *
     *                       (PADDED)              ..  |_____|     | MUL | ----|     | |                           *
     *                                             ..     BASE --- |_____|     | MUL | ----                        *
     *                                                  (PADDED)      BASE --- |_____|                             *
     *                                                              (PADDED)                                       *
     */
    /**
     * POW ports:
     * 
     * A: input port [0]
     * B: input port [1]
     * 
     * Y: output port [0]
     */

    int i, j;
    int BASE_width = node->input_port_sizes[0];
    int EXPO_width = node->input_port_sizes[1];
    int OUT_width = node->output_port_sizes[0];

    int pow_two_by_EXPO_width = shift_left_value_with_overflow_check(0X1, EXPO_width, node->loc);
    nnode_t** mul_nodes = (nnode_t**)vtr::calloc((pow_two_by_EXPO_width - 2), sizeof(nnode_t*));
    signal_list_t** internal_outputs = (signal_list_t**)vtr::calloc(pow_two_by_EXPO_width, sizeof(signal_list_t*));

    /* creating the first and second internal outputs */
    internal_outputs[0] = init_signal_list();
    internal_outputs[1] = init_signal_list();
    int pad_pin = BASE_width - 1;
    for (i = 0; i < OUT_width; i++) {
        /* here a signal with value one (000...0001) should be assigned to interanl output [0] */
        add_pin_to_signal_list(internal_outputs[0], (i == 0) ? get_one_pin(netlist) : get_zero_pin(netlist));

        /* here BASE (act as BASE^1) signal should be assigned to interanl output [1] */
        if (i < BASE_width) {
            /* adding pin BASE[i] */
            add_pin_to_signal_list(internal_outputs[1], node->input_pins[i]);
        } else {
            /* for extra pins, padding with zero */
            add_pin_to_signal_list(internal_outputs[1], (node->attributes->port_a_signed == operation_list::SIGNED)
                                                            ? copy_input_npin(node->input_pins[pad_pin])
                                                            : get_zero_pin(netlist));
        }
    }

    /* creating the mutipication chain */
    for (i = 2; i < pow_two_by_EXPO_width; i++) {
        /**
         * here multipication output [i] should be assigned to interanl output [i] 
         * (BASE^2 ... BASE^(2^n-1)) 
         */
        mul_nodes[i - 2] = make_2port_gate(MULTIPLY, OUT_width, OUT_width, OUT_width, node, traverse_mark_number);
        /* copy signedness */
        if (node->attributes->port_a_signed == operation_list::SIGNED) {
            mul_nodes[i - 2]->attributes->port_a_signed = SIGNED;
            mul_nodes[i - 2]->attributes->port_b_signed = SIGNED;
        }
        /* Adding to mult_list for future checking on hard blocks */
        mult_list = insert_in_vptr_list(mult_list, mul_nodes[i - 2]);
        /* initialize the inernal output signal list */
        internal_outputs[i] = init_signal_list();

        /* hook the inputs and output */
        for (j = 0; j < OUT_width; j++) {
            /* mul input pins */
            add_input_pin_to_node(mul_nodes[i - 2], copy_input_npin(internal_outputs[i - 1]->pins[j]), j);
            add_input_pin_to_node(mul_nodes[i - 2], copy_input_npin(internal_outputs[1]->pins[j]), OUT_width + j);

            /* mul output */
            // specify not gate output pin
            npin_t* new_pin1 = allocate_npin();
            npin_t* new_pin2 = allocate_npin();
            nnet_t* new_net = allocate_nnet();
            new_net->name = make_full_ref_name(NULL, NULL, NULL, mul_nodes[i - 2]->name, j);
            /* hook the output pin into the node */
            add_output_pin_to_node(mul_nodes[i - 2], new_pin1, j);
            /* hook up new pin 1 into the new net */
            add_driver_pin_to_net(new_net, new_pin1);
            /* hook up the new pin 2 to this new net */
            add_fanout_pin_to_net(new_net, new_pin2);

            /* add the output pin to internal outputs */
            add_pin_to_signal_list(internal_outputs[i], new_pin2);
        }
    }

    /* creating the output Multiport MUX node */
    nnode_t* multiport_mux = allocate_nnode(node->loc);
    multiport_mux->type = MULTIPORT_nBIT_SMUX;
    multiport_mux->traverse_visited = traverse_mark_number;
    multiport_mux->name = node_name(multiport_mux, node->name);

    /* add selector port */
    add_input_port_information(multiport_mux, EXPO_width);
    allocate_more_input_pins(multiport_mux, EXPO_width);
    /* hook the exponent signal as selector to the mux node */
    for (i = 0; i < EXPO_width; i++) {
        remap_pin_to_new_node(node->input_pins[BASE_width + i], multiport_mux, i);
    }

    /* selector width offset */
    int offset = EXPO_width;
    /* hook the inputs into mux node */
    for (i = 0; i < pow_two_by_EXPO_width; i++) {
        /*  create input port */
        add_input_port_information(multiport_mux, OUT_width);
        allocate_more_input_pins(multiport_mux, OUT_width);
        /* connect input ports to the mux node */
        for (j = 0; j < OUT_width; j++) {
            if (i == 1 && j < BASE_width) {
                remap_pin_to_new_node(internal_outputs[i]->pins[j], multiport_mux, offset + j);
            } else {
                add_input_pin_to_node(multiport_mux, internal_outputs[i]->pins[j], offset + j);
            }
        }
        offset += OUT_width;
    }

    /*  create input port */
    add_output_port_information(multiport_mux, OUT_width);
    allocate_more_output_pins(multiport_mux, OUT_width);
    /* hook the output pins to the mux node */
    for (i = 0; i < OUT_width; i++) {
        remap_pin_to_new_node(node->output_pins[i], multiport_mux, i);
    }

    // CLEAN UP
    if (OUT_width < BASE_width) {
        for (i = OUT_width; i < BASE_width; i++) {
            /* container for input pin */
            npin_t* input_pin = node->input_pins[i];

            /* detach from pow node */
            node->input_pins[i] = NULL;
            /* detach from its net */
            remove_fanout_pins_from_net(input_pin->net, input_pin, input_pin->pin_net_idx);
            /* free extra pins */
            input_pin->node = NULL;
            free_npin(input_pin);
        }
    }
    for (i = 0; i < pow_two_by_EXPO_width; i++) {
        free_signal_list(internal_outputs[i]);
    }
    vtr::free(internal_outputs);
    vtr::free(mul_nodes);
    free_nnode(node);
}
