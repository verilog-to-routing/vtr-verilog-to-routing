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
#include "types.h"
#include "ast_util.h"
#include "ast_optimizations.h"
#include "parse_making_ast.h"
#include "verilog_bison.h"
#include "verilog_bison_user_defined.h"
#include "util.h"

/* --------------------------------------------------------------------------------------------
 --------------------------------------------------------------------------------------------
 --------------------------------------------------------------------------------------------
AST ASSUMPTION REDUCTION FUNCTIONS
- Do reductions of constant expressions
 --------------------------------------------------------------------------------------------
 --------------------------------------------------------------------------------------------
 --------------------------------------------------------------------------------------------*/

info_ast_visit_t *reduceAST_traverse_node(ast_node_t *node, ast_node_t *from, int position_idx);
/*---------------------------------------------------------------------------------------------
 * (function: newConstantNode)
 *-------------------------------------------------------------------------------------------*/
void optimizations_on_AST(ast_node_t *top)
{
	/* clean up the hard blocks in the AST */
	cleanup_hard_blocks();
	reduceAST_traverse_node(top, NULL, -1);
}

/*---------------------------------------------------------------------------------------------
 * (function: reduceAST_traverse_node)
 *-------------------------------------------------------------------------------------------*/
info_ast_visit_t *reduceAST_traverse_node(ast_node_t *node, ast_node_t *from, int position_idx)
{
	int i;
	info_ast_visit_t *node_details = NULL;
	info_ast_visit_t **children_info = NULL;
	short free_myself = FALSE;

	if (node == NULL)
	{
		/* print out the node and label details */
		return NULL;
	}
	else
	{
		switch(node->type)
		{
			default:
				break;
		}	

		/* allocate the information for all the returned children info */
		children_info = (info_ast_visit_t**)malloc(sizeof(info_ast_visit_t*)*node->num_children);	

		/* traverse all the children */
		for (i = 0; i < node->num_children; i++)
		{
			/* recursively call through the tree going to each instance.  This is depth first search. */
			children_info[i] = reduceAST_traverse_node(node->children[i], node, i);
		}

		/* post process the children */
		switch(node->type)
		{
			default:
				break;
		}

		/* clean up the allocations in the downward traversal */
		for (i = 0; i < node->num_children; i++)
		{
			if(children_info[i] != NULL)
				free(children_info[i]);
		}
		if (children_info != NULL)
			free(children_info);

		if (free_myself == TRUE)
			/* remove the node */
			free_child_in_tree(from, position_idx);
	}

	return node_details;
}

/*---------------------------------------------------------------------------------------------
 * (function: constantFold)
 *-------------------------------------------------------------------------------------------*/
info_ast_visit_t *constantFold(ast_node_t *node)
{
	int i;
	info_ast_visit_t *node_details = NULL;
	info_ast_visit_t **children_info = NULL;
	short free_myself = FALSE;

	if (node == NULL)
	{
		/* print out the node and label details */
		return NULL;
	}
	else
	{
		switch(node->type)
		{
			case NUMBERS:
				/* we're looking for constant expression reductions */

				/* allocate the return  structure */
				node_details = (info_ast_visit_t*)calloc(1, sizeof(info_ast_visit_t));	
				oassert(node_details);

				/* check if it's a constant depending on the number type */
				switch (node->types.number.base)
				{
					case(DEC):
						if (node_details->value != -1)
						{
							node_details->is_constant = TRUE;
							node_details->value = node->types.number.value;
						}
						break;
					case(HEX):
						if (node_details->value != -1)
						{
							node_details->is_constant = TRUE;
							node_details->value = node->types.number.value;
						}
						break;
					case(OCT):
						if (node_details->value != -1)
						{
							node_details->is_constant = TRUE;
							node_details->value = node->types.number.value;
						}
						break;
					case(BIN):
						if (node_details->value != -1)
						{
							node_details->is_constant = TRUE;
							node_details->value = node->types.number.value;
						}
						break;
					case(LONG_LONG):
						node_details->is_constant = TRUE;
						node_details->value = node->types.number.value;
						break;
				}
				
				if (node_details->is_constant == TRUE)
				{
					/* if this is a constant then set up all the other information in the return */	
					node_details->me = node;
					node_details->from = NULL;
				}
				else
				{
					free(node_details);
					node_details = NULL;
				}
				break;
			default:
				break;
		}	

		/* allocate the information for all the returned children info */
		children_info = (info_ast_visit_t**)malloc(sizeof(info_ast_visit_t*)*node->num_children);	

		/* traverse all the children */
		for (i = 0; i < node->num_children; i++)
		{
			/* recursively call through the tree going to each instance.  This is depth first search. */
			children_info[i] = constantFold(node->children[i]);
		}

		/* post process the children */
		switch(node->type)
		{
			case UNARY_OPERATION:
				if ((children_info[0] != NULL) 
					&& (children_info[0]->is_constant == TRUE))
				{
					/* check if we can do a reduction */
					long long new_value;
					short is_reduction = TRUE;
					/* IF can reduce to number constant */
					ast_node_t *new_node;
					
					/* do the operation and store in new node */	
					switch (node->types.operation.op)
					{
						case LOGICAL_NOT:
							new_value = !children_info[0]->value;
							break;
						case BITWISE_NOT:
							new_value = ~children_info[0]->value;
							break;
						case MINUS:
							new_value = -children_info[0]->value;
							break;
						default:
							is_reduction = FALSE;
					}

					if (is_reduction == TRUE)
					{
						node_details = (info_ast_visit_t*)calloc(1, sizeof(info_ast_visit_t));	
						/* store details about the node */
						node_details->is_constant_folded = TRUE;
						node_details->me = node;

						/* create the new node...assumes bitsize of 128 will handle all constant folds */
						new_node = create_tree_node_long_long_number(new_value, 128, node->line_number, node->file_number);

						/* record that this node can be eliminated */
						free_myself = TRUE;
		
						/* replaces from to this new number */
						node_details->from = new_node;
					}
				}
				break;
			case BINARY_OPERATION:
				if ((children_info[0] != NULL) 
					&& (children_info[1] != NULL)
					&& (children_info[0]->is_constant == TRUE) 
					&& (children_info[1]->is_constant == TRUE))
				{
					/* check if we can do a reduction */
					long long new_value;
					short is_reduction = TRUE;
					/* IF can reduce to number constant */
					ast_node_t *new_node;
					
					/* do the operation and store in new node */	
					switch (node->types.operation.op)
					{
						case ADD:
							new_value = children_info[0]->value + children_info[1]->value;
							break;
						case MINUS:
							new_value = children_info[0]->value - children_info[1]->value;
							break;
						case MULTIPLY:
							new_value = children_info[0]->value * children_info[1]->value;
							break;
						case BITWISE_XOR:
							new_value = children_info[0]->value ^ children_info[1]->value;
							break;
						case BITWISE_XNOR:
							new_value = ~(children_info[0]->value ^ children_info[1]->value);
							break;
						case BITWISE_AND:
							new_value = children_info[0]->value & children_info[1]->value;
							break;
						case BITWISE_NAND:
							new_value = ~(children_info[0]->value & children_info[1]->value);
							break;
						case BITWISE_OR:
							new_value = children_info[0]->value | children_info[1]->value;
							break;
						case BITWISE_NOR:
							new_value = ~(children_info[0]->value | children_info[1]->value);
							break;
						case SL:
							new_value = children_info[0]->value << children_info[1]->value;
							break;
						case SR:
							new_value = children_info[0]->value >> children_info[1]->value;
							break;
						case LOGICAL_AND:
							new_value = children_info[0]->value && children_info[1]->value;
							break;
						case LOGICAL_OR:
							new_value = children_info[0]->value || children_info[1]->value;
							break;
						default:
							is_reduction = FALSE;
					}

					if (is_reduction == TRUE)
					{
						node_details = (info_ast_visit_t*)calloc(1, sizeof(info_ast_visit_t));	
						/* store details about the node */
						node_details->is_constant_folded = TRUE;
						node_details->me = node;

						/* create the new node...assumes bitsize of 128 will handle all constant folds */
						new_node = create_tree_node_long_long_number(new_value, 128, node->line_number, node->file_number);
	
						/* record that this node can be eliminated */
						free_myself = TRUE;

						/* return from to this new number */
						node_details->from = new_node;
					}
				}
				break;
			default:
				break;
		}

		/* clean up the allocations in the downward traversal */
		for (i = 0; i < node->num_children; i++)
		{
			if(children_info[i] != NULL)
				free(children_info[i]);
		}
		if (children_info != NULL)
			free(children_info);

		if (free_myself == TRUE)
			/* remove the node */
			free_ast_node(node);
	}

	return node_details;
}
