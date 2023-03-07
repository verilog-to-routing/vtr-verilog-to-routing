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
#ifndef _ODIN_GLOBALS_H_
#define _ODIN_GLOBALS_H_

#include "config_t.h"
#include "hard_soft_logic_mixer.h"
#include "hash_table.h"
#include "odin_types.h"
#include "read_xml_arch_file.h"
#include "string_cache.h"

/**
 * The cutoff for the number of netlist nodes.
 * Technically, Odin-II prints statistics for
 * netlist nodes that the total number of them
 * is greater than this value.
 */
constexpr long long UNUSED_NODE_TYPE = 0;

extern global_args_t global_args;
extern config_t configuration;
extern loc_t my_location;

extern nnode_t *gnd_node;
extern nnode_t *vcc_node;
extern nnode_t *pad_node;

extern char *one_string;
extern char *zero_string;
extern char *pad_string;

extern t_arch Arch;
extern short physical_lut_size;

/* logic optimization mixer, once ODIN is classy, could remove that
 * and pass as member variable
 */
extern HardSoftLogicMixer *mixer;

/**
 * a global var to specify the need for cleanup after
 * receiving a coarsen BLIF file as the input.
 */
extern bool coarsen_cleanup;

extern strmap<file_type_e> file_type_strmap;
extern strmap<operation_list> yosys_subckt_strmap;

#endif // _ODIN_GLOBALS_H_
