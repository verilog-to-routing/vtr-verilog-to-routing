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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <vector>

#include "vtr_error.h"
#include "vtr_time.h"
#include "odin_ii.h"

#include "argparse.hpp"

#include "arch_util.h"

#include "odin_globals.h"
#include "odin_types.h"
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
#include "netlist_statistic.h"
#include "subtractions.h"
#include "vtr_util.h"
#include "vtr_path.h"
#include "vtr_memory.h"
#include "HardSoftLogicMixer.hpp"

#define DEFAULT_OUTPUT "."

loc_t my_location;

t_arch Arch;
global_args_t global_args;
std::vector<t_physical_tile_type> physical_tile_types;
std::vector<t_logical_block_type> logical_block_types;
short physical_lut_size = -1;
int block_tag = -1;
ids default_net_type = WIRE;
HardSoftLogicMixer* mixer;

enum ODIN_ERROR_CODE {
    SUCCESS,
    ERROR_INITIALIZATION,
    ERROR_PARSE_CONFIG,
    ERROR_PARSE_ARGS,
    ERROR_PARSE_ARCH,
    ERROR_SYNTHESIS,
    ERROR_PARSE_BLIF,

};

static void get_physical_luts(std::vector<t_pb_type*>& pb_lut_list, t_mode* mode);
static void get_physical_luts(std::vector<t_pb_type*>& pb_lut_list, t_pb_type* pb_type);
static void set_physical_lut_size();

static ODIN_ERROR_CODE synthesize_verilog() {
    double elaboration_time = wall_time();

    printf("--------------------------------------------------------------------\n");
    printf("High-level synthesis Begin\n");

    FILE* output_blif_file = create_blif(global_args.output_file.value().c_str());

    /* Perform any initialization routines here */
    find_hard_multipliers();
    find_hard_adders();
    //find_hard_adders_for_sub();
    register_hard_blocks();

    module_names_to_idx = sc_new_string_cache();

    /* parse to abstract syntax tree */
    printf("Parser starting - we'll create an abstract syntax tree. Note this tree can be viewed using Grap Viz (see documentation)\n");
    verilog_ast = init_parser();
    parse_to_ast();
    /**
     *  Note that the entry point for ast optimzations is done per module with the
     * function void next_parsed_verilog_file(ast_node_t *file_items_list)
     */

    /* after the ast is made potentially do tagging for downstream links to verilog */
    if (global_args.high_level_block.provenance() == argparse::Provenance::SPECIFIED)
        add_tag_data(verilog_ast);

    /**
     *  Now that we have a parse tree (abstract syntax tree [ast]) of
     *	the Verilog we want to make into a netlist.
     */
    printf("Converting AST into a Netlist. Note this netlist can be viewed using GraphViz (see documentation)\n");
    create_netlist(verilog_ast);
    if (verilog_netlist) {
        // Can't levelize yet since the large muxes can look like combinational loops when they're not
        check_netlist(verilog_netlist);

        //START ################# NETLIST OPTIMIZATION ############################

        /* point for all netlist optimizations. */
        printf("Performing Optimizations of the Netlist\n");
        if (hard_multipliers) {
            /* Perform a splitting of the multipliers for hard block mults */
            reduce_operations(verilog_netlist, MULTIPLY);
            iterate_multipliers(verilog_netlist);
            clean_multipliers();
        }

        if (single_port_rams || dual_port_rams) {
            /* Perform a splitting of any hard block memories */
            iterate_memories(verilog_netlist);
            free_memory_lists();
        }

        if (hard_adders) {
            /* Perform a splitting of the adders for hard block add */
            reduce_operations(verilog_netlist, ADD);
            iterate_adders(verilog_netlist);
            clean_adders();

            /* Perform a splitting of the adders for hard block sub */
            reduce_operations(verilog_netlist, MINUS);
            iterate_adders_for_sub(verilog_netlist);
            clean_adders_for_sub();
        }

        //END ################# NETLIST OPTIMIZATION ############################

        if (configuration.output_netlist_graphs)
            graphVizOutputNetlist(configuration.debug_output_path, "optimized", 2, verilog_netlist); /* Path is where we are */

        /* point where we convert netlist to FPGA or other hardware target compatible format */
        printf("Performing Partial Map to target device\n");
        partial_map_top(verilog_netlist);
        mixer->perform_optimizations(verilog_netlist);

        /* Find any unused logic in the netlist and remove it */
        remove_unused_logic(verilog_netlist);

        /**
         * point for outputs.  This includes soft and hard mapping all structures to the
         * target format.  Some of these could be considred optimizations
         */
        printf("Outputting the netlist to the specified output format\n");

        output_blif(output_blif_file, verilog_netlist);
        module_names_to_idx = sc_free_string_cache(module_names_to_idx);

        cleanup_parser();

        printf("Successful High-level synthesis by Odin\n\tBlif file available at %s\n", global_args.output_file.value().c_str());
        report_mult_distribution();
        report_add_distribution();
        report_sub_distribution();

        compute_statistics(verilog_netlist, true);

        deregister_hard_blocks();

        //cleanup netlist
        free_netlist(verilog_netlist);
    } else {
        printf("Empty blif generated, Empty input or no module declared\n");
    }
    fclose(output_blif_file);

    elaboration_time = wall_time() - elaboration_time;
    printf("Elaboration Time: ");
    print_time(elaboration_time);
    printf("\n--------------------------------------------------------------------\n");

    return SUCCESS;
}

netlist_t* start_odin_ii(int argc, char** argv) {
    double total_time = wall_time();

    try {
        /* Some initialization */
        one_string = vtr::strdup(ONE_VCC_CNS);
        zero_string = vtr::strdup(ZERO_GND_ZERO);
        pad_string = vtr::strdup(ZERO_PAD_ZERO);

    } catch (vtr::VtrError& vtr_error) {
        printf("Odin failed to initialize %s with exit code%d\n", vtr_error.what(), ERROR_INITIALIZATION);
        exit(ERROR_INITIALIZATION);
    }
    mixer = new HardSoftLogicMixer();
    try {
        /* Set up the global arguments to their default. */
        set_default_config();

        /* get the command line options */
        get_options(argc, argv);

        create_directory(configuration.debug_output_path);

    } catch (vtr::VtrError& vtr_error) {
        printf("Odin Failed Reading The command line arguments  %s with exit code%d\n", vtr_error.what(), ERROR_PARSE_ARGS);
        exit(ERROR_PARSE_ARGS);
    } catch (argparse::ArgParseError& arg_error) {
        printf("Odin Failed Reading The command line arguments  %s with exit code%d\n", arg_error.what(), ERROR_PARSE_ARGS);
        exit(ERROR_PARSE_ARGS);
    }

    /* read the confirguration file .. get options presets the config values just in case theyr'e not read in with config file */
    if (global_args.config_file.provenance() == argparse::Provenance::SPECIFIED) {
        printf("Reading Configuration file\n");
        try {
            read_config_file(global_args.config_file.value().c_str());
        } catch (vtr::VtrError& vtr_error) {
            printf("Odin Failed Reading Configuration file %s with exit code%d\n", vtr_error.what(), ERROR_PARSE_CONFIG);
            exit(ERROR_PARSE_CONFIG);
        }
    }

    /* read the FPGA architecture file */
    if (global_args.arch_file.provenance() == argparse::Provenance::SPECIFIED) {
        printf("Architecture: %s\n", vtr::basename(global_args.arch_file.value()).c_str());
        fflush(stdout);

        printf("Reading FPGA Architecture file\n");
        try {
            XmlReadArch(global_args.arch_file.value().c_str(), false, &Arch, physical_tile_types, logical_block_types);
            set_physical_lut_size();
        } catch (vtr::VtrError& vtr_error) {
            printf("Odin Failed to load architecture file: %s with exit code%d\n", vtr_error.what(), ERROR_PARSE_ARCH);
            exit(ERROR_PARSE_ARCH);
        }
    }
    printf("Using Lut input width of: %d\n", physical_lut_size);

    /* do High level Synthesis */
    if (!configuration.list_of_file_names.empty() && configuration.is_verilog_input) {
        for (std::string v_file : global_args.verilog_files.value()) {
            printf("Verilog: %s\n", vtr::basename(v_file).c_str());
        }
        fflush(stdout);

        ODIN_ERROR_CODE error_code = synthesize_verilog();
        if (error_code) {
            printf("Odin Failed to parse Verilog with exit status: %d\n", error_code);
            exit(error_code);
        }

        printf("\n");
    }

    /*************************************************************
     * begin simulation section
     */
    netlist_t* odin_netlist = NULL;
    if (global_args.blif_file.provenance() == argparse::Provenance::SPECIFIED
        || global_args.interactive_simulation
        || global_args.sim_num_test_vectors
        || global_args.sim_vector_input_file.provenance() == argparse::Provenance::SPECIFIED) {
        // if we started with a verilog file read the output that was made since
        // the simulator can only simulate blifs
        if (global_args.blif_file.provenance() != argparse::Provenance::SPECIFIED) {
            configuration.list_of_file_names = {global_args.output_file};
            my_location.file = 0;
        } else {
            printf("Blif: %s\n", vtr::basename(global_args.blif_file.value()).c_str());
            fflush(stdout);
        }

        try {
            odin_netlist = read_blif();
        } catch (vtr::VtrError& vtr_error) {
            printf("Odin Failed to load blif file: %s with exit code:%d \n", vtr_error.what(), ERROR_PARSE_BLIF);
            exit(ERROR_PARSE_BLIF);
        }
    }

    /* Simulate netlist */
    if (odin_netlist && !global_args.interactive_simulation
        && (global_args.sim_num_test_vectors || (global_args.sim_vector_input_file.provenance() == argparse::Provenance::SPECIFIED))) {
        printf("Netlist Simulation Begin\n");
        create_directory(global_args.sim_directory);

        simulate_netlist(odin_netlist);
    }

    compute_statistics(odin_netlist, true);

    printf("--------------------------------------------------------------------\n");
    total_time = wall_time() - total_time;
    printf("Total time: ");
    print_time(total_time);
    printf("\n");
    printf("Odin ran with exit status: %d\n", SUCCESS);
    fflush(stdout);
    delete mixer;
    return odin_netlist;
}

int terminate_odin_ii(netlist_t* odin_netlist) {
    free_netlist(odin_netlist);

    //Clean-up
    free_arch(&Arch);
    free_type_descriptors(logical_block_types);
    free_type_descriptors(physical_tile_types);

    return 0;
}

struct ParseInitRegState {
    int from_str(std::string str) {
        if (str == "0")
            return 0;
        else if (str == "1")
            return 1;
        else if (str == "X")
            return -1;
        std::stringstream msg;
        msg << "Invalid conversion from '" << str << "' (expected one of: " << argparse::join(default_choices(), ", ") << ")";
        throw argparse::ArgParseConversionError(msg.str());
    }

    std::string to_str(int val) {
        if (val == 0)
            return "0";
        else if (val == 1)
            return "1";
        else if (val == -1)
            return "X";
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

    global_args.program_name = parser.prog();

    auto& input_grp = parser.add_argument_group("input files");

    input_grp.add_argument(global_args.config_file, "-c")
        .help("Configuration file")
        .metavar("XML_CONFIGURATION_FILE");

    input_grp.add_argument(global_args.verilog_files, "-V")
        .help("list of Verilog HDL file")
        .nargs('+')
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

    other_grp.add_argument(global_args.print_parse_tokens, "--debug_parser")
        .help("print the parser tokens as they are parsed")
        .default_value("false")
        .action(argparse::Action::STORE_TRUE)
        .metavar("PRINT_PARSE_TOKEN");

    other_grp.add_argument(global_args.permissive, "--permissive")
        .help("Turn possible_error_messages into warning_messages ... unexpected behaviour may occur")
        .default_value("false")
        .action(argparse::Action::STORE_TRUE);

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

    other_grp.add_argument(global_args.adder_def, "--adder_type")
        .help("DEPRECATED")
        .default_value("N/A")
        .metavar("N/A");

    other_grp.add_argument(global_args.adder_cin_global, "--adder_cin_global")
        .help("Defines if the first cin of an adder/subtractor is connected to a global gnd/vdd instead of a dummy adder generating a gnd/vdd.")
        .default_value("false")
        .action(argparse::Action::STORE_TRUE);

    other_grp.add_argument(global_args.top_level_module_name, "--top_module")
        .help("Allow to overwrite the top level module that odin would use")
        .metavar("TOP_LEVEL_MODULE_NAME");

    auto& rand_sim_grp = parser.add_argument_group("random simulation options");

    rand_sim_grp.add_argument(global_args.sim_num_test_vectors, "-g")
        .help("Number of random test vectors to generate")
        .metavar("NUM_VECTORS");

    rand_sim_grp.add_argument(global_args.sim_min_coverage, "--coverage")
        .help("using the g argument we will simulate in blocks until a certain coverage is attained")
        .metavar("MIN_COVERAGE");

    rand_sim_grp.add_argument(global_args.sim_achieve_best, "--best_coverage")
        .help("using the g argument we will simulate in blocks until best coverage is attained")
        .default_value("false")
        .action(argparse::Action::STORE_TRUE);

    rand_sim_grp.add_argument(global_args.sim_random_seed, "-r")
        .help("Random seed")
        .default_value("0")
        .metavar("SEED");

    rand_sim_grp.add_argument(global_args.sim_hold_low, "-L")
        .help("list of primary inputs to hold high at cycle 0, and low for all subsequent cycles")
        .nargs('+')
        .metavar("PRIMARY_INPUTS");

    rand_sim_grp.add_argument(global_args.sim_hold_high, "-H")
        .help("list of primary inputs to hold low at cycle 0, and high for all subsequent cycles")
        .nargs('+')
        .metavar("PRIMARY_INPUTS");

    auto& vec_sim_grp = parser.add_argument_group("vector simulation options");

    vec_sim_grp.add_argument(global_args.sim_vector_input_file, "-t")
        .help("File of predefined input vectors to simulate")
        .metavar("INPUT_VECTOR_FILE");

    vec_sim_grp.add_argument(global_args.sim_vector_output_file, "-T")
        .help("File of predefined output vectors to check against simulation")
        .metavar("OUTPUT_VECTOR_FILE");

    auto& other_sim_grp = parser.add_argument_group("other simulation options");

    other_sim_grp.add_argument(global_args.parralelized_simulation, "-j")
        .help("Number of threads allowed for simulator to use")
        .default_value("1")
        .metavar("PARALEL NODE COUNT");

    other_sim_grp.add_argument(global_args.parralelized_simulation_in_batch, "--batch")
        .help("DEPRECATED")
        .default_value("false")
        .action(argparse::Action::STORE_TRUE)
        .metavar("BATCH FLAG");

    other_sim_grp.add_argument(global_args.sim_directory, "-sim_dir")
        .help("Directory output for simulation")
        .default_value(DEFAULT_OUTPUT)
        .metavar("SIMULATION_DIRECTORY");

    other_sim_grp.add_argument(global_args.sim_generate_three_valued_logic, "-3")
        .help("Generate three valued logic, instead of binary")
        .default_value("false")
        .action(argparse::Action::STORE_TRUE);

    other_sim_grp.add_argument(global_args.interactive_simulation, "--interractive_simulation")
        .help("prevent Odin from freeing element so that application leveraging the simulator can use the nodes")
        .default_value("false")
        .action(argparse::Action::STORE_TRUE);

    other_sim_grp.add_argument<int, ParseInitRegState>(global_args.sim_initial_value, "-U")
        .help("DEPRECATED")
        .default_value("X")
        .metavar("INIT_REG_STATE");

    other_sim_grp.add_argument(global_args.sim_output_both_edges, "-E")
        .help("Output after both edges of the clock (This is by default)")
        .default_value("true")
        .action(argparse::Action::STORE_TRUE);

    other_sim_grp.add_argument(global_args.sim_output_both_edges, "-R")
        .help("DEPRECATED Output after rising edges of the clock only (Default after both edges)")
        .default_value("true")
        .action(argparse::Action::STORE_TRUE);

    other_sim_grp.add_argument(global_args.read_mif_input, "--read_mif")
        .help("look for a mif file to read")
        .default_value("false")
        .action(argparse::Action::STORE_TRUE);

    other_sim_grp.add_argument(global_args.sim_additional_pins, "-p")
        .help(
            "list of additional pins/nodes to monitor during simulation.\n"
            "Eg: \"-p input~0 input~1\" monitors pin 0 and 1 of input, \n"
            "  or \"-p input\" monitors all pins of input as a single port. \n"
            "  or \"-p input~\" monitors all pins of input as separate ports. (split) \n"
            "- Note: Non-existent pins are ignored. \n"
            "- Matching is done via strstr so general strings will match \n"
            "  all similar pins and nodes.\n"
            "    (Eg: FF_NODE will create a single port with all flipflops) \n")
        .nargs('+')
        .metavar("PINS_TO_MONITOR");

    auto& mixing_opt_grp = parser.add_argument_group("mixing hard and soft logic optimization");

    mixing_opt_grp.add_argument(global_args.exact_mults, "--exact_mults")
        .help("To enable mixing hard block and soft logic implementation of adders")
        .default_value("-1")
        .action(argparse::Action::STORE);

    mixing_opt_grp.add_argument(global_args.mults_ratio, "--mults_ratio")
        .help("To enable mixing hard block and soft logic implementation of adders")
        .default_value("-1.0")
        .action(argparse::Action::STORE);

    parser.parse_args(argc, argv);

    //Check required options
    if (!only_one_is_true({
            global_args.config_file.provenance() == argparse::Provenance::SPECIFIED, //have a config file
            global_args.blif_file.provenance() == argparse::Provenance::SPECIFIED,   //have a blif file
            global_args.verilog_files.value().size() > 0                             //have a verilog input list
        })) {
        parser.print_usage();
        error_message(PARSE_ARGS, unknown_location, "%s", "Must include only one of either:\n\ta config file(-c)\n\ta blif file(-b)\n\ta verilog file(-V)\n");
    }

    //adjust thread count
    int thread_requested = global_args.parralelized_simulation;
    int max_thread = std::thread::hardware_concurrency();

    global_args.parralelized_simulation.set(
        std::max(1, std::min(thread_requested, std::min((CONCURENCY_LIMIT - 1), max_thread))), argparse::Provenance::SPECIFIED);

    //Allow some config values to be overriden from command line
    if (!global_args.verilog_files.value().empty()) {
        //parse comma separated list of verilog files
        configuration.list_of_file_names = global_args.verilog_files.value();
        configuration.is_verilog_input = true;
    } else if (global_args.blif_file.provenance() == argparse::Provenance::SPECIFIED) {
        configuration.list_of_file_names = {std::string(global_args.blif_file)};
    }

    if (global_args.arch_file.provenance() == argparse::Provenance::SPECIFIED) {
        configuration.arch_file = global_args.arch_file;
    }

    if (global_args.write_netlist_as_dot.provenance() == argparse::Provenance::SPECIFIED) {
        configuration.output_netlist_graphs = global_args.write_netlist_as_dot;
    }

    if (global_args.write_ast_as_dot.provenance() == argparse::Provenance::SPECIFIED) {
        configuration.output_ast_graphs = global_args.write_ast_as_dot;
    }

    if (global_args.adder_cin_global.provenance() == argparse::Provenance::SPECIFIED) {
        configuration.adder_cin_global = global_args.adder_cin_global;
    }

    if (global_args.print_parse_tokens.provenance() == argparse::Provenance::SPECIFIED) {
        configuration.print_parse_tokens = global_args.print_parse_tokens;
    }

    if (global_args.sim_directory.value() == DEFAULT_OUTPUT) {
        global_args.sim_directory.set(configuration.debug_output_path, argparse::Provenance::SPECIFIED);
    }

    if (configuration.debug_output_path == DEFAULT_OUTPUT) {
        configuration.debug_output_path = global_args.sim_directory;
    }

    if (global_args.permissive.value()) {
        warning_message(PARSE_ARGS, unknown_location, "%s", "Permissive flag is ON. Undefined behaviour may occur\n");
    }
    if (global_args.mults_ratio >= 0.0 && global_args.mults_ratio <= 1.0) {
        delete mixer->_opts[MULTIPLY];
        mixer->_opts[MULTIPLY] = new MultsOpt(global_args.mults_ratio);
    } else if (global_args.exact_mults >= 0) {
        delete mixer->_opts[MULTIPLY];
        mixer->_opts[MULTIPLY] = new MultsOpt(global_args.exact_mults);
    }
}

/*---------------------------------------------------------------------------
 * (function: set_default_options)
 *-------------------------------------------------------------------------*/
void set_default_config() {
    /* Set up the global configuration. */
    configuration.output_type = std::string("blif");
    configuration.output_ast_graphs = 0;
    configuration.output_netlist_graphs = 0;
    configuration.print_parse_tokens = 0;
    configuration.output_preproc_source = 0; // TODO: unused
    configuration.debug_output_path = std::string(DEFAULT_OUTPUT);
    configuration.arch_file = "";

    configuration.fixed_hard_multiplier = 0;
    configuration.split_hard_multiplier = 0;

    configuration.split_memory_width = 0;
    configuration.split_memory_depth = 0;

    configuration.adder_cin_global = false;

    /*
     * Soft logic cutoffs. If a memory or a memory resulting from a split
     * has a width AND depth below these, it will be converted to soft logic.
     */
    configuration.soft_logic_memory_width_threshold = 0;
    configuration.soft_logic_memory_depth_threshold = 0;
}

static void get_physical_luts(std::vector<t_pb_type*>& pb_lut_list, t_mode* mode) {
    for (int i = 0; i < mode->num_pb_type_children; i++) {
        get_physical_luts(pb_lut_list, &mode->pb_type_children[i]);
    }
}

static void get_physical_luts(std::vector<t_pb_type*>& pb_lut_list, t_pb_type* pb_type) {
    if (pb_type) {
        if (pb_type->class_type == LUT_CLASS) {
            pb_lut_list.push_back(pb_type);
        } else {
            for (int i = 0; i < pb_type->num_modes; i++) {
                get_physical_luts(pb_lut_list, &pb_type->modes[i]);
            }
        }
    }
}

static void set_physical_lut_size() {
    std::vector<t_pb_type*> pb_lut_list;

    for (t_logical_block_type& logical_block : logical_block_types) {
        if (logical_block.index != EMPTY_TYPE_INDEX) {
            get_physical_luts(pb_lut_list, logical_block.pb_type);
        }
    }
    for (t_pb_type* pb_lut : pb_lut_list) {
        if (pb_lut) {
            if (pb_lut->num_input_pins < physical_lut_size || physical_lut_size < 1) {
                physical_lut_size = pb_lut->num_input_pins;
            }
        }
    }
}