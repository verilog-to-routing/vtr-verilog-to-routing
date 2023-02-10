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
#ifndef _NETLIST_VISUALIZER_H_
#define _NETLIST_VISUALIZER_H_

#include "odin_types.h"
#include <string>

void graphVizOutputNetlist(std::string path, const char *name, uintptr_t marker_value, netlist_t *input_netlist);
void graphVizOutputCombinationalNet(std::string path, const char *name, uintptr_t marker_value, nnode_t *current_node);

#endif // _NETLIST_VISUALIZER_H_
