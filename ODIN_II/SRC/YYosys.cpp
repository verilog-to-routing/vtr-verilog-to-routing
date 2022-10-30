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
        /* check if Yosys exited abnormally */
        if (!WIFEXITED(yosys_status)) {
            if (WIFSIGNALED(yosys_status)) {
                error_message(PARSER, unknown_location,
                              "Yosys exited with signal %d - %s",
                              WTERMSIG(yosys_status),
                              YOSYS_ELABORATION_ERROR);
            } else if (WIFSTOPPED(yosys_status)) {
                error_message(PARSER, unknown_location,
                              "Yosys stopped with signal %d - %s",
                              WSTOPSIG(yosys_status),
                              YOSYS_ELABORATION_ERROR);
            } else {
                error_message(PARSER, unknown_location, "%s",
                              "Something strange just happened with Yosys child process.\n");
            }
        } else {
            auto yosys_exit_status = WEXITSTATUS(yosys_status);
            if (yosys_exit_status != 0)
                error_message(PARSER, unknown_location,
                              "Yosys exited with status %d - %s",
                              yosys_exit_status,
                              YOSYS_ELABORATION_ERROR);
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

#    ifdef YOSYS_SV_UHDM_PLUGIN
    /* Load SystemVerilog/UHDM plugins in the Yosys frontend */
    run_pass(std::string("plugin -i systemverilog"));
#    endif

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
        std::string aggregated_circuits;
        for (auto circuit : this->verilog_circuits)
            aggregated_circuits += circuit + " ";
            // Read Verilog/SystemVerilog/UHDM files based on their type, considering the SystemVerilog/UHDM plugins
#    ifdef YOSYS_SV_UHDM_PLUGIN
        /* Load SystemVerilog/UHDM plugins in the Yosys frontend */
        switch (configuration.input_file_type) {
            case (file_type_e::_VERILOG): // fallthrough
            case (file_type_e::_VERILOG_HEADER): {
                run_pass(std::string("read_verilog -sv -nolatches " + aggregated_circuits));
                break;
            }
            case (file_type_e::_SYSTEM_VERILOG): {
                run_pass(std::string("read_systemverilog -debug " + aggregated_circuits));
                break;
            }
            case (file_type_e::_UHDM): {
                run_pass(std::string("read_uhdm -debug " + aggregated_circuits));
                break;
            }
            default: {
                error_message(UTIL, unknown_location,
                              "Invalid file type (%s) for Yosys+Odin-II synthesizer.", file_extension_strmap[configuration.input_file_type]);
            }
        }
#    else
        run_pass(std::string("read_verilog -sv -nolatches " + aggregated_circuits));
#    endif
        // Check whether cells match libraries and find top module
        if (global_args.top_level_module_name.provenance() == argparse::Provenance::SPECIFIED) {
            run_pass(std::string("hierarchy -check -top " + global_args.top_level_module_name.value() + " -purge_lib"));
        } else {
            run_pass(std::string("hierarchy -check -auto-top -purge_lib"));
        }

        /* 
         * Translate processes to netlist components such as MUXs, FFs and latches
         * Transform the design into a new one with single top module
         */
        run_pass(std::string("proc; flatten; opt_expr; opt_clean;"));
        /* 
         * Looking for combinatorial loops, wires with multiple drivers and used wires without any driver.
         * "-nodffe" to disable dff -> dffe conversion, and other transforms recognizing clock enable
         * "-nosdff" to disable dff -> sdff conversion, and other transforms recognizing sync resets
         */
        run_pass(std::string("check; opt -nodffe -nosdff;"));
        // Extraction and optimization of finite state machines
        run_pass(std::string("fsm; opt;"));
        // To possibly reduce word sizes by Yosys
        run_pass(std::string("wreduce;"));
        // To applies a collection of peephole optimizers to the current design.
        run_pass(std::string("peepopt; opt_clean;"));
        /* 
         * To merge shareable resources into a single resource. A SAT solver
         * is used to determine if two resources are share-able
         */
        run_pass(std::string("share; opt;"));

        // Use a readable name convention
        // [NOTE]: the 'autoname' process has a high memory footprint for giant netlists
        // we run it after basic optimization passes to reduce the overhead (see issue #2031)
        run_pass(std::string("autoname"));

        // Looking for combinatorial loops, wires with multiple drivers and used wires without any driver.
        run_pass(std::string("check"));
        // Transform asynchronous dffs to synchronous dffs using techlib files provided by Yosys
        run_pass(std::string("techmap -map " + this->odin_techlib + "/adff2dff.v"));
        run_pass(std::string("techmap -map " + this->odin_techlib + "/adffe2dff.v"));

        // Yosys performs various optimizations on memories in the design. Then, it detects DFFs at
        // memory read ports and merges them into the memory port. I.e. it consumes an asynchronous
        // memory port and the flip-flops at its interface and yields a synchronous memory port.
        // Afterwards, Yosys detects cases where an asynchronous read port is only connected via a mux
        // tree to a write port with the same address. When such a connection is found, it is replaced
        // with a new condition on an enable signal, allowing for removal of the read port. Finally
        // Yosys collects memories, their port and create multiport memory cells.
        run_pass(std::string("opt_mem; memory_dff; opt_clean; opt_mem_feedback; opt_clean; memory_collect;"));

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

        /* 
         * Transforming all RTLIL components into LUTs except for memories, adders, subtractors, 
         * multipliers, DFFs with set (VCC) and clear (GND) signals, and DFFs with the set (VCC),
         * clear (GND), and enable signals The Odin-II partial mapper will perform the technology
         * mapping for the above-mentioned circuits
         * 
         * [NOTE]: the purpose of using this pass is to keep the connectivity of internal signals  
         *         in the coarse-grained BLIF file, as they were not properly connected in the 
         *         initial implementation of Yosys+Odin-II, which did not use this pass
         */
        run_pass(std::string("techmap -autoproc */t:$mem */t:$memrd */t:$add */t:$sub */t:$mul */t:$dffsr */t:$dffsre */t:$sr */t:$dlatch */t:$adlatch %% %n"));

        // Transform the design into a new one with single top module
        run_pass(std::string("flatten"));

        // To possibly reduce word sizes by Yosys and fine-graining the basic operations
        run_pass(std::string("wreduce; simplemap */t:$dffsr */t:$dffsre */t:$sr */t:$dlatch */t:$adlatch %% %n;"));
        // Turn all DFFs into simple latches
        run_pass(std::string("dffunmap; opt -fast -noff;"));

        // Check the hierarchy for any unknown modules, and purge all modules (including blackboxes) that aren't used
        run_pass(std::string("hierarchy -check -purge_lib"));
        // "-undriven" to ensure there is no wire without drive
        // "-noff" option to remove all sdffXX and etc. Only dff will remain
        // "opt_muxtree" removes dead branches, "opt_expr" performs const folding and
        // removes "undef" from mux inputs and replace muxes with buffers and inverters
        run_pass(std::string("opt -undriven -full -noff; opt_muxtree; opt_expr -mux_undef -mux_bool -fine;;;"));
        // Use a readable name convention
        run_pass(std::string("autoname;"));
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
