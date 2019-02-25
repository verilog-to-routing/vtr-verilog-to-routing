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
#include <stdarg.h>
#include "odin_types.h"
#include "odin_globals.h"
#include "read_xml_config_file.h"
#include "read_xml_util.h"
#include "pugixml.hpp"
#include "pugixml_util.hpp"
#include "vtr_util.h"
#include "vtr_memory.h"

using namespace pugiutil;

config_t configuration;
char empty_string[] = "";

void read_verilog_files(pugi::xml_node a_node, config_t *config, const pugiutil::loc_data& loc_data);
void read_outputs(pugi::xml_node a_node, config_t *config, const pugiutil::loc_data& loc_data);
void read_debug_switches(pugi::xml_node a_node, config_t *config, const pugiutil::loc_data& loc_data);
void read_optimizations(pugi::xml_node a_node, config_t *config, const pugiutil::loc_data& loc_data);
void set_default_optimization_settings(config_t *config);

/*-------------------------------------------------------------------------
 * (function: read_config_file)
 * This reads an XML config file that specifies what we will do in the tool.
 *
 * See config_t in types.h to see the data structures used in this read.
 *-----------------------------------------------------------------------*/
void read_config_file(char *file_name)
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
        next = get_single_child(config, "verilog_files", loc_data);
        read_verilog_files(next, &configuration, loc_data);

        /* Process the output */
        next = get_single_child(config, "output", loc_data);
        read_outputs(next, &configuration, loc_data);

        /* Process the optimizations */
        set_default_optimization_settings(&configuration);
        next = get_single_child(config, "optimizations", loc_data, OPTIONAL);
        if (next)
        {
            read_optimizations(next, &configuration, loc_data);
        }

        /* Process the debug switches */
        next = get_single_child(config, "debug_outputs", loc_data);
        read_debug_switches(next, &configuration, loc_data);
	} catch (XmlError& e) {
        printf("error: could not parse xml configuration file '%s'\n", file_name);
        return;
	}


	/* Release the full XML tree */
	return;
}

/*-------------------------------------------------------------------------
 * (function: read_verilog_files)
 *-----------------------------------------------------------------------*/
void read_verilog_files(pugi::xml_node a_node, config_t *config, const pugiutil::loc_data& loc_data)
{
	pugi::xml_node child;
	pugi::xml_node junk;

	child = get_first_child(a_node, "verilog_file", loc_data, OPTIONAL);
	while (child != NULL)
	{
		config->list_of_file_names.push_back(child.child_value());
		child = child.next_sibling(child.name());
	}
	return;
}

/*--------------------------------------------------------------------------
 * (function: read_outputs)
 *------------------------------------------------------------------------*/
void read_outputs(pugi::xml_node a_node, config_t *config, const pugiutil::loc_data& loc_data)
{
	pugi::xml_node child;

	child = get_single_child(a_node, "output_type", loc_data, OPTIONAL);
	if (child != NULL)
	{
		config->output_type = vtr::strdup(child.child_value());
	}

	child = get_single_child(a_node, "output_path_and_name", loc_data, OPTIONAL);
	if (child != NULL)
	{
		global_args.output_file.set(vtr::strdup(child.child_value()), argparse::Provenance::SPECIFIED);
	}

	child = get_single_child(a_node, "target", loc_data, OPTIONAL);
	if (child != NULL)
	{
		child = get_single_child(child, "arch_file", loc_data, OPTIONAL);
		if (child != NULL)
		{
			/* Two arch files specified? */
			if (global_args.arch_file != NULL)
			{
				error_message(ARG_ERROR, -1, -1, "%s", "Error: Arch file specified in config file AND command line\n");
			}
			global_args.arch_file.set(vtr::strdup(child.child_value()), argparse::Provenance::SPECIFIED);
		}
	}
	return;
}

/*--------------------------------------------------------------------------
 * (function: read_workload_generation)
 *------------------------------------------------------------------------*/
void read_debug_switches(pugi::xml_node a_node, config_t *config, const pugiutil::loc_data& loc_data)
{
	pugi::xml_node child;

	child = get_single_child(a_node, "output_ast_graphs", loc_data, OPTIONAL);
	if (child != NULL)
	{
		config->output_ast_graphs = atoi(child.child_value());
	}

	child = get_single_child(a_node, "output_netlist_graphs", loc_data, OPTIONAL);
	if (child != NULL)
	{
		config->output_netlist_graphs = atoi(child.child_value());
	}

	child = get_single_child(a_node, "debug_output_path", loc_data, OPTIONAL);
	if (child != NULL)
	{
		config->debug_output_path = std::string(child.child_value());
	}

	child = get_single_child(a_node, "print_parse_tokens", loc_data, OPTIONAL);
	if (child != NULL)
	{
		config->print_parse_tokens = atoi(child.child_value());
	}

	child = get_single_child(a_node, "output_preproc_source", loc_data, OPTIONAL);
	if (child != NULL)
	{
		config->output_preproc_source = atoi(child.child_value());
	}

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
	config->split_memory_width = FALSE;
	config->split_memory_depth = FALSE;
	config->min_hard_adder = 0;
	config->fixed_hard_adder = 0;
	config->split_hard_adder = 1;
	config->min_threshold_adder = 0;
	return;
}

/*--------------------------------------------------------------------------
 * (function: read_optimizations)
 *------------------------------------------------------------------------*/
void read_optimizations(pugi::xml_node a_node, config_t *config, const pugiutil::loc_data& loc_data)
{
	const char *prop;
	pugi::xml_node child;

	child = get_single_child(a_node, "multiply", loc_data, OPTIONAL);
	if (child != NULL)
	{
		prop = get_attribute(child, "size", loc_data, OPTIONAL).as_string(NULL);
		if (prop != NULL)
		{
			config->min_hard_multiplier = atoi(prop);
		}
		else /* Default: No minimum hard multiply size */
			config->min_hard_multiplier = 0;

		prop = get_attribute(child, "padding", loc_data, OPTIONAL).as_string(NULL);
		if (prop != NULL)
		{
			config->mult_padding = atoi(prop);
		}
		else /* Default: Pad to hbpad pins */
			config->mult_padding = -1;

		prop = get_attribute(child, "fixed", loc_data, OPTIONAL).as_string(NULL);
		if (prop != NULL)
		{
			config->fixed_hard_multiplier = atoi(prop);
		}
		else /* Default: No fixed hard multiply size */
			config->fixed_hard_multiplier = 0;

		prop = get_attribute(child, "fracture", loc_data, OPTIONAL).as_string(NULL);
		if (prop != NULL)
		{
			config->split_hard_multiplier = atoi(prop);
		}
		else /* Default: use fractured hard multiply size */
			config->split_hard_multiplier = 1;
	}

	child = get_single_child(a_node, "memory", loc_data, OPTIONAL);
	if (child != NULL)
	{
		prop = get_attribute(child, "split_memory_width", loc_data, OPTIONAL).as_string(NULL);
		if (prop != NULL)
		{
			config->split_memory_width = atoi(prop);
		}
		else /* Default: Do not split memory width! */
			config->split_memory_width = 0;

		prop = get_attribute(child, "split_memory_depth", loc_data, OPTIONAL).as_string(NULL);
		if (prop != NULL)
		{
			if (strcmp(prop, "min") == 0)
				config->split_memory_depth = -1;
			else if (strcmp(prop, "max") == 0)
				config->split_memory_depth = -2;
			else
				config->split_memory_depth = atoi(prop);
		}
		else /* Default: Do not split memory depth! */
			config->split_memory_depth = 0;

	}

	child = get_single_child(a_node, "adder", loc_data, OPTIONAL);
		if (child != NULL)
		{
			prop = get_attribute(child, "size", loc_data, OPTIONAL).as_string(NULL);
			if (prop != NULL)
			{
				config->min_hard_adder = atoi(prop);
			}
			else /* Default: No minimum hard adder size */
				config->min_hard_adder = 0;

			prop = get_attribute(child, "threshold_size", loc_data, OPTIONAL).as_string(NULL);
			if (prop != NULL)
			{
				config->min_threshold_adder = atoi(prop);
			}
			else /* Default: No minimum hard adder size */
				config->min_threshold_adder = 0;

			prop = get_attribute(child, "padding", loc_data, OPTIONAL).as_string(NULL);
			if (prop != NULL)
			{
				config->add_padding = atoi(prop);
			}
			else /* Default: Pad to hbpad pins */
				config->add_padding = -1;

			prop = get_attribute(child, "fixed", loc_data, OPTIONAL).as_string(NULL);
			if (prop != NULL)
			{
				config->fixed_hard_adder = atoi(prop);
			}
			else /* Default: Fixed hard adder size */
				config->fixed_hard_adder = 1;

			prop = get_attribute(child, "fracture", loc_data, OPTIONAL).as_string(NULL);
			if (prop != NULL)
			{
				config->split_hard_adder = atoi(prop);
			}
			else /* Default: use fractured hard adder size */
				config->split_hard_adder = 1;
		}

	return;
}

