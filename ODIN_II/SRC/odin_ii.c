/*
Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/ 
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "vtr_error.h"

#include "globals.h"
#include "types.h"
#include "netlist_utils.h"
#include "arch_types.h"
#include "parse_making_ast.h"
#include "netlist_create_from_ast.h"
#include "ast_util.h"
#include "netlist_optimizations.h"
#include "read_xml_config_file.h"
#include "read_xml_arch_file.h"
#include "partial_map.h"
#include "multipliers.h"
#include "netlist_check.h"
#include "read_blif.h"
#include "output_blif.h"
#include "netlist_cleanup.h"

#include "hard_blocks.h"
#include "memories.h"
#include "simulate_blif.h"

#include "netlist_visualizer.h"
#include "adders.h"
#include "subtractions.h"
#include "vtr_util.h"

int current_parse_file;
t_arch Arch;
global_args_t global_args;
t_type_descriptor* type_descriptors;
int block_tag;

void set_default_options();
void get_options(int argc, char **argv);
void print_usage();

int main(int argc, char **argv)
{
	int num_types;

	/* Some initialization */
	one_string = vtr::strdup("ONE_VCC_CNS");
	zero_string = vtr::strdup("ZERO_GND_ZERO");
	pad_string = vtr::strdup("ZERO_PAD_ZERO");


	printf("--------------------------------------------------------------------\n");
	printf("Welcome to ODIN II version 0.1 - the better High level synthesis tools++ targetting FPGAs (mainly VPR)\n");
	printf("Email: jamieson.peter@gmail.com and ken@unb.ca for support issues\n\n");

	/* Set up the global arguments to their default. */
	set_default_options();
	
	/* get the command line options */
	get_options(argc, argv);

	/* read the confirguration file .. get options presets the config values just in case theyr'e not read in with config file */
	if (global_args.config_file != NULL)
	{
		printf("Reading Configuration file\n");
		read_config_file(global_args.config_file);
	}

	/* read the FPGA architecture file */
	if (global_args.arch_file != NULL)
	{
		printf("Reading FPGA Architecture file\n");
        try {
            XmlReadArch(global_args.arch_file, false, &Arch, &type_descriptors, &num_types);
        } catch(vtr::VtrError& vtr_error) {
            printf("Failed to load architecture file: %s\n", vtr_error.what());
        }
	}
	
	/* do High level Synthesis */
	if (!global_args.blif_file)
	{
		
		double elaboration_time = wall_time();
	
		printf("--------------------------------------------------------------------\n");
		printf("High-level synthesis Begin\n");
		/* Perform any initialization routines here */
		find_hard_multipliers();
		find_hard_adders();
		//find_hard_adders_for_sub();
		register_hard_blocks();
		global_param_table_sc = sc_new_string_cache();
	
		/* parse to abstract syntax tree */
		printf("Parser starting - we'll create an abstract syntax tree.  "
				"Note this tree can be viewed using GraphViz (see documentation)\n");
		parse_to_ast();
		/* Note that the entry point for ast optimzations is done per module with the
		 * function void next_parsed_verilog_file(ast_node_t *file_items_list) */
	
		/* after the ast is made potentially do tagging for downstream links to verilog */
		if (global_args.high_level_block != NULL)
		{
			add_tag_data();
		}
	
		/* Now that we have a parse tree (abstract syntax tree [ast]) of
		 * the Verilog we want to make into a netlist. */
		printf("Converting AST into a Netlist. "
				"Note this netlist can be viewed using GraphViz (see documentation)\n");
		create_netlist();
	
		// Can't levelize yet since the large muxes can look like combinational loops when they're not
		check_netlist(verilog_netlist);
	
		/* point for all netlist optimizations. */
		printf("Performing Optimizations of the Netlist\n");
		netlist_optimizations_top(verilog_netlist);
	
		if (configuration.output_netlist_graphs )
		{
			/* Path is where we are */
			graphVizOutputNetlist(configuration.debug_output_path, "optimized", 1, verilog_netlist);
		}
	
		//*******
	
		/* point where we convert netlist to FPGA or other hardware target compatible format */
		printf("Performing Partial Map to target device\n");
		partial_map_top(verilog_netlist);
	
		/* Find any unused logic in the netlist and remove it */
		remove_unused_logic(verilog_netlist);
	
		/* point for outputs.  This includes soft and hard mapping all structures to the
		 * target format.  Some of these could be considred optimizations */
		printf("Outputting the netlist to blif output format\n");
		output_blif(global_args.output_file, verilog_netlist);
	
		elaboration_time = wall_time() - elaboration_time;
	
		printf("Successful High-level synthesis by Odin in ");
		print_time(elaboration_time);
		printf("\n");
		printf("--------------------------------------------------------------------\n");
	
		// FIXME: free contents?
		sc_free_string_cache(global_param_table_sc);
		
		
	}
	else
	{
        try {
            read_blif(global_args.blif_file);
        } catch(vtr::VtrError& vtr_error) {
            printf("Failed to load blif file: %s\n", vtr_error.what());
        }
	}

	/* Simulate netlist */
	if (global_args.sim_num_test_vectors || global_args.sim_vector_input_file){
		printf("Netlist Simulation Begin\n");
		simulate_netlist(verilog_netlist);
		printf("--------------------------------------------------------------------\n");
	}
	
	report_mult_distribution();
	report_add_distribution();
	report_sub_distribution();
	deregister_hard_blocks();

	return 0;
} 

/*
 * Prints usage information for Odin II. This should be kept up to date with the latest
 * features added to Odin II.
 */
void print_usage()
{
	printf
	(
			"USAGE: odin_II.exe [-c <Configuration> | -b <BLIF> | -V <Verilog HDL>]\n"
			"  -c <XML Configuration File>\n"
			"  -V <Verilog HDL File>\n"
			"  -b <BLIF File>\n"
			" Other options:\n"
			"  -o <output_path and file name>\n"
			"  -a <architecture_file_in_VPR6.0_form>\n"
			"  -G Output netlist graph in graphviz .dot format. (net.dot, opens with dotty)\n"
			"  -A Output AST graph in .dot format.\n"
			"  -W Print all warnings. (Can be substantial.) \n"
			"  -h Print help\n"
			"\n"
			" SIMULATION: Always produces input_vectors, output_vectors,\n"
			"             and ModelSim test.do file.\n"
			"  Activate simulation with either: \n"
			"  -g <Number of random test vectors to generate>\n"
			"     -L <Comma-separated list of primary inputs to hold \n"
			"         high at cycle 0, and low for all subsequent cycles.>\n"
			"     -H <Comma-separated list of primary inputs to hold low at \n"
			"         cycle 0, and high for all subsequent cycles.>\n"
			"     -3 Generate three valued logic. (Default is binary.)\n"
			"     -r <Random seed> (Default is 0.)\n"
			"  -t <input vector file>: Supply a predefined input vector file\n"
			"  -U Default initial register value. Set to -U0, -U1 or -UX (unknown). Default: X\n"

			" Other Simulation Options: \n"
			"  -T <output vector file>: Supply an output vector file to check output\n"
			"                            vectors against.\n"
			"  -E Output after both edges of the clock.\n"
			"     (Default is to output only after the falling edge.)\n"
			"  -R Output after rising edge of the clock only.\n"
			"     (Default is to output only after the falling edge.)\n"
			"  -p <Comma-separated list of additional pins/nodes to monitor\n"
			"      during simulation.>\n"
			"     Eg: \"-p input~0,input~1\" monitors pin 0 and 1 of input, \n"
			"       or \"-p input\" monitors all pins of input as a single port. \n"
			"       or \"-p input~\" monitors all pins of input as separate ports. (split) \n"
			"     - Note: Non-existent pins are ignored. \n"
			"     - Matching is done via strstr so general strings will match \n"
			"       all similar pins and nodes.\n"
			"         (Eg: FF_NODE will create a single port with all flipflops) \n"
	);
	fflush(stdout);
}

/*---------------------------------------------------------------------------------------------
 * (function: get_options)
 *-------------------------------------------------------------------------*/
void get_options(int argc, char **argv)
{
	
	/* Parse the command line options.  */
	const char *optString = "hc:V:WREh:o:a:B:b:N:f:s:S:p:g:r:t:T:L:H:GA3U::";
	int opt = getopt(argc, argv, optString);
	while(opt != -1) 
	{
		switch(opt)
		{
			/* arch file */
			case 'a':
				global_args.arch_file = optarg;
				configuration.arch_file = optarg;
			break;
			/* config file */
			case 'c':
				global_args.config_file = optarg;
			break;
			case 'V':
				global_args.verilog_file = optarg;
			break;
			case 'o':
				global_args.output_file = optarg;
			break;
			case 'b':
				global_args.blif_file = optarg;
			break;
			case 'f':
				warning_message(0, -1, 0, "Option -f: VPR 6.0 doesn't have this feature yet.  You'll need to deal with the output_blif.c differences wrapped by \"if (global_args.high_level_block != NULL)\"\n");
			break;
			case 'g':
				global_args.sim_num_test_vectors = atoi(optarg);
			break;
			case 'r':
				global_args.sim_random_seed = atoi(optarg);
			break;
			case '3':
				global_args.sim_generate_three_valued_logic = 1;
			break;
			case 'L':
				global_args.sim_hold_low = optarg;
			break;
			case 'H':
				global_args.sim_hold_high = optarg;
			break;
			case 't':
				global_args.sim_vector_input_file = optarg;
			break;
			case 'T':
				global_args.sim_vector_output_file = optarg;
			break;
			case 'U':
				global_args.sim_initial_value = -1;
				if(optarg != NULL){
					if(*optarg == '0') global_args.sim_initial_value = 0;
					else if(*optarg == '1') global_args.sim_initial_value = 1;
				}
			break;
			case 'p':
				global_args.sim_additional_pins = optarg;
			break;
			case 'G':
				configuration.output_netlist_graphs = 1;
			break;
			case 'A':
				configuration.output_ast_graphs = 1;
			break;
			case 'W':
				global_args.all_warnings = 1;
			break;
			case 'E':
				global_args.sim_output_both_edges = 1;
			break;
			case 'R':
				global_args.sim_output_rising_edge = 1;
			break;
			case 'h':
			default :
				print_usage();
				error_message(-1, 0, -1, "Invalid arguments.\n");
			break;
		}
		opt = getopt(argc, argv, optString);
	}

	if (
			   !global_args.config_file
			&& !global_args.blif_file
			&& !global_args.verilog_file
			// VPR5 legacy code?
			//&& ((!global_args.activation_blif_file) || (!global_args.activation_netlist_file))
			)
	{
		print_usage();
		error_message(-1,0,-1,"Must include either "
				"a config file, a blif netlist, or a verilog file\n");
	}
	else if ((global_args.config_file && global_args.verilog_file) 
			// VPR5 legacy code?
			//|| global_args.activation_blif_file
			)
	{
		warning_message(-1,0,-1, "Using command line options for verilog input file!!!\n");
	}
}

/*---------------------------------------------------------------------------
 * (function: set_default_options)
 *-------------------------------------------------------------------------*/
void set_default_options()
{
/* Set up the global arguments to their default. */
	global_args.config_file = NULL;
	global_args.verilog_file = NULL;
	global_args.blif_file = NULL;
	global_args.output_file = vtr::strdup("./default_out.blif");
	global_args.arch_file = NULL;
	global_args.activation_blif_file = NULL;
	global_args.activation_netlist_file = NULL;
	global_args.high_level_block = NULL;
	global_args.sim_vector_input_file = NULL;
	global_args.sim_vector_output_file = NULL;
	global_args.sim_additional_pins = NULL;
	global_args.sim_num_test_vectors = 0;
	global_args.sim_generate_three_valued_logic = 0;
	global_args.sim_hold_low = NULL;
	global_args.sim_hold_high = NULL;
	global_args.sim_output_both_edges = 0;
	global_args.sim_output_rising_edge = 0;
	global_args.sim_initial_value = -1;
	global_args.sim_random_seed = 0;
	global_args.all_warnings = 0;

	/* Set up the global configuration. */
	configuration.list_of_file_names = NULL;
	configuration.num_list_of_file_names = 0;
	configuration.output_type = vtr::strdup("blif");
	configuration.output_ast_graphs = 0;
	configuration.output_netlist_graphs = 0;
	configuration.print_parse_tokens = 0;
	configuration.output_preproc_source = 0;
	configuration.debug_output_path = vtr::strdup(".");
	configuration.arch_file = NULL;

	configuration.fixed_hard_multiplier = 0;
	configuration.split_hard_multiplier = 0;

	configuration.split_memory_width = 0;
	configuration.split_memory_depth = 0;

	/*
	 * Soft logic cutoffs. If a memory or a memory resulting from a split
	 * has a width AND depth below these, it will be converted to soft logic.
	 */
	configuration.soft_logic_memory_width_threshold = 0;
	configuration.soft_logic_memory_depth_threshold = 0;
}

