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
#ifndef _HARD_BLOCK_H_
#define _HARD_BLOCK_H_

#include "odin_types.h"

extern STRING_CACHE *hard_block_names;

void register_hard_blocks();
t_model *find_hard_block(const char *name);
void cell_hard_block(nnode_t *node, Yosys::Module *module, netlist_t *netlist, Yosys::Design *design);
void output_hard_blocks_yosys(Yosys::Design *design);
void instantiate_hard_block(nnode_t *node, short mark, netlist_t *netlist);
t_model_ports *get_model_port(t_model_ports *ports, const char *name);

#endif // _HARD_BLOCK_H_
