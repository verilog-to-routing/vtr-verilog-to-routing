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
 * @file This file includes the defintion of basic structure used in 
 * Odin-II to run Yosys API. Technically, this class utilize Yosys 
 * as a seperate synthesizer to generate a coarse-grain netlist. 
 * In the end, the Yosys generated netlist is technology mapped to the
 * target device using Odin-II partial mapper. It worth noting that
 * Yosys performs corase-grain synthesis using the TCL config script
 * located at $ODIN_II_ROOT/regression_test/tool/synth.tcl
 * Users can also generate only coarse-grain the netlist BLIF file using
 * run_yosys.sh script located at the same directory.
 */

#include <cstdlib>
#include "Yosys.hpp"
#include "config_t.h"     // configuration
#include "odin_util.h"    // get_directory
#include "odin_error.h"   // error_message
#include "odin_globals.h" // global_args
#include "vtr_util.h"     // vtr::getline
#include "vtr_memory.h"   // vtr::free

/**
 * @brief Construct the Yosys object
 * required by compiler
 */
Yosys::Yosys() {
    /* to check if Yosys is installed */
    FILE* fp = run_cmd(this->which_yosys.c_str(), "r");
    char* retval = NULL;
    vtr::getline(retval, fp);
    int exit_code = WEXITSTATUS(pclose(fp));

    if (exit_code == 0) {
        /* remove newline */
        retval[strcspn(retval, "\n")] = 0;
        /* get Yosys executable path */
        this->executable = std::string(retval);
    } else {
        error_message(PARSE_ARGS, unknown_location,
                      "Yosys is not installed on the current system, the installation instruction is specified on (%s)", YOSYS_GITHUB_URL);
    }

    /**
     * specify the path to default TCL file.
     * defualt TCL file is located at:
     * "$ODIN_ROOT/regression_test/tools/synth.tcl"
     */
    this->tcl
        = (!configuration.tcl_file.empty())
              ? configuration.tcl_file                                                      // user specified tcl file
              : std::string(global_args.program_root + "/regression_test/tools/synth.tcl"); // default tcl file

    std::string path_prefix = (global_args.output_file.provenance() == argparse::Provenance::SPECIFIED)
                                  ? get_directory(global_args.output_file.value())
                                  : global_args.current_path;
    /* to handle unwanted root access */
    if (path_prefix == "")
        path_prefix = global_args.current_path;

    this->log = std::string(path_prefix + "/" + "yosys_elaboration.log");
    this->blif = std::string(path_prefix + "/" + "yosys_netlist.blif");

    // CLEAN UP
    vtr::free(retval);
}

/**
 * @brief Destruct the Yosys object
 * to avoid memory leakage
 */
Yosys::~Yosys() = default;

/**
 * ---------------------------------------------------------------------------------------------
 * (function: perform_elaboration)
 * 
 * @brief Call Yosys elaboration and set Odin-II input BLIF file
 * to the Yosys output BLIF
 * -------------------------------------------------------------------------------------------*/
void Yosys::perform_elaboration() {
    /* synthesizer validation */
    oassert(configuration.elaborator_type == elaborator_e::_YOSYS);

    if (configuration.tcl_file.empty()) {
        /* init environment varibles of the default script */
        this->set_default_env();
    }

    /* perform elaboration using run_yosys scriptt */
    this->elaborate();
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: set_default_env)
 * 
 * @brief set local environment variables of the default TCL script
 * -------------------------------------------------------------------------------------------*/
void Yosys::set_default_env() {
    /* init environment varibles of the default script */
    this->tcl_primitives = std::string(global_args.program_root + "/../vtr_flow/primitives.v");
    this->tcl_circuit = configuration.list_of_file_names[0]; // [TODO] adding multiple input Verilog files
    this->odin_techlib = std::string(global_args.program_root + "/techlib");

    /* set default tcl script environment variiables */
    set_env("PRIMITIVES", this->tcl_primitives.c_str(), 1);
    set_env("TCL_CIRCUIT", this->tcl_circuit.c_str(), 1);
    set_env("ODIN_TECHLIB", this->odin_techlib.c_str(), 1);
    set_env("TCL_BLIF", this->blif.c_str(), 1);
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: execute)
 * 
 * @brief Run Yosys executable with the class TCL script file
 * -------------------------------------------------------------------------------------------*/
void Yosys::execute() {
    std::string yosys_full_command = this->executable + " -c " + this->tcl + " > " + this->log;

    FILE* fp = run_cmd(yosys_full_command.c_str(), "r");
    auto status = pclose(fp);
    int exit_code = WEXITSTATUS(status);

    printf("Yosys log file file can be found at (%s)\n", this->log.c_str());

    if (exit_code != 0)
        throw vtr::VtrError("Yosys failed to perform elaboration\n");
}

/**
 * ---------------------------------------------------------------------------------------------
 * (function: elaborate)
 * 
 * @brief Call yosys using the specified TCL script to generate
 * a coarse-grain netlist
 * -------------------------------------------------------------------------------------------*/
void Yosys::elaborate() {
    try {
        this->execute();
        printf("Successful Elaboration of digital design by Yosys\n\tCoarse-grain netlist is available at: %s\n\n", this->blif.c_str());

        /* set globals */
        global_args.coarsen.set(true, argparse::Provenance::SPECIFIED);
        global_args.blif_file.set(this->blif, argparse::Provenance::SPECIFIED);
        /* modify Odin-II configurations to run through the techmap flow with coarse-grain netlist */
        configuration.coarsen = true;
        coarsen_cleanup = true;
        /* erase list of inputs to add yosys generated BLIF instead */
        configuration.list_of_file_names.erase(
            configuration.list_of_file_names.begin(), configuration.list_of_file_names.end());
        /* add yosys generated BLIF */
        configuration.list_of_file_names = {std::string(global_args.blif_file)};
        configuration.input_file_type = file_type_e::_BLIF;

    } catch (vtr::VtrError& vtr_error) {
        error_message(PARSE_ARGS, unknown_location, "%s", vtr_error.what());
    }
}
