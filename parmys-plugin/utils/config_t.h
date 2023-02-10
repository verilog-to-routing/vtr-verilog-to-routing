/*
 * Copyright 2022 CAS—Atlantic (University of New Brunswick, CASA)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _CONFIG_T_H_
#define _CONFIG_T_H_

#include "odin_types.h"
#include <string>
#include <vector>

/* This is the data structure that holds config file details */
struct config_t {
    std::vector<std::string> list_of_file_names;

    std::string debug_output_path; // path for where to output the debug outputs
    std::string dsp_verilog;       // path for the output Verilog file including target DSPs' declaration
    bool coarsen;                  // Specify if the input BLIF is coarse-grain

    bool output_netlist_graphs; // switch that outputs netlist graphs per node for use with GraphViz tools

    int min_hard_multiplier; // threshold from hard to soft logic
    int mult_padding;        // setting how multipliers are padded to fit fixed size
    // Flag for fixed or variable hard mult (1 or 0)
    int fixed_hard_multiplier;
    // Flag for splitting hard multipliers If fixed_hard_multiplier is set, this must be 1.
    int split_hard_multiplier;
    // 1 to split memory width down to a size of 1. 0 to split to arch width.
    char split_memory_width;
    // Set to a positive integer to split memory depth to that address width. 0 to split to arch width.
    int split_memory_depth;

    // Flag for fixed or variable hard mult (1 or 0)
    int fixed_hard_adder;
    //  Threshold from hard to soft logic
    int min_threshold_adder;
    // defines if the first cin of an adder/subtractor is connected to a global gnd/vdd
    // or generated using a dummy adder with both inputs set to gnd/vdd
    bool adder_cin_global;

    // If the memory is smaller than both of these, it will be converted to soft logic.
    int soft_logic_memory_depth_threshold;
    int soft_logic_memory_width_threshold;

    std::string arch_file; // Name of the FPGA architecture file
    std::string tcl_file;  // TCL file to be run by yosys
};

extern config_t configuration;

#endif // _CONFIG_T_H_
