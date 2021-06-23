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
#include "memories.h"
#include "block_memories.hh"
#include "memories.h"
#include "adders.h"
#include "division.hh"
#include "latch.hh"
#include "power.hh"
#include "flipflop.hh"
#include "shift.hh"
#include "modulo.hh"
#include "case_equal.hh"
#include "multiplexer.hh"
#include "subtractions.h"

#include "math.h"
#include "vtr_memory.h"
#include "vtr_util.h"

void depth_first_traversal_to_blif_elaborate(short marker_value, netlist_t* netlist);
void depth_first_traverse_blif_elaborate(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);

void blif_elaborate_node(nnode_t* node, short traverse_mark_number, netlist_t* netlist);

static void resolve_logical_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void resolve_shift_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void resolve_case_equal_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void resolve_arithmetic_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void resolve_mux_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void resolve_latch_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void resolve_ff_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static void resolve_memory_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);

/*******************************************************************************************************
 ********************************************** [UTILS] ************************************************
 *******************************************************************************************************/
static void look_for_clocks(netlist_t* netlist);

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
    } else if (configuration.coarsen) {
        /* init hashtable to index memory blocks and ROMs */
        init_block_memory_index();

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
        case GTE:           //fallthrough
        case LTE:           //fallthrough
        case GT:            //fallthrough
        case LT:            //fallthrough
        case LOGICAL_OR:    //fallthrough
        case LOGICAL_AND:   //fallthrough
        case LOGICAL_NOT:   //fallthrough
        case LOGICAL_NOR:   //fallthrough
        case LOGICAL_NAND:  //fallthrough
        case LOGICAL_XOR:   //fallthrough
        case LOGICAL_XNOR:  //fallthrough
        case LOGICAL_EQUAL: //fallthrough
        case NOT_EQUAL: {
            /** 
             * to make sure they have only one output pin for partial mapping phase 
             */
            resolve_logical_nodes(node, traverse_number, netlist);
            break;
        }
        case SL:  //fallthrough
        case SR:  //fallthrough
        case ASL: //fallthrough
        case ASR: {
            /**
             * resolving the shift nodes by making
             * the input port sizes the same
            */
            resolve_shift_nodes(node, traverse_number, netlist);
            break;
        }
        case CASE_EQUAL: //fallthorugh
        case CASE_NOT_EQUAL: {
            /**
             * resolving case equal and not equal nodes by
            */
            resolve_case_equal_nodes(node, traverse_number, netlist);
            break;
        }
        case ADD:      //fallthrough
        case MINUS:    //fallthorugh
        case MULTIPLY: //fallthrough
        case POWER:    //fallthrough
        case MODULO:   //fallthrough
        case DIVIDE: {
            /**
             * resolving the arithmetic node by
             */
            resolve_arithmetic_nodes(node, traverse_number, netlist);
            break;
        }
        case SETCLR: //fallthorugh
        case DLATCH: //fallthorugh
        case ADLATCH: {
            /**
             * resolving the latch nodes
             */
            resolve_latch_nodes(node, traverse_number, netlist);
            break;
        }
        case FF_NODE: //fallthrough
        case ADFF:    //fallthrough
        case SDFF:    //fallthrough
        case DFFE:    //fallthrough
        case ADFFE:   //fallthrough
        case SDFFE:   //fallthrough
        case SDFFCE:  //fallthrough
        case DFFSR:   //fallthrough
        case DFFSRE: {
            /**
             * resolving flip flop nodes 
            */
            resolve_ff_nodes(node, traverse_number, netlist);
            break;
        }
        case PMUX:            //fallthrough
        case MUX_2:           //fallthrough
        case MULTI_PORT_MUX:  //fallthrough
        case MULTI_BIT_MUX_2: //fallthorugh
        case MULTIPORT_nBIT_MUX: {
            /**
             * resolving multiplexer nodes which
            */
            resolve_mux_nodes(node, traverse_number, netlist);
            break;
        }
        case SPRAM: //fallthrough
        case DPRAM: //fallthrough
        case ROM:   //fallthrough
        case BRAM:  //fallthrough
        case MEMORY: {
            /**
             * resolving memory nodes based on the given architecture
            */
            resolve_memory_nodes(node, traverse_number, netlist);
            break;
        }
        case GND_NODE:     //fallthrough
        case VCC_NODE:     //fallthrough
        case PAD_NODE:     //fallthrough
        case INPUT_NODE:   //fallthrough
        case OUTPUT_NODE:  //fallthrough
        case BUF_NODE:     //fallthrough
        case BITWISE_NOT:  //fallthrough
        case BITWISE_AND:  //fallthrough
        case BITWISE_OR:   //fallthrough
        case BITWISE_NAND: //fallthrough
        case BITWISE_NOR:  //fallthrough
        case BITWISE_XNOR: //fallthrough
        case BITWISE_XOR: {
            /* some are already resolved for this phase */
            break;
        }
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
 * (function: resolve_logical_nodes)
 * 
 * @brief resolving the logical nodes by 
 * connecting the ouput pins[1..n] to GND
 * 
 * @param node pointing to a logical node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
static void resolve_logical_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes > 0);
    oassert(node->num_output_port_sizes == 1);

    int i, j;
    /* keep the recors of input widths */
    int width_a = node->input_port_sizes[0];
    int width_b = (node->num_input_port_sizes == 2) ? node->input_port_sizes[1] : -1;
    int max_width = std::max(width_a, width_b);

    /* making a new node with input port sizes equal to max_width and a single output pin */
    nnode_t* new_node = (node->num_input_port_sizes == 1)
                            ? make_1port_gate(node->type, max_width, 1, node, traverse_mark_number)
                            : make_2port_gate(node->type, max_width, max_width, 1, node, traverse_mark_number);

    for (i = 0; i < max_width; i++) {
        /* hook the first input into new node */
        if (i < width_a) {
            remap_pin_to_new_node(node->input_pins[i], new_node, i);
        } else {
            add_input_pin_to_node(new_node, get_zero_pin(netlist), i);
        }
        /* hook the second input into new node if exist */
        if (node->num_input_port_sizes == 2) {
            if (i < width_b) {
                remap_pin_to_new_node(node->input_pins[width_a + i], new_node, max_width + i);
            } else {
                add_input_pin_to_node(new_node, get_zero_pin(netlist), max_width + i);
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
 * (function: resolve_shift_nodes)
 * 
 * @brief resolving the shift nodes by making
 * the input port sizes the same
 * 
 * @param node pointing to a shift node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
static void resolve_shift_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);

    switch (node->type) {
        case SL:  //fallthrough
        case SR:  //fallthrough
        case ASL: //fallthrough
        case ASR: {
            /**
             * resolving the shift nodes by making
             * the input port sizes the same
            */
            equalize_shift_ports(node, traverse_mark_number, netlist);
            break;
        }
        default: {
            error_message(BLIF_ELBORATION, node->loc,
                          "The node(%s) type (%s) is not among Odin's latch types [SL, SR, ASL and ASR]\n", node->name, node->type);
            break;
        }
    }
}

/**
 *-------------------------------------------------------------------------------------------
 * (function: resolve_case_equal_nodes )
 * 
 * @brief resolving flip flop nodes by exloding them into RTL desing 
 * with set, reset, clear signals (synchronoush and asynchronous)
 * 
 * @param node pointing to the netlist node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 *-----------------------------------------------------------------------------------------*/
static void resolve_case_equal_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);

    switch (node->type) {
        case CASE_EQUAL: {
            /**
             * resolving case equal node using xor and and gates
            */
            resolve_case_equal_node(node, traverse_mark_number, netlist);
            break;
        }
        case CASE_NOT_EQUAL: {
            /**
             * resolving case not equal node by putting not gate 
             * after case equal output node
            */
            resolve_case_not_equal_node(node, traverse_mark_number, netlist);
            break;
        }
        default: {
            error_message(BLIF_ELBORATION, node->loc,
                          "The node(%s) type (%s) is not among Odin's latch types \n", node->name, node->type);
            break;
        }
    }
}

/**
 *-------------------------------------------------------------------------------------------
 * (function: resolve_arithmetic_nodes )
 * 
 * @brief check for missing ports such as carry-in/out in case of 
 * dealing with generated netlist from other blif files such Yosys.
 * 
 * @param node pointing to the netlist node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 *-----------------------------------------------------------------------------------------*/
static void resolve_arithmetic_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);

    switch (node->type) {
        case ADD: {
            /** 
             * check for missing ports such as carry-in/out in case of 
             * dealing with generated netlist from Yosys blif file.
             */
            if (hard_adders) {
                node = check_missing_ports(node, traverse_mark_number, netlist);
            }

            /* Adding to add_list for future checking on hard blocks */
            add_list = insert_in_vptr_list(add_list, node);
            break;
        }
        case MINUS: {
            /** 
             * check for missing ports such as carry-in/out in case of 
             * dealing with generated netlist from Yosys blif file.
             */
            if (hard_adders) {
                node = check_missing_ports(node, traverse_mark_number, netlist);
            }

            /* Adding to sub_list for future checking on hard blocks */
            sub_list = insert_in_vptr_list(sub_list, node);
            break;
        }
        case MULTIPLY: {
            /** 
             * check for constant multipication that should turn into RTL design here
             */
            if (!hard_multipliers)
                check_constant_multipication(node, traverse_mark_number, netlist);

            /* Adding to mult_list for future checking on hard blocks */
            mult_list = insert_in_vptr_list(mult_list, node);
            break;
        }
        case POWER: {
            /**
             * resolving the power node
             */
            resolve_power_node(node, traverse_mark_number, netlist);
            break;
        }
        case MODULO: {
            /**
             * resolving the modulo node
             */
            resolve_modulo_node(node, traverse_mark_number, netlist);
            break;
        }
        case DIVIDE: {
            /**
             * resolving the divide node
             */
            resolve_divide_node(node, traverse_mark_number, netlist);
            break;
        }
        default: {
            error_message(BLIF_ELBORATION, node->loc,
                          "The node(%s) type is not among Odin's arithmetic types [ADD, MINUS and MULTIPLY]\n", node->name);
            break;
        }
    }
}

/**
 *-------------------------------------------------------------------------------------------
 * (function: resolve_mux_nodes )
 * 
 * @brief resolving flip flop nodes by exloding them into RTL desing 
 * with set, reset, clear signals (synchronoush and asynchronous)
 * 
 * @param node pointing to the netlist node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 *-----------------------------------------------------------------------------------------*/
static void resolve_mux_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);

    switch (node->type) {
        case MULTIPORT_nBIT_MUX: {
            /**
             * need to reorder the input pins, so that the 
             * selector signal comes at the first place
             */
            make_selector_as_first_port(node);
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
            resolve_pmux_node(node, traverse_mark_number, netlist);
            break;
        }
        case MULTI_BIT_MUX_2: {
            /* postpone to partial mapping phase */
            break;
        }
        case MUX_2: //fallthruough
        case MULTI_PORT_MUX: {
            error_message(BLIF_ELBORATION, node->loc, "node (%s: %s) should have been converted to softer version.", node->type, node->name);
            break;
        }
        default: {
            error_message(BLIF_ELBORATION, node->loc,
                          "The node(%s) type (%s) is not among Odin's latch types [PMUX, MULTIPORT_nBIT_MUX, MULTI_BIT_MUX_2, MUX_2, MULTI_PORT_MUX]\n", node->name, node->type);
            break;
        }
    }
}

/**
 *-------------------------------------------------------------------------------------------
 * (function: resolve_latch_nodes )
 * 
 * @brief resolving the data latch or asynchrounoush data latch nodes
 * 
 * @param node pointing to the netlist node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 *-----------------------------------------------------------------------------------------*/
static void resolve_latch_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);

    switch (node->type) {
        case DLATCH: {
            /**
             * resolving the dlatch node
             */
            resolve_dlatch_node(node, traverse_mark_number, netlist);
            break;
        }
        case ADLATCH: {
            /**
             * resolving the adlatch node
             */
            resolve_adlatch_node(node, traverse_mark_number, netlist);
            break;
        }
        case SETCLR: {
            /**
             * resolving a sr node with set and reset
            */
            resolve_sr_node(node, traverse_mark_number, netlist);
            break;
        }
        default: {
            error_message(BLIF_ELBORATION, node->loc,
                          "The node(%s) type (%s) is not among Odin's latch types [dlatch, asynchronous-dlatch]\n", node->name, node->type);
            break;
        }
    }
}

/**
 *-------------------------------------------------------------------------------------------
 * (function: resolve_ff_nodes )
 * 
 * @brief resolving flip flop nodes by exloding them into RTL desing 
 * with set, reset, clear signals (synchronoush and asynchronous)
 * 
 * @param node pointing to the netlist node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 *-----------------------------------------------------------------------------------------*/
static void resolve_ff_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);

    switch (node->type) {
        case FF_NODE: {
            /**
             * resolving the dff node
             */
            resolve_dff_node(node, traverse_mark_number, netlist);
            break;
        }
        case ADFF: {
            /**
             * resolving a dff node with reset value
            */
            resolve_adff_node(node, traverse_mark_number, netlist);
            break;
        }
        case SDFF: {
            /**
             * resolving a dff node with reset value
            */
            resolve_sdff_node(node, traverse_mark_number, netlist);
            break;
        }
        case DFFE: {
            /**
             * resolving a dff node with enable
            */
            resolve_dffe_node(node, traverse_mark_number, netlist);
            break;
        }
        case ADFFE: {
            /**
             * resolving a dff node with enable and arst
            */
            resolve_adffe_node(node, traverse_mark_number, netlist);
            break;
        }
        case SDFFE: {
            /**
             * resolving a dff node with enable and srst
            */
            resolve_sdffe_node(node, traverse_mark_number, netlist);
            break;
        }
        case SDFFCE: {
            /**
             * resolving a dff node with srst and enable for both output and reset
            */
            resolve_sdffce_node(node, traverse_mark_number, netlist);
            break;
        }
        case DFFSR: {
            /**
             * resolving a dff node with set and reset
            */
            resolve_dffsr_node(node, traverse_mark_number, netlist);
            break;
        }
        case DFFSRE: {
            /**
             * resolving a dff node with set and reset and enable
            */
            resolve_dffsre_node(node, traverse_mark_number, netlist);
            break;
        }
        default: {
            error_message(BLIF_ELBORATION, node->loc,
                          "The node(%s) type (%s) is not among Odin's latch types [dff, adff, sdff, dffe, adffe, sdffe, dffsr, dffsre]\n", node->name, node->type);
            break;
        }
    }
}

/**
 *-------------------------------------------------------------------------------------------
 * (function: resolve_memory_nodes )
 * 
 * @brief resolving flip flop nodes by exloding them into RTL desing 
 * with set, reset, clear signals (synchronoush and asynchronous)
 * 
 * @param node pointing to the netlist node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 *-----------------------------------------------------------------------------------------*/
static void resolve_memory_nodes(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);

    switch (node->type) {
        case SPRAM: {
            /**
             * resolving a single port ram by create the related soft logic
            */
            resolve_single_port_ram(node, traverse_mark_number, netlist);
            break;
        }
        case DPRAM: {
            /**
             * resolving a single port ram by create the related soft logic
            */
            resolve_dual_port_ram(node, traverse_mark_number, netlist);
            break;
        }
        case ROM: {
            /**
             * resolving a read_only_memory node based on the given architecture
            */
            resolve_rom_node(node, traverse_mark_number, netlist);
            break;
        }
        case BRAM: {
            /**
             * resolving a block memory node based on the given architecture
            */
            resolve_bram_node(node, traverse_mark_number, netlist);
            break;
        }
        case MEMORY: {
            /* postpone to partial mapping phase */
            break;
        }
        default: {
            error_message(BLIF_ELBORATION, node->loc,
                          "The node(%s) type (%s) is not among Odin's latch types [SPRAM, DPRAM, ROM and BRAM(RW)]\n", node->name, node->type);
            break;
        }
    }
}

/*******************************************************************************************************
 ********************************************** [UTILS] ************************************************
 *******************************************************************************************************/
/**
 * (function: look_for_clocks)
 * 
 * @brief going through all FF nodes looking for the clock signals.
 * If they are not clock type, they should be altered to a clock node. 
 * Since BUF nodes be removed in the partial mapping, the driver of 
 * the BUF nodes should be considered as a clock node. 
 * 
 * @param netlist pointer to the current netlist file
 */
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
