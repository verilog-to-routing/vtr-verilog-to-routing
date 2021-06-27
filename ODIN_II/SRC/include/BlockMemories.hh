/**
 * Copyright (c) 2021 Seyed Alireza Damghani (sdamghann@gmail.com)
 * 
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
#ifndef _BLOCK_MEMORIES_H_
#define _BLOCK_MEMORIES_H_

#include <unordered_map>

extern vtr::t_linked_vptr* block_memory_list;
extern vtr::t_linked_vptr* read_only_memory_list;

const int REG_INFERRENCE_THRESHOLD_WIDTH = 32;
const int REG_INFERRENCE_THRESHOLD_DEPTH = 20;
const int LUTRAM_MAX_THRESHOLD_AREA = 2560;
const int LUTRAM_MIN_THRESHOLD_AREA = 640;

/*
 * Contains a pointer to the block memory node as well as other
 * information which is used in creating the block memory.
 */
struct block_memory {
    loc_t loc;
    nnode_t* node;

    signal_list_t* read_addr;
    signal_list_t* read_clk;
    signal_list_t* read_data;
    signal_list_t* read_en;

    signal_list_t* write_addr;
    signal_list_t* write_clk;
    signal_list_t* write_data;
    signal_list_t* write_en;

    signal_list_t* clk;

    char* name;
    char* memory_id;
};

typedef std::unordered_map<std::string, block_memory*> block_memory_hashtable;
extern block_memory_hashtable block_memories;

extern void init_block_memory_index();
extern block_memory* lookup_block_memory(char* instance_name_prefix, char* identifier);
extern void free_block_memories();

extern void resolve_bram_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
extern void resolve_rom_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);

extern void iterate_block_memories(netlist_t* netlist);

#endif // _BLOCK_MEMORIES_H_
