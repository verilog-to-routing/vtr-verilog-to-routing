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

#ifndef GLOBALS_H
#define GLOBALS_H

#include "config_t.h"
#include "odin_types.h"
#include "string_cache.h"
#include "hash_table.h"
#include "read_xml_arch_file.h"
#include "hard_soft_logic_mixer.h"

/**
 * The cutoff for the number of netlist nodes. 
 * Technically, Odin-II prints statistics for 
 * netlist nodes that the total number of them
 * is greater than this value. 
 */
constexpr long long UNUSED_NODE_TYPE = 0;

extern t_logical_block_type* type_descriptors;

/* VERILOG SYNTHESIS GLOBALS */
extern ids default_net_type;

extern global_args_t global_args;
extern config_t configuration;
extern loc_t my_location;

extern ast_t* verilog_ast;
extern STRING_CACHE* module_names_to_idx;

extern STRING_CACHE* output_nets_sc;
extern STRING_CACHE* input_nets_sc;

extern netlist_t* syn_netlist;

extern nnode_t** top_input_nodes;
extern long num_top_input_nodes;
extern nnode_t** top_output_nodes;
extern long num_top_output_nodes;
extern nnode_t* gnd_node;
extern nnode_t* vcc_node;
extern nnode_t* pad_node;

extern nnet_t* zero_net;
extern nnet_t* one_net;
extern nnet_t* pad_net;
extern char* one_string;
extern char* zero_string;
extern char* pad_string;

extern t_arch Arch;
extern short physical_lut_size;

/* ACTIVATION ESTIMATION GLOBALS */
extern netlist_t* blif_netlist;

/* logic optimization mixer, once ODIN is classy, could remove that
 * and pass as member variable
 */
extern HardSoftLogicMixer* mixer;

/**
 * a global var to specify the need for cleanup after
 * receiving a coarsen BLIF file as the input.
 */
extern bool coarsen_cleanup;

extern const strbimap<file_type_e> file_extension_strmap;
extern const strbimap<file_type_e> file_type_strmap;
extern const strmap<operation_list> odin_subckt_strmap;

#endif
