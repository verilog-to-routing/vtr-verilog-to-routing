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
#include "odin_globals.h"
#include "odin_types.h"

#include <string.h>

#include "netlist_utils.h"

#include "adder.h"
#include "block_memory.h"
#include "memory.h"
#include "multiplier.h"
#include "parmys_resolve.hpp"
#include "subtractor.h"

#include "vtr_util.h"

USING_YOSYS_NAMESPACE

void dfs_resolve(nnode_t *node, uintptr_t traverse_mark_number, netlist_t *netlist);

void resolve_node(nnode_t *node, short traverse_mark_number, netlist_t *netlist);

static void resolve_arithmetic_nodes(nnode_t *node, uintptr_t traverse_mark_number, netlist_t *netlist);

static void resolve_memory_nodes(nnode_t *node, uintptr_t traverse_mark_number, netlist_t *netlist);

static void look_for_clocks(netlist_t *netlist);

void resolve_top(netlist_t *netlist)
{

    if (configuration.coarsen) {
        init_block_memory_index();

        for (int i = 0; i < netlist->num_top_input_nodes; i++) {
            if (netlist->top_input_nodes[i] != NULL) {
                dfs_resolve(netlist->top_input_nodes[i], RESOLVE_DFS_VALUE, netlist);
            }
        }

        dfs_resolve(netlist->gnd_node, RESOLVE_DFS_VALUE, netlist);
        dfs_resolve(netlist->vcc_node, RESOLVE_DFS_VALUE, netlist);
        dfs_resolve(netlist->pad_node, RESOLVE_DFS_VALUE, netlist);

        look_for_clocks(netlist);

        configuration.coarsen = false;
    }
}

void dfs_resolve(nnode_t *node, uintptr_t traverse_mark_number, netlist_t *netlist)
{
    if (node->traverse_visited != traverse_mark_number) {

        node->traverse_visited = traverse_mark_number;

        for (int i = 0; i < node->num_output_pins; i++) {
            if (node->output_pins[i]->net) {
                nnet_t *next_net = node->output_pins[i]->net;
                if (next_net->fanout_pins) {
                    for (int j = 0; j < next_net->num_fanout_pins; j++) {
                        if (next_net->fanout_pins[j]) {
                            if (next_net->fanout_pins[j]->node) {
                                dfs_resolve(next_net->fanout_pins[j]->node, traverse_mark_number, netlist);
                            }
                        }
                    }
                }
            }
        }

        resolve_node(node, traverse_mark_number, netlist);
    }
}

void resolve_node(nnode_t *node, short traverse_number, netlist_t *netlist)
{
    switch (node->type) {
    case ADD:
    case MINUS:
    case MULTIPLY: {
        resolve_arithmetic_nodes(node, traverse_number, netlist);
        break;
    }
    case SPRAM:
    case DPRAM:
    case ROM:
    case BRAM:
    case YMEM:
    case YMEM2:
    case MEMORY: {
        resolve_memory_nodes(node, traverse_number, netlist);
        break;
    }
    case GND_NODE:
    case VCC_NODE:
    case PAD_NODE:
    case INPUT_NODE:
    case OUTPUT_NODE:
    case HARD_IP:
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
    case SKIP:
        break;
    case ADDER_FUNC:
    case CARRY_FUNC:
    case CLOCK_NODE:
    case GENERIC:
    default:
        error_message(RESOLVE, node->loc, "node (%s: %s) should have been converted to softer version.", node->type, node->name);
        break;
    }
}

static void resolve_arithmetic_nodes(nnode_t *node, uintptr_t traverse_mark_number, netlist_t *netlist)
{
    oassert(node->traverse_visited == traverse_mark_number);

    switch (node->type) {
    case ADD: {
        if (hard_adders) {
            node = check_missing_ports(node, traverse_mark_number, netlist);
        }

        add_list = insert_in_vptr_list(add_list, node);
        break;
    }
    case MINUS: {
        equalize_ports_size(node, traverse_mark_number, netlist);

        sub_list = insert_in_vptr_list(sub_list, node);
        break;
    }
    case MULTIPLY: {
        if (!hard_multipliers)
            check_constant_multipication(node, traverse_mark_number, netlist);
        else
            check_multiplier_port_size(node);

        mult_list = insert_in_vptr_list(mult_list, node);
        break;
    }
    default: {
        error_message(RESOLVE, node->loc, "The node(%s) type is not among Odin's arithmetic types [ADD, MINUS and MULTIPLY]\n", node->name);
        break;
    }
    }
}

static void resolve_memory_nodes(nnode_t *node, uintptr_t traverse_mark_number, netlist_t *netlist)
{
    oassert(node->traverse_visited == traverse_mark_number);

    switch (node->type) {
    case SPRAM: {
        resolve_single_port_ram(node, traverse_mark_number, netlist);
        break;
    }
    case DPRAM: {
        resolve_dual_port_ram(node, traverse_mark_number, netlist);
        break;
    }
    case YMEM: {
        resolve_ymem_node(node, traverse_mark_number, netlist);
        break;
    }
    case YMEM2: {
        resolve_ymem2_node(node, traverse_mark_number, netlist);
        break;
    }
    case MEMORY: {
        break;
    }
    default: {
        error_message(RESOLVE, node->loc, "The node(%s) type (%s) is not among Odin's latch types [SPRAM, DPRAM, ROM and BRAM(RW)]\n", node->name,
                      node->type);
        break;
    }
    }
}

static void look_for_clocks(netlist_t *netlist)
{
    for (int i = 0; i < netlist->num_top_input_nodes; i++) {
        nnode_t *input_node = netlist->top_input_nodes[i];
        if (!strcmp(input_node->name, DEFAULT_CLOCK_NAME))
            input_node->type = CLOCK_NODE;
    }

    for (int i = 0; i < netlist->num_ff_nodes; i++) {
        oassert(netlist->ff_nodes[i]->input_pins[1]->net->num_driver_pins == 1);
        nnode_t *node = netlist->ff_nodes[i]->input_pins[1]->net->driver_pins[0]->node;

        while (node->type == BUF_NODE)
            node = node->input_pins[0]->net->driver_pins[0]->node;

        if (node->type != CLOCK_NODE) {
            node->type = CLOCK_NODE;
        }
    }
}
