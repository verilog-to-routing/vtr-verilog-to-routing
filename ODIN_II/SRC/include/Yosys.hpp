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

#ifndef __YOSYS_H__
#define __YOSYS_H__

#include <string>
#include <stdlib.h>

/* to set local environment variable */
#ifdef WIN32
#    define set_env _setenv
#    define run_cmd _popen
#else
#    define set_env setenv
#    define run_cmd popen
#endif

#define YOSYS_GITHUB_URL "https://github.com/YosysHQ/yosys.git"

/**
 * @brief A class to provide the general object of Yosys synthezier
 */
class Yosys {
  public:
    /**
     * @brief Construct the object
     * required by compiler
     */
    Yosys();
    /**
     * @brief Destruct the object
     * to avoid memory leakage
     */
    ~Yosys();

    /**
    * ---------------------------------------------------------------------------------------------
    * (function: perform_elaboration)
    * 
    * @brief Call Yosys elaboration and set Odin-II input BLIF file
    * to the Yosys output BLIF
    * -------------------------------------------------------------------------------------------*/
    void perform_elaboration();

  protected:
    std::string tcl;
    std::string log;
    std::string blif;
    std::string executable;
    const char* which_yosys = std::string("which yosys").c_str();

  private:
    std::string tcl_primitives;
    std::string tcl_circuit;
    std::string odin_techlib;

    /**
    * ---------------------------------------------------------------------------------------------
    * (function: set_default_env)
    * 
    * @brief set local environment variables of the default TCL script
    * -------------------------------------------------------------------------------------------*/
    void set_default_env();
    /**
    * ---------------------------------------------------------------------------------------------
    * (function: execute)
    * 
    * @brief Run Yosys executable with the class TCL script file
    * -------------------------------------------------------------------------------------------*/
    void execute();
    /**
    * ---------------------------------------------------------------------------------------------
    * (function: elaborate)
    * 
    * @brief Call yosys using the specified TCL script to generate
    * a coarse-grain netlist
    * -------------------------------------------------------------------------------------------*/
    void elaborate();
};

#endif //__VERILOG_H__
