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

#include "blif_elaborate.h"
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

void check_block_ports(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
void add_dummy_carry_out_to_adder_hard_block(nnode_t* new_node);

/*-------------------------------------------------------------------------
 * (function: blif_elaborate_top)
 *-----------------------------------------------------------------------*/
void blif_elaborate_top(netlist_t* netlist) {
    /* 
     * depending on the input blif target choose how to do elaboration 
     * Worth noting blif elaboration does not perform for odin's blif
     * since it is already generated from odin's partial mapping
     */
    if (configuration.blif_type == blif_type_e::_YOSYS_BLIF) {
        /* do the elaboration without any larger structures identified */
        depth_first_traversal_to_blif_elaborate(YOSYS_BLIF_ELABORATE_TRAVERSE_VALUE, netlist);
    } /*
       * else if (...)
       * This spot could be used for other blif files
       * generated from different parsing tools
       */
}

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traversal_to_blif_elaborate()
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

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traverse)
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

/*----------------------------------------------------------------------
 * (function: partial_map_node)
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
        case BITWISE_NOT:
        case BUF_NODE:
        case BITWISE_AND:
        case BITWISE_OR:
        case BITWISE_NAND:
        case BITWISE_NOR:
        case BITWISE_XNOR:
        case BITWISE_XOR:
        case LOGICAL_OR:
        case LOGICAL_AND:
        case LOGICAL_NOR:
        case LOGICAL_NAND:
        case LOGICAL_XOR:
        case LOGICAL_XNOR:
        case LOGICAL_NOT:
        case MINUS:
        case LOGICAL_EQUAL:
        case NOT_EQUAL:
        case GTE:
        case LTE:
        case GT:
        case LT:
        case SL:
        case ASL:
        case SR:
        case ASR:
        case MULTI_PORT_MUX:
        case MULTIPLY:
        case MEMORY:
        case HARD_IP:
        case ADDER_FUNC:
        case CARRY_FUNC:
        case MUX_2:
        case INPUT_NODE:
        case CLOCK_NODE:
        case OUTPUT_NODE:
        case GND_NODE:
        case VCC_NODE:
        case FF_NODE:
        case PAD_NODE:
        case CASE_EQUAL:
        case CASE_NOT_EQUAL:
        case DIVIDE:
        case MODULO:
        default:
            // error_message(NETLIST, node->loc, "%s", "BLIF Elaboration: node should have been converted to softer version.");
            break;
    }
}

/*-------------------------------------------------------------------------------------------
 * (function: check_block_ports )
 * check for missing ports such as carry-in/out in case of dealing 
 * with generated netlist from other blif files such Yosys.
 *-----------------------------------------------------------------------------------------*/
void check_block_ports(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node);
    oassert(netlist);

    if (node->traverse_visited == traverse_mark_number)
        return;

    if (configuration.blif_type == blif_type_e::_YOSYS_BLIF) {
        switch (node->type) {
            case ADD: {
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
            case MINUS:
            case MULTIPLY:
            default: {
                error_message(NETLIST, node->loc,
                              "This should not happen for node(%s) since the check block port function only should have called for add, sub or mult", node->name);
            }
        }
    } /*else other blif files*/
}

/*
 * Adding a dummy carry out output pin to the adder hard block 
 * for the possible future processing of soft adder instatiation
 */
void add_dummy_carry_out_to_adder_hard_block(nnode_t* new_node) {
    char* dummy_cout_name = (char*)vtr::malloc(11 * sizeof(char));
    strcpy(dummy_cout_name, "dummy_cout");

    npin_t* new_pin = allocate_npin();
    new_pin->name = vtr::strdup(dummy_cout_name);
    new_pin->type = OUTPUT;

    char* mapping = (char*)vtr::malloc(6 * sizeof(char));
    strcpy(mapping, "cout");
    new_pin->mapping = vtr::strdup(mapping);

    //add_output_port_information(new_node, 1);
    new_node->output_port_sizes[new_node->num_output_port_sizes - 1]++;
    allocate_more_output_pins(new_node, 1);

    add_output_pin_to_node(new_node, new_pin, new_node->num_output_pins - 1);

    nnet_t* new_net = allocate_nnet();
    new_net->name = vtr::strdup(dummy_cout_name);

    add_driver_pin_to_net(new_net, new_pin);

    vtr::free(dummy_cout_name);
    vtr::free(mapping);
}