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
#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "odin_types.h"

extern vtr::t_linked_vptr *sp_memory_list;
extern vtr::t_linked_vptr *dp_memory_list;
extern t_model *single_port_rams;
extern t_model *dual_port_rams;

#define HARD_RAM_ADDR_LIMIT 33
#define SOFT_RAM_ADDR_LIMIT 10

struct sp_ram_signals {
    signal_list_t *addr;
    signal_list_t *data;
    signal_list_t *out;
    npin_t *we;
    npin_t *clk;
};

struct dp_ram_signals {
    signal_list_t *addr1;
    signal_list_t *addr2;
    signal_list_t *data1;
    signal_list_t *data2;
    signal_list_t *out1;
    signal_list_t *out2;
    npin_t *we1;
    npin_t *we2;
    npin_t *clk;
};

extern sp_ram_signals *init_sp_ram_signals();
extern dp_ram_signals *init_dp_ram_signals();

long get_sp_ram_split_depth();
long get_dp_ram_split_depth();

sp_ram_signals *get_sp_ram_signals(nnode_t *node);
void free_sp_ram_signals(sp_ram_signals *signalsvar);

dp_ram_signals *get_dp_ram_signals(nnode_t *node);
void free_dp_ram_signals(dp_ram_signals *signalsvar);

bool is_sp_ram(nnode_t *node);
bool is_dp_ram(nnode_t *node);

bool is_blif_sp_ram(nnode_t *node);
bool is_blif_dp_ram(nnode_t *node);

void check_memories_and_report_distribution();

long get_sp_ram_depth(nnode_t *node);
long get_dp_ram_depth(nnode_t *node);
long get_sp_ram_width(nnode_t *node);
long get_dp_ram_width(nnode_t *node);

void split_sp_memory_depth(nnode_t *node, int split_size);
void split_dp_memory_depth(nnode_t *node, int split_size);
void split_sp_memory_width(nnode_t *node, int target_size);
void split_dp_memory_width(nnode_t *node, int target_size);
void iterate_memories(netlist_t *netlist);
void free_memory_lists();

void instantiate_soft_single_port_ram(nnode_t *node, short mark, netlist_t *netlist);
void instantiate_soft_dual_port_ram(nnode_t *node, short mark, netlist_t *netlist);

signal_list_t *create_decoder(nnode_t *node, short mark, signal_list_t *input_list, netlist_t *netlist);

extern void add_input_port_to_memory(nnode_t *node, signal_list_t *signalsvar, const char *port_name);
extern void add_output_port_to_memory(nnode_t *node, signal_list_t *signalsvar, const char *port_name);
extern void remap_input_port_to_memory(nnode_t *node, signal_list_t *signalsvar, const char *port_name);

extern nnode_t *create_single_port_ram(sp_ram_signals *spram_signals, nnode_t *node);
extern nnode_t *create_dual_port_ram(dp_ram_signals *dpram_signals, nnode_t *node);

extern void register_memory_model(nnode_t *mem);

extern void resolve_single_port_ram(nnode_t *node, uintptr_t traverse_mark_number, netlist_t *netlist);
extern void resolve_dual_port_ram(nnode_t *node, uintptr_t traverse_mark_number, netlist_t *netlist);

#endif // _MEMORY_H_
