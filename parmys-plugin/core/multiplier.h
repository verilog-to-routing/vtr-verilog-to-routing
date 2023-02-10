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
#ifndef _MULTIPLIER_H_
#define _MULTIPLIER_H_

#include "odin_types.h"
#include "read_xml_arch_file.h"

struct t_multiplier {
    int size_a;
    int size_b;
    int size_out;
    struct t_multiplier *next;
};

enum class mult_port_stat_e {
    NOT_CONSTANT,         // neither of ports are constant
    MULTIPLIER_CONSTANT,  // first input port is constant
    MULTIPICAND_CONSTANT, // second input port is constant
    CONSTANT,             // both input ports are constant
    mult_port_stat_END
};

extern t_model *hard_multipliers;
extern vtr::t_linked_vptr *mult_list;
extern int min_mult;

extern void init_mult_distribution();
extern void report_mult_distribution();
extern void declare_hard_multiplier(nnode_t *node);
extern void instantiate_hard_multiplier(nnode_t *node, short mark, netlist_t *netlist);
extern void instantiate_simple_soft_multiplier(nnode_t *node, short mark, netlist_t *netlist);
extern void connect_constant_mult_outputs(nnode_t *node, signal_list_t *output_signal_list);
extern void find_hard_multipliers();
extern void add_the_blackbox_for_mults_yosys(Yosys::Design *design);
extern void define_mult_function_yosys(nnode_t *node, Yosys::Module *module, Yosys::Design *design);
extern void split_multiplier(nnode_t *node, int a0, int b0, int a1, int b1, netlist_t *netlist);
extern void iterate_multipliers(netlist_t *netlist);
extern bool check_constant_multipication(nnode_t *node, uintptr_t traverse_mark_number, netlist_t *netlist);
extern void check_multiplier_port_size(nnode_t *node);
extern void clean_multipliers();
extern void free_multipliers();

#endif // _MULTIPLIER_H_
