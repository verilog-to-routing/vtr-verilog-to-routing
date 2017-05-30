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
#include "ast_util.h"
#include "errors.h"
#include "globals.h"
#include "odin_util.h"
#include "types.h"
#include "vtr_util.h"

char **get_name_of_pins_number(ast_node_t *var_node, int start, int width);

/*---------------------------------------------------------------------------
 * (function: create_node_w_type)
 *-------------------------------------------------------------------------*/
ast_node_t* create_node_w_type(ids id, int line_number, int file_number)
{
	static long unique_count = 0;

	ast_node_t* new_node;

	new_node = (ast_node_t*)calloc(1, sizeof(ast_node_t));
	oassert(new_node != NULL);
	new_node->type = id;
	
	new_node->children = NULL;
	new_node->num_children = 0;

	new_node->line_number = line_number;
	new_node->file_number = file_number;
	new_node->unique_count = unique_count++;

	new_node->far_tag = 0;
	new_node->high_number = 0;

	return new_node;
}

/*---------------------------------------------------------------------------
 * (function: free_ast_tree_branch)
 *-------------------------------------------------------------------------*/
void free_ast_tree_branch(ast_node_t *node)
{
	int i;

	if (node && !node->shared_node){
		for (i = 0; i < node->num_children; i++)
			free_ast_tree_branch(node->children[i]);
	
		if (node->children != NULL)
			free_me(node->children);
	
		switch(node->type)
			{
				case IDENTIFIERS:
					if (node->types.identifier != NULL)
						free_me(node->types.identifier);
					break;
				case NUMBERS:
					if (node->types.number.number != NULL)
						free_me(node->types.number.number);
					if (node->types.number.binary_string != NULL)
						free_me(node->types.number.binary_string);
					break;
				default:
					break;
			}
	
		free_me(node);
	}

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
 * (function: *create_tree_node_long_long_number)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *create_tree_node_long_long_number(long long number, int line_number, int /*file_number*/)
{
	ast_node_t* new_node = create_node_w_type(NUMBERS, line_number, current_parse_file);
	new_node->types.number.base = LONG_LONG;
	new_node->types.number.value = number;
	new_node->types.number.binary_size = sizeof(number)*8;
	new_node->types.number.binary_string = convert_long_long_to_bit_string(number,sizeof(number)*8);

	return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: create_tree_node_number)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *create_tree_node_number(char* number, int line_number, int /*file_number*/)
{
	ast_node_t* new_node = create_node_w_type(NUMBERS, line_number, current_parse_file);
	char *string_pointer = number;
	int index_string_pointer = 0;
	char *temp_string;
	short flag_constant_decimal = FALSE;

	/* find the ' character if it's a base */
	for (string_pointer=number; *string_pointer; string_pointer++) 
	{
		if (*string_pointer == '\'') 
		{
			break;
		}
		index_string_pointer++;
	}

	if (index_string_pointer == (int)strlen(number))
	{
		flag_constant_decimal = TRUE;
		/* this is base d */
		new_node->types.number.base = DEC;
		/* reset to the front */
		string_pointer = number;

		/* size is the rest of the string */
		new_node->types.number.size = strlen((string_pointer));
		/* assign the remainder of the number to a string */
		new_node->types.number.number = vtr::strdup((string_pointer));
	}	
	else
	{
		/* there is a base in the form: number[bhod]'number */
		switch(tolower(*(string_pointer+1)))
		{
			case 'd':
				new_node->types.number.base = DEC;
				break;
			case 'h':
				new_node->types.number.base = HEX;
				break;
			case 'o':
				new_node->types.number.base = OCT;
				break;
			case 'b':
				new_node->types.number.base = BIN;
				break;
			default:
				oassert(FALSE);
				break;
		}

		/* check if the size matches the design specified size */
		if(index_string_pointer > 0){
			temp_string = vtr::strdup(number); 
			temp_string[index_string_pointer] = '\0';
			/* size is the rest of the string */
			new_node->types.number.is_full = 0;
			new_node->types.number.size = atoi(temp_string); 
		}
		else{
			new_node->types.number.is_full = 1;
			new_node->types.number.size = 1;
		}          
	
		/* move to the digits */
		string_pointer += 2;

		/* assign the remainder of the number to a string */
		new_node->types.number.number = vtr::strdup((string_pointer));
	}

	/* check for decimal numbers without the formal 2'd... format */
	if (flag_constant_decimal == FALSE)
	{
		/* size describes how may bits */
		new_node->types.number.binary_size = new_node->types.number.size;
	}
	else
	{
		/* size is for a constant that needs */
		if (strcmp(new_node->types.number.number, "0") != 0)
		{
			new_node->types.number.binary_size = ceil((log(convert_dec_string_of_size_to_long_long(new_node->types.number.number, new_node->types.number.size)+1))/log(2));
		}
		else
		{
			new_node->types.number.binary_size = 1;
		}
	}
	/* add in the values for all the numbers */
	switch (new_node->types.number.base)
	{
		case(DEC):
			// This will have limited width.
			new_node->types.number.value = convert_dec_string_of_size_to_long_long(new_node->types.number.number, new_node->types.number.size);
			new_node->types.number.binary_string = convert_long_long_to_bit_string(new_node->types.number.value, new_node->types.number.binary_size);
			break;
		case(HEX):
			if(!is_dont_care_string(new_node->types.number.number)){
				new_node->types.number.binary_size *= 4;
				new_node->types.number.value = strtoll(new_node->types.number.number,NULL,16); // This will have limited width.
				// This will have full width.
				new_node->types.number.binary_string = convert_hex_string_of_size_to_bit_string(0, new_node->types.number.number, new_node->types.number.binary_size);
			}
			else{
				new_node->types.number.binary_string = convert_hex_string_of_size_to_bit_string(1, new_node->types.number.number, new_node->types.number.binary_size);
			}
			break;
		case(OCT):
			new_node->types.number.binary_size *= 3;
			new_node->types.number.value = strtoll(new_node->types.number.number,NULL,8); // This will have limited width.
			// This will have full width.
			new_node->types.number.binary_string = convert_oct_string_of_size_to_bit_string(new_node->types.number.number, new_node->types.number.binary_size);
			break;
		case(BIN):
		{
			if(new_node->types.number.is_full == 0){
				// This will have limited width.
				new_node->types.number.value = strtoll(new_node->types.number.number,NULL,2);
				// This will have full width.
		
			}
			new_node->types.number.binary_string = convert_binary_string_of_size_to_bit_string(1, new_node->types.number.number, new_node->types.number.binary_size);
		}
		break;
        default:
            oassert(FALSE);
            break;
    }

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
	node->children = (ast_node_t**)calloc(num_children,sizeof(ast_node_t*));
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
	node->children = (ast_node_t**)realloc(node->children, sizeof(ast_node_t*)*(node->num_children+1));
	node->num_children ++;
	node->children[node->num_children-1] = child;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_child_to_node)
 *-------------------------------------------------------------------------------------------*/
void add_child_at_the_beginning_of_the_node(ast_node_t* node, ast_node_t *child) 
{
    int i;
    ast_node_t *ref, *ref1;
	/* Handle case where we have an empty statement. */
	if (child == NULL)
		return;



	/* allocate space for the children */
	node->children = (ast_node_t**)realloc(node->children, sizeof(ast_node_t*)*(node->num_children+1));
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
 * (function: get_range)
 *  Check the node range is legal. Will return the range if it's legal.
 *  Node should have three children. Second and Third children's type should be NUMBERS.
 *-------------------------------------------------------------------------------------------*/
int get_range(ast_node_t* first_node) 
{
	long temp_value;
	/* look at the first item to see if it has a range */
	if (first_node->children[1] != NULL && first_node->children[1]->type == NUMBERS && first_node->children[2] != NULL && first_node->children[2]->type == NUMBERS)
	{
		/* IF the first element in the list has a second element...that is the range */
		//oassert(first_node->children[2] != NULL); // the third element should be a value
		//oassert((first_node->children[1]->type == NUMBERS) && (first_node->children[2]->type == NUMBERS)); // should be numbers
		if(first_node->children[1]->types.number.value < first_node->children[2]->types.number.value)
		{
			// Reversing the indicies doesn't produce correct code. We need to actually handle these correctly.
			error_message(NETLIST_ERROR, first_node->line_number, first_node->file_number, "Odin doesn't support arrays declared [m:n] where m is less than n.");

			// swap them around
			temp_value = first_node->children[1]->types.number.value;
			first_node->children[1]->types.number.value = first_node->children[2]->types.number.value;
			first_node->children[2]->types.number.value = temp_value;
		}

		return abs(first_node->children[1]->types.number.value - first_node->children[2]->types.number.value) + 1; // 1:0 is 2 spots
	}
	return -1; // indicates no range
}

/*---------------------------------------------------------------------------------------------
 * (function: make_concat_into_list_of_strings)
 * 	0th idx will be the MSbit
 *-------------------------------------------------------------------------------------------*/
void make_concat_into_list_of_strings(ast_node_t *concat_top, char *instance_name_prefix)
{
	int i, j; 
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
			if ((sc_spot = sc_lookup_string(local_symbol_table_sc, temp_string)) != -1)
			{				
				free_me(temp_string);

			if (((ast_node_t*)local_symbol_table_sc->data[sc_spot])->children[1] == NULL)
			{
				concat_top->types.concat.num_bit_strings ++;
				concat_top->types.concat.bit_strings = (char**)realloc(concat_top->types.concat.bit_strings, sizeof(char*)*(concat_top->types.concat.num_bit_strings));
				concat_top->types.concat.bit_strings[concat_top->types.concat.num_bit_strings-1] = get_name_of_pin_at_bit(concat_top->children[i], -1, instance_name_prefix);
			}
			else if (((ast_node_t*)local_symbol_table_sc->data[sc_spot])->children[3] == NULL)
			{
				/* reverse thorugh the range since highest bit in index will be lower in the string indx */
				rnode[1] = resolve_node(FALSE, instance_name_prefix, ((ast_node_t*)local_symbol_table_sc->data[sc_spot])->children[1]);
				rnode[2] = resolve_node(FALSE, instance_name_prefix, ((ast_node_t*)local_symbol_table_sc->data[sc_spot])->children[2]);
				oassert(rnode[1]->type == NUMBERS && rnode[2]->type == NUMBERS);

				for (j = rnode[1]->types.number.value - rnode[2]->types.number.value; j >= 0; j--)
				{
					concat_top->types.concat.num_bit_strings ++;
					concat_top->types.concat.bit_strings = (char**)realloc(concat_top->types.concat.bit_strings, sizeof(char*)*(concat_top->types.concat.num_bit_strings));
					concat_top->types.concat.bit_strings[concat_top->types.concat.num_bit_strings-1] = get_name_of_pin_at_bit(concat_top->children[i], j, instance_name_prefix);
				}
			}
			else if (((ast_node_t*)local_symbol_table_sc->data[sc_spot])->children[3] != NULL)
			{
				oassert(FALSE);	
			}
			}
			else {
				error_message(NETLIST_ERROR, concat_top->line_number, concat_top->file_number, "Missssing declaration of this symbol %s\n", temp_string);
			}
		}
		else if (concat_top->children[i]->type == ARRAY_REF)
		{
			concat_top->types.concat.num_bit_strings ++;
			concat_top->types.concat.bit_strings = (char**)realloc(concat_top->types.concat.bit_strings, sizeof(char*)*(concat_top->types.concat.num_bit_strings));
			concat_top->types.concat.bit_strings[concat_top->types.concat.num_bit_strings-1] = get_name_of_pin_at_bit(concat_top->children[i], 0, instance_name_prefix);
		}
		else if (concat_top->children[i]->type == RANGE_REF)
		{
			rnode[1] = resolve_node(FALSE, instance_name_prefix, concat_top->children[i]->children[1]);
			rnode[2] = resolve_node(FALSE, instance_name_prefix, concat_top->children[i]->children[2]);
			oassert(rnode[1]->type == NUMBERS && rnode[2]->type == NUMBERS);
			oassert(rnode[1]->types.number.value >= rnode[2]->types.number.value);
			int width = abs(rnode[1]->types.number.value - rnode[2]->types.number.value) + 1;

			//for (j = rnode[1]->types.number.value - rnode[2]->types.number.value; j >= 0; j--)
			// Changed to forward to fix concatenation bug.
			for (j = 0; j < width; j++)
			{
				concat_top->types.concat.num_bit_strings ++;
				concat_top->types.concat.bit_strings = (char**)realloc(concat_top->types.concat.bit_strings, sizeof(char*)*(concat_top->types.concat.num_bit_strings));
				concat_top->types.concat.bit_strings[concat_top->types.concat.num_bit_strings-1] = 
					get_name_of_pin_at_bit(concat_top->children[i], ((rnode[1]->types.number.value - rnode[2]->types.number.value))-j, instance_name_prefix);
			}
		}
		else if (concat_top->children[i]->type == NUMBERS)
		{
			if(concat_top->children[i]->types.number.base == DEC)
			{
				error_message(NETLIST_ERROR, concat_top->line_number, concat_top->file_number, "Concatenation can't include decimal numbers due to conflict on bits\n");
			}

			// Changed to reverse to fix concatenation bug.
			for (j = concat_top->children[i]->types.number.binary_size-1; j>= 0; j--)
			{
				concat_top->types.concat.num_bit_strings ++;
				concat_top->types.concat.bit_strings = (char**)realloc(concat_top->types.concat.bit_strings, sizeof(char*)*(concat_top->types.concat.num_bit_strings));
				concat_top->types.concat.bit_strings[concat_top->types.concat.num_bit_strings-1] = get_name_of_pin_at_bit(concat_top->children[i], j, instance_name_prefix);
			}
		}
		else if (concat_top->children[i]->type == CONCATENATE)
		{
			/* forward through list since we build concatenate list in idx order of MSB at index 0 and LSB at index list_size */
			for (j = 0; j < concat_top->children[i]->types.concat.num_bit_strings; j++)
			{
				concat_top->types.concat.num_bit_strings ++;
				concat_top->types.concat.bit_strings = (char**)realloc(concat_top->types.concat.bit_strings, sizeof(char*)*(concat_top->types.concat.num_bit_strings));
				concat_top->types.concat.bit_strings[concat_top->types.concat.num_bit_strings-1] = get_name_of_pin_at_bit(concat_top->children[i], j, instance_name_prefix);
			}
		}
		else {
			error_message(NETLIST_ERROR, concat_top->line_number, concat_top->file_number, "Unsupported operation within a concatenation.\n");
		}
	}
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
		return_string = make_full_ref_name(NULL, NULL, NULL, var_declare->children[0]->types.identifier, var_declare->children[2]->types.number.value+bit);
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
	char *return_string; 
	ast_node_t *rnode[3];

	if (var_node->type == ARRAY_REF)
	{
		oassert(var_node->children[0]->type == IDENTIFIERS);
		oassert(var_node->children[1]->type == NUMBERS);
		return_string = make_full_ref_name(NULL, NULL, NULL, var_node->children[0]->types.identifier, (int)var_node->children[1]->types.number.value);
	}
	else if (var_node->type == RANGE_REF)
	{
		rnode[1] = resolve_node(FALSE, instance_name_prefix, var_node->children[1]);
		rnode[2] = resolve_node(FALSE, instance_name_prefix, var_node->children[2]);
		oassert(var_node->children[0]->type == IDENTIFIERS);
		oassert(rnode[1]->type == NUMBERS && rnode[2]->type == NUMBERS);
		oassert((rnode[1]->types.number.value >= rnode[2]->types.number.value+bit) && bit >= 0);

		return_string = make_full_ref_name(NULL, NULL, NULL, var_node->children[0]->types.identifier, rnode[2]->types.number.value+bit);
	}
	else if ((var_node->type == IDENTIFIERS) && (bit == -1))
	{
		return_string = make_full_ref_name(NULL, NULL, NULL, var_node->types.identifier, -1);
	}
	else if (var_node->type == IDENTIFIERS)
	{
		long sc_spot;
		int pin_index;

		if ((sc_spot = sc_lookup_string(local_symbol_table_sc, var_node->types.identifier)) > -1)
		{
		}
		else {
			pin_index = 0;
			error_message(NETLIST_ERROR, var_node->line_number, var_node->file_number, "Missing declaration of this symbol %s\n", var_node->types.identifier);
		}

		if (((ast_node_t*)local_symbol_table_sc->data[sc_spot])->children[1] == NULL)
		{
			pin_index = bit;
		}
		else if (((ast_node_t*)local_symbol_table_sc->data[sc_spot])->children[3] == NULL)
		{
			oassert(((ast_node_t*)local_symbol_table_sc->data[sc_spot])->children[2]->type == NUMBERS);
			pin_index = ((ast_node_t*)local_symbol_table_sc->data[sc_spot])->children[2]->types.number.value + bit; 
		}
		else
			oassert(FALSE);

		return_string = make_full_ref_name(NULL, NULL, NULL, var_node->types.identifier, pin_index);
	}
	else if (var_node->type == NUMBERS)
	{
		if (bit == -1)
			bit = 0;

		oassert(bit < var_node->types.number.binary_size);
		if (var_node->types.number.binary_string[var_node->types.number.binary_size-bit-1] == '1')
		{
			return_string = (char*)calloc(11+1,sizeof(char)); // ONE_VCC_CNS	
			sprintf(return_string, "ONE_VCC_CNS");
		}
		else if (var_node->types.number.binary_string[var_node->types.number.binary_size-bit-1] == '0')
		{
			return_string = (char*)calloc(13+1,sizeof(char)); // ZERO_GND_ZERO	
			sprintf(return_string, "ZERO_GND_ZERO");
		}
		else
		{
			return_string = NULL;
			oassert(FALSE);
		}
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

			return_string = (char*)calloc(strlen(var_node->types.concat.bit_strings[bit])+1,sizeof(char));
			sprintf(return_string, "%s", var_node->types.concat.bit_strings[bit]);
		}
	}
	else
	{
		return_string = NULL;

		error_message(NETLIST_ERROR, var_node->line_number, var_node->file_number, "Unsupported variable type. var_node->type = %d\n",var_node->type);
		oassert(FALSE);
	}
	
	return return_string;
}

char **get_name_of_pins_number(ast_node_t *var_node, int /*start*/, int width)
{
	char **return_string; 
	oassert(var_node->type == NUMBERS);

	return_string = (char**)calloc(width,sizeof(char*));
	int i, j;
	for (i = 0, j = var_node->types.number.binary_size-1; i < width; i++, j--)
	{
		/* strings are msb is 0th index in string, reverse access */
		if (j >= 0)
		{
			char c = var_node->types.number.binary_string[j];
			switch(c)
			{
			case '1': return_string[i] = vtr::strdup("ONE_VCC_CNS"); break;
			case '0': return_string[i] = vtr::strdup("ZERO_GND_ZERO"); break;
			case 'x': return_string[i] = vtr::strdup("ZERO_GND_ZERO"); break; 
			default: error_message(NETLIST_ERROR, var_node->line_number, var_node->file_number, "Unrecognised character %c in binary string \"%s\"!\n", c, var_node->types.number.binary_string);
			}
		}
		else
			// if the index is too big for the number pad with zero
			return_string[i] = vtr::strdup("ZERO_GND_ZERO");
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
	char_list_t *return_list = (char_list_t*)calloc(1,sizeof(char_list_t));
	ast_node_t *rnode[3];

	int i;
	int width = 0;

	if (var_node->type == ARRAY_REF)
	{
		width = 1;
		return_string = (char**)calloc(1,sizeof(char*));
		rnode[1] = resolve_node(FALSE, instance_name_prefix, var_node->children[1]);
		oassert(rnode[1]->type == NUMBERS);
		oassert(var_node->children[0]->type == IDENTIFIERS);
		return_string[0] = make_full_ref_name(NULL, NULL, NULL, var_node->children[0]->types.identifier, rnode[1]->types.number.value);
	}
	else if (var_node->type == RANGE_REF)
	{
		rnode[0] = resolve_node(FALSE, instance_name_prefix, var_node->children[0]);
		rnode[1] = resolve_node(FALSE, instance_name_prefix, var_node->children[1]);
		rnode[2] = resolve_node(FALSE, instance_name_prefix, var_node->children[2]);
		oassert(rnode[1]->type == NUMBERS && rnode[2]->type == NUMBERS);
		width = abs(rnode[1]->types.number.value - rnode[2]->types.number.value) + 1;
		if (rnode[0]->type == IDENTIFIERS)
		{
			return_string = (char**)calloc(width,sizeof(char*));
			for (i = 0; i < width; i++)
				return_string[i] = make_full_ref_name(NULL, NULL, NULL, rnode[0]->types.identifier, rnode[2]->types.number.value+i);
		}
		else
		{
			oassert(rnode[0]->type == NUMBERS);
			return_string = get_name_of_pins_number(rnode[0], rnode[2]->types.number.value, width);
		}
	}
	else if (var_node->type == IDENTIFIERS)
	{
		/* need to look in the symbol table for details about this identifier (i.e. is it a port) */
		long sc_spot;
		ast_node_t *sym_node;
		
		// try and resolve var_node
		sym_node = resolve_node(FALSE, instance_name_prefix, var_node);

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
            else {
                error_message(NETLIST_ERROR, var_node->line_number, var_node->file_number, "Missing declaration of this symbol %s\n", temp_string);
            }
			free_me(temp_string);

			if (sym_node->children[1] == NULL || sym_node->type == BLOCKING_STATEMENT)
			{
				width = 1;
				return_string = (char**)calloc(width,sizeof(char*));
				return_string[0] = make_full_ref_name(NULL, NULL, NULL, var_node->types.identifier, -1);
			}
			else if (sym_node->children[3] == NULL)
			{
				int index = 0;
				rnode[1] = resolve_node(FALSE, instance_name_prefix, sym_node->children[1]);
				rnode[2] = resolve_node(FALSE, instance_name_prefix, sym_node->children[2]);
				oassert(rnode[1]->type == NUMBERS && rnode[2]->type == NUMBERS);
				width = (rnode[1]->types.number.value - rnode[2]->types.number.value + 1);
				return_string = (char**)calloc(width,sizeof(char*));
				for (i = 0; i < width; i++)
				{
					return_string[index] = make_full_ref_name(NULL, NULL, NULL, var_node->types.identifier, 
						i+rnode[2]->types.number.value);
					index++;
				}
			}
			else if (sym_node->children[3] != NULL)
			{
				oassert(FALSE);	
			}
			else
			{


			}
		}
		else
		{
			oassert(sym_node->type == NUMBERS);
			width = sym_node->types.number.binary_size;
			return_string = get_name_of_pins_number(sym_node, 0, width);
		}
	}
	else if (var_node->type == NUMBERS)
	{
		width = var_node->types.number.binary_size;
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
			return_string = (char**)calloc(width,sizeof(char*));
			for (i = 0; i < width; i++) // 0th bit is MSB so need to access reverse
			{
				return_string[i] = (char*)calloc(strlen(var_node->types.concat.bit_strings[var_node->types.concat.num_bit_strings-i-1])+1,sizeof(char));
				sprintf(return_string[i], "%s", var_node->types.concat.bit_strings[var_node->types.concat.num_bit_strings-i-1]);
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

	/* get the list */
	return_list = get_name_of_pins(var_node, instance_name_prefix);

	for (i = 0; i < return_list->num_strings; i++)
	{
		return_list->strings[i] = make_full_ref_name(instance_name_prefix, NULL, NULL, return_list->strings[i], -1);
	}

	return return_list;
}

/*----------------------------------------------------------------------------
 * (function: resolve_node)
 *--------------------------------------------------------------------------*/
/**
 * Recursively resolves an IDENTIFIER to a parameter into its actual value, 
 * by looking it up in the global_param_table_sc
 * Also try and fold any BINARY_OPERATIONs now that an IDENTIFIER has been
 * resolved
 */
ast_node_t *resolve_node(short initial, char *module_name, ast_node_t *node)
{
	if (node)
	{
		long sc_spot;
		int i;
		STRING_CACHE *local_param_table_sc;

		ast_node_t *node_copy;
		node_copy = (ast_node_t *)calloc(1,sizeof(ast_node_t));
		memcpy(node_copy, node, sizeof(ast_node_t));
		node_copy->children = (ast_node_t **)calloc(node_copy->num_children,sizeof(ast_node_t*));

		for (i = 0; i < node->num_children; i++){
			node_copy->children[i] = resolve_node(initial, module_name, node->children[i]);
		}
		
		long long result = WRONG_CALCULATION;
		switch (node->type){
			
			case IDENTIFIERS:
				if(!initial){
					oassert(module_name);
					sc_spot = sc_lookup_string(global_param_table_sc, module_name);
					oassert(sc_spot != -1);
					local_param_table_sc = (STRING_CACHE *)global_param_table_sc->data[sc_spot];
					sc_spot = sc_lookup_string(local_param_table_sc, node->types.identifier);
					if (sc_spot != -1)
					{
						node = ((ast_node_t *)local_param_table_sc->data[sc_spot]);
						oassert(node->type == NUMBERS);
					}
				}
			break;
			
			case UNARY_OPERATION:
				if(initial){
					result = calculate_unary(node_is_constant(node_copy->children[0]), node_copy->types.operation.op);
				}
				break;

			case BINARY_OPERATION:
				result = calculate_binary(node_is_constant(node_copy->children[0]), node_is_constant(node_copy->children[1]), node_copy->types.operation.op);
				break;
			

			default:
				break;
		}
		if(result != WRONG_CALCULATION){
			node_copy->shared_node = TRUE;
			node = create_tree_node_long_long_number(result, node_copy->line_number, node_copy->file_number);
			node_copy->shared_node = FALSE;
		}

		free_me(node_copy->children);
		free_me(node_copy);
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
char *make_module_param_name(ast_node_t *module_param_list, char *module_name)
{
	char *module_param_name = (char*)calloc(strlen(module_name)+1024,sizeof(char));
	strcpy(module_param_name, module_name);
	
	if (module_param_list)
	{
		int i;
		oassert(module_param_list->num_children > 0);
		strcat(module_param_name, "___");
		for (i = 0; i < module_param_list->num_children; i++)
		{
			oassert(module_param_list->children[i]->children[5]->type == NUMBERS);

			sprintf(module_param_name, "%s_%lld", module_param_name, module_param_list->children[i]->children[5]->types.number.value);
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
	ast_node_t *node_copy;
	int i;

	if(node == NULL){
		return NULL;
	}

	//Copy node
	node_copy = (ast_node_t *)calloc(1,sizeof(ast_node_t));
	memcpy(node_copy, node, sizeof(ast_node_t));

	//Copy contents 
	if(node_copy->types.identifier != NULL){
		node_copy->types.identifier = vtr::strdup(node_copy->types.identifier);
	}

	//Recursively copy its children
	for(i = 0; i < node->num_children; i++){
		node_copy->children[i] = ast_node_deep_copy(node_copy->children[i]);
	}

	return node_copy;
}


/*---------------------------------------------------------------------------------------------
 * (function: calculate_unary)
 *-------------------------------------------------------------------------------------------*/
long long calculate_unary(long long operand_0, short type_of_ops){
	if(operand_0 !=  WRONG_CALCULATION){
		switch (type_of_ops){
			case LOGICAL_NOT:
				return !operand_0;
			case BITWISE_NOT:
				return ~operand_0;
			case MINUS:
				return -operand_0;
			default:
				break;
		}
	}
	return WRONG_CALCULATION;
}

/*---------------------------------------------------------------------------------------------
 * (function: calculate_binary)
 *-------------------------------------------------------------------------------------------*/
long long calculate_binary(long long operand_0, long long operand_1, short type_of_ops){
	if((operand_0 != WRONG_CALCULATION) && (operand_1 != WRONG_CALCULATION)){
		switch (type_of_ops){
			case ADD:
				return operand_0 + operand_1;
			case MINUS:
				return operand_0 - operand_1;
			case MULTIPLY:
				return operand_0 * operand_1;
			case DIVIDE:
				return operand_0 / operand_1;
			case BITWISE_XOR:
				return operand_0 ^ operand_1;
			case BITWISE_XNOR:
				return ~(operand_0 ^ operand_1);
			case BITWISE_AND:
				return operand_0 & operand_1;
			case BITWISE_NAND:
				return ~(operand_0 & operand_1);
			case BITWISE_OR:
				return operand_0 | operand_1;
			case BITWISE_NOR:
				return ~(operand_0 | operand_1);
			case SL:
				return operand_0 << operand_1;
			case SR:
				return operand_0 >> operand_1;
			case LOGICAL_AND:
				return operand_0 && operand_1;
			case LOGICAL_OR:
				return operand_0 || operand_1;
			default:
				break;
		}
	}
	return WRONG_CALCULATION;
}

/*---------------------------------------------------------------------------------------------
 * (function: node_is_constant)
 *-------------------------------------------------------------------------------------------*/
long long node_is_constant(ast_node_t *node){
	if (node && node->type == NUMBERS){
		/* check if it's a constant depending on the number type */
		switch (node->types.number.base){
			case DEC: case HEX: case OCT: case BIN:
				if (node->types.number.value == -1){
					break;
				}
			case(LONG_LONG):
				return (long long) node->types.number.value;
				break;
            default:
				break;
		}
	}
	return WRONG_CALCULATION;
}