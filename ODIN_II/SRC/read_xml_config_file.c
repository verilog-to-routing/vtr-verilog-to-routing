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
#include "types.h"
#include "globals.h"
#include "ezxml.h"
#include "read_xml_config_file.h"
#include "read_xml_util.h"

config_t configuration;

void read_verilog_files(ezxml_t a_node, config_t *config);
void read_outputs(ezxml_t a_node, config_t *config);
void read_debug_switches(ezxml_t a_node, config_t *config);
void read_optimizations(ezxml_t a_node, config_t *config);
void set_default_optimization_settings(config_t *config);

/*-------------------------------------------------------------------------
 * (function: read_config_file)
 * This reads an XML config file that specifies what we will do in the tool.
 *
 * See config_t in types.h to see the data structures used in this read.
 *-----------------------------------------------------------------------*/
void read_config_file(char *file_name)
{
	ezxml_t doc, next;

	/* Parse the xml file */
	oassert(file_name != NULL);
	doc = ezxml_parse_file(file_name);

	if (doc == NULL) 
	{
		printf("error: could not parse xml configuration file\n");
		return;
	}

	/* Root element should be config */
	CheckElement(doc, "config");

	/* Process the verilog files */
	next = FindElement(doc, "verilog_files", (boolean)TRUE);
	read_verilog_files(next, &configuration);
	FreeNode(next);

	/* Process the output */
	next = FindElement(doc, "output", (boolean)TRUE);
	read_outputs(next, &configuration);
	FreeNode(next);

	/* Process the optimizations */
	set_default_optimization_settings(&configuration);
	next = FindElement(doc, "optimizations", (boolean)FALSE);
	if (next)
	{
		read_optimizations(next, &configuration);
		FreeNode(next);		
	}

	/* Process the debug switches */
	next = FindElement(doc, "debug_outputs", (boolean)TRUE);
	read_debug_switches(next, &configuration);
	FreeNode(next);

	/* Release the full XML tree */
	FreeNode(doc);
	return;
}

/*-------------------------------------------------------------------------
 * (function: read_verilog_files)
 *-----------------------------------------------------------------------*/
void read_verilog_files(ezxml_t a_node, config_t *config)
{
	ezxml_t child;
	ezxml_t junk;

	child = ezxml_child(a_node, "verilog_file");
	while (child != NULL)
	{
		if (global_args.verilog_file == NULL)
		{
			config->list_of_file_names = (char**)realloc(config->list_of_file_names, sizeof(char*)*(config->num_list_of_file_names+1));
			config->list_of_file_names[config->num_list_of_file_names] = strdup(ezxml_txt(child));
			config->num_list_of_file_names ++;
		}
		ezxml_set_txt(child, "");
		junk = child;
		child = ezxml_next(child);
		FreeNode(junk);
	}
	return;
}

/*--------------------------------------------------------------------------
 * (function: read_outputs)
 *------------------------------------------------------------------------*/
void read_outputs(ezxml_t a_node, config_t *config)
{
	ezxml_t child;

	child = ezxml_child(a_node, "output_type");
	if (child != NULL)
	{
		config->output_type = strdup(ezxml_txt(child));
		ezxml_set_txt(child, "");
		FreeNode(child);
	}

	child = ezxml_child(a_node, "output_path_and_name");
	if (child != NULL)
	{
		global_args.output_file = strdup(ezxml_txt(child));
		ezxml_set_txt(child, "");
		FreeNode(child);
	}

	child = ezxml_child(a_node, "target");
	if (child != NULL)
	{
		ezxml_t junk = child;

		child = ezxml_child(child, "arch_file");
		if (child != NULL)
		{
			/* Two arch files specified? */
			if (global_args.arch_file != NULL) 
			{
				printf("Error: Arch file specified in config file AND command line\n");
				exit(-1);
			}
			global_args.arch_file = strdup(ezxml_txt(child));
			ezxml_set_txt(child, "");
			FreeNode(child);
		}
		FreeNode(junk);
	}
	return;
}

/*--------------------------------------------------------------------------
 * (function: read_workload_generation)
 *------------------------------------------------------------------------*/
void read_debug_switches(ezxml_t a_node, config_t *config)
{
	ezxml_t child;

	child = ezxml_child(a_node, "output_ast_graphs");
	if (child != NULL)
	{
		config->output_ast_graphs = atoi(ezxml_txt(child));
		ezxml_set_txt(child, "");
		FreeNode(child);
	}

	child = ezxml_child(a_node, "output_netlist_graphs");
	if (child != NULL)
	{
		config->output_netlist_graphs = atoi(ezxml_txt(child));
		ezxml_set_txt(child, "");
		FreeNode(child);
	}

	child = ezxml_child(a_node, "debug_output_path");
	if (child != NULL)
	{
		config->debug_output_path = strdup(ezxml_txt(child));
		ezxml_set_txt(child, "");
		FreeNode(child);
	}

	child = ezxml_child(a_node, "print_parse_tokens");
	if (child != NULL)
	{
		config->print_parse_tokens = atoi(ezxml_txt(child));
		ezxml_set_txt(child, "");
		FreeNode(child);
	}

	child = ezxml_child(a_node, "output_preproc_source");
	if (child != NULL)
	{
		config->output_preproc_source = atoi(ezxml_txt(child));
		ezxml_set_txt(child, "");
		FreeNode(child);
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
void read_optimizations(ezxml_t a_node, config_t *config)
{
	const char *prop;
	ezxml_t child;

	child = ezxml_child(a_node, "multiply");
	if (child != NULL)
	{
		prop = FindProperty(child, "size", (boolean)FALSE);
		if (prop != NULL)
		{
			config->min_hard_multiplier = atoi(prop);
			ezxml_set_attr(child, "size", NULL);
		}
		else /* Default: No minimum hard multiply size */
			config->min_hard_multiplier = 0;
		
		prop = FindProperty(child, "padding", (boolean)FALSE);
		if (prop != NULL)
		{
			config->mult_padding = atoi(prop);
			ezxml_set_attr(child, "padding", NULL);
		}
		else /* Default: Pad to hbpad pins */
			config->mult_padding = -1;

		prop = FindProperty(child, "fixed", (boolean)FALSE);
		if (prop != NULL)
		{
			config->fixed_hard_multiplier = atoi(prop);
			ezxml_set_attr(child, "fixed", NULL);
		}
		else /* Default: No fixed hard multiply size */
			config->fixed_hard_multiplier = 0;

		prop = FindProperty(child, "fracture", (boolean)FALSE);
		if (prop != NULL)
		{
			config->split_hard_multiplier = atoi(prop);
			ezxml_set_attr(child, "fracture", NULL);
		}
		else /* Default: use fractured hard multiply size */
			config->split_hard_multiplier = 1;
		FreeNode(child);
	}

	child = ezxml_child(a_node, "memory");
	if (child != NULL)
	{
		prop = FindProperty(child, "split_memory_width", (boolean)FALSE);
		if (prop != NULL)
		{
			config->split_memory_width = atoi(prop);
			ezxml_set_attr(child, "split_memory_width", NULL);
		}
		else /* Default: Do not split memory width! */
			config->split_memory_width = 0;
		
		prop = FindProperty(child, "split_memory_depth", (boolean)FALSE);
		if (prop != NULL)
		{
			if (strcmp(prop, "min") == 0)
				config->split_memory_depth = -1;
			else if (strcmp(prop, "max") == 0)
				config->split_memory_depth = -2;
			else
				config->split_memory_depth = atoi(prop);
			ezxml_set_attr(child, "split_memory_depth", NULL);
		}
		else /* Default: Do not split memory depth! */
			config->split_memory_depth = 0;
		
		FreeNode(child);
	}

	child = ezxml_child(a_node, "adder");
		if (child != NULL)
		{
			prop = FindProperty(child, "size", (boolean)FALSE);
			if (prop != NULL)
			{
				config->min_hard_adder = atoi(prop);
				ezxml_set_attr(child, "size", NULL);
			}
			else /* Default: No minimum hard adder size */
				config->min_hard_adder = 0;

			prop = FindProperty(child, "threshold_size", (boolean)FALSE);
			if (prop != NULL)
			{
				config->min_threshold_adder = atoi(prop);
				ezxml_set_attr(child, "threshold_size", NULL);
			}
			else /* Default: No minimum hard adder size */
				config->min_threshold_adder = 0;

			prop = FindProperty(child, "padding", (boolean)FALSE);
			if (prop != NULL)
			{
				config->add_padding = atoi(prop);
				ezxml_set_attr(child, "padding", NULL);
			}
			else /* Default: Pad to hbpad pins */
				config->add_padding = -1;

			prop = FindProperty(child, "fixed", (boolean)FALSE);
			if (prop != NULL)
			{
				config->fixed_hard_adder = atoi(prop);
				ezxml_set_attr(child, "fixed", NULL);
			}
			else /* Default: Fixed hard adder size */
				config->fixed_hard_adder = 1;

			prop = FindProperty(child, "fracture", (boolean)FALSE);
			if (prop != NULL)
			{
				config->split_hard_adder = atoi(prop);
				ezxml_set_attr(child, "fracture", NULL);
			}
			else /* Default: use fractured hard adder size */
				config->split_hard_adder = 1;
			FreeNode(child);
		}

	return;
}

