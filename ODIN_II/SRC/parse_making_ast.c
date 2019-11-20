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
#include "globals.h"
#include "types.h"
#include "errors.h"
#include "ast_util.h"
#include "parse_making_ast.h"
#include "string_cache.h"
#include "ast_optimizations.h"
#include "verilog_bison_user_defined.h"
#include "verilog_preprocessor.h"
#include "util.h"
#include "hard_blocks.h"

extern int yylineno;

STRING_CACHE *defines_for_file_sc;
STRING_CACHE **defines_for_module_sc;
STRING_CACHE *modules_inputs_sc;
STRING_CACHE *modules_outputs_sc;

STRING_CACHE *module_names_to_idx;

ast_node_t **block_instantiations_instance;
int size_block_instantiations;

ast_node_t **module_instantiations_instance;
int size_module_instantiations;

int num_modules;
ast_node_t **ast_modules;

ast_node_t **all_file_items_list;
int size_all_file_items_list;

short to_view_parse;

void graphVizOutputPreproc(FILE *yyin, char* path, char *file)
{
	char line[MaxLine];
	FILE *fp;
	char *tmp;

	// strip the ".v" from file
	tmp = strrchr(file, '.');
	oassert(tmp);
	oassert(*(tmp+1) == 'v');
	*tmp = '\0';
	
	// strip the path from file
	tmp = strrchr(file, '/');
	if (tmp) file = tmp;

	sprintf(line, "%s/%s_preproc.v", path, file);
	fp = fopen(line, "w");
	oassert(fp);
	while (fgets(line, MaxLine, yyin))
		fprintf(fp, "%s", line);
	fclose(fp);
	rewind(yyin);
}


/*---------------------------------------------------------------------------------------------
 * (function: parse_to_ast)
 *-------------------------------------------------------------------------------------------*/
void parse_to_ast()
{
	int i;
	extern FILE *yyin;
	extern int yylineno;
	
	/* hooks into macro at the top of verilog_flex.l that shows the tokens as they're parsed.  Set to true if you want to see it go...*/
	to_view_parse = configuration.print_parse_tokens;

	/* initialize the parser */
	init_parser();

	/* open files for parsing */
	if (global_args.verilog_file != NULL)
	{
		/* make a consitant file list so we can access in compiler ... replicating what read config does for the filenames */
		configuration.list_of_file_names = (char**)malloc(sizeof(char*));
		configuration.num_list_of_file_names = 1;
		configuration.list_of_file_names[0] = global_args.verilog_file;

		yyin = fopen(global_args.verilog_file, "r");
		if (yyin == NULL)
		{
			error_message(-1, -1, -1, "cannot open file: %s", global_args.verilog_file);
		}
	
		/*Testing preprocessor - Paddy O'Brien*/
		init_veri_preproc();
		yyin = veri_preproc(yyin);
		cleanup_veri_preproc();

		/* write out the pre-processed file */
		if (configuration.output_preproc_source)
			graphVizOutputPreproc(yyin, configuration.debug_output_path, configuration.list_of_file_names[0]) ;
			
		/* set the file name */	
		current_parse_file = 0;

		/* setup the local parser structures for a file */
		init_parser_for_file();
		/* parse */
		yyparse();
		/* cleanup parser */
		clean_up_parser_for_file();

		fclose(yyin);
	}
	else if (global_args.config_file != NULL)
	{
		/* read all the files in the configuration file */
		for (i = 0; i < configuration.num_list_of_file_names; i++)
		{
			yyin = fopen(configuration.list_of_file_names[i], "r");
			if (yyin == NULL)
			{
				error_message(-1, -1, -1, "cannot open file: %s\n", configuration.list_of_file_names[i]);
			}
		
			/*Testing preprocessor - Paddy O'Brien*/
			init_veri_preproc();
			yyin = veri_preproc(yyin);
			cleanup_veri_preproc();
			
			/* write out the pre-processed file */
			if (configuration.output_preproc_source)
				graphVizOutputPreproc(yyin, configuration.debug_output_path, configuration.list_of_file_names[i]) ;
 			
			/* set the file name */	
			current_parse_file = i;

			/* reset the line count */
			yylineno = 0;

			/* setup the local parser structures for a file */
			init_parser_for_file();
			/* parse next file */
			yyparse();
			/* cleanup parser */
			clean_up_parser_for_file();

			fclose(yyin);
		}
	}

	/* clean up all the structures in the parser */
	cleanup_parser();

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

/*----------------------------------------------------------------------------
 * (function: cleanup_hard_blocks)
 *   This function will correctly label nodes in the AST as being part of a
 *   hard block and not a module. This needs to be done post parsing since
 *   there are no language differences between hard blocks and modules.
 *--------------------------------------------------------------------------*/
void cleanup_hard_blocks()
{
	int i, j;
	ast_node_t *block_node, *instance_node, *connect_list_node;

	for (i = 0; i < size_block_instantiations; i++)
	{
		block_node = block_instantiations_instance[i];
		instance_node = block_node->children[1];
		instance_node->type = HARD_BLOCK_NAMED_INSTANCE;
		connect_list_node = instance_node->children[1];
		connect_list_node->type = HARD_BLOCK_CONNECT_LIST;

		for (j = 0; j < connect_list_node->num_children; j++)
		{
			connect_list_node->children[j]->type = HARD_BLOCK_CONNECT;
		}
	}
	return;
}

/*---------------------------------------------------------------------------------------------
 * (function: init_parser)
 *-------------------------------------------------------------------------------------------*/
void init_parser()
{
	defines_for_file_sc = sc_new_string_cache();

	defines_for_module_sc = NULL;

	/* record of each of the individual modules */
	num_modules = 0; // we're going to record all the modules in a list so we can build a tree of them later
	ast_modules = NULL;
	module_names_to_idx = sc_new_string_cache();
	module_instantiations_instance = NULL;
	size_module_instantiations = 0;
	block_instantiations_instance = NULL;
	size_block_instantiations = 0;

	/* keeps track of all the ast roots */
	all_file_items_list = NULL;
	size_all_file_items_list = 0;
}

/*---------------------------------------------------------------------------------------------
 * (function: cleanup_parser)
 *-------------------------------------------------------------------------------------------*/
void cleanup_parser()
{
	int i;
	
	/* frees all the defines for module string caches (used for parameters) */
	if (num_modules > 0)
	{
		for (i = 0; i < num_modules; i++)
		{
			sc_free_string_cache(defines_for_module_sc[i]);
		}
		
		free(defines_for_module_sc);
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: init_parser_for_file)
 *-------------------------------------------------------------------------------------------*/
void init_parser_for_file()
{
	/* crrate a hash for defines so we can look them up when we find them */
	defines_for_module_sc = (STRING_CACHE**)realloc(defines_for_module_sc, sizeof(STRING_CACHE*)*(num_modules+1));
	defines_for_module_sc[num_modules] = sc_new_string_cache();

	/* create string caches to hookup PORTS with INPUT and OUTPUTs.  This is made per module and will be cleaned and remade at next_module */
	modules_inputs_sc = sc_new_string_cache();
	modules_outputs_sc = sc_new_string_cache();
}

/*---------------------------------------------------------------------------------------------
 * (function: clean_up_parser_for_file)
 *-------------------------------------------------------------------------------------------*/
void clean_up_parser_for_file()
{
	/* cleanup the defines hash */
	sc_free_string_cache(defines_for_file_sc);
}

/*---------------------------------------------------------------------------------------------
 * (function: next_parsed_verilog_file)
 *-------------------------------------------------------------------------------------------*/
void next_parsed_verilog_file(ast_node_t *file_items_list)
{
	int i;
	/* optimization entry point */
	printf("Optimizing module by AST based optimizations\n");
	optimizations_on_AST(file_items_list);

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
	all_file_items_list = (ast_node_t**)realloc(all_file_items_list, sizeof(ast_node_t*)*(size_all_file_items_list+1));
	all_file_items_list[size_all_file_items_list] = file_items_list;
	size_all_file_items_list ++;
}

/*---------------------------------------------------------------------------------------------
 * (function: newSymbolNode)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newSymbolNode(char *id, int line_number)
{
	long sc_spot;
	ast_node_t *current_node;

	if (id[0] == '`')
	{
		/* IF - this is a define replace with number constant */ 
		/* def_reduct */

		/* get the define symbol from the string cache */
		if ((sc_spot = sc_lookup_string(defines_for_file_sc, (id+1))) == -1)
		{
			error_message(PARSE_ERROR, line_number, current_parse_file, "Define \"%s\" used but not declared\n", id);
		}

		/* return the number node */
		return (ast_node_t*)defines_for_file_sc->data[sc_spot];
	}
	else
	{
		/* create node */
		current_node = create_tree_node_id(id, line_number, current_parse_file);
	}

	return current_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newNumberNode)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newNumberNode(char *num, int line_number)
{
	ast_node_t *current_node = create_tree_node_number(num, line_number, current_parse_file);
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
	allocate_children_to_node(new_node, 1, child);

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
 * Basically this functions emulates verilog replication: {5{1'b0}} by concatenating that many
 * children together -- certainly not the most elegant solution, but was the easiest
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newListReplicate(ast_node_t *exp, ast_node_t *child )
{
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(CONCATENATE, yylineno, current_parse_file);
	
	new_node->types.concat.num_bit_strings = -1;

	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, 1, child);
	
	int i;
	for (i = 1; i < exp->types.number.value; i++) 
	{
		add_child_to_node(new_node, child);
	}

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: markAndProcessSymbolListWith)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *markAndProcessSymbolListWith(ids id, ast_node_t *symbol_list)
{
	int i;
	long sc_spot;
	long range_temp_max = 0;
	long range_temp_min = 0;
	ast_node_t *range_min = 0;
	ast_node_t *range_max = 0;
	ast_node_t *newNode = 0;

	for (i = 0; i < symbol_list->num_children; i++)
	{
		/* checks range is legal.  */
		get_range(symbol_list->children[i]);

		if ((i == 0) && (symbol_list->children[0]->children[1] != NULL) && (symbol_list->children[0]->children[2] != NULL)
						&& ((symbol_list->children[0]->children[1]->type == NUMBERS) || (symbol_list->children[0]->children[1]->type == IDENTIFIERS) || (symbol_list->children[0]->children[1]->type == BINARY_OPERATION))
						&& ((symbol_list->children[0]->children[2]->type == NUMBERS) || (symbol_list->children[0]->children[2]->type == IDENTIFIERS)|| (symbol_list->children[0]->children[2]->type == BINARY_OPERATION)))
		{
			/* Do lookup in sc_add_string */
			/* Verify node->type.variables.is_parameter == TRUE */
			/* If type is BINARY_OPERATION, Calculate it*/
			/* ELSE REPORT ERROR */
			if (symbol_list->children[0]->children[1]->type == IDENTIFIERS)
			{
				if ((sc_spot = sc_lookup_string(defines_for_module_sc[num_modules], symbol_list->children[0]->children[1]->types.identifier)) != -1)
				{
					newNode = defines_for_module_sc[num_modules]->data[sc_spot];
					if (newNode->types.variable.is_parameter == TRUE)
					{
						range_max = symbol_list->children[0]->children[1];
						range_temp_max = newNode->types.number.value;
					}
					else
						error_message(PARSE_ERROR, symbol_list->children[0]->children[1]->line_number, current_parse_file,
								"parameter %s don't match\n", symbol_list->children[0]->children[1]->types.identifier);
				}
				else
					error_message(PARSE_ERROR, symbol_list->children[0]->children[1]->line_number, current_parse_file,
								"parameter %s don't match\n", symbol_list->children[0]->children[1]->types.identifier);
			}
			else if(symbol_list->children[0]->children[1]->type == NUMBERS)
			{
				range_max = symbol_list->children[0]->children[1];
				range_temp_max = range_max->types.number.value;
			}
			else if(symbol_list->children[0]->children[1]->type == BINARY_OPERATION)
			{
				range_max = symbol_list->children[0]->children[1];
				range_temp_max = calculate_operation(symbol_list->children[0]->children[1]);
			}

			if (symbol_list->children[0]->children[2]->type == IDENTIFIERS)
			{
				if ((sc_spot = sc_lookup_string(defines_for_module_sc[num_modules], symbol_list->children[0]->children[2]->types.identifier)) != -1)
				{
					newNode = defines_for_module_sc[num_modules]->data[sc_spot];
					if (newNode->types.variable.is_parameter == TRUE)
					{
						range_min = symbol_list->children[0]->children[2];
						range_temp_min = newNode->types.number.value;
					}
					else
						error_message(PARSE_ERROR, symbol_list->children[0]->children[2]->line_number, current_parse_file,
								"parameter %s don't match\n", symbol_list->children[0]->children[2]->types.identifier);
					}
					else
						error_message(PARSE_ERROR, symbol_list->children[0]->children[2]->line_number, current_parse_file,
								"parameter %s don't match\n", symbol_list->children[0]->children[2]->types.identifier);
				}
				else if(symbol_list->children[0]->children[2]->type == NUMBERS)
				{
					range_min = symbol_list->children[0]->children[2];
					range_temp_min = range_min->types.number.value;
				}
				else if(symbol_list->children[0]->children[2]->type == BINARY_OPERATION)
				{
					range_min = symbol_list->children[0]->children[2];
					range_temp_min = calculate_operation(symbol_list->children[0]->children[2]);
				}

			if(range_temp_min > range_temp_max)
				error_message(NETLIST_ERROR, symbol_list->children[0]->children[0]->line_number, current_parse_file, "Odin doesn't support arrays declared [m:n] where m is less than n.");
			//ODIN doesn't support negative number in index now.
			if(range_temp_min < 0 || range_temp_max < 0)
				warning_message(NETLIST_ERROR, symbol_list->children[0]->children[0]->line_number, current_parse_file, "Odin doesn't support negative number in index.");

		}

		if ((symbol_list->children[i]->children[1] == NULL) && (symbol_list->children[i]->children[2] == NULL))
		{
			symbol_list->children[i]->children[1] = range_max;
			symbol_list->children[i]->children[2] = range_min;
		}

		switch(id)
		{
			case PORT:
			{
				short found_match = FALSE;

				symbol_list->children[i]->types.variable.is_port = TRUE;
				
				/* find the related INPUT or OUTPUT definition and store that instead */
				if ((sc_spot = sc_lookup_string(modules_inputs_sc, symbol_list->children[i]->children[0]->types.identifier)) != -1)
				{
					symbol_list->children[i]->types.variable.is_input = TRUE;
					symbol_list->children[i]->children[0] = (ast_node_t*)modules_inputs_sc->data[sc_spot];
					found_match = TRUE;
				}
				if ((found_match == FALSE) && ((sc_spot = sc_lookup_string(modules_outputs_sc, symbol_list->children[i]->children[0]->types.identifier)) != -1))
				{
					symbol_list->children[i]->types.variable.is_output = TRUE;
					symbol_list->children[i]->children[0] = (ast_node_t*)modules_outputs_sc->data[sc_spot];
					found_match = TRUE;
				}
				
				if (found_match == FALSE)
				{
					error_message(PARSE_ERROR, symbol_list->children[i]->line_number, current_parse_file, "No matching input declaration for port %s\n", symbol_list->children[i]->children[0]->types.identifier);
				}
				break;
			}
			case PARAMETER:
			{
				int binary_range = -1;
				if (i == 0)
				{
					binary_range = get_range(symbol_list->children[i]);
				}
				
				/* fifth spot in the children list holds a parameter value */
				if (binary_range != -1)
				{
					/* check that the parameter size matches the number included */
					 if((symbol_list->children[i]->children[5]->types.number.size != 0) 
						&& (symbol_list->children[i]->children[5]->types.number.base == BIN) 
						&& (symbol_list->children[i]->children[5]->types.number.size != binary_range)) 
					{
						error_message(PARSE_ERROR, symbol_list->children[i]->children[5]->line_number, current_parse_file, "parameter %s and range %d don't match\n", symbol_list->children[i]->children[0]->types.identifier, binary_range);
					}
					else
					{
						symbol_list->children[i]->children[5]->types.number.size = binary_range; // assign the binary range 
					}
				}

				/* create an entry in the symbol table for this parameter */
				if ((sc_spot = sc_add_string(defines_for_module_sc[num_modules], symbol_list->children[i]->children[0]->types.identifier)) == -1)
				{
					error_message(PARSE_ERROR, symbol_list->children[i]->children[5]->line_number, current_parse_file, "define has same name (%s).  Other define migh be in another file.  Odin considers a define as global.\n", 
						symbol_list->children[i]->children[0]->types.identifier, 
						((ast_node_t*)(defines_for_module_sc[num_modules]->data[sc_spot]))->line_number);
				}
				symbol_list->children[i]->children[5]->types.variable.is_parameter = TRUE;
				defines_for_module_sc[num_modules]->data[sc_spot] = (void*)symbol_list->children[i]->children[5];
				/* mark the node as shared so we don't delete it */
				symbol_list->children[i]->children[5]->shared_node = TRUE;
				/* now do the mark */
				symbol_list->children[i]->types.variable.is_parameter = TRUE;
				break;
			}
			case INPUT:
				symbol_list->children[i]->types.variable.is_input = TRUE;
				/* add this input to the modules string cache */
				if ((sc_spot = sc_add_string(modules_inputs_sc, symbol_list->children[i]->children[0]->types.identifier)) == -1)
				{
					error_message(PARSE_ERROR, symbol_list->children[i]->children[0]->line_number, current_parse_file, "Module already has input with this name %s\n", symbol_list->children[i]->children[0]->types.identifier);
				}
				/* store the data which is an idx here */
				modules_inputs_sc->data[sc_spot] = (void*)symbol_list->children[i];
				break;
			case OUTPUT:
				symbol_list->children[i]->types.variable.is_output = TRUE;
				/* add this output to the modules string cache */
				if ((sc_spot = sc_add_string(modules_outputs_sc, symbol_list->children[i]->children[0]->types.identifier)) == -1)
				{
					error_message(PARSE_ERROR, symbol_list->children[i]->children[0]->line_number, current_parse_file, "Module already has input with this name %s\n", symbol_list->children[i]->children[0]->types.identifier);
				}
				/* store the data which is an idx here */
				modules_outputs_sc->data[sc_spot] = (void*)symbol_list->children[i];
				break;
			case INOUT:
				symbol_list->children[i]->types.variable.is_inout = TRUE;
				error_message(PARSE_ERROR, symbol_list->children[i]->children[0]->line_number, current_parse_file, "Odin does not handle inouts (%s)\n", symbol_list->children[i]->children[0]->types.identifier);
				break;
			case WIRE:
				symbol_list->children[i]->types.variable.is_wire = TRUE;
				break;
			case REG:
				symbol_list->children[i]->types.variable.is_reg = TRUE;
				break;
			case INTEGER:
				symbol_list->children[i]->types.variable.is_integer = TRUE;
				/* This should be removed when elaboration of integers is added */
				printf("integer data type is currently NOT supported: %s\n", symbol_list->children[i]->children[0]->types.identifier);
				oassert(FALSE);
				break;
			default:
				oassert(FALSE);
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
	allocate_children_to_node(new_node, 2, symbol_node, expression);

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
	allocate_children_to_node(new_node, 3, symbol_node, expression1, expression2);
	/* swap the direction so in form [MSB:LSB] */
	get_range(new_node);

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newBinaryOperation)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newBinaryOperation(operation_list op_id, ast_node_t *expression1, ast_node_t *expression2, int line_number)
{
	info_ast_visit_t *node_details = NULL;
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(BINARY_OPERATION, line_number, current_parse_file);
	/* store the operation type */
	new_node->types.operation.op = op_id;
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, 2, expression1, expression2);

	/* see if this binary expression can have some constant folding */
	node_details = constantFold(new_node);
	if ((node_details != NULL) && (node_details->is_constant_folded == TRUE))
	{
		new_node = node_details->from;
		free(node_details);
	}

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newUnaryOperation)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newUnaryOperation(operation_list op_id, ast_node_t *expression, int line_number)
{
	info_ast_visit_t *node_details = NULL;
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(UNARY_OPERATION, line_number, current_parse_file);
	/* store the operation type */
	new_node->types.operation.op = op_id;
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, 1, expression);

	/* see if this binary expression can have some constant folding */
	node_details = constantFold(new_node);
	if ((node_details != NULL) && (node_details->is_constant_folded == TRUE))
	{
		new_node = node_details->from;
		free(node_details);
	}

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
	allocate_children_to_node(new_node, 1, symbol_node);

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
	allocate_children_to_node(new_node, 1, symbol_node);

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
	allocate_children_to_node(new_node, 2, expression, statement);

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
	allocate_children_to_node(new_node, 1, statement);

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
	allocate_children_to_node(new_node, 2, expression1, expression2);

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
	allocate_children_to_node(new_node, 2, expression1, expression2);

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
	allocate_children_to_node(new_node, 4, initial, compare_expression, terminal, statement);

	/* This needs to be removed once elaboration support is added */
	printf("For statement is NOT supported: line %d\n", line_number);
	oassert(0);

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
	allocate_children_to_node(new_node, 2, compare_expression, statement);

	/* This needs to be removed once elaboration support is added */
	printf("While statement is NOT supported: line %d\n", line_number);
	oassert(0);

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
	allocate_children_to_node(new_node, 3, compare_expression, true_expression, false_expression);

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
	allocate_children_to_node(new_node, 3, compare_expression, true_expression, false_expression);

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
	allocate_children_to_node(new_node, 2, compare_expression, case_list);

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
	allocate_children_to_node(new_node, 2, delay_control, statement);

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
	allocate_children_to_node(new_node, 2, symbol_node, expression);

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newModuleParameter)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newModuleParameter(char* id, ast_node_t *expression, int line_number)
{
	ast_node_t *symbol_node;
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(MODULE_PARAMETER, line_number, current_parse_file);
	if (id != NULL)
	{
		error_message(PARSE_ERROR, line_number, current_parse_file, "Specifying parameters by name not currently supported!\n");
		symbol_node = newSymbolNode(id, line_number);
	}
	else
	{
		symbol_node = NULL;
	}
	
	if (expression->type != NUMBERS)
	{
		error_message(PARSE_ERROR, line_number, current_parse_file, "Parameter value must be of type NUMBERS!\n");
	}

	/* allocate child nodes to this node */
	// leave 4 blank nodes so that expression is the 6th node to behave just like  
	// a default var_declare parameter (see create_symbol_table_for_module)
	allocate_children_to_node(new_node, 6, symbol_node, NULL, NULL, NULL, NULL, expression);

	// set is_parameter for the same reason
	new_node->types.variable.is_parameter = TRUE;

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
	allocate_children_to_node(new_node, 3, symbol_node, module_connect_list, module_parameter_list);

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
	allocate_children_to_node(new_node, 2, symbol_node, module_named_instance);

	// store the hard block symbol name that this calls in a list that will at the end be asociated with the hard block node
	block_instantiations_instance = (ast_node_t **)realloc(block_instantiations_instance, sizeof(ast_node_t*)*(size_block_instantiations+1));
	block_instantiations_instance[size_block_instantiations] = new_node;
	size_block_instantiations++;

	return new_node;
}

/*-------------------------------------------------------------------------
 * (function: newModuleInstance)
 *-----------------------------------------------------------------------*/
ast_node_t *newModuleInstance(char* module_ref_name, ast_node_t *module_named_instance, int line_number)
{
	#ifdef VPR6
	if
	(
		   sc_lookup_string(hard_block_names, module_ref_name) != -1
		|| !strcmp(module_ref_name, "single_port_ram")
		|| !strcmp(module_ref_name, "dual_port_ram")
	)
	{
		return newHardBlockInstance(module_ref_name, module_named_instance, line_number);
	}
	#endif

	// make a unique module name based on its parameter list
	ast_node_t *module_param_list = module_named_instance->children[2];
	char *module_param_name = make_module_param_name(module_param_list, module_ref_name);
	ast_node_t *symbol_node = newSymbolNode(module_param_name, line_number);

	// if this is a parameterised instantiation
	if (module_param_list)
	{
		// which doesn't exist in ast_modules yet
		long sc_spot;
		if ((sc_spot = sc_lookup_string(module_names_to_idx, module_ref_name)) == -1)
		{
			// then add it, but set it to the symbol_node, because the 
			// module in question may not have been parsed yet
			// later, we convert this symbol node back into a module node
			ast_modules = (ast_node_t **)realloc(ast_modules, sizeof(ast_node_t*)*(num_modules+1));
			ast_modules[num_modules] = symbol_node;
			num_modules++;
			sc_spot = sc_add_string(module_names_to_idx, module_param_name);
			module_names_to_idx->data[sc_spot] = symbol_node;
			defines_for_module_sc = (STRING_CACHE**)realloc(defines_for_module_sc, sizeof(STRING_CACHE*)*(num_modules+1));
			defines_for_module_sc[num_modules] = NULL;
		}
	}

	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(MODULE_INSTANCE, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, 2, symbol_node, module_named_instance);

	/* store the module symbol name that this calls in a list that will at the end be asociated with the module node */
	module_instantiations_instance = (ast_node_t **)realloc(module_instantiations_instance, sizeof(ast_node_t*)*(size_module_instantiations+1));
	module_instantiations_instance[size_module_instantiations] = new_node;
	size_module_instantiations++;

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

	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, 4, symbol_node, expression1, expression2, expression3);

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
	allocate_children_to_node(new_node, 1, gate_instance);

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: newAssign)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newAssign(ast_node_t *statement, int line_number)
{
	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(ASSIGN, line_number, current_parse_file);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, 1, statement);

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
	allocate_children_to_node(new_node, 6, symbol_node, expression1, expression2, expression3, expression4, value);

	return new_node;
}

/*-----------------------------------------
 * ----------------------------------------------------
 * (function: newModule)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *newModule(char* module_name, ast_node_t *list_of_ports, ast_node_t *list_of_module_items, int line_number)
{
	long sc_spot;
	ast_node_t *symbol_node = newSymbolNode(module_name, line_number);

	/* create a node for this array reference */
	ast_node_t* new_node = create_node_w_type(MODULE, line_number, current_parse_file);
	/* mark all the ports symbols as ports */
	markAndProcessSymbolListWith(PORT, list_of_ports);
	/* allocate child nodes to this node */
	allocate_children_to_node(new_node, 3, symbol_node, list_of_ports, list_of_module_items);

	/* store the list of modules this module instantiates */
	new_node->types.module.module_instantiations_instance = module_instantiations_instance;
	new_node->types.module.size_module_instantiations = size_module_instantiations;
	new_node->types.module.is_instantiated = FALSE;
	new_node->types.module.index = num_modules;

	/* record this module in the list of modules (for evaluation later in terms of just nodes) */
	ast_modules = (ast_node_t **)realloc(ast_modules, sizeof(ast_node_t*)*(num_modules+1));
	ast_modules[num_modules] = new_node;

	if ((sc_spot = sc_add_string(module_names_to_idx, module_name)) == -1)
	{
		error_message(PARSE_ERROR, line_number, current_parse_file, "module names with the same name -> %s\n", module_name);
	}
	/* store the data which is an idx here */
	module_names_to_idx->data[sc_spot] = (void*)new_node;

	/* now that we've bottom up built the parse tree for this module, go to the next module */
	next_module();

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: next_module)
 *-------------------------------------------------------------------------------------------*/
void next_module()
{
	num_modules ++;

	/* define the string cache for the next module */
	defines_for_module_sc = (STRING_CACHE**)realloc(defines_for_module_sc, sizeof(STRING_CACHE*)*(num_modules+1));
	defines_for_module_sc[num_modules] = sc_new_string_cache();

	/* create a new list for the instantiations list */
	module_instantiations_instance = NULL;
	size_module_instantiations = 0;

	/* old ones are done so clean */
	sc_free_string_cache(modules_inputs_sc);
	sc_free_string_cache(modules_outputs_sc);
	/* make for next module */
	modules_inputs_sc = sc_new_string_cache();
	modules_outputs_sc = sc_new_string_cache();
}

/*--------------------------------------------------------------------------
 * (function: newDefparam)
 *------------------------------------------------------------------------*/
ast_node_t *newDefparam(ids id, ast_node_t *val, int line_number)
{
	ast_node_t *new_node;
	ast_node_t *ref_node;
	char *module_instance_name = (char*)malloc(1024 * sizeof(char));
	int i, j;
	//long sc_spot;
	if(val)
	{
		if(val->num_children > 1)
		{
			for(i = 0; i < val->num_children - 1; i++)
			{
				oassert(val->children[i]->num_children > 0);
				if(i == 0 && val->num_children > 2)
					module_instance_name = val->children[i]->children[0]->types.identifier;
				else if(i == 0 && val->num_children == 2)
					module_instance_name = val->children[i]->children[0]->types.identifier;
				else
				{
					module_instance_name = strcat(module_instance_name, ".");
					module_instance_name = strcat(module_instance_name, val->children[i]->children[0]->types.identifier);
				}
			}
			new_node = val->children[(val->num_children - 1)];
			new_node->type = MODULE_PARAMETER;
			new_node->types.variable.is_parameter = TRUE;
			new_node->shared_node = TRUE;
			new_node->children[5]->types.variable.is_parameter = TRUE;
			new_node->children[5]->shared_node = TRUE;
			new_node->types.identifier = module_instance_name;
		}
	}
	//flag = 0 can't find the instance
	int flag = 0;
	for(j = 0 ; j < size_module_instantiations ; j++)
	{
		if(flag == 0)
		{
			ref_node = module_instantiations_instance[j];
			if(strcmp(ref_node->children[1]->children[0]->types.identifier, module_instance_name) == 0)
			{
				if(ref_node->children[1]->children[2])
					add_child_to_node(ref_node->children[1]->children[2], new_node);
				else
				{
					ast_node_t* symbol_node = create_node_w_type(MODULE_PARAMETER_LIST, line_number, current_parse_file);
					ref_node->children[1]->children[2] = symbol_node;
					add_child_to_node(ref_node->children[1]->children[2], new_node);
				}
				flag = 1;
			}
		}
	}
	//if the instance never showed before, dealt with this parameter in function convert_ast_to_netlist_recursing_via_modules
	if(flag == 0)
	{
		new_node->shared_node = FALSE;
		return new_node;
	//	add_child_to_node(new_node, symbol_node);
	}
	return NULL;
}

/*--------------------------------------------------------------------------
 * (function: newConstant)
 *------------------------------------------------------------------------*/
void newConstant(char *id, char *number, int line_number)
{
	long sc_spot;
	ast_node_t *number_node = newNumberNode(number, line_number);

	/* add the define character string to the parser and maintain the number around */
	/* def_reduct */
	if ((sc_spot = sc_add_string(defines_for_file_sc, id)) == -1)
	{
		error_message(PARSE_ERROR, current_parse_file, line_number, "define with same name (%s) on line %d\n", id, ((ast_node_t*)(defines_for_file_sc->data[sc_spot]))->line_number);
	}
	/* store the data */
	defines_for_file_sc->data[sc_spot] = (void*)number_node;
	/* mark node as shared */
	number_node->shared_node = TRUE;
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
void graphVizOutputAst(char* path, ast_node_t *top)
{
	char path_and_file[4096];
	FILE *fp;
	static int file_num = 0;

	/* open the file */
	sprintf(path_and_file, "%s/%s_ast.dot", path, top->children[0]->types.identifier);
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
	int i;
	int my_label = unique_label_count;

	/* increase the unique count for other nodes since ours is recorded */
	unique_label_count++;

	if (node == NULL)
	{
		/* print out the node and label details */
	}
	else
	{
		switch(node->type)
		{
			case FILE_ITEMS:
				fprintf(fp, "\t%d [label=\"FILE_ITEMS\"];\n", my_label);
				break;
			case MODULE: 
				fprintf(fp, "\t%d [label=\"MODULE\"];\n", my_label);
				break;
			case MODULE_ITEMS:
				fprintf(fp, "\t%d [label=\"MODULE_ITEMS\"];\n", my_label);
				break;
			case VAR_DECLARE:
				{
					char temp[4096] = "";
					if(node->types.variable.is_input)
					{
						sprintf(temp, "%s INPUT", temp);
					}
					if(node->types.variable.is_output)
					{
						sprintf(temp, "%s OUTPUT", temp);
					}
					if(node->types.variable.is_inout)
					{
						sprintf(temp, "%s INOUT", temp);
					}
					if(node->types.variable.is_port)
					{
						sprintf(temp, "%s PORT", temp);
					}
					if(node->types.variable.is_parameter)
					{
						sprintf(temp, "%s PARAMETER", temp);
					}
					if(node->types.variable.is_wire)
					{
						sprintf(temp, "%s WIRE", temp);
					}
					if(node->types.variable.is_reg)
					{
						sprintf(temp, "%s REG", temp);
					}
					fprintf(fp, "\t%d [label=\"VAR_DECLARE %s\"];\n", my_label, temp);
				}
				break;
			case VAR_DECLARE_LIST:
				fprintf(fp, "\t%d [label=\"VAR_DECLARE_LIST\"];\n", my_label);
				break;
			case ASSIGN:
				fprintf(fp, "\t%d [label=\"ASSIGN\"];\n", my_label);
				break;
			case GATE:
				fprintf(fp, "\t%d [label=\"GATE\"];\n", my_label);
				break;
			case GATE_INSTANCE:
				fprintf(fp, "\t%d [label=\"GATE_INSTANCE\"];\n", my_label);
				break;
			case MODULE_CONNECT_LIST:
				fprintf(fp, "\t%d [label=\"MODULE_CONNECT_LIST\"];\n", my_label);
				break;
			case MODULE_CONNECT:
				fprintf(fp, "\t%d [label=\"MODULE_CONNECT\"];\n", my_label);
				break;
			case MODULE_PARAMETER_LIST:
				fprintf(fp, "\t%d [label=\"MODULE_PARAMETER_LIST\"];\n", my_label);
				break;
			case MODULE_PARAMETER:
				fprintf(fp, "\t%d [label=\"MODULE_PARAMETER\"];\n", my_label);
				break;
			case MODULE_NAMED_INSTANCE:
				fprintf(fp, "\t%d [label=\"MODULE_NAMED_INSTANCE\"];\n", my_label);
				break;
			case MODULE_INSTANCE:
				fprintf(fp, "\t%d [label=\"MODULE_INSTANCE\"];\n", my_label);
				break;
			case HARD_BLOCK:
				fprintf(fp, "\t%d [label=\"HARD_BLOCK\"];\n", my_label);
				break;
			case HARD_BLOCK_NAMED_INSTANCE:
				fprintf(fp, "\t%d [label=\"HARD_BLOCK_NAMED_INSTANCE\"];\n", my_label);
				break;
			case HARD_BLOCK_CONNECT:
				fprintf(fp, "\t%d [label=\"HARD_BLOCK_CONNECT\"];\n", my_label);
				break;
			case HARD_BLOCK_CONNECT_LIST:
				fprintf(fp, "\t%d [label=\"HARD_BLOCK_CONNECT_LIST\"];\n", my_label);
				break;
			case BLOCK:
				fprintf(fp, "\t%d [label=\"BLOCK\"];\n", my_label);
				break;
			case NON_BLOCKING_STATEMENT:
				fprintf(fp, "\t%d [label=\"NON_BLOCKING_STATEMENT\"];\n", my_label);
				break;
			case BLOCKING_STATEMENT:
				fprintf(fp, "\t%d [label=\"BLOCKING_STATEMENT\"];\n", my_label);
				break;
			case CASE:
				fprintf(fp, "\t%d [label=\"CASE\"];\n", my_label);
				break;
			case CASE_LIST:
				fprintf(fp, "\t%d [label=\"CASE_LIST\"];\n", my_label);
				break;
			case CASE_ITEM:
				fprintf(fp, "\t%d [label=\"CASE_ITEM\"];\n", my_label);
				break;
			case CASE_DEFAULT:
				fprintf(fp, "\t%d [label=\"CASE_DEFAULT\"];\n", my_label);
				break;
			case ALWAYS:
				fprintf(fp, "\t%d [label=\"ALWAYS\"];\n", my_label);
				break;
			case DELAY_CONTROL:
				fprintf(fp, "\t%d [label=\"DELAY_CONTROL\"];\n", my_label);
				break;
			case POSEDGE:
				fprintf(fp, "\t%d [label=\"POSEDGE\"];\n", my_label);
				break;
			case NEGEDGE:
				fprintf(fp, "\t%d [label=\"NEGEDGE\"];\n", my_label);
				break;
			case IF:
				fprintf(fp, "\t%d [label=\"IF\"];\n", my_label);
				break;
			case IF_Q:
				fprintf(fp, "\t%d [label=\"IF_Q\"];\n", my_label);
				break;
			case BINARY_OPERATION:
				switch (node->types.operation.op)
				{
					case ADD:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION ADD\"];\n", my_label);
						break;
					case MINUS:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION MINUS\"];\n", my_label);
						break;
					case BITWISE_NOT:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION BITWISE_NOT\"];\n", my_label);
						break;
					case BITWISE_AND:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION BITWISE_AND\"];\n", my_label);
						break;
					case BITWISE_OR:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION BITWISE_OR\"];\n", my_label);
						break;
					case BITWISE_NAND:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION BITWISE_NAND\"];\n", my_label);
						break;
					case BITWISE_NOR:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION BITWISE_NOR\"];\n", my_label);
						break;
					case BITWISE_XNOR:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION BITWISE_XNOR\"];\n", my_label);
						break;
					case BITWISE_XOR:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION BITWISE_XOR\"];\n", my_label);
						break;
					case LOGICAL_NOT:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION LOGICAL_NOT\"];\n", my_label);
						break;
					case LOGICAL_OR:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION LOGICAL_OR\"];\n", my_label);
						break;
					case LOGICAL_AND:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION LOGICAL_AND\"];\n", my_label);
						break;
					case MULTIPLY:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION MULTIPLY\"];\n", my_label);
						break;
					case DIVIDE:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION DIVIDE\"];\n", my_label);
						break;
					case MODULO:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION MODULO\"];\n", my_label);
						break;
					case LT:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION LT\"];\n", my_label);
						break;
					case GT:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION GT\"];\n", my_label);
						break;
					case LOGICAL_EQUAL:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION LOGICAL_EQUAL\"];\n", my_label);
						break;
					case NOT_EQUAL:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION NOT_EQUAL\"];\n", my_label);
						break;
					case LTE:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION LTE\"];\n", my_label);
						break;
					case GTE:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION GTE\"];\n", my_label);
						break;
					case SR:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION SR\"];\n", my_label);
						break;
					case SL:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION SL\"];\n", my_label);
						break;
					case CASE_EQUAL:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION CASE_EQUAL\"];\n", my_label);
						break;
					case CASE_NOT_EQUAL:
						fprintf(fp, "\t%d [label=\"BINARY_OPERATION\"];\n", my_label);
						break;
					default:
						break;
				}
				break;
			case UNARY_OPERATION:
				switch (node->types.operation.op)
				{
					case ADD:
						fprintf(fp, "\t%d [label=\"UNARY_OPERATION ADD\"];\n", my_label);
						break;
					case MINUS:
						fprintf(fp, "\t%d [label=\"UNARY_OPERATION MINUS\"];\n", my_label);
						break;
					case BITWISE_NOT:
						fprintf(fp, "\t%d [label=\"UNARY_OPERATION BITWISE_NOT\"];\n", my_label);
						break;
					case BITWISE_AND:
						fprintf(fp, "\t%d [label=\"UNARY_OPERATION BITWISE_AND\"];\n", my_label);
						break;
					case BITWISE_OR:
						fprintf(fp, "\t%d [label=\"UNARY_OPERATION BITWISE_OR\"];\n", my_label);
						break;
					case BITWISE_NAND:
						fprintf(fp, "\t%d [label=\"UNARY_OPERATION BITWISE_NAND\"];\n", my_label);
						break;
					case BITWISE_NOR:
						fprintf(fp, "\t%d [label=\"UNARY_OPERATION BITWISE_NOR\"];\n", my_label);
						break;
					case BITWISE_XNOR:
						fprintf(fp, "\t%d [label=\"UNARY_OPERATION BITWISE_XNOR\"];\n", my_label);
						break;
					case BITWISE_XOR:
						fprintf(fp, "\t%d [label=\"UNARY_OPERATION BITWISE_XOR\"];\n", my_label);
						break;
					case LOGICAL_NOT:
						fprintf(fp, "\t%d [label=\"UNARY_OPERATION LOGICAL_NOT\"];\n", my_label);
						break;
					default:
						break;
				}
				break;
			case ARRAY_REF:
				fprintf(fp, "\t%d [label=\"ARRAY_REF\"];\n", my_label);
				break;
			case RANGE_REF:
				fprintf(fp, "\t%d [label=\"RANGE_REF\"];\n", my_label);
				break;
			case CONCATENATE:
				fprintf(fp, "\t%d [label=\"CONCATENATE\"];\n", my_label);
				break;
			case IDENTIFIERS:
				fprintf(fp, "\t%d [label=\"IDENTIFIERS:%s\"];\n", my_label, node->types.identifier);
				break;
			case NUMBERS:
				switch (node->types.number.base)
				{
					case(DEC):
						fprintf(fp, "\t%d [label=\"NUMBERS DEC:%s\"];\n", my_label, node->types.number.number);
						break;
					case(HEX):
						fprintf(fp, "\t%d [label=\"NUMBERS HEX:%s\"];\n", my_label, node->types.number.number);
						break;
					case(OCT):
						fprintf(fp, "\t%d [label=\"NUMBERS OCT:%s\"];\n", my_label, node->types.number.number);
						break;
					case(BIN):
						fprintf(fp, "\t%d [label=\"NUMBERS BIN:%s\"];\n", my_label, node->types.number.number);
						break;
					case(LONG_LONG):
						fprintf(fp, "\t%d [label=\"NUMBERS LONG_LONG:%lld\"];\n", my_label, node->types.number.value);
						break;
				}
				break;
			default:
				oassert(FALSE);
		}	
	}

	if (node != NULL)
	{
		/* print out the connection with the previous node */
		if (from != NULL)
			fprintf(fp, "\t%d -> %d;\n", from_num, my_label);

		for (i = 0; i < node->num_children; i++)
		{
			graphVizOutputAst_traverse_node(fp, node->children[i], node, my_label);
		}
	}
}

long calculate_operation(ast_node_t *node)
{
	if (node == NULL || node->num_children < 2)
			return 0;
	ast_node_t *newNode;

	long result, operand0, operand1;
	/*Only calculate binary operation currently*/
	if (node->type == BINARY_OPERATION)
	{
		/*calculate the first operand*/
		if(node->children[0]->type == IDENTIFIERS){
			long sc_spot;
			if((sc_spot = sc_lookup_string(defines_for_module_sc[num_modules], node->children[0]->types.identifier)) != -1)
			{
				newNode = defines_for_module_sc[num_modules]->data[sc_spot];
				if (newNode->types.variable.is_parameter == TRUE)
				{
					operand0 = newNode->types.number.value;
				}
				else
					error_message(PARSE_ERROR, node->children[0]->line_number, current_parse_file,
						"parameter %s don't match\n", node->children[0]->types.identifier);
			}
			else
				error_message(PARSE_ERROR, node->children[0]->line_number, current_parse_file,
					"parameter %s don't match\n", node->children[0]->types.identifier);
		}
		else if(node->children[0]->type == NUMBERS)
			operand0 = node->children[0]->types.number.value;
		else if(node->children[0]->type == BINARY_OPERATION)
			operand0 = calculate_operation(node->children[0]);

		/*calculate the second operand*/
		if(node->children[1]->type == IDENTIFIERS){
			long sc_spot;
			if((sc_spot = sc_lookup_string(defines_for_module_sc[num_modules], node->children[1]->types.identifier)) != -1)
			{
				newNode = defines_for_module_sc[num_modules]->data[sc_spot];
				if (newNode->types.variable.is_parameter == TRUE)
				{
					operand1 = newNode->types.number.value;
				}
				else
					error_message(PARSE_ERROR, node->children[1]->line_number, current_parse_file,
						"parameter %s don't match\n", node->children[1]->types.identifier);
			}
			else
				error_message(PARSE_ERROR, node->children[1]->line_number, current_parse_file,
					"parameter %s don't match\n", node->children[1]->types.identifier);
		}
		else if(node->children[1]->type == NUMBERS)
			operand1 = node->children[1]->types.number.value;
		else if(node->children[1]->type == BINARY_OPERATION)
			operand1 = calculate_operation(node->children[1]);

		result = calculate(operand0, operand1, node->types.operation.op);
		if(result < 0)
		{
			error_message(PARSE_ERROR, node->line_number, current_parse_file,
							"Negative numbers are used in the range in ODIN II!");
		}
		return result;
	}
	else
		error_message(PARSE_ERROR, node->line_number, current_parse_file,
				"ODIN II only can handle Binary Operation in Range!");

	return 0;
}
