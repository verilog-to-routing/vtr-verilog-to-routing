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

#include <cstring>

#include "odin_types.h"
#include "odin_globals.h"
#include "netlist_check.h"
#include "netlist_visualizer.h"

#include "vtr_memory.h"

nnode_t* find_node_at_top_of_combo_loop(nnode_t* start_node);
void depth_first_traverse_check_if_forward_leveled(nnode_t* node, uintptr_t traverse_mark_number);

void depth_first_traverse_until_next_ff_or_output(nnode_t* node,
                                                  nnode_t* calling_node,
                                                  uintptr_t traverse_mark_number,
                                                  int seq_level,
                                                  netlist_t* netlist);

/*---------------------------------------------------------------------------------------------
 * (function: check_netlist)
 * Note: netlist passed in needs to be initialized by allocate_netlist() to make sure correctly initialized.
 *-------------------------------------------------------------------------------------------*/
void check_netlist(netlist_t* netlist) {
    /* create a graph output of this netlist */
    if (configuration.output_netlist_graphs) {
        /* Path is where we are */
        graphVizOutputNetlist(configuration.debug_output_path, "net", 1, netlist);
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traverse_until_next_ff_or_output)
 *-------------------------------------------------------------------------------------------*/
void depth_first_traverse_until_next_ff_or_output(nnode_t* node,
                                                  nnode_t* calling_node,
                                                  uintptr_t traverse_mark_number,
                                                  int seq_level,
                                                  netlist_t* netlist) {
    int i, j;
    nnode_t* next_node;
    nnet_t* next_net;

    /* first, check if the clalling node should be recorderd */
    if ((calling_node != NULL) && ((node->type == FF_NODE) || (node->type == OUTPUT_NODE))) {
        /* IF - the this node is the end of a sequential level then the node before needs to be stored */
        if (calling_node->sequential_terminator == false) {
            /* IF - it hasn't been stored before */
            netlist->num_at_sequential_level_combinational_termination_node
                [netlist->num_sequential_level_combinational_termination_nodes - 1]++;
            netlist->sequential_level_combinational_termination_node
                [netlist->num_sequential_level_combinational_termination_nodes - 1]
                = (nnode_t**)vtr::realloc(
                    netlist->sequential_level_combinational_termination_node
                        [netlist->num_sequential_level_combinational_termination_nodes - 1],
                    sizeof(nnode_t*)
                        * netlist->num_at_sequential_level_combinational_termination_node
                              [netlist->num_sequential_level_combinational_termination_nodes - 1]);
            netlist->sequential_level_combinational_termination_node
                [netlist->num_sequential_level_combinational_termination_nodes - 1]
                [netlist->num_at_sequential_level_combinational_termination_node
                     [netlist->num_sequential_level_combinational_termination_nodes - 1]
                 - 1]
                = calling_node;
            /* mark the node locally */
            calling_node->sequential_terminator = true;
        }
    }

    if (node->traverse_visited == traverse_mark_number) {
        /* if already visited then nothing to do */
        return;
    } else if (node->type == CLOCK_NODE) {
        /* since this is a node that touches all flip flops, don't analyze for sequential level */
        return;
    } else if (node->type == FF_NODE) {
        /* ELSE IF - this is a ff_node, so add it to the list for the next sequential level */
        /* mark as traversed */
        node->traverse_visited = traverse_mark_number;
        node->sequential_level = seq_level + 1;

        /* add to the next sequntial list */
        netlist->num_at_sequential_level[seq_level + 1]++;
        netlist->sequential_level_nodes[seq_level + 1]
            = (nnode_t**)vtr::realloc(netlist->sequential_level_nodes[seq_level + 1],
                                      sizeof(nnode_t*) * netlist->num_at_sequential_level[seq_level + 1]);
        netlist->sequential_level_nodes[seq_level + 1][netlist->num_at_sequential_level[seq_level + 1] - 1] = node;

        return;
    } else {
        /* ELSE - this is a node so depth visit it */

        /* mark that we have visitied this node now */
        node->traverse_visited = traverse_mark_number;
        node->sequential_level = seq_level;

        for (i = 0; i < node->num_output_pins; i++) {
            if (node->output_pins[i]->net == NULL) continue;

            next_net = node->output_pins[i]->net;
            for (j = 0; j < next_net->num_fanout_pins; j++) {
                if (next_net->fanout_pins[j] == NULL) continue;

                next_node = next_net->fanout_pins[j]->node;
                if (next_node == NULL) continue;

                /* recursive call point */
                depth_first_traverse_until_next_ff_or_output(next_node, node, traverse_mark_number, seq_level, netlist);
            }
        }
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traverse)
 *-------------------------------------------------------------------------------------------*/
void depth_first_traverse_check_if_forward_leveled(nnode_t* node, uintptr_t traverse_mark_number) {
    int i, j;
    nnode_t* next_node;
    nnet_t* next_net;

    if (node->traverse_visited == traverse_mark_number) {
        return;
    } else {
        /* ELSE - this is a new node so depth visit it */

        /* mark that we have visitied this node now */
        node->traverse_visited = traverse_mark_number;

        for (i = 0; i < node->num_output_pins; i++) {
            if (node->output_pins[i]->net == NULL) continue;

            next_net = node->output_pins[i]->net;
            for (j = 0; j < next_net->num_fanout_pins; j++) {
                if (next_net->fanout_pins[j] == NULL) continue;

                next_node = next_net->fanout_pins[j]->node;
                if (next_node == NULL) continue;

                if ((next_node->forward_level == -1) && (next_node->type != FF_NODE)) {
                    graphVizOutputCombinationalNet(configuration.debug_output_path, "combo_loop", COMBO_LOOP_ERROR,
                                                   /*next_node);*/ find_node_at_top_of_combo_loop(next_node));
                    oassert(false);
                }

                /* recursive call point */
                depth_first_traverse_check_if_forward_leveled(next_node, traverse_mark_number);
            }
        }
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: find_node_at_top_of_combo_loop)
 *-------------------------------------------------------------------------------------------*/
nnode_t* find_node_at_top_of_combo_loop(nnode_t* start_node) {
    int stack_size = 1;
    nnode_t** stack = (nnode_t**)vtr::calloc(stack_size, sizeof(nnode_t*));
    stack[0] = start_node;

    while (true) {
        nnode_t* next_node = stack[--stack_size];
        oassert(next_node->unique_node_data_id == LEVELIZE);
        int* fanouts_visited = (int*)next_node->node_data;
        next_node->node_data = NULL;

        /* check if they've all been marked */
        bool all_visited = true;
        int idx_missed = -1;
        for (int i = 0; i < next_node->num_input_pins; i++) {
            if (fanouts_visited[i] == -1) {
                all_visited = false;
                idx_missed = i;
                break;
            }
        }

        if (!all_visited) {
            for (int i = 0; i < next_node->input_pins[idx_missed]->net->num_driver_pins; i++) {
                if (next_node->input_pins[idx_missed]->net->driver_pins[i]->node->backward_level
                    < next_node->backward_level) {
                    /* IF - the next node has a lower backward level than this node suggests that it is
                     * closer to primary outputs and not in the combo loop */
                    vtr::free(stack);
                    return next_node;
                }

                stack_size++;
                stack = (nnode_t**)vtr::realloc(stack, sizeof(nnode_t*) * stack_size);
                stack[stack_size - 1] = next_node->input_pins[idx_missed]->net->driver_pins[i]->node;
            }
        } else {
            vtr::free(stack);
            return next_node;
        }
    }
}
