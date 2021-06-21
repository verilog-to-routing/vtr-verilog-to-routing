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

#include <string.h>
#include <math.h>
#include "odin_globals.h"
#include "odin_types.h"
#include "odin_util.h"

#include "netlist_utils.h"
#include "node_creation_library.h"
#include "hard_blocks.h"
#include "memories.h"
#include "block_memories.hh"
#include "partial_map.h"
#include "vtr_util.h"
#include "vtr_memory.h"

using vtr::t_linked_vptr;

t_linked_vptr* block_memory_list;
t_linked_vptr* read_only_memory_list;
block_memory_hashtable block_memories;
block_memory_hashtable read_only_memories;

static void add_input_port_to_block_memory(block_memory* memory, signal_list_t* signals, const char* port_name);
static void add_output_port_to_block_memory(block_memory* memory, signal_list_t* signals, const char* port_name);
static void remap_input_port_to_block_memory(block_memory* memory, signal_list_t* signals, const char* port_name);
static void remap_output_port_to_block_memory(block_memory* memory, signal_list_t* signals, const char* port_name);

static block_memory* init_block_memory(nnode_t* node, uintptr_t traverse_mark_number);

static block_memory* create_block_memory(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
static block_memory* create_rom_block_memory(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);

static void register_block_memory_in_arch_model(block_memory* bram);

static signal_list_t* merge_read_write_clks (signal_list_t* rd_clks, signal_list_t* wr_clks, nnode_t* node, netlist_t* netlist);

static void free_block_memory_index(block_memory_hashtable to_free);
static void free_block_memory(block_memory* to_free);


/**
 * (function: init_block_memory_index)
 * 
 * @brief Initialises hashtables to lookup memories based on inputs and names.
 */
void init_block_memory_index() {
    block_memories = block_memory_hashtable();
    read_only_memories = block_memory_hashtable();
}

/**
 * (function: lookup_block_memory)
 * 
 * @brief Looks up an block memory by identifier name in the block memory lookup table.
 * 
 * @param instance_name_prefix memory node name (unique hard block node name)
 * @param identifier memory block id
 */
block_memory* lookup_block_memory(char* instance_name_prefix, char* identifier) {
    char* memory_string = make_full_ref_name(instance_name_prefix, NULL, NULL, identifier, -1);

    block_memory_hashtable::const_iterator mem_out = block_memories.find(std::string(memory_string));

    vtr::free(memory_string);

    if (mem_out == block_memories.end())
        return (NULL);
    else
        return (mem_out->second);
}


/**
 * (function: add_input_port_to_block_memory)
 * 
 * @brief Add an input port to the given block memory.
 * 
 * @param memory pointer to the BRAM node
 * @param signals list of signals to be added as input
 * @param port_name the name of the port to which signals are mapping
 */
static void add_input_port_to_block_memory(block_memory* memory, signal_list_t* signals, const char* port_name) {
    nnode_t* node = memory->node;
    /* remap to the netlist node */
    add_input_port_to_memory(node, signals, port_name);
}

/**
 * (function: add_output_port_to_block_memory)
 * 
 * @brief Add an output port to the given block memory.
 * 
 * @param memory pointer to the BRAM node
 * @param signals list of signals to be added as output
 * @param port_name the name of the port to which signals are mapping
 */
static void add_output_port_to_block_memory(block_memory* memory, signal_list_t* signals, const char* port_name) {
    nnode_t* node = memory->node;
    /* remap to the netlist node */
    add_output_port_to_memory(node, signals, port_name);
}

/**
 * (function: remap_input_port_to_block_memory)
 * 
 * @brief Remap an input port to the given block memory.
 * 
 * @param memory pointer to the BRAM node
 * @param signals list of signals to be remapped as input
 * @param port_name the name of the port to which signals are mapping
 */
static void remap_input_port_to_block_memory(block_memory* memory, signal_list_t* signals, const char* port_name) {
    nnode_t* node = memory->node;
    /* remap to the netlist node */
    remap_input_port_to_memory(node, signals, port_name);
}

/**
 * (function: add_input_port_to_block_memory)
 * 
 * @brief Remap an output port to the given block memory.
 * 
 * @param memory pointer to the BRAM node
 * @param signals list of signals to be remapped as output
 * @param port_name the name of the port to which signals are mapping
 */
static void remap_output_port_to_block_memory(block_memory* memory, signal_list_t* signals, const char* port_name) {
    nnode_t* node = memory->node;
    /* remap to the netlist node */
    remap_output_port_to_memory(node, signals, port_name);
}


/**
 * (function: init_block_memory)
 * 
 * @brief: Remap an output port to the given block memory.
 * 
 * @param node pointing to a spram node
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
static block_memory* init_block_memory(nnode_t* node, uintptr_t traverse_mark_number) {

    block_memory* bram = (block_memory*)vtr::malloc(sizeof(block_memory));

    /** 
     * yosys memory block information 
     * 
     * RD_ADDR:    input port [0] 
     * RD_CLOCK:   input port [1] 
     * RD_DATA:    output port [0] 
     * RD_ENABLE:  input port [2] 
     * WR_ADDR:    input port [3] 
     * WR_CLOCK:   input port [4] 
     * WR_DATA:    input port [5] 
     * WR_ENABLE:  input port [6] 
    */
    bram->read_addr_width  = node->input_port_sizes[0];
    bram->read_clk_width   = node->input_port_sizes[1];
    bram->read_data_width  = node->output_port_sizes[0];
    bram->read_en_width    = node->input_port_sizes[2];

    bram->write_addr_width = node->input_port_sizes[3];
    bram->write_clk_width  = node->input_port_sizes[4];
    bram->write_data_width = node->input_port_sizes[5];
    bram->write_en_width   = node->input_port_sizes[6];

    /* creating new node since we need to reorder some input port for each inferenece mode */
    bram->node = allocate_nnode(node->loc);
    bram->node->type = MEMORY;
    bram->node->traverse_visited = traverse_mark_number;

    char* hb_name = vtr::strdup(DUAL_PORT_RAM_string);
    bram->node->name = make_full_ref_name(hb_name, NULL, NULL, NULL, -1);

    /* some information from ast node is needed in partial mapping */
    bram->node->related_ast_node = node->related_ast_node;
    char* hard_block_identifier = bram->node->related_ast_node->identifier_node->types.identifier;
    /* setting dual port ram as corresponding memory hard block */
    if (hard_block_identifier)
        vtr::free(hard_block_identifier);

    bram->node->related_ast_node->identifier_node->types.identifier = hb_name;

    bram->memory_id = vtr::strdup(node->attributes->memory_id);
    bram->node->attributes->memory_id = vtr::strdup(node->attributes->memory_id);

    /* creating a unique name for the block memory */
    bram->name = make_full_ref_name(bram->node->name, NULL, NULL, bram->memory_id, -1);
    block_memories.emplace(bram->name, bram);

    return(bram);
}  

/**
 * (function: init_block_memory)
 * 
 * @brief: Remap an output port to the given block memory.
 * 
 * @param node pointing to a spram node
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
static block_memory* init_read_only_memory(nnode_t* node, uintptr_t traverse_mark_number) {

    block_memory* rom = (block_memory*)vtr::malloc(sizeof(block_memory));

    /** 
     * yosys single port ram information 
     * 
     * RD_ADDR:    input port [0] 
     * RD_CLOCK:   input port [1] 
     * RD_DATA:    output port [0] 
     * RD_ENABLE:  input port [2] 
    */

    rom->read_addr_width  = node->input_port_sizes[0];
    rom->read_clk_width   = node->input_port_sizes[1];
    rom->read_data_width  = node->output_port_sizes[0];
    rom->read_en_width    = node->input_port_sizes[2];

    rom->write_addr_width = -1;
    rom->write_clk_width  = -1;
    rom->write_data_width = node->output_port_sizes[0]; // will be driven by pad node
    rom->write_en_width   = -1;

    /* creating new node since we need to reorder some input port for each inferenece mode */
    rom->node = allocate_nnode(node->loc);
    rom->node->type = MEMORY;
    rom->node->traverse_visited = traverse_mark_number;

    char* hb_name = vtr::strdup(SINGLE_PORT_RAM_string);
    rom->node->name = make_full_ref_name(hb_name, NULL, NULL, NULL, -1);

    /* some information from ast node is needed in partial mapping */
    rom->node->related_ast_node = node->related_ast_node;
    char* hard_block_identifier = rom->node->related_ast_node->identifier_node->types.identifier;
    /* setting dual port ram as corresponding memory hard block */
    if (hard_block_identifier)
        vtr::free(hard_block_identifier);

    rom->node->related_ast_node->identifier_node->types.identifier = hb_name;

    rom->memory_id = vtr::strdup(node->attributes->memory_id);
    rom->node->attributes->memory_id = vtr::strdup(node->attributes->memory_id);

    /* creating a unique name for the block memory */
    rom->name = make_full_ref_name(rom->node->name, NULL, NULL, rom->memory_id, -1);
    read_only_memories.emplace(rom->name, rom);

    return(rom);
}  

/**
 * (function: create_block_memory)
 * 
 * @brief block_ram will be considered as dual port ram
 * this function reorders inputs and hook pad pins into datain2 
 * it also leaves the second output of the dual port ram unconnected
 * 
 * @param node pointing to a bram node
 * @param traverse_mark_number unique traversal mark for blif elaboration pass
 * @param netlist pointer to the current netlist file
 */
static block_memory* create_block_memory(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes == 7);
    oassert(node->num_output_port_sizes == 1);

    int i, j;
    int offset = 0;

    /* initializing a new block ram */
    block_memory* bram = init_block_memory(node, traverse_mark_number);

    /* INPUTS */
    /* adding the read addr input port as address1 */
    signal_list_t* read_addr_signals = init_signal_list();
    for (i = 0; i < bram->read_addr_width; i++) {
        add_pin_to_signal_list(read_addr_signals, node->input_pins[offset + i]);
    }
    remap_input_port_to_block_memory(bram, read_addr_signals, "addr1");
    free_signal_list(read_addr_signals);


    /* adding the write addr port as address2 */
    signal_list_t* write_addr_signals = init_signal_list();
    offset = bram->read_addr_width + bram->read_clk_width + bram->read_en_width;

    for (i = 0; i < bram->write_addr_width; i++) {
        add_pin_to_signal_list(write_addr_signals, node->input_pins[offset + i]);
    }
    remap_input_port_to_block_memory(bram, write_addr_signals, "addr2");
    free_signal_list(write_addr_signals);


    /* handling multiple clock signals */
    signal_list_t* rd_clk_signals = init_signal_list();
    offset = bram->read_addr_width;
    for (i = 0; i < bram->read_clk_width; i++) {
        add_pin_to_signal_list(rd_clk_signals, node->input_pins[offset + i]);
    }
    signal_list_t* wr_clk_signals = init_signal_list();
    offset = bram->read_addr_width + bram->read_clk_width + bram->read_en_width + bram->write_addr_width;
    for (i = 0; i < bram->write_clk_width; i++) {
        add_pin_to_signal_list(wr_clk_signals, node->input_pins[offset + i]);
    }

    /* merge all read and write clock to a single clock pin */
    signal_list_t* new_clk = merge_read_write_clks(rd_clk_signals, wr_clk_signals, node, netlist);
    add_input_port_to_block_memory(bram, new_clk, "clk");
    free_signal_list(new_clk);


    /* adding the write data port as data1 */
    signal_list_t* wr_data_signals = init_signal_list();
    offset = bram->read_addr_width + bram->read_clk_width + bram->read_en_width + bram->write_addr_width + bram->write_clk_width;
    for (i = 0; i < bram->write_data_width; i++) {
        add_pin_to_signal_list(wr_data_signals, node->input_pins[offset + i]);
    }
    remap_input_port_to_block_memory(bram, wr_data_signals, "data1");
    free_signal_list(wr_data_signals);


    /* we pad the second data port using pad pins */
    signal_list_t* pad_signals = init_signal_list();
    for (i = 0; i < bram->write_data_width; i++) {
        add_pin_to_signal_list(pad_signals, get_pad_pin(netlist));
    }
    add_input_port_to_block_memory(bram, pad_signals, "data2");
    free_signal_list(pad_signals);


    /* add read enable port as we1 */
    signal_list_t* rd_en_signals = init_signal_list();
    offset = bram->read_addr_width + bram->read_clk_width;
    if (bram->read_en_width == 1) {
        add_pin_to_signal_list(rd_en_signals, node->input_pins[offset]);
        /* hook read enable signals into bram node */
        remap_input_port_to_block_memory(bram, rd_en_signals, "we1");

    } else if (bram->read_en_width > 1) {
        /* need to OR all read enable since we1 should be one bit in dual port ram */
        for (j = 0; j < bram->read_en_width; j++) {
            add_pin_to_signal_list(rd_en_signals, node->input_pins[offset + j]);
        }
        /* OR enable signals */
        rd_en_signals = make_or_chain(rd_en_signals, bram->node);
        add_input_port_to_block_memory(bram, rd_en_signals, "we1");
    }
    free_signal_list(rd_en_signals);


    /* add write enable port as we1 */
    signal_list_t* wr_en_signals = init_signal_list();
    offset = bram->read_addr_width + bram->read_clk_width + bram->read_en_width + bram->write_addr_width + bram->write_clk_width + bram->write_data_width;
    if (bram->write_en_width == 1) {
        add_pin_to_signal_list(wr_en_signals, node->input_pins[offset]);

         /* hook write enable signals into bram node */
        remap_input_port_to_block_memory(bram, wr_en_signals, "we2");

    } else if (bram->write_en_width > 1) {
        /* need to OR all write enable since we2 should be one bit in dual port ram */
        for (j = 0; j < bram->write_en_width; j++) {
            add_pin_to_signal_list(wr_en_signals, node->input_pins[offset + j]);
        }
        /* OR enable signals */
        wr_en_signals = make_or_chain(wr_en_signals, bram->node);
        add_input_port_to_block_memory(bram, wr_en_signals, "we2");
    }
    free_signal_list(wr_en_signals);



    /* OUTPUT */
    /* adding the read data port as output1 */
    offset = 0;
    signal_list_t* read_data_signals = init_signal_list();
    for (i = 0; i < bram->read_data_width; i++) {
        add_pin_to_signal_list(read_data_signals, node->output_pins[i]);
    }
    remap_output_port_to_block_memory(bram, read_data_signals, "out1");
    free_signal_list(read_data_signals);

    /* leave second output port unconnected */
    offset = bram->read_data_width;
    signal_list_t* out2_signals = init_signal_list();
    for (i = 0; i < bram->read_data_width; i++) {
        // specify the output pin
        npin_t* new_pin1 = allocate_npin();
        npin_t* new_pin2 = allocate_npin();
        nnet_t* new_net = allocate_nnet();
        new_net->name = make_full_ref_name(NULL, NULL, NULL, bram->node->name, offset + i);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net, new_pin1);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net, new_pin2);

        /* adding to signal list */
        add_pin_to_signal_list(out2_signals, new_pin1);
    }
    add_output_port_to_block_memory(bram, out2_signals, "out2");
    free_signal_list(out2_signals);

    // CLEAN UP
    free_nnode(node);

    return (bram);
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
static block_memory* create_rom_block_memory(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist) {
    oassert(node->traverse_visited == traverse_mark_number);
    oassert(node->num_input_port_sizes == 3);


    int i, j;
    int offset = 0;

    /* initializing a new read only memory */
    block_memory* rom = init_read_only_memory(node, traverse_mark_number);

    /* INPUTS */
    /* adding the read addr input port as address1 */
    signal_list_t* read_addr_signals = init_signal_list();
    for (i = 0; i < rom->read_addr_width; i++) {
        add_pin_to_signal_list(read_addr_signals, node->input_pins[offset + i]);
    }
    remap_input_port_to_block_memory(rom, read_addr_signals, "addr");
    free_signal_list(read_addr_signals);

    /* handling multiple clock signals */
    signal_list_t* read_clk_signals = init_signal_list();
    offset = rom->read_addr_width;
    if (rom->read_clk_width == 1) {
        add_pin_to_signal_list(read_clk_signals, node->input_pins[offset]);
        /* hook read clk signals into rom node */
        remap_input_port_to_block_memory(rom, read_clk_signals, "clk");

    } else if (rom->read_clk_width > 1) {
        /* need to OR all read clk since clk should be one bit in signle port ram */
        for (j = 0; j < rom->read_clk_width; j++) {
            add_pin_to_signal_list(read_clk_signals, node->input_pins[offset + j]);
        }
        /* OR clk signals */
        read_clk_signals = make_or_chain(read_clk_signals, rom->node);
        add_input_port_to_block_memory(rom, read_clk_signals, "clk");
    }
    free_signal_list(read_clk_signals);


    /* we pad the data port using pad pins */
    signal_list_t* pad_signals = init_signal_list();
    for (i = 0; i < rom->write_data_width; i++) {
        add_pin_to_signal_list(pad_signals, get_pad_pin(netlist));
    }
    add_input_port_to_block_memory(rom, pad_signals, "data");
    free_signal_list(pad_signals);


    /* add read enable port as we1 */
    signal_list_t* rd_en_signals = init_signal_list();
    offset = rom->read_addr_width + rom->read_clk_width;
    if (rom->read_en_width == 1) {
        add_pin_to_signal_list(rd_en_signals, node->input_pins[offset]);
        /* hook read enable signals into rom node */
        remap_input_port_to_block_memory(rom, rd_en_signals, "we");

    } else if (rom->read_en_width > 1) {
        /* need to OR all read enable since we1 should be one bit in dual port ram */
        for (j = 0; j < rom->read_en_width; j++) {
            add_pin_to_signal_list(rd_en_signals, node->input_pins[offset + j]);
        }
        /* OR enable signals */
        rd_en_signals = make_or_chain(rd_en_signals, rom->node);
        add_input_port_to_block_memory(rom, rd_en_signals, "we");
    }
    free_signal_list(rd_en_signals);


    /* OUTPUT */
    /* adding the read data port as output1 */
    offset = 0;
    signal_list_t* read_data_signals = init_signal_list();
    for (i = 0; i < rom->read_data_width; i++) {
        add_pin_to_signal_list(read_data_signals, node->output_pins[i]);
    }
    remap_output_port_to_block_memory(rom, read_data_signals, "out");
    free_signal_list(read_data_signals);

    // CLEAN UP
    free_nnode(node);

    return (rom);
}


/**
 * (function: free_block_memory_index_and_finalize_memories)
 * 
 * @brief Frees memory used for indexing block memories. Finalises each
 * memory, making sure it has the right ports, and collapsing
 * the memory if possible.
 */
static void register_block_memory_in_arch_model(block_memory* bram) {
    
    nnode_t* hb_instance = bram->node;

    /* See if the hard block declared is supported by FPGA architecture */
    t_model* hb_model = find_hard_block(hb_instance->related_ast_node->identifier_node->types.identifier);

    if (hb_model) {
        /* Declare the hard block as used for the blif generation */
        hb_model->used = 1;
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
    
    /* create the BRAM and allocate ports according to the DPRAM hard block */
    block_memory* bram = create_block_memory(node, traverse_mark_number, netlist);    

    block_memory_list = insert_in_vptr_list(block_memory_list, bram);
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
    
    /* create the BRAM and allocate ports according to the DPRAM hard block */
    block_memory* rom = create_rom_block_memory(node, traverse_mark_number, netlist);    

    read_only_memory_list = insert_in_vptr_list(read_only_memory_list, rom);
}

/**
 * (function: resolve_bram_node)
 * 
 * @brief create, verify and shrink the bram node
 */
void iterate_block_memories() {

    t_linked_vptr* ptr = block_memory_list;
    while (ptr != NULL) {
        block_memory* bram = (block_memory*)ptr->data_vptr;

        /* validation */
        oassert(bram != NULL);
        oassert(bram->node->type == MEMORY);

        /* register the BRAM in arch model to have the related model (DPRAM) in BLIF for simulation */
        register_block_memory_in_arch_model(bram);
        dp_memory_list = insert_in_vptr_list(dp_memory_list, bram->node);

        ptr = ptr->next;
    }


    ptr = read_only_memory_list;
    while (ptr != NULL) {
        block_memory* rom = (block_memory*)ptr->data_vptr;

        /* validation */
        oassert(rom != NULL);
        oassert(rom->node->type == MEMORY);

        /* register the BRAM in arch model to have the related model (SPRAM) in BLIF for simulation */
        register_block_memory_in_arch_model(rom);
        sp_memory_list = insert_in_vptr_list(sp_memory_list, rom->node);

        ptr = ptr->next;
    } 
}

/**
 * (function: merge_read_write_clks)
 * 
 * @brief create a OR gate with read and write clocks as input
 * 
 * @param rd_clks signal list of read clocks
 * @param wr_clks signal list of write clocks
 * @param node pointing to a bram node
 * @param netlist pointer to the current netlist file
 * 
 * @return single clock pin
 */
static signal_list_t* merge_read_write_clks (signal_list_t* rd_clks, signal_list_t* wr_clks, nnode_t* node, netlist_t* netlist) {

    int i;
    int rd_clks_width = rd_clks->count;
    int wr_clks_width = wr_clks->count;

    /* adding the OR of rd_clk and wr_clk as clock port */
    nnode_t* nor_gate = allocate_nnode(node->loc);
    nor_gate->traverse_visited = node->traverse_visited;
    nor_gate->type = LOGICAL_NOR;
    nor_gate->name = node_name(nor_gate, node->name);

    add_input_port_information(nor_gate, rd_clks_width);
    allocate_more_input_pins(nor_gate, rd_clks_width);
    /* hook input pins */
    for (i = 0; i < rd_clks_width; i++) {
        npin_t* rd_clk = rd_clks->pins[i];
        remap_pin_to_new_node(rd_clk, nor_gate, i);
    }
    add_input_port_information(nor_gate, wr_clks_width);
    allocate_more_input_pins(nor_gate, wr_clks_width);
    /* hook input pins */
    for (i = 0; i < wr_clks_width; i++) {
        npin_t* wr_clk = wr_clks->pins[i];
        remap_pin_to_new_node(wr_clk, nor_gate, rd_clks_width + i);
    }

    /* add nor gate output information */
    add_output_port_information(nor_gate, 1);
    allocate_more_output_pins(nor_gate, 1);
    // specify the output pin
    npin_t* nor_gate_new_pin1 = allocate_npin();
    npin_t* nor_gate_new_pin2 = allocate_npin();
    nnet_t* nor_gate_new_net = allocate_nnet();
    nor_gate_new_net->name = make_full_ref_name(NULL, NULL, NULL, nor_gate->name, 0);
    /* hook the output pin into the node */
    add_output_pin_to_node(nor_gate, nor_gate_new_pin1, 0);
    /* hook up new pin 1 into the new net */
    add_driver_pin_to_net(nor_gate_new_net, nor_gate_new_pin1);
    /* hook up the new pin 2 to this new net */
    add_fanout_pin_to_net(nor_gate_new_net, nor_gate_new_pin2);
  
    /* create a clock node to specify the edge sensitivity */
    nnode_t* clk_node = make_not_gate(nor_gate, node->traverse_visited);
    connect_nodes(nor_gate, 0, clk_node, 0);//make_1port_gate(CLOCK_NODE, 1, 1, node, traverse_mark_number);
    /* create the clk node's output pin */
    npin_t* clk_node_new_pin1 = allocate_npin();
    npin_t* clk_node_new_pin2 = allocate_npin();
    nnet_t* clk_node_new_net = allocate_nnet();
    clk_node_new_net->name = make_full_ref_name(NULL, NULL, NULL, clk_node->name, 0);
    /* hook the output pin into the node */
    add_output_pin_to_node(clk_node, clk_node_new_pin1, 0);
    /* hook up new pin 1 into the new net */
    add_driver_pin_to_net(clk_node_new_net, clk_node_new_pin1);
    /* hook up the new pin 2 to this new net */
    add_fanout_pin_to_net(clk_node_new_net, clk_node_new_pin2);
    clk_node_new_pin2->mapping = vtr::strdup("clk");

    /* adding the new clk node to netlist clocks */
    netlist->clocks = (nnode_t**)vtr::realloc(netlist->clocks, sizeof(nnode_t*) * (netlist->num_clocks + 1));
    netlist->clocks[netlist->num_clocks] = clk_node;
    netlist->num_clocks++;

    signal_list_t* return_signal = init_signal_list();
    add_pin_to_signal_list(return_signal, clk_node_new_pin2);

    free_signal_list(rd_clks);
    free_signal_list(wr_clks);

    return (return_signal);
}

/**
 * (function: free_block_memory_indices)
 * 
 * @brief Frees memory used for indexing block memories.
 */
void free_block_memories() {
    /* check if any block memory indexed */
    if (block_memory_list) {
        free_block_memory_index(block_memories);
        while (block_memory_list != NULL)
            block_memory_list = delete_in_vptr_list(block_memory_list);
    }
    /* check if any read only memory indexed */
    if (read_only_memory_list) {
        free_block_memory_index(read_only_memories);
        while (read_only_memory_list != NULL)
            read_only_memory_list = delete_in_vptr_list(read_only_memory_list);
    }
}

/**
 * (function: free_block_memory_index_and_finalize_memories)
 * 
 * @brief Frees memory used for indexing block memories. Finalises each
 * memory, making sure it has the right ports, and collapsing
 * the memory if possible.
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
 */
static void free_block_memory(block_memory* to_free) {
    //finalize_block_memory(mem_it.second);
    vtr::free(to_free->name);
    vtr::free(to_free->memory_id);
    vtr::free(to_free);
}
