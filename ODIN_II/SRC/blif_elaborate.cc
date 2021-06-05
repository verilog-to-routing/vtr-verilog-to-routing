/*
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <algorithm>
#include "config_t.h"
#include "odin_types.h"
#include "odin_globals.h"

#include "netlist_utils.h"
#include "node_creation_library.h"
#include "odin_util.h"
#include "simulate_blif.h"

#include "blif_elaborate.hh"
#include "multipliers.h"
#include "hard_blocks.h"
#include "math.h"
#include "memories.h"
#include "adders.h"
#include "subtractions.h"
#include "vtr_memory.h"
#include "vtr_util.h"

void depth_first_traversal_to_blif_elaborate(short marker_value, netlist_t* netlist);
void depth_first_traverse_blif_elaborate(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);

void blif_elaborate_node(nnode_t* node, short traverse_mark_number, netlist_t* netlist);

static void resolve_shift_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static nnode_t* resolve_arithmetic_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
// static void add_dummy_carry_out_to_adder_hard_block(nnode_t* new_node);
// static void remap_input_pins_drivers_based_on_mapping (nnode_t* node);
static void resolve_modulo_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void resolve_divide_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void make_selector_as_first_port(nnode_t* node);
static void resolve_pmux_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void resolve_logical_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static nnode_t* resolve_case_equal_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static nnode_t* resolve_case_not_equal_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void resolve_dff_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void resolve_sdff_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void resolve_dffe_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void resolve_adffe_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void resolve_sdffe_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void resolve_dffsr_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);

/*******************************************************************************************************
 ********************************************** [UTILS] ************************************************
 *******************************************************************************************************/
static void look_for_clocks(netlist_t* netlist);
static signal_list_t* constant_shift (signal_list_t* input_signals, const int shift_size, const operation_list shift_type, const int assignment_size, netlist_t* netlist);
static signal_list_t** modify_div_signal_sizes (nnode_t* node, netlist_t* netlist);
static signal_list_t** implement_division(nnode_t* node, signal_list_t** input_signals, netlist_t* netlist);
static void connect_div_output_pins (nnode_t* node, signal_list_t** output_signals, uintptr_t traverse_mark_number, netlist_t* netlist);
static void make_division_by_constant_node (netlist_t* netlist);
static void make_CR_node (nnode_t* node, signal_list_t* input_signal_list, signal_list_t* output_signal_list);
static signal_list_t* CR_output_signal_init ();

/**
 *-------------------------------------------------------------------------
 * (function: blif_elaborate_top)
 * 
 * @brief elaborating the netlist created from input blif file
 * to make it compatible with Odin's partial mapping
 * 
 * @param netlist pointer to the current netlist file
 *-----------------------------------------------------------------------*/
void blif_elaborate_top(netlist_t* netlist) {
    /* 
     * depending on the input blif target how to do elaboration 
     * Worth noting blif elaboration does not perform for odin's 
     * blif since it is already generated from odin's partial 
     * mapping and is completely elaborated
     */
    if (!configuration.coarsen) {
        /**
         *  nothing needs to be done since the netlist 
         *  is already compatible with Odin_II style 
         */
    }
    else if (configuration.coarsen) {
        /* do the elaboration without any larger structures identified */
        depth_first_traversal_to_blif_elaborate(SUBCKT_BLIF_ELABORATE_TRAVERSE_VALUE, netlist);
        /**
         * After blif elaboration, the netlist is flatten. 
         * change it to not do flattening for simulation blif reading 
         */
        configuration.coarsen = false;
    }
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: depth_first_traversal_to_blif_elaborate()
 * 
 * @brief traverse the netlist to do elaboration
 * 
 * @param marker_value unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 *-------------------------------------------------------------------------------------------*/
void depth_first_traversal_to_blif_elaborate(short marker_value, netlist_t* netlist) {
    int i;

    /* start with the primary input list */
    for (i = 0; i < netlist->num_top_input_nodes; i++) {
        if (netlist->top_input_nodes[i] != NULL) {
            depth_first_traverse_blif_elaborate(netlist->top_input_nodes[i], marker_value, netlist);
        }
    }
    /* now traverse the ground and vcc pins  */
    depth_first_traverse_blif_elaborate(netlist->gnd_node, marker_value, netlist);
    depth_first_traverse_blif_elaborate(netlist->vcc_node, marker_value, netlist);
    depth_first_traverse_blif_elaborate(netlist->pad_node, marker_value, netlist);

    /* look for clock nodes */
    look_for_clocks(netlist);
}

/**
 *---------------------------------------------------------------------------------------------
 * (function: depth_first_traverse)
 * 
 * @brief traverse the netlist to do elaboration
 * 
 * @param node pointing to the netlist internal nodes
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 *-------------------------------------------------------------------------------------------*/
void depth_first_traverse_blif_elaborate(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    int i, j;

    if (node->traverse_visited != traverse_mark_number) {
        /*this is a new node so depth visit it */

        /* mark that we have visitied this node now */
        node->traverse_visited = traverse_mark_number;

        for (i = 0; i < node->num_output_pins; i++) {
            if (node->output_pins[i]->net) {
                nnet_t* next_net = node->output_pins[i]->net;
                if (next_net->fanout_pins) {
                    for (j = 0; j < next_net->num_fanout_pins; j++) {
                        if (next_net->fanout_pins[j]) {
                            if (next_net->fanout_pins[j]->node) {
                                /* recursive call point */
                                depth_first_traverse_blif_elaborate(next_net->fanout_pins[j]->node, traverse_mark_number, netlist);
                            }
                        }
                    }
                }
            }
        }

        /* POST traverse map the node since you might delete */
        blif_elaborate_node(node, traverse_mark_number, netlist);
    }
}

/**
 *----------------------------------------------------------------------
 * (function: blif_elaborate_node)
 * 
 * @brief elaborating each netlist node based on their type
 * 
 * @param node pointing to the netlist internal nodes
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 *--------------------------------------------------------------------*/
void blif_elaborate_node(nnode_t* node, short traverse_number, netlist_t* netlist) {
    switch (node->type) {
        case SL:
        case SR:
        case ASL:
        case ASR: {
            /**
             * resolving the shift nodes by making
             * the input port sizes the same
            */
            resolve_shift_node(node, traverse_number, netlist);
            break;
        }
        case LOGICAL_OR:
        case LOGICAL_AND:
        case LOGICAL_NOT:
        case LOGICAL_NOR:
        case LOGICAL_NAND:
        case LOGICAL_XOR:
        case LOGICAL_XNOR:
        case LOGICAL_EQUAL: 
        case NOT_EQUAL: {
            /** 
             * to make sure they have only one output pin for partial mapping phase 
             */
            resolve_logical_node(node, traverse_number, netlist);
            break;
        }
        case CASE_EQUAL: {
            /**
             * resolving case equal node using xor and and gates
            */
            resolve_case_equal_node(node, traverse_number, netlist);
            break;
        }
        case CASE_NOT_EQUAL: {
            /**
             * resolving case not equal node by putting not gate 
             * after case equal output node
            */
            resolve_case_not_equal_node(node, traverse_number, netlist);
            break;
        }
        case ADD: {
            /** 
             * check for missing ports such as carry-in/out in case of 
             * dealing with generated netlist from Yosys blif file.
             */
            if (hard_adders)
                node = resolve_arithmetic_node(node, traverse_number, netlist);

            add_list = insert_in_vptr_list(add_list, node);
            break;
        }
        case MINUS: {
            /** 
             * Adding to sub_list for future checking on hard blocks
             */
            if (hard_adders)
                node = resolve_arithmetic_node(node, traverse_number, netlist);

            sub_list = insert_in_vptr_list(sub_list, node);
            break;
        }
        case MULTIPLY: {
            /** 
             * Adding to mult_list for future checking on hard blocks
             */
            if (hard_multipliers)
                node = resolve_arithmetic_node(node, traverse_number, netlist);

            mult_list = insert_in_vptr_list(mult_list, node);
            break;
        }
        case MODULO: {
            /**
             * resolving the modulo node by
             */
            resolve_modulo_node(node, traverse_number, netlist);
            break;
        }
        case DIVIDE: {
            /**
             * resolving the divide node by
             */
            resolve_divide_node(node, traverse_number, netlist);
            break;
        }
        case FF_NODE: {
            /**
             * resolving the dff node
             */
            resolve_dff_node(node, traverse_number, netlist);
            break;
        }
        case SDFF: {
            /**
             * resolving dff node with reset value
            */
            resolve_sdff_node(node, traverse_number, netlist);
            break;
        }
        case DFFE: {
            /**
             * resolving dff node with enable
            */
            resolve_dffe_node(node, traverse_number, netlist);
            break;
        }
        case ADFFE: {
            /**
             * resolving dff node with enable and srst
            */
            resolve_adffe_node(node, traverse_number, netlist);
            break;
        }
        case SDFFE: {
            /**
             * resolving dff node with enable and srst
            */
            resolve_sdffe_node(node, traverse_number, netlist);
            break;
        }
        case DFFSR: {
            /**
             * resolving dff node with set and reset
            */
            resolve_dffsr_node(node, traverse_number, netlist);
            break;
        }
        case PMUX: {
            /**
             * need to reorder the input pins, so that the 
             * selector signal comes at the first place
             */
            make_selector_as_first_port(node);
            /**
             * resolving pmux node which is using one-hot selector
            */
            resolve_pmux_node(node, traverse_number, netlist);
            break;
        }
        case MULTIPORT_nBIT_MUX: {
            /**
             * need to reorder the input pins, so that the 
             * selector signal comes at the first place
             */
            make_selector_as_first_port(node);            
            break;
        }
        case GND_NODE:
        case VCC_NODE:
        case PAD_NODE:
        case INPUT_NODE:
        case OUTPUT_NODE: 
        case GTE:
        case LTE:
        case GT:
        case LT:
        case BUF_NODE:
        case BITWISE_NOT:
        case BITWISE_AND:
        case BITWISE_OR:
        case BITWISE_NAND:
        case BITWISE_NOR:
        case BITWISE_XNOR:
        case BITWISE_XOR: {
            /* some are already resolved for this phase */
            break;
        }
        case MUX_2:
        case MULTI_PORT_MUX: 
        case MEMORY:
        case HARD_IP:
        case ADDER_FUNC:
        case CARRY_FUNC:
        case CLOCK_NODE:
        case GENERIC:
        default:
            error_message(BLIF_ELBORATION, node->loc, "node (%s: %s) should have been converted to softer version.", node->type, node->name);
            break;
    }
}

/**
 * (function: resolve_case_not_equal_node)
 * 
 * @brief resolving the shift nodes by making
 * the input port sizes the same
 * 
 * @param node pointing to a logical not node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
static void resolve_shift_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);

    /**
     * (SHIFT ports)
     * INPUTS
     *  A: (width_a)
     *  B: (width_b)
     * OUTPUT
     *  Y: width_y
    */
    int port_a_size = node->input_port_sizes[0];
    int port_b_size = node->input_port_sizes[1];
    int port_y_size = node->output_port_sizes[0];

    /* no change is needed */
    if (port_a_size == port_b_size)
        return;

    /* new port size */
    int max_size = std::max(port_a_size, port_b_size);

    /* creating the new node */
    nnode_t* new_shift_node = make_2port_gate(node->type, max_size, max_size, port_y_size, node, traverse_mark_number);

    int i;
    npin_t* extension_pin = NULL;
    
    for (i = 0; i < max_size; i++) {
        /* port a needs to be extended */
        if (port_a_size < max_size) {
            /* remapping the a pins + adding extension pin */
            if (i < port_a_size) {
                /* need to remap existing pin */
                remap_pin_to_new_node(node->input_pins[i],
                                        new_shift_node,
                                        i);
            } else {
                /* need to add extension pin */
                extension_pin = (node->attributes->port_a_signed == SIGNED) ? copy_input_npin(new_shift_node->input_pins[port_a_size - 1]) : get_zero_pin(netlist);
                add_input_pin_to_node(new_shift_node,
                                        extension_pin,
                                        i);
            }

            /* remapping the b pins untouched */
            remap_pin_to_new_node(node->input_pins[i + max_size],
                                    new_shift_node,
                                    i + max_size);
            

        } 
        /* port b needs to be extended */
        else if (port_b_size < max_size) {
            /* remapping the a pins untouched */
            remap_pin_to_new_node(node->input_pins[i],
                                    new_shift_node,
                                    i);

            /* remapping the b pins + adding extension pin */
            if (i < port_b_size) {
                /* need to remap existing pin */
                remap_pin_to_new_node(node->input_pins[i + max_size],
                                        new_shift_node,
                                        i + max_size);
            } else {
                /* need to add extension pin */
                extension_pin = (node->attributes->port_b_signed == SIGNED) ? copy_input_npin(new_shift_node->input_pins[port_b_size - 1 + max_size]) : get_zero_pin(netlist);
                add_input_pin_to_node(new_shift_node,
                                        extension_pin,
                                        i + max_size);
            }
        }
    }

    /* Connecting output pins */
    for (i = 0; i < port_y_size; i++) {
        remap_pin_to_new_node(node->output_pins[i],
                            new_shift_node,
                            i);
    }

    // CLEAN UP
    free_nnode(node);
}

/**
 *-------------------------------------------------------------------------------------------
 * (function: resolve_arithmetic_node )
 * 
 * @brief check for missing ports such as carry-in/out in case of 
 * dealing with generated netlist from other blif files such Yosys.
 * 
 * @param node pointing to the netlist node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 *-----------------------------------------------------------------------------------------*/
static nnode_t* resolve_arithmetic_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);

    nnode_t* new_node = NULL;

    switch (node->type) {
        case ADD: //fallthorugh
        case MINUS: {
            int num_input_port = node->num_input_port_sizes;
            /* check for operations that has 2 operands */
            if (num_input_port == 2) {
                int i;
                int in_port1_size = node->input_port_sizes[0];
                int in_port2_size = node->input_port_sizes[1];
                int out_port_size = (in_port1_size >= in_port2_size) ? in_port1_size + 1 : in_port2_size + 1;

                new_node = make_2port_gate(node->type, in_port1_size, in_port2_size,
                                            out_port_size,
                                            node, traverse_mark_number);

                for (i = 0; i < in_port1_size; i++) {
                    remap_pin_to_new_node(node->input_pins[i],
                                            new_node,
                                            i);
                }

                for (i = 0; i < in_port2_size; i++) {
                    remap_pin_to_new_node(node->input_pins[i + in_port1_size],
                                            new_node,
                                            i + in_port1_size);
                }

                // moving the output pins to the new node
                for (i = 0; i < out_port_size; i++) {
                    if (i < node->num_output_pins) {
                        remap_pin_to_new_node(node->output_pins[i],
                                                new_node,
                                                i);
                    } else {
                        npin_t* new_pin1 = allocate_npin();
                        npin_t* new_pin2 = allocate_npin();
                        nnet_t* new_net = allocate_nnet();
                        new_net->name = make_full_ref_name(NULL, NULL, NULL, new_node->name, i);
                        /* hook the output pin into the node */
                        add_output_pin_to_node(new_node, new_pin1, i);
                        /* hook up new pin 1 into the new net */
                        add_driver_pin_to_net(new_net, new_pin1);
                        /* hook up the new pin 2 to this new net */
                        add_fanout_pin_to_net(new_net, new_pin2);
                    }
                }

                /**
                 * if number of output pins is greater than the max of input pins, 
                 * here we connect the exceeded pins to the GND 
                */
                for (i = out_port_size; i < node->num_output_pins; i++) {
                    /* creating a buf node */
                    nnode_t* buf_node = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
                    /* adding the GND input pin to the buf node */
                    add_input_pin_to_node(buf_node,
                                            get_zero_pin(netlist),
                                            0);
                    /* remapping the outpin to buf node */
                    remap_pin_to_new_node(node->output_pins[i],
                                            buf_node,
                                            0);
                }

                // CLEAN UP
                free_nnode(node);
            }
            /* otherwise there is unary minus, like -A. no need for any change */ 
            else if (num_input_port == 1) {
                new_node = node;
            }
            break;
        }
        case MULTIPLY: {
            /* no need to do anything here for multipy */
            new_node = node;
            break;
        }
        default: {
            error_message(BLIF_ELBORATION, node->loc,
                              "The node(%s) type is not among Odin's arithmetic types [ADD, MINUS and MULTIPLY]\n", node->name);
            break;                
        }    
    }

    return (new_node);
}

/**
 * (function: add_dummy_carry_out_to_adder_hard_block)
 * 
 * @brief Adding a dummy carry out output pin to the adder hard block 
 * for the possible future processing of soft adder instatiation
 * 
 * @param node pointing to the netlist node 
 */
/*
 * static void add_dummy_carry_out_to_adder_hard_block(nnode_t* node) {
 * char* dummy_cout_name = (char*)vtr::malloc(11 * sizeof(char));
 * strcpy(dummy_cout_name, "dummy_cout");
 *
 * npin_t* new_pin = allocate_npin();
 * new_pin->name = vtr::strdup(dummy_cout_name);
 * new_pin->type = OUTPUT;
 *
 * char* mapping = (char*)vtr::malloc(6 * sizeof(char));
 * strcpy(mapping, "cout");
 * new_pin->mapping = vtr::strdup(mapping);
 *
 * //add_output_port_information(new_node, 1);
 * node->output_port_sizes[node->num_output_port_sizes - 1]++;
 * allocate_more_output_pins(node, 1);
 *
 * add_output_pin_to_node(node, new_pin, node->num_output_pins - 1);
 *
 * nnet_t* new_net = allocate_nnet();
 * new_net->name = vtr::strdup(dummy_cout_name);
 *
 * add_driver_pin_to_net(new_net, new_pin);
 *
 * vtr::free(dummy_cout_name);
 * vtr::free(mapping);
 * }
 */

/**
 * (function: resolve_modulo_node)
 * 
 * @brief resolving module node by 
 * 
 * @param node pointing to a logical not node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
static void resolve_modulo_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);

    /*  */









    resolve_divide_node(node, traverse_mark_number, netlist);
}

/**
 * (function: resolve_divide_node)
 * 
 * @brief resolving divide node
 * 
 * @param node pointing to a logical not node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
static void resolve_divide_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    
    /**
     * Div Ports:
     * IN1: Dividend (m bits) 
     * IN2: Divisor  (n bits) 
     * OUT: Quotient (k bits)
    */
    int divisor_width   = node->input_port_sizes[1];

    /** 
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
    /* keep the record of the extesnsion size for divisor for future transformation */
    int new_divisor_width = modified_input_signals[1]->count; // modified_input_signals[1] == new divisor signals
    
    /* validate new sizes */
    oassert(divisor_width == new_divisor_width);

    /* implementation of the divison circuitry using cellular architecture */
    signal_list_t** div_output_lists = implement_division(node, modified_input_signals, netlist);

    /* remap the div output pin to calculated nodes */
    connect_div_output_pins(node, div_output_lists, traverse_mark_number, netlist);

    // CLEAN UP
    free_nnode(node);
}

/**
 * (function: make_selector_as_first_port)
 * 
 * @brief reorder the input signals of the mux in a way 
 * that the selector would come as the first signal
 * 
 * @param node pointing to a mux node 
 */
static void make_selector_as_first_port(nnode_t* node) {
    long num_input_pins = node->num_input_pins;
    npin_t** input_pins = (npin_t**)vtr::malloc(num_input_pins*sizeof(npin_t*)); // the input pins
    
    int num_input_port_sizes = node->num_input_port_sizes;
    int* input_port_sizes = (int*)vtr::malloc(num_input_port_sizes*sizeof(int)); // info about the input ports

    int i, j;
        int acc_port_sizes = 0;
    i = num_input_port_sizes-1;
    while (i > 0) {
        acc_port_sizes += node->input_port_sizes[i-1];
        i--;
    }
    
    for (j = 0; j < node->input_port_sizes[num_input_port_sizes-1]; j++) {
        input_pins[j] = node->input_pins[j + acc_port_sizes];
        input_pins[j]->pin_node_idx = j;
    }

    acc_port_sizes = 0;
    int offset = input_port_sizes[0] = node->input_port_sizes[num_input_port_sizes-1];
    for (i = 1; i < num_input_port_sizes; i++) {
        input_port_sizes[i] = node->input_port_sizes[i-1];

        for (j = 0; j < input_port_sizes[i]; j++) {
            input_pins[offset] = node->input_pins[j + acc_port_sizes];
            input_pins[offset]->pin_node_idx = offset;
            offset++;
        }
        acc_port_sizes += input_port_sizes[i];
    }

    vtr::free(node->input_pins);
    vtr::free(node->input_port_sizes);
    
    node->input_pins = input_pins;
    node->input_port_sizes = input_port_sizes;
}

/**
 * (function: resolve_logical_node)
 * 
 * @brief resolving the logical nodes by 
 * connecting the ouput pins[1..n] to GND
 * 
 * @param node pointing to a logical not node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
static void resolve_logical_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_output_port_sizes == 1);
    
    int i, j;
    for (i = 1; i < node->num_output_pins; i++) {
        npin_t* output_pin = node->output_pins[i];
        nnet_t* output_net = output_pin->net;
        
        // mke GND the driver of all other output pins
        for (j = 0; j < output_net->num_fanout_pins; j++) {
            npin_t* fanout_pin = output_net->fanout_pins[j];
            add_fanout_pin_to_net(netlist->zero_net, fanout_pin);
        }
        
        free_npin(output_pin);
        free_nnet(output_net);
    }

    node->num_output_pins = 1;
    node->output_port_sizes[0] = 1;
}

/**
 * (function: resolve_case_equal_node)
 * 
 * @brief resolving the CASE EQUAL node using XNOR 
 * for each pin and finally AND all XNOR outputs
 * 
 * @param node pointing to a logical not node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
static nnode_t* resolve_case_equal_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);

    nnode_t* output_node = NULL;
    /**
         * <CASE EQUAL internal nodes>
         * 
         *               
         *   A[0] ---  \\‾‾``                          
         *              ||   ``                         
         *              ||   ''O -----------------------------------------   VCC               
         *              ||   ,,                                          |   |       
         *   B[0] ---  //__,,                                           _|___|_                   
         *         (xnor_nodes[0])                                     | 0   1 | (and_nodes[0])     
         *                                                              \ ⏝⏝ /    
         *   A[1] ---  \\‾‾``                                              | 
         *              ||   ``                                            | 
         *              ||   ''O -----------------------------   __________|         
         *              ||   ,,                              |   |                              
         *   B[1] ---  //__,,                               _|___|_                           
         *         (xnor_nodes[1])                         | 0   1 | (and_nodes[1])                       
         *                                                  \ ⏝⏝ /                                       
         *   A[2] ---  \\‾‾``                                  |                     
         *              ||   ``                                |                     
         *              ||   ''O -----------------   __________|                     
         *              ||   ,,                  |   |                                
         *   B[2] ---  //__,,                   _|___|_                                       
         *         (xnor_nodes[2])             | 0   1 | (and_nodes[2])          
         *                                      \ ⏝⏝ /   
         *       ...                               |   (output_node)       
         *                                         |         
         *       ...                          ...                 
         *                                 |                 
         *                                 |                 
         *                                                  
         *                               OUTPUT              
        */

    /**
     * (CASE_EQUAL ports)
     * INPUTS
     *  A: (width)
     *  B: (width)
     * OUTPUT
     *  Y: 1 bit (0=not equal & 1=equal)
    */
    int i;
    int width = node->input_port_sizes[0];
    nnode_t** xnor_nodes = (nnode_t**)vtr::calloc(width, sizeof(nnode_t*));
    nnode_t** and_nodes = (nnode_t**)vtr::calloc(width, sizeof(nnode_t*));
    signal_list_t* xnor_outputs = init_signal_list();
    signal_list_t* and_outputs = init_signal_list();

    for (i = 0; i < width; i++) {
        /*****************************************************************************************/
        /************************************** XNOR_NODES ***************************************/
        /*****************************************************************************************/
        /* creating the XNOR node */
        xnor_nodes[i] = make_2port_gate(LOGICAL_XNOR, 1, 1, 1, node, traverse_mark_number);

        /* Connecting inputs of XNOR node */
        remap_pin_to_new_node(node->input_pins[i],
                              xnor_nodes[i],
                              0);
        remap_pin_to_new_node(node->input_pins[i + width],
                              xnor_nodes[i],
                              1);

        /* Connecting output of XNOR node */
        npin_t* new_pin1 = allocate_npin();
        npin_t* new_pin2 = allocate_npin();
        nnet_t* new_net = allocate_nnet();
        new_net->name = make_full_ref_name(NULL, NULL, NULL, xnor_nodes[i]->name, 0);
        /* hook the output pin into the node */
        add_output_pin_to_node(xnor_nodes[i], new_pin1, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net, new_pin1);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net, new_pin2);

        // Storing the output pins of the current mux stage as the input of the next one
        add_pin_to_signal_list(xnor_outputs, new_pin2);

        /*****************************************************************************************/
        /*************************************** AND_NODES ***************************************/
        /*****************************************************************************************/
        /* creating the AND node */
        and_nodes[i] = make_2port_gate(LOGICAL_AND, 1, 1, 1, node, traverse_mark_number);

        /* Connecting inputs of AND node */
        add_input_pin_to_node(and_nodes[i],
                              xnor_outputs->pins[i],
                              0);

        if (i == 0) {
            add_input_pin_to_node(and_nodes[i],
                                  get_one_pin(netlist),
                                  1);
        } else {
            add_input_pin_to_node(and_nodes[i],
                                  and_outputs->pins[i - 1],
                                  1);
        }

        /* Connecting output of AND node */
        if (i != width - 1) {
            new_pin1 = allocate_npin();
            new_pin2 = allocate_npin();
            new_net = allocate_nnet();
            new_net->name = make_full_ref_name(NULL, NULL, NULL, and_nodes[i]->name, 0);
            /* hook the output pin into the node */
            add_output_pin_to_node(and_nodes[i], new_pin1, 0);
            /* hook up new pin 1 into the new net */
            add_driver_pin_to_net(new_net, new_pin1);
            /* hook up the new pin 2 to this new net */
            add_fanout_pin_to_net(new_net, new_pin2);

            // Storing the output pins of the current mux stage as the input of the next one
            add_pin_to_signal_list(and_outputs, new_pin2);
        } else {
            remap_pin_to_new_node(node->output_pins[0],
                                  and_nodes[i],
                                  0);

            output_node = and_nodes[i];
        }
    }

    // CLEAN UP
    vtr::free(xnor_nodes);
    vtr::free(and_nodes);

    free_signal_list(xnor_outputs);
    free_signal_list(and_outputs);
    
    free_nnode(node);

    return (output_node);
}

/**
 * (function: resolve_case_not_equal_node)
 * 
 * @brief resolving the CASE NOT EQUAL node using 
 * NOT gate after case equal output node
 * 
 * @param node pointing to a logical not node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
static nnode_t* resolve_case_not_equal_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);

    /**
         * <CASE NOT EQUAL internal nodes>
         * 
         *               
         *   A[0] ---  \\‾‾``                          
         *              ||   ``                         
         *              ||   ''O -----------------------------------------   VCC               
         *              ||   ,,                                          |   |       
         *   B[0] ---  //__,,                                           _|___|_                   
         *         (xnor_nodes[0])                                     | 0   1 | (and_nodes[0])     
         *                                                              \ ⏝⏝ /    
         *   A[1] ---  \\‾‾``                                              | 
         *              ||   ``                                            | 
         *              ||   ''O -----------------------------   __________|         
         *              ||   ,,                              |   |                              
         *   B[1] ---  //__,,                               _|___|_                           
         *         (xnor_nodes[1])                         | 0   1 | (and_nodes[1])                       
         *                                                  \ ⏝⏝ /                                       
         *   A[2] ---  \\‾‾``                                  |                     
         *              ||   ``                                |                     
         *              ||   ''O -----------------   __________|                     
         *              ||   ,,                  |   |                                
         *   B[2] ---  //__,,                   _|___|_                                       
         *         (xnor_nodes[2])             | 0   1 | (and_nodes[2])          
         *                                      \ ⏝⏝ /   
         *       ...                               |         
         *                                         |         
         *       ...                          ...                 
         *                                 |                 
         *       ...                       |                 
         *                               _____ 
         *                               \   /
         *                                \ /   (output_node) 
         *                                 |
         *                                 |                
         *               
         *                               OUTPUT              
        */

    /**
     * (CASE_NOT EQUAL ports)
     * INPUTS
     *  A: (width)
     *  B: (width)
     * OUTPUT
     *  Y: 1 bit (0=not equal & 1=equal)
    */
    nnode_t* case_equal = resolve_case_equal_node(node, traverse_mark_number, netlist);

    nnode_t* not_node = make_not_gate(case_equal, traverse_mark_number);
    connect_nodes(case_equal, 0, not_node, 0);

    // specify not gate output pin
    npin_t* new_pin1_not = allocate_npin();
    npin_t* new_pin2_not = allocate_npin();
    nnet_t* new_net_not = allocate_nnet();
    new_net_not->name = make_full_ref_name(NULL, NULL, NULL, not_node->name, 0);
    /* hook the output pin into the node */
    add_output_pin_to_node(not_node, new_pin1_not, 0);
    /* hook up new pin 1 into the new net */
    add_driver_pin_to_net(new_net_not, new_pin1_not);
    /* hook up the new pin 2 to this new net */
    add_fanout_pin_to_net(new_net_not, new_pin2_not);

    return (not_node);
}

/**
 * (function: transform_to_single_bit_dff_nodes)
 * 
 * @brief split the dff node read from yosys blif to
 * FF nodes with input/output width one
 * 
 * @param node pointing to the dff node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
static void resolve_dff_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_output_pins + 1 == node->num_input_pins);

    /**
     * need to reformat the order of pins and the number of ports
     * since odin ff_node has only one port which the last pin is clk
    */
    int i;   
    int width = node->num_output_pins;
    nnode_t* ff_node = make_1port_gate(FF_NODE, width+1, width, node, traverse_mark_number);
    ff_node->attributes->clk_edge_type = node->attributes->clk_edge_type;
    /* remap the last bit of the clk pin to the last input pin since the resit does not work at all */
    int clk_width = node->input_port_sizes[0];
    remap_pin_to_new_node(node->input_pins[0], ff_node, width);

    /* remapping the D inputs to [1..n] */
    for (i = 0; i < width; i++) {
        remap_pin_to_new_node(node->input_pins[clk_width + i], ff_node, i);
    }
    
    /* hook the output pins into the new ff_node */
    for (i = 0; i < width; i++) {
        remap_pin_to_new_node(node->output_pins[i], ff_node, i);
    }  

    /** if it is multibit we will leave it for partial mapping phase,
     * otherwise we need to add single bit ff node to the ff_node 
     * list of the current netlist 
    */
   /* single bit dff, [0]: clk, [1]: D */
   if (node->num_input_pins == 2) {
        /* adding the new generated node to the ff node list of the enetlist */
        netlist->ff_nodes = (nnode_t**)vtr::realloc(netlist->ff_nodes, sizeof(nnode_t*) * (netlist->num_ff_nodes + 1));
        netlist->ff_nodes[netlist->num_ff_nodes] = ff_node;
        netlist->num_ff_nodes++;
   }

    // CLEAN UP
    free_nnode(node); 
}

/**
 * (function: resolve_sdff_node)
 * 
 * @brief resolving the sdff node by connecting 
 * the multiplexing the D input with reset value
 * 
 * @param node pointing to a dffe node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
static void resolve_sdff_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes == 3);
    oassert(node->num_output_port_sizes == 1);

    /* making sure the srst input is 1 bit */
    oassert(node->input_port_sizes[2] == 1);

    /**
     * CLK:  input port 0
     * D:    input port 1
     * SRST: input port 2
     * Q:    output port 3
    */
    int i;
    int CLK_width = node->input_port_sizes[0]; // == 1
    int D_width = node->input_port_sizes[1];

    signal_list_t* reset_value = create_constant_value(node->attributes->sreset_value, D_width, netlist);

    /*****************************************************************************************/
    /**************************************** RST_CHECK **************************************/
    /*****************************************************************************************/
    /* creating equal node to compare the value of enable */
    nnode_t* select_reset = make_2port_gate(LOGICAL_EQUAL, 1, 1, 1, node, traverse_mark_number);
    /* connecting SRST pin */
    remap_pin_to_new_node(node->input_pins[CLK_width + D_width],
                          select_reset,
                          0);

    /* to check the polarity and connect the correspondence signal */
    if (node->attributes->sreset_polarity == ACTIVE_HIGH_SENSITIVITY) {
        add_input_pin_to_node(select_reset,
                              get_one_pin(netlist),
                              1);
    } else if (node->attributes->sreset_polarity == ACTIVE_LOW_SENSITIVITY) {
        add_input_pin_to_node(select_reset,
                              get_zero_pin(netlist),
                              1);
    }

    /* Specifying the level_muxes outputs */
    // Connect output pin to related input pin
    npin_t* new_pin1 = allocate_npin();
    npin_t* select_reset_output = allocate_npin();
    nnet_t* new_net = allocate_nnet();
    new_net->name = make_full_ref_name(NULL, NULL, NULL, select_reset->name, 0);
    /* hook the output pin into the node */
    add_output_pin_to_node(select_reset, new_pin1, 0);
    /* hook up new pin 1 into the new net */
    add_driver_pin_to_net(new_net, new_pin1);
    /* hook up the new pin 2 to this new net */
    add_fanout_pin_to_net(new_net, select_reset_output);

    // make a not of selector
    nnode_t* not_select_reset = make_not_gate(select_reset, traverse_mark_number);
    connect_nodes(select_reset, 0, not_select_reset, 0);

    /* creating a internal nodes to initialize the value of D register */
    nnode_t** muxes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** ff_nodes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));

    for (i = 0; i < D_width; i++) {
        /*****************************************************************************************/
        /*************************************** VALUE_MUX ***************************************/
        /*****************************************************************************************/
        muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* connecting selectors */
        connect_nodes(select_reset, 0, muxes[i], 1);
        connect_nodes(not_select_reset, 0, muxes[i], 0);

        /* connecting the pad node to the 0: input of the mux pin */
        add_input_pin_to_node(muxes[i],
                              reset_value->pins[i],
                              2);

        /* remapping the D[i] pin to 0: mux input */
        remap_pin_to_new_node(node->input_pins[i + CLK_width],
                              muxes[i],
                              3);

        // specify mux_set[i] output pin
        npin_t* new_pin1_mux = allocate_npin();
        npin_t* mux_output_pin = allocate_npin();
        nnet_t* new_net_mux = allocate_nnet();
        new_net_mux->name = make_full_ref_name(NULL, NULL, NULL, muxes[i]->name, i);
        /* hook the output pin into the node */
        add_output_pin_to_node(muxes[i], new_pin1_mux, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net_mux, new_pin1_mux);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net_mux, mux_output_pin);

        /*****************************************************************************************/
        /**************************************** FF_NODE ****************************************/
        /*****************************************************************************************/

        ff_nodes[i] = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        ff_nodes[i]->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /**
         * connecting the data input of 
         * Q = (SRST) ? reset_value : D
         */
        add_input_pin_to_node(ff_nodes[i],
                              mux_output_pin,
                              0);

        /* connecting the clk pin */
        if (i != D_width - 1) {
            add_input_pin_to_node(ff_nodes[i],
                                  copy_input_npin(node->input_pins[0]),
                                  1);
        } else {
            remap_pin_to_new_node(node->input_pins[0],
                                  ff_nodes[i],
                                  1);
        }

        /* connectiong the dffe output pin to the ff_node output pin */
        remap_pin_to_new_node(node->output_pins[i],
                              ff_nodes[i],
                              0);
    }

    // CLEAN UP
    free_signal_list(reset_value);
    free_nnode(node);

    vtr::free(muxes);
    vtr::free(ff_nodes);
}

/**
 * (function: resolve_dffe_node)
 * 
 * @brief resolving the dffe node by connecting 
 * multiplexed D input with Q as the output 
 * 
 * @param node pointing to a dffe node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
static void resolve_dffe_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes == 3);
    oassert(node->num_output_port_sizes == 1);

    /* making sure the enable input is 1 bit */
    oassert(node->input_port_sizes[2] == 1);

    /**
     * CLK: input port 0
     * D:   input port 1
     * EN:  input port 2
     * Q:   output port 3
    */
    int i;
    int CLK_width = node->input_port_sizes[0]; // == 1
    int D_width = node->input_port_sizes[1];

    /*****************************************************************************************/
    /**************************************** EN_CHECK ***************************************/
    /*****************************************************************************************/
    /* creating equal node to compare the value of enable */
    nnode_t* select_enable = make_2port_gate(LOGICAL_EQUAL, 1, 1, 1, node, traverse_mark_number);
    /* connecting EN pin */
    remap_pin_to_new_node(node->input_pins[CLK_width + D_width],
                          select_enable,
                          0);

    /* to check the polarity and connect the correspondence signal */
    if (node->attributes->enable_polarity == ACTIVE_HIGH_SENSITIVITY) {
        add_input_pin_to_node(select_enable,
                              get_one_pin(netlist),
                              1);
    } else if (node->attributes->enable_polarity == ACTIVE_LOW_SENSITIVITY) {
        add_input_pin_to_node(select_enable,
                              get_zero_pin(netlist),
                              1);
    }

    /* Specifying the level_muxes outputs */
    // Connect output pin to related input pin
    npin_t* new_pin1 = allocate_npin();
    npin_t* select_enable_output = allocate_npin();
    nnet_t* new_net = allocate_nnet();
    new_net->name = make_full_ref_name(NULL, NULL, NULL, select_enable->name, 0);
    /* hook the output pin into the node */
    add_output_pin_to_node(select_enable, new_pin1, 0);
    /* hook up new pin 1 into the new net */
    add_driver_pin_to_net(new_net, new_pin1);
    /* hook up the new pin 2 to this new net */
    add_fanout_pin_to_net(new_net, select_enable_output);

    // make a not of selector
    nnode_t* not_select_enable = make_not_gate(select_enable, traverse_mark_number);
    connect_nodes(select_enable, 0, not_select_enable, 0);

    /* creating a internal nodes to initialize the value of D register */
    nnode_t** muxes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** ff_nodes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));

    for (i = 0; i < D_width; i++) {
        /*****************************************************************************************/
        /*************************************** VALUE_MUX ***************************************/
        /*****************************************************************************************/
        muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* connecting selectors */
        connect_nodes(select_enable, 0, muxes[i], 1);
        connect_nodes(not_select_enable, 0, muxes[i], 0);

        /* connecting the pad node to the 0: input of the mux pin */
        add_input_pin_to_node(muxes[i],
                              get_pad_pin(netlist),
                              2);

        /* remapping the D[i] pin to 0: mux input */
        remap_pin_to_new_node(node->input_pins[i + CLK_width],
                              muxes[i],
                              3);

        // specify mux_set[i] output pin
        npin_t* new_pin1_mux = allocate_npin();
        npin_t* mux_output_pin = allocate_npin();
        nnet_t* new_net_mux = allocate_nnet();
        new_net_mux->name = make_full_ref_name(NULL, NULL, NULL, muxes[i]->name, i);
        /* hook the output pin into the node */
        add_output_pin_to_node(muxes[i], new_pin1_mux, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net_mux, new_pin1_mux);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net_mux, mux_output_pin);

        /*****************************************************************************************/
        /**************************************** FF_NODE ****************************************/
        /*****************************************************************************************/

        ff_nodes[i] = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        ff_nodes[i]->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /**
         * connecting the data input of 
         * (EN == EN_POLARITY) ? D : Q
         */
        add_input_pin_to_node(ff_nodes[i],
                              mux_output_pin,
                              0);

        /* connecting the clk pin */
        if (i != D_width - 1) {
            add_input_pin_to_node(ff_nodes[i],
                                  copy_input_npin(node->input_pins[0]),
                                  1);
        } else {
            remap_pin_to_new_node(node->input_pins[0],
                                  ff_nodes[i],
                                  1);
        }

        /* connectiong the dffe output pin to the ff_node output pin */
        remap_pin_to_new_node(node->output_pins[i],
                              ff_nodes[i],
                              0);
    }

    // CLEAN UP
    free_nnode(node);

    vtr::free(muxes);
    vtr::free(ff_nodes);
}

/**
 * (function: resolve_adffe_node)
 * 
 * @brief resolving the adffe node by connecting 
 * the multiplexing the D input with Q ff_node output 
 * and enabling reset value 
 * 
 * @param node pointing to a dffe node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
static void resolve_adffe_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes == 4);
    oassert(node->num_output_port_sizes == 1);

    /* making sure the arst input is 1 bit */
    oassert(node->input_port_sizes[1] == 1);

    /* making sure the en input is 1 bit */
    oassert(node->input_port_sizes[3] == 1);

    /**
     * ARST: input port 0
     * CLK:  input port 1
     * D:    input port 2
     * EN:   input port 3
     * Q:    output port 0
    */
    int i;
    int ARST_width = node->input_port_sizes[0]; // == 1
    int CLK_width = node->input_port_sizes[1]; // == 1
    int D_width = node->input_port_sizes[2];

    /*****************************************************************************************/
    /**************************************** EN_CHECK ***************************************/
    /*****************************************************************************************/
    /* creating equal node to compare the value of enable */
    nnode_t* select_enable = make_2port_gate(LOGICAL_EQUAL, 1, 1, 1, node, traverse_mark_number);
    /* connecting EN pin */
    remap_pin_to_new_node(node->input_pins[ARST_width + CLK_width + D_width],
                          select_enable,
                          0);

    /* to check the polarity and connect the correspondence signal */
    if (node->attributes->enable_polarity == ACTIVE_HIGH_SENSITIVITY) {
        add_input_pin_to_node(select_enable,
                              get_one_pin(netlist),
                              1);
    } else if (node->attributes->enable_polarity == ACTIVE_LOW_SENSITIVITY) {
        add_input_pin_to_node(select_enable,
                              get_zero_pin(netlist),
                              1);
    }

    /* Specifying the level_muxes outputs */
    // Connect output pin to related input pin
    npin_t* new_pin1_en = allocate_npin();
    npin_t* select_enable_output = allocate_npin();
    nnet_t* new_net_en = allocate_nnet();
    new_net_en->name = make_full_ref_name(NULL, NULL, NULL, select_enable->name, 0);
    /* hook the output pin into the node */
    add_output_pin_to_node(select_enable, new_pin1_en, 0);
    /* hook up new pin 1 into the new net */
    add_driver_pin_to_net(new_net_en, new_pin1_en);
    /* hook up the new pin 2 to this new net */
    add_fanout_pin_to_net(new_net_en, select_enable_output);

    // make a not of selector
    nnode_t* not_select_enable = make_not_gate(select_enable, traverse_mark_number);
    connect_nodes(select_enable, 0, not_select_enable, 0);

    /*****************************************************************************************/
    /**************************************** RST_CHECK **************************************/
    /*****************************************************************************************/
    signal_list_t* reset_value = create_constant_value(node->attributes->areset_value, D_width, netlist);

    /* creating equal node to compare the value of enable */
    nnode_t* select_reset = make_2port_gate(LOGICAL_EQUAL, 1, 1, 1, node, traverse_mark_number);
    /* connecting ARST pin */
    remap_pin_to_new_node(node->input_pins[0],
                          select_reset,
                          0);

    /* to check the polarity and connect the correspondence signal */
    if (node->attributes->areset_polarity == ACTIVE_HIGH_SENSITIVITY) {
        add_input_pin_to_node(select_reset,
                              get_one_pin(netlist),
                              1);
    } else if (node->attributes->areset_polarity == ACTIVE_LOW_SENSITIVITY) {
        add_input_pin_to_node(select_reset,
                              get_zero_pin(netlist),
                              1);
    }

    /* Specifying the level_muxes outputs */
    // Connect output pin to related input pin
    npin_t* new_pin1_rst = allocate_npin();
    npin_t* select_reset_output = allocate_npin();
    nnet_t* new_net_rst = allocate_nnet();
    new_net_rst->name = make_full_ref_name(NULL, NULL, NULL, select_reset->name, 0);
    /* hook the output pin into the node */
    add_output_pin_to_node(select_reset, new_pin1_rst, 0);
    /* hook up new pin 1 into the new net */
    add_driver_pin_to_net(new_net_rst, new_pin1_rst);
    /* hook up the new pin 2 to this new net */
    add_fanout_pin_to_net(new_net_rst, select_reset_output);

    // make a not of selector
    nnode_t* not_select_reset = make_not_gate(select_reset, traverse_mark_number);
    connect_nodes(select_reset, 0, not_select_reset, 0);

    /* creating a internal nodes to initialize the value of D register */
    nnode_t** en_muxes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** rst_muxes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** ff_nodes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));

    for (i = 0; i < D_width; i++) {
        /*****************************************************************************************/
        /*************************************** EN_MUX *****************************************/
        /*****************************************************************************************/
        en_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* connecting selectors */
        connect_nodes(select_enable, 0, en_muxes[i], 1);
        connect_nodes(not_select_enable, 0, en_muxes[i], 0);

        /* connecting the pad node to the 0: input of the mux pin */
        add_input_pin_to_node(en_muxes[i],
                              get_pad_pin(netlist),
                              2);

        /* remapping the D[i] pin to 0: mux input */
        remap_pin_to_new_node(node->input_pins[i + ARST_width + CLK_width],
                              en_muxes[i],
                              3);

        // specify mux_set[i] output pin
        npin_t* new_pin1_en_mux = allocate_npin();
        npin_t* en_mux_output_pin = allocate_npin();
        nnet_t* new_net_en_mux = allocate_nnet();
        new_net_en_mux->name = make_full_ref_name(NULL, NULL, NULL, en_muxes[i]->name, i);
        /* hook the output pin into the node */
        add_output_pin_to_node(en_muxes[i], new_pin1_en_mux, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net_en_mux, new_pin1_en_mux);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net_en_mux, en_mux_output_pin);

        /*****************************************************************************************/
        /*************************************** RST_MUX *****************************************/
        /*****************************************************************************************/
        rst_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* connecting selectors */
        connect_nodes(select_reset, 0, rst_muxes[i], 1);
        connect_nodes(not_select_reset, 0, rst_muxes[i], 0);

        /* connecting the pad node to the 0: input of the mux pin */
        add_input_pin_to_node(rst_muxes[i],
                              reset_value->pins[i],
                              2);

        /* remapping the D[i] pin to 0: mux input */
        add_input_pin_to_node(rst_muxes[i],
                              en_mux_output_pin,
                              3);

        // specify mux_set[i] output pin
        npin_t* new_pin1_rst_mux = allocate_npin();
        npin_t* rst_mux_output_pin = allocate_npin();
        nnet_t* new_net_rst_mux = allocate_nnet();
        new_net_rst_mux->name = make_full_ref_name(NULL, NULL, NULL, rst_muxes[i]->name, i);
        /* hook the output pin into the node */
        add_output_pin_to_node(rst_muxes[i], new_pin1_rst_mux, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net_rst_mux, new_pin1_rst_mux);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net_rst_mux, rst_mux_output_pin);

        /*****************************************************************************************/
        /**************************************** FF_NODE ****************************************/
        /*****************************************************************************************/

        ff_nodes[i] = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        ff_nodes[i]->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /**
         * connecting the data input of 
         * Q = (SRST) ? reset_value : D
         */
        add_input_pin_to_node(ff_nodes[i],
                              rst_mux_output_pin,
                              0);

        /* connecting the clk pin */
        if (i != D_width - 1) {
            add_input_pin_to_node(ff_nodes[i],
                                  copy_input_npin(node->input_pins[ARST_width]),
                                  1);
        } else {
            remap_pin_to_new_node(node->input_pins[ARST_width],
                                  ff_nodes[i],
                                  1);
        }

        /* connectiong the dffe output pin to the ff_node output pin */
        remap_pin_to_new_node(node->output_pins[i],
                              ff_nodes[i],
                              0);
    }

    // CLEAN UP
    free_signal_list(reset_value);
    free_nnode(node);

    vtr::free(en_muxes);
    vtr::free(rst_muxes);
    vtr::free(ff_nodes);
}

/**
 * (function: resolve_adffe_node)
 * 
 * @brief resolving the adffe node by connecting 
 * the multiplexing the D input with Q ff_node output 
 * and enabling reset value 
 * 
 * @param node pointing to a dffe node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
static void resolve_sdffe_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes == 4);
    oassert(node->num_output_port_sizes == 1);

    /* making sure the srst input is 1 bit */
    oassert(node->input_port_sizes[3] == 1);

    /* making sure the en input is 1 bit */
    oassert(node->input_port_sizes[2] == 1);

    /**
     * CLK:  input port 0
     * D:    input port 1
     * EN:   input port 2
     * SRST: input port 3
     * Q:    output port 0
    */
    int i;
    int CLK_width = node->input_port_sizes[0]; // == 1
    int D_width = node->input_port_sizes[1];
    int EN_width = node->input_port_sizes[2]; // == 1

    /*****************************************************************************************/
    /**************************************** EN_CHECK ***************************************/
    /*****************************************************************************************/
    /* creating equal node to compare the value of enable */
    nnode_t* select_enable = make_2port_gate(LOGICAL_EQUAL, 1, 1, 1, node, traverse_mark_number);
    /* connecting EN pin */
    remap_pin_to_new_node(node->input_pins[CLK_width + D_width],
                          select_enable,
                          0);

    /* to check the polarity and connect the correspondence signal */
    if (node->attributes->enable_polarity == ACTIVE_HIGH_SENSITIVITY) {
        add_input_pin_to_node(select_enable,
                              get_one_pin(netlist),
                              1);
    } else if (node->attributes->enable_polarity == ACTIVE_LOW_SENSITIVITY) {
        add_input_pin_to_node(select_enable,
                              get_zero_pin(netlist),
                              1);
    }

    /* Specifying the level_muxes outputs */
    // Connect output pin to related input pin
    npin_t* new_pin1_en = allocate_npin();
    npin_t* select_enable_output = allocate_npin();
    nnet_t* new_net_en = allocate_nnet();
    new_net_en->name = make_full_ref_name(NULL, NULL, NULL, select_enable->name, 0);
    /* hook the output pin into the node */
    add_output_pin_to_node(select_enable, new_pin1_en, 0);
    /* hook up new pin 1 into the new net */
    add_driver_pin_to_net(new_net_en, new_pin1_en);
    /* hook up the new pin 2 to this new net */
    add_fanout_pin_to_net(new_net_en, select_enable_output);

    // make a not of selector
    nnode_t* not_select_enable = make_not_gate(select_enable, traverse_mark_number);
    connect_nodes(select_enable, 0, not_select_enable, 0);

    /*****************************************************************************************/
    /**************************************** RST_CHECK **************************************/
    /*****************************************************************************************/
    signal_list_t* reset_value = create_constant_value(node->attributes->sreset_value, D_width, netlist);

    /* creating equal node to compare the value of enable */
    nnode_t* select_reset = make_2port_gate(LOGICAL_EQUAL, 1, 1, 1, node, traverse_mark_number);
    /* connecting SRST pin */
    remap_pin_to_new_node(node->input_pins[CLK_width + D_width + EN_width],
                          select_reset,
                          0);

    /* to check the polarity and connect the correspondence signal */
    if (node->attributes->sreset_polarity == ACTIVE_HIGH_SENSITIVITY) {
        add_input_pin_to_node(select_reset,
                              get_one_pin(netlist),
                              1);
    } else if (node->attributes->sreset_polarity == ACTIVE_LOW_SENSITIVITY) {
        add_input_pin_to_node(select_reset,
                              get_zero_pin(netlist),
                              1);
    }

    /* Specifying the level_muxes outputs */
    // Connect output pin to related input pin
    npin_t* new_pin1_rst = allocate_npin();
    npin_t* select_reset_output = allocate_npin();
    nnet_t* new_net_rst = allocate_nnet();
    new_net_rst->name = make_full_ref_name(NULL, NULL, NULL, select_reset->name, 0);
    /* hook the output pin into the node */
    add_output_pin_to_node(select_reset, new_pin1_rst, 0);
    /* hook up new pin 1 into the new net */
    add_driver_pin_to_net(new_net_rst, new_pin1_rst);
    /* hook up the new pin 2 to this new net */
    add_fanout_pin_to_net(new_net_rst, select_reset_output);

    // make a not of selector
    nnode_t* not_select_reset = make_not_gate(select_reset, traverse_mark_number);
    connect_nodes(select_reset, 0, not_select_reset, 0);

    /* creating a internal nodes to initialize the value of D register */
    nnode_t** en_muxes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** rst_muxes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));
    nnode_t** ff_nodes = (nnode_t**)vtr::calloc(D_width, sizeof(nnode_t*));

    for (i = 0; i < D_width; i++) {
        /*****************************************************************************************/
        /*************************************** EN_MUX *****************************************/
        /*****************************************************************************************/
        en_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* connecting selectors */
        connect_nodes(select_enable, 0, en_muxes[i], 1);
        connect_nodes(not_select_enable, 0, en_muxes[i], 0);

        /* connecting the pad node to the 0: input of the mux pin */
        add_input_pin_to_node(en_muxes[i],
                              get_pad_pin(netlist),
                              2);

        /* remapping the D[i] pin to 0: mux input */
        remap_pin_to_new_node(node->input_pins[i + CLK_width],
                              en_muxes[i],
                              3);

        // specify mux_set[i] output pin
        npin_t* new_pin1_en_mux = allocate_npin();
        npin_t* en_mux_output_pin = allocate_npin();
        nnet_t* new_net_en_mux = allocate_nnet();
        new_net_en_mux->name = make_full_ref_name(NULL, NULL, NULL, en_muxes[i]->name, i);
        /* hook the output pin into the node */
        add_output_pin_to_node(en_muxes[i], new_pin1_en_mux, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net_en_mux, new_pin1_en_mux);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net_en_mux, en_mux_output_pin);

        /*****************************************************************************************/
        /*************************************** RST_MUX *****************************************/
        /*****************************************************************************************/
        rst_muxes[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);

        /* connecting selectors */
        connect_nodes(select_reset, 0, rst_muxes[i], 1);
        connect_nodes(not_select_reset, 0, rst_muxes[i], 0);

        /* connecting the pad node to the 0: input of the mux pin */
        add_input_pin_to_node(rst_muxes[i],
                              reset_value->pins[i],
                              2);

        /* remapping the D[i] pin to 0: mux input */
        add_input_pin_to_node(rst_muxes[i],
                              en_mux_output_pin,
                              3);

        // specify mux_set[i] output pin
        npin_t* new_pin1_rst_mux = allocate_npin();
        npin_t* rst_mux_output_pin = allocate_npin();
        nnet_t* new_net_rst_mux = allocate_nnet();
        new_net_rst_mux->name = make_full_ref_name(NULL, NULL, NULL, rst_muxes[i]->name, i);
        /* hook the output pin into the node */
        add_output_pin_to_node(rst_muxes[i], new_pin1_rst_mux, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net_rst_mux, new_pin1_rst_mux);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net_rst_mux, rst_mux_output_pin);

        /*****************************************************************************************/
        /**************************************** FF_NODE ****************************************/
        /*****************************************************************************************/

        ff_nodes[i] = make_2port_gate(FF_NODE, 1, 1, 1, node, traverse_mark_number);
        ff_nodes[i]->attributes->clk_edge_type = node->attributes->clk_edge_type;

        /**
         * connecting the data input of 
         * Q = (SRST) ? reset_value : D
         */
        add_input_pin_to_node(ff_nodes[i],
                              rst_mux_output_pin,
                              0);

        /* connecting the clk pin */
        if (i != D_width - 1) {
            add_input_pin_to_node(ff_nodes[i],
                                  copy_input_npin(node->input_pins[0]),
                                  1);
        } else {
            remap_pin_to_new_node(node->input_pins[0],
                                  ff_nodes[i],
                                  1);
        }

        /* connectiong the dffe output pin to the ff_node output pin */
        remap_pin_to_new_node(node->output_pins[i],
                              ff_nodes[i],
                              0);
    }

    // CLEAN UP
    free_signal_list(reset_value);
    free_nnode(node);

    vtr::free(en_muxes);
    vtr::free(rst_muxes);
    vtr::free(ff_nodes);
}

/**
 * (function: resolve_dffsr_node)
 * 
 * @brief resolving the dffsr node by connecting 
 * the ouput pins[1..n] to GND/VCC/D based on 
 * the clr/set edge 
 * 
 * @param node pointing to a dffsr node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
static void resolve_dffsr_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes == 4);
    oassert(node->num_output_port_sizes == 1);

    /**
     * CLK: input port 0
     * CLR: input port 1
     * D:   input port 2
     * SET: input port 3
    */
    int width     = node->num_output_pins;
    int CLK_width = node->input_port_sizes[0];
    int CLR_width = node->input_port_sizes[1];
    int D_width   = node->input_port_sizes[2];

    nnode_t** mux_set = (nnode_t**)vtr::malloc(width * sizeof(nnode_t*));
    nnode_t** mux_clr = (nnode_t**)vtr::malloc(width * sizeof(nnode_t*));

    int i;
    for (i = 0; i < width; i++) {
        /*****************************************************************************************/
        /**************************************** MUX_SET ****************************************/
        /*****************************************************************************************/
        mux_set[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);
        
        // connect related pin of second_input to related multiplexer as a selector
        nnode_t* select_set = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
        // SET[i] === mux_set selector[i]
        remap_pin_to_new_node(node->input_pins[CLK_width + CLR_width + D_width + i], select_set, 0);
        
        // make a not of selector
        nnode_t* not_select_set = make_not_gate(select_set, traverse_mark_number);
        connect_nodes(select_set, 0, not_select_set, 0);

        // connect mux_set selector based on the SET polarity
        if (node->attributes->set_edge_type == RISING_EDGE_SENSITIVITY) {
            connect_nodes(select_set, 0, mux_set[i], 1);
            connect_nodes(not_select_set, 0, mux_set[i], 0);
        } else if (node->attributes->set_edge_type == FALLING_EDGE_SENSITIVITY) {
            connect_nodes(select_set, 0, mux_set[i], 0);
            connect_nodes(not_select_set, 0, mux_set[i], 1);
        }

        // remap D[i] to the mux_set[i]
        remap_pin_to_new_node(node->input_pins[CLK_width + CLR_width + i],
                                mux_set[i],
                                2);
        // connect VCC to the mux_set[i]
        add_input_pin_to_node(mux_set[i],
                                get_one_pin(netlist),
                                3);


        // specify mux_set[i] output pin
        npin_t* new_pin1_set = allocate_npin();
        npin_t* new_pin2_set = allocate_npin();
        nnet_t* new_net_set = allocate_nnet();
        new_net_set->name = make_full_ref_name(NULL, NULL, NULL, mux_set[i]->name, 0);
        /* hook the output pin into the node */
        add_output_pin_to_node(mux_set[i], new_pin1_set, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net_set, new_pin1_set);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net_set, new_pin2_set);

        
        /*****************************************************************************************/
        /**************************************** MUX_CLR ****************************************/
        /*****************************************************************************************/
        mux_clr[i] = make_2port_gate(MUX_2, 2, 2, 1, node, traverse_mark_number);
        
        // connect related pin of second_input to related multiplexer as a selector
        nnode_t* select_clr = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);
        // CLR[i] === mux_clr selector[i]
        remap_pin_to_new_node(node->input_pins[CLK_width + i], select_clr, 0);
        
        // make a not of selector
        nnode_t* not_select_clr = make_not_gate(select_clr, traverse_mark_number);
        connect_nodes(select_clr, 0, not_select_clr, 0);

        // connect mux_clr selector based on the CLR polarity
        if (node->attributes->clr_edge_type == RISING_EDGE_SENSITIVITY) {
            connect_nodes(select_clr, 0, mux_clr[i], 1);
            connect_nodes(not_select_clr, 0, mux_clr[i], 0);
        } else if (node->attributes->clr_edge_type == FALLING_EDGE_SENSITIVITY) {
            connect_nodes(select_clr, 0, mux_clr[i], 0);
            connect_nodes(not_select_clr, 0, mux_clr[i], 1);
        }

        // connect mux_set[i] output pin to the mux_clr[i]
        add_input_pin_to_node(mux_clr[i],
                                new_pin2_set,
                                2);
        // connect GND to the mux_clr[i]
        add_input_pin_to_node(mux_clr[i],
                                get_zero_pin(netlist),
                                3);
        // specify mux_clr[i] output pin
        npin_t* new_pin1_clr = allocate_npin();
        npin_t* new_pin2_clr = allocate_npin();
        nnet_t* new_net_clr = allocate_nnet();
        new_net_clr->name = make_full_ref_name(NULL, NULL, NULL, mux_clr[i]->name, 0);
        /* hook the output pin into the node */
        add_output_pin_to_node(mux_clr[i], new_pin1_clr, 0);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net_clr, new_pin1_clr);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net_clr, new_pin2_clr);

        /*****************************************************************************************/
        /**************************************** FF_NODE ****************************************/
        /*****************************************************************************************/
        /* create FF node for DFFSR output */
        nnode_t* ff_node = allocate_nnode(node->loc);

        ff_node->type = FF_NODE;
        ff_node->traverse_visited = traverse_mark_number;
        ff_node->attributes->clk_edge_type = node->attributes->clk_edge_type;

        ff_node->name = node_name(ff_node, NULL);

        add_input_port_information(ff_node, 2);
        allocate_more_input_pins(ff_node, 2);

        add_output_port_information(ff_node, 1);
        allocate_more_output_pins(ff_node, 1);

        /**
         * remap the CLK pin from the dffsr node to the ff node
         * since we do not need it in dff node anymore 
         **/
        if (i == width - 1) {
            /**
             * remap the CLK pin from the dffsr node to the new  
             * ff node since we do not need it in dffsr node anymore 
             **/
            remap_pin_to_new_node(node->input_pins[0], ff_node, 1);
        } else {
            /* add a copy of CLK pin from the dffsr node to the new ff node */
            add_input_pin_to_node(ff_node, copy_input_npin(node->input_pins[0]), 1);
        }


        /**
         * remap D[i] from the dffsr node to the ff node 
         * since we do not need it in dff node anymore 
         **/
        add_input_pin_to_node(ff_node, new_pin2_clr, 0);

        // remap node's output pin to mux_clr
        remap_pin_to_new_node(node->output_pins[i], ff_node, 0);

        netlist->ff_nodes = (nnode_t**)vtr::realloc(netlist->ff_nodes, sizeof(nnode_t*) * (netlist->num_ff_nodes + 1));
        netlist->ff_nodes[netlist->num_ff_nodes] = ff_node;
        netlist->num_ff_nodes++;


    }
    vtr::free(mux_set);
    vtr::free(mux_clr);
    free_nnode(node);
}

/**
 * (function: resolve_pmux_node)
 * 
 * @brief resolving the pmux node including many input
 * multiplexing using a one-hot select signal
 * 
 * @param node pointing to a logical not node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */

static void resolve_pmux_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
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
        level_muxes[i] = make_3port_gate(MULTIPORT_nBIT_MUX, 1, width, width, width, node, traverse_mark_number);
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
            active_bit_muxes[active_idx] = make_3port_gate(MULTIPORT_nBIT_MUX, 1, 1, 1, 1, node, traverse_mark_number);

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
            ternary_muxes[ternary_idx] = make_3port_gate(MULTIPORT_nBIT_MUX, 1, width, width, width, node, traverse_mark_number);
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
            signal_list_t* shifted_B = constant_shift(signal_B_to_shift, i, SR, width, netlist);

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

        if (i < selector_width-1) {
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

/*******************************************************************************************************
 ********************************************** [UTILS] ************************************************
 *******************************************************************************************************/
/**
 *---------------------------------------------------------------------------------------------
 * (function: look_for_clocks)
 * 
 * @brief going through all FF nodes looking for the clock signals.
 * If they are not clock type, they should alter to one. Since BUF
 * nodes be removed in the partial mapping, the driver of the BUF
 * nodes should be considered as a clock node. 
 * 
 * @param netlist pointer to the current netlist file
 *-------------------------------------------------------------------------------------------*/
static void look_for_clocks(netlist_t* netlist) {
    int i;
    /* looking for the global sim clock among top netlist inputsto change its type if needed */
    for (i = 0; i < netlist->num_top_input_nodes; i++) {
        nnode_t* input_node = netlist->top_input_nodes[i];
        if (!strcmp(input_node->name, DEFAULT_CLOCK_NAME))
            input_node->type = CLOCK_NODE;
    }

    for (i = 0; i < netlist->num_ff_nodes; i++) {
        /* at this step, clock nodes must have been driving by one driver */
        oassert(netlist->ff_nodes[i]->input_pins[1]->net->num_driver_pins == 1);
        /* node: clock driver node */
        nnode_t* node = netlist->ff_nodes[i]->input_pins[1]->net->driver_pins[0]->node;

        /* as far as a clock driver node is a BUF node, going through its drivers till finding a non-BUF node */
        while (node->type == BUF_NODE)
            node = node->input_pins[0]->net->driver_pins[0]->node;

        if (node->type != CLOCK_NODE) {
            node->type = CLOCK_NODE;
        }
    }
}

/**
 * (function: constant_shift)
 * 
 * @brief performing constant shift operation on given signal_list
 * 
 * @param input_signals input to be shifted 
 * @param shift_size vonstant shift size
 * @param shift_type shift type: SL, SR, ASL or ASR
 * @param assignment_size width of the output
 * @param netlist pointer to the current netlist file
 */
static signal_list_t* constant_shift(signal_list_t* input_signals, const int shift_size, const operation_list shift_type, const int assignment_size, netlist_t* netlist) {
    signal_list_t* return_list = init_signal_list();

    /* record the size of the shift */
    int input_width = input_signals->count;
    int output_width = assignment_size;
    // int pad_bit = input_width - 1;

    int i;
    switch (shift_type) {
        case SL:
        case ASL: {
            /* connect ZERO to outputs that don't have inputs connected */
            for (i = 0; i < shift_size; i++) {
                if (i < output_width) {
                    // connect 0 to lower outputs
                    npin_t* zero_pin = allocate_npin();
                    add_fanout_pin_to_net(netlist->zero_net, zero_pin);

                    add_pin_to_signal_list(return_list, zero_pin);
                    zero_pin->node = NULL;
                }
            }

            /* connect inputs to outputs */
            for (i = 0; i < output_width - shift_size; i++) {
                if (i < input_width) {
                    // connect higher output pin to lower input pin
                    add_pin_to_signal_list(return_list, input_signals->pins[i]);
                    input_signals->pins[i]->node = NULL;
                } else {
                    npin_t* extension_pin = get_zero_pin(netlist);

                    add_pin_to_signal_list(return_list, extension_pin);
                    extension_pin->node = NULL;
                }
            }
            break;
        }
        case SR: //fallthrough
        case ASR: {
            for (i = shift_size; i < input_width; i++) {
                // connect higher output pin to lower input pin
                if (i - shift_size < output_width) {
                    add_pin_to_signal_list(return_list, input_signals->pins[i]);
                    input_signals->pins[i]->node = NULL;
                }
            }

            /* Extend pad_bit to outputs that don't have inputs connected */
            for (i = output_width - 1; i >= input_width - shift_size; i--) {
                npin_t* extension_pin = NULL;
                // [TODO]: Extra potential feature, check for signedness
                // if (op->children[0]->types.variable.signedness == SIGNED && operation_node->type == ASR) {
                //     extension_pin = copy_input_npin(input_signals->pins[pad_bit]);
                // } else {
                extension_pin = get_zero_pin(netlist);
                // }

                add_pin_to_signal_list(return_list, extension_pin);
                extension_pin->node = NULL;
            }
            break;
        }
        default:
            error_message(NETLIST, unknown_location, "%s", "Internal error, operation is not supported by Odin!\n");
            break;
    }

    //CLEAN UP
    free_signal_list(input_signals);

    return (return_list);
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
static signal_list_t** modify_div_signal_sizes (nnode_t* node, netlist_t* netlist) {
    int i;

    int dividend_width = node->input_port_sizes[0];
    int divisor_width  = node->input_port_sizes[1];

    /* new widths which will be adjusted in the following */
    int new_dividend_width = dividend_width;
    /* m = 2*n -1 */
    if (dividend_width < 2 * divisor_width - 1) {
        /* need for padding dividend with GND */
        while (new_dividend_width < 2 * divisor_width - 1)
            new_dividend_width++;
    }

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
    /* Dividend will be padded by adding zero as LSB (right side) */
    /* hook divisor pins into new node (literally, shift left if needed) */
    for (i = 0; i < divisor_width; i++) {
        add_pin_to_signal_list(divisor_signal_list, node->input_pins[dividend_width + i]);
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

    int dividend_size  = dividend_sig_list->count;
    int divisor_size   = divisor_sig_list->count;
    int quotient_size  = divisor_size;
    int remainder_size = divisor_size;

    /* checking the division circuit restrictions */
    oassert(dividend_size == 2 * divisor_size - 1);

    /* CR nodes array for cellular architecture (need divisor_size row) */
    signal_list_t*** CR_outputs = (signal_list_t***)vtr::calloc(divisor_size, sizeof(signal_list_t**));

    /* creating CR nodes for each row to do the partial division */
    int offset = divisor_size;
    
    /* creating the network */
    for (i = 0; i < divisor_size; i++) {
        /**
         * <DIV internal circuit>                                                                                                                                *
         *        D[m-1] d[n-1]          D[m-n-1] d[n-2]           D[m-n] d[0]                                                             *
         *           |    |                |    |                   |    |                                                                 *
         *           |    |                |    |                   |    |                                                                 *
         *     b    _v____v_   b_     b   _v____v_    b_      b    _v____v_    b_                                                          *
         *  <------|   CR   |<-----------|   CR   |<----    ------|   CR   |<--                                                            *
         * |       |[0][n-1]|            |[0][n-2]|     ....      | [0][0] |                                                               *
         * |__|\___|  i  j  |            |  i  j  |     ....      |  i  j  |                                                               *
         *    |/   |________|----------->|________|-----    ----->|________|-- c_0                                                         *
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
         *                              |/   |________|----------->|________|----    ------>|________|-------->|________|--> c[2]          *
         *                                _c               c    _c            c             _c          c   _c                             *
         *                                                                                                                                 *
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
            npin_t* x  = NULL; /* CR_node_input_signals->pins[0] */
            npin_t* y  = NULL; /* CR_node_input_signals->pins[1] */
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
                nnode_t* not_node = make_1port_gate(LOGICAL_NOT, 1, 1, node, node->traverse_visited);
                /* hook the last borrow pin to the not gate */
                add_input_pin_to_node(not_node, b, 0);

                /* connecting the not_node output pin */
                npin_t* new_pin1 = allocate_npin();
                npin_t* new_pin2 = allocate_npin();
                nnet_t* new_net = allocate_nnet();
                new_net->name = make_full_ref_name(NULL, NULL, NULL, not_node->name, 0);
                /* hook the output pin into the node */
                add_output_pin_to_node(not_node, new_pin1, 0);
                /* hook up new pin 1 into the new net */
                add_driver_pin_to_net(new_net, new_pin1);
                /* hook up the new pin 2 to this new net */
                add_fanout_pin_to_net(new_net, new_pin2);

                /* hook the not node output to the last CR node _c input*/
                _c = new_pin2;
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
        }

        /**
         * increasing the offset by 1 in each iteration to 
         * connect all dividend pins to the first row CR 
         * nodes and all first CR nodes in other rows 
        */
        offset++;
    }

    /* validate the size of output signal lists */
    oassert(quotient_size == quotient_signal_list->count);
    oassert(remainder_size == remainder_signal_list->count);

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
static void connect_div_output_pins (nnode_t* node, signal_list_t** output_signals, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(output_signals);
    
    int i;
    int divisor_width  = node->input_port_sizes[1];
    int quotient_width = node->output_port_sizes[0];

    /* quotient and remainder signal lists returned by implement_division function */
    signal_list_t* quotient_signal_list = output_signals[0];
    signal_list_t* remainder_signal_list = output_signals[1];

    int new_quotient_width   = quotient_signal_list->count;
    int new_remainder_width  = remainder_signal_list->count;
    
    /* validate new sizes */
    oassert(new_quotient_width == divisor_width);
    oassert(new_quotient_width == new_remainder_width);

    if (quotient_width >= new_quotient_width) {       
        for (i = 0; i < quotient_width; i++) {
            npin_t* quotient_pin = quotient_signal_list->pins[i];
            /* creating a buf node to cionnect the calculated quotient to the main div node outputs */
            nnode_t* buf_node = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);

            if (i < new_quotient_width) {    /* connect the calculatd quotient pin as buf node driver */
                add_input_pin_to_node(buf_node, quotient_pin, 0);
                /* remap the main div output pin to the buf node output pin */
                remap_pin_to_new_node(node->output_pins[i], buf_node, 0);
            } else {
                /* connect the calculatd quotient pin as buf node driver */
                add_input_pin_to_node(buf_node, get_zero_pin(netlist), 0);
                /* remap the main div output pin to the buf node output pin */
                remap_pin_to_new_node(node->output_pins[i], buf_node, 0);
            }
        }
        
        
    } else {
        for (i = 0; i < new_quotient_width; i++) {
            npin_t* quotient_pin = quotient_signal_list->pins[i];
            /* creating a buf node to cionnect the calculated quotient to the main div node outputs */
            nnode_t* buf_node = make_1port_gate(BUF_NODE, 1, 1, node, traverse_mark_number);

            if (i < quotient_width) {
                add_input_pin_to_node(buf_node, quotient_pin, 0);
                /* remap the main div output pin to the buf node output pin */
                remap_pin_to_new_node(node->output_pins[i], buf_node, 0);
            } else {
                /* dump the calculated pin since it is extra */
                remove_fanout_pins_from_net(quotient_pin->net,
                                            quotient_pin,
                                            quotient_pin->pin_net_idx);
            }
        }
    }


    /* dunp remainders */
    for (i = 0; i < new_remainder_width; i++) {
        npin_t* remainder_pin = remainder_signal_list->pins[i];
        remove_fanout_pins_from_net(remainder_pin->net,
                                    remainder_pin,
                                    remainder_pin->pin_net_idx);
    }
}

/**
 * (function: make_division_by_constant_node)
 * 
 * @brief performing division by constant (A / 3)
 * 
 * @note this should perfom before partial mapping since
 * some nodes like minus are not resolved yet
 * 
 * @param node pointer to the div node
 * @param netlist pointer to the current netlist file
 */
static void make_division_by_constant_node (netlist_t* /* netlist */) {
    return;
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
static void make_CR_node (nnode_t* node, signal_list_t* input_signal_list, signal_list_t* output_signal_list) {
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
    npin_t* x  = input_signal_list->pins[0];
    npin_t* y  = input_signal_list->pins[1];
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
    if (b_->node)
        remap_pin_to_new_node(b_, fs_node, 0);
    else
        add_input_pin_to_node(fs_node, b_, 0);

    if (x->node)
        remap_pin_to_new_node(x, fs_node, 1);
    else
        add_input_pin_to_node(fs_node, x, 1);

    if (y->node)
        remap_pin_to_new_node(y, fs_node, 2);
    else
        add_input_pin_to_node(fs_node, y, 2);

    /** 
     * connecting output pins 
     * [0]: b
     * [1]: fs_out
    */
    /* b is already created so we only need to hook it up to the node */
    add_output_pin_to_node(fs_node, b, 0);
    b->net->name = make_full_ref_name(NULL, NULL, NULL, fs_node->name, 0);

    /* need to create the fs_output_pin as an internal pin */
    npin_t* new_pin1 = allocate_npin();
    npin_t* fs_output_pin = allocate_npin();
    nnet_t* new_net  = allocate_nnet();
    new_net->name = make_full_ref_name(NULL, NULL, NULL, fs_node->name, 1);
    /* hook the output pin into the node */
    add_output_pin_to_node(fs_node, new_pin1, 1);
    /* hook up new pin 1 into the new net */
    add_driver_pin_to_net(new_net, new_pin1);
    /* hook up the new pin 2 to this new net */
    add_fanout_pin_to_net(new_net, fs_output_pin);    
    

    /*********************************************************************************************************
     ************************************************** MUX **************************************************
     *********************************************************************************************************/
    /* creating the multiplexer node */
    nnode_t* mux_node = make_3port_gate(MULTIPORT_nBIT_MUX, 1, 1, 1, 1, node, node->traverse_visited);
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
static signal_list_t* CR_output_signal_init () {
    /* signal list to be returned */
    signal_list_t* return_sig_list = init_signal_list();

    npin_t* b     = allocate_npin();
    npin_t* b_out = allocate_npin();
    nnet_t* b_net = allocate_nnet();
    /* hook up c into the c net */
    add_driver_pin_to_net(b_net, b);
    /* hook up the c_out to this c_net */
    add_fanout_pin_to_net(b_net, b_out);
    /* storing in the CR output signal for CR creation function */
    add_pin_to_signal_list(return_sig_list, b_out);

    npin_t* p     = allocate_npin();
    npin_t* p_out = allocate_npin();
    nnet_t* p_net = allocate_nnet();
    /* hook up p into the p net */
    add_driver_pin_to_net(p_net, p);
    /* hook up the p_out to this p_net */
    add_fanout_pin_to_net(p_net, p_out);
    /* storing in the CR output signal for CR creation function */
    add_pin_to_signal_list(return_sig_list, p_out);
    
    npin_t* c     = allocate_npin();
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