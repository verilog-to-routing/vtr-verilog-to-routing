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
#ifndef _PARMYS_UPDATE_HPP_
#define _PARMYS_UPDATE_HPP_

#include "odin_types.h"

USING_YOSYS_NAMESPACE

#define DEFAULT_CLOCK_NAME "GLOBAL_SIM_BASE_CLK"

void define_logical_function_yosys(nnode_t *node, Module *module);
void update_design(Design *design, netlist_t *netlist);
void define_MUX_function_yosys(nnode_t *node, Module *module);
void define_SMUX_function_yosys(nnode_t *node, Module *module);
void define_FF_yosys(nnode_t *node, Module *module);

#endif //_PARMYS_UPDATE_HPP_