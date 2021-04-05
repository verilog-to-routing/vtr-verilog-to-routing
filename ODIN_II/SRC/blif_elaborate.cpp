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

void blif_elaborate_node(nnode_t* node, short traverse_number, netlist_t* netlist);

void check_block_ports(nnode_t* node, netlist_t* netlist);
void add_dummy_carry_out_to_adder_hard_block(nnode_t* new_node);

/*-------------------------------------------------------------------------
 * (function: blif_elaborate_top)
 *-----------------------------------------------------------------------*/
void blif_elaborate_top(netlist_t* netlist) {
    /* depending on the input blif target choose how to do elaboration */
    if (strcmp(configuration.output_type.c_str(), "blif") == 0) {
        /* do the elaboration without any larger structures identified */
        depth_first_traversal_to_blif_elaborate(BLIF_ELABORATE_TRAVERSE_VALUE, netlist);
    }
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
void blif_elaborate_node(nnode_t* node, short /*traverse_number*/, netlist_t* netlist) {
    switch (node->type) {
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
            break;
        case ADD: {
            /* 
             * check for missing ports such as carry-in/out in case of 
             * dealing with generated netlist from Yosys blif file.
             */
            if (hard_adders)
                check_block_ports(node, netlist);

            add_list = insert_in_vptr_list(add_list, node);
            break;
        }
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
 * check for missing ports such as carry-in/out in case of 
 * dealing with generated netlist from Yosys blif file.
 *-----------------------------------------------------------------------------------------*/
void check_block_ports(nnode_t* node, netlist_t* netlist) {
    oassert(node);
    oassert(netlist);

    switch(node->type) {

        case ADD: {
            if (node->num_input_port_sizes == 2) {
                add_input_port_information(node, 1);
                allocate_more_input_pins(node, 1);

                char* cin_name = make_full_ref_name(NULL, NULL, node->name, "cin", 0);
                npin_t* cin_pin = get_pad_pin(netlist);
                cin_pin->name = vtr::strdup(cin_name);
                cin_pin->type = INPUT;
                cin_pin->mapping = vtr::strdup("cin");

                add_input_pin_to_node(node, cin_pin, node->num_input_pins-1); 
            }


            // int i;
            // for (i = 0; i < node->num_output_pins ; i++) {
                
            //     npin_t* new_output_pin = allocate_npin();
            //     npin_t* output_pin = node->output_pins[i];

            //     node->output_pins[i] = new_output_pin;

            //     new_output_pin->type = OUTPUT;
            //     new_output_pin->node = output_pin->node;
            //     new_output_pin->pin_node_idx = output_pin->pin_node_idx;
            //     // [TODO]cout_pin->mapping = vtr::strdup("cout");

            //     output_pin->node = NULL;
            //     output_pin->pin_node_idx = -1;

            //     char* new_output_pin_name = (char*)vtr::malloc((strlen(node->name) + 20) * sizeof(char)); /* 6 chars for pin idx */
            //     odin_sprintf(new_output_pin_name, "%s[1]", node->name);
            //     new_output_pin->name = new_output_pin_name;

            //     nnet_t* output_net = allocate_nnet();
            //     output_net->name = output_net->name;

            //     add_driver_pin_to_net(output_net, new_output_pin);
            //     add_fanout_pin_to_net(output_net, output_pin->net->fanout_pins[0]);

            //     new_output_pin->pin_net_idx = output_net->num_fanout_pins-1;
            //     free_npin(output_pin);
            // }


            // if (node->num_output_port_sizes == 1) {
            //     add_output_port_information(node, 1);
            //     allocate_more_output_pins(node, 1);

            //     int i;
            //     for (i = node->num_output_pins-1; i > 0 ; i--) {
            //         move_output_pin(node, i-1, i);
            //     }

            //     node->output_port_sizes[0] = 1;
            //     node->output_port_sizes[1] = node->num_output_pins-1;

            //     char* cout_name = (char*)vtr::malloc((strlen(node->name) + 20) * sizeof(char)); /* 6 chars for pin idx */
            //     odin_sprintf(cout_name, "%s[0]", node->name);
                
            //     npin_t* cout_pin = allocate_npin();
            //     cout_pin->name = vtr::strdup(cout_name);
            //     cout_pin->type = OUTPUT;
            //     cout_pin->mapping = vtr::strdup("cout");

            //     add_output_pin_to_node(node, cout_pin, 0);

            //     nnet_t* cout_net = allocate_nnet();
            //     cout_net->name = vtr::strdup(cout_name);

            //     add_driver_pin_to_net(cout_net, cout_pin);

            //     for (i = 1; i < node->num_output_pins; i++) {

            //         char* node_output_name = (char*)vtr::malloc((strlen(node->name) + 20) * sizeof(char)); /* 6 chars for pin idx */
            //         odin_sprintf(node_output_name, "%s[%d]", node->name, i);
            //         node->output_pins[i]->name = node_output_name;

            //         // nnet_t* net = node->output_pins[i]->net;

            //         // npin_t* output_pin = net->fanout_pins[0];
            //         // npin_t* pin = allocate_npin();

            //         // if (net->name == NULL)
            //         //     net->name = node->output_pins[i]->name;

            //         // pin->name = net->name;

            //         // remove_fanout_pins_from_net(net, output_pin, output_pin->pin_net_idx);
            //         // add_fanout_pin_to_net(net, pin);                    

            //         // nnode_t* buf_node = allocate_nnode(node->loc);
            //         // buf_node->type = BUF_NODE;
            //         // /* create the unique name for this gate */
            //         // buf_node->name = node_name(buf_node, node->name);

            //         // buf_node->related_ast_node = node->related_ast_node;
            //         // /* allocate the pins needed */
            //         // allocate_more_input_pins(buf_node, 1);
            //         // add_input_port_information(buf_node, 1);
            //         // allocate_more_output_pins(buf_node, 1);
            //         // add_output_port_information(buf_node, 1);

            //         // npin_t* buf_input_pin = pin;
            //         // add_input_pin_to_node(buf_node, buf_input_pin, 0);

            //         // /* finally hookup the output pin of the buffer to the orginal driver net */
            //         // npin_t* buf_output_pin = output_pin;
            //         // add_output_pin_to_node(buf_node, buf_output_pin, 0);

            //         // nnet_t* output_net = 
            //         // add_driver_pin_to_net(net, buf_output_pin);
            //     }
                

            //     // Index the net by name.
            //     //output_nets_sc->add("cout", cout_net);
            //     //sc_add_string(output_nets_sc, cout_name);
            // }

            break;
        }
        case MINUS: 
        case MULTIPLY: 
        default: {
            error_message(NETLIST, node->loc, 
                         "This should not happen for node(%s) since the check block port function only should have called for add, sub or mult", node->name);
        }
    }
}

/*
 * Adding a dummy carry out output pin to the adder hard block 
 * for the possible future processing of soft adder instatiation
 */
void add_dummy_carry_out_to_adder_hard_block(nnode_t* new_node) {
    char* dummy_cout_name = (char*)vtr::malloc(11*sizeof(char));
    strcpy(dummy_cout_name, "dummy_cout");

    // model->outputs->names = (char**)vtr::realloc(model->outputs->names, sizeof(char*) * (model->outputs->count + 1));
    // model->outputs->names[model->outputs->count++] =  vtr::strdup(dummy_cout_name);

    // char* mapping = model->outputs->names[i];
    // char* name = (char*)mapping_index->get(mapping);

    // if (!name) error_message(PARSE_BLIF, my_location, "Invalid hard block mapping: %s", model->outputs->names[i]);

    npin_t* new_pin = allocate_npin();
    new_pin->name = vtr::strdup(dummy_cout_name);
    new_pin->type = OUTPUT;
    // new_pin->mapping = get_hard_block_port_name(mapping);

    //add_output_port_information(new_node, 1);
    new_node->output_port_sizes[new_node->num_output_port_sizes-1]++;
    allocate_more_output_pins(new_node, 1);

    add_output_pin_to_node(new_node, new_pin, new_node->num_output_pins - 1);

    nnet_t* new_net = allocate_nnet();
    new_net->name = vtr::strdup(dummy_cout_name);

    add_driver_pin_to_net(new_net, new_pin);

    // Index the net by name.
    // output_nets_hash->add(dummy_cout_name, new_net);

    vtr::free(dummy_cout_name);
}