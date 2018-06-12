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
#include <string.h>
#include <sstream>

#include "vtr_error.h"
#include "vtr_time.h"

#include "argparse.hpp"

#include "arch_util.h"

#include "soft_logic_def_parser.h"
#include "globals.h"
#include "types.h"
#include "netlist_utils.h"
#include "arch_types.h"
#include "parse_making_ast.h"
#include "netlist_create_from_ast.h"
#include "ast_util.h"
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
#include "vtr_path.h"
#include "vtr_memory.h"

#define DEFAULT_OUTPUT "OUTPUT/"

size_t current_parse_file;
t_arch Arch;
global_args_t global_args;
t_type_descriptor* type_descriptors;
int block_tag;

void set_default_config();
void get_options(int argc, char **argv);
void parse_adder_def_file();

int main(int argc, char **argv)
{
    vtr::ScopedFinishTimer t("Odin II");
	int num_types=0;

	/* Some initialization */
	one_string = vtr::strdup("ONE_VCC_CNS");
	zero_string = vtr::strdup("ZERO_GND_ZERO");
	pad_string = vtr::strdup("ZERO_PAD_ZERO");


	printf("--------------------------------------------------------------------\n");
	printf("Welcome to ODIN II version 0.1 - the better High level synthesis tools++ targetting FPGAs (mainly VPR)\n");
	printf("Email: jamieson.peter@gmail.com and ken@unb.ca for support issues\n\n");

	/* Set up the global arguments to their default. */
	set_default_config();

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
            //Clean-up
            free_arch(&Arch);
            free_type_descriptors(type_descriptors, num_types);
            return 1;
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

		/* get odin soft_logic definition file */
    if(!hard_adders && strcmp(global_args.adder_def,"default")!=0)
    {
      if(strcmp(global_args.adder_def,"optimized")==0)
      {
        std::string path = vtr::dirname(argv[0]) + "odin.soft_config";
        read_soft_def_file(path.c_str());
      }
      else
        read_soft_def_file(global_args.adder_def);
    }

		global_param_table_sc = sc_new_string_cache();

		/* parse to abstract syntax tree */
		printf("Parser starting - we'll create an abstract syntax tree.  "
				"Note this tree can be viewed using Grap Viz (see documentation)\n");
		parse_to_ast();
		/* Note that the entry point for ast optimzations is done per module with the
		 * function void next_parsed_verilog_file(ast_node_t *file_items_list) */

		/* after the ast is made potentially do tagging for downstream links to verilog */
		if (global_args.high_level_block)
			add_tag_data();

		/* Now that we have a parse tree (abstract syntax tree [ast]) of
		 * the Verilog we want to make into a netlist. */
		printf("Converting AST into a Netlist. "
				"Note this netlist can be viewed using GraphViz (see documentation)\n");
		create_netlist();

		// Can't levelize yet since the large muxes can look like combinational loops when they're not
		check_netlist(verilog_netlist);

		//START ################# NETLIST OPTIMIZATION ############################

			/* point for all netlist optimizations. */
			printf("Performing Optimizations of the Netlist\n");
			/* Perform a splitting of the multipliers for hard block mults */
		    reduce_operations(verilog_netlist, MULTIPLY);
			iterate_multipliers(verilog_netlist);
			clean_multipliers();

			/* Perform a splitting of any hard block memories */
			iterate_memories(verilog_netlist);
			free_memory_lists();

			/* Perform a splitting of the adders for hard block add */
			reduce_operations(verilog_netlist, ADD);
			iterate_adders(verilog_netlist);
			clean_adders();

			/* Perform a splitting of the adders for hard block sub */
			reduce_operations(verilog_netlist, MINUS);
			iterate_adders_for_sub(verilog_netlist);
			clean_adders_for_sub();

		//END ################# NETLIST OPTIMIZATION ############################

		if (configuration.output_netlist_graphs )
			graphVizOutputNetlist(configuration.debug_output_path, "optimized", 1, verilog_netlist); /* Path is where we are */

		//*******

		/* point where we convert netlist to FPGA or other hardware target compatible format */
		printf("Performing Partial Map to target device\n");
		partial_map_top(verilog_netlist);

		/* Find any unused logic in the netlist and remove it */
		remove_unused_logic(verilog_netlist);

		/* point for outputs.  This includes soft and hard mapping all structures to the
		 * target format.  Some of these could be considred optimizations */
		printf("Outputting the netlist to the specified output format\n");
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

    //Clean-up
    free_arch(&Arch);
    free_type_descriptors(type_descriptors, num_types);

	return 0;
}

struct ParseInitRegState {
    int from_str(std::string str) {
        if      (str == "0") return 0;
        else if (str == "1") return 1;
        else if (str == "X") return -1;
        std::stringstream msg;
        msg << "Invalid conversion from '" << str << "' (expected one of: " << argparse::join(default_choices(), ", ") << ")";
        throw argparse::ArgParseConversionError(msg.str());
    }

    std::string to_str(int val) {
        if (val == 0)       return "0";
        else if (val == 1)  return "1";
        else if (val == -1) return "X";
        std::stringstream msg;
        msg << "Invalid conversion from " << val;
        throw argparse::ArgParseConversionError(msg.str());
    }

    std::vector<std::string> default_choices() {
        return {"0", "1", "X"};
    }
};

/*---------------------------------------------------------------------------------------------
 * (function: get_options)
 *-------------------------------------------------------------------------*/
void get_options(int argc, char** argv) {

    auto parser = argparse::ArgumentParser(argv[0]);

    auto& input_grp = parser.add_argument_group("input files");

    input_grp.add_argument(global_args.config_file, "-c")
            .help("Configuration file")
            .metavar("XML_CONFIGURATION_FILE");

    input_grp.add_argument(global_args.verilog_file, "-V")
            .help("Verilog HDL file")
            .metavar("VERILOG_FILE");

    input_grp.add_argument(global_args.blif_file, "-b")
            .help("BLIF file")
            .metavar("BLIF_FILE");

    auto& output_grp = parser.add_argument_group("output files");

    output_grp.add_argument(global_args.output_file, "-o")
            .help("Output file path")
            .default_value("OUTPUT/default_out.blif")
            .metavar("OUTPUT_FILE_PATH");

    auto& other_grp = parser.add_argument_group("other options");

    other_grp.add_argument(global_args.show_help, "-h")
            .help("Display this help message")
            .action(argparse::Action::HELP);

    other_grp.add_argument(global_args.arch_file, "-a")
            .help("VTR FPGA architecture description file (XML)")
            .metavar("ARCHITECTURE_FILE");

    other_grp.add_argument(global_args.write_netlist_as_dot, "-G")
            .help("Output netlist graph in graphviz .dot format")
            .default_value("false")
            .action(argparse::Action::STORE_TRUE);

    other_grp.add_argument(global_args.write_ast_as_dot, "-A")
            .help("Output AST graph in graphviz .dot format")
            .default_value("false")
            .action(argparse::Action::STORE_TRUE);

    other_grp.add_argument(global_args.all_warnings, "-W")
            .help("Print all warnings (can be substantial)")
            .default_value("false")
            .action(argparse::Action::STORE_TRUE);

    other_grp.add_argument(global_args.black_box_latches, "-black_box_latches")
            .help("Output all Latches as Black Boxes")
            .default_value("false")
            .action(argparse::Action::STORE_TRUE);

    other_grp.add_argument(global_args.adder_def, "--adder_type")
            .help("input file defining adder_type, default is to use \"optimized\" values, use \"ripple\" to fall back onto simple ripple adder")
	        .default_value("default")
	        .metavar("INPUT_FILE")
	        ;

    auto& rand_sim_grp = parser.add_argument_group("random simulation options");

    rand_sim_grp.add_argument(global_args.sim_num_test_vectors, "-g")
            .help("Number of random test vectors to generate")
            .metavar("NUM_VECTORS");

    rand_sim_grp.add_argument(global_args.sim_random_seed, "-r")
            .help("Random seed")
            .default_value("0")
            .metavar("SEED");

    rand_sim_grp.add_argument(global_args.sim_hold_low, "-L")
            .help("Comma-separated list of primary inputs to hold high at cycle 0, and low for all subsequent cycles")
            .metavar("PRIMARY_INPUTS");

    rand_sim_grp.add_argument(global_args.sim_hold_high, "-H")
            .help("Comma-separated list of primary inputs to hold low at cycle 0, and high for all subsequent cycles")
            .metavar("PRIMARY_INPUTS");


    auto& vec_sim_grp = parser.add_argument_group("vector simulation options");

    vec_sim_grp.add_argument(global_args.sim_vector_input_file, "-t")
            .help("File of predefined input vectors to simulate")
            .metavar("INPUT_VECTOR_FILE");

    vec_sim_grp.add_argument(global_args.sim_vector_output_file, "-T")
            .help("File of predefined output vectors to check against simulation")
            .metavar("OUTPUT_VECTOR_FILE");

    auto& other_sim_grp = parser.add_argument_group("other simulation options");

    other_sim_grp.add_argument(global_args.sim_directory, "-sim_dir")
            .help("Directory output for simulation")
            .default_value(DEFAULT_OUTPUT)
            .metavar("SIMULATION_DIRECTORY");

    other_sim_grp.add_argument(global_args.sim_generate_three_valued_logic, "-3")
            .help("Generate three valued logic, instead of binary")
            .default_value("false")
            .action(argparse::Action::STORE_TRUE);

    other_sim_grp.add_argument<int,ParseInitRegState>(global_args.sim_initial_value, "-U")
            .help("Default initial register state")
            .default_value("X")
            .metavar("INIT_REG_STATE");

    other_sim_grp.add_argument(global_args.sim_output_both_edges, "-E")
            .help("Output after both edges of the clock (Default only after the falling edge)")
            .default_value("false")
            .action(argparse::Action::STORE_TRUE);

    other_sim_grp.add_argument(global_args.sim_output_both_edges, "-R")
            .help("Output after rising edges of the clock only (Default only after the falling edge)")
            .default_value("false")
            .action(argparse::Action::STORE_TRUE);

    other_sim_grp.add_argument(global_args.sim_additional_pins, "-p")
            .help("Comma-separated list of additional pins/nodes to monitor during simulation.\n"
                  "Eg: \"-p input~0,input~1\" monitors pin 0 and 1 of input, \n"
                  "  or \"-p input\" monitors all pins of input as a single port. \n"
                  "  or \"-p input~\" monitors all pins of input as separate ports. (split) \n"
                  "- Note: Non-existent pins are ignored. \n"
                  "- Matching is done via strstr so general strings will match \n"
                  "  all similar pins and nodes.\n"
                  "    (Eg: FF_NODE will create a single port with all flipflops) \n")
            .metavar("PINS_TO_MONITOR");

    parser.parse_args(argc, argv);

    //Check required options
    if (   !global_args.config_file
        && !global_args.blif_file
        && !global_args.verilog_file) {
        parser.print_usage();
        error_message(-1,0,-1,"Must include either "
                              "a config file, a blif netlist, or a verilog file\n");
    } else if (global_args.config_file && global_args.verilog_file) {
        warning_message(-1,0,-1, "Using command line options for verilog input file when a config file was specified!\n");
    }

    //Allow some config values to be overriden from command line
    if (global_args.arch_file.provenance() == argparse::Provenance::SPECIFIED) {
        configuration.arch_file = global_args.arch_file;
    }
    if (global_args.write_netlist_as_dot.provenance() == argparse::Provenance::SPECIFIED) {
        configuration.output_netlist_graphs = global_args.write_netlist_as_dot;
    }
    if (global_args.write_ast_as_dot.provenance() == argparse::Provenance::SPECIFIED) {
        configuration.output_ast_graphs = global_args.write_ast_as_dot;
    }
    if (configuration.debug_output_path == DEFAULT_OUTPUT) {
        configuration.debug_output_path = std::string(global_args.sim_directory);
    }
    if (global_args.adder_def.provenance() == argparse::Provenance::SPECIFIED) {
        printf("using experimental soft_logic optimization \n");
    }
}

/*---------------------------------------------------------------------------
 * (function: set_default_options)
 *-------------------------------------------------------------------------*/
void set_default_config()
{
	/* Set up the global configuration. */
	configuration.list_of_file_names = NULL;
	configuration.num_list_of_file_names = 0;
	configuration.output_type = std::string("blif");
	configuration.output_ast_graphs = 0;
	configuration.output_netlist_graphs = 0;
	configuration.print_parse_tokens = 0;
	configuration.output_preproc_source = 0;
	configuration.debug_output_path = std::string(DEFAULT_OUTPUT);
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
