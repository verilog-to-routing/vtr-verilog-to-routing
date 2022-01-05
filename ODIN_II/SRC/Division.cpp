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
 *
 * @file Division.cpp comprises the combinational implementation of 
 * Division operation using shift and subtraction nodes. To utilize
 * this routine of this file, a high-level RTL DIV node is required
 * with port order according to what is mentioned in resolve_div_node.
 * Currently, this file is used by Yosys generated division sub-circuit.
 */

#include "Division.hpp"
#include "node_creation_library.h"
#include "odin_util.h"
#include "netlist_utils.h"
#include "vtr_memory.h"

static void make_CR_node(nnode_t* node, signal_list_t* input_signal_list, signal_list_t* output_signal_list);
static signal_list_t* CR_output_signal_init();
static signal_list_t** modify_div_signal_sizes(nnode_t* node, netlist_t* netlist);
static signal_list_t** implement_division(nnode_t* node, signal_list_t** input_signals, netlist_t* netlist);
static void connect_div_output_pins(nnode_t* node, signal_list_t** output_signals, uintptr_t traverse_mark_number, netlist_t* netlist);

/**
 * (function: resolve_divide_node)
 * 
 * @brief this function resolves a high-level RTL divison node.
 * the node should follow the same port ordering as mentioned below.
 * This function first modifies the divison node signals to check the 
 * required restriction, then it implements division circuitry and 
 * connects outputs accordingly.
 *
 * The division circuit design comes from section 2.6.1 Combinational Divisors
 * of Algebric Circuits by Ruiz et. al 
 * [https://www.springer.com/gp/book/9783642546488]
 * 
 * @param node pointing to a div node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
void resolve_divide_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);

    /**
     * Div Ports:
     * IN1: Dividend (m bits) 
     * IN2: Divisor  (n bits) 
     * OUT: Quotient (k bits)
     * 
     * checking the division restrictions 
     * 
     * 1. m == 2*n - 1
     * 2. divisor MSB == 1 (left)
     * 3. n == k
     */
    /**
     * modify div input signals to provide compatibility 
     * with the first and third restrictions.
     * [0]: Modified Dividend
     * [1]: Modified Divisor
     */
    signal_list_t** modified_input_signals = modify_div_signal_sizes(node, netlist);

    /* implementation of the divison circuitry using cellular architecture */
    signal_list_t** div_output_lists = implement_division(node, modified_input_signals, netlist);

    /* remap the div output pin to calculated nodes */
    connect_div_output_pins(node, div_output_lists, traverse_mark_number, netlist);

    // CLEAN UP
    free_nnode(node);
}

/**
 * (function: modify_div_signal_sizes)
 * 
 * @brief adjusting the div node signal sizes to make it 
 * compatible with the first division restrictions,
 * i.e. m = 2*n-1
 * 
 * @param node pointer to the div node
 * @param netlist pointer to the current netlist file
 * 
 * @return modified signal lists [0]: dividend [1]: divisor
 */
static signal_list_t** modify_div_signal_sizes(nnode_t* node, netlist_t* netlist) {
    int i;

    int dividend_width = node->input_port_sizes[0];
    int divisor_width = node->input_port_sizes[1];

    /* new widths which will be adjusted in the following */
    int new_dividend_width = dividend_width;
    int new_divisor_width = divisor_width;
    /* m = 2*n -1 */
    while (dividend_width > 2 * new_divisor_width - 1) {
        new_divisor_width++;
    }
    /* need for padding dividend with GND */
    while (new_dividend_width < 2 * new_divisor_width - 1)
        new_dividend_width++;

    signal_list_t* dividend_signal_list = init_signal_list();
    signal_list_t* divisor_signal_list = init_signal_list();

    /* Dividend will be padded from the MSB (left side) */
    /* hook dividend pins into corresponding signal list */
    for (i = 0; i < new_dividend_width; i++) {
        if (i < dividend_width) {
            add_pin_to_signal_list(dividend_signal_list, node->input_pins[i]);
        } else {
            add_pin_to_signal_list(dividend_signal_list, get_zero_pin(netlist));
        }
    }

    /* Divisor will be padded from the MSB (left side) */
    /* hook divisor pins into new node (literally, shift left if needed) */
    for (i = 0; i < new_divisor_width; i++) {
        if (i < divisor_width) {
            add_pin_to_signal_list(divisor_signal_list, node->input_pins[dividend_width + i]);
        } else {
            add_pin_to_signal_list(divisor_signal_list, get_zero_pin(netlist));
        }
    }

    /* creating the return list of signal lists */
    signal_list_t** return_sig_list = (signal_list_t**)vtr::calloc(2, sizeof(signal_list_t*));
    return_sig_list[0] = dividend_signal_list;
    return_sig_list[1] = divisor_signal_list;

    return (return_sig_list);
}

/**
 * (function: implement_division)
 * 
 * @brief creating division node (A / B)
 * In high-level, the division is implemented using series of shift and subtractors.
 * After shifting the quotient, it is deducted from the the dividend and the remainder.
 * properly transferred to the next level. With n bit divisor the divison is performed 
 * in n-steps.
 * 
 * @note this should perfom before partial mapping since
 * some nodes like minus are not resolved yet
 * 
 * @param node pointer to the div node
 * @param input_signals modified input signals of the div node
 * @param netlist pointer to the current netlist file
 * 
 * @return two signal lists -> [0]: quotient pins [1]: remainder pins
 */
static signal_list_t** implement_division(nnode_t* node, signal_list_t** input_signals, netlist_t* netlist) {
    /* to be returned signal lists */
    signal_list_t* quotient_signal_list = init_signal_list();
    signal_list_t* remainder_signal_list = init_signal_list();

    /**
     * (Modified DIV ports)
     * 
     * IN1: Dividend (m = 2n-1 bits)
     * IN2: Divisor (n bits)
     * OUT1: Quotient (n bits)
     * OUT2: Remainder (n bits)
     */

    int i, j;
    signal_list_t* dividend_sig_list = input_signals[0];
    signal_list_t* divisor_sig_list = input_signals[1];

    int dividend_size = dividend_sig_list->count;
    int divisor_size = divisor_sig_list->count;

    /* checking the division circuit restrictions */
    oassert(dividend_size == 2 * divisor_size - 1);

    /* CR nodes array for cellular architecture (need divisor_size row) */
    signal_list_t*** CR_outputs = (signal_list_t***)vtr::calloc(divisor_size, sizeof(signal_list_t**));

    /* creating CR nodes for each row to do the partial division */
    int offset = divisor_size;

    /* creating the network */
    for (i = 0; i < divisor_size; i++) {
        /**
         * <DIV internal circuit>                                                                                                          *
         *        D[m-1] d[n-1]          D[m-n-1] d[n-2]           D[m-n] d[0]                                                             *
         *           |    |                |    |                   |    |                                                                 *
         *           |    |                |    |                   |    |                                                                 *
         *     b    _v____v_   b_     b   _v____v_    b_      b    _v____v_    b_                                                          *
         *  <------|   CR   |<-----------|   CR   |<----    ------|   CR   |<--                                                            *
         * |       |[0][n-1]|            |[0][n-2]|     ....      | [0][0] |                                                               *
         * |__|\___|  i  j  |            |  i  j  |     ....      |  i  j  |                                                               *
         *    |/   |________|----------->|________|-----    ----->|________|-- c_2                                                         *
         *      _c     |        c    _c      |       c     _c         |                                                                    *
         *             |                     |                        |                                                                    *
         *             |    __________       |  d[n-1]                |  d[1]      D[m-n+1] d[0]                                           *
         *             |    |        |       |    |                   |    |             |    |                                            *
         *             |    |        |       |    |                   |    |             |    |                                            *
         *       b    _v____v_       |  b   _v____v_   b_       b    _v____v_    b_ b   _v____v_    b_                                     *
         *    <------|   CR   |<--   ------|   CR   |<---    -------|   CR   |<--------|   CR   |<-------                                  *
         *   |       | [1][n] |  |         |[1][n-1]|    ....       | [1][1] |         | [1][0] |                                          *
         *   |__|\___|  i  j  |  gnd       |  i  j  |    ....       |  i  j  |         |  i  j  |                                          *
         *      |/   |________|----------->|________|----  | ------>|________|-------->|________|--> c[1]                                  *
         *        _c               c       _c  |      c    |   _c       |_______ c   _c       |                                            *
         *                                     |           |___                 |             |                                            *
         *                                     |    __________ |_____   d[n-1]  |             |  d[1]      D[m-n+1] d[0]                   *
         *                                     |    |        |       |    |     |__           |    |             |    |                    *
         *                                     |    |        |       |    |       |           |    |             |    |                    *
         *                               b    _v____v_       |  b   _v____v_   b_ |     b    _v____v_    b_ b   _v____v_    b_             *
         *                            <------|   CR   |<--   ------|   CR   |<--- |  -------|   CR   |<--------|   CR   |<-------          *
         *                           |       | [2][n] |  |         |[2][n-1]|    ....       | [2][1] |         | [1][0] |                  *
         *                           |__|\___|  i  j  |  gnd       |  i  j  |    ....       |  i  j  |         |  i  j  |                  *
         *                              |/   |________|----------->|________|----    ------>|________|-------->|________|--> c[0]          *
         *                                _c               c    _c     |      c             _c   |      c   _c      |                      *
         *                                                             |                         |                  |                      *
         *                                                           r_n-1                     r_1                r_0                      *
         *                                      ....                  ....                     ....                ....                    *
         *                                      ....                  ....                     ....                ....                    *
         *                                      ....                  ....                     ....                ....                    *
         */

        /* In the first row, the divisor is not shifting, so we need one less CR node */
        int num_CR_per_row = (i == 0) ? (divisor_size) : (divisor_size + 1);
        CR_outputs[i] = (signal_list_t**)vtr::calloc(num_CR_per_row, sizeof(signal_list_t*));

        for (j = 0; j < num_CR_per_row; j++) {
            /* allocating each CR output pins */
            CR_outputs[i][j] = CR_output_signal_init();
        }

        for (j = 0; j < num_CR_per_row; j++) {
            /** 
             * <CR inputs>
             * [0] = IN1 (D) [m - offset + j]
             * [1] = IN2 (d) [j]
             * [2] = b_      [previous CR borrow out]
             * [3] = _c      [next CR quotiont]
             */
            int dividend_idx = dividend_size - offset + j;
            int divisor_idx = (i == 0) ? (j) : (j == num_CR_per_row - 1) ? (-1) : (j);

            /* setting up containers for CR outptu pins */
            npin_t* b = CR_outputs[i][j]->pins[0];
            npin_t* p = CR_outputs[i][j]->pins[1];
            npin_t* c = CR_outputs[i][j]->pins[2];

            /* specifying the CR input pins */
            signal_list_t* CR_node_input_signals = init_signal_list();
            /* containers for input pins to clarify the code */
            npin_t* x = NULL;  /* CR_node_input_signals->pins[0] */
            npin_t* y = NULL;  /* CR_node_input_signals->pins[1] */
            npin_t* b_ = NULL; /* CR_node_input_signals->pins[2] */
            npin_t* _c = NULL; /* CR_node_input_signals->pins[3] */

            /* allocating input pins for the first row in different since they should connect to Dividend */
            /***** x *****/
            if (i == 0 || j == 0) {
                x = dividend_sig_list->pins[dividend_idx];
            } else {
                /* partial product of the previous row corresponding CR */
                x = CR_outputs[i - 1][j - 1]->pins[1];
            }
            add_pin_to_signal_list(CR_node_input_signals, x);
            /***** y *****/
            if (i != 0 && j == num_CR_per_row - 1) {
                /* borrow output of the previous CR */
                y = CR_outputs[i][j - 1]->pins[0];
            } else {
                if (i == divisor_size - 1)
                    y = divisor_sig_list->pins[divisor_idx];
                else
                    y = copy_input_npin(divisor_sig_list->pins[divisor_idx]);
            }
            add_pin_to_signal_list(CR_node_input_signals, y);
            /***** b_ *****/
            if (j == 0) {
                b_ = get_zero_pin(netlist);
            } else {
                if (i != 0 && j == num_CR_per_row - 1)
                    b_ = get_zero_pin(netlist);
                else
                    /* borrow output of the previous CR */
                    b_ = CR_outputs[i][j - 1]->pins[0];
            }
            add_pin_to_signal_list(CR_node_input_signals, b_);
            /***** _c *****/
            /* making not gate with input b to drive c for the last CR node in each row */
            if (j == num_CR_per_row - 1) {
                /* creating a not gate to connect last borrow to the last c_ */
                nnode_t* not_node = make_inverter(b, node, node->traverse_visited);
                /* hook the not node output to the last CR node _c input*/
                _c = not_node->output_pins[0]->net->fanout_pins[0];
            } else {
                _c = CR_outputs[i][j + 1]->pins[2];
            }
            add_pin_to_signal_list(CR_node_input_signals, _c);

            /**
             *  Making the CR node located at row i and col j
             */
            make_CR_node(node, CR_node_input_signals, CR_outputs[i][j]);

            /*********************************************************/
            /* adding the network outputs to the return signal lists */
            /*********************************************************/
            /* Quotient */
            if (j == 0) {
                /* adding to output signal list for returning */
                add_pin_to_signal_list(quotient_signal_list, c);
            }
            /* Remanider */
            if (i == divisor_size - 1 && j != num_CR_per_row - 1) {
                /* adding to output signal list for returning */
                add_pin_to_signal_list(remainder_signal_list, p);
            }

            // CLEAN UP
            free_signal_list(CR_node_input_signals);
        }

        /**
         * increasing the offset by 1 in each iteration to 
         * connect all dividend pins to the first row CR 
         * nodes and all first CR nodes in other rows 
         */
        offset++;
    }

    // CLEAN UP
    /* input signal list comes from modify div input size function */
    for (i = 0; i < 2; i++) {
        free_signal_list(input_signals[i]);
    }
    vtr::free(input_signals);
    /* local variables */
    for (i = 0; i < divisor_size; i++) {
        /* In the first row, the divisor is not shifting, so we need one less CR node */
        int num_CR_per_row = (i == 0) ? (divisor_size) : (divisor_size + 1);
        /* free each signal list */
        for (j = 0; j < num_CR_per_row; j++) {
            free_signal_list(CR_outputs[i][j]);
        }
        vtr::free(CR_outputs[i]);
    }
    vtr::free(CR_outputs);

    /* retrun signal lists -> [0]: quotient pins [1]: remainder pins */
    signal_list_t** return_sig_list = (signal_list_t**)vtr::calloc(2, sizeof(signal_list_t*));
    return_sig_list[0] = quotient_signal_list;
    return_sig_list[1] = remainder_signal_list;

    return (return_sig_list);
}

/**
 * (function: connect_div_output_pins)
 * 
 * @brief remapping the main div output pins to 
 * the calculated quotient and remainder node
 * 
 * @param node pointer to the div node
 * @param output_signals calculated output signals of the div node
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 */
static void connect_div_output_pins(nnode_t* node, signal_list_t** output_signals, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(output_signals);

    int i;
    int actual_width = node->output_port_sizes[0];

    /* quotient and remainder signal lists returned by implement_division function */
    signal_list_t* quotient_signal_list = output_signals[0];
    signal_list_t* remainder_signal_list = output_signals[1];

    int new_quotient_width = quotient_signal_list->count;
    int new_remainder_width = remainder_signal_list->count;

    switch (node->type) {
        case DIVIDE: {
            int quotient_width = actual_width;
            /* checking the adjusted width with actual one to connect properly */
            if (quotient_width >= new_quotient_width) {
                for (i = 0; i < quotient_width; i++) {
                    /* creating a buf node to cionnect the calculated quotient to the main div node outputs */
                    nnode_t* buf_node = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);

                    if (i < new_quotient_width) {
                        npin_t* quotient_pin = quotient_signal_list->pins[i];
                        /* connect the calculatd quotient pin as buf node driver */
                        add_input_pin_to_node(buf_node, quotient_pin, 0);
                        /* remap the main div output pin to the buf node output pin */
                        remap_pin_to_new_node(node->output_pins[new_quotient_width - i - 1], buf_node, 0);
                    } else {
                        /* connect the calculatd quotient pin as buf node driver */
                        add_input_pin_to_node(buf_node, get_zero_pin(netlist), 0);
                        /* remap the main div output pin to the buf node output pin */
                        remap_pin_to_new_node(node->output_pins[i], buf_node, 0);
                    }
                }

            } else {
                /* keep the record of extension size for future usage */
                int extension_size = new_quotient_width - quotient_width;

                for (i = 0; i < new_quotient_width; i++) {
                    npin_t* quotient_pin = quotient_signal_list->pins[i];
                    /* creating a buf node to cionnect the calculated quotient to the main div node outputs */
                    nnode_t* buf_node = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);

                    if (i >= extension_size) {
                        add_input_pin_to_node(buf_node, quotient_pin, 0);
                        /* remap the main div output pin to the buf node output pin */
                        remap_pin_to_new_node(node->output_pins[new_quotient_width - i - 1], buf_node, 0);
                    } else {
                        /* dump the calculated pin since it is extra */
                        remove_fanout_pins_from_net(quotient_pin->net,
                                                    quotient_pin,
                                                    quotient_pin->pin_net_idx);
                        /* CLEAN UP*/
                        free_npin(quotient_pin);
                    }
                }
            }

            /* dunp remainders */
            for (i = 0; i < new_remainder_width; i++) {
                npin_t* remainder_pin = remainder_signal_list->pins[i];
                remove_fanout_pins_from_net(remainder_pin->net,
                                            remainder_pin,
                                            remainder_pin->pin_net_idx);

                /* CLEAN UP*/
                free_npin(remainder_pin);
            }

            break;
        }
        case MODULO: {
            int remainder_width = actual_width;
            /* checking the adjusted width with actual one to connect properly */
            if (remainder_width >= new_remainder_width) {
                for (i = 0; i < remainder_width; i++) {
                    /* creating a buf node to cionnect the calculated remainder to the main div node outputs */
                    nnode_t* buf_node = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);

                    if (i < new_remainder_width) {
                        npin_t* remainder_pin = remainder_signal_list->pins[i];
                        /* connect the calculatd remainder pin as buf node driver */
                        add_input_pin_to_node(buf_node, remainder_pin, 0);
                        /* remap the main div output pin to the buf node output pin */
                        remap_pin_to_new_node(node->output_pins[i], buf_node, 0);
                    } else {
                        /* connect the calculatd remainder pin as buf node driver */
                        add_input_pin_to_node(buf_node, get_zero_pin(netlist), 0);
                        /* remap the main div output pin to the buf node output pin */
                        remap_pin_to_new_node(node->output_pins[i], buf_node, 0);
                    }
                }

            } else {
                /* remainder_width < new_remainder_width */
                for (i = 0; i < new_remainder_width; i++) {
                    npin_t* remainder_pin = remainder_signal_list->pins[i];
                    /* creating a buf node to cionnect the calculated remainder to the main div node outputs */
                    nnode_t* buf_node = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);

                    if (i < remainder_width) {
                        add_input_pin_to_node(buf_node, remainder_pin, 0);
                        /* remap the main div output pin to the buf node output pin */
                        remap_pin_to_new_node(node->output_pins[i], buf_node, 0);
                    } else {
                        /* dump the calculated pin since it is extra */
                        remove_fanout_pins_from_net(remainder_pin->net,
                                                    remainder_pin,
                                                    remainder_pin->pin_net_idx);
                        /* CLEAN UP*/
                        free_npin(remainder_pin);
                    }
                }
            }

            /* dunp quotient */
            for (i = 0; i < new_quotient_width; i++) {
                npin_t* quotient_pin = quotient_signal_list->pins[i];
                remove_fanout_pins_from_net(quotient_pin->net,
                                            quotient_pin,
                                            quotient_pin->pin_net_idx);

                /* CLEAN UP*/
                free_npin(quotient_pin);
            }

            break;
        }
        default:
            break;
    }

    // CLEAN UP
    for (i = 0; i < 2; i++) {
        free_signal_list(output_signals[i]);
    }
    vtr::free(output_signals);
}

/**
 * (function: make_CR_node)
 * 
 * @brief creating a single bit CR cell required 
 * by the division cellular network
 * 
 * @param node the unresolved div node
 * @param input_signal_list the list including the CR input pins
 *        [0] = x 
 *        [1] = y 
 *        [2] = b_
 *        [3] = _c
 * 
 * @param output_signal_list [0] = b
 *                           [1] = p
 *                           [2] = c 
 */
static void make_CR_node(nnode_t* node, signal_list_t* input_signal_list, signal_list_t* output_signal_list) {
    /* CR  node has fixed four inputs */
    oassert(input_signal_list->count == 4);

    /**
     * <CR interior design>                                                                                  *
     * <all wires are 1 bit wide>                                                                            *
     *                                                                                                       *
     *                                                      x      y                                         *
     *                                         _____________|      |                                         *
     *                                        |             |      |                                         *
     *                                        |           __v______v__                                       *
     *                                        |          |    FULL    |                                      *
     *                                        |   b  <---| SUBTRACTOR |<--- b_                               *
     *                                        |          |____________|                                      *
     *                                        |                 |                                            *
     *                                        |             ____|                                            *
     *                                        |            |                                                 *
     *                                        |            |                                                 *
     *                                        |            |                                                 *
     *                                     ___v____________v____                                             *
     *                                     \  0:           1:  /                                             *
     *                              _c ---> \       MUX       / ---> c                                       *
     *                                       \_______________/                                               *
     *                                               |                                                       *
     *                                               |                                                       *
     *                                               v                                                       *
     *                                               p                                                       *
     *                                                                                                       *
     */

    /* CR input pins */
    npin_t* x = input_signal_list->pins[0];
    npin_t* y = input_signal_list->pins[1];
    npin_t* b_ = input_signal_list->pins[2];
    npin_t* _c = input_signal_list->pins[3];

    /* CR input pins */
    npin_t* b = output_signal_list->pins[0]->net->driver_pins[0];
    npin_t* p = output_signal_list->pins[1]->net->driver_pins[0];
    npin_t* c = output_signal_list->pins[2]->net->driver_pins[0];

    /*********************************************************************************************************
     ********************************************* FULL SUBTRACTOR *******************************************
     *********************************************************************************************************/
    /* creating the full subtractor node */
    nnode_t* fs_node = make_3port_gate(MINUS, 1, 1, 1, 1, node, node->traverse_visited);
    /* allocating one more output port indicating the borrow out */
    add_output_port_information(fs_node, 1);
    allocate_more_output_pins(fs_node, 1);

    /* connecting the input pins */
    if (x->node)
        remap_pin_to_new_node(x, fs_node, 0);
    else
        add_input_pin_to_node(fs_node, x, 0);

    if (y->node)
        remap_pin_to_new_node(y, fs_node, 1);
    else
        add_input_pin_to_node(fs_node, y, 1);

    if (b_->node)
        remap_pin_to_new_node(b_, fs_node, 2);
    else
        add_input_pin_to_node(fs_node, b_, 2);

    /** 
     * connecting output pins 
     * [0]: b
     * [1]: fs_out
     */
    /* b is already created so we only need to hook it up to the node */
    add_output_pin_to_node(fs_node, b, 1);
    b->net->name = make_full_ref_name(NULL, NULL, NULL, fs_node->name, 1);

    /* need to create the fs_output_pin as an internal pin */
    npin_t* new_pin1 = allocate_npin();
    npin_t* fs_output_pin = allocate_npin();
    nnet_t* new_net = allocate_nnet();
    new_net->name = make_full_ref_name(NULL, NULL, NULL, fs_node->name, 0);
    /* hook the output pin into the node */
    add_output_pin_to_node(fs_node, new_pin1, 0);
    /* hook up new pin 1 into the new net */
    add_driver_pin_to_net(new_net, new_pin1);
    /* hook up the new pin 2 to this new net */
    add_fanout_pin_to_net(new_net, fs_output_pin);

    /*********************************************************************************************************
     ************************************************** MUX **************************************************
     *********************************************************************************************************/
    /* creating the multiplexer node */
    nnode_t* mux_node = make_3port_gate(MULTIPORT_nBIT_SMUX, 1, 1, 1, 1, node, node->traverse_visited);
    /* hook the selector c_ into the mux node */
    if (_c->node)
        remap_pin_to_new_node(_c, mux_node, 0);
    else
        add_input_pin_to_node(mux_node, _c, 0);
    /* hook the mux input pins */
    add_input_pin_to_node(mux_node, copy_input_npin(x), 1);
    add_input_pin_to_node(mux_node, fs_output_pin, 2);

    /**
     * connecting mux output pin
     * [0]: p 
     */
    /* p is already created so we only need to hook it up to the node */
    add_output_pin_to_node(mux_node, p, 0);
    p->net->name = make_full_ref_name(NULL, NULL, NULL, mux_node->name, 0);

    /*********************************************************************************************************
     *********************************************** BUF NODE ************************************************
     *********************************************************************************************************/
    /* creating a buf node for c */
    nnode_t* buf_node = make_1port_gate(BUF_NODE, 1, 1, node, node->traverse_visited);
    /* hook a copy of c_ into the buf node */
    add_input_pin_to_node(buf_node, copy_input_npin(mux_node->input_pins[0]), 0);

    /**
     * connecting the output pin
     * [0]: c
     */
    /* p is already created so we only need to hook it up to the node */
    add_output_pin_to_node(buf_node, c, 0);
    c->net->name = make_full_ref_name(NULL, NULL, NULL, buf_node->name, 0);
}

/**
 * (function: CR_output_signal_init)
 * 
 * @brief initialize the output signals of each CR node
 * 
 * @return output signals
 *         [0] = b
 *         [1] = p
 *         [2] = c 
 */
static signal_list_t* CR_output_signal_init() {
    /* signal list to be returned */
    signal_list_t* return_sig_list = init_signal_list();

    npin_t* b = allocate_npin();
    npin_t* b_out = allocate_npin();
    nnet_t* b_net = allocate_nnet();
    /* hook up c into the c net */
    add_driver_pin_to_net(b_net, b);
    /* hook up the c_out to this c_net */
    add_fanout_pin_to_net(b_net, b_out);
    /* storing in the CR output signal for CR creation function */
    add_pin_to_signal_list(return_sig_list, b_out);

    npin_t* p = allocate_npin();
    npin_t* p_out = allocate_npin();
    nnet_t* p_net = allocate_nnet();
    /* hook up p into the p net */
    add_driver_pin_to_net(p_net, p);
    /* hook up the p_out to this p_net */
    add_fanout_pin_to_net(p_net, p_out);
    /* storing in the CR output signal for CR creation function */
    add_pin_to_signal_list(return_sig_list, p_out);

    npin_t* c = allocate_npin();
    npin_t* c_out = allocate_npin();
    nnet_t* c_net = allocate_nnet();
    /* hook up c into the c net */
    add_driver_pin_to_net(c_net, c);
    /* hook up the c_out to this c_net */
    add_fanout_pin_to_net(c_net, c_out);
    /* storing in the CR output signal for CR creation function */
    add_pin_to_signal_list(return_sig_list, c_out);

    return (return_sig_list);
}
