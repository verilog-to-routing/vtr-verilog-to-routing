/*
 * Copyright 2023 CAS—Atlantic (University of New Brunswick, CASA)
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
 */

#include "multiplexer.h"
#include "node_creation_library.h"
#include "netlist_utils.h"

#include "vtr_memory.h"

/**
 * (function: transform_to_single_bit_mux_nodes)
 * 
 * @brief split the mux node to
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
