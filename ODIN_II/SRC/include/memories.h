/*
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef MEMORIES_H
#define MEMORIES_H

#include "odin_types.h"

extern vtr::t_linked_vptr* sp_memory_list;
extern vtr::t_linked_vptr* dp_memory_list;
extern vtr::t_linked_vptr* memory_instances;
extern vtr::t_linked_vptr* memory_port_size_list;
extern t_model* single_port_rams;
extern t_model* dual_port_rams;

#define HARD_RAM_ADDR_LIMIT 33
#define SOFT_RAM_ADDR_LIMIT 10

struct t_memory {
    long size_d1;
    long size_d2;
    long size_addr1;
    long size_addr2;
    long size_out1;
    long size_out2;
    struct s_memory* next;
};

struct t_memory_port_sizes {
    long size;
    char* name;
};

struct sp_ram_signals {
    signal_list_t* addr;
    signal_list_t* data;
    signal_list_t* out;
    npin_t* we;
    npin_t* clk;
};

struct dp_ram_signals {
    signal_list_t* addr1;
    signal_list_t* addr2;
    signal_list_t* data1;
    signal_list_t* data2;
    signal_list_t* out1;
    signal_list_t* out2;
    npin_t* we1;
    npin_t* we2;
    npin_t* clk;
};

extern sp_ram_signals* init_sp_ram_signals();
extern dp_ram_signals* init_dp_ram_signals();

long get_sp_ram_split_depth();
long get_dp_ram_split_depth();

sp_ram_signals* get_sp_ram_signals(nnode_t* node);
void free_sp_ram_signals(sp_ram_signals* signalsvar);

dp_ram_signals* get_dp_ram_signals(nnode_t* node);
void free_dp_ram_signals(dp_ram_signals* signalsvar);

bool is_sp_ram(nnode_t* node);
bool is_dp_ram(nnode_t* node);

bool is_blif_sp_ram(nnode_t* node);
bool is_blif_dp_ram(nnode_t* node);

bool is_ast_sp_ram(ast_node_t* node);
bool is_ast_dp_ram(ast_node_t* node);

void init_memory_distribution();
void check_memories_and_report_distribution();

long get_memory_port_size(const char* name);

long get_sp_ram_depth(nnode_t* node);
long get_dp_ram_depth(nnode_t* node);
long get_sp_ram_width(nnode_t* node);
long get_dp_ram_width(nnode_t* node);

void split_sp_memory_depth(nnode_t* node, int split_size);
void split_dp_memory_depth(nnode_t* node, int split_size);
void split_sp_memory_width(nnode_t* node, int target_size);
void split_dp_memory_width(nnode_t* node, int target_size);
void iterate_memories(netlist_t* netlist);
void free_memory_lists();

void instantiate_soft_single_port_ram(nnode_t* node, short mark, netlist_t* netlist);
void instantiate_soft_dual_port_ram(nnode_t* node, short mark, netlist_t* netlist);

signal_list_t* create_decoder(nnode_t* node, short mark, signal_list_t* input_list);

extern void add_input_port_to_memory(nnode_t* node, signal_list_t* signalsvar, const char* port_name);
extern void add_output_port_to_memory(nnode_t* node, signal_list_t* signalsvar, const char* port_name);
extern void remap_input_port_to_memory(nnode_t* node, signal_list_t* signalsvar, const char* port_name);
extern void remap_output_port_to_memory(nnode_t* node, signal_list_t* signalsvar, const char* port_name);

int* get_spram_hb_ports_sizes(int* hb_instance_ports_sizes, nnode_t* hb_instance);
int* get_dpram_hb_ports_sizes(int* hb_instance_ports_sizes, nnode_t* hb_instance);

extern nnode_t* create_single_port_ram(sp_ram_signals* spram_signals, nnode_t* node);
extern nnode_t* create_dual_port_ram(dp_ram_signals* dpram_signals, nnode_t* node);

extern void register_memory_model(nnode_t* mem);

extern void resolve_single_port_ram(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
extern void resolve_dual_port_ram(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);

#endif // MEMORIES_H
