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
#ifndef _BLOCK_MEMORY_H_
#define _BLOCK_MEMORY_H_

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
    nnode_t *node;

    signal_list_t *read_addr;
    signal_list_t *read_data;
    signal_list_t *read_en;

    signal_list_t *write_addr;
    signal_list_t *write_data;
    signal_list_t *write_en;

    signal_list_t *clk;

    char *name;
    char *memory_id;
};

typedef std::unordered_map<std::string, block_memory_t *> block_memory_hashtable;

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
    vtr::t_linked_vptr *block_memory_list;
    vtr::t_linked_vptr *read_only_memory_list;
    /* hashtable to look up block memories faster */
    block_memory_hashtable block_memories;
    block_memory_hashtable read_only_memories;
};
extern block_memory_information_t block_memories_info;

extern void init_block_memory_index();
extern void free_block_memories();

extern void resolve_ymem_node(nnode_t *node, uintptr_t traverse_mark_number, netlist_t *netlist);
extern void resolve_ymem2_node(nnode_t *node, uintptr_t traverse_mark_number, netlist_t *netlist);
extern void resolve_bram_node(nnode_t *node, uintptr_t traverse_mark_number, netlist_t *netlist);
extern void resolve_rom_node(nnode_t *node, uintptr_t traverse_mark_number, netlist_t *netlist);

extern void iterate_block_memories(netlist_t *netlist);

#endif // _BLOCK_MEMORY_H_
