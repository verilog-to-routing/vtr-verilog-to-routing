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
#ifndef _ADDER_H_
#define _ADDER_H_

#include "odin_types.h"
#include "read_xml_arch_file.h"
#include <vector>

struct t_adder {
    int size_a;
    int size_b;
    int size_cin;
    int size_sumout;
    int size_cout;
    struct t_adder *next;
};

extern t_model *hard_adders;
extern vtr::t_linked_vptr *add_list;
extern vtr::t_linked_vptr *chain_list;
extern vtr::t_linked_vptr *processed_adder_list;
extern int total;
extern int min_add;
extern int min_threshold_adder;

void init_add_distribution();
void report_add_distribution();
void declare_hard_adder(nnode_t *node);
void instantiate_hard_adder(nnode_t *node, short mark, netlist_t *netlist);
void find_hard_adders();
void add_the_blackbox_for_adds_yosys(Yosys::Design *design);
void define_add_function_yosys(nnode_t *node, Yosys::Module *module, Yosys::Design *design);
void split_adder(nnode_t *node, int a, int b, int sizea, int sizeb, int cin, int cout, int count, netlist_t *netlist);
void iterate_adders(netlist_t *netlist);
void clean_adders();
void reduce_operations(netlist_t *netlist, operation_list op);
void traverse_list(operation_list oper, vtr::t_linked_vptr *place);
void match_node(vtr::t_linked_vptr *place, operation_list oper);
int match_ports(nnode_t *node, nnode_t *next_node, operation_list oper);
void traverse_operation_node(ast_node_t *node, char *component[], operation_list op, int *mark);
void merge_nodes(nnode_t *node, nnode_t *next_node);
void remove_list_node(vtr::t_linked_vptr *node, vtr::t_linked_vptr *place);
void remove_fanout_pins(nnode_t *node);
void reallocate_pins(nnode_t *node, nnode_t *next_node);
void free_op_nodes(nnode_t *node);
int match_pins(nnode_t *node, nnode_t *next_node);

void instantiate_add_w_carry_block(int *width, nnode_t *node, short mark, netlist_t *netlist, short subtraction);
nnode_t *check_missing_ports(nnode_t *node, uintptr_t traverse_mark_number, netlist_t *netlist);

#endif // _ADDER_H_
