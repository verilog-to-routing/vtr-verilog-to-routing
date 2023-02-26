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
#include <sstream>

#include "odin_ii.h"

#include "odin_globals.h"
#include "odin_types.h"

#include "hard_soft_logic_mixer.h"
#include "vtr_path.h"

#define DEFAULT_OUTPUT "."

loc_t my_location;

t_arch Arch;
global_args_t global_args;
short physical_lut_size = -1;
HardSoftLogicMixer *mixer;

/* CONSTANT NET ELEMENTS */
char *one_string;
char *zero_string;
char *pad_string;

/*---------------------------------------------------------------------------
 * (function: set_default_options)
 *-------------------------------------------------------------------------*/
void set_default_config()
{
    /* Set up the global configuration. */
    configuration.coarsen = false;
    configuration.tcl_file = "";
    configuration.output_netlist_graphs = 0;
    // TODO: unused
    configuration.debug_output_path = std::string(DEFAULT_OUTPUT);
    configuration.dsp_verilog = "arch_dsp.v";
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
