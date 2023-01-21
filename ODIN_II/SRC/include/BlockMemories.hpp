/*
 * Copyright 2023 CAS—Atlantic (University of New Brunswick, CASA)
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

// Max number of bits for register of array inference
const int LUTRAM_INFERENCE_THRESHOLD_MIN = 80;  // Max number of bits for LUTRAM inference
const int LUTRAM_INFERENCE_THRESHOLD_MAX = 640; // Min number of bits for LUTRAM inference

/*
 * Contains a pointer to the block memory node as well as other
 * information which is used in creating the block memory.
 */
struct block_memory_t {
    loc_t loc;
    nnode_t* node;

    signal_list_t* read_addr;
    signal_list_t* read_data;
    signal_list_t* read_en;

    signal_list_t* write_addr;
    signal_list_t* write_data;
    signal_list_t* write_en;

    signal_list_t* clk;

    char* name;
    char* memory_id;
};

typedef std::unordered_map<std::string, block_memory_t*> block_memory_hashtable;

/**
 * block memories information. variable will be invalid
 * after iterations happen before partial mapping
 */
struct block_memory_information_t {
    /**
     * block_memory_list and read-only_memory_list linked lists
     * include the corresponding memory instances. Each instance
     * comprises memory signal lists, location, memory id and the
     * corresponding netlist node. These linked lists are used in
     * optimization iteration, including signal pruning, REG/LUTRAM
     * threshold checking and mapping to VTR memory blocks. Once the
     * optimization iteration is done, these linked lists are not 
     * valid anymore.
     * 
     * [NOTE] Block memories and read-only memory both use the same
     * structure (block_memory_t*). They only differ in terms of 
     * their member variables initialization. The naming convention
     * is only due to the ease of the coding process.
     */
    vtr::t_linked_vptr* block_memory_list;
    vtr::t_linked_vptr* read_only_memory_list;
    /* hashtable to look up block memories faster */
    block_memory_hashtable block_memories;
    block_memory_hashtable read_only_memories;
};
extern block_memory_information_t block_memories_info;

extern void free_block_memories();

extern void iterate_block_memories(netlist_t* netlist);

#endif // _BLOCK_MEMORIES_H_
