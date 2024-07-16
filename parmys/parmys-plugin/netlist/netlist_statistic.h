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
#ifndef _NETLIST_STATISTIC_H_
#define _NETLIST_STATISTIC_H_

#include "netlist_utils.h"

static const unsigned int traversal_id = 0;
static const uintptr_t mult_optimization_traverse_value = (uintptr_t)&traversal_id;

stat_t *get_stats(nnode_t *node, netlist_t *netlist, uintptr_t traverse_mark_number);

void init_stat(netlist_t *netlist);
void compute_statistics(netlist_t *netlist, bool display);

/**
 * @brief This function will calculate and assign weights related
 * to mixing hard and soft logic implementation for certain kind
 * of logic blocks
 * @param node
 * The node that needs its weight to be assigned
 * @param netlist
 * netlist, has to be passed to the counting functions
 */
void mixing_optimization_stats(nnode_t *node, netlist_t *netlist);

#endif // _NETLIST_STATISTIC_H_
