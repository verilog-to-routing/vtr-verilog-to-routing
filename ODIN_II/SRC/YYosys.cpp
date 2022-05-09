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
 * @file This file includes the definition of the basic structure used
 * in Odin-II to run Yosys API. Technically, this class utilizes Yosys
 * as a separate synthesizer to generate a coarse-grain netlist. In the
 * end, the Yosys generated netlist is technology mapped to the target
 * device using Odin-II partial mapper. It is worth noting that users
 * can also separately run Yosys to performs coarse-grain synthesis using
 * the $ODIN_II_ROOT/regression_test/tool/run_yosys.sh script. This script
 * generates a coarse-grain netlist; then, they can pass the BLIF file to
 * Odin-II with --coarsen flag for performing techmap. Yosys log file is 
 * located in the same place where the Odin-II output BLIF is. To see
 * Yosys logs in the standard output stream you can provide Odin-II with 
 * --show_yosys_log flag.
 */

#include <cstdlib>
#include <fstream>    // ostream
#include <unistd.h>   // fork
#include <sys/wait.h> // wait

#include "YYosys.hpp"
#include "Verilog.hpp"
#include "config_t.h"    // configuration
#include "odin_util.h"   // get_directory
#include "odin_error.h"  // error_message
#include "hard_blocks.h" // hard_block_names

#ifdef ODIN_USE_YOSYS
#    include "kernel/yosys.h" // Yosys
USING_YOSYS_NAMESPACE
#    define YOSYS_ELABORATION_ERROR                        \
        "\n\tERROR: Yosys failed to perform elaboration, " \
        "Please look at the log file for the failure cause or pass \'--show_yosys_log\' to Odin-II to see the logs.\n"
#    define YOSYS_FORK_ERROR \
        "\n\tERROR: Yosys child process failed to be created\n"
#else
#    define YOSYS_INSTALLATION_ERROR                                    \
        "ERROR: It seems Yosys is not installed in the VTR repository." \
        " Please compile the VTR with (" ODIN_USE_YOSYS_STR ") flag.\n"
#endif

/**
 * @brief Construct the Yosys object
 * required by compiler
 */
YYosys::YYosys() {
/* check if Yosys is installed in VTR */
#ifndef ODIN_USE_YOSYS
    error_message(PARSE_ARGS, unknown_location, "%s", YOSYS_INSTALLATION_ERROR);
#else
    /* set Yosys+Odin default variables */
    this->set_default_variables();

    /* create Yosys child process */
    if ((this->yosys_pid = fork()) < 0)
        error_message(UTIL, unknown_location, "%s", YOSYS_FORK_ERROR);
    else {
        /* set up VTR_ROOT path environment variable */
        set_env("VTR_ROOT", std::string(global_args.program_root + "/..").c_str(), 1);
    }

#endif
}

/**
 * @brief Destruct the Yosys object
 * to avoid memory leakage
 */
YYosys::~YYosys() {
#ifndef ODIN_USE_YOSYS
    error_message(PARSE_ARGS, unknown_location, "%s", YOSYS_INSTALLATION_ERROR);
#else
    /* execute only in child process */
    if (this->yosys_pid == 0) {
        /* exit from Yosys executable */
        yosys_shutdown();
        // CLEAN UP
        std::ofstream* ptr = NULL;
        if (!configuration.show_yosys_log && (ptr = dynamic_cast<std::ofstream*>(yosys_log_stream))) {
            ptr->close();
            delete ptr;
        }
    }
#endif
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: perform_elaboration)
 * 
 * @brief Call Yosys elaboration and set Odin-II input BLIF file
 * to the Yosys output BLIF
 * -------------------------------------------------------------------------------------------*/
void YYosys::perform_elaboration() {
#ifndef ODIN_USE_YOSYS
    error_message(PARSE_ARGS, unknown_location, "%s", YOSYS_INSTALLATION_ERROR);
#else
    /* elaborator type validation */
    oassert(configuration.elaborator_type == elaborator_e::_YOSYS);
    /* execute only in child process */
    if (this->yosys_pid == 0) {
        /* initalize Yosys */
        this->init_yosys();
        /* generate and load DSP declarations */
        this->load_target_dsp_blocks();
        /* perform elaboration using Yosys API */
        this->elaborate();

        /* exit successfully */
        _exit(EXIT_SUCCESS);
    }
    /* parent process, i.e., Odin-II */
    else {
        /* wait for the Yosys child process */
        auto yosys_status = -1; // the status of the Yosys fork
        waitpid(0, &yosys_status, 0);
        int yosys_exit_status = WEXITSTATUS(yosys_status);

        if (yosys_exit_status != 0) {
            error_message(PARSER, unknown_location, "%s", YOSYS_ELABORATION_ERROR);
        }
        /* Yosys successfully generated coarse-grain BLIF file */
        this->re_initialize_odin_globals();
    }
#endif
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: load_target_dsp_blocks)
 * 
 * @brief this routine generates a Verilog file, including the 
 * declaration of all DSP blocks available in the targer architecture.
 * Then, the Verilog fle is read by Yosys to make it aware of them
 * -------------------------------------------------------------------------------------------*/
void YYosys::load_target_dsp_blocks() {
#ifndef ODIN_USE_YOSYS
    error_message(PARSE_ARGS, unknown_location, "%s", YOSYS_INSTALLATION_ERROR);
#else
    Verilog::Writer vw = Verilog::Writer();
    vw._create_file(configuration.dsp_verilog.c_str());

    t_model* hb = Arch.models;
    while (hb) {
        // declare hardblocks in a verilog file
        if (strcmp(hb->name, SINGLE_PORT_RAM_string) && strcmp(hb->name, DUAL_PORT_RAM_string) && strcmp(hb->name, "multiply") && strcmp(hb->name, "adder"))
            vw.declare_blackbox(hb->name);

        hb = hb->next;
    }

    vw._write(NULL);
    run_pass(std::string("read_verilog -nomem2reg " + configuration.dsp_verilog));
#endif
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: init_yosys)
 * 
 * @brief Initialize Yosys
 * -------------------------------------------------------------------------------------------*/
void YYosys::init_yosys() {
#ifndef ODIN_USE_YOSYS
    error_message(PARSE_ARGS, unknown_location, "%s", YOSYS_INSTALLATION_ERROR);
#else
    /* must only be performed in the Yosys child process */
    oassert(this->yosys_pid == 0);

    /* specify the yosys logs output stream */
    yosys_log_stream = (configuration.show_yosys_log) ? &std::cout : new std::ofstream(this->log_file);
    log_streams.push_back(yosys_log_stream);

    yosys_setup();
    yosys_banner();

    /* Read VTR baseline library first */
    run_pass(std::string("read_verilog -nomem2reg " + this->vtr_primitives_file));
    run_pass(std::string("setattr -mod -set keep_hierarchy 1 " + std::string(SINGLE_PORT_RAM_string)));
    run_pass(std::string("setattr -mod -set keep_hierarchy 1 " + std::string(DUAL_PORT_RAM_string)));
#endif
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: set_default_variables)
 * 
 * @brief set Yosys+Odin default variables
 * -------------------------------------------------------------------------------------------*/
void YYosys::set_default_variables() {
    /* init Yosys+Odin varibles */
    this->vtr_primitives_file = std::string(global_args.program_root + "/../vtr_flow/primitives.v");
    this->odin_techlib = std::string(global_args.program_root + "/techlib");

    /* Verilog input files */
    this->verilog_circuits = configuration.list_of_file_names;

    /* Yosys output BLIF file */
    std::string path_prefix = (global_args.output_file.provenance() == argparse::Provenance::SPECIFIED)
                                  ? get_directory(global_args.output_file.value())
                                  : global_args.current_path;
    /* handle unwanted root access */
    if (path_prefix == "")
        path_prefix = global_args.current_path;

    this->log_file = std::string(path_prefix + "/" + "elaboration.yosys.log");
    this->coarse_grain_blif = std::string(path_prefix + "/" + "coarsen_netlist.yosys.blif");
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: execute)
 * 
 * @brief Executing Yosys using its API
 * -------------------------------------------------------------------------------------------*/
void YYosys::execute() {
#ifndef ODIN_USE_YOSYS
    error_message(PARSE_ARGS, unknown_location, "%s", YOSYS_INSTALLATION_ERROR);
#else
    /* must only be performed in the Yosys child process */
    oassert(this->yosys_pid == 0);

    if (configuration.tcl_file != "") {
        // run the tcl file by yosys
        run_pass(std::string("tcl " + configuration.tcl_file));
    } else {
        // Read the hardware decription Verilog circuits
        // FOR loop enables include feature for Yosys+Odin (multiple Verilog input files)
        for (auto verilog_circuit : this->verilog_circuits)
            run_pass(std::string("read_verilog -sv -nolatches " + verilog_circuit));

        // Check whether cells match libraries and find top module
        if (global_args.top_level_module_name.provenance() == argparse::Provenance::SPECIFIED) {
            run_pass(std::string("hierarchy -check -top " + global_args.top_level_module_name.value() + " -purge_lib"));
        } else {
            run_pass(std::string("hierarchy -check -auto-top -purge_lib"));
        }

        // Use a readable name convention
        run_pass(std::string("autoname"));
        // Translate processes to netlist components such as MUXs, FFs and latches
        run_pass(std::string("proc; opt;"));
        // Extraction and optimization of finite state machines
        run_pass(std::string("fsm; opt;"));
        // Collects memories, their port and create multiport memory cells
        run_pass(std::string("memory_collect; memory_dff; opt;"));

        // Looking for combinatorial loops, wires with multiple drivers and used wires without any driver.
        run_pass(std::string("check"));
        // Transform asynchronous dffs to synchronous dffs using techlib files provided by Yosys
        run_pass(std::string("techmap -map " + this->odin_techlib + "/adff2dff.v"));
        run_pass(std::string("techmap -map " + this->odin_techlib + "/adffe2dff.v"));
        // To resolve Yosys internal indexed part-select circuitries
        run_pass(std::string("techmap */t:$shift */t:$shiftx"));

        /**
         * convert yosys mem blocks to BRAMs / ROMs
         *
         * [NOTE] : Yosys complains about expression width more than 24 bits.
         * E.g.[63 : 0] memory[18 : 0] == > ERROR : Expression width 33554432 exceeds implementation limit of 16777216 !
         * Therfore, Yosys internal memory cells will be handled inside Odin-II as YMEM cell type.
         *
         * The following commands transform Yosys internal memories into BRAMs/ROMs defined in the Odin-II techlib
         * However, due to the above-mentioned reason they are commented.
         *  Yosys::run_pass(std::string("memory_bram -rules ", this->odin_techlib, "/mem_rules.txt"))
         *  Yosys::run_pass(std::string("techmap -map ", this->odin_techlib, "/mem_map.v"));
         */

        // Transform the design into a new one with single top module
        run_pass(std::string("flatten"));
        // Transforms PMUXes into trees of regular multiplexers
        run_pass(std::string("pmuxtree"));
        // To possibly reduce word sizes by Yosys
        run_pass(std::string("wreduce"));
        // "-undirven" to ensure there is no wire without drive
        // -noff #potential option to remove all sdffXX and etc. Only dff will remain
        // "opt_muxtree" removes dead branches, "opt_expr" performs const folding and
        // removes "undef" from mux inputs and replace muxes with buffers and inverters
        run_pass(std::string("opt -undriven -full; opt_muxtree; opt_expr -mux_undef -mux_bool -fine;;;"));
        // Use a readable name convention
        run_pass(std::string("autoname"));
        // Print statistics
        run_pass(std::string("stat"));
    }

#endif
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: elaborate)
 * 
 * @brief Perform Yosys elaboration and set the Odin-II input BLIF
 * file with the Yosys generated output BLIF file
 * -------------------------------------------------------------------------------------------*/
void YYosys::elaborate() {
    /* must only be performed in the Yosys child process */
    oassert(this->yosys_pid == 0);

    /* execute coarse-grain elaboration steps */
    this->execute();
    printf("Successful Elaboration of the design by Yosys\n");
    /* ask Yosys to generate the output BLIF file */
    this->output_blif();
    printf("\tCoarse-grain netlist is available at: %s\n\n", this->coarse_grain_blif.c_str());
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: output_blif)
 * 
 * @brief Ask Yosys to generate an output BLIF file in 
 * the path specified in this->coarse_grain_blif
 * -------------------------------------------------------------------------------------------*/
void YYosys::output_blif() {
#ifndef ODIN_USE_YOSYS
    error_message(PARSE_ARGS, unknown_location, "%s", YOSYS_INSTALLATION_ERROR);
#else
    /* must only be performed in the Yosys child process */
    oassert(this->yosys_pid == 0);

    // "-param" is to print non-standard cells parameters
    // "-impltf" is to not show the definition of primary netlist ports, i.e., VCC, GND and PAD, in the output.
    run_pass(std::string("write_blif -blackbox -param -impltf " + this->coarse_grain_blif));
#endif
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: re_initialize_odin_globals)
 * 
 * @brief Modify Odin-II global variables to activate the techmap flow
 * -------------------------------------------------------------------------------------------*/
void YYosys::re_initialize_odin_globals() {
    /* must NOT be performed in the Yosys child process */
    oassert(this->yosys_pid != 0);

    /* set Odin-II globals */
    global_args.coarsen.set(true, argparse::Provenance::SPECIFIED);
    global_args.blif_file.set(this->coarse_grain_blif, argparse::Provenance::SPECIFIED);
    /* modify Odin-II configurations to run through the techmap flow with a coarse-grain netlist */
    configuration.coarsen = true;
    coarsen_cleanup = true;
    /* erase list of inputs to add yosys generated BLIF instead */
    configuration.list_of_file_names.erase(
        configuration.list_of_file_names.begin(), configuration.list_of_file_names.end());
    /* add Yosys generated BLIF */
    configuration.list_of_file_names = {std::string(global_args.blif_file)};
    configuration.input_file_type = file_type_e::_BLIF;
}
