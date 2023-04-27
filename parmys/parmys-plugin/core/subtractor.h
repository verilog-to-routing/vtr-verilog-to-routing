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
#ifndef _SUBTRACTOR_H_
#define _SUBTRACTOR_H_

#include "adder.h"
#include "read_xml_arch_file.h"

extern vtr::t_linked_vptr *sub_list;
extern vtr::t_linked_vptr *sub_chain_list;

extern void report_sub_distribution();
extern void declare_hard_adder_for_sub(nnode_t *node);
extern void instantiate_hard_adder_subtraction(nnode_t *node, short mark, netlist_t *netlist);
extern void split_adder_for_sub(nnode_t *node, int a, int b, int sizea, int sizeb, int cin, int cout, int count, netlist_t *netlist);
extern void iterate_adders_for_sub(netlist_t *netlist);
extern void instantiate_sub_w_borrow_block(nnode_t *node, short traverse_mark_number, netlist_t *netlist);
extern void clean_adders_for_sub();

#endif // _SUBTRACTOR_H_
