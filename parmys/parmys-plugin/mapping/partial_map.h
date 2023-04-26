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
#ifndef _PARTIAL_MAP_H_
#define _PARTIAL_MAP_H_

void partial_map_top(netlist_t *netlist);
void instantiate_add_w_carry(nnode_t *node, short mark, netlist_t *netlist);
void instantiate_multi_port_mux(nnode_t *node, short mark, netlist_t *netlist);
void depth_first_traversal_to_partial_map(short marker_value, netlist_t *netlist);
void instantiate_multi_port_n_bits_mux(nnode_t *node, short mark, netlist_t * /*netlist*/);

#endif // _PARTIAL_MAP_H_
