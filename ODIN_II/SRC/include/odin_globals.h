#ifndef GLOBALS_H
#define GLOBALS_H

#include "odin_types.h"
#include "string_cache.h"
#include "read_xml_arch_file.h"

extern t_logical_block_type* type_descriptors;

/* VERILOG SYNTHESIS GLOBALS */
extern ids default_net_type;

extern global_args_t global_args;
extern config_t configuration;
extern int current_parse_file;

extern long num_modules;
extern ast_node_t** ast_modules;
extern STRING_CACHE* module_names_to_idx;

extern STRING_CACHE* output_nets_sc;
extern STRING_CACHE* input_nets_sc;

extern netlist_t* verilog_netlist;

extern ast_node_t* top_module;
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
/* ACTIVATION ESTIMATION GLOBALS */
extern netlist_t* blif_netlist;

/* Global variable for read_blif function call */
extern netlist_t* read_blif_netlist;

extern long file_line_number;

#endif
