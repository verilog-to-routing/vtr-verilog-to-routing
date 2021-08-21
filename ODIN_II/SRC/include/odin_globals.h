#ifndef GLOBALS_H
#define GLOBALS_H

#include "config_t.h"
#include "odin_types.h"
#include "string_cache.h"
#include "Hashtable.hpp"
#include "read_xml_arch_file.h"
#include "HardSoftLogicMixer.hpp"

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

extern strmap<file_type_e> file_type_strmap;
extern strmap<elaborator_e> elaborator_strmap;
extern strmap<operation_list> odin_subckt_strmap;
extern strmap<operation_list> yosys_subckt_strmap;

#endif
