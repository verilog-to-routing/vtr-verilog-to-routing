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
#include "read_xml_config_file.h"
#include "odin_globals.h"
#include "odin_types.h"
#include "pugixml.hpp"
#include "read_xml_util.h"
#include "vtr_memory.h"
#include "vtr_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace pugiutil;

USING_YOSYS_NAMESPACE

config_t configuration;

void read_inputs(pugi::xml_node a_node, config_t *config, const pugiutil::loc_data &loc_data);
void read_outputs(pugi::xml_node a_node, const pugiutil::loc_data &loc_data);
void read_debug_switches(pugi::xml_node a_node, config_t *config, const pugiutil::loc_data &loc_data);
void read_optimizations(pugi::xml_node a_node, config_t *config, const pugiutil::loc_data &loc_data);
void set_default_optimization_settings(config_t *config);

extern HardSoftLogicMixer *mixer;

/*-------------------------------------------------------------------------
 * (function: read_config_file)
 * This reads an XML config file that specifies what we will do in the tool.
 *
 * See config_t in types.h to see the data structures used in this read.
 *-----------------------------------------------------------------------*/
void read_config_file(const char *file_name)
{
    pugi::xml_node config, next;

    /* Parse the xml file */
    oassert(file_name != NULL);

    pugi::xml_document doc;
    pugiutil::loc_data loc_data;
    try {
        loc_data = pugiutil::load_xml(doc, file_name);

        /* Root element should be config */
        config = get_single_child(doc, "config", loc_data);

        /* Process the verilog files */
        next = get_single_child(config, "inputs", loc_data);
        read_inputs(next, &configuration, loc_data);

        /* Process the output */
        next = get_single_child(config, "output", loc_data);
        read_outputs(next, loc_data);

        /* Process the optimizations */
        set_default_optimization_settings(&configuration);
        next = get_single_child(config, "optimizations", loc_data, OPTIONAL);
        if (next) {
            read_optimizations(next, &configuration, loc_data);
        }

        /* Process the debug switches */
        next = get_single_child(config, "debug_outputs", loc_data);
        read_debug_switches(next, &configuration, loc_data);

    } catch (XmlError &e) {
        log("error: could not parse xml configuration file '%s': %s\n", file_name, e.what());
        return;
    }

    /* Release the full XML tree */
    return;
}

/*-------------------------------------------------------------------------
 * (function: read_verilog_files)
 *-----------------------------------------------------------------------*/
void read_inputs(pugi::xml_node a_node, config_t *config, const pugiutil::loc_data &loc_data)
{
    pugi::xml_node child;
    pugi::xml_node junk;

    child = get_single_child(a_node, "input_type", loc_data, OPTIONAL);
    if (child != NULL) {
        file_type_strmap[child.child_value()];
    }

    child = get_first_child(a_node, "input_path_and_name", loc_data, OPTIONAL);
    while (child != NULL) {
        config->list_of_file_names.push_back(child.child_value());
        child = child.next_sibling(child.name());
    }
    return;
}

/*--------------------------------------------------------------------------
 * (function: read_outputs)
 *------------------------------------------------------------------------*/
void read_outputs(pugi::xml_node a_node, const pugiutil::loc_data &loc_data)
{
    pugi::xml_node child;

    child = get_single_child(a_node, "output_type", loc_data, OPTIONAL);
    if (child != NULL) {
        file_type_strmap[child.child_value()];
    }

    child = get_single_child(a_node, "output_path_and_name", loc_data, OPTIONAL);
    if (child != NULL) {
        global_args.output_file = child.child_value();
    }

    child = get_single_child(a_node, "target", loc_data, OPTIONAL);
    if (child != NULL) {
        child = get_single_child(child, "arch_file", loc_data, OPTIONAL);
        if (child != NULL) {
            /* Two arch files specified? */
            if (global_args.arch_file != "") {
                error_message(PARSE_ARGS, unknown_location, "%s", "Error: Arch file specified in config file AND command line\n");
            }
            global_args.arch_file = child.child_value();
        }
    }
    return;
}

/*--------------------------------------------------------------------------
 * (function: read_workload_generation)
 *------------------------------------------------------------------------*/
void read_debug_switches(pugi::xml_node a_node, config_t *config, const pugiutil::loc_data &loc_data)
{
    pugi::xml_node child;

    child = get_single_child(a_node, "output_ast_graphs", loc_data, OPTIONAL);

    child = get_single_child(a_node, "output_netlist_graphs", loc_data, OPTIONAL);
    if (child != NULL) {
        config->output_netlist_graphs = atoi(child.child_value());
    }

    child = get_single_child(a_node, "debug_output_path", loc_data, OPTIONAL);
    if (child != NULL) {
        config->debug_output_path = child.child_value();
    }

    child = get_single_child(a_node, "print_parse_tokens", loc_data, OPTIONAL);

    return;
}

/*--------------------------------------------------------------------------
 * (function: set_default_optimization_settings)
 *------------------------------------------------------------------------*/
void set_default_optimization_settings(config_t *config)
{
    config->min_hard_multiplier = 0;
    config->fixed_hard_multiplier = 0;
    config->mult_padding = -1; /* unconn */
    config->split_hard_multiplier = 1;
    config->split_memory_width = false;
    config->split_memory_depth = false;
    config->fixed_hard_adder = 0;
    config->min_threshold_adder = 0;
    return;
}

/*--------------------------------------------------------------------------
 * (function: read_optimizations)
 *------------------------------------------------------------------------*/
void read_optimizations(pugi::xml_node a_node, config_t *config, const pugiutil::loc_data &loc_data)
{
    const char *prop;
    pugi::xml_node child;

    child = get_single_child(a_node, "multiply", loc_data, OPTIONAL);
    if (child != NULL) {
        prop = get_attribute(child, "size", loc_data, OPTIONAL).as_string(NULL);
        if (prop != NULL) {
            config->min_hard_multiplier = atoi(prop);
        } else /* Default: No minimum hard multiply size */
            config->min_hard_multiplier = 0;

        prop = get_attribute(child, "padding", loc_data, OPTIONAL).as_string(NULL);
        if (prop != NULL) {
            config->mult_padding = atoi(prop);
        } else /* Default: Pad to hbpad pins */
            config->mult_padding = -1;

        prop = get_attribute(child, "fixed", loc_data, OPTIONAL).as_string(NULL);
        if (prop != NULL) {
            config->fixed_hard_multiplier = atoi(prop);
        } else /* Default: No fixed hard multiply size */
            config->fixed_hard_multiplier = 0;

        prop = get_attribute(child, "fracture", loc_data, OPTIONAL).as_string(NULL);
        if (prop != NULL) {
            config->split_hard_multiplier = atoi(prop);
        } else /* Default: use fractured hard multiply size */
            config->split_hard_multiplier = 1;
    }

    child = get_single_child(a_node, "mix_soft_hard_blocks", loc_data, OPTIONAL);
    if (child != NULL) {
        prop = get_attribute(child, "mults_ratio", loc_data, OPTIONAL).as_string(NULL);
        if (prop != NULL) {
            float ratio = atof(prop);
            if (ratio >= 0.0 && ratio <= 1.0) {
                delete mixer->_opts[MULTIPLY];
                mixer->_opts[MULTIPLY] = new MultsOpt(ratio);
            }
        }

        prop = get_attribute(child, "exact_mults", loc_data, OPTIONAL).as_string(NULL);
        if (prop != NULL) {
            int exact = atoi(prop);
            if (exact >= 0) {
                delete mixer->_opts[MULTIPLY];
                mixer->_opts[MULTIPLY] = new MultsOpt(exact);
            }
        }
    }

    child = get_single_child(a_node, "memory", loc_data, OPTIONAL);
    if (child != NULL) {
        prop = get_attribute(child, "split_memory_width", loc_data, OPTIONAL).as_string(NULL);
        if (prop != NULL) {
            config->split_memory_width = atoi(prop);
        } else /* Default: Do not split memory width! */
            config->split_memory_width = 0;

        prop = get_attribute(child, "split_memory_depth", loc_data, OPTIONAL).as_string(NULL);
        if (prop != NULL) {
            if (strcmp(prop, "min") == 0)
                config->split_memory_depth = -1;
            else if (strcmp(prop, "max") == 0)
                config->split_memory_depth = -2;
            else
                config->split_memory_depth = atoi(prop);
        } else /* Default: Do not split memory depth! */
            config->split_memory_depth = 0;
    }

    child = get_single_child(a_node, "adder", loc_data, OPTIONAL);
    if (child != NULL) {
        prop = get_attribute(child, "size", loc_data, OPTIONAL).as_string(NULL);

        prop = get_attribute(child, "threshold_size", loc_data, OPTIONAL).as_string(NULL);
        if (prop != NULL) {
            config->min_threshold_adder = atoi(prop);
        } else /* Default: No minimum hard adder size */
            config->min_threshold_adder = 0;

        prop = get_attribute(child, "padding", loc_data, OPTIONAL).as_string(NULL);

        prop = get_attribute(child, "fixed", loc_data, OPTIONAL).as_string(NULL);
        if (prop != NULL) {
            config->fixed_hard_adder = atoi(prop);
        } else /* Default: Fixed hard adder size */
            config->fixed_hard_adder = 1;

        prop = get_attribute(child, "fracture", loc_data, OPTIONAL).as_string(NULL);
    }

    return;
}