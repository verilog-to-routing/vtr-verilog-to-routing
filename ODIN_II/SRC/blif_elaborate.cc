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

static nnode_t* check_block_ports(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
// static void add_dummy_carry_out_to_adder_hard_block(nnode_t* new_node);
static void transform_to_single_bit_dff_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
// static void remap_input_pins_drivers_based_on_mapping (nnode_t* node);
static void make_selector_as_first_port(nnode_t* node);
static void resolve_logical_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void resolve_sdff_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void resolve_dffe_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void resolve_adffe_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void resolve_sdffe_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void resolve_dffsr_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void resolve_pmux_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static signal_list_t* constant_shift (signal_list_t* input_signals, const int shift_size, const operation_list shift_type, const int assignment_size, netlist_t* netlist);
static nnode_t* resolve_case_equal_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static nnode_t* resolve_case_not_equal_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);

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
        case ADD: {
            /** 
             * check for missing ports such as carry-in/out in case of 
             * dealing with generated netlist from Yosys blif file.
             */
            if (hard_adders)
                node = check_block_ports(node, traverse_number, netlist);

            add_list = insert_in_vptr_list(add_list, node);
            break;
        }
        case MINUS: {
            /** 
             * check for missing ports such as carry-in/out in case of 
             * dealing with generated netlist from Yosys blif file.
             */
            if (hard_adders)
                node = check_block_ports(node, traverse_number, netlist);

            sub_list = insert_in_vptr_list(sub_list, node);
            break;
        }
        case FF_NODE: {
            /**
             * split the dff node read from yosys blif to
             * FF nodes with input/output width one 
             */
            transform_to_single_bit_dff_nodes(node, traverse_number, netlist);
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
        case MULTIPLY:
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
        case SL:
        case ASL:
        case SR:
        case ASR:
        case MEMORY:
        case HARD_IP:
        case ADDER_FUNC:
        case CARRY_FUNC:
        case CLOCK_NODE:
        case DIVIDE:
        case GENERIC:
        case MODULO:
        default:
            error_message(BLIF_ELBORATION, node->loc, "node (%s: %s) should have been converted to softer version.", node->type, node->name);
            break;
    }
}

/**
 *-------------------------------------------------------------------------------------------
 * (function: check_block_ports )
 * 
 * @brief check for missing ports such as carry-in/out in case of 
 * dealing with generated netlist from other blif files such Yosys.
 * 
 * @param node pointing to the netlist node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 *-----------------------------------------------------------------------------------------*/
static nnode_t* check_block_ports(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);

    nnode_t* new_node = NULL;

    if (configuration.coarsen) {
        switch (node->type) {
            case ADD:
            case MINUS: {
                if (node->num_input_port_sizes == 2) {
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
                }

                free_nnode(node);
                break;
            }
            case MULTIPLY:
            default: {
                error_message(BLIF_ELBORATION, node->loc,
                              "This should not happen for node(%s) since the check block port function only should have called for add, sub or mult", node->name);
            }
        }
    }

    return new_node;
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
 * (function: transform_to_single_bit_dff_nodes)
 * 
 * @brief split the dff node read from yosys blif to
 * FF nodes with input/output width one
 * 
 * @param node pointing to the dff node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
static void transform_to_single_bit_dff_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_output_pins + 1 == node->num_input_pins);

    int i;
    int num_ff_nodes = node->num_output_pins;

    /**
     * input_pin[0] -> CLK
     * input_pin[1..n] -> D
     * output_pin[0..n-1] -> Q
     */
    for (i = 0; i < num_ff_nodes; i++) {
        nnode_t* ff_node = allocate_nnode(node->loc);

        ff_node->type = FF_NODE;
        ff_node->traverse_visited = traverse_mark_number;
        ff_node->attributes->clk_edge_type = node->attributes->clk_edge_type;

        ff_node->name = node_name(ff_node, NULL);
        //[todo] check the num of ports
        add_input_port_information(ff_node, 2);
        allocate_more_input_pins(ff_node, 2);

        add_output_port_information(ff_node, 1);
        allocate_more_output_pins(ff_node, 1);

        if (i == num_ff_nodes - 1) {
            /**
             * remap the CLK pin from the dff node to the last splitted 
             * ff node since we do not need it in dff node anymore 
             **/
            remap_pin_to_new_node(node->input_pins[0], ff_node, 1);
        } else {
            /* add a copy of CLK pin from the dff node to the splitted ff node */
            add_input_pin_to_node(ff_node, copy_input_npin(node->input_pins[0]), 1);
        }

        /**
         * remap the input_pin[i+1]/output_pin[i] from the dff node to the 
         * last splitted ff node since we do not need it in dff node anymore 
         **/
        remap_pin_to_new_node(node->input_pins[i + 1], ff_node, 0);
        remap_pin_to_new_node(node->output_pins[i], ff_node, 0);

        netlist->ff_nodes = (nnode_t**)vtr::realloc(netlist->ff_nodes, sizeof(nnode_t*) * (netlist->num_ff_nodes + 1));
        netlist->ff_nodes[netlist->num_ff_nodes] = ff_node;
        netlist->num_ff_nodes++;
    }

    free_nnode(node);
}

/**
 * (function: remap_input_pins_drivers_based_on_mapping)
 * 
 * @brief make the complete name of a pin using mapping record and node name. 
 * Finding its net, the function would connect the input pin as its driver pin  
 * 
 * @param node pointing to the netlist node 
 */
/* static void remap_input_pins_drivers_based_on_mapping (nnode_t* node) {
    int i, j, k;
    int acc_port_sizes = 0;
    for (i = 0; i < node->num_input_port_sizes; i++) {
        for (j = 0; j < node->input_port_sizes[i]; j++) {
            npin_t* input_pin = node->input_pins[j + acc_port_sizes];
            npin_t* input_pin_driver_pin = input_pin->net->driver_pins[0];
            char* net_name = make_full_ref_name(node->name, NULL, NULL, input_pin->mapping, j);

            nnet_t* output_net = (nnet_t*)output_nets_hash->get(net_name);

            if (!output_net)
                error_message(BLIF_ELBORATION, my_location, "Error: Could not hook up the pin %s: not available.", input_pin->name);


            output_net->driver_pins[0]->net = NULL;
            free_nnode(output_net->driver_pins[0]->node); //[TODO] free related node

            output_net->num_driver_pins = 0;
            add_driver_pin_to_net(output_net, input_pin_driver_pin);
            vtr::free(net_name);
        }
        acc_port_sizes += node->input_port_sizes[i];
    }

    acc_port_sizes = 0;
    for (i = 0; i < node->num_output_port_sizes; i++) {
        for (j = 0; j < node->output_port_sizes[i]; j++) {
            npin_t* output_pin = node->output_pins[j + acc_port_sizes];
            nnet_t* node_output_net = output_pin->net;
            char* net_name = make_full_ref_name(node->name, NULL, NULL, output_pin->mapping, j);

            // find the new driver and its net
            nnet_t* mapped_output_net = (nnet_t*)output_nets_hash->get(net_name);

            if (!mapped_output_net)
                error_message(BLIF_ELBORATION, my_location, "Error: Could not hook up the pin %s: not available.", node_output_net->name);
            
            remove_fanout_pins_from_net(mapped_output_net, mapped_output_net->fanout_pins[0], 0);
            for (k = 0; k < node_output_net->num_fanout_pins; k++) {
                add_fanout_pin_to_net(mapped_output_net, node_output_net->fanout_pins[k]);
            }
            vtr::free(net_name);
        }
        acc_port_sizes += node->input_port_sizes[i];
    }
} */

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
         * <pmux internal>
         * 
         *              S[0]
         *               |                 S[1] 
         *             |\|                  |                                S[2]
         *             | \                |\|                                 |
         *      A ---  |0:|               | \                               |\|
         *             |  | ------------->|0:|                              | \
         *      B ---  |1:|               |  | ---------------------------->|0:|
         * [width-1:0] | /          ----->|1:|                              |  | -->       ...
         *             |/           |     | /                     --------->|1:|
         *       (level_mux_0)      |     |/                      |         | /
         *                          |(level_mux_1)                |         |/
         *                          |                             |    (level_mux_1)       
         *              S[0]        |                             |   
         *               |          |                 S[1]        |   
         *             |\|          |                  |          |  
         *             | \          |                |\|          |   
         *       0 --->|0:|         |                | \          |   
         *             |  |---------0--------------->|0:|         |                                      
         *       1 --->|1:|   |     |                |  |---------0---->         ...                                      
         *             | /    |     |          1 --->|1:|   |     |                                              
         *             |/     |     |                | /    |     |                                                                 
         *        (active_bit |     |                |/     |     |                                          
         *          _mux_0)   |     |           (active_bit |     |                             
         *                    |     |              _mux_1)  |     |                                
         *                    |     |                       |     |                                                                                                
         *                  |\|     |                       |     |                                                                                                                           
         *                  | \     |                       |     |                                                                                                               
         *     B >> 2^1 --->|0:|    |                     |\|     |                                                                   
         *                  |  |-----                     | \     |                                                                   
         *      'hxxx   --->|1:|             B >> 2^2 --->|0:|    |                                                                       
         *                  | /                           |  |-----                                                                   
         *                  |/                'hxxx   --->|1:|                                                                        
         *            (ternay_mux_0)                      | /                                     
         *                                                |/                                          
         *                                          (ternay_mux_1)        
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

    return return_list;
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