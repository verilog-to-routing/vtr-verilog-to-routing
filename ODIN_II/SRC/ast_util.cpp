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
#include <ctype.h>
#include <stdarg.h>
#include <math.h>
#include <algorithm>
#include "odin_globals.h"
#include "odin_types.h"

#include "ast_util.h"
#include "odin_util.h"
#include "vtr_util.h"
#include "vtr_memory.h"

char **get_name_of_pins_number(ast_node_t *var_node, int start, int width);
char *get_name_of_pin_number(ast_node_t *var_node, int bit);
void update_tree_tag(ast_node_t *node, int cases, int tagged);

// HIGH LEVEL AST TAG
	static int high_level_id;

	/*---------------------------------------------------------------------------
	 * (function: update_tree)
	 *-------------------------------------------------------------------------*/
	void update_tree_tag(ast_node_t *node, int high_level_block_type_to_search, int tag)
	{
		long i;
		int tagged = tag;
		if (node){

			if(node->type == high_level_block_type_to_search)
				tagged = ++high_level_id;

			if(tagged > -1){
				node->far_tag = tagged;
				node->high_number = node->line_number;
			}

			for (i=0; i < node->num_children; i++)
				update_tree_tag(node->children[i], high_level_block_type_to_search, tagged);
		}
	}

	/*---------------------------------------------------------------------------
	 * (function: add_tag_data)
	 *-------------------------------------------------------------------------*/
	void add_tag_data()
	{
		long i;
		high_level_id = -1;

		ids type = NO_ID;
		if(!strcmp(global_args.high_level_block,"if")){
			type = IF;
		}else if(!strcmp(global_args.high_level_block,"always")){
			type = ALWAYS;
		}else if(!strcmp(global_args.high_level_block,"module")){
			type = MODULE;
		}

		if(type != NO_ID){
			for (i = 0; i < num_modules ; i++)
				update_tree_tag(ast_modules[i],type, -1);
		}
	}

// END HIGH LEVEL TAG

/*---------------------------------------------------------------------------
 * (function: create_node_w_type)
 *-------------------------------------------------------------------------*/
static ast_node_t* create_node_w_type(ids id, int line_number, int file_number, bool update_unique_count)
{
	oassert(id != NO_ID);

	static long unique_count = 0;

	ast_node_t* new_node;

	new_node = (ast_node_t*)vtr::calloc(1, sizeof(ast_node_t));
	oassert(new_node != NULL);

	initial_node(new_node, id, line_number, file_number, unique_count);

	if(update_unique_count)
		unique_count += 1;

	return new_node;
}

ast_node_t* create_node_w_type_no_count(ids id, int line_number, int file_number)
{
	return create_node_w_type(id, line_number, file_number, false);
}

/*---------------------------------------------------------------------------
 * (function: create_node_w_type)
 *-------------------------------------------------------------------------*/
ast_node_t* create_node_w_type(ids id, int line_number, int file_number)
{
	return create_node_w_type(id, line_number, file_number, true);
}

/*---------------------------------------------------------------------------
 * (function: free_assignement_of_node_keep_tree)
 * free a node type but stay in tree structure
 *-------------------------------------------------------------------------*/
void free_assignement_of_node_keep_tree(ast_node_t *node)
{
	if(node){
		int i;
		vtr::free(node->types.identifier);
		switch(node->type){
			case NUMBERS:
				if (node->types.vnumber != nullptr)
					delete node->types.vnumber;
				node->types.vnumber = nullptr;
				break;

			case CONCATENATE:
				for(i=0; i<node->types.concat.num_bit_strings; i++){
					vtr::free(node->types.concat.bit_strings);
				}

			default:
				break;
		}
	}
}
/*---------------------------------------------------------------------------
 * (function: free_single_node)
 * free a node whose type is IDENTIFERS
 *-------------------------------------------------------------------------*/
ast_node_t *free_single_node(ast_node_t *node)
{
	if(node){
		free_assignement_of_node_keep_tree(node);
		vtr::free(node->children);
		vtr::free(node);
	}
	return NULL;

}

/*---------------------------------------------------------------------------
 * (function: free_all_children)
 *-------------------------------------------------------------------------*/
void free_all_children(ast_node_t *node)
{
	long i;

	if (node){
		for (i = 0; i < node->num_children; i++)
			node->children[i] = free_whole_tree(node->children[i]);
		vtr::free(node->children);
		node->children = NULL;
		node->num_children = 0;
	}
	
}

/*---------------------------------------------------------------------------
 * (function: free_whole_tree)
 *-------------------------------------------------------------------------*/
ast_node_t *free_whole_tree(ast_node_t *node)
{
	free_all_children(node);
	return free_single_node(node);
}

/*---------------------------------------------------------------------------------------------
 * (function: create_tree_node_id)
 *-------------------------------------------------------------------------------------------*/
ast_node_t* create_tree_node_id(char* string, int line_number, int /*file_number*/)
{
	ast_node_t* new_node = create_node_w_type(IDENTIFIERS, line_number, current_parse_file);
	new_node->types.identifier = string;

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: create_tree_node_number)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *create_tree_node_number(char *input_number, int line_number, int /* file_number */)
{
	ast_node_t* new_node = create_node_w_type(NUMBERS, line_number, current_parse_file);
	new_node->types.vnumber = new VNumber(input_number);

	return new_node;

}

/*---------------------------------------------------------------------------------------------
 * (function: create_tree_node_number)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *create_tree_node_number(VNumber& input_number, int line_number, int /* file_number */)
{
	ast_node_t* new_node = create_node_w_type(NUMBERS, line_number, current_parse_file);
	new_node->types.vnumber = new VNumber(input_number);

	return new_node;

}

/*---------------------------------------------------------------------------------------------
 * (function: create_tree_node_number)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *create_tree_node_number(long input_number, int line_number, int /* file_number */)
{
	ast_node_t* new_node = create_node_w_type(NUMBERS, line_number, current_parse_file);
	new_node->types.vnumber = new VNumber(input_number);

	return new_node;

}

/*---------------------------------------------------------------------------
 * (function: allocate_children_to_node)
 *-------------------------------------------------------------------------*/
void allocate_children_to_node(ast_node_t* node, int num_children, ...)
{
	va_list ap;
	int i;

	/* allocate space for the children */
	node->children = (ast_node_t**)vtr::malloc(sizeof(ast_node_t*)*num_children);
	node->num_children = num_children;

	/* set the virtual arguments */
	va_start(ap, num_children);

	for (i = 0; i < num_children; i++)
	{
		node->children[i] = va_arg(ap, ast_node_t*);
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: add_child_to_node)
 *-------------------------------------------------------------------------------------------*/
void add_child_to_node(ast_node_t* node, ast_node_t *child)
{
	/* Handle case where we have an empty statement. */
	if (child == NULL)
		return;

	/* allocate space for the children */
	node->children = (ast_node_t**)vtr::realloc(node->children, sizeof(ast_node_t*)*(node->num_children+1));
	node->num_children ++;
	node->children[node->num_children-1] = child;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_child_to_node)
 *-------------------------------------------------------------------------------------------*/
void add_child_at_the_beginning_of_the_node(ast_node_t* node, ast_node_t *child)
{
    long i;
    ast_node_t *ref, *ref1;
	/* Handle case where we have an empty statement. */
	if (child == NULL)
		return;



	/* allocate space for the children */
	node->children = (ast_node_t**)vtr::realloc(node->children, sizeof(ast_node_t*)*(node->num_children+1));
    node->num_children ++;

    ref = node->children[0];

    for(i = 1; i < node->num_children; i++){
        ref1 = node->children[i];
        node->children[i] = ref;
        ref = ref1;
    }

    node->children[0] = child;

}

/*---------------------------------------------------------------------------------------------
 * (function: expand_node_list_at)
 *-------------------------------------------------------------------------------------------*/
ast_node_t **expand_node_list_at(ast_node_t **list, long old_size, long to_add, long start_idx)
{
	if (list)
	{
		long new_size = old_size + to_add;
		list = (ast_node_t **)vtr::realloc(list, sizeof(ast_node_t*) * new_size);

		long i;
		for (i = new_size-1; i >= (start_idx + to_add); i--)
		{
			list[i] = list[i-to_add];
		}
		return list;
	}
	return NULL;
}

void resolve_concat_sizes(ast_node_t *concat_top, char *instance_name_prefix)
{
	long i;
	if (concat_top->type == CONCATENATE)
	{
		for (i = 0; i < concat_top->num_children; i++)
		{
			resolve_concat_sizes(concat_top->children[i], instance_name_prefix);
		}
	}
	else if (concat_top->type == IDENTIFIERS)
	{
		ast_node_t *temp_node = resolve_node(NULL, instance_name_prefix, concat_top);
		if (temp_node != concat_top)
		{
			oassert(temp_node->type == NUMBERS);
			resolve_concat_sizes(temp_node, instance_name_prefix);
			free(temp_node);
		}
		else
		{
			char *temp_string = make_full_ref_name(NULL, NULL, NULL, concat_top->types.identifier, -1);
			long sc_spot;
			if ((sc_spot = sc_lookup_string(local_symbol_table_sc, temp_string)) == -1)
			{
				error_message(NETLIST_ERROR, concat_top->line_number, concat_top->file_number, "Missing declaration of this symbol %s\n", temp_string);
			}
			else
			{
				temp_node = ((ast_node_t*)local_symbol_table_sc->data[sc_spot]);

				if (temp_node->children[1] == NULL && temp_node->children[5] != NULL)
				{				
					resolve_concat_sizes(temp_node->children[5], instance_name_prefix);
				}
				else
				{
					oassert(temp_node->children[3] == NULL);
				}
			}
			vtr::free(temp_string);
		}
	}
	else if (concat_top->type == BINARY_OPERATION || concat_top->type == UNARY_OPERATION)
	{
		for (i = 0; i < concat_top->num_children; i++)
		{
			resolve_concat_sizes(concat_top->children[i], instance_name_prefix);
		}
	}
	else if (concat_top->type == IF_Q)
	{
		/* check true/false expressions */
		resolve_concat_sizes(concat_top->children[1], instance_name_prefix);
		resolve_concat_sizes(concat_top->children[2], instance_name_prefix);
	}
	else if (concat_top->type == NUMBERS)
	{
		/* verify that the number that this represents is sized */
		if (!(concat_top->types.vnumber->is_defined_size()))
		{
			error_message(NETLIST_ERROR, concat_top->line_number, concat_top->file_number, "%s", "Unsized constants cannot be concatenated.\n");
		}
	}
	else if (!(concat_top->type == ARRAY_REF || concat_top->type == RANGE_REF))
	{
		error_message(NETLIST_ERROR, concat_top->line_number, concat_top->file_number, "%s", "Unsupported operation within a concatenation.\n");
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: make_concat_into_list_of_strings)
 * 	0th idx will be the MSbit
 *-------------------------------------------------------------------------------------------*/
void make_concat_into_list_of_strings(ast_node_t *concat_top, char *instance_name_prefix)
{
	long i;
	int j;
	ast_node_t *rnode[3];

	concat_top->types.concat.num_bit_strings = 0;
	concat_top->types.concat.bit_strings = NULL;

	/* recursively do all embedded concats */
	for (i = 0; i < concat_top->num_children; i++)
	{
		if (concat_top->children[i]->type == CONCATENATE)
		{
			make_concat_into_list_of_strings(concat_top->children[i], instance_name_prefix);
		}
	}

	for (i = 0; i < concat_top->num_children; i++)
	{
		if (concat_top->children[i]->type == IDENTIFIERS)
		{
			char *temp_string = make_full_ref_name(NULL, NULL, NULL, concat_top->children[i]->types.identifier, -1);
			long sc_spot;
			if ((sc_spot = sc_lookup_string(local_symbol_table_sc, temp_string)) == -1)
			{
				error_message(NETLIST_ERROR, concat_top->line_number, concat_top->file_number, "Missing declaration of this symbol %s\n", temp_string);
			}
			else
			{
				if (((ast_node_t*)local_symbol_table_sc->data[sc_spot])->children[1] == NULL)
				{
					concat_top->types.concat.num_bit_strings ++;
					concat_top->types.concat.bit_strings = (char**)vtr::realloc(concat_top->types.concat.bit_strings, sizeof(char*)*(concat_top->types.concat.num_bit_strings));
					concat_top->types.concat.bit_strings[concat_top->types.concat.num_bit_strings-1] = get_name_of_pin_at_bit(concat_top->children[i], -1, instance_name_prefix);
				}
				else if (((ast_node_t*)local_symbol_table_sc->data[sc_spot])->children[3] == NULL)
				{
					/* reverse thorugh the range since highest bit in index will be lower in the string indx */
					rnode[1] = resolve_node(NULL, instance_name_prefix, ((ast_node_t*)local_symbol_table_sc->data[sc_spot])->children[1]);
					rnode[2] = resolve_node(NULL, instance_name_prefix, ((ast_node_t*)local_symbol_table_sc->data[sc_spot])->children[2]);
					oassert(rnode[1]->type == NUMBERS && rnode[2]->type == NUMBERS);

					///TODO	WHats this?? compareo bit string but uses value ??? value is honestly not the right thing to look for...
					// we should restrict ODIN to use binary representation only for easier support of 'x' and 'z' value..
					// or at least it's the only thing that makes sense.
					// also this gives us the ability to write our own math class for binary and ave better control of what is happening and better sense of it TO.
					// this causes bugs.. theres a patchy workaround put in bit string but we need a better permannet fix.
					for (j = rnode[1]->types.vnumber->get_value() - rnode[2]->types.vnumber->get_value(); j >= 0; j--)
					{
						concat_top->types.concat.num_bit_strings ++;
						concat_top->types.concat.bit_strings = (char**)vtr::realloc(concat_top->types.concat.bit_strings, sizeof(char*)*(concat_top->types.concat.num_bit_strings));
						concat_top->types.concat.bit_strings[concat_top->types.concat.num_bit_strings-1] = get_name_of_pin_at_bit(concat_top->children[i], j, instance_name_prefix);
					}
				}
				else if (((ast_node_t*)local_symbol_table_sc->data[sc_spot])->children[3] != NULL)
				{
					oassert(FALSE);
				}
			}
			vtr::free(temp_string);
		}
		else if (concat_top->children[i]->type == ARRAY_REF)
		{
			concat_top->types.concat.num_bit_strings ++;
			concat_top->types.concat.bit_strings = (char**)vtr::realloc(concat_top->types.concat.bit_strings, sizeof(char*)*(concat_top->types.concat.num_bit_strings));
			concat_top->types.concat.bit_strings[concat_top->types.concat.num_bit_strings-1] = get_name_of_pin_at_bit(concat_top->children[i], 0, instance_name_prefix);
		}
		else if (concat_top->children[i]->type == RANGE_REF)
		{
			rnode[1] = resolve_node(NULL, instance_name_prefix, concat_top->children[i]->children[1]);
			rnode[2] = resolve_node(NULL, instance_name_prefix, concat_top->children[i]->children[2]);
			oassert(rnode[1]->type == NUMBERS && rnode[2]->type == NUMBERS);
			oassert(rnode[1]->types.vnumber->get_value() >= rnode[2]->types.vnumber->get_value());
			int width = abs(rnode[1]->types.vnumber->get_value() - rnode[2]->types.vnumber->get_value()) + 1;

			//for (j = rnode[1]->types.vnumber->get_value() - rnode[2]->types.vnumber->get_value(); j >= 0; j--)
			// Changed to forward to fix concatenation bug.
			for (j = 0; j < width; j++)
			{
				concat_top->types.concat.num_bit_strings ++;
				concat_top->types.concat.bit_strings = (char**)vtr::realloc(concat_top->types.concat.bit_strings, sizeof(char*)*(concat_top->types.concat.num_bit_strings));
				concat_top->types.concat.bit_strings[concat_top->types.concat.num_bit_strings-1] =
					get_name_of_pin_at_bit(concat_top->children[i], ((rnode[1]->types.vnumber->get_value() - rnode[2]->types.vnumber->get_value()))-j, instance_name_prefix);
			}
		}
		else if (concat_top->children[i]->type == NUMBERS)
		{
			if (concat_top->children[i]->types.vnumber->is_defined_size())
			{
				// Changed to reverse to fix concatenation bug.
				for (j = concat_top->children[i]->types.vnumber->size()-1; j>= 0; j--)
				{
					concat_top->types.concat.num_bit_strings ++;
					concat_top->types.concat.bit_strings = (char**)vtr::realloc(concat_top->types.concat.bit_strings, sizeof(char*)*(concat_top->types.concat.num_bit_strings));
					concat_top->types.concat.bit_strings[concat_top->types.concat.num_bit_strings-1] = get_name_of_pin_at_bit(concat_top->children[i], j, instance_name_prefix);
				
				}
			}
			else
			{
				error_message(NETLIST_ERROR, concat_top->line_number, concat_top->file_number, "%s", "Unsized constants cannot be concatenated.\n");
			}
		}
		else if (concat_top->children[i]->type == CONCATENATE)
		{
			/* forward through list since we build concatenate list in idx order of MSB at index 0 and LSB at index list_size */
			for (j = 0; j < concat_top->children[i]->types.concat.num_bit_strings; j++)
			{
				concat_top->types.concat.num_bit_strings ++;
				concat_top->types.concat.bit_strings = (char**)vtr::realloc(concat_top->types.concat.bit_strings, sizeof(char*)*(concat_top->types.concat.num_bit_strings));
				concat_top->types.concat.bit_strings[concat_top->types.concat.num_bit_strings-1] = get_name_of_pin_at_bit(concat_top->children[i], j, instance_name_prefix);
			}
		}
		else {
			error_message(NETLIST_ERROR, concat_top->line_number, concat_top->file_number, "%s", "Unsupported operation within a concatenation.\n");
		}
	}
}

/*---------------------------------------------------------------------------
 * (function: change_to_number_node)
 * change the original AST node to a NUMBER node or change the value of the node
 * originate from the function: create_tree_node_number() in ast_util.c
 *-------------------------------------------------------------------------*/
void change_to_number_node(ast_node_t *node, long value)
{
	char *temp_ident = NULL;
	if (node->types.identifier != NULL) 
	{
		temp_ident = strdup(node->types.identifier);
	}
	free_assignement_of_node_keep_tree(node);
	free_all_children(node);

	node->type = NUMBERS;
	node->types.identifier = temp_ident;
	node->types.vnumber = new VNumber(value);
}

/*---------------------------------------------------------------------------------------------
 * (function: get_name of_port_at_bit)
 * 	Assume module connections can be one of: Array entry, Concat, Signal, Array range reference
 *-------------------------------------------------------------------------------------------*/
char *get_name_of_var_declare_at_bit(ast_node_t *var_declare, int bit)
{
	char *return_string = NULL;

	/* calculate the port details */
	if (var_declare->children[1] == NULL)
	{
		oassert(bit == 0);
		return_string = make_full_ref_name(NULL, NULL, NULL, var_declare->children[0]->types.identifier, -1);
	}
	else if (var_declare->children[3] == NULL)
	{
		oassert(var_declare->children[2]->type == NUMBERS);
		return_string = make_full_ref_name(NULL, NULL, NULL, var_declare->children[0]->types.identifier, var_declare->children[2]->types.vnumber->get_value()+bit);
	}
	else if (var_declare->children[3] != NULL)
	{
		/* MEMORY output */
		oassert(FALSE);
	}

	return return_string;
}

/*---------------------------------------------------------------------------------------------
 * (function: get_name of_port_at_bit)
 * 	Assume module connections can be one of: Array entry, Concat, Signal, Array range reference
 *-------------------------------------------------------------------------------------------*/
char *get_name_of_pin_at_bit(ast_node_t *var_node, int bit, char *instance_name_prefix)
{
	char *return_string = NULL;
	ast_node_t *rnode[3];

	if (var_node->type == ARRAY_REF)
	{
		var_node->children[1] = resolve_node(NULL, instance_name_prefix, var_node->children[1]);
		oassert(var_node->children[0]->type == IDENTIFIERS);
		oassert(var_node->children[1]->type == NUMBERS);
		return_string = make_full_ref_name(NULL, NULL, NULL, var_node->children[0]->types.identifier, (int)var_node->children[1]->types.vnumber->get_value());
	}
	else if (var_node->type == RANGE_REF)
	{		
		oassert(bit >= 0);

		rnode[1] = resolve_node(NULL, instance_name_prefix, var_node->children[1]);
		rnode[2] = resolve_node(NULL, instance_name_prefix, var_node->children[2]);
		oassert(var_node->children[0]->type == IDENTIFIERS);
		oassert(rnode[1]->type == NUMBERS);
		oassert(rnode[2]->type == NUMBERS);
		oassert(rnode[1]->types.vnumber->get_value() >= rnode[2]->types.vnumber->get_value()+bit);

		return_string = make_full_ref_name(NULL, NULL, NULL, var_node->children[0]->types.identifier, rnode[2]->types.vnumber->get_value()+bit);
	}
	else if ((var_node->type == IDENTIFIERS) && (bit == -1))
	{
		return_string = make_full_ref_name(NULL, NULL, NULL, var_node->types.identifier, -1);
	}
	else if (var_node->type == IDENTIFIERS)
	{
		long sc_spot;
		int pin_index;

		if ((sc_spot = sc_lookup_string(local_symbol_table_sc, var_node->types.identifier)) == -1)
		{
			error_message(NETLIST_ERROR, var_node->line_number, var_node->file_number, "Missing declaration of this symbol %s\n", var_node->types.identifier);
		}

		if (((ast_node_t*)local_symbol_table_sc->data[sc_spot])->children[1] == NULL)
		{
			pin_index = bit;
		}
		else if (((ast_node_t*)local_symbol_table_sc->data[sc_spot])->children[3] == NULL)
		{
			oassert(((ast_node_t*)local_symbol_table_sc->data[sc_spot])->children[2]->type == NUMBERS);
			pin_index = ((ast_node_t*)local_symbol_table_sc->data[sc_spot])->children[2]->types.vnumber->get_value() + bit;
		}
		else
			oassert(FALSE);

		return_string = make_full_ref_name(NULL, NULL, NULL, var_node->types.identifier, pin_index);
	}
	else if (var_node->type == NUMBERS)
	{
		return_string = get_name_of_pin_number(var_node, bit);
	}
	else if (var_node->type == CONCATENATE)
	{
		if (var_node->types.concat.num_bit_strings == 0)
		{
			return_string = NULL;
			oassert(FALSE);
		}
		else
		{
			if (var_node->types.concat.num_bit_strings == -1)
			{
				/* If this hasn't been made into a string list then do it */
				make_concat_into_list_of_strings(var_node, instance_name_prefix);
			}

			return_string = (char*)vtr::malloc(sizeof(char)*strlen(var_node->types.concat.bit_strings[bit])+1);
			odin_sprintf(return_string, "%s", var_node->types.concat.bit_strings[bit]);
		}
	}
	else
	{
		return_string = NULL;

		error_message(NETLIST_ERROR, var_node->line_number, var_node->file_number, "Unsupported variable type. var_node->type = %ld\n",var_node->type);
	}

	return return_string;
}

char **get_name_of_pins_number(ast_node_t *var_node, int /*start*/, int width)
{
	char **return_string;
	oassert(var_node->type == NUMBERS);

	return_string = (char**)vtr::malloc(sizeof(char*)*width);
	int i;
	for (i = 0; i < width; i++)//
	{
		return_string[i] = get_name_of_pin_number(var_node, i);
	}
	return return_string;
}

char *get_name_of_pin_number(ast_node_t *var_node, int bit)
{
	oassert(var_node->type == NUMBERS);
	char *return_string = NULL;

	if (bit == -1)
		bit = 0;

	BitSpace::bit_value_t c = var_node->types.vnumber->get_bit_from_lsb(bit);
	switch(c)
	{
		case BitSpace::_1: return_string = vtr::strdup(ONE_VCC_CNS); break;
		case BitSpace::_0: return_string = vtr::strdup(ZERO_GND_ZERO); break;
		case BitSpace::_x: return_string = vtr::strdup(ZERO_GND_ZERO); break;
		default: 
			error_message(NETLIST_ERROR, var_node->line_number, var_node->file_number, "Unrecognised character %c in binary string \"%s\"!\n", c, var_node->types.vnumber->to_bit_string().c_str());
			break;
	}

	return return_string;
}

/*---------------------------------------------------------------------------------------------
 * (function: get_name_of_pins
 * 	Assume module connections can be one of: Array entry, Concat, Signal, Array range reference
 * 	Return a list of strings
 *-------------------------------------------------------------------------------------------*/
char_list_t *get_name_of_pins(ast_node_t *var_node, char *instance_name_prefix)
{ 
	char **return_string = NULL;
	char_list_t *return_list = (char_list_t*)vtr::malloc(sizeof(char_list_t));
	ast_node_t *rnode[3];

	int i;
	int width = 0;

	if (var_node->type == ARRAY_REF)
	{
		width = 1;
		return_string = (char**)vtr::malloc(sizeof(char*));
		rnode[1] = resolve_node(NULL, instance_name_prefix, var_node->children[1]);
		oassert(rnode[1] && rnode[1]->type == NUMBERS);
		oassert(var_node->children[0]->type == IDENTIFIERS);
		return_string[0] = make_full_ref_name(NULL, NULL, NULL, var_node->children[0]->types.identifier, rnode[1]->types.vnumber->get_value());
	}
	else if (var_node->type == RANGE_REF)
	{
		rnode[0] = resolve_node(NULL, instance_name_prefix, var_node->children[0]);
		rnode[1] = resolve_node(NULL, instance_name_prefix, var_node->children[1]);
		rnode[2] = resolve_node(NULL, instance_name_prefix, var_node->children[2]);
		oassert(rnode[1]->type == NUMBERS && rnode[2]->type == NUMBERS);
		width = abs(rnode[1]->types.vnumber->get_value() - rnode[2]->types.vnumber->get_value()) + 1;
		if (rnode[0]->type == IDENTIFIERS)
		{
			return_string = (char**)vtr::malloc(sizeof(char*)*width);
			for (i = 0; i < width; i++)
				return_string[i] = make_full_ref_name(NULL, NULL, NULL, rnode[0]->types.identifier, rnode[2]->types.vnumber->get_value()+i);
		}
		else
		{
			oassert(rnode[0]->type == NUMBERS);
			return_string = get_name_of_pins_number(rnode[0], rnode[2]->types.vnumber->get_value(), width);
		}
	}
	else if (var_node->type == IDENTIFIERS)
	{
		/* need to look in the symbol table for details about this identifier (i.e. is it a port) */
		long sc_spot;
		ast_node_t *sym_node;

		// try and resolve var_node
		sym_node = resolve_node(NULL, instance_name_prefix, var_node);

		if (sym_node == var_node)
		{
			char *temp_string = make_full_ref_name(NULL, NULL, NULL, var_node->types.identifier, -1);

            if ((sc_spot = sc_lookup_string(function_local_symbol_table_sc, temp_string)) > -1)
			{
                sym_node = (ast_node_t*)function_local_symbol_table_sc->data[sc_spot];
			}
		    else if ((sc_spot = sc_lookup_string(local_symbol_table_sc, temp_string)) > -1)
			{
                sym_node = (ast_node_t*)local_symbol_table_sc->data[sc_spot];
			}
            else 
			{
                error_message(NETLIST_ERROR, var_node->line_number, var_node->file_number, "Missing declaration of this symbol %s\n", temp_string);
            }
			
			vtr::free(temp_string);

			if (sym_node->children[1] == NULL || sym_node->type == BLOCKING_STATEMENT)
			{
				width = 1;
				return_string = (char**)vtr::malloc(sizeof(char*)*width);
				return_string[0] = make_full_ref_name(NULL, NULL, NULL, var_node->types.identifier, -1);
			}
			else if (sym_node->children[3] == NULL)
			{
				int index = 0;
				rnode[1] = resolve_node(NULL, instance_name_prefix, sym_node->children[1]);
				rnode[2] = resolve_node(NULL, instance_name_prefix, sym_node->children[2]);
				oassert(rnode[1]->type == NUMBERS && rnode[2]->type == NUMBERS);
				width = (rnode[1]->types.vnumber->get_value() - rnode[2]->types.vnumber->get_value() + 1);
				return_string = (char**)vtr::malloc(sizeof(char*)*width);
				for (i = 0; i < width; i++)
				{
					return_string[index] = make_full_ref_name(NULL, NULL, NULL, var_node->types.identifier,
						i+rnode[2]->types.vnumber->get_value());
					index++;
				}
			}
			else if (sym_node->children[3] != NULL)
			{
				oassert(FALSE);
			}
		}
		else
		{
			oassert(sym_node->type == NUMBERS);
			width = sym_node->types.vnumber->size();
			return_string = get_name_of_pins_number(sym_node, 0, width);
		}
	}
	else if (var_node->type == NUMBERS)
	{
		width = var_node->types.vnumber->size();
		return_string = get_name_of_pins_number(var_node, 0, width);
	}
	else if (var_node->type == CONCATENATE)
	{
		if (var_node->types.concat.num_bit_strings == 0)
		{
			oassert(FALSE);
		}
		else
		{
			if (var_node->types.concat.num_bit_strings == -1)
			{
				/* If this hasn't been made into a string list then do it */
				make_concat_into_list_of_strings(var_node, instance_name_prefix);
			}

			width = var_node->types.concat.num_bit_strings;
			return_string = (char**)vtr::malloc(sizeof(char*)*width);
			for (i = 0; i < width; i++) // 0th bit is MSB so need to access reverse
			{
				return_string[i] = (char*)vtr::malloc(sizeof(char)*strlen(var_node->types.concat.bit_strings[var_node->types.concat.num_bit_strings-i-1])+1);
				odin_sprintf(return_string[i], "%s", var_node->types.concat.bit_strings[var_node->types.concat.num_bit_strings-i-1]);
			}
		}
	}
	else
	{
		oassert(FALSE);
	}

	return_list->strings = return_string;
	return_list->num_strings = width;

	return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function: get_name_of_pins_with_prefix
 *-------------------------------------------------------------------------------------------*/
char_list_t *get_name_of_pins_with_prefix(ast_node_t *var_node, char *instance_name_prefix)
{
	int i;
	char_list_t *return_list;
	char *temp_str;

	/* get the list */
	return_list = get_name_of_pins(var_node, instance_name_prefix);

	for (i = 0; i < return_list->num_strings; i++)
	{
		temp_str = vtr::strdup(return_list->strings[i]);
		vtr::free(return_list->strings[i]);
		return_list->strings[i] = make_full_ref_name(instance_name_prefix, NULL, NULL, temp_str, -1);
		vtr::free(temp_str);
	}

	return return_list;
}

/*----------------------------------------------------------------------------
 * (function: resolve_node)
 *--------------------------------------------------------------------------*/
/**
 * Recursively resolves an IDENTIFIER to a parameter into its actual value,
 * by looking it up in the local_param_table_sc
 * Also try and fold any BINARY_OPERATIONs now that an IDENTIFIER has been
 * resolved
 */
ast_node_t *resolve_node(STRING_CACHE *local_param_table_sc, char *module_name, ast_node_t *node)
{
	long sc_spot = -1;

	// Not sure this should even be used.
	if(local_param_table_sc == NULL 
	&& module_name != NULL)
	{
		sc_spot = sc_lookup_string(global_param_table_sc, module_name);
		if (sc_spot != -1)
		{
			local_param_table_sc = (STRING_CACHE *)global_param_table_sc->data[sc_spot];
		}
	}

	if (node)
	{
		oassert(node->type != NO_ID);
		ast_node_t *node_copy;
		node_copy = ast_node_copy(node);
		node_copy->children = (ast_node_t **)vtr::calloc(node_copy->num_children,sizeof(ast_node_t*));

		long i;
		for (i = 0; i < node->num_children; i++)
		{
			node_copy->children[i] = resolve_node(local_param_table_sc, module_name, node->children[i]);
		}

		ast_node_t *newNode = NULL;
		switch (node->type){

			case IDENTIFIERS:
			{
				if(local_param_table_sc != NULL)
				{
					sc_spot = sc_lookup_string(local_param_table_sc, node->types.identifier);
					if (sc_spot != -1){
						newNode = ast_node_deep_copy((ast_node_t *)local_param_table_sc->data[sc_spot]);
					}
				}
			}
			break;

			case UNARY_OPERATION:
				newNode = fold_unary(node_copy);
				break;

			case BINARY_OPERATION:
				newNode = fold_binary(node_copy);
				break;

			default:
				break;
		}

		if (node_is_constant(newNode)){
			newNode->shared_node = node->shared_node;

			// /* clean up */
			// if (node->type != IDENTIFIERS) {
			// 	node = free_whole_tree(node);
			// }
			
			node = newNode;
		}

		vtr::free(node_copy->children);
		vtr::free(node_copy);
	}
	return node;
}

/*----------------------------------------------------------------------------
 * (function: resolve_ast_node)
 *--------------------------------------------------------------------------*/
/**
 * Recursively resolves an IDENTIFIER to a parameter into its actual value,
 * by looking it up in the local_param_table_sc
 * Also try and fold any BINARY_OPERATIONs now that an IDENTIFIER has been
 * resolved
 */

ast_node_t *resolve_ast_node(STRING_CACHE *local_param_table_sc, short initial, char *module_name, ast_node_t *node)
{
	if (node)
	{
		ast_node_t *node_copy;
		node_copy = ast_node_copy(node);
		node_copy->children = (ast_node_t **)vtr::calloc(node_copy->num_children,sizeof(ast_node_t*));

		long i;
		for (i = 0; i < node->num_children; i++){
			node_copy->children[i] = resolve_ast_node(local_param_table_sc, initial, module_name, node->children[i]);
		}
		ast_node_t *newNode = NULL;
		switch (node->type){
			
			case UNARY_OPERATION:
				newNode = fold_unary(node_copy);
				break;

			case BINARY_OPERATION:
				newNode = fold_binary(node_copy);
				break;

			default:
				break;
		}

		if (node_is_ast_constant(newNode, local_param_table_sc)){
			newNode->shared_node = node->shared_node;

			/* clean up */
			if (node->type != IDENTIFIERS) {
				node = free_whole_tree(node);
			}
			
			node = newNode;
		}

		vtr::free(node_copy->children);
		vtr::free(node_copy);
	}
	return node;
}

/*----------------------------------------------------------------------------
 * (function: make_module_param_name)
 *--------------------------------------------------------------------------*/
/**
 * Make a unique name for a module based on its parameter list
 * e.g. for a "mod #(0,1,2,3) a(b,c,d)" instantiation you get name___0_1_2_3
 */
char *make_module_param_name(STRING_CACHE *defines_for_module_sc, ast_node_t *module_param_list, char *module_name)
{
	char *module_param_name = (char*)vtr::malloc((strlen(module_name)+1024) * sizeof(char));
	strcpy(module_param_name, module_name);

	if (module_param_list)
	{
		long i;
		oassert(module_param_list->num_children > 0);
		strcat(module_param_name, "___");
		for (i = 0; i < module_param_list->num_children; i++)
		{
			if (module_param_list->children[i]->children[5]) 
			{
				ast_node_t *node = resolve_node(defines_for_module_sc, module_name, module_param_list->children[i]->children[5]);
				oassert(node->type == NUMBERS);
				odin_sprintf(module_param_name, "%s_%ld", module_param_name, node->types.vnumber->get_value());
			}
		}
	}

	return module_param_name;
}




/*---------------------------------------------------------------------------------------------
 * (function: move_ast_node)
 * move node from src to dest
 *-------------------------------------------------------------------------------------------*/
void move_ast_node(ast_node_t *src, ast_node_t *dest, ast_node_t *node)
{
	int i, j;
	int number;
	number = src->num_children;
	for(i = 0; i < number; i++)
	{
		if(src->children[i]->unique_count == node->unique_count)
		{
			number = number - 1;
			src->num_children = number;
			for(j = i; j < number; j++)
			{
				src->children[j] = src->children[j + 1];
			}
		}
	}
	add_child_to_node(dest, node);
}

/*---------------------------------------------------------------------------------------------
 * (function: ast_node_deep_copy)
 * copy node and its children recursively; return new subtree
 *-------------------------------------------------------------------------------------------*/
ast_node_t *ast_node_deep_copy(ast_node_t *node){
	ast_node_t *node_copy = ast_node_copy(node);

	if (node && node_copy)
	{
		//Create a new child list;
		node_copy->children = (ast_node_t**)vtr::malloc(sizeof(ast_node_t*)*node->num_children);

		//Recursively copy its children
		for(long i = 0; i < node->num_children; i++){
			node_copy->children[i] = ast_node_deep_copy(node->children[i]);
		}
	}
  
	return node_copy;
}

/*---------------------------------------------------------------------------------------------
 * (function: ast_node_copy)
 * copy node; return new node
 *-------------------------------------------------------------------------------------------*/
ast_node_t *ast_node_copy(ast_node_t *node){
	ast_node_t *node_copy;

	if(node == NULL){
		return NULL;
	}

	//Copy node
	node_copy = (ast_node_t *)vtr::malloc(sizeof(ast_node_t));
	memcpy(node_copy, node, sizeof(ast_node_t));	

	//Copy contents
	if (node->type == NUMBERS && node->types.vnumber)
		node_copy->types.vnumber = new VNumber((*node->types.vnumber));

	node_copy->types.identifier = vtr::strdup(node->types.identifier);
	node_copy->children = NULL;

	return node_copy;
}

/*---------------------------------------------------------------------------------------------
 * (function: calculate_unary)
 * TODO ! what does verilog say about !d'00001 ??
 * we currently make it as small as possible(+1) such that d'00001 becomes (in bin) 01
 * so it would become 10, is that what is defined?
 *-------------------------------------------------------------------------------------------*/
ast_node_t *fold_unary(ast_node_t *node)
{
	operation_list op_id = node->types.operation.op;
	ast_node_t *child_0 = node->children[0];

	if(node_is_constant(child_0))
	{
		short success = FALSE;
		VNumber voperand_0 = *(child_0->types.vnumber);
		VNumber vresult;

		switch (op_id){
			case LOGICAL_NOT:
				vresult = V_LOGICAL_NOT(voperand_0);
				success = TRUE;
				break;

			case BITWISE_NOT:
				vresult = V_BITWISE_NOT(voperand_0);
				success = TRUE;
				break;

			case MINUS:
				vresult = V_MINUS(voperand_0);
				success = TRUE;
				break;

			case ADD:
				vresult = V_ADD(voperand_0);
				success = TRUE;
				break;

			case BITWISE_OR:
				vresult = V_BITWISE_OR(voperand_0);
				success = TRUE;
				break;

			case BITWISE_NAND:
				vresult = V_BITWISE_NAND(voperand_0);
				success = TRUE;
				break;

			case BITWISE_NOR:
				vresult = V_BITWISE_NOR(voperand_0);
				success = TRUE;
				break;

			case BITWISE_XNOR:
				vresult = V_BITWISE_XNOR(voperand_0);
				success = TRUE;
				break;

			case BITWISE_XOR:
				vresult = V_BITWISE_XOR(voperand_0);
				success = TRUE;
				break;

			case CLOG2:
				if(voperand_0.size() > ODIN_STD_BITWIDTH)
					warning_message(PARSE_ERROR, child_0->line_number, child_0->file_number, "argument is %ld-bits but ODIN limit is %lu-bits \n",voperand_0.size(),ODIN_STD_BITWIDTH);

				vresult = VNumber(clog2(voperand_0.get_value(), voperand_0.size()));
				success = TRUE;
				break;

			default:
				break;
		}

		if(success){
			ast_node_t *new_num = create_tree_node_number(vresult, child_0->line_number, child_0->file_number);
			return new_num;		
		}
	}
	return NULL;
}

/*---------------------------------------------------------------------------------------------
 * (function: calculate_binary)
 *-------------------------------------------------------------------------------------------*/
ast_node_t * fold_binary(ast_node_t *node)
{
	operation_list op_id = node->types.operation.op;
	ast_node_t *child_0 = node->children[0];
	ast_node_t *child_1 = node->children[1];

	if(node_is_constant(child_0) &&  node_is_constant(child_1))
	{
		short success = FALSE;
		VNumber voperand_0 = *(child_0->types.vnumber);
		VNumber voperand_1 = *(child_1->types.vnumber);
		// long size_0 = ;
		// long size_1;
		VNumber vresult;

		switch (op_id){
			case ADD:
				vresult = V_ADD(voperand_0, voperand_1);
				success = TRUE;
				break;

			case MINUS:
				vresult = V_MINUS(voperand_0, voperand_1);
				success = TRUE;
				break;

			case MULTIPLY:
				vresult = V_MULTIPLY(voperand_0, voperand_1);
				success = TRUE;
				break;

			case DIVIDE:
				vresult = V_DIV(voperand_0, voperand_1);
				success = TRUE;
				break;

			case BITWISE_XOR:
				vresult = V_BITWISE_XOR(voperand_0, voperand_1);
				success = TRUE;
				break;

			case BITWISE_XNOR:
				vresult = V_BITWISE_XNOR(voperand_0, voperand_1);
				success = TRUE;
				break;

			case BITWISE_AND:
				vresult = V_BITWISE_AND(voperand_0, voperand_1);
				success = TRUE;
				break;

			case BITWISE_NAND:
				vresult = V_BITWISE_NAND(voperand_0, voperand_1);
				success = TRUE;
				break;

			case BITWISE_OR:
				vresult = V_BITWISE_OR(voperand_0, voperand_1);
				success = TRUE;
				break;

			case BITWISE_NOR:
				vresult = V_BITWISE_NOR(voperand_0, voperand_1);
				success = TRUE;
				break;

			case SL:
				vresult = V_SHIFT_LEFT(voperand_0, voperand_1);
				success = TRUE;
				break;

			case SR:
				vresult = V_SHIFT_RIGHT(voperand_0, voperand_1);
				success = TRUE;
				break;

            case ASR:
			{
				vresult = V_SIGNED_SHIFT_RIGHT(voperand_0, voperand_1);
                success = TRUE;
                break;
			}
			case LOGICAL_AND:
				vresult = V_LOGICAL_AND(voperand_0, voperand_1);
				success = TRUE;
				break;

			case LOGICAL_OR:
				vresult = V_LOGICAL_OR(voperand_0, voperand_1);
				success = TRUE;
				break;

			case MODULO:
				vresult = V_MOD(voperand_0, voperand_1);
				success = TRUE;
				break;

			case LT:
				vresult = V_LT(voperand_0, voperand_1);
				success = TRUE;
				break;

			case GT:
				vresult = V_GT(voperand_0, voperand_1);
				success = TRUE;
				break;

			case LOGICAL_EQUAL:
				vresult = V_LE(voperand_0, voperand_1);
				success = TRUE;
				break;

			case NOT_EQUAL:
				vresult = V_NOT_EQUAL(voperand_0, voperand_1);
				success = TRUE;
				break;

			case LTE:
				vresult = V_LE(voperand_0, voperand_1);
				success = TRUE;
				break;

			case GTE:
				vresult = V_GE(voperand_0, voperand_1);
				success = TRUE;
				break;

			case CASE_EQUAL:
				vresult = V_CASE_EQUAL(voperand_0, voperand_1);
				success = TRUE;
				break;

			case CASE_NOT_EQUAL:
				vresult = V_CASE_NOT_EQUAL(voperand_0, voperand_1);
				success = TRUE;
				break;

			default:
				break;
		}

		if(success){
			ast_node_t *new_num = create_tree_node_number(vresult, child_0->line_number, child_0->file_number);
			return new_num;
		}
	}
	return NULL;
}

/*---------------------------------------------------------------------------------------------
 * (function: node_is_constant)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *node_is_constant(ast_node_t *node){
	if (node && 
		node->type == NUMBERS && 
		node->types.vnumber != nullptr &&
		!(node->types.vnumber->is_dont_care_string()))
	{
		return node;
	}
	return NULL;
}

/*---------------------------------------------------------------------------------------------
 * (function: node_is_ast_constant)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *node_is_ast_constant(ast_node_t *node){
	if (node && 
		(node_is_constant(node) 
		|| (node->types.variable.is_parameter == TRUE)
		|| (node->type == UNARY_OPERATION && node_is_ast_constant(node->children[0]))
		|| (node->type == BINARY_OPERATION && node_is_ast_constant(node->children[0]) && node_is_ast_constant(node->children[1]))))
	{
		return node;
	}
	return NULL;
}

/*---------------------------------------------------------------------------------------------
 * (function: node_is_ast_constant)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *node_is_ast_constant(ast_node_t *node, STRING_CACHE *defines_for_module_sc){
	if (node && (node_is_constant(node) 
		|| (node->types.variable.is_parameter == TRUE)
		|| (node->type == UNARY_OPERATION && node_is_ast_constant(node->children[0], defines_for_module_sc))
		|| (node->type == BINARY_OPERATION && node_is_ast_constant(node->children[0], defines_for_module_sc) && node_is_ast_constant(node->children[1], defines_for_module_sc))))
	{
		return node;
	}
	else if (node && node->type == IDENTIFIERS) {
		int sc_spot;
		if ((sc_spot = sc_lookup_string(defines_for_module_sc, node->types.identifier)) != -1
			&& node_is_ast_constant((ast_node_t *)defines_for_module_sc->data[sc_spot]))
		{
			return node;
		}
	}
	return NULL;
}

/*---------------------------------------------------------------------------
 * (function: initial_node)
 *-------------------------------------------------------------------------*/
void initial_node(ast_node_t *new_node, ids id, int line_number, int file_number, int unique_counter)
{
	new_node->type = id;
	new_node->children = NULL;
	new_node->num_children = 0;
	new_node->unique_count = unique_counter; //++count_id;
	new_node->line_number = line_number;
	new_node->file_number = file_number;
	new_node->far_tag = 0;
	new_node->high_number = 0;
	new_node->shared_node = FALSE;
	new_node->hb_port = 0;
	new_node->net_node = 0;
	new_node->is_read_write = 0;
	new_node->types.vnumber = nullptr;
	new_node->types.identifier = NULL;
}

/*---------------------------------------------------------------------------
 * (function: clog2)
 *-------------------------------------------------------------------------*/
long clog2(long value_in, int length) 
{
	if (value_in == 0) return 0;

	long result;

	/* negative numbers may be larger than they need to be */
	if (value_in < 0 && value_in >= std::numeric_limits<int32_t>::min()) return 32; 

	if (length > 32) 
	{
		uint64_t unsigned_val = (uint64_t) value_in;
		result = (long) ceil(log2((double) unsigned_val));
	}
	else
	{
		uint32_t unsigned_val = (uint32_t) value_in;
		result = (long) ceil(log2((double) unsigned_val));
	}

	return result;
}