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

static void check_block_ports(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
// static void add_dummy_carry_out_to_adder_hard_block(nnode_t* new_node);
static void transform_to_one_bit_dff_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void transform_to_one_bit_mux_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void resolve_instance_node(nnode_t* node);
static void remap_input_pins_drivers_based_on_mapping (nnode_t* node);
static void make_selector_as_the_first_port(nnode_t* node);

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
     * depending on the input blif target choose how to do elaboration 
     * Worth noting blif elaboration does not perform for odin's blif
     * since it is already generated from odin's partial mapping
     */
    if (configuration.in_blif_type == blif_type_e::_ODIN_BLIF) {
        /**
         *  nothing needs to be done since the netlist 
         *  is already compatible with Odin_II style 
         */
    }
    else if (configuration.in_blif_type == blif_type_e::_SUBCKT_BLIF) {
        /* do the elaboration without any larger structures identified */
        depth_first_traversal_to_blif_elaborate(SUBCKT_BLIF_ELABORATE_TRAVERSE_VALUE, netlist);
    } /*
       * else if (...)
       * This spot could be used for other blif files
       * generated from different parsing tools
       */
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
        case ADD: {
            /* 
             * check for missing ports such as carry-in/out in case of 
             * dealing with generated netlist from Yosys blif file.
             */
            if (hard_adders)
                check_block_ports(node, traverse_number, netlist);

            add_list = insert_in_vptr_list(add_list, node);
            break;
        }
        case MINUS: {
            /* 
             * check for missing ports such as carry-in/out in case of 
             * dealing with generated netlist from Yosys blif file.
             */
            if (hard_adders)
                check_block_ports(node, traverse_number, netlist);

            sub_list = insert_in_vptr_list(sub_list, node);
            break;
        }
        case FF_NODE: {
            /*
             * split the dff node read from yosys blif to
             * FF nodes with input/output width one 
             */
            transform_to_one_bit_dff_nodes(node, traverse_number, netlist);
            break;
        }
        case INSTANCE: {
            /*
             * INSTANCE nodes represent the module/funtion or task instantiation.
             * As a result, they are only showing the connection of an instantiated item.
             * Here we connect the drivers of instantiated instance to the input signals.
            */
            resolve_instance_node(node);
            break;
        }
        case MUX_2:
        case MULTI_PORT_MUX: 
        case MULTI_BIT_MUX_2: {
            /* need to reorder the input pins, so that the selector signal comes at the first place */
            make_selector_as_the_first_port(node);
            /* 
             * split the mux node into mux node with 
             * input/output width one 
             */
            transform_to_one_bit_mux_nodes(node, traverse_number, netlist);
            
            break;
        }
        case GND_NODE:
        case VCC_NODE:
        case PAD_NODE:
        case INPUT_NODE:
        case OUTPUT_NODE: 
        case BUF_NODE:
        case LOGICAL_OR:
        case LOGICAL_AND:
        case LOGICAL_NOR:
        case LOGICAL_NAND:
        case LOGICAL_XOR:
        case LOGICAL_XNOR:
        case LOGICAL_NOT:
        case LOGICAL_EQUAL: {
            /* some are already resolved for this phase */
            break;
        }
        case BITWISE_NOT:
        case BITWISE_AND:
        case BITWISE_OR:
        case BITWISE_NAND:
        case BITWISE_NOR:
        case BITWISE_XNOR:
        case BITWISE_XOR:
        case NOT_EQUAL:
        case GTE:
        case LTE:
        case GT:
        case LT:
        case SL:
        case ASL:
        case SR:
        case ASR:
        case MULTIPLY:
        case MEMORY:
        case HARD_IP:
        case ADDER_FUNC:
        case CARRY_FUNC:
        case CLOCK_NODE:
        case CASE_EQUAL:
        case CASE_NOT_EQUAL:
        case GENERIC:
        case DIVIDE:
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
static void check_block_ports(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node);
    oassert(netlist);

    if (node->traverse_visited == traverse_mark_number)
        return;

    if (configuration.in_blif_type == blif_type_e::_SUBCKT_BLIF) {
        switch (node->type) {
            case ADD:
            case MINUS: {
                if (node->num_input_port_sizes == 2) {
                    add_input_port_information(node, 1);
                    allocate_more_input_pins(node, 1);

                    char* cin_name = make_full_ref_name(NULL, NULL, node->name, "cin", 0);
                    npin_t* cin_pin = get_pad_pin(netlist);
                    cin_pin->name = vtr::strdup(cin_name);
                    cin_pin->type = INPUT;
                    cin_pin->mapping = vtr::strdup("cin");

                    add_input_pin_to_node(node, cin_pin, node->num_input_pins - 1);
                }
                break;
            }
            case MULTIPLY:
            default: {
                error_message(BLIF_ELBORATION, node->loc,
                              "This should not happen for node(%s) since the check block port function only should have called for add, sub or mult", node->name);
            }
        }
    } /*else other blif files*/
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
 * (function: transform_to_one_bit_dff_nodes)
 * 
 * @brief split the dff node read from yosys blif to
 * FF nodes with input/output width one
 * 
 * @param node pointing to the dff node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
static void transform_to_one_bit_dff_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
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
        /* [TODO]: clock sensitivity is not specified in the yosys blif file */
        ff_node->edge_type = ASYNCHRONOUS_SENSITIVITY;

        /* Name the flipflop based on the name of its output pin */
        const char* ff_base_name = node_name_based_on_op(ff_node);
        ff_node->name = (char*)vtr::malloc(sizeof(char) * (strlen(node->output_pins[i]->name) + strlen(ff_base_name) + 2));
        odin_sprintf(ff_node->name, "%s_%s", node->output_pins[i]->name, ff_base_name);
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
 * (function: transform_to_one_bit_mux_nodes)
 * 
 * @brief split the mux node read from yosys blif to
 * the same type nodes with input/output width one
 * 
 * @param node pointing to the mux node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
static void transform_to_one_bit_mux_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* /* netlist */) {
    int i, j;
    /* to check all mux inputs have the same width(except [0] which is selector) */
    for (i = 2; i < node->num_input_port_sizes; i++) {
        oassert(node->input_port_sizes[i] == node->input_port_sizes[1]);
    }
    
    int selector_width = node->input_port_sizes[0];
    int num_input_ports = node->num_input_port_sizes;
    int num_mux_nodes = node->num_output_pins;

    /**
     * input_port[0] -> SEL
     * input_pin[SEL_WIDTH..n] -> MUX inputs
     * output_pin[0..n-1] -> MUX outputs
     */
    for (i = 0; i < num_mux_nodes; i++) {
        nnode_t* mux_node = allocate_nnode(node->loc);

        mux_node->type = node->type;
        mux_node->traverse_visited = traverse_mark_number;

        /* Name the flipflop based on the name of its output pin */
        // const char* mux_base_name = node_name_based_on_op(mux_node);
        // mux_node->name = (char*)vtr::malloc(sizeof(char) * (strlen(node->output_pins[i]->name) + strlen(mux_base_name) + 2));
        // odin_sprintf(mux_node->name, "%s_%s", node->output_pins[i]->name, mux_base_name);
        mux_node->name = node_name(mux_node, NULL);

        add_input_port_information(mux_node, selector_width);
        allocate_more_input_pins(mux_node, selector_width);

        add_output_port_information(mux_node, 1);
        allocate_more_output_pins(mux_node, 1);

        if (i == num_mux_nodes - 1) {
            /**
             * remap the SEL pins from the mux node 
             * to the last splitted mux node  
             */
            for (j = 0; j < selector_width; j++) {
                remap_pin_to_new_node(node->input_pins[j], mux_node, j);
            }
                
        } else {
            /* add a copy of SEL pins from the mux node to the splitted mux nodes */
            for (j = 0; j < selector_width; j++) {
                add_input_pin_to_node(mux_node, copy_input_npin(node->input_pins[j]), j);
            }
        }

        /**
         * remap the input_pin[i+1]/output_pin[i] from the dff node to the 
         * last splitted ff node since we do not need it in dff node anymore 
         **/
        int acc_port_sizes = selector_width;
        for (j = 1; j < num_input_ports; j++) {
            add_input_port_information(mux_node, 1);
            allocate_more_input_pins(mux_node, 1);

            remap_pin_to_new_node(node->input_pins[i + acc_port_sizes], mux_node, selector_width + j - 1);
            acc_port_sizes += node->input_port_sizes[j];
        }

        remap_pin_to_new_node(node->output_pins[i], mux_node, 0);
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
static void remap_input_pins_drivers_based_on_mapping (nnode_t* node) {
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
}

static void resolve_instance_node(nnode_t* node) {
    /* to the input pin drivers' name based on the input pin mapping */
    remap_input_pins_drivers_based_on_mapping(node);
    /*
     * changes its type to buffer, 
     * since the signals are already connected, 
     * we do not need to keep instance node for final output 
     */
    node->type = BUF_NODE;
}

/**
 * (function: make_selector_as_the_first_port)
 * 
 * @brief reorder the input signals of the mux in a way 
 * that the selector would come as the first signal
 * 
 * @param node pointing to a mux node 
 */
void make_selector_as_the_first_port(nnode_t* node) {
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
    
    node->input_pins = input_pins;
    node->input_port_sizes = input_port_sizes;
}