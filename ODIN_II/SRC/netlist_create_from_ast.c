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
#include "types.h"
#include "globals.h"
#include "errors.h"
#include "netlist_utils.h"
#include "odin_util.h"
#include "ast_util.h"
#include "netlist_create_from_ast.h"
#include "string_cache.h"
#include "netlist_visualizer.h"
#include "parse_making_ast.h"
#include "node_creation_library.h"
#include "util.h"
#include "multipliers.h"
#include "hard_blocks.h"
#include "memories.h"
#include "implicit_memory.h"
#include "adders.h"
#include "subtractions.h"

/* NAMING CONVENTIONS
 {previous_string}.module_name+instance_name
 {previous_string}.module_name+instance_name^signal_name
 {previous_string}.module_name+instance_name^signal_name~bit
*/

#define INSTANTIATE_DRIVERS 1
#define ALIAS_INPUTS 2

#define COMBINATIONAL 1
#define SEQUENTIAL 2

STRING_CACHE *output_nets_sc;
STRING_CACHE *input_nets_sc;

STRING_CACHE *local_symbol_table_sc;
STRING_CACHE *global_param_table_sc;
ast_node_t** local_symbol_table;
int num_local_symbol_table;
signal_list_t *local_clock_list;
short local_clock_found;
int local_clock_idx;

/* CONSTANT NET ELEMENTS */
char *one_string = "ONE_VCC_CNS";
char *zero_string = "ZERO_GND_ZERO";
char *pad_string = "ZERO_PAD_ZERO";

ast_node_t *top_module;

netlist_t *verilog_netlist;

int netlist_create_line_number = -2;

int type_of_circuit;


/* PROTOTYPES */

void convert_ast_to_netlist_recursing_via_modules(ast_node_t* current_module, char *instance_name, int level);
signal_list_t *netlist_expand_ast_of_module(ast_node_t* node, char *instance_name_prefix);

void create_all_driver_nets_in_this_module(char *instance_name_prefix);

ast_node_t *find_top_module();

void create_top_driver_nets(ast_node_t* module, char *instance_name_prefix);
void create_top_output_nodes(ast_node_t* module, char *instance_name_prefix);
nnet_t* define_nets_with_driver(ast_node_t* var_declare, char *instance_name_prefix);
nnet_t* define_nodes_and_nets_with_driver(ast_node_t* var_declare, char *instance_name_prefix);

void connect_hard_block_and_alias(ast_node_t* module_instance, char *instance_name_prefix, int outport_size);
void connect_module_instantiation_and_alias(short PASS, ast_node_t* module_instance, char *instance_name_prefix);
void create_symbol_table_for_module(ast_node_t* module_items, char *module_name);
int check_for_initial_reg_value(ast_node_t* var_declare, long long *value);

signal_list_t *concatenate_signal_lists(signal_list_t **signal_lists, int num_signal_lists);

signal_list_t *create_gate(ast_node_t* gate, char *instance_name_prefix);
signal_list_t *create_hard_block(ast_node_t* block, char *instance_name_prefix);
signal_list_t *create_pins(ast_node_t* var_declare, char *name, char *instance_name_prefix);
signal_list_t *create_output_pin(ast_node_t* var_declare, char *instance_name_prefix);
signal_list_t *assignment_alias(ast_node_t* assignment, char *instance_name_prefix);
signal_list_t *create_operation_node(ast_node_t *op, signal_list_t **input_lists, int list_size, char *instance_name_prefix);

void terminate_continuous_assignment(ast_node_t *node, signal_list_t* assignment, char *instance_name_prefix);
void terminate_registered_assignment(ast_node_t *always_node, signal_list_t* assignment, signal_list_t *potential_clocks, char *instance_name_prefix);

signal_list_t *create_case(ast_node_t *case_ast, char *instance_name_prefix);
void create_case_control_signals(ast_node_t *case_list_of_items, ast_node_t *compare_against, nnode_t *case_node, char *instance_name_prefix);
signal_list_t *create_case_mux_statements(ast_node_t *case_list_of_items, nnode_t *case_node, char *instance_name_prefix);
signal_list_t *create_if(ast_node_t *if_ast, char *instance_name_prefix);
void create_if_control_signals(ast_node_t *if_expression, nnode_t *if_node, char *instance_name_prefix);
signal_list_t *create_if_mux_statements(ast_node_t *if_ast, nnode_t *if_node, char *instance_name_prefix);
signal_list_t *create_if_for_question(ast_node_t *if_ast, char *instance_name_prefix);
signal_list_t *create_if_question_mux_expressions(ast_node_t *if_ast, nnode_t *if_node, char *instance_name_prefix);
signal_list_t *create_mux_statements(signal_list_t **statement_lists, nnode_t *case_node, int num_statement_lists, char *instance_name_prefix);
signal_list_t *create_mux_expressions(signal_list_t **expression_lists, nnode_t *mux_node, int num_expression_lists, char *instance_name_prefix);

signal_list_t *evaluate_sensitivity_list(ast_node_t *delay_control, char *instance_name_prefix);

int alias_output_assign_pins_to_inputs(char_list_t *output_list, signal_list_t *input_list, ast_node_t *node);

int find_smallest_non_numerical(ast_node_t *node, signal_list_t **input_list, int num_input_lists);
void pad_with_zeros(ast_node_t* node, signal_list_t *list, int pad_size, char *instance_name_prefix);

void look_for_clocks(netlist_t *netlist);


/*----------------------------------------------------------------------------
 * (function: create_param_table_for_module)
 *--------------------------------------------------------------------------*/
/**
 * Scan through all VAR_DECLARE_LISTs in MODULE_ITEMS and create a hash table
 * for all parameters in this instantiation, taking into account if they
 * are being overridden by their parent
 */
void create_param_table_for_module(ast_node_t* parent_parameter_list, ast_node_t *module_items, char *module_name)
{
	/* with the top module we need to visit the entire ast tree */
	int i, j, k;
	char *temp_string;
	long sc_spot;
	oassert(module_items->type == MODULE_ITEMS);
	int parameter_num = 0;
	int parameter_count = 0;
	STRING_CACHE *local_param_table_sc;

	sc_spot = sc_add_string(global_param_table_sc, module_name);
	if (global_param_table_sc->data[sc_spot] != NULL) 
		return;
	local_param_table_sc = sc_new_string_cache();
	global_param_table_sc->data[sc_spot] = local_param_table_sc;

	/* search for VAR_DECLARE_LISTS */
	if (module_items->num_children > 0)
	{
		for(i = 0; i < module_items->num_children; i++)
		{
			if(module_items->children[i]->type == VAR_DECLARE_LIST)
			{
				/* go through the vars in this declare list, resolves the missing identifiers node */
				for(j = 0; j < module_items->children[i]->num_children; j++)
				{
					ast_node_t *var_declare = module_items->children[i]->children[j];

					if ((var_declare->types.variable.is_input) ||
						(var_declare->types.variable.is_output) ||
						(var_declare->types.variable.is_reg) ||
						(var_declare->types.variable.is_wire)) continue;

					oassert(module_items->children[i]->children[j]->type == VAR_DECLARE);
					oassert(var_declare->types.variable.is_parameter);

					parameter_num++;
					/* make the string to add to the string cache */
					temp_string = make_full_ref_name(NULL, NULL, NULL, var_declare->children[0]->types.identifier, -1);

					if (var_declare->types.variable.is_parameter)
					{
						ast_node_t *node = resolve_node(module_name, var_declare->children[5]);
						oassert(node->type == NUMBERS);
						sc_spot = sc_add_string(local_param_table_sc, temp_string);
						local_param_table_sc->data[sc_spot] = (void *)node;
					}
					free(temp_string);
				}
			}
		}

		if(parent_parameter_list)
		{
			// defparam before calling instance
			for(i = 0; i < parent_parameter_list->num_children; i ++)
			{
				if(parent_parameter_list->children[i]->children[0] && parent_parameter_list->children[i]->shared_node == FALSE)
				{
					ast_node_t *var_declare = parent_parameter_list->children[i];
					ast_node_t *node = resolve_node(module_name, var_declare->children[5]);
					oassert(node->type == NUMBERS);
					sc_spot = sc_add_string(local_param_table_sc, var_declare->children[0]->types.identifier);
					local_param_table_sc->data[sc_spot] = (void *)node;
				}
			}

			for(i = 0; i < parent_parameter_list->num_children; i ++)
			{
				//using defparam to override parameter, after calling instance
				if(parent_parameter_list->children[i]->children[0])
				{
					if(parent_parameter_list->children[i]->shared_node == TRUE)
					{
						ast_node_t *var_declare = parent_parameter_list->children[i];
						ast_node_t *node = resolve_node(module_name, var_declare->children[5]);
						oassert(node->type == NUMBERS);
						sc_spot = sc_add_string(local_param_table_sc, var_declare->children[0]->types.identifier);
						local_param_table_sc->data[sc_spot] = (void *)node;
					}
				}
				else
				{
					parameter_count++;
					int parameter_idx = 0;
					for(k = 0; k < module_items->num_children; k++)
					{
						/* go through the vars in this declare list, resolves the missing identifiers node */
						if(module_items->children[k]->type == VAR_DECLARE_LIST)
						{
							for(j = 0; j < module_items->children[k]->num_children; j++)
							{
								ast_node_t *var_declare = module_items->children[k]->children[j];

								if ((var_declare->types.variable.is_input) ||
									(var_declare->types.variable.is_output) ||
									(var_declare->types.variable.is_reg) ||
									(var_declare->types.variable.is_wire)) continue;

								oassert(module_items->children[k]->children[j]->type == VAR_DECLARE);
								oassert(var_declare->types.variable.is_parameter);

								parameter_idx++;

								/* make the string to add to the string cache */
								temp_string = make_full_ref_name(NULL, NULL, NULL, var_declare->children[0]->types.identifier, -1);

								if (var_declare->types.variable.is_parameter)
								{
									//parameter_num++;
									if(parameter_idx == parameter_count)
									{
										var_declare = parent_parameter_list->children[i];
										ast_node_t *node = resolve_node(module_name, var_declare->children[5]);
										oassert(node->type == NUMBERS);
										sc_spot = sc_add_string(local_param_table_sc, temp_string);
										local_param_table_sc->data[sc_spot] = (void *)node;
									}
									else
									{
										ast_node_t *node = resolve_node(module_name, var_declare->children[5]);
										oassert(node->type == NUMBERS);
										sc_spot = sc_add_string(local_param_table_sc, temp_string);
										local_param_table_sc->data[sc_spot] = (void *)node;
									}
								}
								free(temp_string);
							}
						}
					}
					if(parameter_idx == 0)
					{
						error_message(NETLIST_ERROR, parent_parameter_list->line_number, parent_parameter_list->file_number,
								"There is no parameters in %s !",
								 module_name);
					}
				}
			}

		}

		if(parameter_count > parameter_num && parameter_count > 0)
		{
			error_message(NETLIST_ERROR, parent_parameter_list->line_number, parent_parameter_list->file_number,
					"There are less parameters (%d) in %s than there are specified in the module instantiation (%d)!",
					parameter_num, module_name, parameter_count);
		}
		else if(parameter_count < parameter_num && parameter_count > 0)
		{
			warning_message(NETLIST_ERROR, parent_parameter_list->line_number, parent_parameter_list->file_number,
					"There are more parameters (>%d) in %s than there are specified in the module instantiation (%d)!",
					parameter_count, module_name, parameter_num);
		}
	}
}



/*---------------------------------------------------------------------------------------------
 * (function: create_netlist)
 *--------------------------------------------------------------------------*/
void create_netlist()
{
	/* just build all the fundamental elements, and then hookup with port definitions...every net has
	 * a named net as a variable instance...even modules...the only trick with modules will
	 * be instance names.  There are a few implied nets as in Muxes for cases, signals
	 * for flip-flops and memories */

	// Alias the symbol nodes in ast_modules to the actual MODULE nodes
	int i;
	for (i = 0; i < num_modules; i++)
	{
		if (ast_modules[i]->type == IDENTIFIERS)
		{
			// double check that it's in modules_names_to_idx
			char *module_param_name = ast_modules[i]->types.identifier;
			long sc_spot = sc_lookup_string(module_names_to_idx, module_param_name);
			oassert(sc_spot > -1);
			// now isolate the original module name
			char *underscores = strstr(module_param_name, "___");
			oassert(underscores);
			int len = underscores - module_param_name;
			char *module_name = (char *)malloc((len+1)*sizeof(char));
			strncpy(module_name, module_param_name, len);
			module_name[len] = '\0';
			// verify that it does exist
			long sc_spot2 = sc_lookup_string(module_names_to_idx, module_name);
			oassert(sc_spot2 > -1);
			free(module_name);
			// create a new MODULE node with new IDENTIFIER, but keep same ports and module_items
			ast_node_t *module = (ast_node_t *)module_names_to_idx->data[sc_spot2];
			ast_node_t *symbol_node = newSymbolNode(module_param_name, module->line_number);
			ast_node_t* new_node = create_node_w_type(MODULE, module->line_number, module->file_number);
			allocate_children_to_node(new_node, 3, symbol_node, module->children[1], module->children[2]);
			module->types.module.is_instantiated = TRUE;
			new_node->types.module.index = i;
			new_node->types.module.is_instantiated = TRUE;
			// and to the module_names_to_idx for parameterised name
			module_names_to_idx->data[sc_spot] = new_node;
			ast_modules[i] = new_node;
		}
		oassert(ast_modules[i]->type == MODULE);
	}

	/* we will find the top module */
	top_module = find_top_module();

	/* Since the modules are in a tree, we will bottom up build the netlist.  Essentially,
	 * we will go to the leafs of the module tree, build them upwards such that when we search for the nets,
	 * we will find them and can hook them up at that point */

	/* PASS 1 - we make all the nets based on registers defined in modules */
	
	/* initialize the string caches that hold the aliasing of module nets and input pins */
	output_nets_sc = sc_new_string_cache();
	input_nets_sc = sc_new_string_cache();
	/* initialize the storage of the top level drivers.  Assigned in create_top_driver_nets */
	verilog_netlist = allocate_netlist();

	// create the parameter table for the top module
	create_param_table_for_module(NULL, top_module->children[2], "top");

	/* now recursively parse the modules by going through the tree of modules starting at top */
	create_top_driver_nets(top_module, "top");
	init_implicit_memory_index();
	convert_ast_to_netlist_recursing_via_modules(top_module, "top", 0);
	free_implicit_memory_index_and_finalize_memories();
	create_top_output_nodes(top_module, "top");

	/* now look for high-level signals */
	look_for_clocks(verilog_netlist); 
}

/*---------------------------------------------------------------------------------------------
   * (function: look_for_clocks)
 *-------------------------------------------------------------------------------------------*/
void look_for_clocks(netlist_t *netlist)
{
	int i; 

	for (i = 0; i < netlist->num_ff_nodes; i++)
	{
		if (netlist->ff_nodes[i]->input_pins[1]->net->driver_pin->node->type != CLOCK_NODE)
		{
			netlist->ff_nodes[i]->input_pins[1]->net->driver_pin->node->type = CLOCK_NODE;
		}
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: find_top_module)
 * 	Finds the top module based on that it is not called by anyone else
 * 	Assumes there is only one top
 *-------------------------------------------------------------------------------------------*/
ast_node_t *find_top_module()
{
	int i, j;
	long sc_spot;
	int found_top = -1;

	/* go through all the instantiations for each module and mark them if they've be instantiated */
	for (i = 0; i < num_modules; i++)
	{
		for (j = 0; j < ast_modules[i]->types.module.size_module_instantiations; j++)
		{
			/* Check to see if the module is a hard block - a hard block is 
				never the top level! */
			#ifdef VPR6
			if ((sc_spot = sc_lookup_string(hard_block_names, ast_modules[i]->types.module.module_instantiations_instance[j]->children[0]->types.identifier)) == -1)
			#endif
			{
				// get the module index from the string cache
				if ((sc_spot = sc_lookup_string(module_names_to_idx, 
								ast_modules[i]->types.module.module_instantiations_instance[j]->children[0]->types.identifier)) == -1)
				{
					error_message(NETLIST_ERROR, 
							ast_modules[i]->line_number, 
							ast_modules[i]->file_number, 
							"Can't find module name (%s)\n", 
							ast_modules[i]->types.module.module_instantiations_instance[j]->children[0]->types.identifier);
				}

				/* use that number to mark this module as instantiated */
				ast_modules[((ast_node_t*)module_names_to_idx->data[sc_spot])->types.module.index]->types.module.is_instantiated = TRUE;
			}
		}
	}

	/* now check for which module wasn't marked...this one will be the top */
	for (i = 0; i < num_modules; i++)
	{
		if ((ast_modules[i]->types.module.is_instantiated == FALSE) && (found_top == -1))
		{
			found_top = i;
		}
		else if ((ast_modules[i]->types.module.is_instantiated == FALSE) && (found_top != -1))
		{
			error_message(NETLIST_ERROR, ast_modules[i]->line_number, ast_modules[i]->file_number,
					"Two top level modules - Odin II cannot deal with these types of designs\n");
		}
	}

	/* check atleast one module is top ... and only one */
	if (found_top == -1)
	{
		error_message(NETLIST_ERROR, -1, -1, "Could not find a top level module\n");
	}

	return ast_modules[found_top];
}

/*---------------------------------------------------------------------------------------------
 * (function: convert_ast_to_netlist_recursing_via_modules)
 * 	Recurses through modules by depth first traversal of the tree of modules.  Expands
 * 	the netlists at each level.
 *-------------------------------------------------------------------------------------------*/
void convert_ast_to_netlist_recursing_via_modules(ast_node_t* current_module, char *instance_name, int level)
{
	signal_list_t *list = NULL;

	/* BASE CASE is when there are no other instantiations of modules in this module */
	if (current_module->types.module.size_module_instantiations == 0)
	{
		list = netlist_expand_ast_of_module(current_module, instance_name);
	}
	else
	{
		/* ELSE - we need to visit all the children before */
		int i, j;
		//check for defparam
		for(i = 0; i <current_module->num_children; i++)
		{
			if(current_module->children[i]->type == MODULE_ITEMS)
			{
				for(j = 0; j < current_module->children[i]->num_children; j++)
				{
					if(current_module->children[i]->children[j]->type == MODULE_PARAMETER)
					{
						int k;
						int flag = 0;
						for(k = 0; k < current_module->types.module.size_module_instantiations; k++)
						{
							if(current_module->types.module.module_instantiations_instance[k]->children[1])
							{
								ast_node_t *module_instance = current_module->types.module.module_instantiations_instance[k]->children[1];
								if(strcmp(module_instance->children[0]->types.identifier, current_module->children[i]->children[j]->types.identifier) == 0)
								{

									if(module_instance->children[2])
									{
										move_ast_node(current_module->children[i], module_instance->children[2], current_module->children[i]->children[j]);
									}
									else
									{
										ast_node_t* new_node = create_node_w_type(MODULE_PARAMETER_LIST, module_instance->line_number, current_parse_file);
										module_instance->children[2] = new_node;
										move_ast_node(current_module->children[i], module_instance->children[2], current_module->children[i]->children[j]);
									}
									flag = 1;
									break;
								}
							}
						}
						if(flag == 0)
						{
							error_message(NETLIST_ERROR, current_module->line_number, current_module->file_number,
									"Can't find module name %s\n", current_module->children[i]->children[j]->types.identifier);
						}
					}
				}
			}
		}

		for (i = 0; i < current_module->types.module.size_module_instantiations; i++)
		{
			/* make the stringed up module instance name - instance name is
			 * MODULE_INSTANCE->MODULE_NAMED_INSTANCE(child[1])->IDENTIFIER(child[0]).
			 * module name is MODULE_INSTANCE->IDENTIFIER(child[0])
			 */
			char *temp_instance_name = make_full_ref_name(instance_name,
					current_module->types.module.module_instantiations_instance[i]->children[0]->types.identifier,
					current_module->types.module.module_instantiations_instance[i]->children[1]->children[0]->types.identifier,
					NULL, -1);

			long sc_spot;
			/* lookup the name of the module associated with this instantiated point */
			if ((sc_spot = sc_lookup_string(module_names_to_idx, current_module->types.module.module_instantiations_instance[i]->children[0]->types.identifier)) == -1)
			{
				error_message(NETLIST_ERROR, current_module->line_number, current_module->file_number,
						"Can't find module name %s\n", current_module->types.module.module_instantiations_instance[i]->children[0]->types.identifier);
			}

			ast_node_t *parent_parameter_list = current_module->types.module.module_instantiations_instance[i]->children[1]->children[2];
			// create the parameter table for the instantiated module
			create_param_table_for_module(parent_parameter_list,
				/* module_items */
				((ast_node_t*)module_names_to_idx->data[sc_spot])->children[2],
				temp_instance_name);

			/* recursive call point */
			convert_ast_to_netlist_recursing_via_modules(((ast_node_t*)module_names_to_idx->data[sc_spot]), temp_instance_name, level+1);

			/* free the string */
			free(temp_instance_name);
		}
		
		/* once we've done everyone lower, we can do this module */
		list = netlist_expand_ast_of_module(current_module, instance_name);
	}

	if (list) free_signal_list(list);
}

/*---------------------------------------------------------------------------
 * (function: netlist_expand_ast_of_module)
 * 	Main recursion point that looks at the abstract syntax tree.
 * 	Allows for a pre amble, looks at children (dfs), then does post amble
 * 	Can skip children traverse and the ambles...
 *-------------------------------------------------------------------------*/
signal_list_t *netlist_expand_ast_of_module(ast_node_t* node, char *instance_name_prefix)
{
	/* with the top module we need to visit the entire ast tree */
	int i;
	short *child_skip_list = NULL; // list of children not to traverse into
	short skip_children = FALSE; // skips the DFS completely if TRUE 
	signal_list_t *return_sig_list = NULL;
	signal_list_t **children_signal_list = NULL;

	if (node == NULL)
	{
		/* print out the node and label details */
	}
	else
	{
		/* make a skip list of nodes in the tree we don't need to visit for this node.
		 * For example, a module we know that the 0th entry is the identifier of the module */
		if (node->num_children > 0)
		{
			child_skip_list = (short*)calloc(node->num_children, sizeof(short));
			children_signal_list = (signal_list_t**)malloc(sizeof(signal_list_t*)*node->num_children);
		}

		/* ------------------------------------------------------------------------------*/
		/* PREAMBLE */
		switch(node->type)
		{
			case FILE_ITEMS:
				oassert(FALSE);
				break;
			case MODULE: 
				/* set the skip list */
				child_skip_list[0] = TRUE; /* skip the identifier */
				child_skip_list[1] = TRUE; /* skip portlist ... we'll use where they're defined */
				break;
			case MODULE_ITEMS: 
				/* items include: wire, reg, input, outputs, assign, gate, module_instance, always */

				/* make the symbol table */
				local_symbol_table_sc = sc_new_string_cache();
				local_symbol_table = NULL;
				num_local_symbol_table = 0;
				create_symbol_table_for_module(node, instance_name_prefix);
				local_clock_found = FALSE;

				/* create all the driven nets based on the "reg" registers */		
				create_all_driver_nets_in_this_module(instance_name_prefix);

				/* process elements in the list */
				if (node->num_children > 0)
				{
					for (i = 0; i < node->num_children; i++)
					{
						if (node->children[i]->type == VAR_DECLARE_LIST)
						{
							/* IF - The port lists of this module are handled else where */
							child_skip_list[i] = TRUE;
						}
						else if (node->children[i]->type == MODULE_INSTANCE)
						{
							/* ELSE IF - we deal with instantiations of modules twice to alias input and output nets.  In this
							 * pass we are looking for any drivers emerging from a module */

							/* make the aliases for all the drivers as they're passed through modules */
							connect_module_instantiation_and_alias(INSTANTIATE_DRIVERS, node->children[i], instance_name_prefix);

							/* is a call site for another module.  Alias names to nets and pins */
							child_skip_list[i] = TRUE;
						}
					}
				}
				break;
			case GATE:
				/* create gate instances */
				return_sig_list = create_gate(node, instance_name_prefix);
				/* create_gate does it's own instantiations so skip the children in the traverse */
				skip_children = TRUE;
				break;
			/* ---------------------- */
			/* All these are input references that we need to grab their pins from by create_pin */
			case ARRAY_REF:
			{
				return_sig_list = create_pins(node, NULL, instance_name_prefix);
				skip_children = TRUE;
				break;
			}
			case IDENTIFIERS:
			{
				return_sig_list = create_pins(node, NULL, instance_name_prefix);
				break;
			}
			case RANGE_REF:
			case NUMBERS:
			{
				return_sig_list = create_pins(node, NULL, instance_name_prefix);
				/* children are traversed in the create_pin function */
				//skip_children = TRUE;
				break;
			}
			/* ---------------------- */
			case ASSIGN:
				/* combinational path */
				type_of_circuit = COMBINATIONAL;
				break;
			case BLOCKING_STATEMENT:
			{
				if (type_of_circuit == SEQUENTIAL)
					error_message(NETLIST_ERROR, node->line_number, node->file_number,
							"ODIN doesn't handle blocking statements in Sequential blocks\n");

				return_sig_list = assignment_alias(node, instance_name_prefix);
				skip_children = TRUE;
				break;
			}
			case NON_BLOCKING_STATEMENT:
			{
				if (type_of_circuit != SEQUENTIAL)
					error_message(NETLIST_ERROR, node->line_number, node->file_number,
							"ODIN doesn't handle non blocking statements in combinational blocks\n");

				return_sig_list = assignment_alias(node, instance_name_prefix);
				skip_children = TRUE;
				break;
			}
			case ALWAYS:
				/* evaluate if this is a sensitivity list with posedges/negedges (=SEQUENTIAL) or none (=COMBINATIONAL) */
				local_clock_list = evaluate_sensitivity_list(node->children[0], instance_name_prefix);
				child_skip_list[0] = TRUE;
				break;
			case CASE:
				return_sig_list = create_case(node, instance_name_prefix);
				skip_children = TRUE;
				break;
			case IF:
				return_sig_list = create_if(node, instance_name_prefix);
				skip_children = TRUE;
				break;
			case IF_Q:
				return_sig_list = create_if_for_question(node, instance_name_prefix);
				skip_children = TRUE;
				break;
			#ifdef VPR6
			case HARD_BLOCK:
				/* set the skip list */
				child_skip_list[0] = TRUE; /* skip the identifier */
				child_skip_list[1] = TRUE; /* skip portlist ... we'll use where they're defined */
				return_sig_list = create_hard_block(node, instance_name_prefix);
				break;
			#endif
			default:
				break;
		}	
		/* ------------------------------------------------------------------------------*/
		/* This is the depth first traverse (DFT aka DFS) of the ast nodes that make up the netlist. */
		if (skip_children == FALSE)
		{
			/* traverse all the children */
			for (i = 0; i < node->num_children; i++)
			{
				if (child_skip_list[i] == FALSE)
				{
					/* recursively call through the tree going to each instance.  This is depth first traverse. */
					children_signal_list[i] = netlist_expand_ast_of_module(node->children[i], instance_name_prefix);
				}
			}
		}

		/* ------------------------------------------------------------------------------*/
		/* POST AMBLE - process the children */
		switch(node->type)
		{
			case FILE_ITEMS:
				error_message(NETLIST_ERROR, node->line_number, node->file_number,
						"FILE_ITEMS are not supported by Odin.\n");
				break;
			case CONCATENATE:
				return_sig_list = concatenate_signal_lists(children_signal_list, node->num_children);
				break;
			case MODULE_ITEMS: 
			{
				if (node->num_children > 0)
				{
					for (i = 0; i < node->num_children; i++)
					{
						if (node->children[i]->type == MODULE_INSTANCE)
						{
							/* make the aliases for all the drivers as they're passed through modules.  This is the
 							second time we call this */
							connect_module_instantiation_and_alias(ALIAS_INPUTS, node->children[i], instance_name_prefix);
						}
					}
				}

				/* free the symbol table for this module since we're done processing */
				sc_free_string_cache(local_symbol_table_sc);
				free(local_symbol_table);

				break;
			}
			case ASSIGN:
				oassert(node->num_children == 1);	
				/* attach the drivers to the driver nets */
				terminate_continuous_assignment(node, children_signal_list[0], instance_name_prefix); 	
				break;
			case ALWAYS:
				/* attach the drivers to the driver nets */
				if (type_of_circuit == COMBINATIONAL)
				{
					/* idx 1 element since always has DELAY Control first */
					terminate_continuous_assignment(node, children_signal_list[1], instance_name_prefix); 	
				}
				else if (type_of_circuit == SEQUENTIAL)
				{
					terminate_registered_assignment(node, children_signal_list[1], local_clock_list, instance_name_prefix); 	
				}
				else
				{
					printf("Assignment outside of always block.");
					oassert(FALSE);
				}
				break;
			case BINARY_OPERATION: 
				oassert(node->num_children == 2);
				return_sig_list = create_operation_node(node, children_signal_list, node->num_children, instance_name_prefix);
				break;
			case UNARY_OPERATION:
				oassert(node->num_children == 1);
				return_sig_list = create_operation_node(node, children_signal_list, node->num_children, instance_name_prefix);
				break;
			case BLOCK: 
				return_sig_list = combine_lists(children_signal_list, node->num_children);
				break;
			#ifdef VPR6
			case RAM: 
				connect_memory_and_alias(node, instance_name_prefix);
				break;
			case HARD_BLOCK:
				connect_hard_block_and_alias(node, instance_name_prefix, return_sig_list->count);
				break;
			#endif
			case IF:
			default:
				break;
		}
		/* ------------------------------------------------------------------------------*/
	}

	/* cleaning */
	if (child_skip_list != NULL)
	{
		free(child_skip_list);
	}
	if (children_signal_list != NULL)
	{
		free(children_signal_list);
	}

	return return_sig_list;
}

/*
 * Concatenates the given signal lists together.
 *
 * Ignores the carry out from adders to maintain width expectations.
 */
signal_list_t *concatenate_signal_lists(signal_list_t **signal_lists, int num_signal_lists)
{
	signal_list_t *return_list = init_signal_list();

	int i;
	for (i = num_signal_lists - 1; i >= 0; i--)
	{
		// When concatenating, ignore the carry out from adders so that they occupy the expected width.
		if (signal_lists[i]->is_adder)
			signal_lists[i]->count--;

		int j;
		for (j = 0; j < signal_lists[i]->count; j++)
		{
			npin_t *pin = signal_lists[i]->pins[j];
			add_pin_to_signal_list(return_list, pin);
		}
		free_signal_list(signal_lists[i]);
	}

	return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function: create_all_driver_nets_in_this_module)
 * 	Use to define all the nets that will connect to instantiated nodes.  This means we go
 * 	through the var_declare_list of module variables and all registers are made
 * 	as drivers.
 *-------------------------------------------------------------------------------------------*/
void create_all_driver_nets_in_this_module(char *instance_name_prefix)
{
	/* with the top module we need to visit the entire ast tree */
	int i;

	/* use the symbol table to decide if driver */
	for (i = 0; i < num_local_symbol_table; i++)
	{
		oassert(local_symbol_table[i]->type == VAR_DECLARE);
		if (
			/* all registers are drivers */
				(local_symbol_table[i]->types.variable.is_reg)
			/* a wire that is an input can be a driver */
			|| (
					   (local_symbol_table[i]->types.variable.is_wire)
					&& (!local_symbol_table[i]->types.variable.is_input)
			)
			/* any output that has nothing else is an implied wire driver */
			|| (
					   (local_symbol_table[i]->types.variable.is_output)
					&& (!local_symbol_table[i]->types.variable.is_reg)
					&& (!local_symbol_table[i]->types.variable.is_wire)
			)
		   )
		{
			/* create nets based on this driver as the name */
			define_nets_with_driver(local_symbol_table[i], instance_name_prefix);
		}

	}
}

/*--------------------------------------------------------------------------
 * (function: create_top_driver_nets)
 * 	This function creates the drivers for the top module.  Top module is 
 * 	slightly special since all inputs are actually drivers.
 * 	Also make the 0 and 1 constant nodes at this point.
 *  Note: Also creates hbpad signal for padding hard block inputs.
 *-------------------------------------------------------------------------*/
void create_top_driver_nets(ast_node_t* module, char *instance_name_prefix)
{
	/* with the top module we need to visit the entire ast tree */
	int i, j;
	long sc_spot;
	ast_node_t *module_items = module->children[2];
	npin_t *new_pin;

	oassert(module_items->type == MODULE_ITEMS);

	/* search for VAR_DECLARE_LISTS */
	if (module_items->num_children > 0)
	{
		for (i = 0; i < module_items->num_children; i++)
		{
			if (module_items->children[i]->type == VAR_DECLARE_LIST)
			{
				/* go through the vars in this declare list looking for inputs to make drivers */
				for (j = 0; j < module_items->children[i]->num_children; j++)
				{	
					if (module_items->children[i]->children[j]->types.variable.is_input)
					{
						define_nodes_and_nets_with_driver(module_items->children[i]->children[j], instance_name_prefix);
					}
				}
			}
		}
	}
	else
	{
		error_message(NETLIST_ERROR, module_items->line_number, module_items->file_number, "Module is empty\n");
	}

	/* create the constant nets */
	verilog_netlist->zero_net = allocate_nnet();
	verilog_netlist->gnd_node = allocate_nnode();
	verilog_netlist->gnd_node->type = GND_NODE;
	allocate_more_output_pins(verilog_netlist->gnd_node, 1);
	add_output_port_information(verilog_netlist->gnd_node, 1);
	new_pin = allocate_npin();
	add_output_pin_to_node(verilog_netlist->gnd_node, new_pin, 0);
	add_driver_pin_to_net(verilog_netlist->zero_net, new_pin);

	verilog_netlist->one_net = allocate_nnet();
	verilog_netlist->vcc_node = allocate_nnode();
	verilog_netlist->vcc_node->type = VCC_NODE;
	allocate_more_output_pins(verilog_netlist->vcc_node, 1);
	add_output_port_information(verilog_netlist->vcc_node, 1);
	new_pin = allocate_npin();
	add_output_pin_to_node(verilog_netlist->vcc_node, new_pin, 0);
	add_driver_pin_to_net(verilog_netlist->one_net, new_pin);

	verilog_netlist->pad_net = allocate_nnet();
	verilog_netlist->pad_node = allocate_nnode();
	verilog_netlist->pad_node->type = PAD_NODE;
	allocate_more_output_pins(verilog_netlist->pad_node, 1);
	add_output_port_information(verilog_netlist->pad_node, 1);
	new_pin = allocate_npin();
	add_output_pin_to_node(verilog_netlist->pad_node, new_pin, 0);
	add_driver_pin_to_net(verilog_netlist->pad_net, new_pin);

	/* CREATE the driver for the ZERO */
	zero_string = make_full_ref_name(instance_name_prefix, NULL, NULL, zero_string, -1);
	verilog_netlist->gnd_node->name = zero_string;

	sc_spot = sc_add_string(output_nets_sc, zero_string);
	if (output_nets_sc->data[sc_spot] != NULL)
	{
		error_message(NETLIST_ERROR, module_items->line_number, module_items->file_number, "Error in Odin\n");
	}
	/* store the data which is an idx here */
	output_nets_sc->data[sc_spot] = (void*)verilog_netlist->zero_net;
	verilog_netlist->zero_net->name = zero_string;

	/* CREATE the driver for the ONE and store twice */
	one_string = make_full_ref_name(instance_name_prefix, NULL, NULL, one_string, -1);
	verilog_netlist->vcc_node->name = one_string;

	sc_spot = sc_add_string(output_nets_sc, one_string);
	if (output_nets_sc->data[sc_spot] != NULL)
	{
		error_message(NETLIST_ERROR, module_items->line_number, module_items->file_number, "Error in Odin\n");
	}
	/* store the data which is an idx here */
	output_nets_sc->data[sc_spot] = (void*)verilog_netlist->one_net;
	verilog_netlist->one_net->name = one_string;

	/* CREATE the driver for the PAD */
	pad_string = make_full_ref_name(instance_name_prefix, NULL, NULL, pad_string, -1);
	verilog_netlist->pad_node->name = pad_string;

	sc_spot = sc_add_string(output_nets_sc, pad_string);
	if (output_nets_sc->data[sc_spot] != NULL)
	{
		error_message(NETLIST_ERROR, module_items->line_number, module_items->file_number, "Error in Odin\n");
	}
	/* store the data which is an idx here */
	output_nets_sc->data[sc_spot] = (void*)verilog_netlist->pad_net;
	verilog_netlist->pad_net->name = pad_string;
}

/*---------------------------------------------------------------------------------------------
 * (function: create_top_output_nodes)
 * 	This function is for the top module and makes references for the output pins
 * 	as actual nodes in the netlist and hooks them up to the netlist as it has been 
 * 	created.  Therefore, this is one of the last steps when creating the netlist.
 *-------------------------------------------------------------------------------------------*/
void create_top_output_nodes(ast_node_t* module, char *instance_name_prefix)
{
	/* with the top module we need to visit the entire ast tree */
	int i, j, k;
	long sc_spot;
	ast_node_t *module_items = module->children[2];
	nnode_t *new_node;

	oassert(module_items->type == MODULE_ITEMS);

	/* search for VAR_DECLARE_LISTS */
	if (module_items->num_children > 0)
	{
		for (i = 0; i < module_items->num_children; i++)
		{
			if (module_items->children[i]->type == VAR_DECLARE_LIST)
			{
				/* go through the vars in this declare list */
				for (j = 0; j < module_items->children[i]->num_children; j++)
				{	
					if (module_items->children[i]->children[j]->types.variable.is_output)
					{
						char *full_name;
						ast_node_t *var_declare = module_items->children[i]->children[j];
						npin_t *new_pin;

						/* decide what type of variable this is */
						if (var_declare->children[1] == NULL)
						{
							/* IF - this is a identifier then find it in the output_nets and hook it up to a newly created node */
							/* get the name of the pin */
							full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, var_declare->children[0]->types.identifier, -1);
	
							/* check if the instantiation pin exists. */
							if ((sc_spot = sc_lookup_string(output_nets_sc, full_name)) == -1)
							{
								error_message(NETLIST_ERROR, var_declare->children[0]->line_number, var_declare->children[0]->file_number,
										"Output pin (%s) is not hooked up!!!\n", full_name);
							}
	
							new_pin = allocate_npin();
							/* hookup this pin to this net */
							add_fanout_pin_to_net((nnet_t*)output_nets_sc->data[sc_spot], new_pin);
	
							/* create the node */
							new_node = allocate_nnode();
							new_node->related_ast_node = module_items->children[i]->children[j];
							new_node->name = full_name;
							new_node->type = OUTPUT_NODE;
							/* allocate the pins needed */
							allocate_more_input_pins(new_node, 1);
							add_input_port_information(new_node, 1);
	
							/* hookup the pin, and node */
							add_input_pin_to_node(new_node, new_pin, 0);

							/* record this node */	
							verilog_netlist->top_output_nodes = realloc(verilog_netlist->top_output_nodes, sizeof(nnode_t*)*(verilog_netlist->num_top_output_nodes+1));
							verilog_netlist->top_output_nodes[verilog_netlist->num_top_output_nodes] = new_node;
							verilog_netlist->num_top_output_nodes++;
						}	
						else if (var_declare->children[3] == NULL)
						{
							ast_node_t *node_max = resolve_node(instance_name_prefix, var_declare->children[1]);
							ast_node_t *node_min = resolve_node(instance_name_prefix, var_declare->children[2]);
							oassert(node_min->type == NUMBERS && node_max->type == NUMBERS);
							/* assume digit 1 is largest */
							for (k = node_min->types.number.value; k <= node_max->types.number.value; k++)
							{
								/* get the name of the pin */
								full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, var_declare->children[0]->types.identifier, k);
		
								/* check if the instantiation pin exists. */
								if ((sc_spot = sc_lookup_string(output_nets_sc, full_name)) == -1)
								{
									error_message(NETLIST_ERROR, var_declare->children[0]->line_number, var_declare->children[0]->file_number,
											"Output pin (%s) is not hooked up!!!\n", full_name);
								}
								new_pin = allocate_npin();
								/* hookup this pin to this net */
								add_fanout_pin_to_net((nnet_t*)output_nets_sc->data[sc_spot], new_pin);
		
								/* create the node */
								new_node = allocate_nnode();
								new_node->related_ast_node = module_items->children[i]->children[j];
								new_node->name = full_name;
								new_node->type = OUTPUT_NODE;
								/* allocate the pins needed */
								allocate_more_input_pins(new_node, 1);
								add_input_port_information(new_node, 1);
		
								/* hookup the pin, and node */
								add_input_pin_to_node(new_node, new_pin, 0);

								/* record this node */	
								verilog_netlist->top_output_nodes = realloc(verilog_netlist->top_output_nodes, sizeof(nnode_t*)*(verilog_netlist->num_top_output_nodes+1));
								verilog_netlist->top_output_nodes[verilog_netlist->num_top_output_nodes] = new_node;
								verilog_netlist->num_top_output_nodes++;
							}
						}
						else
						{
							/* Implicit memory */
							error_message(NETLIST_ERROR, var_declare->children[0]->line_number, var_declare->children[0]->file_number,
									"Memory not handled ... yet in create_top_output_nodes!\n");
						}
					}
				}
			}
		}
	}
	else
	{
		error_message(NETLIST_ERROR, module_items->line_number, module_items->file_number, "Empty module\n");
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: define_nets_with_driver)
 * Given a register declaration, this function defines it as a driver.
 *-------------------------------------------------------------------------------------------*/
nnet_t* define_nets_with_driver(ast_node_t* var_declare, char *instance_name_prefix)
{
	int i;
	char *temp_string;
	long sc_spot;
	nnet_t *new_net = NULL;
			
	if (var_declare->children[1] == NULL)
	{
		/* FOR single driver signal since spots 1, 2, 3, 4 are NULL */

		/* create the net */
		new_net = allocate_nnet();

		/* make the string to add to the string cache */
		temp_string = make_full_ref_name(instance_name_prefix, NULL, NULL, var_declare->children[0]->types.identifier, -1);

		/* look for that element */
		sc_spot = sc_add_string(output_nets_sc, temp_string);
		if (output_nets_sc->data[sc_spot] != NULL)
		{
			error_message(NETLIST_ERROR, var_declare->children[0]->line_number, var_declare->children[0]->file_number,
					"Net (%s) with the same name already created\n", temp_string);
		}
		/* store the data which is an idx here */
		output_nets_sc->data[sc_spot] = (void*)new_net;
		new_net->name = temp_string;
		
		/* Check if this net should have an initial value */
		if(var_declare->types.variable.is_initialized){
			new_net->has_initial_value = TRUE;
			/* Initial net value should only be either 1 or 0 */
			new_net->initial_value = var_declare->types.variable.initial_value ? 1 : 0;
		}
	}	
	else if (var_declare->children[3] == NULL)
	{
		ast_node_t *node_max = resolve_node(instance_name_prefix, var_declare->children[1]);
		ast_node_t *node_min = resolve_node(instance_name_prefix, var_declare->children[2]);

		/* FOR array driver  since sport 3 and 4 are NULL */
		oassert((node_max->type == NUMBERS) && (node_min->type == NUMBERS));
		
		/* Check if this array driver should have an initial value */
		long long initial_value = 0;
		if(var_declare->types.variable.is_initialized){
			initial_value = var_declare->types.variable.initial_value;
		}
		
		
		/* This register declaration is a range as opposed to a single bit so we need to define each element */
		/* assume digit 1 is largest */
		for (i = node_min->types.number.value; i <= node_max->types.number.value; i++)
		{
			/* create the net */
			new_net = allocate_nnet();

			/* create the string to add to the cache */
			temp_string = make_full_ref_name(instance_name_prefix, NULL, NULL, var_declare->children[0]->types.identifier, i);

			sc_spot = sc_add_string(output_nets_sc, temp_string);
			if (output_nets_sc->data[sc_spot] != NULL)
			{
				error_message(NETLIST_ERROR, var_declare->children[0]->line_number, var_declare->children[0]->file_number,
						"Net (%s) with the same name already created\n", temp_string);
			}		
			/* store the data which is an idx here */
			output_nets_sc->data[sc_spot] = (void*)new_net;
			new_net->name = temp_string;
			
			/* Assign initial value to this net if it exists */
			if(var_declare->types.variable.is_initialized){
				new_net->has_initial_value = TRUE;
				/* Grab LSB */
				new_net->initial_value = initial_value & 0x01;
				/* Shift out lowest bit */
				initial_value >>= 1;
			}
		}
	}
	/* Implicit memory */
	else if (var_declare->children[3] != NULL)
	{
		ast_node_t *node_max1 = resolve_node(instance_name_prefix, var_declare->children[1]);
		ast_node_t *node_min1 = resolve_node(instance_name_prefix, var_declare->children[2]);

		ast_node_t *node_max2 = resolve_node(instance_name_prefix, var_declare->children[3]);
		ast_node_t *node_min2 = resolve_node(instance_name_prefix, var_declare->children[4]);

		oassert((node_max1->type == NUMBERS) && (node_min1->type == NUMBERS));
		oassert((node_max2->type == NUMBERS) && (node_min2->type == NUMBERS));

		char *name = var_declare->children[0]->types.identifier;

		long long data_min = node_min1->types.number.value;
		long long data_max = node_max1->types.number.value;

		if (data_min != 0)
			error_message(NETLIST_ERROR, var_declare->children[0]->line_number, var_declare->children[0]->file_number,
					"%s: right memory index must be zero\n", name);

		oassert(data_min <= data_max);

		long long addr_min = node_min2->types.number.value;
		long long addr_max = node_max2->types.number.value;

		if (addr_min != 0)
			error_message(NETLIST_ERROR, var_declare->children[0]->line_number, var_declare->children[0]->file_number,
					"%s: right memory address index must be zero\n", name);

		oassert(addr_min <= addr_max);

		int data_width = data_max - data_min + 1;
		long long words = addr_max - addr_min + 1;

		create_implicit_memory_block(data_width, words, name, instance_name_prefix);
	}

	return new_net;
}

/*---------------------------------------------------------------------------------------------
 * (function:  define_nodes_and_nets_with_driver)
 * 	Similar to define_nets_with_driver except this one is for top level nodes and
 * 	is making the input pins into nodes and drivers.
 *-------------------------------------------------------------------------------------------*/
nnet_t* define_nodes_and_nets_with_driver(ast_node_t* var_declare, char *instance_name_prefix)
{
	int i;
	char *temp_string;
	long sc_spot;
	nnet_t *new_net = NULL;
	npin_t *new_pin;
	nnode_t *new_node;
				
	if (var_declare->children[1] == NULL)
	{
		/* FOR single driver signal since spots 1, 2, 3, 4 are NULL */

		/* create the net */
		new_net = allocate_nnet();

		temp_string = make_full_ref_name(instance_name_prefix, NULL, NULL, var_declare->children[0]->types.identifier, -1);

		sc_spot = sc_add_string(output_nets_sc, temp_string);
		if (output_nets_sc->data[sc_spot] != NULL)
		{
			error_message(NETLIST_ERROR, var_declare->children[0]->line_number, var_declare->children[0]->file_number,
					"Net (%s) with the same name already created\n", temp_string);
		}
		/* store the data which is an idx here */
		output_nets_sc->data[sc_spot] = (void*)new_net;
		new_net->name = temp_string;

		/* create this node and make the pin connection to the net */
		new_pin = allocate_npin();
		new_node = allocate_nnode();
		new_node->name = temp_string;

		new_node->related_ast_node = var_declare;
		new_node->type = INPUT_NODE;
		/* allocate the pins needed */
		allocate_more_output_pins(new_node, 1);
		add_output_port_information(new_node, 1);

		/* hookup the pin, net, and node */
		add_output_pin_to_node(new_node, new_pin, 0);
		add_driver_pin_to_net(new_net, new_pin);

		/* store it in the list of input nodes */
		verilog_netlist->top_input_nodes = (nnode_t**)realloc(verilog_netlist->top_input_nodes, sizeof(nnode_t*)*(verilog_netlist->num_top_input_nodes+1));
		verilog_netlist->top_input_nodes[verilog_netlist->num_top_input_nodes] = new_node;
		verilog_netlist->num_top_input_nodes++;
	}	
	else if (var_declare->children[3] == NULL)
	{
		/* FOR array driver  since sport 3 and 4 are NULL */
		ast_node_t *node_max = resolve_node(instance_name_prefix, var_declare->children[1]);
		ast_node_t *node_min = resolve_node(instance_name_prefix, var_declare->children[2]);
		oassert((node_max->type == NUMBERS) && (node_min->type == NUMBERS)) ;
		
		/* assume digit 1 is largest */
		for (i = node_min->types.number.value; i <= node_max->types.number.value; i++)
		{
			/* create the net */
			new_net = allocate_nnet();

			/* FOR single driver signal */
			temp_string = make_full_ref_name(instance_name_prefix, NULL, NULL, var_declare->children[0]->types.identifier, i);

			sc_spot = sc_add_string(output_nets_sc, temp_string);
			if (output_nets_sc->data[sc_spot] != NULL)
			{
				error_message(NETLIST_ERROR, var_declare->children[0]->line_number, var_declare->children[0]->file_number,
						"Net (%s) with the same name already created\n", temp_string);
			}		
			/* store the data which is an idx here */
			output_nets_sc->data[sc_spot] = (void*)new_net;
			new_net->name = temp_string;

			/* create this node and make the pin connection to the net */
			new_pin = allocate_npin();
			new_node = allocate_nnode();
	
			new_node->related_ast_node = var_declare;
			new_node->name = temp_string;
			new_node->type = INPUT_NODE;
			/* allocate the pins needed */
			allocate_more_output_pins(new_node, 1);
			add_output_port_information(new_node, 1);
	
			/* hookup the pin, net, and node */
			add_output_pin_to_node(new_node, new_pin, 0);
			add_driver_pin_to_net(new_net, new_pin);
	
			/* store it in the list of input nodes */
			verilog_netlist->top_input_nodes = (nnode_t**)realloc(verilog_netlist->top_input_nodes, sizeof(nnode_t*)*(verilog_netlist->num_top_input_nodes+1));
			verilog_netlist->top_input_nodes[verilog_netlist->num_top_input_nodes] = new_node;
			verilog_netlist->num_top_input_nodes++;
		}
	}
	else if (var_declare->children[3] != NULL)
	{
		/* Implicit memory */
		printf("Unhandled implicit memory in define_nodes_and_nets_with_driver\n");
		oassert(FALSE);
	}

	return new_net;
}

/*---------------------------------------------------------------------------------------------
 * (function: create_symbol_table_for_module)
 * 	Creates a lookup of the variables declared here so that in the analysis we can look
 * 	up the definition of it to decide what to do.
 *-------------------------------------------------------------------------------------------*/
void create_symbol_table_for_module(ast_node_t* module_items, char *module_name)
{
	/* with the top module we need to visit the entire ast tree */
	int i, j;
	char *temp_string;
	long sc_spot;
	oassert(module_items->type == MODULE_ITEMS);

	/* search for VAR_DECLARE_LISTS */
	if (module_items->num_children > 0)
	{
		for (i = 0; i < module_items->num_children; i++)
		{
			if (module_items->children[i]->type == VAR_DECLARE_LIST)
			{
				/* go through the vars in this declare list */
				for (j = 0; j < module_items->children[i]->num_children; j++)
				{	
					ast_node_t *var_declare = module_items->children[i]->children[j];

					/* parameters are already dealt with */
					if (var_declare->types.variable.is_parameter)
						continue;

					oassert(module_items->children[i]->children[j]->type == VAR_DECLARE);
					oassert(	(var_declare->types.variable.is_input) ||
						(var_declare->types.variable.is_output) ||
						(var_declare->types.variable.is_reg) ||
						(var_declare->types.variable.is_wire));

					/* make the string to add to the string cache */
					temp_string = make_full_ref_name(NULL, NULL, NULL, var_declare->children[0]->types.identifier, -1);
					/* look for that element */
					sc_spot = sc_add_string(local_symbol_table_sc, temp_string);
					if (local_symbol_table_sc->data[sc_spot] != NULL)
					{
						/* ERROR checks here 
						 * output with reg is fine
						 * output with wire is fine 
						 * Then update the stored string chache entry with information */
						if ((var_declare->types.variable.is_input)
								&& (
									   (((ast_node_t*)local_symbol_table_sc->data[sc_spot])->types.variable.is_reg)
									|| (((ast_node_t*)local_symbol_table_sc->data[sc_spot])->types.variable.is_wire)
								)
						)
						{
							error_message(NETLIST_ERROR, var_declare->line_number, var_declare->file_number,
									"Input defined as wire or reg means it is a driver in this module.  Not possible\n");
						}
						/* MORE ERRORS ... could check for same declaration name ... */
						else if (var_declare->types.variable.is_output)
						{
							/* copy all the reg and wire info over */
							((ast_node_t*)local_symbol_table_sc->data[sc_spot])->types.variable.is_output = TRUE;
							
							/* check for an initial value and copy it over if found */
							long long initial_value;
							if(check_for_initial_reg_value(var_declare, &initial_value)){
								((ast_node_t*)local_symbol_table_sc->data[sc_spot])->types.variable.is_initialized = TRUE;
								((ast_node_t*)local_symbol_table_sc->data[sc_spot])->types.variable.initial_value = initial_value;
							}
						}
						else if ((var_declare->types.variable.is_reg) || (var_declare->types.variable.is_wire))
						{
							/* copy the output status over */
							((ast_node_t*)local_symbol_table_sc->data[sc_spot])->types.variable.is_wire = var_declare->types.variable.is_wire;
							((ast_node_t*)local_symbol_table_sc->data[sc_spot])->types.variable.is_reg = var_declare->types.variable.is_reg;
							
							/* check for an initial value and copy it over if found */
							long long initial_value;
							if(check_for_initial_reg_value(var_declare, &initial_value)){
								((ast_node_t*)local_symbol_table_sc->data[sc_spot])->types.variable.is_initialized = TRUE;
								((ast_node_t*)local_symbol_table_sc->data[sc_spot])->types.variable.initial_value = initial_value;
							}
						}
						else
						{
							abort();
						}
					}
					else
					{
						/* store the data which is an idx here */
						local_symbol_table_sc->data[sc_spot] = (void *)var_declare;

						/* store the symbol */		
						local_symbol_table = (ast_node_t **)realloc(local_symbol_table, sizeof(ast_node_t*)*(num_local_symbol_table+1));
						local_symbol_table[num_local_symbol_table] = (void *)var_declare;
						num_local_symbol_table ++;
						
						/* check for an initial value and store it if found */
						long long initial_value;
						if(check_for_initial_reg_value(var_declare, &initial_value)){
							var_declare->types.variable.is_initialized = TRUE;
							var_declare->types.variable.initial_value = initial_value;
						}
					}
					free(temp_string);
				}
			}
		}
	}
	else
	{
		error_message(NETLIST_ERROR, module_items->line_number, module_items->file_number, "Empty module\n");
	}
}

/*--------------------------------------------------------------------------
 * (function: check_for_initial_reg_value)
 * 	This function looks at a VAR_DECLARE AST node and checks to see
 *  if the register declaration includes an initial value.
 *  Returns the initial value in *value if one is found.
 *  Added by Conor
 *-------------------------------------------------------------------------*/
int check_for_initial_reg_value(ast_node_t* var_declare, long long *value){
	oassert(var_declare->type == VAR_DECLARE);
	// Initial value is always the last child, if one exists
	if(var_declare->children[5] != NULL){
		*value = var_declare->children[5]->types.number.value;
		return TRUE;
	}
	return FALSE;
}

#ifdef VPR6
/*--------------------------------------------------------------------------
 * (function: connect_memory_and_alias)
 * 	This function looks at the memory instantiation points and checks
 * 		if there are any inputs that don't have a driver at this level.
 *  	If they don't then name needs to be aliased for higher levels to
 *  	understand.  If there exists a driver, then we connect the two.
 *
 * 	Assume that ports are written in the same order as the instantiation.
 *-------------------------------------------------------------------------*/
void connect_memory_and_alias(ast_node_t* hb_instance, char *instance_name_prefix)
{
	ast_node_t *hb_connect_list;
	int i, j;
	long sc_spot_output;
	long sc_spot_input_new, sc_spot_input_old;

	/* Note: Hard Block port matching is checked in earlier node processing */
	hb_connect_list = hb_instance->children[1]->children[1];
	for (i = 0; i < hb_connect_list->num_children; i++)
	{
		int port_size;

		ast_node_t *hb_var_node = hb_connect_list->children[i]->children[0];
		ast_node_t *hb_instance_var_node = hb_connect_list->children[i]->children[1];
		char *ip_name = hb_var_node->types.identifier;



		/* We can ignore inputs since they are already resolved */
		enum PORTS port_dir = (!strcmp(ip_name, "out") || !strcmp(ip_name, "out1") || !strcmp(ip_name, "out2"))
				?OUT_PORT:IN_PORT;
		if ((port_dir == OUT_PORT) || (port_dir == INOUT_PORT))
		{
			char *name_of_hb_input;
			char *full_name;
			char *alias_name;

			name_of_hb_input = get_name_of_pin_at_bit(hb_instance_var_node, -1, instance_name_prefix);
			full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_hb_input, -1);
			free(name_of_hb_input);
			alias_name = make_full_ref_name(instance_name_prefix,
					hb_instance->children[0]->types.identifier,
					hb_instance->children[1]->children[0]->types.identifier,
					hb_connect_list->children[i]->children[0]->types.identifier, -1);

			/* Lookup port size in cache */
			port_size = get_memory_port_size(alias_name);
			free(alias_name);
			oassert(port_size != 0);

			for (j = 0; j < port_size; j++)
			{
				/* This is an output pin from the hard block. We need to
				 * alias this output pin with it's calling name here so that
				 * everyone can see it at this level.
				 *
				 * Make the new string for the alias name */
				if (port_size > 1)
				{
					name_of_hb_input = get_name_of_pin_at_bit(hb_instance_var_node, j, instance_name_prefix);
					full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_hb_input, -1);
					free(name_of_hb_input);

					alias_name = make_full_ref_name(instance_name_prefix,
							hb_instance->children[0]->types.identifier,
							hb_instance->children[1]->children[0]->types.identifier,
							hb_connect_list->children[i]->children[0]->types.identifier, j);
				}
				else
				{
					oassert(j == 0);
					name_of_hb_input = get_name_of_pin_at_bit(hb_instance_var_node, -1, instance_name_prefix);
					full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_hb_input, -1);
					free(name_of_hb_input);

					alias_name = make_full_ref_name(instance_name_prefix,
							hb_instance->children[0]->types.identifier,
							hb_instance->children[1]->children[0]->types.identifier,
							hb_connect_list->children[i]->children[0]->types.identifier, -1);
				}

				/* Search for the old_input name */
				sc_spot_input_old = sc_lookup_string(input_nets_sc, alias_name);

				/* check if the instantiation pin exists */
				if ((sc_spot_output = sc_lookup_string(output_nets_sc, full_name)) == -1)
				{
					/* IF - no driver, then assume that it needs to be 
					   aliased to move up as an input */
					if ((sc_spot_input_new = sc_lookup_string(input_nets_sc, full_name)) == -1)
					{
						/* if this input is not yet used in this module
						   then we'll add it */
						sc_spot_input_new = sc_add_string(input_nets_sc, full_name);
						/* copy the pin to the old spot */
						input_nets_sc->data[sc_spot_input_new] = input_nets_sc->data[sc_spot_input_old];
					}
					else
					{
						/* already exists so we'll join the nets */
						combine_nets((nnet_t*)input_nets_sc->data[sc_spot_input_old], (nnet_t*)input_nets_sc->data[sc_spot_input_new], verilog_netlist);
					}
				}
				else
				{
					/* ELSE - we've found a matching net, 
					   so add this pin to the net */
					nnet_t* net = (nnet_t*)output_nets_sc->data[sc_spot_output];
					nnet_t* in_net = (nnet_t*)input_nets_sc->data[sc_spot_input_old];

					/* if they haven't been combined already,
					   then join the inputs and output */
					in_net->name = net->name;
					combine_nets(net, in_net, verilog_netlist);

					/* since the driver net is deleted,
					   copy the spot of the in_net over */
					input_nets_sc->data[sc_spot_input_old] = (void*)in_net;
					output_nets_sc->data[sc_spot_output] = (void*)in_net;

					/* if this input is not yet used in this module
					   then we'll add it */
					sc_spot_input_new = sc_add_string(input_nets_sc, full_name);
					/* copy the pin to the old spot */
					input_nets_sc->data[sc_spot_input_new] = (void *)in_net;
				}

				free(full_name);
				free(alias_name);
			}
		}
	}

	return;
}

/*--------------------------------------------------------------------------
 * (function: connect_hard_block_and_alias)
 * 	This function looks at the hard block instantiation points and checks
 * 		if there are any inputs that don't have a driver at this level.
 *  	If they don't then name needs to be aliased for higher levels to
 *  	understand.  If there exists a driver, then we connect the two.
 *
 * 	Assume that ports are written in the same order as the instantiation.
 * 	Also, we will assume that the portwidths have to match
 *-------------------------------------------------------------------------*/
void connect_hard_block_and_alias(ast_node_t* hb_instance, char *instance_name_prefix, int outport_size)
{
	t_model *hb_model;
	ast_node_t *hb_connect_list;
	int i, j;
	long sc_spot_output;
	long sc_spot_input_new, sc_spot_input_old;

	/* See if the hard block declared is supported by FPGA architecture */
	char *identifier = hb_instance->children[0]->types.identifier;
	hb_model = find_hard_block(identifier);
	if (!hb_model)
	{
		error_message(NETLIST_ERROR, hb_instance->line_number, hb_instance->file_number, "Found Hard Block \"%s\": Not supported by FPGA Architecture\n", identifier);
	}

	/* Note: Hard Block port matching is checked in earlier node processing */
	hb_connect_list = hb_instance->children[1]->children[1];
	for (i = 0; i < hb_connect_list->num_children; i++)
	{
		int port_size;
		int flag = 0;
		enum PORTS port_dir;
		ast_node_t *hb_var_node = hb_connect_list->children[i]->children[0];
		ast_node_t *hb_instance_var_node = hb_connect_list->children[i]->children[1];

		/* We can ignore inputs since they are already resolved */
		port_dir = hard_block_port_direction(hb_model, hb_var_node->types.identifier);
		if ((port_dir == OUT_PORT) || (port_dir == INOUT_PORT))
		{
			port_size = hard_block_port_size(hb_model, hb_var_node->types.identifier);
			oassert(port_size != 0);
			port_size = outport_size;

			if(strcmp(hb_model->name, "adder") == 0)
			{
				if(strcmp(hb_var_node->types.identifier, "cout") == 0 && flag == 0)
					port_size = 1;
				else if(strcmp(hb_var_node->types.identifier, "sumout") == 0 && flag == 0)
					port_size = port_size - 1;
			}

			for (j = 0; j < port_size; j++)
			{
				/* This is an output pin from the hard block. We need to
				 * alias this output pin with it's calling name here so that
				 * everyone can see it at this level
				 */
				char *name_of_hb_input;
				char *full_name;
				char *alias_name;

				/* make the new string for the alias name */
				if (port_size > 1)
				{
					name_of_hb_input = get_name_of_pin_at_bit(hb_instance_var_node, j, instance_name_prefix);
					full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_hb_input, -1);
					free(name_of_hb_input);

					alias_name = make_full_ref_name(instance_name_prefix,
							hb_instance->children[0]->types.identifier,
							hb_instance->children[1]->children[0]->types.identifier,
							hb_connect_list->children[i]->children[0]->types.identifier, j);
				}
				else
				{
					oassert(j == 0);
					name_of_hb_input = get_name_of_pin_at_bit(hb_instance_var_node, -1, instance_name_prefix);
					full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_hb_input, -1);
					free(name_of_hb_input);

					alias_name = make_full_ref_name(instance_name_prefix,
							hb_instance->children[0]->types.identifier,
							hb_instance->children[1]->children[0]->types.identifier,
							hb_connect_list->children[i]->children[0]->types.identifier, -1);
				}

				/* Search for the old_input name */
				sc_spot_input_old = sc_lookup_string(input_nets_sc, alias_name);

				/* check if the instantiation pin exists */
				if ((sc_spot_output = sc_lookup_string(output_nets_sc, full_name)) == -1)
				{
					/* IF - no driver, then assume that it needs to be 
					   aliased to move up as an input */
					if ((sc_spot_input_new = sc_lookup_string(input_nets_sc, full_name)) == -1)
					{
						/* if this input is not yet used in this module
						   then we'll add it */
						sc_spot_input_new = sc_add_string(input_nets_sc, full_name);
						/* copy the pin to the old spot */
						input_nets_sc->data[sc_spot_input_new] = input_nets_sc->data[sc_spot_input_old];
					}
					else
					{
						/* already exists so we'll join the nets */
						combine_nets((nnet_t*)input_nets_sc->data[sc_spot_input_old], (nnet_t*)input_nets_sc->data[sc_spot_input_new], verilog_netlist);
					}
				}
				else
				{
					/* ELSE - we've found a matching net, 
					   so add this pin to the net */
					nnet_t* net = (nnet_t*)output_nets_sc->data[sc_spot_output];
					nnet_t* in_net = (nnet_t*)input_nets_sc->data[sc_spot_input_old];


					/* if they haven't been combined already,
						then join the inputs and output */
					if(in_net != NULL)
					{
						in_net->name = net->name;
						combine_nets(net, in_net, verilog_netlist);

                                        
						/* since the driver net is deleted,
							copy the spot of the in_net over */
						input_nets_sc->data[sc_spot_input_old] = (void*)in_net;
						output_nets_sc->data[sc_spot_output] = (void*)in_net;

						/* if this input is not yet used in this module
							then we'll add it */
						sc_spot_input_new = sc_add_string(input_nets_sc, full_name);
						/* copy the pin to the old spot */
						input_nets_sc->data[sc_spot_input_new] = (void *)in_net;
					}
				}

				free(full_name);
				free(alias_name);
			}
		}
	}

	return;
}
#endif

/*--------------------------------------------------------------------------
 * (function: connect_module_instantiation_and_alias)
 * 	This function looks at module instantiation points and does one of two things:
 * 		On the first pass this function looks at an instantiation (which has already
 * 		been processed) and checks if there are drivers in there that need to be seen
 * 		as we process the instantiating module.  
 *
 * 		On the second pass this function looks at an instantiation and checks if
 * 		there are any inputs that don't have a driver at this level.  If they don't then
 * 		name needs to be aliased for higher levels to understand.  If there exists a 
 * 		driver, then we connect the two.
 *
 * 	Assume that ports are written in the same order as the instantiation.  Also,
 * 	we will assume that the portwidths have to match
 *-------------------------------------------------------------------------*/
void connect_module_instantiation_and_alias(short PASS, ast_node_t* module_instance, char *instance_name_prefix)
{
	int i, j;
	ast_node_t *module_node;
	ast_node_t *module_list;
	ast_node_t *module_instance_list = module_instance->children[1]->children[1]; // MODULE_INSTANCE->MODULE_INSTANCE_NAME(child[1])->MODULE_CONNECT_LIST(child[1])
	long sc_spot;
	long sc_spot_output;
	long sc_spot_input_old;
	long sc_spot_input_new;

	char *module_instance_name = module_instance->children[0]->types.identifier;
	
	/* lookup the node of the module associated with this instantiated module */
	if ((sc_spot = sc_lookup_string(module_names_to_idx, module_instance_name)) == -1)
	{
		error_message(NETLIST_ERROR, module_instance->line_number, module_instance->file_number,
				"Can't find module %s\n", module_instance_name);
	}
	
	if (module_instance_name != module_instance->children[0]->types.identifier)
		free(module_instance_name);

	module_node = (ast_node_t*)module_names_to_idx->data[sc_spot];
	module_list = module_node->children[1]; // MODULE->VAR_DECLARE_LIST(child[1])

	if (module_list->num_children != module_instance_list->num_children)
	{
		error_message(NETLIST_ERROR, module_instance->line_number, module_instance->file_number,
				"Module instantiation (%s) and definition don't match in terms of ports\n",
				module_instance->children[0]->types.identifier);
	}

	for (i = 0; i < module_list->num_children; i++)
	{
		int port_size = 0;
		// VAR_DECLARE_LIST(child[i])->VAR_DECLARE_PORT(child[0])->VAR_DECLARE_input-or-output(child[0])
		ast_node_t *module_var_node = module_list->children[i]->children[0];
		// MODULE_CONNECT_LIST(child[i])->MODULE_CONNECT(child[1]) // child[0] is for aliasing
		ast_node_t *module_instance_var_node = module_instance_list->children[i]->children[1];
		
		if ( 
			   // skip inputs on pass 1
			   ((PASS == INSTANTIATE_DRIVERS) && (module_list->children[i]->children[0]->types.variable.is_input))
			   // skip outputs on pass 2
			|| ((PASS == ALIAS_INPUTS) && (module_list->children[i]->children[0]->types.variable.is_output))
		)
		{
			continue;
		}

		/* calculate the port details */
		if (module_var_node->children[1] == NULL)
		{
			port_size = 1;
		}
		else if (module_var_node->children[3] == NULL)
		{
			char *module_name = make_full_ref_name(instance_name_prefix, 
				// module_name
				module_instance->children[0]->types.identifier, 
				// instance name
				module_instance->children[1]->children[0]->types.identifier, 
				NULL, -1);
			ast_node_t *node1 = resolve_node(module_name, module_var_node->children[1]);
			ast_node_t *node2 = resolve_node(module_name, module_var_node->children[2]);
			free(module_name);
			oassert(node2->type == NUMBERS && node1->type == NUMBERS);
			/* assume all arrays declared [largest:smallest] */
			oassert(node2->types.number.value <= node1->types.number.value); 
			port_size = node1->types.number.value - node2->types.number.value + 1;
		}
		else if (module_var_node->children[3] != NULL)
		{
			/* Implicit memory */
			printf("Unhandled implicit memory in connect_module_instantiation_and_alias\n");
			oassert(FALSE);
		}
		for (j = 0; j < port_size; j++)
		{
			
			if (module_list->children[i]->children[0]->types.variable.is_input)
			{
				/* IF - this spot in the module list is an input, then we need to find it in the
				 * string cache (as its old name), check if the new_name (the instantiation name)
 				 * is already a driver at this level.  IF it is a driver then we can hook the two up.
 				 * IF it's not we need to alias the pin name as it is used here since it
 				 * must be driven at a higher level of hierarchy in the tree of modules */
				char *name_of_module_instance_of_input;
				char *full_name;
				char *alias_name;
				
				if (port_size > 1)
				{		
					/* Get the name of the module instantiation pin */
					name_of_module_instance_of_input = get_name_of_pin_at_bit(module_instance_var_node, j, instance_name_prefix);
					full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_module_instance_of_input, -1); 
					free(name_of_module_instance_of_input);

					/* make the new string for the alias name - has to be a identifier in the instantiated modules old names */
					name_of_module_instance_of_input = get_name_of_var_declare_at_bit(module_list->children[i]->children[0], j);
					alias_name = make_full_ref_name(instance_name_prefix,
							module_instance->children[0]->types.identifier,
							module_instance->children[1]->children[0]->types.identifier,
							name_of_module_instance_of_input, -1);

					free(name_of_module_instance_of_input);
				}
				else
				{
					oassert(j == 0);

					/* Get the name of the module instantiation pin */
					name_of_module_instance_of_input = get_name_of_pin_at_bit(module_instance_var_node, -1, instance_name_prefix);
					full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_module_instance_of_input, -1); 
					free(name_of_module_instance_of_input);

					name_of_module_instance_of_input = get_name_of_var_declare_at_bit(module_list->children[i]->children[0], 0);
					alias_name = make_full_ref_name(instance_name_prefix,
							module_instance->children[0]->types.identifier,
							module_instance->children[1]->children[0]->types.identifier,
							name_of_module_instance_of_input, -1);

					free(name_of_module_instance_of_input);
				}
				

				
				
				/* search for the old_input name */
				if ((sc_spot_input_old = sc_lookup_string(input_nets_sc, alias_name)) == -1)
                {
					/* doesn it have to exist since might only be used in module */
					error_message(NETLIST_ERROR, module_instance->line_number, module_instance->file_number,
							"This module port %s is unused in module %s", alias_name, module_node->children[0]->types.identifier);
				}
				
				/* CMM - Check if this pin should be driven by the top level VCC or GND drivers	*/
				if (strstr(full_name, "ONE_VCC_CNS") != NULL)
				{
					join_nets(verilog_netlist->one_net, (nnet_t*)input_nets_sc->data[sc_spot_input_old]);
					input_nets_sc->data[sc_spot_input_old] = (void*)verilog_netlist->one_net;
				}
				else if (strstr(full_name, "ZERO_GND_ZERO") != NULL)
				{
					join_nets(verilog_netlist->zero_net, (nnet_t*)input_nets_sc->data[sc_spot_input_old]);
					input_nets_sc->data[sc_spot_input_old] = (void*)verilog_netlist->zero_net;
				}
				/* check if the instantiation pin exists. */
				else if ((sc_spot_output = sc_lookup_string(output_nets_sc, full_name)) == -1)
				{
					/* IF - no driver, then assume that it needs to be aliased to move up as an input */
					if ((sc_spot_input_new = sc_lookup_string(input_nets_sc, full_name)) == -1)
					{
						/* if this input is not yet used in this module then we'll add it */
						sc_spot_input_new = sc_add_string(input_nets_sc, full_name);
						
						/* copy the pin to the old spot */
						input_nets_sc->data[sc_spot_input_new] = input_nets_sc->data[sc_spot_input_old];
					}
					else
					{
						/* already exists so we'll join the nets */
						combine_nets((nnet_t*)input_nets_sc->data[sc_spot_input_old], (nnet_t*)input_nets_sc->data[sc_spot_input_new], verilog_netlist);
					}
				}
				else
				{
					/* ELSE - we've found a matching net, so add this pin to the net */
					nnet_t* net = (nnet_t*)output_nets_sc->data[sc_spot_output];
					nnet_t* in_net = (nnet_t*)input_nets_sc->data[sc_spot_input_old];

					if ((net != in_net) && (net->combined == TRUE))
					{
						/* if they haven't been combined already, then join the inputs and output */
						join_nets(net, in_net);
						/* since the driver net is deleted, copy the spot of the in_net over */
						input_nets_sc->data[sc_spot_input_old] = (void*)net;
					}
					else if ((net != in_net) && (net->combined == FALSE))
					{
						/* if they haven't been combined already, then join the inputs and output */
						combine_nets(net, in_net, verilog_netlist);
						/* since the driver net is deleted, copy the spot of the in_net over */
						output_nets_sc->data[sc_spot_output] = (void*)in_net;
					}
				}

				/* IF the designer uses port names then make sure they line up */
				if (module_instance_list->children[i]->children[0] != NULL)
				{
					if (strcmp(module_instance_list->children[i]->children[0]->types.identifier,
							module_list->children[i]->children[0]->children[0]->types.identifier) != 0
					)
					{
						error_message(NETLIST_ERROR, module_list->children[i]->children[0]->line_number, module_list->children[i]->children[0]->file_number,
								"This module entry does not match up correctly (%s != %s).  Odin expects the order of ports to be the same\n",
								module_instance_list->children[i]->children[0]->types.identifier,
								module_list->children[i]->children[0]->children[0]->types.identifier
						);
					}
				}

				free(full_name);
				free(alias_name);
			}
			else if (module_list->children[i]->children[0]->types.variable.is_output)
			{
				/* ELSE IF - this is an output pin from the module.  We need to alias this output
				 * pin with it's calling name here so that everyone can see it at this level */
				char *name_of_module_instance_of_input;
				char *full_name;
				char *alias_name;
				
				/* make the new string for the alias name - has to be a identifier in the
				 * instantiated modules old names */
				if (port_size > 1)
				{		
					/* Get the name of the module instantiation pin */
					name_of_module_instance_of_input = get_name_of_pin_at_bit(module_instance_var_node, j, instance_name_prefix);
					full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_module_instance_of_input, -1); 
					free(name_of_module_instance_of_input);

					name_of_module_instance_of_input = get_name_of_var_declare_at_bit(module_list->children[i]->children[0], j);
					alias_name = make_full_ref_name(instance_name_prefix,
							module_instance->children[0]->types.identifier,
							module_instance->children[1]->children[0]->types.identifier, name_of_module_instance_of_input, -1);
					free(name_of_module_instance_of_input);
				}
				else
				{
					oassert(j == 0);
					/* Get the name of the module instantiation pin */
					name_of_module_instance_of_input = get_name_of_pin_at_bit(module_instance_var_node, -1, instance_name_prefix);
					full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_module_instance_of_input, -1); 
					free(name_of_module_instance_of_input);

					name_of_module_instance_of_input = get_name_of_var_declare_at_bit(module_list->children[i]->children[0], 0);
					alias_name = make_full_ref_name(instance_name_prefix,
							module_instance->children[0]->types.identifier,
							module_instance->children[1]->children[0]->types.identifier, name_of_module_instance_of_input, -1);
					free(name_of_module_instance_of_input);
				}
				

				/* check if the instantiation pin exists. */
				if ((sc_spot_output = sc_lookup_string(output_nets_sc, alias_name)) == -1)
				{
					error_message(NETLIST_ERROR, module_list->children[i]->children[0]->line_number, module_list->children[i]->children[0]->file_number,
							"This output (%s) must exist...must be an error\n", alias_name);
				}
				

				
				
				
				/* can already be there */
				sc_spot_input_new = sc_add_string(output_nets_sc, full_name);
				
				/* Copy over the initial value data from the net alias to the corresponding 
				   flip-flop node if one exists. This is necessary if an initial value is 
				   assigned on a higher-level module since the flip-flop node will have
				   already been instantiated without any initial value. */
				nnet_t *output_net = (nnet_t*)output_nets_sc->data[sc_spot_output];
				nnet_t *input_new_net = (nnet_t*)output_nets_sc->data[sc_spot_input_new];
				if(output_net->driver_pin && output_net->driver_pin->node){
					if(output_net->driver_pin->node->type == FF_NODE && input_new_net->has_initial_value){
						output_net->driver_pin->node->has_initial_value = input_new_net->has_initial_value;
						output_net->driver_pin->node->initial_value = input_new_net->initial_value;
					}
				}
				
				/* add this alias for the net */
				output_nets_sc->data[sc_spot_input_new] = output_nets_sc->data[sc_spot_output];

				/* IF the designer users port names then make sure they line up */
				if (module_instance_list->children[i]->children[0] != NULL)
				{
					if (strcmp(module_instance_list->children[i]->children[0]->types.identifier, module_list->children[i]->children[0]->children[0]->types.identifier) != 0)
					{
						error_message(NETLIST_ERROR, module_list->children[i]->children[0]->line_number, module_list->children[i]->children[0]->file_number,
								"This module entry does not match up correctly (%s != %s).  Odin expects the order of ports to be the same\n",
								module_instance_list->children[i]->children[0]->types.identifier,
								module_list->children[i]->children[0]->children[0]->types.identifier
						);
					}
				}

				free(full_name);
				free(alias_name);
			}
		}	
	}
}



/*---------------------------------------------------------------------------------------------
 * (function: create_pins)
 * 	Create pin creates the pins representing this naming isntance, adds them to the input
 * 	nets list (if not already there), checks if a driver already exists (and hooks input
 * 	to output if needed), and adds the pin to the list.
 * 	Note: only for input paths...
 *-------------------------------------------------------------------------------------------*/
signal_list_t *create_pins(ast_node_t* var_declare, char *name, char *instance_name_prefix)
{
	int i;
	signal_list_t *return_sig_list = init_signal_list();
	long sc_spot;
	long sc_spot_output;
	npin_t *new_pin;
	nnet_t *new_in_net;
	char_list_t *pin_lists = NULL;

	if (name == NULL)
	{
		/* get all the pins */
		pin_lists = get_name_of_pins_with_prefix(var_declare, instance_name_prefix);
	}
	else if (var_declare == NULL)
	{
		/* if you have the name and just want a pin then use this method */
		pin_lists = (char_list_t*)malloc(sizeof(char_list_t)*1);
		pin_lists->strings = (char**)malloc(sizeof(char*));
		pin_lists->strings[0] = name;
		pin_lists->num_strings = 1;
	}
	else
	{
		error_message(0,0,-1,"Invalid state or internal error");
	}


	for (i = 0; i < pin_lists->num_strings; i++)
	{
		new_pin = allocate_npin();
		new_pin->name = pin_lists->strings[i];

		/* check if the instantiation pin exists. */
		if (strstr(pin_lists->strings[i], "ONE_VCC_CNS") != NULL)
		{
			add_fanout_pin_to_net(verilog_netlist->one_net, new_pin);
			sc_spot = sc_add_string(input_nets_sc, pin_lists->strings[i]);
			input_nets_sc->data[sc_spot] = (void*)verilog_netlist->one_net;
		}
		else if (strstr(pin_lists->strings[i], "ZERO_GND_ZERO") != NULL)
		{
			add_fanout_pin_to_net(verilog_netlist->zero_net, new_pin);
			sc_spot = sc_add_string(input_nets_sc, pin_lists->strings[i]);
			input_nets_sc->data[sc_spot] = (void*)verilog_netlist->zero_net;
		}
		else
		{
			/* search for the input name  if already exists...needs to be added to
			 * string cache in case it's an input pin */
			if ((sc_spot = sc_lookup_string(input_nets_sc, pin_lists->strings[i])) == -1)
			{	
				new_in_net = allocate_nnet();
	
				sc_spot = sc_add_string(input_nets_sc, pin_lists->strings[i]);
				input_nets_sc->data[sc_spot] = (void*)new_in_net;
			}
	
			/* store the pin in this net */
			add_fanout_pin_to_net((nnet_t*)input_nets_sc->data[sc_spot], new_pin);

			if ((sc_spot_output = sc_lookup_string(output_nets_sc, pin_lists->strings[i])) != -1)
			{
				/* ELSE - we've found a matching net, so add this pin to the net */
				nnet_t* net = (nnet_t*)output_nets_sc->data[sc_spot_output];

				if ((net != (nnet_t*)input_nets_sc->data[sc_spot]) && net->combined )
				{
					/* IF - the input and output nets don't match, then they need to be joined */
					join_nets(net, (nnet_t*)input_nets_sc->data[sc_spot]);
					/* since the driver net is deleted, copy the spot of the in_net over */
					input_nets_sc->data[sc_spot] = (void*)net;
				}
				else if ((net != (nnet_t*)input_nets_sc->data[sc_spot]) && !net->combined)
				{
					/* IF - the input and output nets don't match, then they need to be joined */

					combine_nets(net, (nnet_t*)input_nets_sc->data[sc_spot], verilog_netlist);
					/* since the driver net is deleted, copy the spot of the in_net over */
					output_nets_sc->data[sc_spot_output] = (void*)input_nets_sc->data[sc_spot];
				}
			}
		}

		/* add the pin now stored in the string chache to the returned signal list */		
		add_pin_to_signal_list(return_sig_list, new_pin);
	}
	
	return return_sig_list; 
}

/*---------------------------------------------------------------------------------------------
 * (function: create_output_pin)
 * 	Create OUTPUT pin creates a pin representing this naming isntance, adds it to the input
 * 	nets list (if not already there) and adds the pin to the list.
 * 	Note: only for output drivers...
 *-------------------------------------------------------------------------------------------*/
signal_list_t *create_output_pin(ast_node_t* var_declare, char *instance_name_prefix)
{
	char *name_of_pin;
	char *full_name;
	signal_list_t *return_sig_list = init_signal_list();
	npin_t *new_pin;
	
	/* get the name of the pin */
	name_of_pin = get_name_of_pin_at_bit(var_declare, -1, instance_name_prefix);
	full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_pin, -1); 
	free(name_of_pin);

	new_pin = allocate_npin();
	new_pin->name = full_name;

	add_pin_to_signal_list(return_sig_list, new_pin);

	return return_sig_list; 
}

/*---------------------------------------------------------------------------------------------
 * (function: assignment_alias)
 *-------------------------------------------------------------------------------------------*/
signal_list_t *assignment_alias(ast_node_t* assignment, char *instance_name_prefix)
{
	ast_node_t *left  = assignment->children[0];
	ast_node_t *right = assignment->children[1];

	implicit_memory *left_memory  = lookup_implicit_memory_reference_ast(instance_name_prefix, left);
	implicit_memory *right_memory = lookup_implicit_memory_reference_ast(instance_name_prefix, right);

	signal_list_t *in_1 = NULL;

	signal_list_t *right_outputs = NULL;
	signal_list_t *right_inputs = NULL;


	/* process the signal for the input gate */
	if (right_memory)
	{
		if (!is_valid_implicit_memory_reference_ast(instance_name_prefix, right))
			error_message(NETLIST_ERROR, assignment->line_number, assignment->file_number,
							"Invalid addressing mode for implicit memory %s.\n", right_memory->name);

		signal_list_t* address = netlist_expand_ast_of_module(right->children[1], instance_name_prefix);
		// Pad/shrink the address to the depth of the memory.
		{
			while(address->count < right_memory->addr_width)
				add_pin_to_signal_list(address, get_zero_pin(verilog_netlist));
			address->count = right_memory->addr_width;
		}

		add_input_port_to_implicit_memory(right_memory, address, "addr1");

		// Zero the write enable.
		signal_list_t *we = init_signal_list();
		add_pin_to_signal_list(we, get_zero_pin(verilog_netlist));
		add_input_port_to_implicit_memory(right_memory, we, "we1");

		// Zero the data input.
		signal_list_t *data = init_signal_list();
		int i;
		for (i = 0; i < right_memory->data_width; i++)
			add_pin_to_signal_list(data, get_zero_pin(verilog_netlist));
		add_input_port_to_implicit_memory(right_memory, data, "data1");


		// Right inputs are the inputs to the memory. This will contain the address only.
		right_inputs = init_signal_list();
		char *name = right->children[0]->types.identifier;
		for (i = 0; i < address->count; i++)
		{
			npin_t *pin = address->pins[i];
			pin->name = make_full_ref_name(instance_name_prefix, NULL, NULL, name, i);
			add_pin_to_signal_list(right_inputs, pin);
		}

		// Right outputs will be the outputs of the memory. They will be
		// treated the same as the outputs from the RHS of any assignment.
		right_outputs = init_signal_list();
		signal_list_t *outputs = init_signal_list();
		for (i = 0; i < right_memory->data_width; i++)
		{
			npin_t *pin = allocate_npin();
			add_pin_to_signal_list(outputs, pin);
			pin->name = make_full_ref_name("", NULL, NULL, right_memory->node->name, i);
			nnet_t *net = allocate_nnet();
			add_driver_pin_to_net(net, pin);
			pin = allocate_npin();
			add_fanout_pin_to_net(net, pin);
			//right_outputs->pins[i] = pin;
			add_pin_to_signal_list(right_outputs, pin);
		}
		add_output_port_to_implicit_memory(right_memory, outputs, "out1");
		free_signal_list(outputs);

		free_signal_list(address);
		free_signal_list(we);
		free_signal_list(data);
	}
	else
	{
		in_1 = netlist_expand_ast_of_module(right, instance_name_prefix);
		oassert(in_1 != NULL);
	}

	char_list_t *out_list;

	if (left_memory)
	{
		if (!is_valid_implicit_memory_reference_ast(instance_name_prefix, left))
			error_message(NETLIST_ERROR, assignment->line_number, assignment->file_number,
							"Invalid addressing mode for implicit memory %s.\n", left_memory->name);

		// A memory can only be written from a clocked sequential block.
		if (type_of_circuit != SEQUENTIAL)
		{
			out_list = NULL;
			error_message(NETLIST_ERROR, assignment->line_number, assignment->file_number,
				"Assignment to implicit memories is only supported within sequential circuits.\n");
		}
		else
		{
			// Make sure the memory is addressed.
			signal_list_t* address = netlist_expand_ast_of_module(left->children[1], instance_name_prefix);

			// Pad/shrink the address to the depth of the memory.
			{
				while(address->count < left_memory->addr_width)
					add_pin_to_signal_list(address, get_zero_pin(verilog_netlist));
				address->count = left_memory->addr_width;
			}

			add_input_port_to_implicit_memory(left_memory, address, "addr2");

			signal_list_t *data;
			if (right_memory)
				data = right_outputs;
			else
				data = in_1;

			// Pad/shrink the data to the width of the memory.
			{
				while(data->count < left_memory->data_width)
					add_pin_to_signal_list(data, get_zero_pin(verilog_netlist));
				data->count = left_memory->data_width;
			}

			add_input_port_to_implicit_memory(left_memory, data, "data2");

			signal_list_t *we = init_signal_list();
			add_pin_to_signal_list(we, get_one_pin(verilog_netlist));
			add_input_port_to_implicit_memory(left_memory, we, "we2");

			in_1 = init_signal_list();
			char *name = left->children[0]->types.identifier;
			int i;
			int pin_index = left_memory->data_width + left_memory->data_width + left_memory->addr_width + 2;
			for (i = 0; i < address->count; i++)
			{
				npin_t *pin = address->pins[i];
				pin->name = make_full_ref_name(instance_name_prefix, NULL, NULL, name, pin_index++);
				add_pin_to_signal_list(in_1, pin);
			}
			free_signal_list(address);

			for (i = 0; i < data->count; i++)
			{
				npin_t *pin = data->pins[i];
				pin->name = make_full_ref_name(instance_name_prefix, NULL, NULL, name, pin_index++);
				add_pin_to_signal_list(in_1, pin);
			}
			free_signal_list(data);

			for (i = 0; i < we->count; i++)
			{
				npin_t *pin = we->pins[i];
				pin->name = make_full_ref_name(instance_name_prefix, NULL, NULL, name, pin_index++);
				add_pin_to_signal_list(in_1, pin);
			}
			free_signal_list(we);

			out_list = NULL;
		}
	}
	else
	{
		out_list = get_name_of_pins_with_prefix(left, instance_name_prefix);
	}




	signal_list_t *return_list;
	return_list = init_signal_list();

	if (!right_memory && !left_memory)
	{
		int output_size = alias_output_assign_pins_to_inputs(out_list, in_1, assignment);

		if (output_size < in_1->count)
		{
			/* need to shrink the output list */
			int i;
			for (i = 0; i < output_size; i++)
				add_pin_to_signal_list(return_list, in_1->pins[i]);

			free_signal_list(in_1);
		}
		else
		{
			return_list = in_1;
		}

		free(out_list->strings);
		free(out_list);
	}
	else
	{
		if (right_memory)
		{
			// Register inputs for later assignment directly to the memory.
			int i;
			for (i = 0; i < right_inputs->count; i++)
			{
				add_pin_to_signal_list(return_list, right_inputs->pins[i]);
				register_implicit_memory_input(right_inputs->pins[i]->name, right_memory);
			}
			free_signal_list(right_inputs);

			// Alias the outputs like a regular assignment if the thing on the left isn't a memory.
			if (!left_memory)
			{
				int output_size = alias_output_assign_pins_to_inputs(out_list, right_outputs, assignment);
				for (i = 0; i < output_size; i++)
				{
					npin_t *pin = right_outputs->pins[i];
					add_pin_to_signal_list(return_list, pin);
				}
				free_signal_list(right_outputs);
				free(out_list->strings);
				free(out_list);
			}


		}

		if (left_memory)
		{
			// Index all inputs to the left memory for implicit memory direct assignment.
			int i;
			for (i = 0; i < in_1->count; i++)
			{
				add_pin_to_signal_list(return_list, in_1->pins[i]);
				register_implicit_memory_input(in_1->pins[i]->name, left_memory);
			}
			free_signal_list(in_1);
		}
	}

	return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function: terminate_registered_assignment)
 *-------------------------------------------------------------------------------------------*/
void terminate_registered_assignment(ast_node_t *always_node, signal_list_t* assignment, signal_list_t *potential_clocks, char *instance_name_prefix)
{
	oassert(potential_clocks != NULL);

	/* figure out which one is the clock */
	if (local_clock_found == FALSE)
	{
		int i;
		for (i = 0; i < potential_clocks->count; i++)
		{
			nnet_t *temp_net;
			/* searching for the clock with no net */
			long sc_spot = sc_spot = sc_lookup_string(output_nets_sc, potential_clocks->pins[i]->name);
			if (sc_spot == -1)
			{
				sc_spot = sc_lookup_string(input_nets_sc, potential_clocks->pins[i]->name);
				if (sc_spot == -1)
				{
					error_message(NETLIST_ERROR, always_node->line_number, always_node->file_number,
							"Sensitivity list element (%s) is not a driver or net ... must be\n", potential_clocks->pins[i]->name);
				}
				temp_net = (nnet_t*)input_nets_sc->data[sc_spot];
			}
			else
			{
				temp_net = (nnet_t*)output_nets_sc->data[sc_spot];
			}
	
			
			if ((((temp_net->num_fanout_pins == 1) && (temp_net->fanout_pins[0]->node == NULL)) || (temp_net->num_fanout_pins == 0))
				&& (local_clock_found == TRUE))
			{
				error_message(NETLIST_ERROR, always_node->line_number, always_node->file_number,
						"Suspected second clock (%s).  In a sequential sensitivity list, Odin expects the "
						"clock not to drive anything and any other signals in this list to drive stuff.  "
						"For example, a reset in the sensitivy list has to be hooked up to something in the always block.\n",
						potential_clocks->pins[i]->name);
			}
			else if (temp_net->num_fanout_pins == 0)
			{
				/* If this element is in the sensitivity list and doesn't drive anything it's the clock */
				local_clock_found = TRUE;
				local_clock_idx = i;
			}
			else if ((temp_net->num_fanout_pins == 1) && (temp_net->fanout_pins[0]->node == NULL))
			{
				/* If this element is in the sensitivity list and doesn't drive anything it's the clock */
				local_clock_found = TRUE;
				local_clock_idx = i;
			}

		}
	}

	nnet_t *clock_net = potential_clocks->pins[local_clock_idx]->net;

	signal_list_t *memory_inputs = init_signal_list();
	int i;
	for (i = 0; i < assignment->count; i++)
	{
		npin_t *pin = assignment->pins[i];
		implicit_memory *memory = lookup_implicit_memory_input(pin->name);
		if (memory)
		{
			add_pin_to_signal_list(memory_inputs, pin);
		}
		else
		{
			/* look up the net */
			int sc_spot = sc_lookup_string(output_nets_sc, pin->name);
			if (sc_spot == -1)
			{
				error_message(NETLIST_ERROR, always_node->line_number, always_node->file_number,
						"Assignment is missing driver (%s)\n", pin->name);
			}
			nnet_t *net = output_nets_sc->data[sc_spot];

			/* HERE create the ff node and hookup everything */
			nnode_t *ff_node = allocate_nnode();
			ff_node->related_ast_node = always_node;
		
			ff_node->type = FF_NODE;
			/* create the unique name for this gate */
			ff_node->name = node_name(ff_node, instance_name_prefix);
			
			/* Copy over the initial value information from the net */
			ff_node->has_initial_value = net->has_initial_value;
			ff_node->initial_value = net->initial_value;
			
			
			/* allocate the pins needed */
			allocate_more_input_pins(ff_node, 2);
			add_input_port_information(ff_node, 1);
			allocate_more_output_pins(ff_node, 1);
			add_output_port_information(ff_node, 1);
			
			

			/* add the clock to the flip_flop */
			/* add a fanout pin */
			npin_t *fanout_pin_of_clock = allocate_npin();
			add_fanout_pin_to_net(clock_net, fanout_pin_of_clock);
			add_input_pin_to_node(ff_node, fanout_pin_of_clock, 1);

			/* hookup the driver pin (the in_1) to to this net (the lookup) */
			add_input_pin_to_node(ff_node, pin, 0);

			/* finally hookup the output pin of the flip flop to the orginal driver net */
			npin_t *ff_output_pin = allocate_npin();
			add_output_pin_to_node(ff_node, ff_output_pin, 0);

			if(net->driver_pin)
			{
				error_message(NETLIST_ERROR, always_node->line_number, always_node->file_number,
						"You've defined the driver \"%s\" twice\n", get_pin_name(pin->name));
			}
			add_driver_pin_to_net(net, ff_output_pin);

			verilog_netlist->ff_nodes = (nnode_t**)realloc(verilog_netlist->ff_nodes, sizeof(nnode_t*)*(verilog_netlist->num_ff_nodes+1));
			verilog_netlist->ff_nodes[verilog_netlist->num_ff_nodes] = ff_node;
			verilog_netlist->num_ff_nodes++;
		}
	}

	for (i = 0; i < memory_inputs->count; i++)
	{
		npin_t *pin = memory_inputs->pins[i];
		implicit_memory *memory = lookup_implicit_memory_input(pin->name);
		nnode_t *node = memory->node;

		int j;
		for (j = 0; j < node->num_input_pins; j++)
		{
			npin_t *original_pin = node->input_pins[j];
			if (original_pin->name && pin->name && !strcmp(original_pin->name, pin->name))
			{
				pin->mapping = original_pin->mapping;
				add_input_pin_to_node(node, pin, j);
				break;
			}
		}

		if (!memory->clock_added)
		{
			npin_t *clock_pin = allocate_npin();
			add_fanout_pin_to_net(clock_net, clock_pin);
			signal_list_t *clock = init_signal_list();
			add_pin_to_signal_list(clock, clock_pin);
			add_input_port_to_implicit_memory(memory, clock, "clk");
			free_signal_list(clock);
			memory->clock_added = TRUE;
		}
	}
	free_signal_list(memory_inputs);

	free_signal_list(assignment);
}

/*---------------------------------------------------------------------------------------------
 * (function: terminate_continuous_assignment)
 *-------------------------------------------------------------------------------------------*/
void terminate_continuous_assignment(ast_node_t *node, signal_list_t* assignment, char *instance_name_prefix)
{
	signal_list_t *memory_inputs = init_signal_list();
	int i;
	for (i = 0; i < assignment->count; i++)
	{
		npin_t *pin = assignment->pins[i];
		implicit_memory *memory = lookup_implicit_memory_input(pin->name);
		if (memory)
		{
			add_pin_to_signal_list(memory_inputs, pin);
		}
		else
		{
			/* look up the net */
			long sc_spot = sc_lookup_string(output_nets_sc, pin->name);
			if (sc_spot == -1)
			{
				error_message(NETLIST_ERROR, node->line_number, node->file_number,
						"Assignment (%s) is missing driver\n", pin->name);
			}

			nnet_t *net = output_nets_sc->data[sc_spot];

			if (net->name == NULL)
				net->name = pin->name;

			nnode_t *buf_node = allocate_nnode();
			buf_node->type = BUF_NODE;
			/* create the unique name for this gate */
			buf_node->name = node_name(buf_node, instance_name_prefix);

			buf_node->related_ast_node = node;
			/* allocate the pins needed */
			allocate_more_input_pins(buf_node, 1);
			add_input_port_information(buf_node, 1);
			allocate_more_output_pins(buf_node, 1);
			add_output_port_information(buf_node, 1);

			npin_t *buf_input_pin = pin;
			add_input_pin_to_node(buf_node, buf_input_pin, 0);

			/* finally hookup the output pin of the buffer to the orginal driver net */
			npin_t *buf_output_pin = allocate_npin();
			add_output_pin_to_node(buf_node, buf_output_pin, 0);

			if(net->driver_pin != NULL)
			{
				error_message(NETLIST_ERROR, node->line_number, node->file_number,
						"You've defined this driver %s twice. \n "
						"\tNote that Odin II does not currently support combinational a = ? overiding for if and case blocks.\n", pin->name);
			}
			add_driver_pin_to_net(net, buf_output_pin);
		}
	}

	for (i = 0; i < memory_inputs->count; i++)
	{
		npin_t *pin = memory_inputs->pins[i];
		implicit_memory *memory = lookup_implicit_memory_input(pin->name);
		nnode_t *node = memory->node;

		int j;
		for (j = 0; j < node->num_input_pins; j++)
		{
			npin_t *original_pin = node->input_pins[j];
			if (original_pin->name && pin->name && !strcmp(original_pin->name, pin->name))
			{
				pin->mapping = original_pin->mapping;
				add_input_pin_to_node(node, pin, j);
				break;
			}
		}
	}
	free_signal_list(memory_inputs);

	free_signal_list(assignment);
}

/*---------------------------------------------------------------------------------------------
 * (function: alias_output_assign_pins_to_inputs)
 * 	Makes the names of the pins in the input list have the name of the output assignment
 *-------------------------------------------------------------------------------------------*/
int alias_output_assign_pins_to_inputs(char_list_t *output_list, signal_list_t *input_list, ast_node_t *node)
{
	int i;

	if (output_list->num_strings >= input_list->count)
	{
		for (i = 0; i < input_list->count; i++)
		{
			input_list->pins[i]->name = output_list->strings[i];
		}
		for (i = input_list->count; i < output_list->num_strings; i++)
		{
			if (global_args.all_warnings)
				warning_message(NETLIST_ERROR, node->line_number, node->file_number,
						"More nets to drive than drivers, padding with ZEROs for driver %s\n", output_list->strings[i]);

			add_pin_to_signal_list(input_list, get_zero_pin(verilog_netlist));
			input_list->pins[i]->name = output_list->strings[i];
		}

		return output_list->num_strings;
	}
	else
	{
		for (i = 0; i < output_list->num_strings; i++)
		{
			input_list->pins[i]->name = output_list->strings[i];
		}

		if (global_args.all_warnings)
			warning_message(NETLIST_ERROR, node->line_number, node->file_number,
					"Alias: More driver pins than nets to drive: sometimes using decimal numbers causes this problem\n");
		return output_list->num_strings;
	}
}

/*--------------------------------------------------------------------------
 * (function: create_gate)
 * 	This function creates a gate node in the netlist and hooks up the inputs
 * 	and outputs.
 *------------------------------------------------------------------------*/
signal_list_t *create_gate(ast_node_t* gate, char *instance_name_prefix)
{
	signal_list_t *in_1, *in_2, *out_1;
	nnode_t *gate_node;
	ast_node_t *gate_instance = gate->children[0];

	if (gate_instance->children[3] == NULL)
	{
		/* IF one input gate */
		
		/* process the signal for the input gate */
		in_1 = netlist_expand_ast_of_module(gate_instance->children[2], instance_name_prefix);
		/* process the signal for the input ga$te */
		out_1 = create_output_pin(gate_instance->children[1], instance_name_prefix);
		oassert((in_1 != NULL) && (out_1 != NULL));

		/* create the node */
		gate_node = allocate_nnode();
		/* store all the relevant info */
		gate_node->related_ast_node = gate;
		gate_node->type = gate->types.operation.op;
		oassert(gate_node->type > 0);
		gate_node->name = node_name(gate_node, instance_name_prefix);
		/* allocate the pins needed */
		allocate_more_input_pins(gate_node, 1);
		add_input_port_information(gate_node, 1);
		allocate_more_output_pins(gate_node, 1);
		add_output_port_information(gate_node, 1);
		
		/* hookup the input pins */
		hookup_input_pins_from_signal_list(gate_node, 0, in_1, 0, 1, verilog_netlist);
		/* hookup the output pins */
		hookup_output_pins_from_signal_list(gate_node, 0, out_1, 0, 1);

		free_signal_list(in_1);
	}
	else
	{
		/* ELSE 2 input gate */
		
		/* process the signal for the input gate */
		in_1 = netlist_expand_ast_of_module(gate_instance->children[2], instance_name_prefix);
		in_2 = netlist_expand_ast_of_module(gate_instance->children[3], instance_name_prefix);
		/* process the signal for the input gate */
		out_1 = create_output_pin(gate_instance->children[1], instance_name_prefix);
		oassert((in_1 != NULL) && (in_2 != NULL) && (out_1 != NULL));

		/* create the node */
		gate_node = allocate_nnode();
		/* store all the relevant info */
		gate_node->related_ast_node = gate;
		gate_node->type = gate->types.operation.op;
		oassert(gate_node->type > 0);

		gate_node->name = node_name(gate_node, instance_name_prefix);
		/* allocate the needed pins */
		allocate_more_input_pins(gate_node, 2);
		add_input_port_information(gate_node, 1);
		add_input_port_information(gate_node, 1);
		allocate_more_output_pins(gate_node, 1);
		add_output_port_information(gate_node, 1);
		
		/* hookup the input pins */
		hookup_input_pins_from_signal_list(gate_node, 0, in_1, 0, 1, verilog_netlist);
		hookup_input_pins_from_signal_list(gate_node, 1, in_2, 0, 1, verilog_netlist);
		/* hookup the output pins */
		hookup_output_pins_from_signal_list(gate_node, 0, out_1, 0, 1);

		free_signal_list(in_1);
		free_signal_list(in_2);
	}
	
	return out_1;
}



/*----------------------------------------------------------------------------
 * (function: create_operation_node)
 *--------------------------------------------------------------------------*/
signal_list_t *create_operation_node(ast_node_t *op, signal_list_t **input_lists, int list_size, char *instance_name_prefix)
{
	int i;
	signal_list_t *return_list = init_signal_list();
	nnode_t *operation_node;
	int max_input_port_width = -1;
	int output_port_width = -1;
	int input_port_width = -1;
	int current_idx;

	/* create the node */
	operation_node = allocate_nnode();
	/* store all the relevant info */
	operation_node->related_ast_node = op;
	operation_node->type = op->types.operation.op;
	operation_node->name = node_name(operation_node, instance_name_prefix);

	current_idx = 0;

	/* analyse the inputs */
	for (i = 0; i < list_size; i++)
	{
		if (max_input_port_width < input_lists[i]->count)
		{
			max_input_port_width = input_lists[i]->count;
		}
	}
		
	switch(operation_node->type)
	{
		case BITWISE_NOT: // ~	
			/* only one input port */
			output_port_width = max_input_port_width;
			input_port_width = output_port_width;
			break;
		case ADD: // +
			/* add the largest bit width + the other input padded with 0's */
			return_list->is_adder = TRUE;
			output_port_width = max_input_port_width + 1;
			input_port_width = max_input_port_width;

			add_list = insert_in_vptr_list(add_list, operation_node);
			break;
		case MINUS: // -
			/* subtract the largest bit width + the other input padded with 0's ... concern for 2's comp */
			output_port_width = max_input_port_width;
			input_port_width = output_port_width;
			sub_list = insert_in_vptr_list(sub_list, operation_node);

			break;
		case MULTIPLY: // *
			/* pad the smaller one with 0's */
			output_port_width = input_lists[0]->count + input_lists[1]->count;
			input_port_width = -2;

			/* Record multiply nodes for netlist optimization */
			mult_list = insert_in_vptr_list(mult_list, operation_node);
			break;
		case BITWISE_AND: // &
		case BITWISE_OR: // |
		case BITWISE_NAND: // ~&
		case BITWISE_NOR: // ~| 
		case BITWISE_XNOR: // ~^ 
		case BITWISE_XOR: // ^
			/* we'll padd the other inputs with 0, do it for the largest and throw a warning */
			if (list_size == 2)
			{
				output_port_width = max_input_port_width;
				input_port_width = output_port_width;
			}
			else 
			{
				oassert(list_size == 1);
				/* Logical reduction - same as a logic function */
				output_port_width = 1;
				input_port_width = max_input_port_width;
			}
			break;
		case SR: // >>
		case SL: // <<
			/* Shifts doesn't matter about port size, but second input needs to be a number */
			output_port_width = max_input_port_width;
			input_port_width = -2;
			break;
		case LOGICAL_NOT: // ! 
		case LOGICAL_OR: // ||
		case LOGICAL_AND: // &&
			/* only one input port */
			output_port_width = 1;
			input_port_width = max_input_port_width;
			break;
		case LT: // <
		case GT: // >
		case LOGICAL_EQUAL: // == 
		case NOT_EQUAL: // !=
		case LTE: // <=
		case GTE: // >=
		{
			if (input_lists[0]->count != input_lists[1]->count)
			{
				int index_of_smallest = find_smallest_non_numerical(op, input_lists, 2);

				input_port_width = input_lists[index_of_smallest]->count;

				/* pad the input lists */
				pad_with_zeros(op, input_lists[0], input_port_width, instance_name_prefix);
				pad_with_zeros(op, input_lists[1], input_port_width, instance_name_prefix);
			}
			else
			{
				input_port_width = input_lists[0]->count;
			}
			output_port_width = 1;
			break;
		}
		case DIVIDE: // /
			error_message(NETLIST_ERROR,  op->line_number, op->file_number, "Divide operation not supported by Odin\n");
			break;
		case MODULO: // %
			error_message(NETLIST_ERROR,  op->line_number, op->file_number, "Modulo operation not supported by Odin\n");
			break;
		default:
			error_message(NETLIST_ERROR,  op->line_number, op->file_number, "Operation not supported by Odin\n");
			break;
	}

	oassert(input_port_width != -1);
	oassert(output_port_width != -1);

	for (i = 0; i < list_size; i++)
	{
		if ((operation_node->type == SR) || (operation_node->type == SL))
		{
			/* Need to check that 2nd operand is constant */
			ast_node_t *second = resolve_node(instance_name_prefix, op->children[1]);
			if (second->type != NUMBERS)
				error_message(NETLIST_ERROR, op->line_number, op->file_number, "Odin only supports constant shifts at present\n");
			oassert(second->type == NUMBERS);

			/* for shift left or right, it's actually a one port operation. The 2nd port is constant */
			if (i == 0)
			{
				/* allocate the pins needed */
				allocate_more_input_pins(operation_node, input_lists[i]->count);
				/* record this port size */
				add_input_port_information(operation_node, input_lists[i]->count);
				/* hookup the input pins */
				hookup_input_pins_from_signal_list(operation_node, current_idx, input_lists[i], 0, input_lists[i]->count, verilog_netlist);
			}
		}
		else if (input_port_width != -2)
		{
			/* IF taking port widths based on preset */
			/* allocate the pins needed */
			allocate_more_input_pins(operation_node, input_port_width);
			/* record this port size */
			add_input_port_information(operation_node, input_port_width);

			/* hookup the input pins = will do padding of zeros for smaller port */
			hookup_input_pins_from_signal_list(operation_node, current_idx, input_lists[i], 0, input_port_width, verilog_netlist);
			current_idx += input_port_width;
		}
		else
		{
			/* ELSE if taking the port widths as they are */
			/* allocate the pins needed */
			allocate_more_input_pins(operation_node, input_lists[i]->count);
			/* record this port size */
			add_input_port_information(operation_node, input_lists[i]->count);

			/* hookup the input pins */
			hookup_input_pins_from_signal_list(operation_node, current_idx, input_lists[i], 0, input_lists[i]->count, verilog_netlist);
			
			current_idx += input_lists[i]->count;
		}


	}
	/* allocate the pins for the ouput port */
	allocate_more_output_pins(operation_node, output_port_width);
	add_output_port_information(operation_node, output_port_width);
		
	/* make the inplicit output list and hook up the outputs */
	for (i = 0; i < output_port_width; i++)
	{
		npin_t *new_pin1;
		npin_t *new_pin2;
		nnet_t *new_net;
		new_pin1 = allocate_npin();
		new_pin2 = allocate_npin();
		new_net = allocate_nnet();
		new_net->name = operation_node->name;
		/* hook the output pin into the node */
		add_output_pin_to_node(operation_node, new_pin1, i);
		/* hook up new pin 1 into the new net */
		add_driver_pin_to_net(new_net, new_pin1);
		/* hook up the new pin 2 to this new net */
		add_fanout_pin_to_net(new_net, new_pin2);
		
		/* add the new_pin 2 to the list of outputs */
		add_pin_to_signal_list(return_list, new_pin2);
	}

	for (i = 0; i < list_size; i++)
	{
		free_signal_list(input_lists[i]);
	}

	return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function: evaluate_sensitivity_list)
 *-------------------------------------------------------------------------------------------*/
signal_list_t *evaluate_sensitivity_list(ast_node_t *delay_control, char *instance_name_prefix)
{
	int i;
	short edge_type = -1;
	signal_list_t *return_sig_list = init_signal_list();
	signal_list_t *temp_list;

	if (delay_control == NULL)
	{
		/* Assume always @(*) */
		free_signal_list(return_sig_list);
		return_sig_list = NULL;
		type_of_circuit = COMBINATIONAL;

		return return_sig_list;
	}

	oassert(delay_control->type == DELAY_CONTROL);

	for (i = 0; i < delay_control->num_children; i++)
	{
		if (	((delay_control->children[i]->type == NEGEDGE) || (delay_control->children[i]->type == POSEDGE)) 
			&& 
			((edge_type == -1) || (edge_type == SEQUENTIAL)))
		{
			edge_type = SEQUENTIAL;		

			temp_list = create_pins(delay_control->children[i]->children[0], NULL, instance_name_prefix);
			oassert(temp_list->count == 1);

			add_pin_to_signal_list(return_sig_list, temp_list->pins[0]);
			free_signal_list(temp_list);
		}
		else if ((edge_type == -1) || (edge_type == COMBINATIONAL))
		{
			/* ELSE - a combinational edge and we don't need to do anything */
			edge_type = COMBINATIONAL;
		}
		else
		{
			error_message(NETLIST_ERROR, delay_control->line_number, delay_control->file_number,
					"Sensitivity list switches from sequential and combinational.  You can't define something like always @(posedge clock or a).\n");
		}
	}

	/* update the analysis type of this block of statements */
	if (edge_type == -1)
	{
		error_message(NETLIST_ERROR, delay_control->line_number, delay_control->file_number, "Sensitivity list error...looks empty?\n");
	}
	else if (edge_type == COMBINATIONAL)
	{
		free_signal_list(return_sig_list);
		return_sig_list = NULL;
		type_of_circuit = edge_type;
	}
	else if (edge_type == SEQUENTIAL)
	{
		type_of_circuit = edge_type;
	}

	return return_sig_list;
}

/*---------------------------------------------------------------------------------------------
 * (function: create_if_for_question)
 *-------------------------------------------------------------------------------------------*/
signal_list_t *create_if_for_question(ast_node_t *if_ast, char *instance_name_prefix)
{
	signal_list_t *return_list;
	nnode_t *if_node;

	/* create the node */
	if_node = allocate_nnode();
	/* store all the relevant info */
	if_node->related_ast_node = if_ast;
	if_node->type = MULTI_PORT_MUX; // port 1 = control, port 2+ = mux options
	if_node->name = node_name(if_node, instance_name_prefix);

	/* create the control structure for the if node */
	create_if_control_signals(if_ast->children[0], if_node, instance_name_prefix);

	/* create the statements and integrate them into the mux */
	return_list = create_if_question_mux_expressions(if_ast, if_node, instance_name_prefix);

	return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function:  create_if_question_mux_expressions)
 *-------------------------------------------------------------------------------------------*/
signal_list_t *create_if_question_mux_expressions(ast_node_t *if_ast, nnode_t *if_node, char *instance_name_prefix)
{
	signal_list_t **if_expressions;
	signal_list_t *return_list;
	int i;

	/* make storage for statements and expressions */
	if_expressions = (signal_list_t**)malloc(sizeof(signal_list_t*)*2);

	/* now we will process the statements and add to the other ports */
	for (i = 0; i < 2; i++)
	{
		if (if_ast->children[i+1] != NULL) // checking to see if expression exists.  +1 since first child is control expression
		{
			/* IF - this is a normal case item, then process the case match and the details of the statement */
			if_expressions[i] = netlist_expand_ast_of_module(if_ast->children[i+1], instance_name_prefix);
		}
		else 
		{
			error_message(NETLIST_ERROR, if_ast->line_number, if_ast->file_number, "No such thing as a a = b ? z;\n");
		}
	}
	
	/* now with all the lists sorted, we do the matching and proper propogation */	
	return_list = create_mux_expressions(if_expressions, if_node, 2, instance_name_prefix);

	return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function: create_if)
 *-------------------------------------------------------------------------------------------*/
signal_list_t *create_if(ast_node_t *if_ast, char *instance_name_prefix)
{
	signal_list_t *return_list;
	nnode_t *if_node;

	/* create the node */
	if_node = allocate_nnode();
	/* store all the relevant info */
	if_node->related_ast_node = if_ast;
	if_node->type = MULTI_PORT_MUX; // port 1 = control, port 2+ = mux options
	if_node->name = node_name(if_node, instance_name_prefix);

	/* create the control structure for the if node */
	create_if_control_signals(if_ast->children[0], if_node, instance_name_prefix);

	/* create the statements and integrate them into the mux */
	return_list = create_if_mux_statements(if_ast, if_node, instance_name_prefix);

	return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function:  create_if_control_signals)
 *-------------------------------------------------------------------------------------------*/
void create_if_control_signals(ast_node_t *if_expression, nnode_t *if_node, char *instance_name_prefix)
{
	signal_list_t *if_logic_expression;
	signal_list_t *if_logic_expression_final;
	nnode_t *not_node;
	npin_t *not_pin;
	signal_list_t *out_pin_list;

	/* reserve the first 2 pins of the mux for the control signals */
	allocate_more_input_pins(if_node, 2);
	/* record this port size */
	add_input_port_information(if_node, 2);
	
	/* get the logic */
	if_logic_expression = netlist_expand_ast_of_module(if_expression, instance_name_prefix);
	oassert(if_logic_expression != NULL);

	if(if_logic_expression->count != 1)
	{
		nnode_t *or_gate;
		signal_list_t *default_expression;
		
		or_gate = make_1port_logic_gate_with_inputs(LOGICAL_OR, if_logic_expression->count, if_logic_expression, if_node, -1);
		default_expression = make_output_pins_for_existing_node(or_gate, 1);
		
		/* copy that output pin to be put into the default */
		add_input_pin_to_node(if_node, default_expression->pins[0], 0);

		if_logic_expression_final = default_expression;
	}
	else
	{
		/* hookup this pin to the spot in the if_node */
		add_input_pin_to_node(if_node, if_logic_expression->pins[0], 0);
		if_logic_expression_final = if_logic_expression;
	}

	/* hookup pin again to not gate and then to other spot */
	not_pin = copy_input_npin(if_logic_expression_final->pins[0]);

	/* make a NOT gate that collects all the other signals and if they're all off */
	not_node = make_not_gate_with_input(not_pin, if_node, -1);

	/* get the output pin of the not gate .... also adds a net inbetween and the linking output pin to node and net */
	out_pin_list = make_output_pins_for_existing_node(not_node, 1);
	oassert(out_pin_list->count == 1);
			

	// Mark the else condition for the simulator.
	out_pin_list->pins[0]->is_default = TRUE;

	/* copy that output pin to be put into the default */
	add_input_pin_to_node(if_node, out_pin_list->pins[0], 1);

	free_signal_list(out_pin_list);
}

/*---------------------------------------------------------------------------------------------
 * (function:  create_if_mux_statements)
 *-------------------------------------------------------------------------------------------*/
signal_list_t *create_if_mux_statements(ast_node_t *if_ast, nnode_t *if_node, char *instance_name_prefix)
{
	signal_list_t **if_statements;
	signal_list_t *return_list;
	int i;

	/* make storage for statements and expressions */
	if_statements = (signal_list_t**)malloc(sizeof(signal_list_t*)*2);

	/* now we will process the statements and add to the other ports */
	for (i = 0; i < 2; i++)
	{
		if (if_ast->children[i+1] != NULL) // checking to see if statement exists.  +1 since first child is control expression
		{
			/* IF - this is a normal case item, then process the case match and the details of the statement */
			if_statements[i] = netlist_expand_ast_of_module(if_ast->children[i+1], instance_name_prefix);
			sort_signal_list_alphabetically(if_statements[i]);
		}
		else 
		{
			/* ELSE - there is no if/else and we need to make a place holder since it means there will be implied signals */
			if_statements[i] = init_signal_list();
		}
	}
	
	/* now with all the lists sorted, we do the matching and proper propagation */
	return_list = create_mux_statements(if_statements, if_node, 2, instance_name_prefix);

	return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function: create_case)
 *-------------------------------------------------------------------------------------------*/
signal_list_t *create_case(ast_node_t *case_ast, char *instance_name_prefix)
{
	signal_list_t *return_list;
	nnode_t *case_node;
	ast_node_t *case_list_of_items;
	ast_node_t *case_match_input;

	/* create the node */
	case_node = allocate_nnode();
	/* store all the relevant info */
	case_node->related_ast_node = case_ast;
	case_node->type = MULTI_PORT_MUX; // port 1 = control, port 2+ = mux options
	case_node->name = node_name(case_node, instance_name_prefix);

	/* pull out the identifier to match to "i.e. case(a_in)" - this is the pins we match against */
	case_match_input = case_ast->children[0];

	/* point to the case list */
	case_list_of_items = case_ast->children[1];
	
	/* create all the control structures for the case mux ... each bit will turn on one of the paths ... one hot mux */
	create_case_control_signals(case_list_of_items, case_match_input, case_node, instance_name_prefix);

	/* create the statements and integrate them into the mux */
	return_list = create_case_mux_statements(case_list_of_items, case_node, instance_name_prefix);

	return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function:  create_case_control_signals)
 *-------------------------------------------------------------------------------------------*/
void create_case_control_signals(ast_node_t *case_list_of_items, ast_node_t *compare_against, nnode_t *case_node, char *instance_name_prefix)
{
	int i;
	signal_list_t *other_expressions_pin_list = init_signal_list();

	/* reserve the first X pins of the mux for the control signals where X is the number of items */
	allocate_more_input_pins(case_node, case_list_of_items->num_children);
	/* record this port size */
	add_input_port_information(case_node, case_list_of_items->num_children);
	
	/* now go through each of the case items and build the comparison expressions */
	for (i = 0; i < case_list_of_items->num_children; i++)
	{
		if (case_list_of_items->children[i]->type == CASE_ITEM)
		{
			/* IF - this is a normal case item, then process the case match and the details of the statement */
			signal_list_t *case_compare_expression;
			signal_list_t **case_compares = (signal_list_t **)malloc(sizeof(signal_list_t*)*2);
			ast_node_t *logical_equal = create_node_w_type(BINARY_OPERATION, -1, -1);
			logical_equal->types.operation.op = LOGICAL_EQUAL;

			/* get the signals to compare against */
			case_compares[0] = netlist_expand_ast_of_module(compare_against, instance_name_prefix);
			case_compares[1] = netlist_expand_ast_of_module(case_list_of_items->children[i]->children[0], instance_name_prefix);

			/* make a LOGIC_EQUAL gate that collects all the other signals and if they're all off */
			case_compare_expression = create_operation_node(logical_equal, case_compares, 2, instance_name_prefix);
			oassert(case_compare_expression->count == 1);

			/* hookup this pin to the spot in the case_node */
			add_input_pin_to_node(case_node, case_compare_expression->pins[0], i);

			/* copy that output pin to be put into the default */
			add_pin_to_signal_list(other_expressions_pin_list, copy_input_npin(case_compare_expression->pins[0]));
			
			/* clean up */
			free_signal_list(case_compare_expression);

			free(case_compares);
		}
		else if (case_list_of_items->children[i]->type == CASE_DEFAULT)
		{
			/* take all the other pins from the case expressions and intstall in the condition */
			nnode_t *default_node;
			signal_list_t *default_expression;

			oassert(i == case_list_of_items->num_children - 1); // has to be at the end

			/* make a NOR gate that collects all the other signals and if they're all off */
			default_node = make_1port_logic_gate_with_inputs(LOGICAL_NOR, case_list_of_items->num_children-1, other_expressions_pin_list, case_node, -1);
			default_expression = make_output_pins_for_existing_node(default_node, 1);
			
			// Mark the "default" case for simulation.
			default_expression->pins[0]->is_default = TRUE;

			/* copy that output pin to be put into the default */
			add_input_pin_to_node(case_node, default_expression->pins[0], i);
		}
		else
		{
			oassert(FALSE);
		}
	}
}

/*---------------------------------------------------------------------------------------------
 * (function:  create_case_mux_statements)
 *-------------------------------------------------------------------------------------------*/
signal_list_t *create_case_mux_statements(ast_node_t *case_list_of_items, nnode_t *case_node, char *instance_name_prefix)
{
	signal_list_t **case_statement;
	signal_list_t *return_list;
	int i;

	/* make storage for statements and expressions */
	case_statement = (signal_list_t**)malloc(sizeof(signal_list_t*)*(case_list_of_items->num_children));

	/* now we will process the statements and add to the other ports */
	for (i = 0; i < case_list_of_items->num_children; i++)
	{
		if (case_list_of_items->children[i]->type == CASE_ITEM)
		{
			/* IF - this is a normal case item, then process the case match and the details of the statement */
			case_statement[i] = netlist_expand_ast_of_module(case_list_of_items->children[i]->children[1], instance_name_prefix);
			sort_signal_list_alphabetically(case_statement[i]);
		}
		else if (case_list_of_items->children[i]->type == CASE_DEFAULT)
		{
			oassert(i == case_list_of_items->num_children - 1); // has to be at the end
			case_statement[i] = netlist_expand_ast_of_module(case_list_of_items->children[i]->children[0], instance_name_prefix);
			sort_signal_list_alphabetically(case_statement[i]);
		}
		else
		{
			oassert(FALSE);
		}
	}
	
	/* now with all the lists sorted, we do the matching and proper propogation */	
	return_list = create_mux_statements(case_statement, case_node, case_list_of_items->num_children, instance_name_prefix);

	return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function:  create_mux_statements)
 *-------------------------------------------------------------------------------------------*/
signal_list_t *create_mux_statements(signal_list_t **statement_lists, nnode_t *mux_node, int num_statement_lists, char *instance_name_prefix)
{
	int i, j;
	signal_list_t *combined_lists;
	int *per_case_statement_idx;
	signal_list_t *return_list = init_signal_list();
	int in_index = 1;
	int out_index = 0;

	/* allocate and initialize indexes */
	per_case_statement_idx = (int*)calloc(sizeof(int), num_statement_lists);

	/* make the uber list and sort it */
	combined_lists = combine_lists_without_freeing_originals(statement_lists, num_statement_lists);
	sort_signal_list_alphabetically(combined_lists);

	for (i = 0; i < combined_lists->count; i++)
	{
		int i_skip = 0; // iskip is the number of statemnts that do have this signal so we can skip in the combine list
		npin_t *new_pin1;
		npin_t *new_pin2;
		nnet_t *new_net;
		new_pin1 = allocate_npin();
		new_pin2 = allocate_npin();
		new_net = allocate_nnet();

		/* allocate a port the width of all the signals ... one MUX */
		allocate_more_input_pins(mux_node, num_statement_lists);
		/* record the port size */
		add_input_port_information(mux_node, num_statement_lists);

		/* allocate the pins for the ouput port and pass out that pin for higher statements */
		allocate_more_output_pins(mux_node, 1);
		add_output_pin_to_node(mux_node, new_pin1, out_index);
		/* hook up new pin 1 into the new net */
		add_driver_pin_to_net(new_net, new_pin1);
		/* hook up the new pin 2 to this new net */
		add_fanout_pin_to_net(new_net, new_pin2);
		/* name it with this name */
		new_pin2->name = combined_lists->pins[i]->name;
		/* add this pin to the return list */
		add_pin_to_signal_list(return_list, new_pin2);

		/* going through each of the statement lists looking for common ones and building implied ones if they're not there */
		for (j = 0; j < num_statement_lists; j++)
		{
			int pin_index = in_index*num_statement_lists+j;

			/* check if the current element for this case statement is defined */ 	
			if (
					   (per_case_statement_idx[j] < statement_lists[j]->count)
					&& (strcmp(combined_lists->pins[i]->name, statement_lists[j]->pins[per_case_statement_idx[j]]->name) == 0)
			)
			{
				/* If they match then we have a signal with this name and we can attach the pin */ 
				add_input_pin_to_node(mux_node, statement_lists[j]->pins[per_case_statement_idx[j]], pin_index);

				per_case_statement_idx[j]++; // increment the local index
				i_skip ++; // it's a match so the combo list will have atleast +1 entries the same
			}
			else
			{
				/* Don't match, so this signal is an IMPLIED SIGNAL !!! */
				npin_t *pin = combined_lists->pins[i];

				/* implied signal for mux */
				if (type_of_circuit == SEQUENTIAL)
				{
					if (lookup_implicit_memory_input(pin->name))
					{
						// If the mux feeds an implicit memory, imply zero.
						add_input_pin_to_node(mux_node, get_zero_pin(verilog_netlist), pin_index);
					}
					else
					{
						/* lookup this driver name */
						signal_list_t *this_pin_list = create_pins(NULL, pin->name, instance_name_prefix);
						oassert(this_pin_list->count == 1);
						//add_a_input_pin_to_node_spot_idx(mux_node, get_zero_pin(verilog_netlist), pin_index);
						add_input_pin_to_node(mux_node, this_pin_list->pins[0], pin_index);
						/* clean up */
						free_signal_list(this_pin_list);
					}
				}
				else if (type_of_circuit == COMBINATIONAL)
				{
					/* DON'T CARE - so hookup zero */
					add_input_pin_to_node(mux_node, get_zero_pin(verilog_netlist), pin_index);
					// Allows the simulator to be aware of the implied nature of this signal.
					mux_node->input_pins[pin_index]->is_implied = TRUE;
				}
				else
					oassert(FALSE);
			}
		}

		i += i_skip - 1; // for every match move index i forward except this one wihich is handled by for i++ 
		in_index++;
		out_index++;
	}

	/* clean up */
	for (i = 0; i < num_statement_lists; i++)
	{
		free_signal_list(statement_lists[i]);
	}
	free_signal_list(combined_lists);

	return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function:  create_mux_expressions)
 *-------------------------------------------------------------------------------------------*/
signal_list_t *create_mux_expressions(signal_list_t **expression_lists, nnode_t *mux_node, int num_expression_lists, char *instance_name_prefix)
{
	int i, j;
	signal_list_t *return_list = init_signal_list();
	int max_index = -1;

	/* find the biggest element */
	for (i = 0; i < num_expression_lists; i++)
	{
		if (max_index < expression_lists[i]->count)
		{
			max_index = expression_lists[i]->count;
		}
	}

	for (i = 0; i < max_index; i++)
	{
		npin_t *new_pin1;
		npin_t *new_pin2;
		nnet_t *new_net;
		new_pin1 = allocate_npin();
		new_pin2 = allocate_npin();
		new_net = allocate_nnet();

		/* allocate a port the width of all the signals ... one MUX */
		allocate_more_input_pins(mux_node, num_expression_lists);
		/* record the port information */
		add_input_port_information(mux_node, num_expression_lists);

		/* allocate the pins for the ouput port and pass out that pin for higher statements */
		allocate_more_output_pins(mux_node, 1);
		add_output_pin_to_node(mux_node, new_pin1, i);
		/* hook up new pin 1 into the new net */
		add_driver_pin_to_net(new_net, new_pin1);
		/* hook up the new pin 2 to this new net */
		add_fanout_pin_to_net(new_net, new_pin2);
		/* name it with this name */
		new_pin2->name = NULL; 
		/* add this pin to the return list */
		add_pin_to_signal_list(return_list, new_pin2);

		/* going through each of the statement lists looking for common ones and building implied ones if they're not there */
		for (j = 0; j < num_expression_lists; j++)
		{
			int pin_index = (i+1)*num_expression_lists+j;

			/* check if the current element for this case statement is defined */ 	
			if (i < expression_lists[j]->count)
			{
				/* If there is a signal */
				add_input_pin_to_node(mux_node, expression_lists[j]->pins[i], pin_index);

			}
			else
			{
				/* Don't match, so this signal is an IMPLIED SIGNAL !!! */
				/* implied signal for mux */
				/* DON'T CARE - so hookup zero */
				add_input_pin_to_node(mux_node, get_zero_pin(verilog_netlist), pin_index);
			}
		}
	}

	/* clean up */
	for (i = 0; i < num_expression_lists; i++)
	{
		free_signal_list(expression_lists[i]);
	}

	return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function:  pad_compares_to_smallest_non_numerical_implementation)
 *-------------------------------------------------------------------------------------------*/
int find_smallest_non_numerical(ast_node_t *node, signal_list_t **input_list, int num_input_lists)
{
	int i;
	int smallest;
	int smallest_idx;
	short *tested = (short*)calloc(sizeof(short), num_input_lists);
	short found_non_numerical = FALSE;
	
	while(found_non_numerical == FALSE)
	{
		smallest_idx = -1;
		smallest = -1;

		/* find the smallest width, now verify that it's not a number */
		for (i = 0; i < num_input_lists; i++)
		{
			if (tested[i] == 1)
			{
				/* skip the ones we've already tried */
				continue;
			}
			if ((smallest == -1) || (smallest >= input_list[i]->count))
			{
				smallest = input_list[i]->count;
				smallest_idx = i;
			}
		}	

		if (smallest_idx == -1)
		{
			error_message(NETLIST_ERROR, node->line_number, node->file_number, "all numbers in padding non numericals\n");
		}
		else
		{
			/* mark that we're evaluating this input */
			tested[smallest_idx] = TRUE;

			/* check if the smallest is not a number */
			for (i = 0; i < input_list[smallest_idx]->count; i++)
			{
				if (input_list[smallest_idx]->pins[i]->name == NULL)
				{
					/* Not a number so this is the smallest */
					found_non_numerical = TRUE;
					break;
				}
				if (!((strstr(input_list[smallest_idx]->pins[i]->name, "ONE_VCC_CNS") != NULL)
						|| strstr(input_list[smallest_idx]->pins[i]->name, "ZERO_GND_ZERO") != NULL))
				{
					/* Not a number so this is the smallest */
					found_non_numerical = TRUE;
					break;
				}
			}
		}
	}

	return smallest_idx;
}

/*---------------------------------------------------------------------------------------------
 * (function: pad_with_zeros)
 *-------------------------------------------------------------------------------------------*/
void pad_with_zeros(ast_node_t* node, signal_list_t *list, int pad_size, char *instance_name_prefix)
{
	int i;

	if (pad_size > list->count)
	{
		for (i = list->count; i < pad_size; i++)
		{
			if (global_args.all_warnings)
				warning_message(NETLIST_ERROR, node->line_number, node->file_number,
						"Padding an input port with 0 for operation (likely compare)\n");
			add_pin_to_signal_list(list, get_zero_pin(verilog_netlist));
		}
	}
	else if (pad_size < list->count) 
	{
		if (global_args.all_warnings)
			warning_message(NETLIST_ERROR, node->line_number, node->file_number,
					"More driver pins than nets to drive.  This means that for this operation you are losing some of the most significant bits\n");
	}
}

#ifdef VPR6
/*--------------------------------------------------------------------------
 * (function: create_dual_port_ram_block)
 * 	This function creates a dual port ram block node in the netlist 
 *	and hooks up the inputs and outputs.
 *------------------------------------------------------------------------*/
signal_list_t *create_dual_port_ram_block(ast_node_t* block, char *instance_name_prefix, t_model* hb_model)
{
	if (!hb_model || !is_ast_dp_ram(block))
		error_message(NETLIST_ERROR, block->line_number, block->file_number, "Error in creating dual port ram\n");

	block->type = RAM;

	ast_node_t *block_instance = block->children[1];
	ast_node_t *block_list = block_instance->children[1];
	ast_node_t *block_connect;

	/* create the node */
	nnode_t *block_node = allocate_nnode();
	/* store all of the relevant info */
	block_node->related_ast_node = block;
	block_node->type = HARD_IP;
	block_node->name = hard_node_name(block_node, instance_name_prefix, block->children[0]->types.identifier, block_instance->children[0]->types.identifier);
	
	/* Declare the hard block as used for the blif generation */
	hb_model->used = 1;

	/* Need to do a sanity check to make sure ports line up */
	t_model_ports *hb_ports;
	int i;
	for (i = 0; i < block_list->num_children; i++)
	{
		block_connect = block_list->children[i];
		char *ip_name = block_connect->children[0]->types.identifier;
		hb_ports = hb_model->inputs;

		while (hb_ports && strcmp(hb_ports->name, ip_name))
			hb_ports = hb_ports->next;

		if (!hb_ports)
		{
			hb_ports = hb_model->outputs;
			while ((hb_ports != NULL) && (strcmp(hb_ports->name, ip_name) != 0))
				hb_ports = hb_ports->next;
		}

		if (!hb_ports)
			error_message(NETLIST_ERROR, block->line_number, block->file_number, "Non-existant port %s in hard block %s\n", ip_name, block->children[0]->types.identifier);

		/* Link the signal to the port definition */
		block_connect->children[1]->hb_port = (void *)hb_ports;
	}

	signal_list_t **in_list = (signal_list_t **)malloc(sizeof(signal_list_t *)*block_list->num_children);
	int out_port_size1 = 0;
	int out_port_size2 = 0;
	int current_idx = 0;
	for (i = 0; i < block_list->num_children; i++)
	{
		int port_size;
		ast_node_t *block_port_connect;

		in_list[i] = NULL;
		block_connect = block_list->children[i]->children[0];
		block_port_connect = block_list->children[i]->children[1];
		hb_ports = (t_model_ports *)block_list->children[i]->children[1]->hb_port;
		char *ip_name = block_connect->types.identifier;

		if (hb_ports->dir == IN_PORT)
		{
			/* Create the pins for port if needed */
			in_list[i] = create_pins(block_port_connect, NULL, instance_name_prefix);
			port_size = in_list[i]->count;
			if (strcmp(hb_ports->name, "data1") == 0)
				out_port_size1 = port_size;
			if (strcmp(hb_ports->name, "data2") == 0)
				out_port_size2 = port_size;

			int j;
			for (j = 0; j < port_size; j++)
				in_list[i]->pins[j]->mapping = ip_name;

			/* allocate the pins needed */
			allocate_more_input_pins(block_node, port_size);
			/* record this port size */
			add_input_port_information(block_node, port_size);

			/* hookup the input pins */
			hookup_hb_input_pins_from_signal_list(block_node, current_idx, in_list[i], 0, port_size, verilog_netlist);

			/* Name any grounded ports in the block mapping */
			for (j = port_size; j < port_size; j++)
				block_node->input_pins[current_idx+j]->mapping = strdup(ip_name);
			current_idx += port_size;
		}
	}

	if (out_port_size2 != 0)
		oassert(out_port_size2 == out_port_size1);

	int current_out_idx = 0;
	signal_list_t *return_list = init_signal_list();
	for (i = 0; i < block_list->num_children; i++)
	{
		block_connect = block_list->children[i]->children[0];
		hb_ports = (t_model_ports *)block_list->children[i]->children[1]->hb_port;
		char *ip_name = block_connect->types.identifier;

		int out_port_size;
		if (strcmp(hb_ports->name, "out1") == 0)
			out_port_size = out_port_size1;
		else
			out_port_size = out_port_size2;

		if (hb_ports->dir != IN_PORT)
		{
			allocate_more_output_pins(block_node, out_port_size);
			add_output_port_information(block_node, out_port_size);

			char *alias_name = make_full_ref_name(
					instance_name_prefix,
					block->children[0]->types.identifier,
					block->children[1]->children[0]->types.identifier,
					ip_name, -1);

			t_memory_port_sizes *ps = (t_memory_port_sizes *)malloc(sizeof(t_memory_port_sizes));
			ps->size = out_port_size;
			ps->name = alias_name;
			memory_port_size_list = insert_in_vptr_list(memory_port_size_list, ps);

			/* make the implicit output list and hook up the outputs */
			int j;
			for (j = 0; j < out_port_size; j++)
			{
				char *pin_name = make_full_ref_name(
						instance_name_prefix,
						block->children[0]->types.identifier,
						block->children[1]->children[0]->types.identifier,
						ip_name,
						(out_port_size > 1) ? j : -1
				);


				npin_t *new_pin1 = allocate_npin();
				new_pin1->mapping = make_signal_name(hb_ports->name, -1);
				new_pin1->name = pin_name;
				npin_t *new_pin2 = allocate_npin();
				nnet_t *new_net = allocate_nnet();
				new_net->name = hb_ports->name;
				/* hook the output pin into the node */
				add_output_pin_to_node(block_node, new_pin1, current_out_idx + j);
				/* hook up new pin 1 into the new net */
				add_driver_pin_to_net(new_net, new_pin1);
				/* hook up the new pin 2 to this new net */
				add_fanout_pin_to_net(new_net, new_pin2);

				/* add the new_pin 2 to the list of outputs */
				add_pin_to_signal_list(return_list, new_pin2);

				/* add the net to the list of inputs */
				long sc_spot = sc_add_string(input_nets_sc, pin_name);
				input_nets_sc->data[sc_spot] = (void*)new_net;
			}
			current_out_idx += j;
		}
	}

	for (i = 0; i < block_list->num_children; i++)
		free_signal_list(in_list[i]);

	dp_memory_list = insert_in_vptr_list(dp_memory_list, block_node);
	block_node->type = MEMORY;

	return return_list;
}

/*--------------------------------------------------------------------------
 * (function: create_single_port_ram_block)
 * 	This function creates a single port ram block node in the netlist 
 *	and hooks up the inputs and outputs.
 *------------------------------------------------------------------------*/
signal_list_t *create_single_port_ram_block(ast_node_t* block, char *instance_name_prefix, t_model* hb_model)
{
	if (!hb_model || !is_ast_sp_ram(block))
		error_message(NETLIST_ERROR, block->line_number, block->file_number, "Error in creating single port ram\n");

	// EDDIE: Uses new enum in ids: RAM (opposed to MEMORY from operation_t previously)
	block->type = RAM;
	signal_list_t *return_list = init_signal_list();
	int current_idx = 0;

	/* create the node */
	nnode_t *block_node = allocate_nnode();
	/* store all of the relevant info */
	block_node->related_ast_node = block;
	block_node->type = HARD_IP;
	ast_node_t *block_instance = block->children[1];
	ast_node_t *block_list = block_instance->children[1];
	block_node->name = hard_node_name(block_node,
			instance_name_prefix,
			block->children[0]->types.identifier,
			block_instance->children[0]->types.identifier
	);
	
	/* Declare the hard block as used for the blif generation */
	hb_model->used = 1;

	/* Need to do a sanity check to make sure ports line up */
	ast_node_t *block_connect;
	char *ip_name = NULL;
	int i;
	t_model_ports *hb_ports;
	for (i = 0; i < block_list->num_children; i++)
	{
		block_connect = block_list->children[i];
		ip_name = block_connect->children[0]->types.identifier;
		hb_ports = hb_model->inputs;

		while (hb_ports && strcmp(hb_ports->name, ip_name))
			hb_ports = hb_ports->next;

		if (!hb_ports)
		{
			hb_ports = hb_model->outputs;
			while (hb_ports && strcmp(hb_ports->name, ip_name))
				hb_ports = hb_ports->next;
		}

		if (!hb_ports)
			error_message(NETLIST_ERROR, block->line_number, block->file_number, "Non-existant port %s in hard block %s\n", ip_name, block->children[0]->types.identifier);

		/* Link the signal to the port definition */
		block_connect->children[1]->hb_port = (void *)hb_ports;
	}

	/* Need to make sure ALL ports are defined */
	hb_ports = hb_model->inputs;
	i = 0;
	while (hb_ports)
	{
		i++;
		hb_ports = hb_ports->next;
	}

	hb_ports = hb_model->outputs;
	while (hb_ports)
	{
		i++;
		hb_ports = hb_ports->next;
	}

	if (i != block_list->num_children)
		error_message(NETLIST_ERROR, block->line_number, block->file_number, "Not all ports defined in hard block %s\n", ip_name);

	signal_list_t **in_list = (signal_list_t **)malloc(sizeof(signal_list_t *)*block_list->num_children);
	int out_port_size = 0;
	for (i = 0; i < block_list->num_children; i++)
	{
		int port_size;
		ast_node_t *block_port_connect;

		in_list[i] = NULL;
		block_connect = block_list->children[i]->children[0];
		block_port_connect = block_list->children[i]->children[1];
		hb_ports = (t_model_ports *)block_list->children[i]->children[1]->hb_port;
		char *ip_name = block_connect->types.identifier;

		if (hb_ports->dir == IN_PORT)
		{
			/* Create the pins for port if needed */
			in_list[i] = create_pins(block_port_connect, NULL, instance_name_prefix);
			port_size = in_list[i]->count;
			if (strcmp(hb_ports->name, "data") == 0)
				out_port_size = port_size;

			int j;
			for (j = 0; j < port_size; j++)
				in_list[i]->pins[j]->mapping = ip_name;

			/* allocate the pins needed */
			allocate_more_input_pins(block_node, port_size);
			/* record this port size */
			add_input_port_information(block_node, port_size);

			/* hookup the input pins */
			hookup_hb_input_pins_from_signal_list(block_node, current_idx, in_list[i], 0, port_size, verilog_netlist);

			/* Name any grounded ports in the block mapping */
			for (j = port_size; j < port_size; j++)
				block_node->input_pins[current_idx+j]->mapping = strdup(ip_name);
			current_idx += port_size;
		}
	}

	int current_out_idx = 0;
	for (i = 0; i < block_list->num_children; i++)
	{
		block_connect = block_list->children[i]->children[0];
		hb_ports = (t_model_ports *)block_list->children[i]->children[1]->hb_port;
		char *ip_name = block_connect->types.identifier;

		if (hb_ports->dir != IN_PORT)
		{
			allocate_more_output_pins(block_node, out_port_size);
			add_output_port_information(block_node, out_port_size);

			char *alias_name = make_full_ref_name(
					instance_name_prefix,
					block->children[0]->types.identifier,
					block->children[1]->children[0]->types.identifier,
					ip_name, -1
			);
			t_memory_port_sizes *ps = (t_memory_port_sizes *)malloc(sizeof(t_memory_port_sizes));
			ps->size = out_port_size;
			ps->name = alias_name;
			memory_port_size_list = insert_in_vptr_list(memory_port_size_list, ps);

			/* make the implicit output list and hook up the outputs */
			int j;
			for (j = 0; j < out_port_size; j++)
			{
				char *pin_name = make_full_ref_name(instance_name_prefix,
					block->children[0]->types.identifier,
					block->children[1]->children[0]->types.identifier,
					ip_name,
					(out_port_size > 1) ? j : -1
				);

				npin_t *new_pin1 = allocate_npin();
				new_pin1->mapping = make_signal_name(hb_ports->name, -1);
				new_pin1->name = pin_name;
				npin_t *new_pin2 = allocate_npin();

				nnet_t *new_net = allocate_nnet();
				new_net->name = hb_ports->name;
				/* hook the output pin into the node */
				add_output_pin_to_node(block_node, new_pin1, current_out_idx + j);
				/* hook up new pin 1 into the new net */
				add_driver_pin_to_net(new_net, new_pin1);
				/* hook up the new pin 2 to this new net */
				add_fanout_pin_to_net(new_net, new_pin2);

				/* add the new_pin 2 to the list of outputs */
				add_pin_to_signal_list(return_list, new_pin2);

				/* add the net to the list of inputs */
				long sc_spot = sc_add_string(input_nets_sc, pin_name);
				input_nets_sc->data[sc_spot] = (void*)new_net;
			}
			current_out_idx += j;
		}
	}

	for (i = 0; i < block_list->num_children; i++)
	{
		free_signal_list(in_list[i]);
	}

	sp_memory_list = insert_in_vptr_list(sp_memory_list, block_node);
	block_node->type = MEMORY;
	block->net_node = block_node;

	return return_list;
}

/*
 * Creates an architecture independent memory block which will be mapped
 * to soft logic during the partial map.
 */
signal_list_t *create_soft_single_port_ram_block(ast_node_t* block, char *instance_name_prefix)
{
	char *identifier = block->children[0]->types.identifier;

	if (!is_ast_sp_ram(block))
		error_message(NETLIST_ERROR, block->line_number, block->file_number, "Error in creating soft single port ram\n");

	block->type = RAM;

	// create the node
	nnode_t *block_node = allocate_nnode();
	// store all of the relevant info
	block_node->related_ast_node = block;
	block_node->type = HARD_IP;
	ast_node_t *block_instance = block->children[1];
	ast_node_t *block_list = block_instance->children[1];
	block_node->name = hard_node_name(block_node,
			instance_name_prefix,
			identifier,
			block_instance->children[0]->types.identifier
	);

	int i;
	signal_list_t **in_list = (signal_list_t **)malloc(sizeof(signal_list_t *)*block_list->num_children);
	int out_port_size = 0;
	int current_idx = 0;
	for (i = 0; i < block_list->num_children; i++)
	{
		in_list[i] = NULL;
		ast_node_t *block_connect = block_list->children[i]->children[0];
		ast_node_t *block_port_connect = block_list->children[i]->children[1];

		char *ip_name = block_connect->types.identifier;

		if (strcmp(ip_name, "out"))
		{
			// Create the pins for port if needed
			in_list[i] = create_pins(block_port_connect, NULL, instance_name_prefix);
			int port_size = in_list[i]->count;
			if (!strcmp(ip_name, "data"))
				out_port_size = port_size;

			int j;
			for (j = 0; j < port_size; j++)
				in_list[i]->pins[j]->mapping = ip_name;

			// allocate the pins needed
			allocate_more_input_pins(block_node, port_size);

			// record this port size
			add_input_port_information(block_node, port_size);

			// hookup the input pins
			hookup_hb_input_pins_from_signal_list(block_node, current_idx, in_list[i], 0, port_size, verilog_netlist);

			// Name any grounded ports in the block mapping
			for (j = port_size; j < port_size; j++)
				block_node->input_pins[current_idx+j]->mapping = strdup(ip_name);
			current_idx += port_size;
		}
	}

	int current_out_idx = 0;
	signal_list_t *return_list = init_signal_list();
	for (i = 0; i < block_list->num_children; i++)
	{
		ast_node_t *block_connect = block_list->children[i]->children[0];
		char *ip_name = block_connect->types.identifier;

		if (!strcmp(ip_name, "out"))
		{
			allocate_more_output_pins(block_node, out_port_size);
			add_output_port_information(block_node, out_port_size);

			char *alias_name = make_full_ref_name(
					instance_name_prefix,
					identifier,
					block->children[1]->children[0]->types.identifier,
					block_connect->types.identifier, -1
			);
			t_memory_port_sizes *ps = (t_memory_port_sizes *)malloc(sizeof(t_memory_port_sizes));
			ps->size = out_port_size;
			ps->name = alias_name;
			memory_port_size_list = insert_in_vptr_list(memory_port_size_list, ps);

			// make the implicit output list and hook up the outputs
			int j;
			for (j = 0; j < out_port_size; j++)
			{
				char *pin_name;
				if (out_port_size > 1)
				{
					pin_name = make_full_ref_name(instance_name_prefix,
							identifier,
							block->children[1]->children[0]->types.identifier,
							block_connect->types.identifier, j
					);
				}
				else
				{
					pin_name = make_full_ref_name(
							instance_name_prefix,
							identifier,
							block->children[1]->children[0]->types.identifier,
							block_connect->types.identifier, -1
					);
				}

				npin_t *new_pin1 = allocate_npin();
				new_pin1->mapping = ip_name;
				new_pin1->name = pin_name;

				npin_t *new_pin2 = allocate_npin();

				nnet_t *new_net = allocate_nnet();
				new_net->name = ip_name;

				// hook the output pin into the node
				add_output_pin_to_node(block_node, new_pin1, current_out_idx + j);
				// hook up new pin 1 into the new net
				add_driver_pin_to_net(new_net, new_pin1);
				// hook up the new pin 2 to this new net
				add_fanout_pin_to_net(new_net, new_pin2);

				// add the new_pin 2 to the list of outputs
				add_pin_to_signal_list(return_list, new_pin2);

				// add the net to the list of inputs
				long sc_spot = sc_add_string(input_nets_sc, pin_name);
				input_nets_sc->data[sc_spot] = (void*)new_net;
			}
			current_out_idx += j;
		}
	}

	for (i = 0; i < block_list->num_children; i++)
		free_signal_list(in_list[i]);

	free(in_list);

	block_node->type = MEMORY;
	block->net_node = block_node;

	return return_list;
}

/*
 * Creates an architecture independent memory block which will be mapped
 * to soft logic during the partial map.
 */
signal_list_t *create_soft_dual_port_ram_block(ast_node_t* block, char *instance_name_prefix)
{
	char *identifier = block->children[0]->types.identifier;
	char *instance_name = block->children[1]->children[0]->types.identifier;

	if (!is_ast_dp_ram(block))
		error_message(NETLIST_ERROR, block->line_number, block->file_number, "Error in creating soft dual port ram\n");

	block->type = RAM;

	// create the node
	nnode_t *block_node = allocate_nnode();
	// store all of the relevant info
	block_node->related_ast_node = block;
	block_node->type = HARD_IP;
	ast_node_t *block_instance = block->children[1];
	ast_node_t *block_list = block_instance->children[1];
	block_node->name = hard_node_name(block_node,
			instance_name_prefix,
			identifier,
			block_instance->children[0]->types.identifier
	);

	int i;
	signal_list_t **in_list = (signal_list_t **)malloc(sizeof(signal_list_t *)*block_list->num_children);
	int out1_size = 0;
	int out2_size = 0;
	int current_idx = 0;
	for (i = 0; i < block_list->num_children; i++)
	{
		in_list[i] = NULL;
		ast_node_t *block_connect = block_list->children[i]->children[0];
		ast_node_t *block_port_connect = block_list->children[i]->children[1];

		char *ip_name = block_connect->types.identifier;

		int is_output = !strcmp(ip_name, "out1") || !strcmp(ip_name, "out2");

		if (!is_output)
		{
			// Create the pins for port if needed
			in_list[i] = create_pins(block_port_connect, NULL, instance_name_prefix);
			int port_size = in_list[i]->count;

			if (!strcmp(ip_name, "data1"))
				out1_size = port_size;
			else if (!strcmp(ip_name, "data2"))
				out2_size = port_size;

			int j;
			for (j = 0; j < port_size; j++)
				in_list[i]->pins[j]->mapping = ip_name;

			// allocate the pins needed
			allocate_more_input_pins(block_node, port_size);

			// record this port size
			add_input_port_information(block_node, port_size);

			// hookup the input pins
			hookup_hb_input_pins_from_signal_list(block_node, current_idx, in_list[i], 0, port_size, verilog_netlist);

			// Name any grounded ports in the block mapping
			for (j = port_size; j < port_size; j++)
				block_node->input_pins[current_idx+j]->mapping = strdup(ip_name);

			current_idx += port_size;
		}
	}

	oassert(out1_size == out2_size);

	int current_out_idx = 0;
	signal_list_t *return_list = init_signal_list();
	for (i = 0; i < block_list->num_children; i++)
	{
		ast_node_t *block_connect = block_list->children[i]->children[0];
		char *ip_name = block_connect->types.identifier;

		int is_out1 = !strcmp(ip_name, "out1");
		int is_out2 = !strcmp(ip_name, "out2");
		int is_output = is_out1 || is_out2;

		if (is_output)
		{
			char *alias_name = make_full_ref_name(
				instance_name_prefix,
				identifier,
				instance_name,
				ip_name, -1);

			int port_size = is_out1 ? out1_size : out2_size;

			allocate_more_output_pins(block_node, port_size);
			add_output_port_information(block_node, port_size);

			t_memory_port_sizes *ps = (t_memory_port_sizes *)malloc(sizeof(t_memory_port_sizes));
			ps->size = port_size;
			ps->name = strdup(alias_name);
			memory_port_size_list = insert_in_vptr_list(memory_port_size_list, ps);

			// make the implicit output list and hook up the outputs
			int j;
			for (j = 0; j < port_size; j++)
			{
				char *pin_name = make_full_ref_name(
						instance_name_prefix,
						identifier,
						instance_name,
						ip_name,
						(port_size > 1) ? j : -1
				);

				npin_t *new_pin1 = allocate_npin();
				new_pin1->mapping = ip_name;
				new_pin1->name = pin_name;

				npin_t *new_pin2 = allocate_npin();

				nnet_t *new_net = allocate_nnet();
				new_net->name = ip_name;

				// hook the output pin into the node
				add_output_pin_to_node(block_node, new_pin1, current_out_idx + j);
				// hook up new pin 1 into the new net
				add_driver_pin_to_net(new_net, new_pin1);
				// hook up the new pin 2 to this new net
				add_fanout_pin_to_net(new_net, new_pin2);

				// add the new_pin 2 to the list of outputs
				add_pin_to_signal_list(return_list, new_pin2);

				// add the net to the list of inputs
				long sc_spot = sc_add_string(input_nets_sc, pin_name);
				input_nets_sc->data[sc_spot] = (void*)new_net;
			}
			current_out_idx += j;
		}
	}

	for (i = 0; i < block_list->num_children; i++)
		free_signal_list(in_list[i]);

	free(in_list);

	block_node->type = MEMORY;
	block->net_node = block_node;


	return return_list;
}

/*--------------------------------------------------------------------------
 * (function: create_hard_block)
 * 	This function creates a hard block node in the netlist and hooks up the 
 * 	inputs and outputs.
 *------------------------------------------------------------------------*/

signal_list_t *create_hard_block(ast_node_t* block, char *instance_name_prefix)
{
	signal_list_t **in_list, *return_list;
	nnode_t *block_node;
	ast_node_t *block_instance = block->children[1];
	ast_node_t *block_list = block_instance->children[1];
	ast_node_t *block_connect;
	char *ip_name;
	t_model_ports *hb_ports = NULL;
	int i, j, current_idx, current_out_idx;
	int is_mult = 0;
	int mult_size = 0;
	int adder_size = 0;
	int is_adder = 0;

	char *identifier = block->children[0]->types.identifier;

	/* See if the hard block declared is supported by FPGA architecture */
	t_model *hb_model = find_hard_block(identifier);


	/* single_port_ram's are a special case due to splitting */
	if (is_ast_sp_ram(block))
	{
		if (hb_model)
			return create_single_port_ram_block(block, instance_name_prefix, hb_model);
		else
			return create_soft_single_port_ram_block(block, instance_name_prefix);
	}

	/* dual_port_ram's are a special case due to splitting */
	if (is_ast_dp_ram(block))
	{
		if (hb_model)
			return create_dual_port_ram_block(block, instance_name_prefix, hb_model);
		else
			return create_soft_dual_port_ram_block(block, instance_name_prefix);
	}

	if (!hb_model)
	{
		error_message(NETLIST_ERROR, block->line_number, block->file_number,
				"Found Hard Block \"%s\": Not supported by FPGA Architecture\n", identifier);
	}

	/* memory's are a special case due to splitting */
	if (strcmp(hb_model->name, "multiply") == 0)
	{
		is_mult = 1;
	}
	else if(strcmp(hb_model->name, "adder") == 0)
	{
		is_adder = 1;
	}

	return_list = init_signal_list();
	current_idx = 0;
	current_out_idx = 0;

	/* create the node */
	block_node = allocate_nnode();
	/* store all of the relevant info */
	block_node->related_ast_node = block;
	block_node->type = HARD_IP;
	block_node->name = hard_node_name(
			block_node,
			instance_name_prefix,
			block->children[0]->types.identifier,
			block_instance->children[0]->types.identifier
	);
	
	/* Declare the hard block as used for the blif generation */
	hb_model->used = 1;

	/* Need to do a sanity check to make sure ports line up */
	for (i = 0; i < block_list->num_children; i++)
	{
		block_connect = block_list->children[i];
		ip_name = block_connect->children[0]->types.identifier;
		hb_ports = hb_model->inputs;
		while ((hb_ports != NULL) && (strcmp(hb_ports->name, ip_name) != 0))
			hb_ports = hb_ports->next;
		if (hb_ports == NULL)
		{
			hb_ports = hb_model->outputs;
			while ((hb_ports != NULL) && (strcmp(hb_ports->name, ip_name) != 0))
				hb_ports = hb_ports->next;
		}

		if (hb_ports == NULL)
		{
			printf("Non-existant port %s in hard block %s\n", ip_name, block->children[0]->types.identifier);
			block_connect->children[1]->hb_port = NULL;
			oassert(FALSE);
		}

		/* Link the signal to the port definition */
		block_connect->children[1]->hb_port = (void *)hb_ports;
	}

	in_list = (signal_list_t **)malloc(sizeof(signal_list_t *)*block_list->num_children);
	for (i = 0; i < block_list->num_children; i++)
	{
		int port_size;
		ast_node_t *block_port_connect;

		in_list[i] = NULL;
		block_connect = block_list->children[i]->children[0];
		block_port_connect = block_list->children[i]->children[1];
		hb_ports = (t_model_ports *)block_list->children[i]->children[1]->hb_port;
		ip_name = block_connect->types.identifier;

		if (hb_ports->dir == IN_PORT)
		{
			int min_size;

			/* Create the pins for port if needed */
			in_list[i] = create_pins(block_port_connect, NULL, instance_name_prefix);

			/* Only map the required number of pins to match port size */
			port_size = hb_ports->size;
			if (in_list[i]->count < port_size)
				min_size = in_list[i]->count;
			else
				min_size = port_size;

			/* IF a multiplier - leave input size arbitrary with no padding */
			if (is_mult == 1)
			{
				min_size = in_list[i]->count;
				port_size = in_list[i]->count;
				mult_size = mult_size + min_size;
			}

			/* IF a adder -*/
			if (is_adder == 1)
			{
				min_size = in_list[i]->count;
				port_size = in_list[i]->count;
				if(min_size > adder_size)
				{
					adder_size = min_size;
				}
			}

			for (j = 0; j < min_size; j++)
				in_list[i]->pins[j]->mapping = ip_name;

			/* allocate the pins needed */
			allocate_more_input_pins(block_node, port_size);
			/* record this port size */
			add_input_port_information(block_node, port_size);

			/* hookup the input pins */
			hookup_hb_input_pins_from_signal_list(block_node, current_idx, in_list[i], 0, port_size, verilog_netlist);

			/* Name any grounded ports in the block mapping */
			for (j = min_size; j < port_size; j++)
				block_node->input_pins[current_idx+j]->mapping = strdup(ip_name);
			current_idx += port_size;
		}
		else
		{
			/* IF a multiplier - need to process the output pins last!!! */
			/* Makes the assumption that a multiplier has only 1 output */
			if (is_mult == 0 && is_adder == 0)
			{
				allocate_more_output_pins(block_node, hb_ports->size);
				add_output_port_information(block_node, hb_ports->size);

				/* make the implicit output list and hook up the outputs */
				for (j = 0; j < hb_ports->size; j++)
				{
					npin_t *new_pin1;
					npin_t *new_pin2;
					nnet_t *new_net;
					char *pin_name;
					long sc_spot;
	
					if (hb_ports->size > 1)
						pin_name = make_full_ref_name(block_node->name, NULL, NULL, hb_ports->name, j);
					else
						pin_name = make_full_ref_name(block_node->name, NULL, NULL, hb_ports->name, -1);
	
					new_pin1 = allocate_npin();
					new_pin1->mapping = make_signal_name(hb_ports->name, -1);

					new_pin1->name = pin_name;
					new_pin2 = allocate_npin();
					new_net = allocate_nnet();
					new_net->name = hb_ports->name;
					/* hook the output pin into the node */
					add_output_pin_to_node(block_node, new_pin1, current_out_idx + j);
					/* hook up new pin 1 into the new net */
					add_driver_pin_to_net(new_net, new_pin1);
					/* hook up the new pin 2 to this new net */
					add_fanout_pin_to_net(new_net, new_pin2);
	
					/* add the new_pin 2 to the list of outputs */
					add_pin_to_signal_list(return_list, new_pin2);

					/* add the net to the list of inputs */
					sc_spot = sc_add_string(input_nets_sc, pin_name);
					input_nets_sc->data[sc_spot] = (void*)new_net;
				}
				current_out_idx += j;
			}
		}
	}

	/* IF a multiplier - need to process the output pins now */
	/* Size of the output is estimated to be size of the inputs added */
	if (is_mult == 1)
	{
		allocate_more_output_pins(block_node, mult_size);
		add_output_port_information(block_node, mult_size);

		/* make the implicit output list and hook up the outputs */
		for (j = 0; j < mult_size; j++)
		{
			npin_t *new_pin1;
			npin_t *new_pin2;
			nnet_t *new_net;
			char *pin_name;
			long sc_spot;
	
			if (hb_ports->size > 1)
				pin_name = make_full_ref_name(block_node->name, NULL, NULL, hb_ports->name, j);
			else
				pin_name = make_full_ref_name(block_node->name, NULL, NULL, hb_ports->name, -1);

			new_pin1 = allocate_npin();
			if (hb_ports->size > 1)
				new_pin1->mapping = make_signal_name(hb_ports->name, j);
			else
				new_pin1->mapping = make_signal_name(hb_ports->name, -1);

			new_pin2 = allocate_npin();
			new_net = allocate_nnet();
			new_net->name = hb_ports->name;
			/* hook the output pin into the node */
			add_output_pin_to_node(block_node, new_pin1, current_out_idx + j);
			/* hook up new pin 1 into the new net */
			add_driver_pin_to_net(new_net, new_pin1);
			/* hook up the new pin 2 to this new net */
			add_fanout_pin_to_net(new_net, new_pin2);
	
			/* add the new_pin 2 to the list of outputs */
			add_pin_to_signal_list(return_list, new_pin2);

			/* add the net to the list of inputs */
			sc_spot = sc_add_string(input_nets_sc, pin_name);
			input_nets_sc->data[sc_spot] = (void*)new_net;
		}
		current_out_idx += j;
	}
	/* IF a multiplier - need to process the output pins now */
	else if (is_adder == 1)
	{
		/*adder_size is the size of sumout*/
		allocate_more_output_pins(block_node, adder_size + 1);
		add_output_port_information(block_node, adder_size + 1);


		for (j = 0; j < adder_size + 1; j++)
		{
			npin_t *new_pin1;
			npin_t *new_pin2;
			nnet_t *new_net;
			char *pin_name;
			long sc_spot;

			new_pin1 = allocate_npin();
			new_pin2 = allocate_npin();
			new_net = allocate_nnet();
			/*For sumout - make the implicit output list and hook up the outputs */
			if(j < adder_size)
			{
				if (adder_size > 1)
				{
					pin_name = make_full_ref_name(block_node->name, NULL, NULL, hb_ports->name, j);
					new_pin1->mapping = make_signal_name(hb_ports->name, j);
				}
				else
				{
					pin_name = make_full_ref_name(block_node->name, NULL, NULL, hb_ports->name, -1);
					new_pin1->mapping = make_signal_name(hb_ports->name, -1);
				}
				new_net->name = hb_ports->name;
			}
			/*For cout - make the implicit output list and hook up the outputs */
			else
			{
				pin_name = make_full_ref_name(block_node->name, NULL, NULL, hb_ports->next->name, -1);
				new_pin1->mapping = make_signal_name(hb_ports->next->name, -1);
				new_net->name = hb_ports->next->name;
			}

			/* hook the output pin into the node */
			add_output_pin_to_node(block_node, new_pin1, current_out_idx + j);
			/* hook up new pin 1 into the new net */
			add_driver_pin_to_net(new_net, new_pin1);
			/* hook up the new pin 2 to this new net */
			add_fanout_pin_to_net(new_net, new_pin2);

			/* add the new_pin 2 to the list of outputs */
			add_pin_to_signal_list(return_list, new_pin2);

			/* add the net to the list of inputs */
			sc_spot = sc_add_string(input_nets_sc, pin_name);
			input_nets_sc->data[sc_spot] = (void*)new_net;
		}
		current_out_idx += j;
	}

	for (i = 0; i < block_list->num_children; i++)
	{
		free_signal_list(in_list[i]);
	}

	/* Add multiplier to list for later splitting and optimizing */
	if (is_mult == 1)
		mult_list = insert_in_vptr_list(mult_list, block_node);
	/* Add adder to list for later splitting and optimizing */
	if (is_adder == 1)
		add_list = insert_in_vptr_list(add_list, block_node);

	return return_list;
}
#endif
