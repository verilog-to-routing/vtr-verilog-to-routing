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

#include "argparse.hpp"

#include "globals.h"
#include "types.h"
#include "netlist_utils.h"
#include "arch_types.h"
#include "parse_making_ast.h"
#include "netlist_create_from_ast.h"
#include "outputs.h"
#include "netlist_optimizations.h"
#include "read_xml_config_file.h"
#include "read_xml_arch_file.h"
#include "partial_map.h"
#include "multipliers.h"
#include "netlist_check.h"
#include "read_blif.h"
#include "read_netlist.h"
#include "activity_estimation.h"
#include "high_level_data.h"
#include "hard_blocks.h"
#include "memories.h"
#include "simulate_blif.h"
#include "errors.h"
#include "netlist_visualizer.h"
#include "adders.h"
#include "subtractions.h"
#include "odin_ii_func.h"
#include "vtr_util.h"

int current_parse_file;
t_arch Arch;
global_args_t global_args;
t_type_descriptor* type_descriptors;
int block_tag;

void get_options(int argc, char **argv);
void print_usage();

#ifdef VPR5
void do_activation_estimation(int num_types, t_type_descriptor * type_descriptors);
#endif

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
		#ifdef VPR5
		t_clocks ClockDetails = { 0 };
		t_power PowerDetails = { 0 };
		XmlReadArch(global_args.arch_file, false, &Arch, &type_descriptors, &num_types, &ClockDetails, &PowerDetails);
		#endif
		#ifdef VPR6
        try {
            XmlReadArch(global_args.arch_file, false, &Arch, &type_descriptors, &num_types);
        } catch(vtr::VtrError& vtr_error) {
            printf("Failed to load architecture file: %s\n", vtr_error.what());
        }
		#endif
	}

	#ifdef VPR5
	if (global_args.activation_blif_file != NULL && global_args.activation_netlist_file != NULL)
	{
		do_activation_estimation(num_types, type_descriptors);
	}
	else
	#endif
	{
		if (!global_args.blif_file)
		{
			/* High level synthesis tool */
			do_high_level_synthesis();
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
		do_simulation_of_netlist();
	}

	#ifdef VPR6
	report_mult_distribution();
	report_add_distribution();
	report_sub_distribution();
	deregister_hard_blocks();
	#endif

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
			#ifdef VPR5
			"  -B <blif_file_for_activation_estimation> \n"
			"     Use with: -N <net_file_for_activation_estimation>\n"
			#endif
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
	/* Set up the global config to default. */
	set_default_config();

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
            .default_value("default_out.blif")
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

    auto& rand_sim_grp = parser.add_argument_group("random simulation options");
    
    rand_sim_grp.add_argument(global_args.sim_num_test_vectors, "-g")
            .help("Number of random test vectors to generate")
            .metavar("NUM_VECTORS");

    rand_sim_grp.add_argument(global_args.sim_num_test_vectors, "-r")
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
}
