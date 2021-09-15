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
 * @file: this file includes the circuitry implementation of case_equal 
 * and case_not_equal high-level netlist nodes. The implementations 
 * are XNOR, AND and invertor gates based. 
 */

#include "CaseEqual.hpp"
#include "Division.hpp"
#include "node_creation_library.h"
#include "odin_util.h"
#include "netlist_utils.h"
#include "vtr_memory.h"

/**
 * (function: resolve_case_equal_node)
 * 
 * @brief resolving the CASE EQUAL node using XNOR 
 * for each pin and finally AND all XNOR outputs
 * 
 * @param node pointing to a netlist node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
nnode_t* resolve_case_equal_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
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
 * @param node pointing to a netlist node 
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
nnode_t* resolve_case_not_equal_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
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
