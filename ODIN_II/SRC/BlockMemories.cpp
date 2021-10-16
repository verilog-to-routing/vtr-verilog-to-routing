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
 *
 * @file: This file includes the definitions of the routines to map 
 * block memories to VTR compatible memory types, i.e., Single Port RAM 
 * and Dual Port RAM. The definition of block memory and read-only memory
 * is provided in techlib directory in the Odin-II root directory. 
 * Basically, a memory block with both read and write accesses that has a
 * separate port for each operation is called BRAM. While following the 
 * same definition, a read-only memory block is referred to as a BRAM that
 * has only read access (even multiple accesses). This function also 
 * includes ymem block support which somehow represents the Yosys internal
 * memory cell.
 */

#include <string.h>
#include <math.h>
#include "odin_globals.h"
#include "odin_types.h"
#include "odin_util.h"
#include "ast_util.h"

#include "netlist_utils.h"
#include "node_creation_library.h"
#include "hard_blocks.h"
#include "memories.h"
#include "BlockMemories.hpp"
#include "partial_map.h"
#include "vtr_util.h"
#include "vtr_memory.h"

using vtr::t_linked_vptr;
/* global linked list including block memory instances */
struct block_memory_information_t block_memories_info;

static block_memory_t* init_block_memory(nnode_t* node, netlist_t* netlist);

void map_bram_to_mem_hardblocks(block_memory_t* bram, netlist_t* netlist);
void map_rom_to_mem_hardblocks(block_memory_t* rom, netlist_t* netlist);

static void create_r_single_port_ram(block_memory_t* rom, netlist_t* netlist);
static void create_2r_dual_port_ram(block_memory_t* rom, netlist_t* netlist);
static void create_nr_single_port_ram(block_memory_t* rom, netlist_t* netlist);
static void create_rw_single_port_ram(block_memory_t* bram, netlist_t* /* netlist */);
static void create_rw_dual_port_ram(block_memory_t* bram, netlist_t* netlist);
static void create_r2w_dual_port_ram(block_memory_t* bram, netlist_t* netlist);
static void create_2rw_dual_port_ram(block_memory_t* bram, netlist_t* netlist);
static void create_2r2w_dual_port_ram(block_memory_t* bram, netlist_t* netlist);
static void create_nrmw_dual_port_ram(block_memory_t* bram, netlist_t* netlist);

static nnode_t* ymem_to_rom(nnode_t* node, uintptr_t traverse_mark_number);
static nnode_t* ymem_to_bram(nnode_t* node, uintptr_t traverse_mark_number);

static bool check_same_addrs(block_memory_t* bram);
static void perform_optimization(block_memory_t* memory);
static signal_list_t* split_cascade_port(signal_list_t* signalvar, signal_list_t* selectors, int desired_width, nnode_t* node, netlist_t* netlist);
static void decode_out_port(signal_list_t* src, signal_list_t* outs, signal_list_t* selectors, nnode_t* node, netlist_t* netlist);

static void cleanup_block_memory_old_node(nnode_t* old_node);

static void free_block_memory_index(block_memory_hashtable to_free);
static void free_block_memory(block_memory_t* to_free);

/**
 * (function: init_block_memory_index)
 * 
 * @brief Initialises hashtables to lookup memories based on inputs and names.
 */
void init_block_memory_index() {
    block_memories_info.block_memories = block_memory_hashtable();
    block_memories_info.read_only_memories = block_memory_hashtable();
}

/**
 * (function: lookup_block_memory)
 * 
 * @brief Looks up an block memory by identifier name in the block memory lookup table.
 * 
 * @param instance_name_prefix memory node name (unique hard block node name)
 * @param identifier memory block id
 */
block_memory_t* lookup_block_memory(char* instance_name_prefix, char* identifier) {
    char* memory_string = make_full_ref_name(instance_name_prefix, NULL, NULL, identifier, -1);

    block_memory_hashtable::const_iterator mem_out = block_memories_info.block_memories.find(std::string(memory_string));

    vtr::free(memory_string);

    if (mem_out == block_memories_info.block_memories.end())
        return (NULL);
    else
        return (mem_out->second);
}

/**
 * (function: init_block_memory)
 * 
 * @brief: initialize bram signals
 * 
 * @param node pointing to a bram node
 * @param netlist pointer to the current netlist file
 */
static block_memory_t* init_block_memory(nnode_t* node, netlist_t* /* netlist */) {
    int i, offset;
    block_memory_t* bram = (block_memory_t*)vtr::malloc(sizeof(block_memory_t));

    /** 
     * BRAM information 
     * 
     * CLK:        input port [0] 
     * RD_ADDR:    input port [1] 
     * RD_DATA:    output port[0] 
     * RD_ENABLE:  input port [2] 
     * WR_ADDR:    input port [3] 
     * WR_DATA:    input port [4] 
     * WR_ENABLE:  input port [5] 
     */

    int CLK_width = node->input_port_sizes[0];
    int RD_ADDR_width = node->input_port_sizes[1];
    int RD_DATA_width = node->output_port_sizes[0];
    int RD_ENABLE_width = node->input_port_sizes[2];
    int WR_ADDR_width = node->input_port_sizes[3];
    int WR_DATA_width = node->input_port_sizes[4];
    int WR_ENABLE_width = node->input_port_sizes[5];

    oassert(CLK_width == 1);

    /* INPUT */

    /* CLK */
    offset = 0;
    bram->clk = init_signal_list();
    add_pin_to_signal_list(bram->clk, node->input_pins[offset]);

    /* read address pins */
    offset += CLK_width;
    bram->read_addr = init_signal_list();
    for (i = 0; i < RD_ADDR_width; ++i) {
        add_pin_to_signal_list(bram->read_addr, node->input_pins[i + offset]);
    }

    /* read enable pins */
    offset += RD_ADDR_width;
    bram->read_en = init_signal_list();
    for (i = 0; i < RD_ENABLE_width; ++i) {
        add_pin_to_signal_list(bram->read_en, node->input_pins[i + offset]);
    }

    /* write addr pins */
    offset += RD_ENABLE_width;
    bram->write_addr = init_signal_list();
    for (i = 0; i < WR_ADDR_width; ++i) {
        add_pin_to_signal_list(bram->write_addr, node->input_pins[i + offset]);
    }

    /* write data pins */
    offset += WR_ADDR_width;
    bram->write_data = init_signal_list();
    for (i = 0; i < WR_DATA_width; ++i) {
        add_pin_to_signal_list(bram->write_data, node->input_pins[i + offset]);
    }

    /* write enable clk pins */
    offset += WR_DATA_width;
    bram->write_en = init_signal_list();
    for (i = 0; i < WR_ENABLE_width; ++i) {
        add_pin_to_signal_list(bram->write_en, node->input_pins[i + offset]);
    }

    /* OUTPUT */
    /* read clk pins */
    offset = 0;
    bram->read_data = init_signal_list();
    for (i = 0; i < RD_DATA_width; ++i) {
        add_pin_to_signal_list(bram->read_data, node->output_pins[i + offset]);
    }

    /* creating new node since we need to reorder some input port for each inferenece mode */
    bram->node = node;

    /* keep track of location since we need for further node creation */
    bram->loc = node->loc;

    bram->memory_id = vtr::strdup(node->attributes->memory_id);

    /* creating a unique name for the block memory */
    bram->name = make_full_ref_name(bram->node->name, NULL, NULL, bram->memory_id, -1);
    block_memories_info.block_memories.emplace(bram->name, bram);

    return (bram);
}

/**
 * (function: init_block_memory)
 * 
 * @brief: initialize rom signals
 * 
 * @param node pointing to a rom node
 * @param netlist pointer to the current netlist file
 */
static block_memory_t* init_read_only_memory(nnode_t* node, netlist_t* netlist) {
    int i, offset;
    block_memory_t* rom = (block_memory_t*)vtr::malloc(sizeof(block_memory_t));

    /** 
     * ROM information
     * 
     * CLK:        input port [0] 
     * RD_ADDR:    input port [1] 
     * RD_DATA:    output port [0] 
     * RD_ENABLE:  input port [2] 
     */

    int CLK_width = node->input_port_sizes[0];
    int RD_ADDR_width = node->input_port_sizes[1];
    int RD_DATA_width = node->output_port_sizes[0];
    int RD_ENABLE_width = node->input_port_sizes[2];
    int WR_DATA_width = node->output_port_sizes[0];

    oassert(CLK_width == 1);

    /* INPUT */
    /* CLK */
    offset = 0;
    rom->clk = init_signal_list();
    add_pin_to_signal_list(rom->clk, node->input_pins[offset]);

    /* read address pins */
    offset += CLK_width;
    rom->read_addr = init_signal_list();
    for (i = 0; i < RD_ADDR_width; ++i) {
        add_pin_to_signal_list(rom->read_addr, node->input_pins[i + offset]);
    }

    /* read enable pins */
    offset += RD_ADDR_width;
    rom->read_en = init_signal_list();
    for (i = 0; i < RD_ENABLE_width; ++i) {
        add_pin_to_signal_list(rom->read_en, node->input_pins[i + offset]);
    }

    /* OUTPUT */
    offset = 0;
    rom->read_data = init_signal_list();
    for (i = 0; i < RD_DATA_width; ++i) {
        add_pin_to_signal_list(rom->read_data, node->output_pins[i + offset]);
    }

    /* PAD DATA IN */
    /* we pad the data_in port for rom using pad pins */
    rom->write_data = init_signal_list();
    for (i = 0; i < WR_DATA_width; ++i) {
        add_pin_to_signal_list(rom->write_data, get_pad_pin(netlist));
    }

    /* no need to these variables in rom */
    rom->write_addr = NULL;
    rom->write_en = NULL;

    /* creating new node since we need to reorder some input port for each inferenece mode */
    rom->node = node;

    /* keep track of location since we need for further node creation */
    rom->loc = node->loc;

    rom->memory_id = vtr::strdup(node->attributes->memory_id);

    /* creating a unique name for the block memory */
    rom->name = make_full_ref_name(rom->node->name, NULL, NULL, rom->memory_id, -1);
    block_memories_info.read_only_memories.emplace(rom->name, rom);

    return (rom);
}

/**
 * (function: create_r_single_port_ram)
 * 
 * @brief read_only_memory will be considered as single port ram
 * this function reorders inputs and add data_in input to new node
 * which is connected to pad node
 * 
 * @param rom pointing to a rom node node
 * @param netlist pointer to the current netlist file
 */
static void create_r_single_port_ram(block_memory_t* rom, netlist_t* netlist) {
    nnode_t* old_node = rom->node;
    int num_rd_ports = old_node->attributes->RD_PORTS;
    int num_wr_ports = old_node->attributes->WR_PORTS;

    /* should have been resovled before this function */
    oassert(num_rd_ports == 1);
    oassert(num_wr_ports == 0);

    /* single port ram signals */
    sp_ram_signals* signals = (sp_ram_signals*)vtr::calloc(1, sizeof(dp_ram_signals));

    /* INPUTS */
    /* adding the read addr input port as address1 */
    signals->addr = rom->read_addr;

    /* handle clk signal */
    signals->clk = rom->clk->pins[0];

    /**
     * we pad the data_in port using pad pins 
     * rom->write_data is already filled with pad pins
     */
    signals->data = rom->write_data;

    /* there is no write data to set any we, so it will be connected to GND */
    signals->we = get_zero_pin(netlist);
    delete_npin(rom->read_en->pins[0]);

    /* OUTPUT */
    /* adding the read data port as output1 */
    signals->out = rom->read_data;

    create_single_port_ram(signals, old_node);

    // CLEAN UP rom src node
    cleanup_block_memory_old_node(old_node);
    vtr::free(signals);
}

/**
 * (function: create_2r_dual_port_ram)
 * 
 * @brief in this case one write port MUST have the 
 * same address of the single read port.
 * Will be instantiated into a DPRAM.
 * 
 * @param bram pointer to the block memory
 * @param netlist pointer to the current netlist file
 */
static void create_2r_dual_port_ram(block_memory_t* bram, netlist_t* netlist) {
    nnode_t* old_node = bram->node;

    int i, offset;
    int data_width = bram->node->attributes->DBITS;
    int addr_width = bram->node->attributes->ABITS;
    int num_rd_ports = old_node->attributes->RD_PORTS;
    int num_wr_ports = old_node->attributes->WR_PORTS;

    /* should have been resovled before this function */
    oassert(num_rd_ports == 2);
    oassert(num_wr_ports == 0);
    oassert(bram->read_addr->count == 2 * addr_width);
    oassert(bram->read_data->count == 2 * data_width);

    /* create a list of dpram ram signals */
    dp_ram_signals* signals = (dp_ram_signals*)vtr::malloc(sizeof(dp_ram_signals));

    /* split read addr and add the first half to the addr1 */
    signals->addr1 = init_signal_list();
    for (i = 0; i < addr_width; ++i) {
        add_pin_to_signal_list(signals->addr1, bram->read_addr->pins[i]);
    }

    /* add pad pins as data1 */
    signals->data1 = init_signal_list();
    for (i = 0; i < data_width; ++i) {
        add_pin_to_signal_list(signals->data1, get_pad_pin(netlist));
    }

    /* there is no write data to set any we, so it will be connected to GND */
    signals->we1 = get_zero_pin(netlist);
    delete_npin(bram->read_en->pins[0]);

    /* add clk signal as dpram clk signal */
    signals->clk = bram->clk->pins[0];

    /* split read data and add the first half to the out1 */
    signals->out1 = init_signal_list();
    for (i = 0; i < data_width; ++i) {
        add_pin_to_signal_list(signals->out1, bram->read_data->pins[i]);
    }

    /* add the second half of the read addr to addr2 */
    offset = addr_width;
    signals->addr2 = init_signal_list();
    for (i = 0; i < addr_width; ++i) {
        add_pin_to_signal_list(signals->addr2, bram->read_addr->pins[i + offset]);
    }

    /* there is no write data to set any we, so it will be connected to GND */
    signals->we2 = get_zero_pin(netlist);
    delete_npin(bram->read_en->pins[1]);

    /* add the second half of the write data to data2 */
    signals->data2 = init_signal_list();
    for (i = 0; i < data_width; ++i) {
        add_pin_to_signal_list(signals->data2, get_pad_pin(netlist));
    }

    /* add the second half of the read data to out2 */
    offset = data_width;
    signals->out2 = init_signal_list();
    for (i = 0; i < data_width; ++i) {
        add_pin_to_signal_list(signals->out2, bram->read_data->pins[i + offset]);
    }

    /* create a new dual port ram */
    create_dual_port_ram(signals, old_node);

    // CLEAN UP
    cleanup_block_memory_old_node(old_node);
    free_dp_ram_signals(signals);
}

/**
 * (function: create_nr_single_port_ram)
 * 
 * @brief multiple read ports are multiplexed using read enable.
 * Then, the rom will be mapped to a SPRAM
 * 
 * @param rom pointing to a rom node node
 * @param netlist pointer to the current netlist file
 */
static void create_nr_single_port_ram(block_memory_t* rom, netlist_t* netlist) {
    nnode_t* old_node = rom->node;
    int data_width = rom->node->attributes->DBITS;
    int addr_width = rom->node->attributes->ABITS;
    int num_rd_ports = old_node->attributes->RD_PORTS;
    int num_wr_ports = old_node->attributes->WR_PORTS;

    /* validation */
    oassert(num_rd_ports > 1);
    oassert(num_wr_ports == 0);

    /* single port ram signals */
    sp_ram_signals* signals = (sp_ram_signals*)vtr::calloc(1, sizeof(dp_ram_signals));
    signal_list_t* selectors = NULL;

    /* INPUTS */
    selectors = copy_input_signals(rom->read_en);
    /* adding the muxed read addrs as spram address */
    signals->addr = split_cascade_port(rom->read_addr,
                                       selectors,
                                       addr_width,
                                       old_node,
                                       netlist);

    /* add clk singnal */
    signals->clk = rom->clk->pins[0];

    /**
     * rom->write_data is already initialized with pad pins in rom init
     */
    signals->data = copy_input_signals(rom->write_data);

    /* there is no write data to set any we, so it will be connected to GND */
    signals->we = get_zero_pin(netlist);

    /* OUTPUT */
    /* leave it empty, so create_single port ram function create a new pins */
    signals->out = NULL;

    /* create a SPRAM */
    nnode_t* spram = create_single_port_ram(signals, old_node);

    signal_list_t* spram_outputs = init_signal_list();
    for (int i = 0; i < data_width; ++i) {
        add_pin_to_signal_list(spram_outputs, spram->output_pins[i]);
    }

    /* decode the spram outputs to the n rom output ports */
    decode_out_port(spram_outputs, rom->read_data, rom->read_en, old_node, netlist);

    // CLEAN UP rom src node
    free_signal_list(selectors);
    free_signal_list(spram_outputs);

    free_sp_ram_signals(signals);
    cleanup_block_memory_old_node(old_node);
}

/**
 * (function: create_rw_single_port_ram)
 * 
 * @brief creates a single port ram for a block memory 
 * with one read and one write port that has the same  
 * addr for both read and write
 * 
 * @param bram pointing to a bram node node
 * @param netlist pointer to the current netlist file
 */
static void create_rw_single_port_ram(block_memory_t* bram, netlist_t* /* netlist */) {
    int i;
    nnode_t* old_node = bram->node;
    int num_rd_ports = old_node->attributes->RD_PORTS;
    int num_wr_ports = old_node->attributes->WR_PORTS;

    /* should have been resovled before this function */
    oassert(num_rd_ports == 1);
    oassert(num_wr_ports == 1);

    /* single port ram signals */
    sp_ram_signals* signals = (sp_ram_signals*)vtr::calloc(1, sizeof(dp_ram_signals));

    /* the wr addr will be deleted since we do not need it anymore */
    for (i = 0; i < bram->write_addr->count; ++i) {
        npin_t* wr_addr_pin = bram->write_addr->pins[i];
        /* delete pin */
        delete_npin(wr_addr_pin);
    }

    /* INPUTS */
    /* adding the read addr input port as address1 */
    signals->addr = bram->read_addr;

    /* handling clock signals */
    signals->clk = bram->clk->pins[0];

    /**
     * we pad the data_in port using pad pins 
     * bram->write_data is already filled with pad pins
     */
    signals->data = bram->write_data;

    /* the rd enables will be deleted since we do not need it anymore */
    for (i = 0; i < bram->read_en->count; ++i) {
        npin_t* rd_en_pin = bram->read_en->pins[i];
        /* delete pin */
        delete_npin(rd_en_pin);
    }

    /* merge all enables */
    if (bram->write_en->count > 1) {
        /* need to OR all write enable since we1 should be one bit in single port ram */
        // bram->write_en = make_chain(LOGICAL_OR, bram->write_en, old_node);
        for (i = 1; i < bram->write_en->count; ++i) {
            delete_npin(bram->write_en->pins[i]);
        }
    }
    signals->we = bram->write_en->pins[0];

    /* OUTPUT */
    /* adding the read data port as output1 */
    signals->out = bram->read_data;

    create_single_port_ram(signals, old_node);

    // CLEAN UP bram src node
    cleanup_block_memory_old_node(old_node);
    vtr::free(signals);
}

/**
 * (function: create_rw_dual_port_ram)
 * 
 * @brief block_ram will be considered as dual port ram
 * this function reorders inputs and hook pad pins into datain2 
 * it also leaves the second output of the dual port ram unconnected
 * 
 * @param bram pointing to a block memory
 * @param netlist pointer to the current netlist file
 */
static void create_rw_dual_port_ram(block_memory_t* bram, netlist_t* netlist) {
    int i;
    nnode_t* old_node = bram->node;
    int num_rd_ports = old_node->attributes->RD_PORTS;
    int num_wr_ports = old_node->attributes->WR_PORTS;

    /* should have been resovled before this function */
    oassert(num_rd_ports == 1);
    oassert(num_wr_ports == 1);

    /* dual port ram signals */
    dp_ram_signals* signals = (dp_ram_signals*)vtr::calloc(1, sizeof(dp_ram_signals));

    /* INPUTS */
    signals->addr1 = bram->read_addr;
    signals->addr2 = bram->write_addr;

    /* adding the write addr port as address2 */
    signals->clk = bram->clk->pins[0];

    /* adding the write data port as data1 */
    signals->data2 = bram->write_data;

    /* we pad the second data port using pad pins */
    signal_list_t* pad_signals = init_signal_list();
    for (i = 0; i < bram->write_data->count; ++i) {
        add_pin_to_signal_list(pad_signals, get_pad_pin(netlist));
    }
    signals->data1 = pad_signals;

    for (i = 0; i < bram->read_en->count; ++i) {
        /* delete all read enable pins, since no need to write from addr1 */
        delete_npin(bram->read_en->pins[0]);
    }
    signals->we1 = get_zero_pin(netlist);

    /* adding wr_en as we of port 2 */
    signals->we2 = bram->write_en->pins[0];

    /* OUTPUT */
    /* adding the read data port as output1 */
    signals->out1 = bram->read_data;

    /* leave second output port unconnected */
    int offset = bram->read_data->count;
    signal_list_t* out2_signals = init_signal_list();
    for (i = 0; i < bram->read_data->count; i++) {
        // specify the output pin
        npin_t* new_pin1 = allocate_npin();
        npin_t* new_pin2 = allocate_npin();
        nnet_t* new_net = allocate_nnet();
        new_net->name = make_full_ref_name(NULL, NULL, NULL, bram->name, offset + i);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net, new_pin1);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net, new_pin2);

        /* adding to signal list */
        add_pin_to_signal_list(out2_signals, new_pin1);
    }
    signals->out2 = out2_signals;

    create_dual_port_ram(signals, old_node);

    // CLEAN UP
    cleanup_block_memory_old_node(old_node);
    free_signal_list(pad_signals);
    free_signal_list(out2_signals);
    vtr::free(signals);
}

/**
 * (function: create_r2w_dual_port_ram)
 * 
 * @brief in this case one write port MUST have the 
 * same address of the single read port.
 * Will be instantiated into a DPRAM.
 * 
 * @param bram pointer to the block memory
 * @param netlist pointer to the current netlist file
 */
static void create_r2w_dual_port_ram(block_memory_t* bram, netlist_t* netlist) {
    nnode_t* old_node = bram->node;

    int i, offset;
    int data_width = bram->node->attributes->DBITS;
    int addr_width = bram->node->attributes->ABITS;
    int num_rd_ports = old_node->attributes->RD_PORTS;
    int num_wr_ports = old_node->attributes->WR_PORTS;

    /* should have been resovled before this function */
    oassert(num_rd_ports == 1);
    oassert(num_wr_ports == 2);

    /* create a list of dpram ram signals */
    dp_ram_signals* signals = (dp_ram_signals*)vtr::malloc(sizeof(dp_ram_signals));

    /* add read address as addr1 to dpram signal lists */
    signals->addr1 = init_signal_list();
    for (i = 0; i < bram->read_addr->count; ++i) {
        add_pin_to_signal_list(signals->addr1, bram->read_addr->pins[i]);
    }

    /* split wr_addr, wr_data and wr_en ports */
    offset = addr_width;
    signal_list_t* wr_addr1 = init_signal_list();
    signal_list_t* wr_addr2 = init_signal_list();
    for (i = 0; i < addr_width; ++i) {
        add_pin_to_signal_list(wr_addr1, bram->write_addr->pins[i]);
        add_pin_to_signal_list(wr_addr2, bram->write_addr->pins[i + offset]);
    }

    oassert(bram->write_en->count == 2);
    npin_t* wr_en1 = bram->write_en->pins[0];
    npin_t* wr_en2 = bram->write_en->pins[1];

    offset = data_width;
    signal_list_t* wr_data1 = init_signal_list();
    signal_list_t* wr_data2 = init_signal_list();
    for (i = 0; i < data_width; ++i) {
        add_pin_to_signal_list(wr_data1, bram->write_data->pins[i]);
        add_pin_to_signal_list(wr_data2, bram->write_data->pins[i + offset]);
    }

    /**
     * [NOTE]:
     * Odin-II handle memory block with more than two distint
     * address ports by muxing read/write ports based on en
     */
    bool first_match = sigcmp(wr_addr1, bram->read_addr);
    bool second_match = sigcmp(wr_addr2, bram->read_addr);

    if (!first_match && !second_match) {
        first_match = sigcmp(bram->read_addr, wr_addr1);
        second_match = sigcmp(bram->read_addr, wr_addr2);
        if (!first_match && !second_match) {
            // CLEAN UP
            free_signal_list(wr_addr1);
            free_signal_list(wr_addr2);
            free_signal_list(wr_data1);
            free_signal_list(wr_data2);

            /* all ports have different address */
            create_nrmw_dual_port_ram(bram, netlist);
            return;
        }
    }

    /**
     * one write address is always equal to read addr.
     * As a result, the corresponding write data will be mapped to data1 
     */
    signals->data1 = (first_match) ? wr_data1 : wr_data2;

    /* free read en since we do not need in DPRAM model */
    delete_npin(bram->read_en->pins[0]);
    /* add write enable signals for matched wr port as we1 */
    signals->we1 = (first_match) ? wr_en1 : wr_en2;

    /* add clk signal as dpram clk signal */
    signals->clk = bram->clk->pins[0];

    /* map read data to the out1 */
    signals->out1 = init_signal_list();
    for (i = 0; i < bram->read_data->count; ++i) {
        add_pin_to_signal_list(signals->out1, bram->read_data->pins[i]);
    }

    /* OTHER WR RELATED PORTS */
    /* addr2 for another write addr */
    signals->addr2 = (first_match) ? wr_addr2 : wr_addr1;

    /* add write enable signals for second wr port as we2 */
    signals->we2 = (first_match) ? wr_en2 : wr_en1;

    /* the rest of write data pin is for data2 */
    signals->data2 = (first_match) ? wr_data2 : wr_data1;

    /* out2 will be unconnected */
    signals->out2 = init_signal_list();
    for (i = 0; i < signals->out1->count; ++i) {
        /* create the clk node's output pin */
        npin_t* new_pin1 = allocate_npin();
        npin_t* new_pin2 = allocate_npin();
        nnet_t* new_net = allocate_nnet();
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net, new_pin1);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net, new_pin2);

        /* hook the output pin into the node */
        add_pin_to_signal_list(signals->out2, new_pin1);
    }

    /* create a new dual port ram */
    create_dual_port_ram(signals, old_node);

    // CLEAN UP
    /* free matched wr addr pins since they are as the same as read addr */
    for (i = 0; i < addr_width; ++i) {
        npin_t* pin = (first_match) ? wr_addr1->pins[i] : wr_addr2->pins[i];
        /* delete pin */
        delete_npin(pin);
    }
    free_signal_list((first_match) ? wr_addr1 : wr_addr2);

    cleanup_block_memory_old_node(old_node);
    free_dp_ram_signals(signals);
}

/**
 * (function: create_2rw_dual_port_ram)
 * 
 * @brief creates a dual port ram. The given bram should have
 * a pair of read addr and write addr with the same addrs
 * 
 * @param bram pointer to the block memory
 * @param netlist pointer to the current netlist file
 */
static void create_2rw_dual_port_ram(block_memory_t* bram, netlist_t* netlist) {
    nnode_t* old_node = bram->node;

    int i, offset;
    int data_width = bram->node->attributes->DBITS;
    int addr_width = bram->node->attributes->ABITS;
    int num_rd_ports = old_node->attributes->RD_PORTS;
    int num_wr_ports = old_node->attributes->WR_PORTS;

    /* should have been resovled before this function */
    oassert(num_rd_ports == 2);
    oassert(num_wr_ports == 1);

    /* create a list of dpram ram signals */
    dp_ram_signals* signals = (dp_ram_signals*)vtr::malloc(sizeof(dp_ram_signals));

    /* add write address as addr1 to dpram signal lists */
    signals->addr1 = init_signal_list();
    for (i = 0; i < bram->write_addr->count; ++i) {
        add_pin_to_signal_list(signals->addr1, bram->write_addr->pins[i]);
    }

    /**
     * one write address is always equal to read addr.
     * As a result, the corresponding write data will be mapped to data1 
     */
    signals->data1 = init_signal_list();
    for (i = 0; i < data_width; ++i) {
        add_pin_to_signal_list(signals->data1, bram->write_data->pins[i]);
    }

    oassert(bram->write_en->count == 1);
    /* add write enable signals for matched wr port as we1 */
    signals->we1 = bram->write_en->pins[0];

    /* split rd_addr, rd_data ports */
    offset = addr_width;
    signal_list_t* rd_addr1 = init_signal_list();
    signal_list_t* rd_addr2 = init_signal_list();
    for (i = 0; i < addr_width; ++i) {
        add_pin_to_signal_list(rd_addr1, bram->read_addr->pins[i]);
        add_pin_to_signal_list(rd_addr2, bram->read_addr->pins[i + offset]);
    }

    offset = data_width;
    signal_list_t* rd_data1 = init_signal_list();
    signal_list_t* rd_data2 = init_signal_list();
    for (i = 0; i < data_width; ++i) {
        add_pin_to_signal_list(rd_data1, bram->read_data->pins[i]);
        add_pin_to_signal_list(rd_data2, bram->read_data->pins[i + offset]);
    }

    /* delete rd pins since we use corresponding wr_en and zero*/
    for (i = 0; i < bram->read_en->count; ++i) {
        delete_npin(bram->read_en->pins[i]);
    }

    /**
     * [NOTE]:
     * Odin-II handle memory block with more than two distint
     * address ports using muxed read/write ports
     */
    bool first_match = sigcmp(bram->write_addr, rd_addr1);
    bool second_match = sigcmp(bram->write_addr, rd_addr2);

    if (!first_match && !second_match) {
        first_match = sigcmp(rd_addr1, bram->write_addr);
        second_match = sigcmp(rd_addr2, bram->write_addr);
        if (!first_match && !second_match) {
            // CLEAN UP
            free_signal_list(rd_addr1);
            free_signal_list(rd_addr2);
            free_signal_list(rd_data1);
            free_signal_list(rd_data2);

            /* all ports have different address */
            create_nrmw_dual_port_ram(bram, netlist);
            return;
        }
    }

    /* map matched read data to the out1 */
    signals->out1 = (first_match) ? rd_data1 : rd_data2;

    /* add merged clk signal as dpram clk signal */
    signals->clk = bram->clk->pins[0];

    /* OTHER WR RELATED PORTS */
    /* addr2 for another write addr */
    signals->addr2 = (first_match) ? rd_addr2 : rd_addr1;

    /* en 2 should always be 0 since there is no second write port */
    signals->we2 = get_zero_pin(netlist);

    /* the rest of write data pin is for data2 */
    signals->data2 = init_signal_list();
    for (i = 0; i < data_width; ++i) {
        add_pin_to_signal_list(signals->data2, get_pad_pin(netlist));
    }

    /* out2 map to other read data */
    signals->out2 = (first_match) ? rd_data2 : rd_data1;

    /* create a new dual port ram */
    create_dual_port_ram(signals, old_node);

    // CLEAN UP
    /* free matched rd addr pins since they are as the same as write addr */
    for (i = 0; i < addr_width; ++i) {
        npin_t* pin = (first_match) ? rd_addr1->pins[i] : rd_addr2->pins[i];
        /* delete pin */
        delete_npin(pin);
    }
    free_signal_list((first_match) ? rd_addr1 : rd_addr2);

    cleanup_block_memory_old_node(old_node);
    free_dp_ram_signals(signals);
}

/**
 * (function: create_2r2w_dual_port_ram)
 * 
 * @brief creates a dual port ram. The given bram should have
 * two pairs of read addr and write addr with the same addrs
 * 
 * @param bram pointer to the block memory
 * @param netlist pointer to the current netlist file
 */
static void create_2r2w_dual_port_ram(block_memory_t* bram, netlist_t* netlist) {
    nnode_t* old_node = bram->node;

    int i, offset;
    int data_width = bram->node->attributes->DBITS;
    int addr_width = bram->node->attributes->ABITS;
    int num_rd_ports = old_node->attributes->RD_PORTS;
    int num_wr_ports = old_node->attributes->WR_PORTS;

    /* should have been resovled before this function */
    oassert(num_rd_ports == 2);
    oassert(num_wr_ports == 2);
    oassert(bram->read_addr->count == 2 * addr_width);
    oassert(bram->read_data->count == 2 * data_width);

    /* split wr_addr, wr_data and wr_en ports */
    offset = addr_width;
    signal_list_t* wr_addr1 = init_signal_list();
    signal_list_t* wr_addr2 = init_signal_list();
    for (i = 0; i < addr_width; ++i) {
        add_pin_to_signal_list(wr_addr1, bram->write_addr->pins[i]);
        add_pin_to_signal_list(wr_addr2, bram->write_addr->pins[i + offset]);
    }

    oassert(bram->write_en->count == 2);
    npin_t* wr_en1 = bram->write_en->pins[0];
    npin_t* wr_en2 = bram->write_en->pins[1];

    offset = data_width;
    signal_list_t* wr_data1 = init_signal_list();
    signal_list_t* wr_data2 = init_signal_list();
    for (i = 0; i < data_width; ++i) {
        add_pin_to_signal_list(wr_data1, bram->write_data->pins[i]);
        add_pin_to_signal_list(wr_data2, bram->write_data->pins[i + offset]);
    }

    /* split rd_addr, rd_data ports */
    offset = addr_width;
    signal_list_t* rd_addr1 = init_signal_list();
    signal_list_t* rd_addr2 = init_signal_list();
    for (i = 0; i < addr_width; ++i) {
        add_pin_to_signal_list(rd_addr1, bram->read_addr->pins[i]);
        add_pin_to_signal_list(rd_addr2, bram->read_addr->pins[i + offset]);
    }

    offset = data_width;
    signal_list_t* rd_data1 = init_signal_list();
    signal_list_t* rd_data2 = init_signal_list();
    for (i = 0; i < data_width; ++i) {
        add_pin_to_signal_list(rd_data1, bram->read_data->pins[i]);
        add_pin_to_signal_list(rd_data2, bram->read_data->pins[i + offset]);
    }

    /* delete rd pins since we use corresponding wr_en and zero*/
    for (i = 0; i < bram->read_en->count; ++i) {
        delete_npin(bram->read_en->pins[i]);
    }

    /**
     * [NOTE]:
     * Odin-II handle memory block with more than two distint
     * address ports using muxed read/write ports
     */
    bool first_match_read1 = sigcmp(wr_addr1, rd_addr1);
    bool second_match_read1 = sigcmp(wr_addr2, rd_addr1);
    if (!first_match_read1 && !second_match_read1) {
        first_match_read1 = sigcmp(rd_addr1, wr_addr1);
        second_match_read1 = sigcmp(rd_addr1, wr_addr2);
    }

    if (!first_match_read1 && !second_match_read1) {
        // CLEAN UP
        free_signal_list(wr_addr1);
        free_signal_list(wr_addr2);
        free_signal_list(wr_data1);
        free_signal_list(wr_data2);
        free_signal_list(rd_addr1);
        free_signal_list(rd_addr2);
        free_signal_list(rd_data1);
        free_signal_list(rd_data2);

        /* all ports have different address */
        create_nrmw_dual_port_ram(bram, netlist);
        return;
    }

    /* create a list of dpram ram signals */
    dp_ram_signals* signals = (dp_ram_signals*)vtr::malloc(sizeof(dp_ram_signals));

    /* hook the first half os splitted addr into addr1 */
    signals->addr1 = rd_addr1;

    /* hook the second half os splitted addr into addr2 */
    signals->addr2 = rd_addr2;

    /* split read data and add the first half to the out1 */
    signals->out1 = rd_data1;

    /* add the second half of the read data to out2 */
    signals->out2 = rd_data2;

    /* split write data and add the first half to the data1 */
    signals->data1 = (first_match_read1) ? wr_data1 : wr_data2;

    /* split write data and add the second half to the data2 */
    signals->data2 = (first_match_read1) ? wr_data2 : wr_data1;

    /* add write enable signals for first wr port as we1 */
    signals->we1 = wr_en1;

    /* add clk signal as dpram clk signal */
    signals->clk = bram->clk->pins[0];

    /* add write enable signals for the second wr port as we2 */
    signals->we2 = wr_en2;

    /* create a new dual port ram */
    create_dual_port_ram(signals, old_node);

    // CLEAN UP
    /* at this point wr_addr and rd_addr must be the same */
    oassert(bram->read_addr->count == bram->write_addr->count);
    /* the wr addr will be deleted since we do not need it anymore */
    for (i = 0; i < bram->write_addr->count; ++i) {
        npin_t* wr_addr_pin = bram->write_addr->pins[i];
        /* delete pin */
        delete_npin(wr_addr_pin);
    }
    free_signal_list(wr_addr1);
    free_signal_list(wr_addr2);

    cleanup_block_memory_old_node(old_node);
    free_dp_ram_signals(signals);
}

/**
 * (function: create_nrmw_dual_port_ram)
 * 
 * @brief multiple read ports are multiplexed using read enable.
 * multiple write ports are multiplexed using write enable.
 * Then, the BRAM will be mapped to a DPRAM
 * 
 * @param bram pointing to a bram node node
 * @param netlist pointer to the current netlist file
 */
static void create_nrmw_dual_port_ram(block_memory_t* bram, netlist_t* netlist) {
    int i;
    nnode_t* old_node = bram->node;
    int data_width = bram->node->attributes->DBITS;
    int addr_width = bram->node->attributes->ABITS;
    int num_rd_ports = old_node->attributes->RD_PORTS;
    int num_wr_ports = old_node->attributes->WR_PORTS;

    /* should have been resovled before this function */
    oassert(num_rd_ports > 2);
    oassert(num_wr_ports > 2);

    /* dual port ram signals */
    dp_ram_signals* signals = (dp_ram_signals*)vtr::calloc(1, sizeof(dp_ram_signals));
    signal_list_t* selectors = NULL;

    /* INPUTS */
    selectors = copy_input_signals(bram->read_en);
    /* adding the read addr input port as address1 */
    signals->addr1 = split_cascade_port(bram->read_addr,
                                        selectors,
                                        addr_width,
                                        old_node,
                                        netlist);
    free_signal_list(selectors);

    selectors = copy_input_signals(bram->write_en);
    /* adding the write addr port as address2 */
    signals->addr2 = split_cascade_port(bram->write_addr,
                                        selectors,
                                        addr_width,
                                        old_node,
                                        netlist);
    free_signal_list(selectors);

    /* handling clock signals */
    signals->clk = bram->clk->pins[0];

    /* we pad the first data port using pad pins */
    signals->data1 = init_signal_list();
    for (i = 0; i < data_width; ++i) {
        add_pin_to_signal_list(signals->data1, get_pad_pin(netlist));
    }
    selectors = copy_input_signals(bram->write_en);
    /* adding the write data port as data2 */
    signals->data2 = split_cascade_port(bram->write_data,
                                        selectors,
                                        data_width,
                                        old_node,
                                        netlist);
    free_signal_list(selectors);

    /* first port does not have data, so the enable is GND */
    signals->we1 = get_zero_pin(netlist);

    /* create vcc signas as the value of we2 when the write_en pins are active */
    signal_list_t* vcc_signals = init_signal_list();
    for (i = 0; i < num_wr_ports; ++i) {
        add_pin_to_signal_list(vcc_signals, get_one_pin(netlist));
    }
    signal_list_t* we2_signal = split_cascade_port(vcc_signals,
                                                   bram->write_en,
                                                   1,
                                                   old_node,
                                                   netlist);

    signals->we2 = we2_signal->pins[0];

    /* OUTPUT */
    /* leaving out1 of dpram null, so it will create a new pins */
    signals->out1 = NULL;
    signals->out2 = NULL;

    /* create a DPRAM node */
    nnode_t* dpram = create_dual_port_ram(signals, old_node);

    signal_list_t* dpram_outputs = init_signal_list();
    for (i = 0; i < data_width; ++i) {
        add_pin_to_signal_list(dpram_outputs, dpram->output_pins[i]);
    }

    /* decode the spram outputs to the n bram output ports */
    decode_out_port(dpram_outputs, bram->read_data, bram->read_en, old_node, netlist);

    // CLEAN UP
    cleanup_block_memory_old_node(old_node);
    free_signal_list(dpram_outputs);
    free_signal_list(we2_signal);
    free_signal_list(vcc_signals);
    free_dp_ram_signals(signals);
}

/**
 * (function: map_rom_to_mem_hardblocks)
 * 
 * @brief mapping a read-only memory to single_port_ram or 
 * dual_port_ram according to the number and source of its ports
 * 
 * @param rom pointer to the read only memory
 * @param netlist pointer to the current netlist file
 */
void map_rom_to_mem_hardblocks(block_memory_t* rom, netlist_t* netlist) {
    nnode_t* node = rom->node;

    int width = node->attributes->DBITS;
    int depth = shift_left_value_with_overflow_check(0X1, node->attributes->ABITS, rom->loc);

    int rd_ports = node->attributes->RD_PORTS;
    int wr_ports = node->attributes->WR_PORTS;

    /* Read Only Memory validateion */
    oassert(wr_ports == 0);

    int rom_relative_area = depth * width;
    t_model* lutram_model = find_hard_block(LUTRAM_string);

    if (lutram_model != NULL
        && (LUTRAM_INFERENCE_THRESHOLD_MIN <= rom_relative_area)
        && (rom_relative_area <= LUTRAM_INFERENCE_THRESHOLD_MAX)) {
        /* map to LUTRAM */
        // nnode_t* lutram = NULL;
        /* TODO */
    } else {
        /* need to split the rom from the data width */
        if (rd_ports == 1) {
            /* create the ROM and allocate ports according to the SPRAM hard block */
            create_r_single_port_ram(rom, netlist);

        } else if (rd_ports == 2) {
            /* create the ROM and allocate ports according to the DPRAM hard block */
            create_2r_dual_port_ram(rom, netlist);

        } else {
            /* more than 2 read ports wil be handle using multiplexed ports and a SPRAM */
            create_nr_single_port_ram(rom, netlist);
        }
    }
}

/**
 * (function: map_bram_to_mem_hardblocks)
 * 
 * @brief mapping a block_memory (has both read and write access) to single_port_ram 
 * or dual_port_ram according to the number and source of its ports
 * 
 * @param bram pointer to the block memory
 * @param netlist pointer to the current netlist file
 */
void map_bram_to_mem_hardblocks(block_memory_t* bram, netlist_t* netlist) {
    nnode_t* node = bram->node;

    int width = node->attributes->DBITS;
    int depth = shift_left_value_with_overflow_check(0X1, node->attributes->ABITS, bram->loc);

    /* since the data1_w + data2_w == data_out for DPRAM */
    long rd_ports = node->attributes->RD_PORTS;
    long wr_ports = node->attributes->WR_PORTS;

    /**
     * Potential place for checking block ram if their relative 
     * size is less than a threshold, they could be mapped on LUTRAM
     */

    int bram_relative_area = depth * width;
    t_model* lutram_model = find_hard_block(LUTRAM_string);

    if (lutram_model != NULL
        && (LUTRAM_INFERENCE_THRESHOLD_MIN <= bram_relative_area)
        && (bram_relative_area <= LUTRAM_INFERENCE_THRESHOLD_MAX)) {
        /* map to LUTRAM */
        // nnode_t* lutram = NULL;
        /* TODO */
    } else {
        if (wr_ports == (rd_ports == 1)) {
            if (check_same_addrs(bram)) {
                /* create a single port ram and allocate ports according to the SPRAM hard block */
                create_rw_single_port_ram(bram, netlist);

            } else {
                /* create a dual port ram and allocate ports according to the DPRAM hard block */
                create_rw_dual_port_ram(bram, netlist);
            }

        } else if (rd_ports == 1 && wr_ports == 2) {
            /* create a dual port ram and allocate ports according to the DPRAM hard block */
            create_r2w_dual_port_ram(bram, netlist);

        } else if (rd_ports == 2 && wr_ports == 1) {
            /* create a dual port ram and allocate ports according to the DPRAM hard block */
            create_2rw_dual_port_ram(bram, netlist);

        } else if (rd_ports == 2 && wr_ports == 2) {
            /* create a dual port ram and allocate ports according to the DPRAM hard block */
            create_2r2w_dual_port_ram(bram, netlist);

        } else {
            /* create a dual port ram and muxed all read together and all writes together */
            create_nrmw_dual_port_ram(bram, netlist);
        }
    }
}
/**
 * (function: resolve_bram_node)
 * 
 * @brief create, verify and shrink the bram node
 * 
 * @param node pointing to a bram node
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
void resolve_bram_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);

    /* validate bram port sizes */
    oassert(node->num_input_port_sizes == 6);
    oassert(node->num_output_port_sizes == 1);

    /* initializing a new block ram */
    block_memory_t* bram = init_block_memory(node, netlist);

    /* perform optimization on memory inference */
    perform_optimization(bram);

    block_memories_info.block_memory_list = insert_in_vptr_list(block_memories_info.block_memory_list, bram);
}

/**
 * (function: resolve_rom_node)
 * 
 * @brief read_only_memory will be considered as single port ram
 * this function reorders inputs and add data_in input to new node
 * which is connected to pad node
 * 
 * @param node pointing to a rom node node
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
void resolve_rom_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);

    /* validate port sizes */
    oassert(node->num_input_port_sizes == 3);
    oassert(node->num_output_port_sizes == 1);

    /* create the rom and allocate ports according to the DPRAM hard block */
    block_memory_t* rom = init_read_only_memory(node, netlist);

    /* perform optimization on memory inference */
    perform_optimization(rom);

    block_memories_info.read_only_memory_list = insert_in_vptr_list(block_memories_info.read_only_memory_list, rom);
}

/**
 * (function: resolve_ymem_node)
 * 
 * @brief change ymem to bram or rom
 * 
 * @param node pointing to a bram node
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
void resolve_ymem_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);

    nnode_t* transformed_mem = NULL;

    /* check for BRAM */
    if ((node->num_input_port_sizes == 7) && (node->num_output_port_sizes == 1)) {
        /* create BRAM node */
        transformed_mem = ymem_to_bram(node, traverse_mark_number);
        /* resolve bram node */
        resolve_bram_node(transformed_mem, traverse_mark_number, netlist);
    }
    /* check for ROM */
    else if ((node->num_input_port_sizes == 3) && (node->num_output_port_sizes == 1)) {
        /* create BRAM node */
        transformed_mem = ymem_to_rom(node, traverse_mark_number);
        /* resolve bram node */
        resolve_rom_node(transformed_mem, traverse_mark_number, netlist);
    }
}

/**
 * (function: iterate_block_memories)
 * 
 * @brief iterate over block memories to map them to DP/SPRAMs
 * 
 * @param netlist pointer to the current netlist file
 */
void iterate_block_memories(netlist_t* netlist) {
    t_linked_vptr* ptr = block_memories_info.block_memory_list;
    while (ptr != NULL) {
        block_memory_t* bram = (block_memory_t*)ptr->data_vptr;

        /* validation */
        oassert(bram != NULL);

        map_bram_to_mem_hardblocks(bram, netlist);
        ptr = ptr->next;
    }

    ptr = block_memories_info.read_only_memory_list;
    while (ptr != NULL) {
        block_memory_t* rom = (block_memory_t*)ptr->data_vptr;

        /* validation */
        oassert(rom != NULL);

        map_rom_to_mem_hardblocks(rom, netlist);
        ptr = ptr->next;
    }
}

/**
 * (function: check_same_addrs)
 * 
 * @brief this function is to check if the read addr
 *  and write addr is the same or not.
 * 
 * @param bram pointer to the block memory
 * 
 * @return read_addr == write_addr
 */
static bool check_same_addrs(block_memory_t* bram) {
    int read_addr_width = bram->read_addr->count;
    int write_addr_width = bram->write_addr->count;

    /* first check is the width, if they have not the equal width return false */
    if (read_addr_width != write_addr_width) {
        return (false);
    }
    /* if they have the equal width, here is to check their driver */
    else
        return (sigcmp(bram->read_addr, bram->write_addr));
}

/**
 * (function: ymem_to_rom)
 * 
 * @brief map ymem to rom
 * 
 * @param node pointing to a bram node
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 */
static nnode_t* ymem_to_rom(nnode_t* node, uintptr_t traverse_mark_number) {
    oassert(node->traverse_visited == traverse_mark_number);

    int i;
    int offset, new_offset = 0;
    int addr_width = node->attributes->ABITS;
    int data_width = node->attributes->DBITS;
    int num_rd_ports = node->attributes->RD_PORTS;

    int RD_ADDR_width = node->input_port_sizes[0];
    int RD_CLK_width = node->input_port_sizes[1];
    int RD_DATA_width = node->output_port_sizes[0];
    int RD_ENABLE_width = node->input_port_sizes[2];

    /* check for BRAM */
    oassert(node->num_input_port_sizes == 3);
    oassert(node->num_output_port_sizes == 1);

    /* create BRAM node */
    nnode_t* transformed_mem = allocate_nnode(node->loc);
    transformed_mem->traverse_visited = traverse_mark_number;
    transformed_mem->type = ROM;
    copy_attribute(transformed_mem->attributes, node->attributes);
    transformed_mem->name = node_name(transformed_mem, node->name);
    transformed_mem->related_ast_node = node->related_ast_node;
    /** 
     * ROM information 
     * 
     * RD_ADDR:    input port [0]  ==> CLK:        input port [0] 
     * RD_CLK:     input port [1]  ==> RD_ADDR:    input port [1] 
     * RD_DATA:    output port[0]  ==> RD_DATA:    output port [0] 
     * RD_ENABLE:  input port [2]  ==> RD_ENABLE:  input port [2] 
     */

    /* CLK */
    offset = RD_ADDR_width;
    add_input_port_information(transformed_mem, 1);
    allocate_more_input_pins(transformed_mem, 1);
    for (i = 0; i < RD_CLK_width; i++) {
        if (i == 0) {
            remap_pin_to_new_node(node->input_pins[i + offset], transformed_mem, 0);
        } else {
            /* delete extra pins */
            delete_npin(node->input_pins[i + offset]);
        }
    }
    new_offset += 1;

    /* RD_ADDR */
    offset = 0;
    oassert(RD_ADDR_width == num_rd_ports * addr_width);
    add_input_port_information(transformed_mem, RD_ADDR_width);
    allocate_more_input_pins(transformed_mem, RD_ADDR_width);
    for (i = 0; i < RD_ADDR_width; i++) {
        remap_pin_to_new_node(node->input_pins[i + offset], transformed_mem, i + new_offset);
    }
    new_offset += RD_ADDR_width;

    /* RD_ENABLE */
    offset = RD_ADDR_width + RD_CLK_width;
    oassert(RD_ENABLE_width == num_rd_ports);
    add_input_port_information(transformed_mem, RD_ENABLE_width);
    allocate_more_input_pins(transformed_mem, RD_ENABLE_width);
    for (i = 0; i < RD_ENABLE_width; i++) {
        remap_pin_to_new_node(node->input_pins[i + offset], transformed_mem, i + new_offset);
    }
    new_offset += RD_ENABLE_width;

    /* RD_DATA */
    offset = 0;
    oassert(RD_DATA_width == num_rd_ports * data_width);
    add_output_port_information(transformed_mem, RD_DATA_width);
    allocate_more_output_pins(transformed_mem, RD_DATA_width);
    for (i = 0; i < RD_DATA_width; i++) {
        remap_pin_to_new_node(node->output_pins[i + offset], transformed_mem, i);
    }

    // CLEAN UP
    free_nnode(node);

    return (transformed_mem);
}

/**
 * (function: ymem_to_bram)
 * 
 * @brief map ymem to bram
 * 
 * @param node pointing to a bram node
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 */
static nnode_t* ymem_to_bram(nnode_t* node, uintptr_t traverse_mark_number) {
    oassert(node->traverse_visited == traverse_mark_number);

    int i;
    int offset, new_offset = 0;
    int addr_width = node->attributes->ABITS;
    int data_width = node->attributes->DBITS;
    int num_rd_ports = node->attributes->RD_PORTS;
    int num_wr_ports = node->attributes->WR_PORTS;

    int RD_ADDR_width = node->input_port_sizes[0];
    int RD_CLK_width = node->input_port_sizes[1];
    int RD_DATA_width = node->output_port_sizes[0];
    int RD_ENABLE_width = node->input_port_sizes[2];
    int WR_ADDR_width = node->input_port_sizes[3];
    int WR_CLK_width = node->input_port_sizes[4];
    int WR_DATA_width = node->input_port_sizes[5];
    int WR_ENABLE_width = node->input_port_sizes[6];

    /* check for BRAM */
    oassert(node->num_input_port_sizes == 7);
    oassert(node->num_output_port_sizes == 1);

    /* create BRAM node */
    nnode_t* transformed_mem = allocate_nnode(node->loc);
    transformed_mem->traverse_visited = traverse_mark_number;
    transformed_mem->type = BRAM;
    copy_attribute(transformed_mem->attributes, node->attributes);
    transformed_mem->name = node_name(transformed_mem, node->name);
    transformed_mem->related_ast_node = node->related_ast_node;
    /** 
     * BRAM information 
     * 
     * RD_ADDR:    input port [0]  ==> CLK:        input port [0] 
     * RD_CLK:     input port [1]  ==> RD_ADDR:    input port [1]   
     *   RD_DATA:    output port[0]  ==> RD_DATA:    output port[0]   
     * RD_ENABLE:  input port [2]  ==> RD_ENABLE:  input port [2]   
     * WR_ADDR:    input port [3]  ==> WR_ADDR:    input port [3]   
     * WR_CLK:     input port [4]  ==> WR_DATA:    input port [4]   
     * WR_DATA:    input port [5]  ==> WR_ENABLE:  input port [5]   
     * WR_ENABLE:  input port [6] 
     */

    /* CLK */
    offset = RD_ADDR_width + RD_CLK_width + RD_ENABLE_width + WR_ADDR_width;
    add_input_port_information(transformed_mem, 1);
    allocate_more_input_pins(transformed_mem, 1);
    for (i = 0; i < WR_CLK_width; i++) {
        if (i == 0) {
            remap_pin_to_new_node(node->input_pins[i + offset], transformed_mem, 0);
        } else {
            /* delete extra pins */
            delete_npin(node->input_pins[i + offset]);
        }
    }
    new_offset += 1;

    /* RD_ADDR */
    offset = 0;
    oassert(RD_ADDR_width == num_rd_ports * addr_width);
    add_input_port_information(transformed_mem, RD_ADDR_width);
    allocate_more_input_pins(transformed_mem, RD_ADDR_width);
    for (i = 0; i < RD_ADDR_width; i++) {
        remap_pin_to_new_node(node->input_pins[i + offset], transformed_mem, i + new_offset);
    }
    new_offset += RD_ADDR_width;

    /* RD_CLK */
    offset = RD_ADDR_width;
    oassert(RD_CLK_width == num_rd_ports);
    for (i = 0; i < RD_CLK_width; i++) {
        delete_npin(node->input_pins[i + offset]);
    }

    /* RD_ENABLE */
    offset = RD_ADDR_width + RD_CLK_width;
    oassert(RD_ENABLE_width == num_rd_ports);
    add_input_port_information(transformed_mem, RD_ENABLE_width);
    allocate_more_input_pins(transformed_mem, RD_ENABLE_width);
    for (i = 0; i < RD_ENABLE_width; i++) {
        remap_pin_to_new_node(node->input_pins[i + offset], transformed_mem, i + new_offset);
    }
    new_offset += RD_ENABLE_width;

    /* WR_ADDR */
    offset = RD_ADDR_width + RD_CLK_width + RD_ENABLE_width;
    oassert(WR_ADDR_width == num_wr_ports * addr_width);
    add_input_port_information(transformed_mem, WR_ADDR_width);
    allocate_more_input_pins(transformed_mem, WR_ADDR_width);
    for (i = 0; i < WR_ADDR_width; i++) {
        remap_pin_to_new_node(node->input_pins[i + offset], transformed_mem, i + new_offset);
    }
    new_offset += WR_ADDR_width;

    /* WR_DATA */
    offset = RD_ADDR_width + RD_CLK_width + RD_ENABLE_width + WR_ADDR_width + WR_CLK_width;
    oassert(WR_DATA_width == num_wr_ports * data_width);
    add_input_port_information(transformed_mem, WR_DATA_width);
    allocate_more_input_pins(transformed_mem, WR_DATA_width);
    for (i = 0; i < WR_DATA_width; i++) {
        remap_pin_to_new_node(node->input_pins[i + offset], transformed_mem, i + new_offset);
    }
    new_offset += WR_DATA_width;

    /* WR_ENABLE */
    offset = RD_ADDR_width + RD_CLK_width + RD_ENABLE_width + WR_ADDR_width + WR_CLK_width + WR_DATA_width;
    oassert(WR_ENABLE_width == num_wr_ports * data_width);
    add_input_port_information(transformed_mem, num_wr_ports);
    allocate_more_input_pins(transformed_mem, num_wr_ports);
    for (i = 0; i < WR_ENABLE_width; i++) {
        if (i % data_width == 0)
            remap_pin_to_new_node(node->input_pins[i + offset], transformed_mem, new_offset++);
        else
            delete_npin(node->input_pins[i + offset]);
    }

    /* RD_DATA */
    offset = 0;
    oassert(RD_DATA_width == num_rd_ports * data_width);
    add_output_port_information(transformed_mem, RD_DATA_width);
    allocate_more_output_pins(transformed_mem, RD_DATA_width);
    for (i = 0; i < RD_DATA_width; i++) {
        remap_pin_to_new_node(node->output_pins[i + offset], transformed_mem, i);
    }

    // CLEAN UP
    free_nnode(node);

    return (transformed_mem);
}

/**
 * (function: perform_optimization)
 * 
 * @brief this function is to perform optimization on block 
 * memories and roms. Optimization includes address width 
 * reduction base on the mem size
 * 
 * @param bram pointer to the block memory
 */
static void perform_optimization(block_memory_t* memory) {
    nnode_t* node = memory->node;
    int depth = node->attributes->size;
    int addr_width = node->attributes->ABITS;
    int rd_ports = node->attributes->RD_PORTS;
    int wr_ports = node->attributes->WR_PORTS;

    int needed_addr_width = 0;
    long shifted_value = 0;
    /* calculate the needed address width */
    while (shifted_value < depth) {
        needed_addr_width++;
        shifted_value = shift_left_value_with_overflow_check(0X1, needed_addr_width, node->loc);
    }

    /**
     * [NOTE]: At this point there is a need to take care of multiple read 
     * or write address pins (like dual port ram) since in the case of having
     * arithmetic operation in address pins (cause to extend addr to 32 bits), 
     * yosys would NOT handle it before creting the mem subcircuit.
     * e.g.:
     * 
     *      wire [3:0] read_addr;
     *      read_data <= mem[read_addr / 3] 
     *  
     * the signal representing the read addr signal in $mem subcircuit is 32 bits. 
     * However, we only need four bits
     */

    /* check if block memory has a read addr */
    if (memory->read_addr) {
        /* prune read address to reduce its width based on the needed addr width if needed */
        memory->read_addr = prune_signal(memory->read_addr, addr_width, needed_addr_width, rd_ports);
    }

    /* check if block memory has a write addr */
    if (memory->write_addr) {
        /* prune write address to reduce its width based on the needed addr width if needed */
        memory->write_addr = prune_signal(memory->write_addr, addr_width, needed_addr_width, wr_ports);
    }

    /* update new read addr width */
    node->attributes->ABITS = needed_addr_width;
}

/**
 * (function: split_cascade_port)
 * 
 * @brief split the given signal list into chunks of desired_width size. 
 * Then, cascade them with selectors pin. In this function, assumed that
 * the order of selectors is matched to the order of signals
 * 
 * @param signalvar list of signals (like write data)
 * @param selectors list of selectors (like write enables)
 * @param desired_width final output width
 * @param node pointer to the corresponding node
 * @param netlist pointer to the current netlist
 * 
 * @return last item outputs in the chain of cascaded signals
 */
static signal_list_t* split_cascade_port(signal_list_t* signalvar, signal_list_t* selectors, int desired_width, nnode_t* node, netlist_t* netlist) {
    /* check if cascade is needed */
    if (signalvar->count == desired_width) {
        return (signalvar);
    }

    /* validate signals list size */
    oassert(signalvar->count % desired_width == 0);

    int i, j;
    int num_chunk = signalvar->count / desired_width;
    signal_list_t* return_value = NULL;
    /* validate selector size */
    oassert(selectors->count == num_chunk);

    /* initialize splitted signals */
    signal_list_t** splitted_signals = split_signal_list(signalvar, desired_width);

    /* create cascaded multiplexers */
    nnode_t** muxes = (nnode_t**)vtr::calloc(num_chunk, sizeof(nnode_t*));
    signal_list_t** internal_outputs = (signal_list_t**)vtr::calloc(num_chunk, sizeof(signal_list_t*));
    for (i = 0; i < num_chunk; ++i) {
        /* mux inputs */
        signal_list_t** mux_inputs = (signal_list_t**)vtr::calloc(2, sizeof(signal_list_t*));
        mux_inputs[0] = init_signal_list();
        if (i == 0) {
            /* the first port of the first mux should be driven by PAD node */
            for (j = 0; j < desired_width; j++) {
                add_pin_to_signal_list(mux_inputs[0], get_pad_pin(netlist));
            }
        } else {
            /* the first port of the rest muxe should be driven by previous mux output */
            for (j = 0; j < desired_width; j++) {
                add_pin_to_signal_list(mux_inputs[0], internal_outputs[i - 1]->pins[j]);
            }
        }

        /* hook the splitted signals[i] as the second mux input */
        mux_inputs[1] = init_signal_list();
        for (j = 0; j < desired_width; j++) {
            add_pin_to_signal_list(mux_inputs[1], splitted_signals[i]->pins[j]);
        }

        /* handle mux selector and create the multiplexer */
        {
            /* create a signal list for selector pin */
            signal_list_t* selector_i = init_signal_list();
            add_pin_to_signal_list(selector_i, selectors->pins[i]);
            /* a regular multiplexer instatiation */
            muxes[i] = make_multiport_smux(mux_inputs,
                                           selector_i,
                                           2,
                                           NULL,
                                           node,
                                           netlist);

            // CLEAN UP
            free_signal_list(selector_i);
        }

        /* initialize the internal outputs */
        internal_outputs[i] = init_signal_list();
        for (j = 0; j < desired_width; j++) {
            npin_t* output_pin = muxes[i]->output_pins[j];
            nnet_t* output_net = output_pin->net;
            /* add new fanout */
            npin_t* new_pin = allocate_npin();
            add_fanout_pin_to_net(output_net, new_pin);
            /* keep the record of the new pin as internal outputs */
            add_pin_to_signal_list(internal_outputs[i], new_pin);
        }

        // CLEAN UP
        free_signal_list(mux_inputs[0]);
        free_signal_list(mux_inputs[1]);
        vtr::free(mux_inputs);
    }

    return_value = internal_outputs[num_chunk - 1];

    // CLEAN UP
    for (i = 0; i < num_chunk; ++i) {
        free_signal_list(splitted_signals[i]);
    }
    vtr::free(splitted_signals);

    /* free internal output signal list expect the last one since it is the return value */
    for (i = 0; i < num_chunk - 1; ++i) {
        free_signal_list(internal_outputs[i]);
    }
    vtr::free(internal_outputs);
    vtr::free(muxes);

    return (return_value);
}

/**
 * (function: decode_out_port)
 * 
 * @brief decode the memory outputs to the n output ports
 * 
 * @param src the mux input that will pass if en is 1
 * @param outs list of signals (like write data)
 * @param selectors list of selectors (like write enables)
 * @param node pointer to the corresponding node
 * @param netlist pointer to the current netlist
 */
static void decode_out_port(signal_list_t* src, signal_list_t* outs, signal_list_t* selectors, nnode_t* node, netlist_t* netlist) {
    int width = src->count;
    /* validate signals list size */
    oassert(width != 0);
    oassert(outs->count % width == 0);

    int i, j;
    int num_chunk = outs->count / width;

    /* initialize splitted signals */
    signal_list_t** splitted_signals = split_signal_list(outs, width);

    /* validate selector size */
    oassert(selectors->count == num_chunk);

    /* adding fanout pins to src pin nets */
    signal_list_t** src_nets_fanouts = (signal_list_t**)vtr::calloc(width, sizeof(signal_list_t*));
    /* create the n fanout pin for src pins, since they are output pins of a memory */
    for (i = 0; i < width; ++i) {
        npin_t* src_pin = src->pins[i];
        /* validate that it is output */
        oassert(src_pin->type == OUTPUT);

        /* init the related sig list */
        src_nets_fanouts[i] = init_signal_list();
        /* add fanouts */
        for (j = 0; j < num_chunk; j++) {
            npin_t* new_pin = allocate_npin();
            /* adding fanout pin to the src_pin net */
            add_fanout_pin_to_net(src_pin->net, new_pin);
            /* keep the record of the newly added pin */
            add_pin_to_signal_list(src_nets_fanouts[i], new_pin);
        }
    }

    /* create multiplexers */
    nnode_t** muxes = (nnode_t**)vtr::calloc(num_chunk, sizeof(nnode_t*));
    for (i = 0; i < num_chunk; ++i) {
        /* mux inputs */
        signal_list_t** mux_inputs = (signal_list_t**)vtr::calloc(2, sizeof(signal_list_t*));
        mux_inputs[0] = init_signal_list();
        /* the first port of the first mux should be driven by PAD node */
        for (j = 0; j < width; j++) {
            add_pin_to_signal_list(mux_inputs[0], get_pad_pin(netlist));
        }

        /* hook the splitted signals[i] as the second mux input */
        mux_inputs[1] = init_signal_list();
        for (j = 0; j < width; j++) {
            add_pin_to_signal_list(mux_inputs[1], src_nets_fanouts[j]->pins[i]);
        }

        /* handle mux selector and create the multiplexer */
        {
            /* create a signal list for selector pin */
            signal_list_t* selector_i = init_signal_list();
            add_pin_to_signal_list(selector_i, selectors->pins[i]);
            /* a regular multiplexer instatiation */
            muxes[i] = make_multiport_smux(mux_inputs,
                                           selector_i,
                                           2,
                                           splitted_signals[i],
                                           node,
                                           netlist);

            // CLEAN UP
            free_signal_list(selector_i);
        }

        // CLEAN UP
        free_signal_list(mux_inputs[0]);
        free_signal_list(mux_inputs[1]);
        vtr::free(mux_inputs);
    }

    // CLEAN UP
    for (i = 0; i < num_chunk; ++i) {
        free_signal_list(splitted_signals[i]);
    }
    vtr::free(splitted_signals);
    for (i = 0; i < width; ++i) {
        free_signal_list(src_nets_fanouts[i]);
    }
    vtr::free(src_nets_fanouts);
    vtr::free(muxes);
}

/**
 * (function: cleanup_block_memory_old_node)
 * 
 * @brief Frees memory used for indexing block memories.
 * 
 * @param node pointer to the old node
 */
static void cleanup_block_memory_old_node(nnode_t* old_node) {
    int i;
    for (i = 0; i < old_node->num_input_pins; ++i) {
        npin_t* pin = old_node->input_pins[i];

        if (pin)
            old_node->input_pins[i] = NULL;
    }

    for (i = 0; i < old_node->num_output_pins; ++i) {
        npin_t* pin = old_node->output_pins[i];

        if (pin)
            old_node->output_pins[i] = NULL;
    }

    /* clean up */
    free_nnode(old_node);
}

/**
 * (function: free_block_memory_indices)
 * 
 * @brief Frees memory used for indexing block memories.
 */
void free_block_memories() {
    /* check if any block memory indexed */
    if (block_memories_info.block_memory_list) {
        free_block_memory_index(block_memories_info.block_memories);
        while (block_memories_info.block_memory_list != NULL)
            block_memories_info.block_memory_list = delete_in_vptr_list(block_memories_info.block_memory_list);
    }
    /* check if any read only memory indexed */
    if (block_memories_info.read_only_memory_list) {
        free_block_memory_index(block_memories_info.read_only_memories);
        while (block_memories_info.read_only_memory_list != NULL)
            block_memories_info.read_only_memory_list = delete_in_vptr_list(block_memories_info.read_only_memory_list);
    }
}

/**
 * (function: free_block_memory_index)
 * 
 * @brief Frees memory used for indexing block memories. Finalises each
 * memory, making sure it has the right ports, and collapsing
 * the memory if possible.
 * 
 * @param to_free to be freed block memory hashtable
 */
void free_block_memory_index(block_memory_hashtable to_free) {
    if (!to_free.empty()) {
        for (auto mem_it : to_free) {
            free_block_memory(mem_it.second);
        }
    }
    to_free.clear();
}

/**
 * (function: free_block_memory_index_and_finalize_memories)
 * 
 * @brief Frees memory used for indexing block memories. Finalises each
 * memory, making sure it has the right ports, and collapsing
 * the memory if possible.
 * 
 * @param to_free to be freed block memory structure
 */
static void free_block_memory(block_memory_t* to_free) {
    free_signal_list(to_free->read_addr);
    free_signal_list(to_free->read_data);
    free_signal_list(to_free->read_en);
    free_signal_list(to_free->write_addr);
    free_signal_list(to_free->write_data);
    free_signal_list(to_free->write_en);
    free_signal_list(to_free->clk);

    vtr::free(to_free->name);
    vtr::free(to_free->memory_id);

    vtr::free(to_free);
}
