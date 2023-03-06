/*
 * Copyright 2022 CAS—Atlantic (University of New Brunswick, CASA)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "netlist_utils.h"
#include "netlist_visualizer.h"
#include "odin_globals.h"
#include "odin_types.h"
#include "odin_util.h"
#include "vtr_memory.h"
#include "vtr_util.h"

void depth_first_traverse_visualize(nnode_t *node, FILE *fp, uintptr_t traverse_mark_number);
void depth_first_traversal_graph_display(FILE *out, uintptr_t marker_value, netlist_t *netllist);

void forward_traversal_net_graph_display(FILE *out, uintptr_t marker_value, nnode_t *node);
void backward_traversal_net_graph_display(FILE *out, uintptr_t marker_value, nnode_t *node);

/*---------------------------------------------------------------------------------------------
 * (function: graphVizOutputNetlist)
 *-------------------------------------------------------------------------------------------*/
void graphVizOutputNetlist(std::string path, const char *name, uintptr_t marker_value, netlist_t *netlist)
{
    char path_and_file[4096];
    FILE *fp;

    /* open the file */
    odin_sprintf(path_and_file, "%s/%s.dot", path.c_str(), name);
    fp = fopen(path_and_file, "w");

    /* open graph */
    fprintf(fp, "digraph G {\n\tranksep=.25;\n");

    depth_first_traversal_graph_display(fp, marker_value, netlist);

    /* close graph */
    fprintf(fp, "}\n");
    fclose(fp);
}

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traversal_start()
 *-------------------------------------------------------------------------------------------*/
void depth_first_traversal_graph_display(FILE *out, uintptr_t marker_value, netlist_t *netlist)
{
    /* start with the primary input list */
    for (int i = 0; i < netlist->num_top_input_nodes; i++) {
        if (netlist->top_input_nodes[i] != NULL) {
            depth_first_traverse_visualize(netlist->top_input_nodes[i], out, marker_value);
        }
    }
    /* now traverse the ground and vcc pins */
    if (netlist->gnd_node != NULL)
        depth_first_traverse_visualize(netlist->gnd_node, out, marker_value);
    if (netlist->vcc_node != NULL)
        depth_first_traverse_visualize(netlist->vcc_node, out, marker_value);
}

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traverse)
 *-------------------------------------------------------------------------------------------*/
void depth_first_traverse_visualize(nnode_t *node, FILE *fp, uintptr_t traverse_mark_number)
{
    nnode_t *next_node;
    nnet_t *next_net;

    if (node->traverse_visited == traverse_mark_number) {
        return;
    } else {
        /* ELSE - this is a new node so depth visit it */
        char *temp_string;
        char *temp_string2;

        node->traverse_visited = traverse_mark_number;

        temp_string = vtr::strdup(make_simple_name(node->name, "^-+.", '_').c_str());
        if ((node->type == FF_NODE) || (node->type == BUF_NODE)) {
            fprintf(fp, "\t\"%s\" [shape=box];\n", temp_string);
        } else if (node->type == INPUT_NODE) {
            fprintf(fp, "\t\"%s\" [shape=triangle];\n", temp_string);
        } else if (node->type == CLOCK_NODE) {
            fprintf(fp, "\t\"%s\" [shape=triangle];\n", temp_string);
        } else if (node->type == OUTPUT_NODE) {
            fprintf(fp, "\t\"%s_O\" [shape=triangle];\n", temp_string);
        } else {
            fprintf(fp, "\t\"%s\"\n", temp_string);
        }
        vtr::free(temp_string);

        for (int i = 0; i < node->num_output_pins; i++) {
            if (node->output_pins[i]->net == NULL)
                continue;

            next_net = node->output_pins[i]->net;
            for (int j = 0; j < next_net->num_fanout_pins; j++) {
                npin_t *pin = next_net->fanout_pins[j];
                if (pin) {
                    next_node = pin->node;
                    if (next_node == NULL)
                        continue;
                    // To see just combinational stuff...also comment above triangels and box
                    //				if ((next_node->type == FF_NODE) || (next_node->type == INPUT_NODE) || (next_node->type == OUTPUT_NODE))
                    //					continue;
                    //				if ((node->type == FF_NODE) || (node->type == INPUT_NODE) || (node->type == OUTPUT_NODE))
                    //					continue;

                    temp_string = vtr::strdup(make_simple_name(node->name, "^-+.", '_').c_str());
                    temp_string2 = vtr::strdup(make_simple_name(next_node->name, "^-+.", '_').c_str());
                    /* renaming for output nodes */
                    if (node->type == OUTPUT_NODE) {
                        /* renaming for output nodes */
                        char *temp_string_old = temp_string;
                        temp_string = (char *)vtr::malloc(sizeof(char) * strlen(temp_string) + 1 + 2);
                        odin_sprintf(temp_string, "%s_O", temp_string_old);
                        free(temp_string_old);
                    }
                    if (next_node->type == OUTPUT_NODE) {
                        /* renaming for output nodes */
                        char *temp_string2_old = temp_string2;
                        temp_string2 = (char *)vtr::malloc(sizeof(char) * strlen(temp_string2) + 1 + 2);
                        odin_sprintf(temp_string2, "%s_O", temp_string2_old);
                        free(temp_string2_old);
                    }

                    fprintf(fp, "\t\"%s\" -> \"%s\"", temp_string, temp_string2);
                    if (next_net->fanout_pins[j]->name)
                        fprintf(fp, "[label=\"%s\"]", next_net->fanout_pins[j]->name);
                    fprintf(fp, ";\n");

                    vtr::free(temp_string);
                    vtr::free(temp_string2);

                    depth_first_traverse_visualize(next_node, fp, traverse_mark_number);
                }
            }
        }
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: graphVizOutputCobinationalNet)
 *-------------------------------------------------------------------------------------------*/
void graphVizOutputCombinationalNet(std::string path, const char *name, uintptr_t marker_value, nnode_t *current_node)
{
    char path_and_file[4096];
    FILE *fp;

    /* open the file */
    odin_sprintf(path_and_file, "%s/%s.dot", path.c_str(), name);
    fp = fopen(path_and_file, "w");

    /* open graph */
    fprintf(fp, "digraph G {\n\tranksep=.25;\n");

    forward_traversal_net_graph_display(fp, marker_value, current_node);
    backward_traversal_net_graph_display(fp, marker_value, current_node);

    /* close graph */
    fprintf(fp, "}\n");
    fclose(fp);
}

/*---------------------------------------------------------------------------------------------
 * (function: forward_traversal_net_graph_display()
 *	TODO check if stack of node is freed
 *-------------------------------------------------------------------------------------------*/
void forward_traversal_net_graph_display(FILE *fp, uintptr_t marker_value, nnode_t *node)
{
    nnode_t **stack_of_nodes;
    int index_in_stack = 0;
    int num_stack_of_nodes = 1;
    char *temp_string;
    char *temp_string2;

    stack_of_nodes = (nnode_t **)vtr::malloc(sizeof(nnode_t *) * 1);
    stack_of_nodes[0] = node;

    while (index_in_stack != num_stack_of_nodes) {
        nnode_t *current_node = stack_of_nodes[index_in_stack];

        /* mark it */
        current_node->traverse_visited = marker_value;

        /* printout the details of it */
        temp_string = vtr::strdup(make_simple_name(current_node->name, "^-+.", '_').c_str());
        if (index_in_stack == 0) {
            fprintf(fp, "\t%s [shape=box,color=red];\n", temp_string);
        } else if ((current_node->type == FF_NODE) || (current_node->type == BUF_NODE)) {
            fprintf(fp, "\t%s [shape=box];\n", temp_string);
        } else if (current_node->type == INPUT_NODE) {
            fprintf(fp, "\t%s [shape=triangle];\n", temp_string);
        } else if (current_node->type == CLOCK_NODE) {
            fprintf(fp, "\t%s [shape=triangle];\n", temp_string);
        } else if (current_node->type == OUTPUT_NODE) {
            fprintf(fp, "\t%s_O [shape=triangle];\n", temp_string);
        } else {
            fprintf(fp, "\t%s [label=\"%d:%d\"];\n", temp_string, current_node->forward_level, current_node->backward_level);
        }
        vtr::free(temp_string);

        /* at each node visit all the outputs */
        for (int j = 0; j < current_node->num_output_pins; j++) {
            if (current_node->output_pins[j] == NULL)
                continue;

            for (int k = 0; k < current_node->output_pins[j]->net->num_fanout_pins; k++) {
                if ((current_node->output_pins[j] == NULL) || (current_node->output_pins[j]->net == NULL) ||
                    (current_node->output_pins[j]->net->fanout_pins[k] == NULL))
                    continue;

                /* visit the fanout point */
                nnode_t *next_node = current_node->output_pins[j]->net->fanout_pins[k]->node;

                if (next_node == NULL)
                    continue;

                temp_string = vtr::strdup(make_simple_name(current_node->name, "^-+.", '_').c_str());
                temp_string2 = vtr::strdup(make_simple_name(next_node->name, "^-+.", '_').c_str());
                if (current_node->type == OUTPUT_NODE) {
                    /* renaming for output nodes */
                    temp_string = (char *)vtr::realloc(temp_string, sizeof(char) * strlen(temp_string) + 1 + 2);
                    odin_sprintf(temp_string, "%s_O", temp_string);
                }
                if (next_node->type == OUTPUT_NODE) {
                    /* renaming for output nodes */
                    temp_string2 = (char *)vtr::realloc(temp_string2, sizeof(char) * strlen(temp_string2) + 1 + 2);
                    odin_sprintf(temp_string2, "%s_O", temp_string2);
                }

                fprintf(fp, "\t%s -> %s [label=\"%s\"];\n", temp_string, temp_string2, current_node->output_pins[j]->net->fanout_pins[k]->name);

                vtr::free(temp_string);
                vtr::free(temp_string2);

                if ((next_node->traverse_visited != marker_value) && (next_node->type != FF_NODE)) {
                    /* IF - not visited yet then add to list */
                    stack_of_nodes = (nnode_t **)vtr::realloc(stack_of_nodes, sizeof(nnode_t *) * (num_stack_of_nodes + 1));
                    stack_of_nodes[num_stack_of_nodes] = next_node;
                    num_stack_of_nodes++;
                }
            }
        }

        /* process next element in net */
        index_in_stack++;
    }

    for (int i = 0; i < num_stack_of_nodes; i++) {
        free_nnode(stack_of_nodes[i]);
    }
    vtr::free(stack_of_nodes);
}

/*---------------------------------------------------------------------------------------------
 * (function: backward_traversal_net_graph_display()
 *-------------------------------------------------------------------------------------------*/
void backward_traversal_net_graph_display(FILE *fp, uintptr_t marker_value, nnode_t *node)
{
    char *temp_string;
    char *temp_string2;
    nnode_t **stack_of_nodes;
    int index_in_stack = 0;
    int num_stack_of_nodes = 1;

    stack_of_nodes = (nnode_t **)vtr::malloc(sizeof(nnode_t *) * 1);
    stack_of_nodes[0] = node;

    while (index_in_stack != num_stack_of_nodes) {
        nnode_t *current_node = stack_of_nodes[index_in_stack];

        /* mark it */
        current_node->traverse_visited = marker_value;

        /* printout the details of it */
        temp_string = vtr::strdup(make_simple_name(current_node->name, "^-+.", '_').c_str());
        if (index_in_stack != 0) {
            if ((current_node->type == FF_NODE) || (current_node->type == BUF_NODE)) {
                fprintf(fp, "\t%s [shape=box];\n", temp_string);
            } else if (current_node->type == INPUT_NODE) {
                fprintf(fp, "\t%s [shape=triangle];\n", temp_string);
            } else if (current_node->type == CLOCK_NODE) {
                fprintf(fp, "\t%s [shape=triangle];\n", temp_string);
            } else if (current_node->type == OUTPUT_NODE) {
                fprintf(fp, "\t%s_O [shape=triangle];\n", temp_string);
            } else {
                fprintf(fp, "\t%s [label=\"%d:%d\"];\n", temp_string, current_node->forward_level, current_node->backward_level);
            }
        }
        vtr::free(temp_string);

        /* at each node visit all the outputs */
        for (int j = 0; j < current_node->num_input_pins; j++) {
            if (current_node->input_pins[j] == NULL)
                continue;

            if ((current_node->input_pins[j] == NULL) || (current_node->input_pins[j]->net == NULL) ||
                (current_node->input_pins[j]->net->num_driver_pins == 0))
                continue;

            /* visit the fanout point */

            for (int k = 0; k < current_node->input_pins[j]->net->num_driver_pins; k++) {
                nnode_t *next_node = current_node->input_pins[j]->net->driver_pins[k]->node;
                if (next_node == NULL)
                    continue;

                temp_string = vtr::strdup(make_simple_name(current_node->name, "^-+.", '_').c_str());
                temp_string2 = vtr::strdup(make_simple_name(next_node->name, "^-+.", '_').c_str());

                fprintf(fp, "\t%s -> %s [label=\"%s\"];\n", temp_string2, temp_string, current_node->input_pins[j]->name);

                vtr::free(temp_string);
                vtr::free(temp_string2);

                if ((next_node->traverse_visited != marker_value) && (next_node->type != FF_NODE)) {
                    /* IF - not visited yet then add to list */
                    stack_of_nodes = (nnode_t **)vtr::realloc(stack_of_nodes, sizeof(nnode_t *) * (num_stack_of_nodes + 1));
                    stack_of_nodes[num_stack_of_nodes] = next_node;
                    num_stack_of_nodes++;
                }
            }
        }

        /* process next element in net */
        index_in_stack++;
    }

    for (int i = 0; i < num_stack_of_nodes; i++) {
        free_nnode(stack_of_nodes[i]);
    }
    vtr::free(stack_of_nodes);
}
