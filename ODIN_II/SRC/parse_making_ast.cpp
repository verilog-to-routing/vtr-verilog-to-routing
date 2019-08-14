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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <sstream>
#include "odin_globals.h"
#include "odin_types.h"
#include "odin_util.h"
#include "ast_util.h"
#include "parse_making_ast.h"
#include "string_cache.h"
#include "verilog_bison_user_defined.h"
#include "verilog_preprocessor.h"
#include "hard_blocks.h"
#include "vtr_util.h"
#include "vtr_memory.h"

extern int yylineno;

//for module
STRING_CACHE **defines_for_module_sc;
STRING_CACHE *modules_inputs_sc;
STRING_CACHE *modules_outputs_sc;
STRING_CACHE *module_instances_sc;

//for function
STRING_CACHE **defines_for_function_sc;
STRING_CACHE *functions_inputs_sc;
STRING_CACHE *functions_outputs_sc;

STRING_CACHE *module_names_to_idx;
STRING_CACHE *instantiated_modules;

ast_node_t **module_variables_not_defined;
int size_module_variables_not_defined;
ast_node_t **function_instantiations_instance;
int size_function_instantiations;
ast_node_t **function_instantiations_instance_by_module;
int size_function_instantiations_by_module;

long num_modules;
ast_node_t **ast_modules;

int num_functions;
ast_node_t **ast_functions;
ast_node_t **all_file_items_list;
int size_all_file_items_list;

short to_view_parse;

/*
 * File-scope function declarations
 */
ast_node_t *newFunctionAssigning(ast_node_t *expression1, ast_node_t *expression2, int line_number);
ast_node_t *resolve_ports(ids top_type, ast_node_t *symbol_list);

/*---------------------------------------------------------------------------------------------
 * (function: parse_to_ast)
 *-------------------------------------------------------------------------------------------*/
void parse_to_ast()
{
	extern FILE *yyin;
	extern int yylineno;

	/* hooks into macro at the top of verilog_flex.l that shows the tokens as they're parsed.  Set to true if you want to see it go...*/
	to_view_parse = configuration.print_parse_tokens;

	/* initialize the parser */
	init_veri_preproc();

	/* read all the files in the configuration file */
	current_parse_file =0;
	while (current_parse_file < configuration.list_of_file_names.size())
	{
		assert_supported_file_extension(configuration.list_of_file_names[current_parse_file], current_parse_file);

		yyin = fopen(configuration.list_of_file_names[current_parse_file].c_str(), "r");
		if (yyin == NULL)
		{
			error_message(ARG_ERROR, -1, -1, "cannot open file: %s\n", configuration.list_of_file_names[current_parse_file].c_str());
		}

		yyin = veri_preproc(yyin);

		/* reset the line count */
		yylineno = 0;

		/* setup the local parser structures for a file */
		init_parser_for_file();
		/* parse next file */
		yyparse();

		fclose(yyin);
		current_parse_file++;
	}

	/* clean up all the structures in the parser */
	cleanup_veri_preproc();

	/* for error messages - this is in case we use any of the parser functions after parsing (i.e. create_case_control_signals()) */
	current_parse_file = -1;
}

/* --------------------------------------------------------------------------------------------
 --------------------------------------------------------------------------------------------
 --------------------------------------------------------------------------------------------
BASIC PARSING FUNCTIONS
 Assume that all `defines are constants so we can change the constant into a number (see def_reduct by performing a search in this file)
 --------------------------------------------------------------------------------------------
 --------------------------------------------------------------------------------------------
 --------------------------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------------------------
 * (function: init_parser)
 *-------------------------------------------------------------------------------------------*/
void init_parser()
{

	defines_for_module_sc = NULL;

	defines_for_function_sc = NULL;
	/* record of each of the individual modules */
	num_modules = 0; // we're going to record all the modules in a list so we can build a tree of them later
	num_functions = 0;
	ast_modules = NULL;
	ast_functions = NULL;
	function_instantiations_instance = NULL;
	module_variables_not_defined = NULL;
	size_module_variables_not_defined = 0;
  	size_function_instantiations = 0;
  	function_instantiations_instance_by_module = NULL;
	size_function_instantiations_by_module = 0;

	/* keeps track of all the ast roots */
	all_file_items_list = NULL;
	size_all_file_items_list = 0;
}

/*---------------------------------------------------------------------------------------------
 * (function: cleanup_parser)
 *-------------------------------------------------------------------------------------------*/
void cleanup_parser()
{
	long i;

	/* frees all the defines for module string caches (used for parameters) */

	for (i = 0; i <= num_modules; i++)
	{
		sc_free_string_cache(defines_for_module_sc[i]);
	}

	vtr::free(defines_for_module_sc);

}

/*---------------------------------------------------------------------------------------------
 * (function: init_parser_for_file)
 *-------------------------------------------------------------------------------------------*/
void init_parser_for_file()
{
	/* crrate a hash for defines so we can look them up when we find them */
	defines_for_module_sc = (STRING_CACHE**)vtr::realloc(defines_for_module_sc, sizeof(STRING_CACHE*)*(num_modules+1));
	defines_for_module_sc[num_modules] = sc_new_string_cache();

	/* create string caches to hookup PORTS with INPUT and OUTPUTs.  This is made per module and will be cleaned and remade at next_module */
	modules_inputs_sc = sc_new_string_cache();
	modules_outputs_sc = sc_new_string_cache();
	functions_inputs_sc = sc_new_string_cache();
	functions_outputs_sc = sc_new_string_cache();
	instantiated_modules = sc_new_string_cache();
	module_instances_sc = sc_new_string_cache();
}

/*---------------------------------------------------------------------------------------------
 * (function: clean_up_parser_for_file)
 *-------------------------------------------------------------------------------------------*/
void cleanup_parser_for_file()
{
	/* create string caches to hookup PORTS with INPUT and OUTPUTs.  This is made per module and will be cleaned and remade at next_module */
	modules_inputs_sc = sc_free_string_cache(modules_inputs_sc);
	modules_outputs_sc = sc_free_string_cache(modules_outputs_sc);
	functions_inputs_sc = sc_free_string_cache(functions_inputs_sc);
	functions_outputs_sc = sc_free_string_cache(functions_outputs_sc);
	instantiated_modules = sc_free_string_cache(instantiated_modules);
	module_instances_sc = sc_free_string_cache(module_instances_sc);
}

/*---------------------------------------------------------------------------------------------
 * (function: next_parsed_verilog_file)
 *-------------------------------------------------------------------------------------------*/
void next_parsed_verilog_file(ast_node_t *file_items_list)
{
	long i;
	/* optimization entry point */
	printf("Optimizing module by AST based optimizations\n");
	if (configuration.output_ast_graphs == 1)
	{
		/* IF - we want outputs for the graphViz files of each module */
		for (i = 0; i < file_items_list->num_children; i++)
		{
			assert(file_items_list->children[i]);
			graphVizOutputAst(configuration.debug_output_path, file_items_list->children[i]);
		}
	}

	/* store the root of this files ast */
	all_file_items_list = (ast_node_t**)vtr::realloc(all_file_items_list, sizeof(ast_node_t*)*(size_all_file_items_list+1));
	all_file_items_list[size_all_file_items_list] = file_items_list;
	size_all_file_items_list ++;
}

/*---------------------------------------------------------------------------------------------
 * (function: newSymbolNode)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newSymbolNode(char *id, int line_number)
{
	return create_tree_node_id(id, line_number, current_parse_file);

}

/*---------------------------------------------------------------------------------------------
 * (function: newNumberNode)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newNumberNode(char *num, int line_number)
{
	ast_node_t *current_node = create_tree_node_number(num, line_number, current_parse_file);
	vtr::free(num);
	return current_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newList)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newList(ids node_type, ast_node_t *child)
{
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(node_type, yylineno, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { child });

	return new_node;
}

ast_node_t *newfunctionList(ids node_type, ast_node_t *child)
{
    /* create a output node for this array reference that is going to be the first child */
   	ast_node_t* output_node = create_node_w_type(IDENTIFIERS, yylineno, current_parse_file);
    /* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(node_type, yylineno, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { output_node, child });

    return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newList_entry)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newList_entry(ast_node_t *list, ast_node_t *child)
{
	/* allocate child nodes to this node */
	add_child_to_node(list, child);

	return list;
}

/*---------------------------------------------------------------------------------------------
 * (function: newListReplicate)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newListReplicate(ast_node_t *exp, ast_node_t *child)
{
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(REPLICATE, yylineno, current_parse_file);

	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { exp, child });

	return new_node;
}

static ast_node_t *resolve_symbol_node(ids top_type, ast_node_t *symbol_node)
{
	ast_node_t *to_return = NULL;

	switch(symbol_node->type)
	{
		case IDENTIFIERS:
		{
			ast_node_t *newNode = NULL;
			if(top_type == MODULE) 
			{
				long sc_spot = sc_lookup_string(defines_for_module_sc[num_modules], symbol_node->types.identifier);
				if(sc_spot != -1)
					newNode = (ast_node_t *)defines_for_module_sc[num_modules]->data[sc_spot];
			}
       		else if(top_type == FUNCTION) 
			{
				long sc_spot = sc_lookup_string(defines_for_function_sc[num_functions], symbol_node->types.identifier);
				if (sc_spot != -1)
					newNode = (ast_node_t *)defines_for_function_sc[num_functions]->data[sc_spot];
			}

			if (newNode && newNode->types.variable.is_parameter == true)
			{
				to_return = symbol_node;
			}
			else
			{
				error_message(PARSE_ERROR, symbol_node->line_number, current_parse_file,
					"no match for parameter %s\n", symbol_node->types.identifier);
			}
			break;
		}
		case NUMBERS:
		case BINARY_OPERATION:
		case UNARY_OPERATION:
		{
			to_return = symbol_node;
			break;
		}
		default:
		{
			to_return = NULL;
			break;
		}
	}

	return to_return;
}

ast_node_t *resolve_ports(ids top_type, ast_node_t *symbol_list)
{
	ast_node_t *unprocessed_ports = NULL;
	long sc_spot;

	for (long i = 0; i < symbol_list->num_children; i++)
	{
		if (symbol_list->children[i]->types.variable.is_port)
		{
			ast_node_t *this_port = symbol_list->children[i];

			if (!unprocessed_ports)
			{
				unprocessed_ports = newList(VAR_DECLARE_LIST, this_port);
			} 
			else 
			{
				unprocessed_ports = newList_entry(unprocessed_ports, this_port);
			}

			/* grab and update all typeless ports immediately following this one */
			long j = 0;
			for (j = i + 1; j < symbol_list->num_children && !(symbol_list->children[j]->types.variable.is_port); j++)
			{
				/* port type */
				symbol_list->children[j]->types.variable.is_input = this_port->types.variable.is_input;
				symbol_list->children[j]->types.variable.is_output = this_port->types.variable.is_output;
				symbol_list->children[j]->types.variable.is_inout = this_port->types.variable.is_inout;

				/* net type */
				symbol_list->children[j]->types.variable.is_wire = this_port->types.variable.is_wire;
				symbol_list->children[j]->types.variable.is_reg = this_port->types.variable.is_reg;
				symbol_list->children[j]->types.variable.is_integer = this_port->types.variable.is_integer;

				/* signedness */
				symbol_list->children[j]->types.variable.is_signed = this_port->types.variable.is_signed;

				/* range */
				if (symbol_list->children[j]->children[1] == NULL)
				{
					symbol_list->children[j]->children[1] = ast_node_deep_copy(this_port->children[1]);
					symbol_list->children[j]->children[2] = ast_node_deep_copy(this_port->children[2]);
					symbol_list->children[j]->children[3] = ast_node_deep_copy(this_port->children[3]);
					symbol_list->children[j]->children[4] = ast_node_deep_copy(this_port->children[4]);

					if (this_port->num_children == 8)
					{
						symbol_list->children[j]->children = (ast_node_t**) realloc(symbol_list->children[j]->children, sizeof(ast_node_t*)*8);
						symbol_list->children[j]->children[7] = ast_node_deep_copy(symbol_list->children[j]->children[5]);
						symbol_list->children[j]->children[5] = ast_node_deep_copy(this_port->children[5]);
						symbol_list->children[j]->children[6] = ast_node_deep_copy(this_port->children[6]);
					}
				}

				/* error checking */
				symbol_list->children[j] = markAndProcessPortWith(MODULE, NO_ID, NO_ID, symbol_list->children[j], this_port->types.variable.is_signed);
			}
		}
		else
		{
			if (top_type == MODULE)
			{
				/* find the related INPUT or OUTPUT definition and store that instead */
				if ((sc_spot = sc_lookup_string(modules_inputs_sc, symbol_list->children[i]->children[0]->types.identifier)) != -1)
				{
					oassert(((ast_node_t*)modules_inputs_sc->data[sc_spot])->type == VAR_DECLARE);
					free_whole_tree(symbol_list->children[i]);
					symbol_list->children[i] = (ast_node_t*)modules_inputs_sc->data[sc_spot];
					oassert(symbol_list->children[i]->types.variable.is_input);
				}
				else if ((sc_spot = sc_lookup_string(modules_outputs_sc, symbol_list->children[i]->children[0]->types.identifier)) != -1)
				{
					oassert(((ast_node_t*)modules_outputs_sc->data[sc_spot])->type == VAR_DECLARE);
					free_whole_tree(symbol_list->children[i]);
					symbol_list->children[i] = (ast_node_t*)modules_outputs_sc->data[sc_spot];
					oassert(symbol_list->children[i]->types.variable.is_output);
				}
				else
				{
					error_message(PARSE_ERROR, symbol_list->children[i]->line_number, current_parse_file, "No matching declaration for port %s\n", symbol_list->children[i]->children[0]->types.identifier);
				}
			}
			else if (top_type == FUNCTION)
			{
				/* find the related INPUT or OUTPUT definition and store that instead */
				if ((sc_spot = sc_lookup_string(functions_inputs_sc, symbol_list->children[i]->children[0]->types.identifier)) != -1)
				{
					oassert(((ast_node_t*)functions_inputs_sc->data[sc_spot])->type == VAR_DECLARE);
					free_whole_tree(symbol_list->children[i]);
					symbol_list->children[i] = (ast_node_t*)functions_inputs_sc->data[sc_spot];
					oassert(symbol_list->children[i]->types.variable.is_input);
				}
				else if ((sc_spot = sc_lookup_string(functions_outputs_sc, symbol_list->children[i]->children[0]->types.identifier)) != -1)
				{
					oassert(((ast_node_t*)functions_outputs_sc->data[sc_spot])->type == VAR_DECLARE);
					free_whole_tree(symbol_list->children[i]);
					symbol_list->children[i] = (ast_node_t*)functions_outputs_sc->data[sc_spot];
					oassert(symbol_list->children[i]->types.variable.is_output);
				}
				else
				{
					error_message(PARSE_ERROR, symbol_list->children[i]->line_number, current_parse_file, "No matching declaration for port %s\n", symbol_list->children[i]->children[0]->types.identifier);
				}
			}

			symbol_list->children[i]->types.variable.is_port = true;
		}
	}

	return unprocessed_ports;
}

ast_node_t *markAndProcessPortWith(ids top_type, ids port_id, ids net_id, ast_node_t *port, bool is_signed)
{
	long sc_spot;
	STRING_CACHE *this_inputs_sc = NULL;
	STRING_CACHE *this_outputs_sc = NULL;
	const char *top_type_name = (top_type == MODULE) ? "Module" : "Function";
	ids temp_net_id = NO_ID;

	if (port->types.variable.is_port)
	{
		oassert(false && "Port was already marked");
	}

	if (top_type == MODULE)
	{
		this_inputs_sc = modules_inputs_sc;
		this_outputs_sc = modules_outputs_sc;

	}
	else if (top_type == FUNCTION)
	{
		this_inputs_sc = functions_inputs_sc;
		this_outputs_sc = functions_outputs_sc;
	}

	/* look for processed inputs with this name */
	sc_spot = sc_lookup_string(this_inputs_sc, port->children[0]->types.identifier);
	if (sc_spot > -1 && ((ast_node_t*)this_inputs_sc->data[sc_spot])->types.variable.is_port)
	{
		error_message(PARSE_ERROR, port->line_number, current_parse_file, "%s already has input with this name %s\n", 
			top_type_name, ((ast_node_t*)this_inputs_sc->data[sc_spot])->children[0]->types.identifier);
	}

	/* look for processed outputs with this name */
	sc_spot = sc_lookup_string(this_outputs_sc, port->children[0]->types.identifier);
	if (sc_spot > -1 && ((ast_node_t*)this_outputs_sc->data[sc_spot])->types.variable.is_port)
	{
		error_message(PARSE_ERROR, port->line_number, current_parse_file, "%s already has output with this name %s\n", 
			top_type_name, ((ast_node_t*)this_outputs_sc->data[sc_spot])->children[0]->types.identifier);
	}

	switch (net_id)
	{
		case REG:
			if (port_id == INPUT)
			{
				error_message(NETLIST_ERROR, port->line_number, port->file_number, "%s",
									"Input cannot be defined as a reg\n");
			}
			if (port_id == INOUT)
			{
				error_message(NETLIST_ERROR, port->line_number, port->file_number, "%s",
									"Inout cannot be defined as a reg\n");
			}
			port->types.variable.is_reg = true;
			port->types.variable.is_wire = false;
			port->types.variable.is_integer = false;
			break;

		case INTEGER:
			if (port_id == INPUT)
			{
				error_message(NETLIST_ERROR, port->line_number, port->file_number, "%s",
									"Input cannot be defined as an integer\n");
			}
			else if (port_id == INOUT)
			{
				error_message(NETLIST_ERROR, port->line_number, port->file_number, "%s",
									"Inout cannot be defined as an integer\n");
			}
			else if (port_id == OUTPUT)
			{
				/* cannot support signed ports right now (integers are signed) */
				error_message(PARSE_ERROR, port->line_number, current_parse_file, 
						"Odin does not handle signed ports (%s)\n", port->children[0]->types.identifier);
			}
			port->types.variable.is_integer = true;
			port->types.variable.is_reg = false;
			port->types.variable.is_wire = false;
			break;

		case WIRE:
			if ((port->num_children == 6 && port->children[5] != NULL)
				|| (port->num_children == 8 && port->children[7] != NULL))
			{
				error_message(NETLIST_ERROR, port->line_number, port->file_number, "%s",
									"Ports of type net cannot be initialized\n");
			}
			port->types.variable.is_wire = true;
			port->types.variable.is_reg = false;
			port->types.variable.is_integer = false;
			break;

		default:
			if ((port->num_children == 6 && port->children[5] != NULL)
				|| (port->num_children == 8 && port->children[7] != NULL))
			{
				error_message(NETLIST_ERROR, port->line_number, port->file_number, "%s",
									"Ports with undefined type cannot be initialized\n");
			}

			if (port->types.variable.is_reg 
				&& !(port->types.variable.is_wire) 
				&& !(port->types.variable.is_integer))  
			{
				temp_net_id = REG;
			}
			else if (port->types.variable.is_integer
				&& !(port->types.variable.is_wire)
				&& !(port->types.variable.is_reg)) 
			{
				temp_net_id = INTEGER;
			}
			else if (port->types.variable.is_wire
				&& !(port->types.variable.is_reg) 
				&& !(port->types.variable.is_integer))
			{
				temp_net_id = WIRE;
			}
			else
			{
				/* port cannot have more than one type */
				oassert(!(port->types.variable.is_wire) &&
						!(port->types.variable.is_reg) &&
						!(port->types.variable.is_integer));
			}

			if (net_id == NO_ID)
			{
				/* will be marked later */
				net_id = temp_net_id;
			}

			break;
	}

	switch (port_id)
	{
		case INPUT:
			port->types.variable.is_input = true;
			port->types.variable.is_output = false;
			port->types.variable.is_inout = false;

			/* add this input to the modules string cache */
			sc_spot = sc_add_string(this_inputs_sc, port->children[0]->types.identifier);

			/* store the data which is an idx here */
			this_inputs_sc->data[sc_spot] = (void*)port;

			break;

		case OUTPUT:
			port->types.variable.is_output = true;
			port->types.variable.is_input = false;
			port->types.variable.is_inout = false;

			/* add this output to the modules string cache */
			sc_spot = sc_add_string(this_outputs_sc, port->children[0]->types.identifier);

			/* store the data which is an idx here */
			this_outputs_sc->data[sc_spot] = (void*)port;

			break;

		case INOUT:
			port->types.variable.is_inout = true;
			port->types.variable.is_input = false;
			port->types.variable.is_output = false;
			error_message(PARSE_ERROR, port->line_number, current_parse_file, "Odin does not handle inouts (%s)\n", port->children[0]->types.identifier);
			break;

		default:
			if (port->types.variable.is_input 
				&& !(port->types.variable.is_output) 
				&& !(port->types.variable.is_inout))  
			{
				port = markAndProcessPortWith(top_type, INPUT, net_id, port, is_signed);
			}
			else if (port->types.variable.is_output
				&& !(port->types.variable.is_input)
				&& !(port->types.variable.is_inout)) 
			{
				port = markAndProcessPortWith(top_type, OUTPUT, net_id, port, is_signed);
			}
			else if (port->types.variable.is_inout
				&& !(port->types.variable.is_input)
				&& !(port->types.variable.is_output)) 
			{
				error_message(PARSE_ERROR, port->line_number, current_parse_file, "Odin does not handle inouts (%s)\n", port->children[0]->types.identifier);
				port = markAndProcessPortWith(top_type, INOUT, net_id, port, is_signed);
			}
			else
			{
				// shouldn't ever get here...
				oassert(port->types.variable.is_input
						|| port->types.variable.is_output
						|| port->types.variable.is_inout);
			}

			break;
	}

	if (is_signed)
	{
		/* cannot support signed ports right now */
		error_message(PARSE_ERROR, port->line_number, current_parse_file, 
				"Odin does not handle signed ports (%s)\n", port->children[0]->types.identifier);
	}

	port->types.variable.is_signed = is_signed;
	port->types.variable.is_port = true;

	return port;
}

ast_node_t *markAndProcessParameterWith(ids top_type, ids id, ast_node_t *parameter, bool is_signed)
{
	oassert((top_type == MODULE || top_type == FUNCTION) 
		&& "can only use MODULE or FUNCTION as top type");
		
	long sc_spot;
	STRING_CACHE **this_defines_sc = NULL;
	long this_num_modules = 0;

	if (top_type == MODULE)
	{
		this_defines_sc = defines_for_module_sc;
		this_num_modules = num_modules;

	}
	else if (top_type == FUNCTION)
	{
		this_defines_sc = defines_for_function_sc;
		this_num_modules = num_functions;
	}

	oassert(this_defines_sc);
	oassert(this_defines_sc[this_num_modules]);

	/* create an entry in the symbol table for this parameter */
	if ((sc_spot = sc_lookup_string(this_defines_sc[this_num_modules], parameter->children[0]->types.identifier)) > -1)
	{
		error_message(PARSE_ERROR, parameter->children[5]->line_number, current_parse_file, 
				"Module already has parameter with this name (%s)\n", parameter->children[0]->types.identifier);
	}
	sc_spot = sc_add_string(this_defines_sc[this_num_modules], parameter->children[0]->types.identifier);
	
	this_defines_sc[this_num_modules]->data[sc_spot] = (void*)parameter->children[5];


	if (id == PARAMETER)
	{
		parameter->children[5]->types.variable.is_parameter = true;
		parameter->types.variable.is_parameter = true;
	}
	else if (id == LOCALPARAM)
	{
		parameter->children[5]->types.variable.is_localparam = true;
		parameter->types.variable.is_localparam = true;
	}

	if (is_signed)
	{
		/* cannot support signed parameters right now */
		error_message(PARSE_ERROR, parameter->line_number, current_parse_file, 
				"Odin does not handle signed parameters (%s)\n", parameter->children[0]->types.identifier);
	}

	parameter->children[5]->types.variable.is_signed = is_signed;
	parameter->types.variable.is_signed = is_signed;

	return parameter;
}

ast_node_t *markAndProcessSymbolListWith(ids top_type, ids id, ast_node_t *symbol_list, bool is_signed)
{
	long i;
	ast_node_t *range_min = NULL;
	ast_node_t *range_max = NULL;


	if(symbol_list)
	{
		if(symbol_list->children[0] && symbol_list->children[0]->children[1])
		{
			range_max = resolve_symbol_node(top_type, symbol_list->children[0]->children[1]);
			range_min = resolve_symbol_node(top_type, symbol_list->children[0]->children[2]);
		}

		for (i = 0; i < symbol_list->num_children; i++)
		{

			if ((symbol_list->children[i]->children[1] == NULL) && (symbol_list->children[i]->children[2] == NULL))
			{
				symbol_list->children[i]->children[1] = ast_node_deep_copy(range_max);
				symbol_list->children[i]->children[2] = ast_node_deep_copy(range_min);
			}
			
			if(top_type == MODULE) {

				switch(id)
				{
					case PARAMETER:
					case LOCALPARAM:
					{
						markAndProcessParameterWith(top_type, id, symbol_list->children[i], is_signed);
						break;
					}
					case INPUT:
					case OUTPUT:
					case INOUT:
						symbol_list->children[i] = markAndProcessPortWith(top_type, id, NO_ID, symbol_list->children[i], is_signed);
						break;
					case WIRE:
						if (is_signed)
						{
							/* cannot support signed nets right now */
							error_message(PARSE_ERROR, symbol_list->children[i]->line_number, current_parse_file, 
									"Odin does not handle signed nets (%s)\n", symbol_list->children[i]->children[0]->types.identifier);
						}
						symbol_list->children[i]->types.variable.is_wire = true;
						break;
					case REG:
						if (is_signed)
						{
							/* cannot support signed regs right now */
							error_message(PARSE_ERROR, symbol_list->children[i]->line_number, current_parse_file, 
									"Odin does not handle signed regs (%s)\n", symbol_list->children[i]->children[0]->types.identifier);
						}
						symbol_list->children[i]->types.variable.is_reg = true;
						break;
					case INTEGER:
						oassert(is_signed && "Integers must always be signed");
						symbol_list->children[i]->types.variable.is_signed = is_signed;			
						symbol_list->children[i]->types.variable.is_integer = true;
						break;
					case GENVAR:
						oassert(is_signed && "Genvars must always be signed");
						symbol_list->children[i]->types.variable.is_signed = is_signed;			
						symbol_list->children[i]->types.variable.is_genvar = true;
						break;
					default:
						oassert(false);
				}
			}
			else if(top_type == FUNCTION) {

				switch(id)
				{
					case PARAMETER:
					case LOCALPARAM:
					{
						markAndProcessParameterWith(top_type, id, symbol_list->children[i], is_signed);
						break;
					}
					case INPUT:
					case OUTPUT:
					case INOUT:
						symbol_list->children[i] = markAndProcessPortWith(top_type, id, NO_ID, symbol_list->children[i], is_signed);
						break;
					case WIRE:
						if ((symbol_list->children[i]->num_children == 6 && symbol_list->children[i]->children[5] != NULL)
							|| (symbol_list->children[i]->num_children == 8 && symbol_list->children[i]->children[7] != NULL))
						{
							error_message(NETLIST_ERROR, symbol_list->children[i]->line_number, symbol_list->children[i]->file_number, "%s",
									"Nets cannot be initialized\n");
						}
						if (is_signed)
						{
							/* cannot support signed nets right now */
							error_message(PARSE_ERROR, symbol_list->children[i]->line_number, current_parse_file, 
									"Odin does not handle signed nets (%s)\n", symbol_list->children[i]->children[0]->types.identifier);
						}
						symbol_list->children[i]->types.variable.is_wire = true;
						break;
					case REG:
						if (is_signed)
						{
							/* cannot support signed regs right now */
							error_message(PARSE_ERROR, symbol_list->children[i]->line_number, current_parse_file, 
									"Odin does not handle signed regs (%s)\n", symbol_list->children[i]->children[0]->types.identifier);
						}
						symbol_list->children[i]->types.variable.is_reg = true;
						break;
					case INTEGER:
						oassert(is_signed && "Integers must always be signed");
						symbol_list->children[i]->types.variable.is_signed = is_signed;
						symbol_list->children[i]->types.variable.is_integer = true;
						break;
					default:
						oassert(false);
				}
			}

		}
	}

	return symbol_list;
}
/*---------------------------------------------------------------------------------------------
 * (function: newArrayRef)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newArrayRef(char *id, ast_node_t *expression, int line_number)
{
	/* allocate or check if there's a node for this */
	ast_node_t *symbol_node = newSymbolNode(id, line_number);
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(ARRAY_REF, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { symbol_node, expression });

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newRangeRef)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newRangeRef(char *id, ast_node_t *expression1, ast_node_t *expression2, int line_number)
{
	/* allocate or check if there's a node for this */
	ast_node_t *symbol_node = newSymbolNode(id, line_number);
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(RANGE_REF, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { symbol_node, expression1, expression2 });

	return new_node;
}


/*---------------------------------------------------------------------------------------------
 * (function: newPartSelectRangeRef)
 * 
 * NB!! only support [msb:lsb], will always resolve to this syntax
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newMinusColonRangeRef(char *id, ast_node_t *expression1, ast_node_t *expression2, int line_number)
{
	ast_node_t *msb = NULL;
	ast_node_t *lsb = NULL;

	if ( expression1 == NULL )
	{
		error_message(PARSE_ERROR, line_number, current_parse_file, 
			"first expression for range ref is NULL %s", id);
	}
	else if ( expression2 == NULL )
	{
		error_message(PARSE_ERROR, line_number, current_parse_file, 
			"first expression for range ref is NULL  %s", id);
	}
	
	// expression 1 is the msb here since we subtract expression 2 from it
	msb = expression1;

	ast_node_t *number_one = create_tree_node_number(1L, line_number, current_parse_file);
	ast_node_t *size_to_index = newBinaryOperation(MINUS, expression2, number_one, line_number);

	lsb = newBinaryOperation(MINUS, ast_node_deep_copy(expression1), size_to_index, line_number);


	return newRangeRef(id, msb, lsb, line_number);
}

/*---------------------------------------------------------------------------------------------
 * (function: newPartSelectRangeRef)
 * 
 * NB!! only support [msb:lsb], will always resolve to this syntax
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newPlusColonRangeRef(char *id, ast_node_t *expression1, ast_node_t *expression2, int line_number)
{
	ast_node_t *msb = NULL;
	ast_node_t *lsb = NULL;

	if ( expression1 == NULL )
	{
		error_message(PARSE_ERROR, line_number, current_parse_file, 
			"first expression for range ref is NULL %s", id);
	}
	else if ( expression2 == NULL )
	{
		error_message(PARSE_ERROR, line_number, current_parse_file, 
			"first expression for range ref is NULL  %s", id);
	}
	
	// expression 1 is the lsb here since we add expression 2 to it
	lsb = expression1;

	ast_node_t *number_one = create_tree_node_number(1L, line_number, current_parse_file);
	ast_node_t *size_to_index = newBinaryOperation(MINUS, expression2, number_one, line_number);

	msb = newBinaryOperation(ADD, ast_node_deep_copy(expression1), size_to_index, line_number);

	return newRangeRef(id, msb, lsb, line_number);
}


/*---------------------------------------------------------------------------------------------
 * (function: newBinaryOperation)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newBinaryOperation(operation_list op_id, ast_node_t *expression1, ast_node_t *expression2, int line_number)
{
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(BINARY_OPERATION, line_number, current_parse_file);
	/* store the operation type */
	new_node->types.operation.op = op_id;
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { expression1, expression2 });

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newUnaryOperation)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newUnaryOperation(operation_list op_id, ast_node_t *expression, int line_number)
{
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(UNARY_OPERATION, line_number, current_parse_file);
	/* store the operation type */
	new_node->types.operation.op = op_id;
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { expression });

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newNegedgeSymbol)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newNegedgeSymbol(char *symbol, int line_number)
{
	/* get the symbol node */
	ast_node_t *symbol_node = newSymbolNode(symbol, line_number);
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(NEGEDGE, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { symbol_node });

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newPosedgeSymbol)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newPosedgeSymbol(char *symbol, int line_number)
{
	/* get the symbol node */
	ast_node_t *symbol_node = newSymbolNode(symbol, line_number);
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(POSEDGE, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { symbol_node });

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newCaseItem)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newCaseItem(ast_node_t *expression, ast_node_t *statement, int line_number)
{
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(CASE_ITEM, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { expression, statement });

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newDefaultCase)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newDefaultCase(ast_node_t *statement, int line_number)
{
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(CASE_DEFAULT, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { statement });

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newNonBlocking)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newParallelConnection(ast_node_t *expression1, ast_node_t *expression2, int line_number)
{
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(SPECIFY_PAL_CONNECTION_STATEMENT, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { expression1, expression2 });

	return new_node;
}
/*---------------------------------------------------------------------------------------------
 * (function: newNonBlocking)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newNonBlocking(ast_node_t *expression1, ast_node_t *expression2, int line_number)
{
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(NON_BLOCKING_STATEMENT, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { expression1, expression2 });

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newInitial)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newInitial(ast_node_t *expression1, int line_number)
{
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(INITIALS, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { expression1 });

	return new_node;
}
/*---------------------------------------------------------------------------------------------
 * (function: newBlocking)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newBlocking(ast_node_t *expression1, ast_node_t *expression2, int line_number)
{
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(BLOCKING_STATEMENT, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { expression1, expression2 });

	return new_node;
}
/*---------------------------------------------------------------------------------------------
 * (function: newBlocking)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newFunctionAssigning(ast_node_t *expression1, ast_node_t *expression2, int line_number)
{
    char *label = vtr::strdup(expression1->types.identifier);

	ast_node_t *node = newSymbolNode(label,line_number);

    expression2->children[1]->children[1]->children[0] = newModuleConnection(NULL,  node, line_number);

	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(BLOCKING_STATEMENT, line_number, current_parse_file);

    /* allocate child nodes to this node */
	allocate_children_to_node(new_node, { expression1, expression2 });

    return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newFor)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newFor(ast_node_t *initial, ast_node_t *compare_expression, ast_node_t *terminal, ast_node_t *statement, int line_number)
{
	/* create a node for this for reference */
	ast_node_t* new_node = create_node_w_type(FOR, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { initial, compare_expression, terminal, statement });

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newWhile)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newWhile(ast_node_t *compare_expression, ast_node_t *statement, int line_number)
{
	/* create a node for this for reference */
	ast_node_t* new_node = create_node_w_type(WHILE, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { compare_expression, statement });

	/* This needs to be removed once support is added */
	error_message(PARSE_ERROR, line_number, current_parse_file, "%s", "While statements are NOT supported");
	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newIf)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newIf(ast_node_t *compare_expression, ast_node_t *true_expression, ast_node_t *false_expression, int line_number)
{
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(IF, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { compare_expression, true_expression, false_expression });

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newIfQuestion) for f = a ? b : c;
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newIfQuestion(ast_node_t *compare_expression, ast_node_t *true_expression, ast_node_t *false_expression, int line_number)
{
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(IF_Q, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { compare_expression, true_expression, false_expression });

	return new_node;
}
/*---------------------------------------------------------------------------------------------
 * (function: newCase)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newCase(ast_node_t *compare_expression, ast_node_t *case_list, int line_number)
{
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(CASE, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { compare_expression, case_list });

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newAlways)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newAlways(ast_node_t *delay_control, ast_node_t *statement, int line_number)
{
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(ALWAYS, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { delay_control, statement });

	return new_node;
}
/*---------------------------------------------------------------------------------------------
 * (function: newModuleConnection)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newModuleConnection(char* id, ast_node_t *expression, int line_number)
{
	ast_node_t *symbol_node;
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(MODULE_CONNECT, line_number, current_parse_file);
	if (id != NULL)
	{
		symbol_node = newSymbolNode(id, line_number);
	}
	else
	{
		symbol_node = NULL;
	}

	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { symbol_node, expression });

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newModuleParameter)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newModuleParameter(char* id, ast_node_t *expression, int line_number)
{
	ast_node_t *symbol_node = NULL;
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(MODULE_PARAMETER, line_number, current_parse_file);
	if (id != NULL)
	{
		symbol_node = newSymbolNode(id, line_number);
	}

	/* allocate child nodes to this node */
	// leave 4 blank nodes so that expression is the 6th node to behave just like
	// a default var_declare parameter (see create_symbol_table_for_module)
	allocate_children_to_node(new_node, { symbol_node, NULL, NULL, NULL, NULL, expression });

	// set is_parameter for the same reason
	new_node->types.variable.is_parameter = true;

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newModuleNamedInstance)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newModuleNamedInstance(char* unique_name, ast_node_t *module_connect_list, ast_node_t *module_parameter_list, int line_number)
{
	ast_node_t *symbol_node = newSymbolNode(unique_name, line_number);

	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(MODULE_NAMED_INSTANCE, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { symbol_node, module_connect_list, module_parameter_list });

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newModuleNamedInstance)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newFunctionNamedInstance(ast_node_t *module_connect_list, ast_node_t *module_parameter_list, int line_number)
{
	std::string buffer("function_instance_");
	buffer += std::to_string(size_function_instantiations_by_module);

	char *unique_name = vtr::strdup(buffer.c_str());

    ast_node_t *symbol_node = newSymbolNode(unique_name, line_number);

    /* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(FUNCTION_NAMED_INSTANCE, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { symbol_node, module_connect_list, module_parameter_list });

	return new_node;
}
/*-------------------------------------------------------------------------
 * (function: newHardBlockInstance)
 *-----------------------------------------------------------------------*/
ast_node_t *newHardBlockInstance(char* module_ref_name, ast_node_t *module_named_instance, int line_number)
{
	ast_node_t *symbol_node = newSymbolNode(module_ref_name, line_number);

	// create a node for this array reference
	ast_node_t* new_node = create_node_w_type(HARD_BLOCK, line_number, current_parse_file);
	// allocate child nodes to this node
	allocate_children_to_node(new_node, { symbol_node, module_named_instance });

	// mark as hard block
	module_named_instance->type = HARD_BLOCK_NAMED_INSTANCE;

	ast_node_t *connect_list_node = module_named_instance->children[1];
	connect_list_node->type = HARD_BLOCK_CONNECT_LIST;

	for (int i = 0; i < connect_list_node->num_children; i++)
	{
		connect_list_node->children[i]->type = HARD_BLOCK_CONNECT;
	}

	return new_node;
}

/*-------------------------------------------------------------------------
 * (function: newModuleInstance)
 *-----------------------------------------------------------------------*/
ast_node_t *newModuleInstance(char* module_ref_name, ast_node_t *module_named_instance, int line_number)
{
	long i;
	/* create a node for this array reference */
	ast_node_t* new_master_node = create_node_w_type(MODULE_INSTANCE, line_number, current_parse_file);
	for(i = 0; i < module_named_instance->num_children; i++)
	{
		/* check if this name was already used */
		long sc_spot = sc_add_string(module_instances_sc, module_named_instance->children[i]->children[0]->types.identifier);
		if (module_instances_sc->data[sc_spot] != NULL)
		{
			error_message(PARSE_ERROR, line_number, current_parse_file, 
				"Module already has an instance with this name (%s)\n", 
				module_named_instance->children[i]->children[0]->types.identifier);
		}
		module_instances_sc->data[sc_spot] = module_named_instance->children[i];

		ast_node_t *symbol_node = newSymbolNode(module_ref_name, line_number);

		/* create a node for this array reference */
		ast_node_t* new_node = create_node_w_type(MODULE_INSTANCE, line_number, current_parse_file);
		/* allocate child nodes to this node */
			allocate_children_to_node(new_node, { symbol_node, module_named_instance->children[i] });
			if(i == 0) allocate_children_to_node(new_master_node, { new_node });
			else add_child_to_node(new_master_node,new_node);

		/* store the module symbol name that this calls in a list that will at the end be asociated with the module node */
		sc_add_string(instantiated_modules, module_ref_name);

		/* if this module has already been parsed, update */
		for (int j = 0; j < num_modules; j++)
		{
			if (sc_lookup_string(instantiated_modules, ast_modules[j]->children[0]->types.identifier) != -1)
			{
				ast_modules[j]->types.module.is_instantiated = true;
			}
		}
	}

	vtr::free(module_named_instance->children);
	vtr::free(module_named_instance);
	return new_master_node;
}

/*-------------------------------------------------------------------------
 * (function: newFunctionInstance)
 *-----------------------------------------------------------------------*/
ast_node_t *newFunctionInstance(char* function_ref_name, ast_node_t *function_named_instance, int line_number)
{
	ast_node_t *symbol_node = newSymbolNode(function_ref_name, line_number);

    /* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(FUNCTION_INSTANCE, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { symbol_node, function_named_instance });

	/* store the module symbol name that this calls in a list that will at the end be asociated with the module node */
	function_instantiations_instance_by_module = (ast_node_t **)vtr::realloc(function_instantiations_instance_by_module, sizeof(ast_node_t*)*(size_function_instantiations_by_module+1));
	function_instantiations_instance_by_module[size_function_instantiations_by_module] = new_node;
	size_function_instantiations_by_module++;

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newGateInstance)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newGateInstance(char* gate_instance_name, ast_node_t *expression1, ast_node_t *expression2, ast_node_t *expression3, int line_number)
{
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(GATE_INSTANCE, line_number, current_parse_file);
	ast_node_t *symbol_node = NULL;

	if (gate_instance_name != NULL)
	{
		symbol_node = newSymbolNode(gate_instance_name, line_number);
	}

	char *newChar = vtr::strdup(expression1->types.identifier);
	ast_node_t *newVar = newVarDeclare(newChar, NULL, NULL, NULL, NULL, NULL, line_number);
	ast_node_t *newVarList = newList(VAR_DECLARE_LIST, newVar);
	ast_node_t *newVarMaked = markAndProcessSymbolListWith(MODULE,WIRE, newVarList, false);
	if(size_module_variables_not_defined == 0){
		module_variables_not_defined = (ast_node_t **)vtr::calloc(1, sizeof(ast_node_t*));
	}
	else{
		module_variables_not_defined = (ast_node_t **)vtr::realloc(module_variables_not_defined, sizeof(ast_node_t*)*(size_module_variables_not_defined+1));
	}
	module_variables_not_defined[size_module_variables_not_defined] = newVarMaked;
	size_module_variables_not_defined++;
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { symbol_node, expression1, expression2, expression3 });

	return new_node;
}

ast_node_t *newMultipleInputsGateInstance(char* gate_instance_name, ast_node_t *expression1, ast_node_t *expression2, ast_node_t *expression3, int line_number)
{
    long i;
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(GATE_INSTANCE, line_number, current_parse_file);

	ast_node_t* symbol_node = NULL;

	if (gate_instance_name != NULL)
	{
		symbol_node = newSymbolNode(gate_instance_name, line_number);
	}

    char *newChar = vtr::strdup(expression1->types.identifier);

    ast_node_t *newVar = newVarDeclare(newChar, NULL, NULL, NULL, NULL, NULL, line_number);

    ast_node_t *newVarList = newList(VAR_DECLARE_LIST, newVar);

    ast_node_t *newVarMarked = markAndProcessSymbolListWith(MODULE, WIRE, newVarList, false);

    if(size_module_variables_not_defined == 0){
       module_variables_not_defined = (ast_node_t **)vtr::calloc(1, sizeof(ast_node_t*));
    }
    else{
       module_variables_not_defined = (ast_node_t **)vtr::realloc(module_variables_not_defined, sizeof(ast_node_t*)*(size_module_variables_not_defined+1));
    }

    module_variables_not_defined[size_module_variables_not_defined] = newVarMarked;

    size_module_variables_not_defined++;

    allocate_children_to_node(new_node, { symbol_node, expression1, expression2 });

    /* allocate n children nodes to this node */
    for(i = 1; i < expression3->num_children; i++) add_child_to_node(new_node, expression3->children[i]);
	
	/* clean up */
	if (expression3->type == MODULE_CONNECT) expression3 = free_single_node(expression3);

    return new_node;
}
/*---------------------------------------------------------------------------------------------
 * (function: newGate)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newGate(operation_list op_id, ast_node_t *gate_instance, int line_number)
{
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(GATE, line_number, current_parse_file);
	/* store the operation type */
	new_node->types.operation.op = op_id;
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { gate_instance });

	return new_node;
}



/*---------------------------------------------------------------------------------------------
 * (function: newVarDeclare)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newVarDeclare(char* symbol, ast_node_t *expression1, ast_node_t *expression2, ast_node_t *expression3, ast_node_t *expression4, ast_node_t *value, int line_number)
{
	ast_node_t *symbol_node = newSymbolNode(symbol, line_number);

	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(VAR_DECLARE, line_number, current_parse_file);

	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { symbol_node, expression1, expression2, expression3, expression4, value });

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newIntegerTypeVarDeclare)
 *-------------------------------------------------------------------------------------------*/

ast_node_t *newIntegerTypeVarDeclare(char* symbol, ast_node_t * /*expression1*/ , ast_node_t * /*expression2*/ , ast_node_t *expression3, ast_node_t *expression4, ast_node_t *value, int line_number)
{
	ast_node_t *symbol_node = newSymbolNode(symbol, line_number);

    ast_node_t *number_node_with_value_31 = create_tree_node_number(31L, line_number,current_parse_file);

    ast_node_t *number_node_with_value_0 = create_tree_node_number(0L, line_number,current_parse_file);

	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(VAR_DECLARE, line_number, current_parse_file);

	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { symbol_node, number_node_with_value_31, number_node_with_value_0, expression3, expression4, value });

	return new_node;
}
/*-----------------------------------------
 * ----------------------------------------------------
 * (function: newModule)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newModule(char* module_name, ast_node_t *list_of_parameters, ast_node_t *list_of_ports, ast_node_t *list_of_module_items, int line_number)
{
	int i;
	long j, k;
	long sc_spot;
	ast_node_t *symbol_node = newSymbolNode(module_name, line_number);

	if( sc_lookup_string(hard_block_names, module_name) != -1
		|| !strcmp(module_name, SINGLE_PORT_RAM_string)
		|| !strcmp(module_name, DUAL_PORT_RAM_string)
	)
	{
		error_message(PARSE_ERROR, line_number, current_parse_file, 
			"Module name collides with hard block of the same name (%s)\n", module_name);		
	}

	/* create a node for this array reference */
	ast_node_t *new_node = create_node_w_type(MODULE, line_number, current_parse_file);
	
	/* mark all the ports symbols as ports */
	ast_node_t *port_declarations = resolve_ports(MODULE, list_of_ports);

	/* ports are expected to be in module items */
	if (port_declarations)
	{
		add_child_to_node_at_index(list_of_module_items, port_declarations, 0);
	}

	/* parameters are expected to be in module items */
	if (list_of_parameters)
	{
		add_child_to_node_at_index(list_of_module_items, list_of_parameters, 0);
	}


	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { symbol_node, list_of_ports, list_of_module_items });

	/* check if this module has been instantiated */
	if (new_node->types.module.is_instantiated == false)
	{
		new_node->types.module.is_instantiated = (sc_lookup_string(instantiated_modules, module_name) != -1);
	}
	new_node->types.module.index = num_modules;

	new_node->types.function.function_instantiations_instance = function_instantiations_instance_by_module;
	new_node->types.function.size_function_instantiations = size_function_instantiations_by_module;
	new_node->types.function.is_instantiated = false;
	new_node->types.function.index = num_functions;
	/* record this module in the list of modules (for evaluation later in terms of just nodes) */
	ast_modules = (ast_node_t **)vtr::realloc(ast_modules, sizeof(ast_node_t*)*(num_modules+1));
	ast_modules[num_modules] = new_node;
	for(i = 0; i < size_module_variables_not_defined; i++)
	{
		short variable_found = false;
		for(j = 0; j < list_of_module_items->num_children && variable_found == false; j++){
		if(list_of_module_items->children[j]->type == VAR_DECLARE_LIST){
			for(k = 0; k < list_of_module_items->children[j]->num_children; k++){
				if(strcmp(list_of_module_items->children[j]->children[k]->children[0]->types.identifier,module_variables_not_defined[i]->children[0]->children[0]->types.identifier) == 0) 
					variable_found = true;
				}
			}
		}
		if(!variable_found) add_child_to_node_at_index(list_of_module_items, module_variables_not_defined[i], 0);
		else 				free_whole_tree(module_variables_not_defined[i]);
	}

	/* clean up */
	vtr::free(module_variables_not_defined);
	sc_spot = sc_add_string(module_names_to_idx, module_name);
	if (module_names_to_idx->data[sc_spot] != NULL)
	{
		error_message(PARSE_ERROR, line_number, current_parse_file, "module names with the same name -> %s\n", module_name);
	}
	/* store the data which is an idx here */
	module_names_to_idx->data[sc_spot] = (void*)new_node;
	
	/* now that we've bottom up built the parse tree for this module, go to the next module */
	next_module();

	return new_node;
}

/*-----------------------------------------
 * ----------------------------------------------------
 * (function: newFunction)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newFunction(ast_node_t *list_of_ports, ast_node_t *list_of_module_items, int line_number) //function and module declaration work the same way (Lucas Cambuim)
{

	long i,j;
	long sc_spot;
	ast_node_t *var_node;
	ast_node_t *symbol_node, *output_node;


	char *function_name = vtr::strdup(list_of_ports->children[0]->children[0]->types.identifier);

	output_node = newList(VAR_DECLARE_LIST, list_of_ports->children[0]);

	markAndProcessSymbolListWith(FUNCTION, OUTPUT, output_node, list_of_ports->children[0]->types.variable.is_signed);

	add_child_to_node_at_index(list_of_module_items, output_node, 0);

	char *label = vtr::strdup(list_of_ports->children[0]->children[0]->types.identifier);

	var_node = newVarDeclare(label, NULL, NULL, NULL, NULL, NULL, yylineno);

	list_of_ports->children[0] = var_node;


	for(i = 0; i < list_of_module_items->num_children; i++) {
		if(list_of_module_items->children[i]->type == VAR_DECLARE_LIST){
			for(j = 0; j < list_of_module_items->children[i]->num_children; j++) {
				if(list_of_module_items->children[i]->children[j]->types.variable.is_input){
                    label = vtr::strdup(list_of_module_items->children[i]->children[j]->children[0]->types.identifier);
                    var_node = newVarDeclare(label, NULL, NULL, NULL, NULL, NULL, yylineno);
					newList_entry(list_of_ports,var_node);
				}
			}
		}
	}

	symbol_node = newSymbolNode(function_name, line_number);

	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(FUNCTION, line_number, current_parse_file);
	
	/* mark all the ports symbols as ports */
	ast_node_t *port_declarations = resolve_ports(FUNCTION, list_of_ports);
	if (port_declarations)
	{
		add_child_to_node_at_index(list_of_module_items, port_declarations, 0);
	}

	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { symbol_node, list_of_ports, list_of_module_items });

	/* store the list of modules this module instantiates */
	new_node->types.function.function_instantiations_instance = function_instantiations_instance;
	new_node->types.function.size_function_instantiations = size_function_instantiations;
	new_node->types.function.is_instantiated = false;

	/* record this module in the list of modules (for evaluation later in terms of just nodes) */
	ast_modules = (ast_node_t **)vtr::realloc(ast_modules, sizeof(ast_node_t*)*(num_modules+1));
	ast_modules[num_modules] = new_node;
	sc_spot = sc_add_string(module_names_to_idx, function_name);
	if (module_names_to_idx->data[sc_spot] != NULL)
	{
		error_message(PARSE_ERROR, line_number, current_parse_file, "module names with the same name -> %s\n", function_name);
	}
	/* store the data which is an idx here */
	module_names_to_idx->data[sc_spot] = (void*)new_node;

	/* now that we've bottom up built the parse tree for this module, go to the next module */
	next_function();

	return new_node;
}
/*---------------------------------------------------------------------------------------------
 * (function: next_function)
 *-------------------------------------------------------------------------------------------*/
void next_function()
{
	num_functions++;
    //num_modules++;

	/* define the string cache for the next function */
	defines_for_function_sc = (STRING_CACHE**)vtr::realloc(defines_for_function_sc, sizeof(STRING_CACHE*)*(num_functions+1));
	defines_for_function_sc[num_functions] = sc_new_string_cache();

	/* create a new list for the instantiations list */
	function_instantiations_instance = NULL;
	size_function_instantiations = 0;

	/* old ones are done so clean */
	sc_free_string_cache(functions_inputs_sc);
	sc_free_string_cache(functions_outputs_sc);
	/* make for next function */
	functions_inputs_sc = sc_new_string_cache();
	functions_outputs_sc = sc_new_string_cache();
}
/*---------------------------------------------------------------------------------------------
 * (function: next_module)
 *-------------------------------------------------------------------------------------------*/
void next_module()
{
	num_modules ++;
    num_functions = 0;

	/* define the string cache for the next module */
	defines_for_module_sc = (STRING_CACHE**)vtr::realloc(defines_for_module_sc, sizeof(STRING_CACHE*)*(num_modules+1));
	defines_for_module_sc[num_modules] = sc_new_string_cache();

	function_instantiations_instance_by_module = NULL;
	size_function_instantiations_by_module = 0;

	module_variables_not_defined = NULL;
	size_module_variables_not_defined = 0;
	/* old ones are done so clean */
	sc_free_string_cache(modules_inputs_sc);
	sc_free_string_cache(modules_outputs_sc);
	sc_free_string_cache(module_instances_sc);
	/* make for next module */
	modules_inputs_sc = sc_new_string_cache();
	modules_outputs_sc = sc_new_string_cache();
	module_instances_sc = sc_new_string_cache();
}

/*--------------------------------------------------------------------------
 * (function: newDefparam)
 *------------------------------------------------------------------------*/
ast_node_t *newDefparam(ids /*id*/, ast_node_t *val, int line_number)
{
	ast_node_t *new_node = NULL;

	if(val)
	{
		if(val->num_children > 1)
		{
			std::string module_instance_name = "";
			for(long i = 0; i < val->num_children - 1; i++)
			{
				oassert(val->children[i]->num_children > 0);

				if( i != 0 || val->num_children < 2 )
				{
					module_instance_name += ".";
				}

				module_instance_name += val->children[i]->children[0]->types.identifier;
			}

			new_node = val->children[(val->num_children - 1)];

			new_node->type = MODULE_PARAMETER;
			new_node->types.variable.is_parameter = true;
			new_node->children[5]->types.variable.is_parameter = true;
			new_node->types.identifier = vtr::strdup(module_instance_name.c_str());
			new_node->line_number = line_number;

			val->children[(val->num_children - 1)] = NULL;
			val = free_whole_tree(val);			
		}
	}

	return new_node;
}

/* --------------------------------------------------------------------------------------------
 --------------------------------------------------------------------------------------------
 --------------------------------------------------------------------------------------------
AST VIEWING FUNCTIONS
 --------------------------------------------------------------------------------------------
 --------------------------------------------------------------------------------------------
 --------------------------------------------------------------------------------------------*/

int unique_label_count;
/*---------------------------------------------------------------------------
 * (function: graphVizOutputAst)
 *-------------------------------------------------------------------------*/
void graphVizOutputAst(std::string path, ast_node_t *top)
{
	char path_and_file[4096];
	FILE *fp;
	static int file_num = 0;

	/* open the file */
	odin_sprintf(path_and_file, "%s/%s_ast.dot", path.c_str(), top->children[0]->types.identifier);
	fp = fopen(path_and_file, "w");
	file_num++;

        /* open graph */
        fprintf(fp, "digraph G {\t\nranksep=.25;\n");
	unique_label_count = 0;

	graphVizOutputAst_traverse_node(fp, top, NULL, -1);

        /* close graph */
        fprintf(fp, "}\n");
	fclose(fp);
}

/*---------------------------------------------------------------------------------------------
 * (function: graphVizOutputAst_traverse_node)
 *-------------------------------------------------------------------------------------------*/
void graphVizOutputAst_traverse_node(FILE *fp, ast_node_t *node, ast_node_t *from, int from_num)
{
	if(!node)
		return;

	/* increase the unique count for other nodes since ours is recorded */
	int my_label = unique_label_count++;

	fprintf(fp, "\t%d [label=\"", my_label);
	fprintf(fp, "%s", ids_STR[node->type]);
	switch(node->type)
	{
		case VAR_DECLARE:
		{
			std::stringstream temp;
			if(node->types.variable.is_input)		temp << " INPUT";
			if(node->types.variable.is_output)		temp << " OUTPUT";
			if(node->types.variable.is_inout)		temp << " INOUT";
			if(node->types.variable.is_port)		temp << " PORT";
			if(node->types.variable.is_parameter)	temp << " PARAMETER";
			if(node->types.variable.is_wire)		temp << " WIRE";
			if(node->types.variable.is_reg)			temp << " REG";

			fprintf(fp, ": %s",  temp.str().c_str());
			break;
		}
		case NUMBERS:
			fprintf(fp, ": %s (%s)",  
				node->types.vnumber->to_bit_string().c_str(), 
				node->types.vnumber->to_printable().c_str());
			break;

		case UNARY_OPERATION: //fallthrough
		case BINARY_OPERATION:
			fprintf(fp, ": %s", name_based_on_op(node->types.operation.op));
			break;

		case IDENTIFIERS:
			fprintf(fp, ": %s", node->types.identifier);
			break;
		
		default:
			break;
	}
	fprintf(fp, "\"];\n");		
	
	/* print out the connection with the previous node */
	if (from != NULL)
		fprintf(fp, "\t%d -> %d;\n", from_num, my_label);

	for (long i = 0; i < node->num_children; i++)
	{
		graphVizOutputAst_traverse_node(fp, node->children[i], node, my_label);
	}

}
/*---------------------------------------------------------------------------------------------
 * (function: newVarDeclare) for 2D Array
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newVarDeclare2D(char* symbol, ast_node_t *expression1, ast_node_t *expression2, ast_node_t *expression3, ast_node_t *expression4, ast_node_t *expression5, ast_node_t *expression6,ast_node_t *value, int line_number)
{
	ast_node_t *symbol_node = newSymbolNode(symbol, line_number);

	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(VAR_DECLARE, line_number, current_parse_file);

	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { symbol_node, expression1, expression2, expression3, expression4, expression5, expression6, value });
	return new_node;
}
/*---------------------------------------------------------------------------------------------
 * (function: newArrayRef) for 2D Array
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newArrayRef2D(char *id, ast_node_t *expression1, ast_node_t *expression2, int line_number)
{
	/* allocate or check if there's a node for this */
	ast_node_t *symbol_node = newSymbolNode(id, line_number);
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(ARRAY_REF, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { symbol_node, expression1,expression2 });
	return new_node;
}
/*---------------------------------------------------------------------------------------------
 * (function: newRangeRef) for 2D Array
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newRangeRef2D(char *id, ast_node_t *expression1, ast_node_t *expression2, ast_node_t *expression3, ast_node_t *expression4, int line_number)
{
	/* allocate or check if there's a node for this */
	ast_node_t *symbol_node = newSymbolNode(id, line_number);
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(RANGE_REF, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, { symbol_node, expression1, expression2, expression3, expression4 });

	return new_node;
}
