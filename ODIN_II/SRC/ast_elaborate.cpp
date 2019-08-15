	/*
Copyright (c) 2009 Peter Andrew Jamieson (jamieson.peter@gmail.com)

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
#include <math.h>
#include "odin_types.h"
#include "adders.h"
#include "ast_util.h"
#include "odin_util.h"
#include "ast_elaborate.h"
#include "ast_loop_unroll.h"
#include "hard_blocks.h"
#include "memories.h"
#include "multipliers.h"
#include "odin_util.h"
#include "parse_making_ast.h"
#include "verilog_bison.h"
#include "netlist_create_from_ast.h"
#include "netlist_utils.h"
#include "ctype.h"
#include "vtr_util.h"
#include "vtr_memory.h"
#include <string>
#include <iostream>
#include <vector>
#include <stack>

// #define read_node  1
// #define write_node 2

// #define e_data  1
// #define e_operation 2
// #define e_variable 3

// #define N 64
// #define Max_size 64

// long count_id = 0;
// long count_assign = 0;
// long count;
// long count_write;
//enode *head, *p;

// void reduce_assignment_expression();
// void reduce_assignment_expression(ast_node_t *ast_module);
// void find_assign_node(ast_node_t *node, std::vector<ast_node_t *> list, char *module_name);

// struct enode
// {
// 	struct
// 	{
// 		short operation;
// 		int data;
// 		std::string variable;
// 	}type;

// 	int id;
// 	int flag;
// 	int priority;
// 	struct enode *next;
// 	struct enode *pre;

// };

// void record_tree_info(ast_node_t *node);
// void store_exp_list(ast_node_t *node);
// void create_enode(ast_node_t *node);
// bool simplify_expression();
// bool adjoin_constant();
// enode *replace_enode(int data, enode *t, int mark);
// bool combine_constant();
// bool delete_continuous_multiply();
// void construct_new_tree(enode *tail, ast_node_t *node, int line_num, int file_num);
// void reduce_enode_list();
// enode *find_tail(enode *node);
// int check_exp_list(enode *tail);
// void create_ast_node(enode *temp, ast_node_t *node, int line_num, int file_num);
// void create_op_node(ast_node_t *node, enode *temp, int line_num, int file_num);
// void free_exp_list();
// void change_exp_list(enode *beign, enode *end, enode *s, int flag);
// enode *copy_enode_list(enode *new_head, enode *begin, enode *end);
// void copy_enode(enode *node, enode *new_node);
// bool check_tree_operation(ast_node_t *node);
// void check_operation(enode *begin, enode *end);
// bool check_mult_bracket(std::vector<int> list);

ast_node_t *finalize_ast(ast_node_t *node, ast_node_t *parent, STRING_CACHE_LIST *local_string_cache_list, bool is_generate_region, bool fold_expressions, ast_node_t ***unassigned_defparams, int *num_defparams);
ast_node_t *reduce_expressions(ast_node_t *node, STRING_CACHE_LIST *local_string_cache_list, long *max_size, long assignment_size);

void update_string_caches(STRING_CACHE_LIST *local_string_cache_list);
void update_instance_parameter_table(ast_node_t *instance, STRING_CACHE *instance_param_table_sc);

void convert_2D_to_1D_array(ast_node_t **var_declare);
void convert_2D_to_1D_array_ref(ast_node_t **node, STRING_CACHE_LIST *local_string_cache_list);
char *make_chunk_size_name(char *instance_name_prefix, char *array_name);
ast_node_t *get_chunk_size_node(char *instance_name_prefix, char *array_name, STRING_CACHE_LIST *local_string_cache_list);

bool verify_terminal(ast_node_t *top, ast_node_t *iterator);
void verify_genvars(ast_node_t *node, STRING_CACHE_LIST *local_string_cache_list, char ***other_genvars, int num_genvars);

ast_node_t *look_for_matching_hard_block(ast_node_t *node, char *hard_block_name, STRING_CACHE_LIST *local_string_cache_list);
ast_node_t *look_for_matching_soft_logic(ast_node_t *node, char *hard_block_name);


int simplify_ast_module(ast_node_t **ast_module, STRING_CACHE_LIST *local_string_cache_list)
{
	/* resolve constant expressions */
	update_string_caches(local_string_cache_list);
	bool is_module = (*ast_module)->type == MODULE ? true : false;

	ast_node_t **unassigned_defparams = NULL;
	*ast_module = finalize_ast(*ast_module, NULL, local_string_cache_list, is_module, true, &unassigned_defparams, NULL);
	*ast_module = reduce_expressions(*ast_module, local_string_cache_list, NULL, 0);

	return 1;
}

/* ---------------------------------------------------------------------------------------------------
 * (function: finalize_ast)
 * 
 * This function resolves all parameters, folds constant expressions that can be self-determined
 * (e.g. array/range refs, generate constructs), resolves module instances into module copies and hard
 * blocks (while updating their parameter tables with defparams), and adds symbols that it finds to 
 * the symbol table.
 * Basic sanity checks also happen here that weren't caught during parsing, e.g. checking port
 * lists.
 * --------------------------------------------------------------------------------------------------- */
ast_node_t *finalize_ast(ast_node_t *node, ast_node_t *parent, STRING_CACHE_LIST *local_string_cache_list, bool is_generate_region, bool fold_expressions, ast_node_t ***unassigned_defparams, int *num_defparams)
{
	short *child_skip_list = NULL; // list of children not to traverse into
	short skip_children = false; // skips the DFS completely if true

	STRING_CACHE *local_param_table_sc = local_string_cache_list->local_param_table_sc;
	STRING_CACHE *local_symbol_table_sc = local_string_cache_list->local_symbol_table_sc;

	if (node)
	{
		if (node->num_children > 0)
		{
			child_skip_list = (short*)vtr::calloc(node->num_children, sizeof(short));
		}

		/* pre-amble */
		switch (node->type)
		{
			case MODULE:
			{
				oassert(child_skip_list);

				// skip identifier
				child_skip_list[0] = true;

				num_defparams = (int *)vtr::calloc(1, sizeof(int));
				*num_defparams = 0;

				break;
			}
			case FUNCTION:
			{
				if (parent != NULL)
				{
					skip_children = true;
				}
				break;
			}
			case FUNCTION_INSTANCE:
			{
				// check ports
				ast_node_t *connect_list = node->children[1]->children[1];
				bool is_ordered_list;
				if (connect_list->children[1]->children[0]) // skip first connection
				{
					is_ordered_list = false; // name was specified
				}
				else
				{
					is_ordered_list = true;
				}

				for (int i = 1; i < connect_list->num_children; i++)
				{
					if ((connect_list->children[i]->children[0] && is_ordered_list)
						|| (!connect_list->children[i]->children[0] && !is_ordered_list))
					{
						error_message(PARSE_ERROR, node->line_number, node->file_number, 
								"%s", "Cannot mix port connections by name and port connections by ordered list\n");
					}
				}

				skip_children = true;

				break;
			}
			case MODULE_ITEMS:
			{
				/* look in the string cache for in-line continuous assignments */
				ast_node_t **local_symbol_table = local_string_cache_list->local_symbol_table;
				int num_local_symbol_table = local_string_cache_list->num_local_symbol_table;

				for (int i = 0; i < num_local_symbol_table; i++)
				{
					ast_node_t *var_declare = local_symbol_table[i];
					if (var_declare->types.variable.is_wire && var_declare->children[5] != NULL)
					{
						/* in-line assignment; split into its own continuous assignment */
						ast_node_t *id = ast_node_copy(var_declare->children[0]);
						ast_node_t *val = var_declare->children[5];
						var_declare->children[5] = NULL;

						ast_node_t *blocking_node = newBlocking(id, val, var_declare->line_number);
						ast_node_t *assign_node = newList(ASSIGN, blocking_node);

						add_child_to_node(node, assign_node);
					}
				}

				break;
			}
			case MODULE_INSTANCE:
			{
				/* flip hard blocks */

				if (node->num_children == 1)
				{
					// check ports
					ast_node_t *connect_list = node->children[0]->children[1]->children[1];
					bool is_ordered_list;
					if (connect_list->children[0]->children[0])
					{
						is_ordered_list = false; // name was specified
					}
					else
					{
						is_ordered_list = true;
					}

					for (int i = 1; i < connect_list->num_children; i++)
					{
						if ((connect_list->children[i]->children[0] && is_ordered_list)
							|| (!connect_list->children[i]->children[0] && !is_ordered_list))
						{
							error_message(PARSE_ERROR, node->line_number, node->file_number, 
									"%s", "Cannot mix port connections by name and port connections by ordered list\n");
						}
					}

					char *module_ref_name = node->children[0]->children[0]->types.identifier;
					long sc_spot = sc_lookup_string(hard_block_names, module_ref_name);

					/* TODO: strcmp on "multiply", "adder" for soft logic implementation? */
					if
					(
						sc_spot != -1
						|| !strcmp(module_ref_name, SINGLE_PORT_RAM_string)
						|| !strcmp(module_ref_name, DUAL_PORT_RAM_string)
					)
					{
						ast_node_t *hb_node = look_for_matching_hard_block(node->children[0], module_ref_name, local_string_cache_list);
						if (hb_node != node->children[0])
						{
							free_whole_tree(node);
							node = hb_node;

							child_skip_list = (short *)vtr::realloc(child_skip_list, sizeof(short)*node->num_children);
							child_skip_list[1] = false;
							break;
						}
					}
				}
				break;
			}
			case VAR_DECLARE:
			{
				oassert(child_skip_list);
				child_skip_list[0] = true;
				break;
			}
			case IDENTIFIERS:
			{
				if (local_param_table_sc != NULL && node->types.identifier)
				{
					long sc_spot = sc_lookup_string(local_param_table_sc, node->types.identifier);
					if (sc_spot != -1)
					{
						ast_node_t *newNode = ast_node_deep_copy((ast_node_t *)local_param_table_sc->data[sc_spot]);

						if (newNode->type != NUMBERS)
						{
							newNode = finalize_ast(newNode, parent, local_string_cache_list, is_generate_region, true, unassigned_defparams, num_defparams);
							oassert(newNode->type == NUMBERS);

							/* this forces parameter values as unsigned, since we don't currently support signed keyword...
							must be changed once support is added */
							VNumber *temp = newNode->types.vnumber;
							VNumber *to_unsigned = new VNumber(V_UNSIGNED(*temp));
							newNode->types.vnumber = to_unsigned;
							delete temp;
							
							if (newNode->type != NUMBERS)
							{
								error_message(NETLIST_ERROR, node->line_number, node->file_number, "Parameter %s is not a constant expression\n", node->types.identifier);
							}
						}

						node = free_whole_tree(node);
						node = newNode;
					}
				}
				break;
			}
			case FOR:
			{
				oassert(child_skip_list);

				// look ahead for parameters in loop conditions
				node->children[0] = finalize_ast(node->children[0], node, local_string_cache_list, is_generate_region, true, unassigned_defparams, num_defparams);
				node->children[1] = finalize_ast(node->children[1], node, local_string_cache_list, is_generate_region, true, unassigned_defparams, num_defparams);
				node->children[2] = finalize_ast(node->children[2], node, local_string_cache_list, is_generate_region, true, unassigned_defparams, num_defparams);
				
				// update skip list
				child_skip_list[0] = true;
				child_skip_list[1] = true;
				child_skip_list[2] = true;

				// if this is a loop generate construct, verify constant expressions
				if (is_generate_region)
				{
					ast_node_t *iterator = NULL;
					ast_node_t *initial = node->children[0];
					ast_node_t *compare_expression = node->children[1];
					ast_node_t *terminal = node->children[2];

					char **genvar_list = NULL;
					verify_genvars(node, local_string_cache_list, &genvar_list, 0);
					if (genvar_list) vtr::free(genvar_list);

					long sc_spot = sc_lookup_string(local_symbol_table_sc, initial->children[0]->types.identifier);
					oassert(sc_spot > -1);

					iterator = (ast_node_t *)local_symbol_table_sc->data[sc_spot];

					if ( !(node_is_constant(initial->children[1]))
						|| !(node_is_constant(compare_expression->children[1]))
						|| !(verify_terminal(terminal->children[1], iterator))
					)
					{
						error_message(NETLIST_ERROR, node->line_number, node->file_number, 
							"%s","Loop generate construct conditions must be constant expressions");
					}
				}
				
				node = unroll_for_loop(node, parent, local_string_cache_list);
				break;
			}
			case IF_Q:
			case IF:
			{
				ast_node_t *to_return = NULL;

				node->children[0] = finalize_ast(node->children[0], node, local_string_cache_list, is_generate_region, true, unassigned_defparams, num_defparams);
				ast_node_t *child_condition = node->children[0];
				if(node_is_constant(child_condition))
				{
					VNumber condition = *(child_condition->types.vnumber);

					if(V_TRUE(condition))
					{
						to_return = node->children[1];
						node->children[1] = NULL;
					}
					else if(V_FALSE(condition))
					{
						to_return = node->children[2];
						node->children[2] = NULL;
					}
					else if (node->type == IF && is_generate_region)
					{
						error_message(NETLIST_ERROR, node->line_number, node->file_number, 
							"%s","Could not resolve conditional generate construct");
					}
					// otherwise we keep it as is to build the circuitry
				}
				else if (node->type == IF && is_generate_region)
				{
					error_message(NETLIST_ERROR, node->line_number, node->file_number, 
						"%s","Could not resolve conditional generate construct");
				}

				if (to_return)
				{
					free_whole_tree(node);
					node = to_return;
				}
				break;
			}
			case CASE:
			{
				ast_node_t *to_return = NULL;

				node->children[0] = finalize_ast(node->children[0], node, local_string_cache_list, is_generate_region, true, unassigned_defparams, num_defparams);
				ast_node_t *child_condition = node->children[0];
				if(node_is_constant(child_condition))
				{
					ast_node_t *case_list = node->children[1];
					for (int i = 0; i < case_list->num_children; i++)
					{
						ast_node_t *item = case_list->children[i];
						if (i == case_list->num_children - 1 && item->type == CASE_DEFAULT)
						{
							to_return = item->children[0];
							item->children[0] = NULL;
						}
						else
						{
							if (item->type != CASE_ITEM)
							{
								error_message(NETLIST_ERROR, node->line_number, node->file_number, 
									"%s","Default case must only be the last case");
							}

							item->children[0] = finalize_ast(item->children[0], item, local_string_cache_list, is_generate_region, true, unassigned_defparams, num_defparams);
							if (node_is_constant(item->children[0]))
							{
								VNumber eval = V_CASE_EQUAL(*(child_condition->types.vnumber), *(item->children[0]->types.vnumber));
								if (V_TRUE(eval))
								{
									to_return = item->children[1];
									item->children[1] = NULL;
									break;
								}
							}
							else if (is_generate_region)
							{
								error_message(NETLIST_ERROR, node->line_number, node->file_number, 
									"%s","Could not resolve conditional generate construct");
							}
							else
							{
								/* encountered non-constant item - don't continue searching */
								break;
							}
						}
					}

					if (to_return)
					{
						free_whole_tree(node);
						node = to_return;
					}
					else if (is_generate_region)
					{
						/* no case */
						error_message(NETLIST_ERROR, node->line_number, node->file_number, 
							"%s","Could not resolve conditional generate construct");
					}
				}
				else if (is_generate_region)
				{
					error_message(NETLIST_ERROR, node->line_number, node->file_number, 
						"%s","Could not resolve conditional generate construct");
				}

				break;
			}
			case WHILE:
			{
				break;
			}
			case ALWAYS: // fallthrough
			case INITIALS:
			{
				is_generate_region = false;
				break;
			}
			case BLOCKING_STATEMENT:
			case NON_BLOCKING_STATEMENT:
			{
				/* fill out parameters and try to resolv self-determined expressions */
				if (node->children[1]->type != FUNCTION_INSTANCE)
				{
					node->children[0] = finalize_ast(node->children[0], node, local_string_cache_list, is_generate_region, true, unassigned_defparams, num_defparams);
					
					if (node->children[1]->type != NUMBERS)
					{
						node->children[1] = finalize_ast(node->children[1], node, local_string_cache_list, is_generate_region, false, unassigned_defparams, num_defparams);
						if (node->children[1] == NULL)
						{
							/* resulting from replication of zero */
							error_message(NETLIST_ERROR, node->line_number, node->file_number, 
								"%s","Cannot perform assignment with nonexistent value");
						}
					}

					skip_children = true;
				}
				break;
			}
			case REPLICATE:
			{
				node->children[0] = finalize_ast(node->children[0], node, local_string_cache_list, is_generate_region, true, unassigned_defparams, num_defparams);
				if (!(node_is_constant(node->children[0])))
				{
					error_message(NETLIST_ERROR, node->line_number, node->file_number, 
						"%s","Replication constant must be a constant expression");
				}
				else if (node->children[0]->types.vnumber->is_dont_care_string())
				{
					error_message(NETLIST_ERROR, node->line_number, node->file_number, 
						"%s","Replication constant cannot contain x or z");
				}

				ast_node_t *new_node = NULL;
				int64_t value = node->children[0]->types.vnumber->get_value();
				if (value > 0)
				{
					new_node = create_node_w_type(CONCATENATE, node->line_number, node->file_number);				
					for (size_t i = 0; i < value; i++)
					{
						add_child_to_node(new_node, ast_node_deep_copy(node->children[1]));
					}
				}

				node->children[1] = free_whole_tree(node->children[1]);
				node->children[1] = NULL;
				node->num_children--;

				node->children[0] = free_whole_tree(node->children[0]);
				node->children[0] = new_node;

				break;
			}
			case RANGE_REF:
			case ARRAY_REF:
			{
				/* these need to be folded if they are binary expressions... regardless of fold_expressions */
				for (int i = 1; i < node->num_children; i++)
				{
					node->children[i] = finalize_ast(node->children[i], node, local_string_cache_list, is_generate_region, true, unassigned_defparams, num_defparams);
				}
				skip_children = true;
				break;
			}
			default:
			{
				break;
			}
		}

		if (child_skip_list && node->num_children > 0 && skip_children == false)
		{
			/* use while loop and boolean to prevent optimizations
				since number of children may change during recursion */

			bool done = false;
			int i = 0;

			/* traverse all the children */
			while (done == false)
			{
				int num_original_children = node->num_children;
				if (child_skip_list[i] == false)
				{
					/* recurse */
					ast_node_t *old_child = node->children[i];
					ast_node_t *new_child = finalize_ast(old_child, node, local_string_cache_list, is_generate_region, fold_expressions, unassigned_defparams, num_defparams);
					node->children[i] = new_child;

					if (old_child != new_child)
					{
						/* recurse on this child again in case it can be further reduced 
							(e.g. resolved generate constructs) */
						i--;
					}
				}

				if (num_original_children != node->num_children)
				{
					child_skip_list = (short *)vtr::realloc(child_skip_list, sizeof(short)*node->num_children);
					for (int j = num_original_children; j < node->num_children; j++)
					{
						child_skip_list[j] = false;
					}
				}

				i++;				
				if (i >= node->num_children)
				{
					done = true;
				}
			}
		}

		/* post-amble */
		switch (node->type)
		{
			case MODULE:
			{
				int i;

				/* update module instance parameters */
				if (*unassigned_defparams)
				{
					for (i = 0; i < *num_defparams; i++)
					{
						if((*unassigned_defparams)[i] != NULL)
						{
							warning_message(NETLIST_ERROR, (*unassigned_defparams)[i]->line_number, (*unassigned_defparams)[i]->file_number,
									"Can't find instance name %s\n", (*unassigned_defparams)[i]->types.identifier);
						}
					}
					vtr::free((*unassigned_defparams));
				}

				vtr::free(num_defparams);

				for (i = 0; i < node->types.module.size_module_instantiations; i++)
				{
				
					/* make the stringed up module instance name - instance name is
					* MODULE_INSTANCE->MODULE_NAMED_INSTANCE(child[1])->IDENTIFIER(child[0]).
					* module name is MODULE_INSTANCE->IDENTIFIER(child[0])
					*/
					ast_node_t *temp_instance = node->types.module.module_instantiations_instance[i];
					char *instance_name_prefix = local_string_cache_list->instance_name_prefix;

					char *temp_instance_name = make_full_ref_name(instance_name_prefix,
						temp_instance->children[0]->types.identifier,
						temp_instance->children[1]->children[0]->types.identifier,
						NULL, -1);

					long sc_spot;
					/* lookup the name of the module associated with this instantiated point */
					if ((sc_spot = sc_lookup_string(module_names_to_idx, temp_instance->children[0]->types.identifier)) == -1)
					{
						error_message(NETLIST_ERROR, node->line_number, node->file_number,
								"Can't find module name %s\n", temp_instance->children[0]->types.identifier);
					}

					/* make a unique copy of this module */
					ast_node_t *instance = ast_node_deep_copy((ast_node_t *)module_names_to_idx->data[sc_spot]);
					
					long sc_spot_2 = sc_add_string(module_names_to_idx, temp_instance_name);
					oassert(sc_spot_2 > -1 && module_names_to_idx->data[sc_spot_2] == NULL);
					module_names_to_idx->data[sc_spot_2] = (void *)instance;
					
					ast_modules = (ast_node_t **)vtr::realloc(ast_modules, sizeof(ast_node_t*)*(sc_spot_2 + 1));
					ast_modules[sc_spot_2] = instance;
					
					/* update the parameter table for the instantiated module */
					STRING_CACHE *original_param_table_sc = ((ast_node_t *)module_names_to_idx->data[sc_spot])->types.module.parameter_list;
					instance->types.module.parameter_list = copy_param_table_sc(original_param_table_sc);
					STRING_CACHE *instance_param_table_sc = instance->types.module.parameter_list;

					update_instance_parameter_table(temp_instance, instance_param_table_sc);

					/* create the string cache list for the instantiated module */
					STRING_CACHE_LIST *module_string_cache_list = (STRING_CACHE_LIST*)vtr::calloc(1, sizeof(STRING_CACHE_LIST));
					module_string_cache_list->parent = local_string_cache_list;
					
					int num_children = local_string_cache_list->num_children;
					local_string_cache_list->children = (STRING_CACHE_LIST **)vtr::realloc(local_string_cache_list->children, sizeof(STRING_CACHE_LIST *)*(num_children + 1));
					local_string_cache_list->children[i] = module_string_cache_list;
					local_string_cache_list->num_children++;			
					
					module_string_cache_list->instance_name_prefix = vtr::strdup(temp_instance_name);
					module_string_cache_list->scope_id = vtr::strdup(temp_instance->children[1]->children[0]->types.identifier);
					module_string_cache_list->local_param_table_sc = instance_param_table_sc;

					/* create the symbol table for the instantiated module */
					module_string_cache_list->local_symbol_table_sc = sc_new_string_cache();
					module_string_cache_list->num_local_symbol_table = 0;
					module_string_cache_list->local_symbol_table = NULL;
					create_symbol_table_for_scope(instance->children[2], module_string_cache_list);

					/* elaboration */
					simplify_ast_module(&instance, module_string_cache_list);

					/* clean up */
					vtr::free(temp_instance_name);
				}

				int function_offset = i;
				for (i = 0; i < node->types.function.size_function_instantiations; i++)
				{
				
					/* make the stringed up module instance name - instance name is
					* MODULE_INSTANCE->MODULE_NAMED_INSTANCE(child[1])->IDENTIFIER(child[0]).
					* module name is MODULE_INSTANCE->IDENTIFIER(child[0])
					*/
					ast_node_t *temp_instance = node->types.function.function_instantiations_instance[i];
					char *instance_name_prefix = local_string_cache_list->instance_name_prefix;

					char *temp_instance_name = make_full_ref_name(instance_name_prefix,
						temp_instance->children[0]->types.identifier,
						temp_instance->children[1]->children[0]->types.identifier,
						NULL, -1);

					long sc_spot;
					/* lookup the name of the function associated with this instantiated point */
					if ((sc_spot = sc_lookup_string(module_names_to_idx, temp_instance->children[0]->types.identifier)) == -1)
					{
						error_message(NETLIST_ERROR, node->line_number, node->file_number,
								"Can't find function name %s\n", temp_instance->children[0]->types.identifier);
					}

					/* make a unique copy of this function */
					ast_node_t *instance = ast_node_deep_copy((ast_node_t *)module_names_to_idx->data[sc_spot]);
					
					long sc_spot_2 = sc_add_string(module_names_to_idx, temp_instance_name);
					oassert(sc_spot_2 > -1 && module_names_to_idx->data[sc_spot_2] == NULL);
					module_names_to_idx->data[sc_spot_2] = (void *)instance;
					
					ast_modules = (ast_node_t **)vtr::realloc(ast_modules, sizeof(ast_node_t*)*(sc_spot_2 + 1));
					ast_modules[sc_spot_2] = instance;

					/* create the string cache list for the instantiated function */
					STRING_CACHE_LIST *function_string_cache_list = (STRING_CACHE_LIST*)vtr::calloc(1, sizeof(STRING_CACHE_LIST));
					function_string_cache_list->parent = local_string_cache_list;
					
					int num_children = local_string_cache_list->num_children;
					local_string_cache_list->children = (STRING_CACHE_LIST **)vtr::realloc(local_string_cache_list->children, sizeof(STRING_CACHE_LIST *)*(num_children + 1));
					local_string_cache_list->children[function_offset + i] = function_string_cache_list;
					local_string_cache_list->num_children++;			
					
					function_string_cache_list->instance_name_prefix = vtr::strdup(temp_instance_name);
					function_string_cache_list->scope_id = vtr::strdup(temp_instance->children[1]->children[0]->types.identifier);

					/* create the symbol table for the instantiated module */
					function_string_cache_list->local_symbol_table_sc = sc_new_string_cache();
					function_string_cache_list->num_local_symbol_table = 0;
					function_string_cache_list->local_symbol_table = NULL;
					create_symbol_table_for_scope(instance->children[2], function_string_cache_list);

					/* elaboration */
					simplify_ast_module(&instance, function_string_cache_list);

					/* clean up */
					vtr::free(temp_instance_name);
				}

				break;
			}
			case MODULE_PARAMETER:
			{				
				if (parent->type != MODULE_PARAMETER_LIST)
				{
					char *instance_name = node->types.identifier;
					char *module_ref_name = local_string_cache_list->instance_name_prefix;

					long sc_spot = sc_lookup_string(module_names_to_idx, module_ref_name);
					ast_node_t *module = (ast_node_t *)module_names_to_idx->data[sc_spot];

					int num_instances = module->types.module.size_module_instantiations;
					bool found = false;

					if (num_instances > 0)
					{
						for (int i = 0; i < num_instances && !found; i++)
						{
							ast_node_t *temp_instance_node = module->types.module.module_instantiations_instance[i];
							char *temp_instance_name = temp_instance_node->children[1]->children[0]->types.identifier;
							if (strcmp(instance_name, temp_instance_name) == 0)
							{
								found = true;
								ast_node_t *defparam = ast_node_deep_copy(node);

								if (!temp_instance_node->children[1]->children[2])
								{
									ast_node_t* new_list = create_node_w_type(MODULE_PARAMETER_LIST, temp_instance_node->line_number, current_parse_file);
									temp_instance_node->children[1]->children[2] = new_list;
								}
								ast_node_t *module_param_list = temp_instance_node->children[1]->children[2];
								add_child_to_node(module_param_list, defparam);

								for (int j = 0; j < parent->num_children; j++)
								{
									if (node == parent->children[j])
									{
										remove_child_from_node_at_index(parent, j);
										node = parent->children[j];
										break;
									}
								}

							}
						}
					}

					if (!found)
					{
						ast_node_t *defparam = ast_node_deep_copy(node);
						(*unassigned_defparams) = (ast_node_t **)vtr::realloc((*unassigned_defparams), sizeof(ast_node_t *)*(*num_defparams + 1));
						(*unassigned_defparams)[*num_defparams] = defparam;
						(*num_defparams)++;

						oassert((*unassigned_defparams)[*num_defparams - 1]->types.identifier);

						for (int i = 0; i < parent->num_children; i++)
						{
							if (node == parent->children[i])
							{
								remove_child_from_node_at_index(parent, i);
								node = parent->children[i];
								break;
							}
						}
					}
				}
				
				break;
			}
			case MODULE_INSTANCE:
			{
				/* this should be encountered once generate constructs are resolved, so we don't have
					stray instances that never get instantiated */

				if (node->num_children == 2)
				{
					char *instance_name_prefix = local_string_cache_list->instance_name_prefix;

					long sc_spot = sc_lookup_string(module_names_to_idx, instance_name_prefix);
					ast_node_t *module = (ast_node_t *)module_names_to_idx->data[sc_spot];

					int num_instances = module->types.module.size_module_instantiations;
					char *new_instance_name = node->children[1]->children[0]->types.identifier;
					bool found = false;
					for (int i = 0; i < num_instances; i++)
					{
						ast_node_t *temp_instance_node = module->types.module.module_instantiations_instance[i];
						char *temp_instance_name = temp_instance_node->children[1]->children[0]->types.identifier;
						if (strcmp(new_instance_name, temp_instance_name) == 0)
						{
							found = true;
						}
					}

					if (found) 
					{
						break;
					}

					module->types.module.module_instantiations_instance = (ast_node_t **)vtr::realloc(module->types.module.module_instantiations_instance, sizeof(ast_node_t*)*(num_instances+1));
					module->types.module.module_instantiations_instance[num_instances] = node;
					module->types.module.size_module_instantiations++;

					/* look for defparams that occurred before instance was declared */
					if (num_defparams)
					{
						for (int i = 0; i < *num_defparams; i++)
						{
							if ((*unassigned_defparams)[i] && 
								(strcmp((*unassigned_defparams)[i]->types.identifier, new_instance_name) == 0)
							)
							{
								ast_node_t *new_defparam = (*unassigned_defparams)[i];
								(*unassigned_defparams)[i] = NULL;
								if (!node->children[1]->children[2])
								{
									ast_node_t* new_list = create_node_w_type(MODULE_PARAMETER_LIST, node->line_number, current_parse_file);
									node->children[1]->children[2] = new_list;
								}
								add_child_to_node(node->children[1]->children[2], new_defparam);
							}
						}
					}
				}
				break;
			}
			case VAR_DECLARE:
			{
				// pack 2d array into 1d
				if (node->num_children == 8 
				&& node->children[5] 
				&& node->children[6])
				{
					convert_2D_to_1D_array(&node);
				}
				
				break;
			}
			case BINARY_OPERATION:
			{
				if (node->children[0] == NULL || node->children[1] == NULL)
				{
					/* resulting from replication of zero */
					error_message(NETLIST_ERROR, node->line_number, node->file_number, 
						"%s","Cannot perform operation with nonexistent value");
				}

				/* only fold if size can be self-determined */
				if (fold_expressions)
				{
					ast_node_t *new_node = fold_binary(&node);
					if (node_is_constant(new_node))
					{
						/* resize as needed */
						long child_0_size = node->children[0]->types.vnumber->size();
						long child_1_size = node->children[1]->types.vnumber->size();

						long new_size = (child_0_size > child_1_size) ? child_0_size : child_1_size;

						/* clean up */
						free_resolved_children(node);
													
						change_to_number_node(node, VNumber(*(new_node->types.vnumber), new_size));
					}
					new_node = free_whole_tree(new_node);
				}
				
				break;
			}
			case UNARY_OPERATION:
			{
				if (node->children[0] == NULL)
				{
					/* resulting from replication of zero */
					error_message(NETLIST_ERROR, node->line_number, node->file_number, 
						"%s","Cannot perform operation with nonexistent value");
				}

				/* only fold if size can be self-determined */
				if (fold_expressions)
				{
					ast_node_t *new_node = fold_unary(&node);
					if (node_is_constant(new_node))
					{
						/* clean up */
						free_resolved_children(node);
						
						change_to_number_node(node, VNumber(*(new_node->types.vnumber), new_node->types.vnumber->size()));
					}
					new_node = free_whole_tree(new_node);
				}
				break;
			}
			case REPLICATE:
			{
				/* remove intermediate REPLICATE node */
				ast_node_t *child = node->children[0];
				node->children[0] = NULL;
				node = free_whole_tree(node);
				node = child;
				break;
			}
			case CONCATENATE:
			{
				if (node->num_children > 0)
				{
					size_t current = 0;
					size_t previous = -1;
					bool previous_is_constant = false;

					while (current < node->num_children)
					{
						bool current_is_constant = node_is_constant(node->children[current]);

						if(previous_is_constant && current_is_constant)
						{
							resolve_concat_sizes(node->children[previous], local_string_cache_list);
							resolve_concat_sizes(node->children[current], local_string_cache_list);

							VNumber new_value = V_CONCAT({*(node->children[previous]->types.vnumber), *(node->children[current]->types.vnumber)});

							node->children[current] = free_whole_tree(node->children[current]);

							delete node->children[previous]->types.vnumber;
							node->children[previous]->types.vnumber = new VNumber(new_value);
						}
						else if(node->children[current] != NULL)
						{
							previous += 1;
							previous_is_constant = current_is_constant;
							node->children[previous] = node->children[current];
						}
						current += 1;
					}

					node->num_children = previous+1;

					if (node->num_children == 0)
					{
						// could we simply warn and continue ? any ways we can recover rather than fail?
						/* resulting replication(s) of zero */
						error_message(NETLIST_ERROR, node->line_number, node->file_number, 
							"%s","Cannot concatenate zero bitstrings");
					}

					// node was all constant
					if (node->num_children == 1)
					{
						ast_node_t *tmp = node->children[0];
						node->children[0] = NULL;
						free_whole_tree(node);
						node = tmp;
					}

				}

				break;
			}
			case RANGE_REF:
			{
				bool is_constant_ref = node_is_constant(node->children[1]);
				if (is_constant_ref)
				{
					is_constant_ref = is_constant_ref && !(node->children[1]->types.vnumber->is_dont_care_string());
				}

				is_constant_ref = is_constant_ref && node_is_constant(node->children[2]);
				if (is_constant_ref)
				{
					is_constant_ref = is_constant_ref && !(node->children[2]->types.vnumber->is_dont_care_string());
				}

				if (!is_constant_ref)
				{
					/* note: indexed part-selects (-: and +:) should support non-constant base expressions, 
							 e.g. my_var[some_input+:3] = 4'b0101, but we don't support it right now... */

					error_message(NETLIST_ERROR, node->line_number, node->file_number, 
							"%s","Part-selects can only contain constant expressions");
				}

				break;
			}
			case ARRAY_REF:
			{
				bool is_constant_ref = node_is_constant(node->children[1]);
				if (is_constant_ref)
				{
					is_constant_ref = is_constant_ref && !(node->children[1]->types.vnumber->is_dont_care_string());
				}
				
				if (node->num_children == 3)
				{
					is_constant_ref = is_constant_ref && node_is_constant(node->children[2]);
					if (is_constant_ref)
					{
						is_constant_ref = is_constant_ref && !(node->children[2]->types.vnumber->is_dont_care_string());
					}
				}

				if (!is_constant_ref)
				{
					// this must be an implicit memory
					long sc_spot = sc_lookup_string(local_symbol_table_sc, node->children[0]->types.identifier);
					oassert(sc_spot > -1);
					((ast_node_t *)local_symbol_table_sc->data[sc_spot])->types.variable.is_memory = true;
				}

				if (node->num_children == 3) convert_2D_to_1D_array_ref(&node, local_string_cache_list);
				break;
			}
			case ALWAYS: // fallthrough
			case INITIALS:
			{
				is_generate_region = true;
				break;
			}
			default:
			{
				break;
			}
		}

		if (child_skip_list)
		{
			child_skip_list = (short*)vtr::free(child_skip_list);
		}
	}

	return node;
}

/* ---------------------------------------------------------------------------------------------------
 * (function: reduce_expressions)
 *
 * This function folds remaining constant expressions now that symbols are found (e.g. for assign)
 * (TODO) and resolves hierarchical references, and call netlist_expand on the way back up.
 * --------------------------------------------------------------------------------------------------- */
ast_node_t *reduce_expressions(ast_node_t *node, STRING_CACHE_LIST *local_string_cache_list, long *max_size, long assignment_size)
{
	short *child_skip_list = NULL; // list of children not to traverse into
	short skip_children = false; // skips the DFS completely if true

	if (node)
	{
		oassert(node->type != NO_ID);

		STRING_CACHE *local_symbol_table_sc = local_string_cache_list->local_symbol_table_sc;
		
		if (node->num_children > 0)
		{
			child_skip_list = (short*)vtr::calloc(node->num_children, sizeof(short));
		}

		switch (node->type)
		{
			case MODULE:
			{
				oassert(child_skip_list);
				// skip identifier
				child_skip_list[0] = true;
				break;
			}
			case FUNCTION:
			{
				skip_children = true;
				break;
			}	
			case BLOCKING_STATEMENT:
			case NON_BLOCKING_STATEMENT:
			{
				/* try to resolve */
				if (node->children[1]->type != FUNCTION_INSTANCE)
				{
					node->children[0] = reduce_expressions(node->children[0], local_string_cache_list, NULL, 0);

					assignment_size = get_size_of_variable(node->children[0], local_string_cache_list);
					max_size = (long*)calloc(1, sizeof(long));
					
					if (node->children[1]->type != NUMBERS)
					{
						node->children[1] = reduce_expressions(node->children[1], local_string_cache_list, max_size, assignment_size);
						if (node->children[1] == NULL)
						{
							/* resulting from replication of zero */
							error_message(NETLIST_ERROR, node->line_number, node->file_number, 
								"%s","Cannot perform assignment with nonexistent value");
						}
					}
					else
					{
						VNumber *temp = node->children[1]->types.vnumber;
						node->children[1]->types.vnumber = new VNumber(*temp, assignment_size);
						delete temp;
					}

					vtr::free(max_size);

					/* cast to unsigned if necessary */
					if (node_is_constant(node->children[1]))
					{
						char *id = NULL;
						if (node->children[0]->type == IDENTIFIERS)
						{
							id = node->children[0]->types.identifier;
						}
						else
						{
							id = node->children[0]->children[0]->types.identifier;
						}

						long sc_spot = sc_lookup_string(local_symbol_table_sc, id);
						if (sc_spot > -1)
						{
							bool is_signed = ((ast_node_t *)local_symbol_table_sc->data[sc_spot])->types.variable.is_signed;
							if (!is_signed)
							{
								VNumber *temp = node->children[1]->types.vnumber;
								VNumber *to_unsigned = new VNumber(V_UNSIGNED(*temp));
								node->children[1]->types.vnumber = to_unsigned;
								delete temp;
							}
							else
							{
								/* leave as is */
							}
						}
					}
					else
					{
						/* signed keyword is not supported, meaning unresolved values will already be handled as
							unsigned at the netlist level... must update once signed support is added */
					}
					
					assignment_size = 0;
					skip_children = true;
				}
				break;
			}
			default:
			{
				break;
			}
		}

		if (child_skip_list && node->num_children > 0 && skip_children == false)
		{
			/* use while loop and boolean to prevent optimizations
				since number of children may change during recursion */

			bool done = false;
			int i = 0;

			/* traverse all the children */
			while (done == false)
			{
				int num_original_children = node->num_children;
				if (child_skip_list[i] == false)
				{
					/* recurse */
					ast_node_t *old_child = node->children[i];
					ast_node_t *new_child = reduce_expressions(old_child, local_string_cache_list, max_size, assignment_size);
					node->children[i] = new_child;

					if (old_child != new_child)
					{
						/* recurse on this child again in case it can be further reduced */
						i--;
					}
				}

				if (num_original_children != node->num_children)
				{
					child_skip_list = (short *)vtr::realloc(child_skip_list, sizeof(short)*node->num_children);
					for (int j = num_original_children; j < node->num_children; j++)
					{
						child_skip_list[j] = false;
					}
				}

				i++;				
				if (i >= node->num_children)
				{
					done = true;
				}
			}
		}

		/* post-amble */
		switch (node->type)
		{
			case CONCATENATE:
			{
				resolve_concat_sizes(node, local_string_cache_list);
				break;
			}
			case BINARY_OPERATION:
			{
				if (node->children[0] == NULL || node->children[1] == NULL)
				{
					/* resulting from replication of zero */
					error_message(NETLIST_ERROR, node->line_number, node->file_number, 
						"%s","Cannot perform operation with nonexistent value");
				}

				ast_node_t *new_node = fold_binary(&node);
				if (node_is_constant(new_node))
				{
					/* resize as needed */
					long new_size;
					long this_size = new_node->types.vnumber->size();

					if (assignment_size > 0)
					{
						new_size = assignment_size;
					}
					else if (max_size)
					{
						new_size = *max_size;
					}
					else
					{
						new_size = this_size;
					}

					/* clean up */
					free_resolved_children(node);
												
					change_to_number_node(node, VNumber(*(new_node->types.vnumber), new_size));
				}
				new_node = free_whole_tree(new_node);
				break;
			}
			case UNARY_OPERATION:
			{
				if (node->children[0] == NULL)
				{
					/* resulting from replication of zero */
					error_message(NETLIST_ERROR, node->line_number, node->file_number, 
						"%s","Cannot perform operation with nonexistent value");
				}

				ast_node_t *new_node = fold_unary(&node);
				if (node_is_constant(new_node))
				{
					/* resize as needed */
					long new_size;
					long this_size = new_node->types.vnumber->size();

					if (assignment_size > 0)
					{
						new_size = assignment_size;
					}
					else if (max_size)
					{
						new_size = *max_size;
					}
					else
					{
						new_size = this_size;
					}

					/* clean up */
					free_resolved_children(node);
					
					change_to_number_node(node, VNumber(*(new_node->types.vnumber), new_size));
				}
				new_node = free_whole_tree(new_node);
				break;
			}
			case IDENTIFIERS:
			{
				// look up to resolve unresolved range refs
				ast_node_t *var_node = NULL;

				if (node->types.identifier)
				{
					long sc_spot = sc_lookup_string(local_symbol_table_sc, node->types.identifier);
					if (sc_spot > -1)
					{
						var_node = (ast_node_t *)local_symbol_table_sc->data[sc_spot];

						if (var_node->children[1] != NULL)
						{
							oassert(node_is_constant(var_node->children[1]) && node_is_constant(var_node->children[2]));
						}
						if (var_node->children[3] != NULL)
						{
							oassert(node_is_constant(var_node->children[3]) && node_is_constant(var_node->children[4]));
						}
						if (var_node->num_children == 8 && var_node->children[5])
						{
							oassert(node_is_constant(var_node->children[5]) && node_is_constant(var_node->children[6]));
						}

						local_symbol_table_sc->data[sc_spot] = (void *)var_node;
					}

					if (max_size)
					{
						long var_size = get_size_of_variable(node, local_string_cache_list);
						if (var_size > *max_size)
						{
							*max_size = var_size;
						}
					}
				}

				break;
			}
			case NUMBERS:
			{
				if (max_size)
				{
					if (node->types.vnumber->size() > (*max_size))
					{
						*max_size = node->types.vnumber->size();
					}
				}

				break;
			}
			default:
			{
				break;
			}
		}

		if (child_skip_list)
		{
			child_skip_list = (short*)vtr::free(child_skip_list);
		}
	}

	return node;
}

void update_string_caches(STRING_CACHE_LIST *local_string_cache_list)
{
	STRING_CACHE *local_param_table_sc = local_string_cache_list->local_param_table_sc;
	ast_node_t **local_symbol_table = local_string_cache_list->local_symbol_table;
	int num_local_symbol_table = local_string_cache_list->num_local_symbol_table;

	int i = 0;

	if (local_param_table_sc)
	{
		while (i < local_param_table_sc->size && local_param_table_sc->data[i] != NULL)
		{
			ast_node_t *param_val = (ast_node_t *)local_param_table_sc->data[i];
			if (param_val->type == VAR_DECLARE && !node_is_constant(param_val->children[5]))
			{
				ast_node_t *resolved_val = finalize_ast(param_val->children[5], NULL, local_string_cache_list, true, true, NULL, NULL);
				oassert(resolved_val);
				param_val->children[5] = NULL;
				free_whole_tree(param_val);
				param_val = resolved_val;
			}
			else if (!node_is_constant(param_val))
			{
				param_val = finalize_ast(param_val, NULL, local_string_cache_list, true, true, NULL, NULL);
			}
			oassert(node_is_constant(param_val));

			/* this forces parameter values as unsigned, since we don't currently support signed keyword...
					must be changed once support is added */
			VNumber *temp = param_val->types.vnumber;
			VNumber *to_unsigned = new VNumber(V_UNSIGNED(*temp));
			param_val->types.vnumber = to_unsigned;
			delete temp;

			local_param_table_sc->data[i] = (void *)param_val;
			i++;
		}
	}

	for (i = 0; i < num_local_symbol_table; i++)
	{
		ast_node_t *var_declare = local_symbol_table[i];
		if ((var_declare->children[1] && !node_is_constant(var_declare->children[1]))
			|| (var_declare->children[2] && !node_is_constant(var_declare->children[2]))
			|| (var_declare->children[3] && !node_is_constant(var_declare->children[3]))
			|| (var_declare->children[4] && !node_is_constant(var_declare->children[4]))
			|| var_declare->num_children == 8
		)
		{
			local_symbol_table[i] = finalize_ast(var_declare, NULL, local_string_cache_list, true, true, NULL, NULL);
		}
	}
}

void update_instance_parameter_table(ast_node_t *instance, STRING_CACHE *instance_param_table_sc)
{
	if (instance->children[1]->children[2])
	{
		long sc_spot;
		ast_node_t *parameter_override_list = instance->children[1]->children[2];
		int param_count = 0;

		bool is_by_name 	=  parameter_override_list->children[0]->types.variable.is_parameter 
							&& parameter_override_list->children[0]->children[0] != NULL;

		/* error checks for overrides passed directly into instance */
		for (int j = 0; j < parameter_override_list->num_children; j++)
		{
			if (parameter_override_list->children[j]->types.variable.is_parameter)
			{
				param_count++;
				if (is_by_name != (parameter_override_list->children[j]->children[0] != NULL))
				{
					error_message(PARSE_ERROR, parameter_override_list->line_number, parameter_override_list->file_number, 
							"%s", "Cannot mix parameters passed by name and parameters passed by ordered list\n");
				}
				else if (is_by_name)
				{
					char *param_id = parameter_override_list->children[j]->children[0]->types.identifier;
					
					for (int k = 0; k < j; k++)
					{
						if (!strcmp(param_id, parameter_override_list->children[k]->children[0]->types.identifier))
						{
							error_message(PARSE_ERROR, parameter_override_list->line_number, parameter_override_list->file_number, 
								"Cannot override the same parameter twice (%s) within a module instance\n",
								parameter_override_list->children[j]->children[0]->types.identifier);
						}
					}

					sc_spot = sc_lookup_string(instance_param_table_sc, param_id);
					if (sc_spot == -1)
					{
						error_message(NETLIST_ERROR, parameter_override_list->line_number, parameter_override_list->file_number,
							"Can't find parameter name %s in module %s\n",
							param_id, instance->children[0]->types.identifier);
					}
				}
				else
				{
					char *param_id = vtr::strdup(instance_param_table_sc->string[j]);
					ast_node_t *param_symbol = newSymbolNode(param_id, parameter_override_list->children[j]->line_number);
					parameter_override_list->children[j]->children[0] = param_symbol;
				}
			}
			else
			{
				oassert(parameter_override_list->children[j]->types.variable.is_defparam);
				break;
			}
		}

		if (!is_by_name && param_count > instance_param_table_sc->free)
		{
			error_message(NETLIST_ERROR, parameter_override_list->line_number, parameter_override_list->file_number,
				"There are more parameters (%d) passed into %s than there are specified in the module (%d)!",
				param_count, instance->children[1]->children[0]->types.identifier, instance_param_table_sc->free);
		}

		/* fill out the overrides */
		for (int j = (parameter_override_list->num_children - 1); j >= 0; j--)
		{
			char *param_id = parameter_override_list->children[j]->children[0]->types.identifier;
			sc_spot = sc_lookup_string(instance_param_table_sc, param_id);
			if (sc_spot == -1)
			{
				error_message(NETLIST_ERROR, parameter_override_list->line_number, parameter_override_list->file_number,
					"Can't find parameter name %s in module %s\n",
					param_id, instance->children[0]->types.identifier);
			}
			else
			{
				/* update this parameter and remove all other overrides with this name */
				free_whole_tree(((ast_node_t *)instance_param_table_sc->data[sc_spot]));
				instance_param_table_sc->data[sc_spot] = (void *)parameter_override_list->children[j]->children[5];
				parameter_override_list->children[j]->children[5] = NULL;

				for (int k = 0; k < j; k++)
				{
					if (!strcmp(parameter_override_list->children[k]->children[0]->types.identifier, param_id))
					{
						ast_node_t *temp = parameter_override_list->children[k];
						remove_child_from_node_at_index(parameter_override_list, k);
						temp = NULL;
						j--;
						k--;
					}
				}

				remove_child_from_node_at_index(parameter_override_list, j);
			}
		}
		// check if there are still overrides in the list that haven't been deleted

		if (parameter_override_list->num_children > 0)
		{
			oassert(false); // not reachable???
		}
	}
}

void convert_2D_to_1D_array(ast_node_t **var_declare)
{
	ast_node_t *node_max2  = (*var_declare)->children[3];
	ast_node_t *node_min2  = (*var_declare)->children[4];

	ast_node_t *node_max3  = (*var_declare)->children[5];
	ast_node_t *node_min3  = (*var_declare)->children[6];

	oassert(node_min2->type == NUMBERS && node_max2->type == NUMBERS);		
	oassert(node_min3->type == NUMBERS && node_max3->type == NUMBERS);

	long addr_min = node_min2->types.vnumber->get_value();
	long addr_max = node_max2->types.vnumber->get_value();

	long addr_min1= node_min3->types.vnumber->get_value();
	long addr_max1= node_max3->types.vnumber->get_value();

	if((addr_min > addr_max) || (addr_min1 > addr_max1))
	{
		error_message(NETLIST_ERROR, (*var_declare)->children[0]->line_number, (*var_declare)->children[0]->file_number, "%s",
				"Odin doesn't support arrays declared [m:n] where m is less than n.");
	}	
	else if((addr_min < 0 || addr_max < 0) || (addr_min1 < 0 || addr_max1 < 0))
	{
		error_message(NETLIST_ERROR, (*var_declare)->children[0]->line_number, (*var_declare)->children[0]->file_number, "%s",
				"Odin doesn't support negative number in index.");
	}

	char *name = (*var_declare)->children[0]->types.identifier;

	if (addr_min != 0 || addr_min1 != 0)
	{
		error_message(NETLIST_ERROR, (*var_declare)->children[0]->line_number, (*var_declare)->children[0]->file_number,
				"%s: right memory address index must be zero\n", name);
	}
	
	long addr_chunk_size = (addr_max1 - addr_min1 + 1);

	(*var_declare)->chunk_size = addr_chunk_size;

	long new_address_max = (addr_max - addr_min + 1)*addr_chunk_size -1;

	change_to_number_node((*var_declare)->children[3], new_address_max);
	change_to_number_node((*var_declare)->children[4], 0);

	(*var_declare)->children[5] = free_whole_tree((*var_declare)->children[5]);
	(*var_declare)->children[6] = free_whole_tree((*var_declare)->children[6]);

	(*var_declare)->children[5] = (*var_declare)->children[7];
	(*var_declare)->children[7] = NULL; 

	(*var_declare)->num_children -= 2;

}


void convert_2D_to_1D_array_ref(ast_node_t **node, STRING_CACHE_LIST *local_string_cache_list)
{
	STRING_CACHE *local_symbol_table_sc = local_string_cache_list->local_symbol_table_sc;

	char * temp_string = make_full_ref_name(NULL, NULL, NULL, (*node)->children[0]->types.identifier, -1);

	// look up chunk size
	long sc_spot = sc_lookup_string(local_symbol_table_sc, temp_string);
	oassert(sc_spot != -1);

	ast_node_t *array_size = ast_node_deep_copy((ast_node_t *)local_symbol_table_sc->data[sc_spot]);
	
	change_to_number_node(array_size, array_size->chunk_size);

	ast_node_t *array_row = (*node)->children[1];
	ast_node_t *array_col = (*node)->children[2];

	// build the new AST
	ast_node_t *new_node_1 = newBinaryOperation(MULTIPLY, array_row, array_size, (*node)->children[0]->line_number);
	ast_node_t *new_node_2 = newBinaryOperation(ADD, new_node_1, array_col, (*node)->children[0]->line_number);

	vtr::free(temp_string);

	(*node)->children[1] = new_node_2;
	(*node)->children[2] = NULL;
	(*node)->num_children -= 1;


}


bool verify_terminal(ast_node_t *top, ast_node_t *iterator)
{
	if (top)
	{
		if (top->type == BINARY_OPERATION)
		{
			return verify_terminal(top->children[0], iterator) && verify_terminal(top->children[1], iterator);
		}
		else if (top->type == UNARY_OPERATION)
		{
			return verify_terminal(top->children[0], iterator);
		}
		else if (top->type == IDENTIFIERS)
		{
			return (strcmp(top->types.identifier, iterator->children[0]->types.identifier) == 0);
		}
		else
		{
			return node_is_constant(top);
		}
	}
	else
	{
		return false;
	}
}

void verify_genvars(ast_node_t *node, STRING_CACHE_LIST *local_string_cache_list, char ***other_genvars, int num_genvars)
{
	if (node)
	{
		if (node->type == FOR)
		{
			STRING_CACHE *local_symbol_table_sc = local_string_cache_list->local_symbol_table_sc;
			ast_node_t *initial = node->children[0];
			ast_node_t *terminal = node->children[2];
			ast_node_t *body = node->children[3];
			ast_node_t *iterator = initial->children[0];

			if (strcmp(initial->children[0]->types.identifier, terminal->children[0]->types.identifier) != 0)
			{
				/* must match */
				error_message(NETLIST_ERROR, node->line_number, node->file_number, 
					"%s","Must use the same genvar for initial condition and iteration condition");
			}			

			long sc_spot = sc_lookup_string(local_symbol_table_sc, iterator->types.identifier);
			if (sc_spot < 0)
			{
				error_message(NETLIST_ERROR, iterator->line_number, iterator->file_number, 
					"Missing declaration of this symbol %s\n", iterator->types.identifier);
			}
			else if (!((ast_node_t *)local_symbol_table_sc->data[sc_spot])->types.variable.is_genvar)
			{
				error_message(NETLIST_ERROR, node->line_number, node->file_number, 
					"%s","Iterator for loop generate construct must be declared as a genvar");
			}

			// check genvars that were already found
			for (int i=0; i < num_genvars; i++)
			{
				if (!strcmp(iterator->types.identifier, (*other_genvars)[i]))
				{
					error_message(NETLIST_ERROR, node->line_number, node->file_number, 
						"%s","Cannot reuse a genvar in a nested loop sequence");
				}
			}

			(*other_genvars) = (char **)vtr::realloc((*other_genvars), sizeof(char*)*(num_genvars+1));
			(*other_genvars)[num_genvars] = iterator->types.identifier;
			num_genvars++;

			// look for nested loops to verify that each genvar is used in only one loop
			for (int i=0; i < body->num_children; i++)
			{
				verify_genvars(body->children[i], local_string_cache_list, other_genvars, num_genvars);
			}
		}
		else
		{
			// look for nested loops to verify that each genvar is used in only one loop
			for (int i=0; i < node->num_children; i++)
			{
				verify_genvars(node->children[i], local_string_cache_list, other_genvars, num_genvars);
			}
		}
	}
}

ast_node_t *look_for_matching_hard_block(ast_node_t *node, char *hard_block_name, STRING_CACHE_LIST *local_string_cache_list)
{
	t_model *hb_model = find_hard_block(hard_block_name);
	bool is_hb = true;

	if (!hb_model)
	{
		/* check for soft logic RAM */
		return look_for_matching_soft_logic(node, hard_block_name);
	}
	else
	{
		t_model_ports *hb_input_ports = hb_model->inputs;
		t_model_ports *hb_output_ports = hb_model->outputs;

		int num_hb_inputs = 0;
		int num_hb_outputs = 0;

		while (hb_input_ports != NULL)
		{
			num_hb_inputs++;
			hb_input_ports = hb_input_ports->next;
		}

		while (hb_output_ports != NULL)
		{
			num_hb_outputs++;
			hb_output_ports = hb_output_ports->next;
		}


		ast_node_t *connect_list = node->children[1]->children[1];

		/* first check if the number of ports match up */
		if (connect_list->num_children != (num_hb_inputs + num_hb_outputs))
		{
			is_hb = false;
		}
		

		if (is_hb && connect_list->children[0]->children[0] != NULL)
		{
			/* number of ports match and ports were passed in by name;
			 * if all port names match up, this is a hard block */

			for (int i = 0; i < connect_list->num_children && is_hb; i++)
			{
				oassert(connect_list->children[i]->children[0]);
				char *id = connect_list->children[i]->children[0]->types.identifier;
				hb_input_ports = hb_model->inputs;
				hb_output_ports = hb_model->outputs;

				while ((hb_input_ports != NULL) && (strcmp(hb_input_ports->name, id) != 0))
				{
					hb_input_ports = hb_input_ports->next;
				}

				if (hb_input_ports == NULL)
				{
					while ((hb_output_ports != NULL) && (strcmp(hb_output_ports->name, id) != 0))
					{
						hb_output_ports = hb_output_ports->next;
					}
				}
				else
				{
					/* matching input was found; move on to the next port connection */
					continue;
				}

				if (hb_output_ports == NULL)
				{
					/* name doesn't match up with any of the defined ports; this is not a hard block */
					is_hb = false;
				}
				else
				{
					/* matching output was found; move on to the next port connection */
					continue; 
				}
			}
		}
		else if (is_hb)
		{
			/* number of ports match and ports were passed in by ordered list; 
			 * this is risky, but we will try to do some "smart" mapping to mark inputs and outputs 
			 * by evaluating the port connections to determine the order */

			warning_message(NETLIST_ERROR, connect_list->line_number, connect_list->file_number, 
				"Attempting to convert this instance to a hard block (%s) -	unnamed port connections will be matched according to hard block specification and may produce unexpected results\n", hard_block_name);

			t_model_ports *hb_ports_1 = NULL, *hb_ports_2 = NULL;
			bool is_input, is_output;
			int num_ports;

			hb_input_ports = hb_model->inputs;
			hb_output_ports = hb_model->outputs;

			/* decide whether to look for inputs or outputs based on what there are less of */
			if (num_hb_inputs <= num_hb_outputs)
			{
				is_input = true;
				is_output = false;
				num_ports = num_hb_inputs;
			}
			else
			{
				is_input = false;
				is_output = true;
				num_ports = num_hb_outputs;
			}

			STRING_CACHE *local_symbol_table_sc = local_string_cache_list->local_symbol_table_sc;

			int i;

			/* look through the first N (num_ports) port connections to look for a match */
			for (i = 0; i < num_ports; i++)
			{
				char *port_id = connect_list->children[i]->children[1]->types.identifier;
				long sc_spot = sc_lookup_string(local_symbol_table_sc, port_id);
				oassert(sc_spot > -1);
				ast_node_t *var_declare = (ast_node_t *)local_symbol_table_sc->data[sc_spot];

				if (var_declare->types.variable.is_output == is_output
					&& var_declare->types.variable.is_input == is_input)
				{
					/* found a match! check if it's an input or output */
					if (is_input)
					{
						hb_ports_1 = hb_model->inputs;
						hb_ports_2 = hb_model->outputs;
					}
					else
					{
						hb_ports_1 = hb_model->outputs;
						hb_ports_2 = hb_model->inputs;
					}
					break;
				}
				else if (var_declare->types.variable.is_output != is_output
					&& var_declare->types.variable.is_input != is_input)
				{
					/* found the opposite of what we were looking for */
					if (is_input)
					{
						hb_ports_1 = hb_model->outputs;
						hb_ports_2 = hb_model->inputs;
					}
					else
					{
						hb_ports_1 = hb_model->inputs;
						hb_ports_2 = hb_model->outputs;
					}
					break;
				}
			}

			/* if a match hasn't been found yet, look through the last N (num_ports) port connections */
			if (!(hb_ports_1 && hb_ports_2))
			{
				for (i = connect_list->num_children - num_ports; i < connect_list->num_children; i++)
				{
					char *port_id = connect_list->children[i]->children[1]->types.identifier;
					long sc_spot = sc_lookup_string(local_symbol_table_sc, port_id);
					oassert(sc_spot > -1);
					ast_node_t *var_declare = (ast_node_t *)local_symbol_table_sc->data[sc_spot];

					if (var_declare->types.variable.is_output == is_output
						&& var_declare->types.variable.is_input == is_input)
					{
						/* found a match! since we're at the other end, inputs/outputs should be reversed */
						if (is_input)
						{
							hb_ports_1 = hb_model->outputs;
							hb_ports_2 = hb_model->inputs;
						}
						else
						{
							hb_ports_1 = hb_model->inputs;
							hb_ports_2 = hb_model->outputs;
						}
						break;
					}
					else if (var_declare->types.variable.is_output != is_output
						&& var_declare->types.variable.is_input != is_input)
					{
						/* found the opposite of what we were looking for */
						if (is_input)
						{
							hb_ports_1 = hb_model->inputs;
							hb_ports_2 = hb_model->outputs;
						}
						else
						{
							hb_ports_1 = hb_model->outputs;
							hb_ports_2 = hb_model->inputs;
						}
						break;
					}
				}
			}

			/* if a match hasn't been found, then there is no way to tell what should be done first;
			 * we will default to inputs first, then outputs after (this is an arbitrary decision) */
			if (!(hb_ports_1 && hb_ports_2))
			{
				hb_ports_1 = hb_model->inputs;
				hb_ports_2 = hb_model->outputs;
			}

			/* attach new port identifiers for later reference when building the hard block */
			i = 0;
			while (hb_ports_1)
			{
				oassert(connect_list->children[i] && !connect_list->children[i]->children[0]);
				connect_list->children[i]->children[0] = newSymbolNode(vtr::strdup(hb_ports_1->name), connect_list->line_number);
				hb_ports_1 = hb_ports_1->next;
				i++;
			}
			while (hb_ports_2)
			{
				oassert(connect_list->children[i] && !connect_list->children[i]->children[0]);
				connect_list->children[i]->children[0] = newSymbolNode(vtr::strdup(hb_ports_2->name), connect_list->line_number);
				hb_ports_2 = hb_ports_2->next;
				i++;
			}
		}
	}

	if (is_hb)
	{
		ast_node_t *instance = ast_node_deep_copy(node->children[1]);
		char *new_hard_block_name = vtr::strdup(hard_block_name);
		
		return newHardBlockInstance(new_hard_block_name, instance, instance->line_number);
	}
	else
	{
		return look_for_matching_soft_logic(node, hard_block_name);
	}
}

ast_node_t *look_for_matching_soft_logic(ast_node_t *node, char *hard_block_name)
{
	bool is_hb = true;

	if (!is_ast_sp_ram(node) && !is_ast_dp_ram(node) && !is_ast_adder(node) && !is_ast_multiplier(node))
	{
		is_hb = false;
	}

	if (is_hb)
	{
		ast_node_t *instance = ast_node_deep_copy(node->children[1]);
		char *new_hard_block_name = vtr::strdup(hard_block_name);
		return newHardBlockInstance(new_hard_block_name, instance, instance->line_number);
	}
	else
	{
		return node;
	}
}

// /*---------------------------------------------------------------------------
//  * (function: reduce_assignment_expression)
//  * reduce the number nodes which can be calculated to optimize the AST
//  *-------------------------------------------------------------------------*/
// void reduce_assignment_expression(ast_node_t *ast_module)
// {
// 	head = NULL;
// 	p = NULL;
// 	ast_node_t *T = NULL;

// 	if (count_id < ast_module->unique_count)
// 		count_id = ast_module->unique_count;

// 	count_assign = 0;
// 	std::vector<ast_node_t *> list_assign;
// 	find_assign_node(ast_module, list_assign, ast_module->children[0]->types.identifier);
// 	for (long j = 0; j < count_assign; j++)
// 	{
// 		if (check_tree_operation(list_assign[j]->children[1]) && (list_assign[j]->children[1]->num_children > 0))
// 		{
// 			store_exp_list(list_assign[j]->children[1]);

// 			if (simplify_expression())
// 			{
// 				enode *tail = find_tail(head);
// 				free_whole_tree(list_assign[j]->children[1]);
// 				T = (ast_node_t*)vtr::malloc(sizeof(ast_node_t));
// 				construct_new_tree(tail, T, list_assign[j]->line_number, list_assign[j]->file_number);
// 				list_assign[j]->children[1] = T;
// 			}
// 			free_exp_list();
// 		}
// 	}
// }

// /*---------------------------------------------------------------------------
//  * (function: search_assign_node)
//  * find the top of the assignment expression
//  *-------------------------------------------------------------------------*/
// void find_assign_node(ast_node_t *node, std::vector<ast_node_t *> list, char *module_name)
// {
// 	if (node && node->num_children > 0 && (node->type == BLOCKING_STATEMENT || node->type == NON_BLOCKING_STATEMENT))
// 	{
// 		list.push_back(node);
// 	}

// 	for (long i = 0; node && i < node->num_children; i++)
// 		find_assign_node(node->children[i], list, module_name);
// }

// /*---------------------------------------------------------------------------
//  * (function: simplify_expression)
//  * simplify the expression stored in the linked_list
//  *-------------------------------------------------------------------------*/
// bool simplify_expression()
// {
// 	bool build = adjoin_constant();
// 	reduce_enode_list();

// 	if(delete_continuous_multiply())
// 		build = true;

// 	if(combine_constant())
// 		build = true;

// 	reduce_enode_list();
// 	return build;
// }

// /*---------------------------------------------------------------------------
//  * (function: find_tail)
//  *-------------------------------------------------------------------------*/
// enode *find_tail(enode *node)
// {
// 	enode *temp;
// 	enode *tail = NULL;
// 	for (temp = node; temp != NULL; temp = temp->next)
// 		if (temp->next == NULL)
// 			tail = temp;

// 	return tail;
// }

// /*---------------------------------------------------------------------------
//  * (function: reduce_enode_list)
//  *-------------------------------------------------------------------------*/
// void reduce_enode_list()
// {
// 	enode *temp;
// 	int a;

// 	while(head != NULL && (head->type.data == 0) && (head->next->priority == 2) && (head->next->type.operation == '+')){
// 		temp=head;
// 		head = head->next->next;
// 		head->pre = NULL;

// 		vtr::free(temp->next);
// 		vtr::free(temp);
// 	}

// 	if(head == NULL){
// 		return;
// 	}

// 	temp = head->next;
// 	while (temp != NULL)
// 	{
// 		if ((temp->flag == 1) && (temp->pre->priority == 2))
// 		{
// 			if (temp->type.data == 0)
// 			{
// 				if (temp->next == NULL)
// 					temp->pre->pre->next = NULL;
// 				else
// 				{
// 					temp->pre->pre->next = temp->next;
// 					temp->next->pre = temp->pre->pre;
// 				}
// 				vtr::free(temp->pre);

// 				enode *toBeDeleted = temp;
// 				temp = temp->next;
// 				vtr::free(toBeDeleted);
// 			} else if (temp->type.data < 0)
// 			{
// 				if (temp->pre->type.operation == '+')
// 					temp->pre->type.operation = '-';
// 				else
// 					temp->pre->type.operation = '+';
// 				a = temp->type.data;
// 				temp->type.data = -a;

// 				temp = temp->next;
// 			} else {
// 				temp = temp->next;
// 			}
// 		} else {
// 			temp = temp->next;
// 		}
// 	}
// }

// /*---------------------------------------------------------------------------
//  * (function: store_exp_list)
//  *-------------------------------------------------------------------------*/
// void store_exp_list(ast_node_t *node)
// {
// 	enode *temp;
// 	head = (enode*)vtr::malloc(sizeof(enode));
// 	p = head;
// 	record_tree_info(node);
// 	temp = head;
// 	head = head->next;
// 	head->pre = NULL;
// 	p->next = NULL;
// 	vtr::free(temp);

// }

// /*---------------------------------------------------------------------------
//  * (function: record_tree_info)
//  *-------------------------------------------------------------------------*/
// void record_tree_info(ast_node_t *node)
// {
// 	if (node){
// 		if (node->num_children > 0)
// 		{
// 			record_tree_info(node->children[0]);
// 			create_enode(node);
// 			if (node->num_children > 1)
// 				record_tree_info(node->children[1]);
// 		}else{
// 			create_enode(node);
// 		}
// 	}
// }

// /*---------------------------------------------------------------------------
//  * (function: create_enode)
//  * store elements of an expression in nodes consisting the linked_list
//  *-------------------------------------------------------------------------*/
// void create_enode(ast_node_t *node)
// {
// 	enode *s = (enode*)vtr::malloc(sizeof(enode));
// 	s->flag = -1;
// 	s->priority = -1;
// 	s->id = node->unique_count;
	
// 	switch (node->type)
// 	{
// 		case NUMBERS:
// 		{
// 			s->type.data = node->types.vnumber->get_value();
// 			s->flag = 1;
// 			s->priority = 0;
// 			break;
// 		}
// 		case BINARY_OPERATION:
// 		{
// 			s->flag = 2;
// 			switch(node->types.operation.op)
// 			{
// 				case ADD:
// 				{
// 					s->type.operation = '+';
// 					s->priority = 2;
// 					break;
// 				}
// 				case MINUS:
// 				{
// 					s->type.operation = '-';
// 					s->priority = 2;
// 					break;
// 				}
// 				case MULTIPLY:
// 				{
// 					s->type.operation = '*';
// 					s->priority = 1;
// 					break;
// 				}
// 				case DIVIDE:
// 				{
// 					s->type.operation = '/';
// 					s->priority = 1;
// 					break;
// 				}
// 				default:
// 				{
// 					break;
// 				}
// 			}
// 			break;
// 		}
// 		case IDENTIFIERS:
// 		{
// 			s->type.variable = node->types.identifier;
// 			s->flag = 3;
// 			s->priority = 0;
// 			break;
// 		}
// 		default:
// 		{
// 			break;
// 		}
// 	}

// 	p->next = s;
// 	s->pre = p;
// 	p = s;

// }

// /*---------------------------------------------------------------------------
//  * (function: adjoin_constant)
//  * compute the constant numbers in the linked_list
//  *-------------------------------------------------------------------------*/
// bool adjoin_constant()
// {
// 	bool build = false;
// 	enode *t, *replace;
// 	int a, b, result = 0;
// 	int mark;
// 	for (t = head; t->next!= NULL; )
// 	{
// 		mark = 0;
// 		if ((t->flag == 1) && (t->next->next != NULL) && (t->next->next->flag ==1))
// 		{
// 			switch (t->next->priority)
// 			{
// 				case 1:
// 					a = t->type.data;
// 					b = t->next->next->type.data;
// 					if (t->next->type.operation == '*')
// 						result = a * b;
// 					else
// 						result = a / b;
// 					replace = replace_enode(result, t, 1);
// 					build = true;
// 					mark = 1;
// 					break;

// 				case 2:
// 					if (((t == head) || (t->pre->priority == 2)) && ((t->next->next->next == NULL) ||(t->next->next->next->priority ==2)))
// 					{
// 						a = t->type.data;
// 						b = t->next->next->type.data;
// 						if (t->pre->type.operation == '+')
// 						{
// 							if (t->next->type.operation == '+')
// 								result = a + b;
// 							else
// 								result = a - b;
// 						}
// 						if (t->pre->type.operation == '-')
// 						{
// 							if (t->next->type.operation == '+')
// 								result = a - b;
// 							else
// 								result = a + b;
// 						}

// 						replace = replace_enode(result, t, 1);
// 						build = true;
// 						mark = 1;
// 					}
// 					break;
//                 default:
//                     //pass
//                     break;
// 			}
// 		}
// 		if (mark == 0)
// 			t = t->next;
// 		else
// 			if (mark == 1)
// 				t = replace;

// 	}
// 	return build;
// }

// /*---------------------------------------------------------------------------
//  * (function: replace_enode)
//  *-------------------------------------------------------------------------*/
// enode *replace_enode(int data, enode *t, int mark)
// {
// 	enode *replace;
// 	replace = (enode*)vtr::malloc(sizeof(enode));
// 	replace->type.data = data;
// 	replace->flag = 1;
// 	replace->priority = 0;

// 	if (t == head)
// 	{
// 		replace->pre = NULL;
// 		head = replace;
// 	}

// 	else
// 	{
// 		replace->pre = t->pre;
// 		t->pre->next = replace;
// 	}

// 	if (mark == 1)
// 	{
// 		replace->next = t->next->next->next;
// 		if (t->next->next->next == NULL)
// 			replace->next = NULL;
// 		else
// 			t->next->next->next->pre = replace;
// 		vtr::free(t->next->next);
// 		vtr::free(t->next);
// 	}
// 	else
// 	{
// 		replace->next = t->next;
// 		t->next->pre = replace;
// 	}

// 	vtr::free(t);
// 	return replace;

// }

// /*---------------------------------------------------------------------------
//  * (function: combine_constant)
//  *-------------------------------------------------------------------------*/
// bool combine_constant()
// {
// 	enode *temp, *m, *s1, *s2, *replace;
// 	int a, b, result;
// 	int mark;
// 	bool build = false;

// 	for (temp = head; temp != NULL; )
// 	{
// 		mark = 0;
// 		if ((temp->flag == 1) && (temp->next != NULL) && (temp->next->priority == 2))
// 		{
// 			if ((temp == head) || (temp->pre->priority == 2))
// 			{
// 				for (m = temp->next; m != NULL; m = m->next)
// 				{
// 					if((m->flag == 1) && (m->pre->priority == 2) && ((m->next == NULL) || (m->next->priority == 2)))
// 					{
// 						s1 = temp;
// 						s2 = m;
// 						a = s1->type.data;
// 						b = s2->type.data;
// 						if ((s1 == head) || (s1->pre->type.operation == '+'))
// 						{
// 							if (s2->pre->type.operation == '+')
// 								result = a + b;
// 							else
// 								result = a - b;
// 						}
// 						else
// 						{
// 							if (s2->pre->type.operation == '+')
// 								result = a - b;
// 							else
// 								result = a + b;
// 						}
// 						replace = replace_enode(result, s1, 2);
// 						if (s2->next == NULL)
// 						{
// 							s2->pre->pre->next = NULL;
// 							mark = 2;
// 						}
// 						else
// 						{
// 							s2->pre->pre->next = s2->next;
// 							s2->next->pre = s2->pre->pre;
// 						}

// 						vtr::free(s2->pre);
// 						vtr::free(s2);
// 						if (replace == head)
// 						{
// 							temp = replace;
// 							mark = 1;
// 						}
// 						else
// 							temp = replace->pre;
// 						build = true;

// 						break;
// 					}
// 				}
// 			}
// 		}
// 		if (mark == 0)
// 			temp = temp->next;
// 		else
// 			if (mark == 1)
// 				continue;
// 			else
// 				break;
// 	}
// 	return build;

// }

// /*---------------------------------------------------------------------------
//  * (function: construct_new_tree)
//  *-------------------------------------------------------------------------*/
// void construct_new_tree(enode *tail, ast_node_t *node, int line_num, int file_num)
// {
// 	enode *temp, *tail1 = NULL, *tail2 = NULL;
// 	int prio = 0;

// 	if (tail == NULL)
// 		return;

// 	prio = check_exp_list(tail);
// 	if (prio == 1)
// 	{
// 		for (temp = tail; temp != NULL; temp = temp->pre)
// 			if ((temp->flag == 2) && (temp->priority == 2))
// 			{
// 				create_ast_node(temp, node, line_num, file_num);
// 				tail1 = temp->pre;
// 				tail2 = find_tail(temp->next);
// 				tail1->next = NULL;
// 				temp->next->pre = NULL;
// 				break;
// 			}

// 	}
// 	else
// 		if(prio == 2)
// 		{
// 			for (temp = tail; temp != NULL; temp = temp->pre)
// 				if ((temp->flag ==2) && (temp->priority == 1))
// 				{
// 					create_ast_node(temp, node, line_num, file_num);
// 					tail1 = temp->pre;
// 					tail2 = temp->next;
// 					tail2->pre = NULL;
// 					tail2->next = NULL;
// 					break;
// 				}
// 		}
// 		else
// 			if (prio == 3)
// 				create_ast_node(tail, node, line_num, file_num);

// 	if (prio == 1 || prio == 2)
// 	{
// 		node->children = (ast_node_t**)vtr::malloc(2*sizeof(ast_node_t*));
// 		node->children[0] = (ast_node_t*)vtr::malloc(sizeof(ast_node_t));
// 		node->children[1] = (ast_node_t*)vtr::malloc(sizeof(ast_node_t));

// 		create_ast_node(tail1, node->children[0], line_num, file_num);
// 		create_ast_node(tail2, node->children[1], line_num, file_num);
		
// 		construct_new_tree(tail1, node->children[0], line_num, file_num);
// 		construct_new_tree(tail2, node->children[1], line_num, file_num);
// 	}

// 	return;
// }

// /*---------------------------------------------------------------------------
//  * (function: check_exp_list)
//  *-------------------------------------------------------------------------*/
// int check_exp_list(enode *tail)
// {
// 	enode *temp;
// 	for (temp = tail; temp != NULL; temp = temp->pre)
// 		if ((temp->flag == 2) && (temp->priority == 2))
// 			return 1;

// 	for (temp = tail; temp != NULL; temp = temp->pre)
// 		if ((temp->flag == 2) && (temp->priority ==1))
// 			return 2;

// 	return 3;
// }

// /*---------------------------------------------------------------------------
//  * (function: delete_continuous_multiply)
//  *-------------------------------------------------------------------------*/
// bool delete_continuous_multiply()
// {
// 	enode *temp, *t, *m, *replace;
// 	bool build = false;
// 	int a, b, result;
// 	int mark;
// 	for (temp = head; temp != NULL; )
// 	{
// 		mark = 0;
// 		if ((temp->flag == 1) && (temp->next != NULL) && (temp->next->priority == 1))
// 		{
// 			for (t = temp->next; t != NULL; t = t->next)
// 			{
// 				if ((t->flag == 1) && (t->pre->priority ==1))
// 				{
// 					for (m = temp->next; m != t; m = m->next)
// 					{
// 						if ((m->flag == 2) && (m->priority != 1))
// 						{
// 							mark = 1;
// 							break;
// 						}

// 					}
// 					if (mark == 0)
// 					{
// 						a = temp->type.data;
// 						b = t->type.data;
// 						if (t->pre->type.operation == '*')
// 							result = a * b;
// 						else
// 							result = a / b;
// 						replace = replace_enode(result, temp, 3);
// 						if (t->next == NULL)
// 							t->pre->pre->next = NULL;
// 						else
// 						{
// 							t->pre->pre->next = t->next;
// 							t->next->pre = t->pre->pre;
// 						}
// 						mark = 2;
// 						build = true;
// 						vtr::free(t->pre);
// 						vtr::free(t);
// 						break;
// 					}
// 					break;
// 				}
// 			}
// 		}
// 		if ((mark == 0) || (mark == 1))
// 			temp = temp->next;
// 		else
// 			if (mark == 2)
// 				temp = replace;

// 	}
// 	return build;
// }

// /*---------------------------------------------------------------------------
//  * (function: create_ast_node)
//  *-------------------------------------------------------------------------*/
// void create_ast_node(enode *temp, ast_node_t *node, int line_num, int file_num)
// {
// 	switch(temp->flag)
// 	{
// 		case 1:
// 			initial_node(node, NUMBERS, line_num, file_num, ++count_id);
// 			change_to_number_node(node, temp->type.data);
// 		break;

// 		case 2:
// 			create_op_node(node, temp, line_num, file_num);
// 		break;

// 		case 3:
// 			initial_node(node, IDENTIFIERS, line_num, file_num, ++count_id);
// 			node->types.identifier = vtr::strdup(temp->type.variable.c_str());
// 		break;

// 		default:
// 		break;
// 	}

// }

// /*---------------------------------------------------------------------------
//  * (function: create_op_node)
//  *-------------------------------------------------------------------------*/
// void create_op_node(ast_node_t *node, enode *temp, int line_num, int file_num)
// {
// 	initial_node(node, BINARY_OPERATION, line_num, file_num, ++count_id);
// 	node->num_children = 2;
// 	switch(temp->type.operation)
// 	{
// 		case '+':
// 			node->types.operation.op = ADD;
// 		break;

// 		case '-':
// 			node->types.operation.op = MINUS;
// 		break;

// 		case '*':
// 			node->types.operation.op = MULTIPLY;
// 		break;

// 		case '/':
// 			node->types.operation.op = DIVIDE;
// 		break;
//         default:
//             oassert(false);
//         break;
// 	}

// }

// /*---------------------------------------------------------------------------
//  * (function: free_exp_list)
//  *-------------------------------------------------------------------------*/
// void free_exp_list()
// {
// 	enode *next, *temp;
// 	for (temp = head; temp != NULL; temp = next)
// 	{
// 		next = temp->next;
// 		vtr::free(temp);
// 	}
// }

// /*---------------------------------------------------------------------------
//  * (function: change_exp_list)
//  *-------------------------------------------------------------------------*/
// void change_exp_list(enode *begin, enode *end, enode *s, int flag)
// {
// 	enode *temp, *new_head, *tail, *p2, *partial = NULL, *start = NULL;
// 	int mark;
// 	switch (flag)
// 	{
// 		case 1:
// 		{
// 			for (temp = begin; temp != end; temp = p2->next)
// 			{
// 				mark = 0;
// 				for (p2 = temp; p2 != end->next; p2 = p2->next)
// 				{
// 					if (p2 == end)
// 					{
// 						mark = 2;
// 						break;
// 					}
// 					if ((p2->flag == 2) && (p2->priority == 2))
// 					{
// 						partial = p2->pre;
// 						mark = 1;
// 						break;
// 					}
// 				}
// 				if (mark == 1)
// 				{
// 					new_head = (enode*)vtr::malloc(sizeof(enode));
// 					tail = copy_enode_list(new_head, end->next, s);
// 					tail = tail->pre;
// 					vtr::free(tail->next);
// 					partial->next = new_head;
// 					new_head->pre = partial;
// 					tail->next = p2;
// 					p2->pre = tail;
// 				}
// 				if (mark == 2)
// 					break;
// 			}
// 			break;
// 		}

// 		case 2:
// 		{
// 			for (temp = begin; temp != end->next; temp = temp->next)
// 				if ((temp->flag == 2) && (temp->priority == 2))
// 				{
// 					start = temp;
// 					break;
// 				}
// 			for (temp = start; temp != end->next; temp = partial->next)
// 			{
// 				mark = 0;
// 				for (p2 = temp; p2 != end->next; p2 = p2->next)
// 				{
// 					if (p2 == end)
// 					{
// 						mark = 2;
// 						break;
// 					}
// 					if ((p2->flag == 2) && (p2->priority == 2))
// 					{
// 						partial = p2->next;
// 						mark = 1;
// 						break;
// 					}
// 				}
// 				if (mark == 1)
// 				{
// 					new_head = (enode*)vtr::malloc(sizeof(enode));
// 					tail = copy_enode_list(new_head, s, begin->pre);
// 					tail = tail->pre;
// 					vtr::free(tail->next);
// 					p2->next = new_head;
// 					new_head->pre = p2;
// 					tail->next = partial;
// 					partial->pre = tail;

// 				}
// 				if (mark == 2)
// 					break;
// 			}
// 			break;
// 		}
//         default:
//             break;

// 	}
// }

// /*---------------------------------------------------------------------------
//  * (function: copy_enode_list)
//  *-------------------------------------------------------------------------*/
// enode *copy_enode_list(enode *new_head, enode *begin, enode *end)
// {
// 	enode *temp, *new_enode, *next_enode;
// 	new_enode = new_head;
// 	for (temp = begin; temp != end->next; temp = temp->next)
// 	{
// 		copy_enode(temp, new_enode);
// 		next_enode = (enode*)vtr::malloc(sizeof(enode));
// 		new_enode->next = next_enode;
// 		next_enode->pre = new_enode;
// 		new_enode = next_enode;
// 	}

// 	return new_enode;
// }

// /*---------------------------------------------------------------------------
//  * (function: copy_enode)
//  *-------------------------------------------------------------------------*/
// void copy_enode(enode *node, enode *new_node)
// {
// 	new_node->type = node->type;
// 	new_node->flag = node->flag;
// 	new_node->priority = node->priority;
// 	new_node->id = -1;
// }

// /*---------------------------------------------------------------------------
//  * (function: check_tree_operation)
//  *-------------------------------------------------------------------------*/
// bool check_tree_operation(ast_node_t *node)
// {
// 	if (node && (node->type == BINARY_OPERATION) && ((node->types.operation.op == ADD ) || (node->types.operation.op == MINUS) || (node->types.operation.op == MULTIPLY)))
// 		return true;

// 	for (long i = 0; node &&  i < node->num_children; i++)
// 		if(check_tree_operation(node->children[i]))
// 			return true;

// 	return false;
// }

// /*---------------------------------------------------------------------------
//  * (function: check_operation)
//  *-------------------------------------------------------------------------*/
// void check_operation(enode *begin, enode *end)
// {
// 	enode *temp, *op;
// 	for (temp = begin; temp != head; temp = temp->pre)
// 	{
// 		if (temp->flag == 2 && temp->priority == 2 && temp->type.operation == '-')
// 		{
// 			for (op = begin; op != end; op = op->next)
// 			{
// 				if (op->flag == 2 && op->priority == 2)
// 				{
// 					if (op->type.operation == '+')
// 						op->type.operation = '-';
// 					else
// 						op->type.operation = '+';
// 				}
// 			}
// 		}
// 	}
// }
