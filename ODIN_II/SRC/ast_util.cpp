/*
 * Copyright (c) 2009 Peter Andrew Jamieson (jamieson.peter@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
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

char** get_name_of_pins_number(ast_node_t* var_node, int start, int width);
char* get_name_of_pin_number(ast_node_t* var_node, int bit);
void update_tree_tag(ast_node_t* node, int cases, int tagged);
STRING_CACHE* copy_param_table_sc(STRING_CACHE* to_copy);

// HIGH LEVEL AST TAG
static int high_level_id;

/*---------------------------------------------------------------------------
 * (function: update_tree)
 *-------------------------------------------------------------------------*/
void update_tree_tag(ast_node_t* node, int high_level_block_type_to_search, int tag) {
    long i;
    int tagged = tag;
    if (node) {
        if (node->type == high_level_block_type_to_search)
            tagged = ++high_level_id;

        if (tagged > -1) {
            node->far_tag = tagged;
            node->high_number = node->line_number;
        }

        for (i = 0; i < node->num_children; i++)
            update_tree_tag(node->children[i], high_level_block_type_to_search, tagged);
    }
}

/*---------------------------------------------------------------------------
 * (function: add_tag_data)
 *-------------------------------------------------------------------------*/
void add_tag_data(ast_t* ast) {
    long i;
    high_level_id = -1;

    ids type = NO_ID;
    if (global_args.high_level_block.value() == "if") {
        type = IF;
    } else if (global_args.high_level_block.value() == "always") {
        type = ALWAYS;
    } else if (global_args.high_level_block.value() == "module") {
        type = MODULE;
    }

    if (type != NO_ID) {
        for (i = 0; i < ast->top_modules_count; i++)
            update_tree_tag(ast->top_modules[i], type, -1);
    }
}

// END HIGH LEVEL TAG

/*---------------------------------------------------------------------------
 * (function: allocate_ast)
 *-------------------------------------------------------------------------*/
ast_t* allocate_ast() {
    ast_t* new_ast;

    new_ast = (ast_t*)vtr::calloc(1, sizeof(ast_t));
    oassert(new_ast != NULL);

    new_ast->top_modules = NULL;
    new_ast->top_modules_count = 0;

    return new_ast;
}

/*---------------------------------------------------------------------------
 * (function: free_ast)
 *-------------------------------------------------------------------------*/
ast_t* free_ast(ast_t* to_delete) {
    vtr::free(to_delete);
    to_delete = NULL;
    return to_delete;
}

/*---------------------------------------------------------------------------
 * (function: add_top_module_to_ast)
 *-------------------------------------------------------------------------*/
void add_top_module_to_ast(ast_t* ast, ast_node_t* to_add) {
    oassert(ast);
    if (to_add) {
        ast->top_modules_count += 1;
        ast->top_modules
            = (ast_node_t**)vtr::realloc(ast->top_modules, sizeof(ast_node_t*) * ast->top_modules_count);

        ast->top_modules[ast->top_modules_count - 1] = to_add;
    }
}

/*---------------------------------------------------------------------------
 * (function: create_node_w_type)
 *-------------------------------------------------------------------------*/
static ast_node_t* create_node_w_type(ids id, int line_number, int file_number, bool update_unique_count) {
    oassert(id != NO_ID);

    static long unique_count = 0;

    ast_node_t* new_node;

    new_node = (ast_node_t*)vtr::calloc(1, sizeof(ast_node_t));
    oassert(new_node != NULL);

    initial_node(new_node, id, line_number, file_number, unique_count);

    if (update_unique_count)
        unique_count += 1;

    return new_node;
}

ast_node_t* create_node_w_type_no_count(ids id, int line_number, int file_number) {
    return create_node_w_type(id, line_number, file_number, false);
}

/*---------------------------------------------------------------------------
 * (function: create_node_w_type)
 *-------------------------------------------------------------------------*/
ast_node_t* create_node_w_type(ids id, int line_number, int file_number) {
    return create_node_w_type(id, line_number, file_number, true);
}

/*---------------------------------------------------------------------------
 * (function: free_assignement_of_node_keep_tree)
 * free a node type but stay in tree structure
 *-------------------------------------------------------------------------*/
void free_assignement_of_node_keep_tree(ast_node_t* node) {
    if (node) {
        int i;
        vtr::free(node->types.identifier);
        switch (node->type) {
            case NUMBERS:
                if (node->types.vnumber != nullptr)
                    delete node->types.vnumber;
                node->types.vnumber = nullptr;
                break;

            case CONCATENATE:
                for (i = 0; i < node->types.concat.num_bit_strings; i++) {
                    if (node->types.concat.bit_strings[i])
                        vtr::free(node->types.concat.bit_strings[i]);
                }
                vtr::free(node->types.concat.bit_strings);

            default:
                break;
        }
    }
}
/*---------------------------------------------------------------------------
 * (function: free_single_node)
 * free a node whose type is IDENTIFERS
 *-------------------------------------------------------------------------*/
ast_node_t* free_single_node(ast_node_t* node) {
    if (node) {
        free_assignement_of_node_keep_tree(node);
        vtr::free(node->children);
        vtr::free(node);
    }
    return NULL;
}

/*---------------------------------------------------------------------------
 * (function: free_all_children)
 *-------------------------------------------------------------------------*/
void free_all_children(ast_node_t* node) {
    long i;

    if (node) {
        for (i = 0; i < node->num_children; i++)
            node->children[i] = free_whole_tree(node->children[i]);
        vtr::free(node->children);
        node->children = NULL;
        node->num_children = 0;
    }
}

/*---------------------------------------------------------------------------
 * (function: free_all_children)
 *-------------------------------------------------------------------------*/
void free_resolved_children(ast_node_t* node) {
    long i;

    if (node) {
        for (i = 0; i < node->num_children; i++) {
            node->children[i] = free_resolved_tree(node->children[i]);
            node->children[i] = NULL;
        }

        vtr::free(node->children);
        node->children = NULL;
        node->num_children = 0;
    }
}

/*---------------------------------------------------------------------------
 * (function: free_whole_tree)
 *-------------------------------------------------------------------------*/
ast_node_t* free_whole_tree(ast_node_t* node) {
    free_all_children(node);
    return free_single_node(node);
}

/*---------------------------------------------------------------------------
 * (function: free_whole_tree)
 *-------------------------------------------------------------------------*/
ast_node_t* free_resolved_tree(ast_node_t* node) {
    free_resolved_children(node);
    if (node->type != IDENTIFIERS) {
        node = free_single_node(node);
    }

    return NULL;
}

/*---------------------------------------------------------------------------------------------
 * (function: create_tree_node_id)
 *-------------------------------------------------------------------------------------------*/
ast_node_t* create_tree_node_id(char* string, int line_number, int /*file_number*/) {
    ast_node_t* new_node = create_node_w_type(IDENTIFIERS, line_number, current_parse_file);
    new_node->types.identifier = string;

    return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: create_tree_node_number)
 *-------------------------------------------------------------------------------------------*/
ast_node_t* create_tree_node_number(char* input_number, int line_number, int /* file_number */) {
    ast_node_t* new_node = create_node_w_type(NUMBERS, line_number, current_parse_file);
    new_node->types.vnumber = new VNumber(input_number);

    return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: create_tree_node_number)
 *-------------------------------------------------------------------------------------------*/
ast_node_t* create_tree_node_number(VNumber& input_number, int line_number, int /* file_number */) {
    ast_node_t* new_node = create_node_w_type(NUMBERS, line_number, current_parse_file);
    new_node->types.vnumber = new VNumber(input_number);

    return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: create_tree_node_number)
 *-------------------------------------------------------------------------------------------*/
ast_node_t* create_tree_node_number(long input_number, int line_number, int /* file_number */) {
    ast_node_t* new_node = create_node_w_type(NUMBERS, line_number, current_parse_file);
    new_node->types.vnumber = new VNumber(input_number);

    return new_node;
}

/*---------------------------------------------------------------------------
 * (function: allocate_children_to_node)
 *-------------------------------------------------------------------------*/
void allocate_children_to_node(ast_node_t* node, std::vector<ast_node_t*> children_list) {
    /* allocate space for the children */
    node->num_children = children_list.size();
    node->children = (ast_node_t**)vtr::malloc(sizeof(ast_node_t*) * node->num_children);

    for (size_t i = 0; i < node->num_children; i++) {
        node->children[i] = children_list[i];
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: add_child_to_node)
 *-------------------------------------------------------------------------------------------*/
void add_child_to_node(ast_node_t* node, ast_node_t* child) {
    /* Handle case where we have an empty statement. */
    if (child == NULL)
        return;

    /* allocate space for the children */
    node->children = (ast_node_t**)vtr::realloc(node->children, sizeof(ast_node_t*) * (node->num_children + 1));
    node->num_children++;
    node->children[node->num_children - 1] = child;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_child_to_node_at_index)
 *-------------------------------------------------------------------------------------------*/
void add_child_to_node_at_index(ast_node_t* node, ast_node_t* child, int index) {
    if (index < 0 || index > node->num_children)
        return;

    node->children = (ast_node_t**)vtr::realloc(node->children, sizeof(ast_node_t*) * (node->num_children + 1));
    for (int i = node->num_children; i > index; i--) {
        node->children[i] = node->children[i - 1];
    }

    node->children[index] = child;
    node->num_children++;
}

/*---------------------------------------------------------------------------------------------
 * (function: remove_child_from_node_at_index)
 *-------------------------------------------------------------------------------------------*/
void remove_child_from_node_at_index(ast_node_t* node, int index) {
    if (index < 0 || index >= node->num_children)
        return;

    if (node->children[index]) {
        node->children[index] = free_whole_tree(node->children[index]);
        node->children[index] = NULL;
    }

    for (int i = index; i < (node->num_children - 1); i++) {
        node->children[i] = node->children[i + 1];
        node->children[i + 1] = NULL;
    }

    node->num_children--;
}

/*---------------------------------------------------------------------------------------------
 * (function: expand_node_list_at)
 *-------------------------------------------------------------------------------------------*/
ast_node_t** expand_node_list_at(ast_node_t** list, long old_size, long to_add, long start_idx) {
    if (list) {
        long new_size = old_size + to_add;
        list = (ast_node_t**)vtr::realloc(list, sizeof(ast_node_t*) * new_size);

        long i;
        for (i = new_size - 1; i >= (start_idx + to_add); i--) {
            list[i] = list[i - to_add];
        }
        return list;
    }
    return NULL;
}

/*---------------------------------------------------------------------------------------------
 * (function: make_concat_into_list_of_strings)
 * 	0th idx will be the MSbit
 *-------------------------------------------------------------------------------------------*/
void make_concat_into_list_of_strings(ast_node_t* concat_top, char* instance_name_prefix, sc_hierarchy* local_ref) {
    long i;
    int j;
    ast_node_t* rnode[3] = {0};

    concat_top->types.concat.num_bit_strings = 0;
    concat_top->types.concat.bit_strings = NULL;

    /* recursively do all embedded concats */
    for (i = 0; i < concat_top->num_children; i++) {
        if (concat_top->children[i]->type == CONCATENATE) {
            make_concat_into_list_of_strings(concat_top->children[i], instance_name_prefix, local_ref);
        }
    }

    for (i = 0; i < concat_top->num_children; i++) {
        if (concat_top->children[i]->type == IDENTIFIERS) {
            char* temp_string = make_full_ref_name(NULL, NULL, NULL, concat_top->children[i]->types.identifier, -1);
            ast_node_t* var_declare = resolve_hierarchical_name_reference(local_ref, temp_string);
            if (var_declare == NULL) {
                error_message(NETLIST_ERROR, concat_top->line_number, concat_top->file_number, "Missing declaration of this symbol %s\n", temp_string);
            } else {
                if (var_declare->children[1] == NULL) {
                    concat_top->types.concat.num_bit_strings++;
                    concat_top->types.concat.bit_strings = (char**)vtr::realloc(concat_top->types.concat.bit_strings, sizeof(char*) * (concat_top->types.concat.num_bit_strings));
                    concat_top->types.concat.bit_strings[concat_top->types.concat.num_bit_strings - 1] = get_name_of_pin_at_bit(concat_top->children[i], -1, instance_name_prefix, local_ref);
                } else if (var_declare->children[3] == NULL) {
                    /* reverse thorugh the range since highest bit in index will be lower in the string indx */
                    rnode[1] = var_declare->children[1];
                    rnode[2] = var_declare->children[2];
                    oassert(rnode[1]->type == NUMBERS && rnode[2]->type == NUMBERS);

                    for (j = rnode[1]->types.vnumber->get_value() - rnode[2]->types.vnumber->get_value(); j >= 0; j--) {
                        concat_top->types.concat.num_bit_strings++;
                        concat_top->types.concat.bit_strings = (char**)vtr::realloc(concat_top->types.concat.bit_strings, sizeof(char*) * (concat_top->types.concat.num_bit_strings));
                        concat_top->types.concat.bit_strings[concat_top->types.concat.num_bit_strings - 1] = get_name_of_pin_at_bit(concat_top->children[i], j, instance_name_prefix, local_ref);
                    }
                } else if (var_declare->children[3] != NULL) {
                    oassert(false);
                }
            }
            vtr::free(temp_string);
        } else if (concat_top->children[i]->type == ARRAY_REF) {
            concat_top->types.concat.num_bit_strings++;
            concat_top->types.concat.bit_strings = (char**)vtr::realloc(concat_top->types.concat.bit_strings, sizeof(char*) * (concat_top->types.concat.num_bit_strings));
            concat_top->types.concat.bit_strings[concat_top->types.concat.num_bit_strings - 1] = get_name_of_pin_at_bit(concat_top->children[i], 0, instance_name_prefix, local_ref);
        } else if (concat_top->children[i]->type == RANGE_REF) {
            rnode[1] = concat_top->children[i]->children[1];
            rnode[2] = concat_top->children[i]->children[2];
            oassert(rnode[1]->type == NUMBERS && rnode[2]->type == NUMBERS);
            oassert(rnode[1]->types.vnumber->get_value() >= rnode[2]->types.vnumber->get_value());
            int width = abs(rnode[1]->types.vnumber->get_value() - rnode[2]->types.vnumber->get_value()) + 1;

            //for (j = rnode[1]->types.vnumber->get_value() - rnode[2]->types.vnumber->get_value(); j >= 0; j--)
            // Changed to forward to fix concatenation bug.
            for (j = 0; j < width; j++) {
                concat_top->types.concat.num_bit_strings++;
                concat_top->types.concat.bit_strings = (char**)vtr::realloc(concat_top->types.concat.bit_strings, sizeof(char*) * (concat_top->types.concat.num_bit_strings));
                concat_top->types.concat.bit_strings[concat_top->types.concat.num_bit_strings - 1] = get_name_of_pin_at_bit(concat_top->children[i], ((rnode[1]->types.vnumber->get_value() - rnode[2]->types.vnumber->get_value())) - j, instance_name_prefix, local_ref);
            }
        } else if (concat_top->children[i]->type == NUMBERS) {
            if (concat_top->children[i]->types.vnumber->is_defined_size()) {
                // Changed to reverse to fix concatenation bug.
                for (j = concat_top->children[i]->types.vnumber->size() - 1; j >= 0; j--) {
                    concat_top->types.concat.num_bit_strings++;
                    concat_top->types.concat.bit_strings = (char**)vtr::realloc(concat_top->types.concat.bit_strings, sizeof(char*) * (concat_top->types.concat.num_bit_strings));
                    concat_top->types.concat.bit_strings[concat_top->types.concat.num_bit_strings - 1] = get_name_of_pin_at_bit(concat_top->children[i], j, instance_name_prefix, local_ref);
                }
            } else {
                error_message(NETLIST_ERROR, concat_top->line_number, concat_top->file_number, "%s", "Unsized constants cannot be concatenated.\n");
            }
        } else if (concat_top->children[i]->type == CONCATENATE) {
            /* forward through list since we build concatenate list in idx order of MSB at index 0 and LSB at index list_size */
            for (j = 0; j < concat_top->children[i]->types.concat.num_bit_strings; j++) {
                concat_top->types.concat.num_bit_strings++;
                concat_top->types.concat.bit_strings = (char**)vtr::realloc(concat_top->types.concat.bit_strings, sizeof(char*) * (concat_top->types.concat.num_bit_strings));
                concat_top->types.concat.bit_strings[concat_top->types.concat.num_bit_strings - 1] = get_name_of_pin_at_bit(concat_top->children[i], j, instance_name_prefix, local_ref);
            }
        } else {
            error_message(NETLIST_ERROR, concat_top->line_number, concat_top->file_number, "%s", "Unsupported operation within a concatenation.\n");
        }
    }
}

/*---------------------------------------------------------------------------
 * (function: change_to_number_node)
 * change the original AST node to a NUMBER node or change the value of the node
 * originate from the function: create_tree_node_number() in ast_util.c
 *-------------------------------------------------------------------------*/
void change_to_number_node(ast_node_t* node, long value) {
    return change_to_number_node(node, VNumber(value));
}

/*---------------------------------------------------------------------------
 * (function: change_to_number_node)
 * change the original AST node to a NUMBER node or change the value of the node
 * originate from the function: create_tree_node_number() in ast_util.c
 *-------------------------------------------------------------------------*/
void change_to_number_node(ast_node_t* node, VNumber number) {
    char* temp_ident = NULL;
    if (node->types.identifier != NULL) {
        temp_ident = strdup(node->types.identifier);
    }
    free_assignement_of_node_keep_tree(node);
    free_all_children(node);

    node->type = NUMBERS;
    node->types.identifier = temp_ident;
    node->types.vnumber = new VNumber(number);
}

/*---------------------------------------------------------------------------------------------
 * (function: get_name of_port_at_bit)
 * 	Assume module connections can be one of: Array entry, Concat, Signal, Array range reference
 *-------------------------------------------------------------------------------------------*/
char* get_name_of_var_declare_at_bit(ast_node_t* var_declare, int bit) {
    char* return_string = NULL;

    /* calculate the port details */
    if (var_declare->children[1] == NULL) {
        oassert(bit == 0);
        return_string = make_full_ref_name(NULL, NULL, NULL, var_declare->children[0]->types.identifier, -1);
    } else if (var_declare->children[3] == NULL) {
        oassert(var_declare->children[2]->type == NUMBERS);
        return_string = make_full_ref_name(NULL, NULL, NULL, var_declare->children[0]->types.identifier, var_declare->children[2]->types.vnumber->get_value() + bit);
    } else if (var_declare->children[3] != NULL) {
        /* MEMORY output */
        oassert(false);
    }

    return return_string;
}

/*---------------------------------------------------------------------------------------------
 * (function: get_name of_port_at_bit)
 * 	Assume module connections can be one of: Array entry, Concat, Signal, Array range reference
 *-------------------------------------------------------------------------------------------*/
char* get_name_of_pin_at_bit(ast_node_t* var_node, int bit, char* instance_name_prefix, sc_hierarchy* local_ref) {
    char* return_string = NULL;
    ast_node_t* rnode[3] = {0};

    // STRING_CACHE *local_symbol_table_sc = local_ref->local_symbol_table_sc;

    if (var_node->type == ARRAY_REF) {
        oassert(var_node->children[0]->type == IDENTIFIERS);
        oassert(var_node->children[1]->type == NUMBERS);
        return_string = make_full_ref_name(NULL, NULL, NULL, var_node->children[0]->types.identifier, (int)var_node->children[1]->types.vnumber->get_value());
    } else if (var_node->type == RANGE_REF) {
        oassert(bit >= 0);

        rnode[1] = var_node->children[1];
        rnode[2] = var_node->children[2];

        oassert(var_node->children[0]->type == IDENTIFIERS);
        oassert(rnode[1]->type == NUMBERS);
        oassert(rnode[2]->type == NUMBERS);
        oassert(rnode[1]->types.vnumber->get_value() >= rnode[2]->types.vnumber->get_value() + bit);

        return_string = make_full_ref_name(NULL, NULL, NULL, var_node->children[0]->types.identifier, rnode[2]->types.vnumber->get_value() + bit);
    } else if ((var_node->type == IDENTIFIERS) && (bit == -1)) {
        return_string = make_full_ref_name(NULL, NULL, NULL, var_node->types.identifier, -1);
    } else if (var_node->type == IDENTIFIERS) {
        ast_node_t* symbol_node;
        int pin_index = 0;

        if ((symbol_node = resolve_hierarchical_name_reference(local_ref, var_node->types.identifier)) == NULL) {
            error_message(NETLIST_ERROR, var_node->line_number, var_node->file_number, "Missing declaration of this symbol %s\n", var_node->types.identifier);
        }

        if (symbol_node->children[1] == NULL) {
            pin_index = bit;
        } else if (symbol_node->children[3] == NULL) {
            oassert(symbol_node->children[2]->type == NUMBERS);
            pin_index = symbol_node->children[2]->types.vnumber->get_value() + bit;
        } else
            oassert(false);

        return_string = make_full_ref_name(NULL, NULL, NULL, var_node->types.identifier, pin_index);
    } else if (var_node->type == NUMBERS) {
        return_string = get_name_of_pin_number(var_node, bit);
    } else if (var_node->type == CONCATENATE && bit >= 0) {
        if (var_node->types.concat.num_bit_strings == 0) {
            return_string = NULL;
            oassert(false);
        } else {
            ast_node_t* temp = var_node;
            if (var_node->types.concat.num_bit_strings == -1) {
                /* If this hasn't been made into a string list then do it */
                make_concat_into_list_of_strings(var_node, instance_name_prefix, local_ref);
            }

            return_string = (char*)vtr::malloc(sizeof(char) * strlen(var_node->types.concat.bit_strings[bit]) + 1);
            odin_sprintf(return_string, "%s", var_node->types.concat.bit_strings[bit]);

            if (var_node != temp) {
                var_node = free_whole_tree(var_node);
            }
        }
    } else {
        return_string = NULL;

        error_message(NETLIST_ERROR, var_node->line_number, var_node->file_number, "Unsupported variable type. var_node->type = %s\n", ast_node_name_based_on_ids(var_node));
    }

    return return_string;
}

char** get_name_of_pins_number(ast_node_t* var_node, int /*start*/, int width) {
    char** return_string;
    oassert(var_node->type == NUMBERS);

    return_string = (char**)vtr::malloc(sizeof(char*) * width);
    int i;
    for (i = 0; i < width; i++) {
        return_string[i] = get_name_of_pin_number(var_node, i);
    }
    return return_string;
}

char* get_name_of_pin_number(ast_node_t* var_node, int bit) {
    oassert(var_node->type == NUMBERS);
    char* return_string = NULL;

    if (bit == -1)
        bit = 0;

    BitSpace::bit_value_t c = var_node->types.vnumber->get_bit_from_lsb(bit);
    switch (c) {
        case BitSpace::_1:
            return_string = vtr::strdup(ONE_VCC_CNS);
            break;
        case BitSpace::_0:
            return_string = vtr::strdup(ZERO_GND_ZERO);
            break;
        case BitSpace::_x:
            return_string = vtr::strdup(ZERO_GND_ZERO);
            break;
        case BitSpace::_z:
            return_string = vtr::strdup(ZERO_PAD_ZERO);
            break;
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
char_list_t* get_name_of_pins(ast_node_t* var_node, char* instance_name_prefix, sc_hierarchy* local_ref) {
    char** return_string = NULL;
    char_list_t* return_list = (char_list_t*)vtr::malloc(sizeof(char_list_t));
    ast_node_t* rnode[3] = {0};
    int i;
    int width = 0;

    if (var_node->type == ARRAY_REF) {
        width = 1;
        return_string = (char**)vtr::malloc(sizeof(char*));

        rnode[1] = var_node->children[1];
        oassert(rnode[1] && rnode[1]->type == NUMBERS);
        oassert(var_node->children[0]->type == IDENTIFIERS);

        return_string[0] = make_full_ref_name(NULL, NULL, NULL, var_node->children[0]->types.identifier, rnode[1]->types.vnumber->get_value());
    } else if (var_node->type == RANGE_REF) {
        rnode[0] = var_node->children[0];
        rnode[1] = var_node->children[1];
        rnode[2] = var_node->children[2];

        oassert(rnode[1]->type == NUMBERS && rnode[2]->type == NUMBERS);
        width = abs(rnode[1]->types.vnumber->get_value() - rnode[2]->types.vnumber->get_value()) + 1;
        if (rnode[0]->type == IDENTIFIERS) {
            return_string = (char**)vtr::malloc(sizeof(char*) * width);
            for (i = 0; i < width; i++)
                return_string[i] = make_full_ref_name(NULL, NULL, NULL, rnode[0]->types.identifier, rnode[2]->types.vnumber->get_value() + i);
        } else {
            oassert(rnode[0]->type == NUMBERS);
            return_string = get_name_of_pins_number(rnode[0], rnode[2]->types.vnumber->get_value(), width);
        }
    } else if (var_node->type == IDENTIFIERS) {
        /* need to look in the symbol table for details about this identifier (i.e. is it a port) */

        char* temp_string = make_full_ref_name(NULL, NULL, NULL, var_node->types.identifier, -1);
        ast_node_t* sym_node = resolve_hierarchical_name_reference(local_ref, temp_string);

        if (sym_node == NULL) {
            error_message(NETLIST_ERROR, var_node->line_number, var_node->file_number, "Missing declaration of this symbol %s\n", temp_string);
        }

        vtr::free(temp_string);

        if (sym_node && sym_node->children && sym_node->type) {
            if (sym_node->children[1] == NULL || sym_node->type == BLOCKING_STATEMENT) {
                width = 1;
                return_string = (char**)vtr::malloc(sizeof(char*) * width);
                return_string[0] = make_full_ref_name(NULL, NULL, NULL, var_node->types.identifier, -1);
            } else if (sym_node->children[2] != NULL && sym_node->children[3] == NULL) {
                int index = 0;

                rnode[1] = sym_node->children[1];
                rnode[2] = sym_node->children[2];
                oassert(rnode[1]->type == NUMBERS && rnode[2]->type == NUMBERS);

                width = (rnode[1]->types.vnumber->get_value() - rnode[2]->types.vnumber->get_value() + 1);
                return_string = (char**)vtr::malloc(sizeof(char*) * width);

                for (i = 0; i < width; i++) {
                    return_string[index] = make_full_ref_name(NULL, NULL, NULL, var_node->types.identifier,
                                                              i + rnode[2]->types.vnumber->get_value());
                    index++;
                }
            }

            else if (sym_node->children[3] != NULL) {
                oassert(false);
            }
        }
    } else if (var_node->type == NUMBERS) {
        width = var_node->types.vnumber->size();
        return_string = get_name_of_pins_number(var_node, 0, width);
    } else if (var_node->type == CONCATENATE) {
        if (var_node->types.concat.num_bit_strings == 0) {
            oassert(false);
        } else {
            if (var_node->types.concat.num_bit_strings == -1) {
                make_concat_into_list_of_strings(var_node, instance_name_prefix, local_ref);
            }

            width = var_node->types.concat.num_bit_strings;
            return_string = (char**)vtr::malloc(sizeof(char*) * width);
            for (i = 0; i < width; i++) // 0th bit is MSB so need to access reverse
            {
                return_string[i] = (char*)vtr::malloc(sizeof(char) * strlen(var_node->types.concat.bit_strings[var_node->types.concat.num_bit_strings - i - 1]) + 1);
                odin_sprintf(return_string[i], "%s", var_node->types.concat.bit_strings[var_node->types.concat.num_bit_strings - i - 1]);
            }
        }
    } else {
        oassert(false);
    }

    return_list->strings = return_string;
    return_list->num_strings = width;

    return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function: get_name_of_pins_with_prefix
 *-------------------------------------------------------------------------------------------*/
char_list_t* get_name_of_pins_with_prefix(ast_node_t* var_node, char* instance_name_prefix, sc_hierarchy* local_ref) {
    int i;
    char_list_t* return_list;
    char* temp_str;

    /* get the list */
    return_list = get_name_of_pins(var_node, instance_name_prefix, local_ref);

    for (i = 0; i < return_list->num_strings; i++) {
        temp_str = vtr::strdup(return_list->strings[i]);
        vtr::free(return_list->strings[i]);
        return_list->strings[i] = make_full_ref_name(instance_name_prefix, NULL, NULL, temp_str, -1);
        vtr::free(temp_str);
    }

    return return_list;
}

/*----------------------------------------------------------------------------
 * (function: get_size_of_variable)
 *--------------------------------------------------------------------------*/
long get_size_of_variable(ast_node_t* node, sc_hierarchy* local_ref) {
    long assignment_size = 0;
    long sc_spot = 0;
    ast_node_t* var_declare = NULL;

    STRING_CACHE* local_param_table_sc = local_ref->local_param_table_sc;

    switch (node->type) {
        case IDENTIFIERS: {
            sc_spot = sc_lookup_string(local_param_table_sc, node->types.identifier);
            if (sc_spot > -1) {
                var_declare = ast_node_deep_copy((ast_node_t*)local_param_table_sc->data[sc_spot]);

                if (node_is_constant(var_declare)) {
                    assignment_size = var_declare->types.vnumber->size();
                } else {
                    error_message(NETLIST_ERROR, node->line_number, node->file_number, "Parameter %s is not a constant expression\n", node->types.identifier);
                }

                free_whole_tree(var_declare);
                return assignment_size;
            }

            ast_node_t* sym_node = resolve_hierarchical_name_reference(local_ref, node->types.identifier);
            if (sym_node != NULL) {
                var_declare = sym_node; // (ast_node_t *)local_symbol_table_sc->data[sc_spot];
                break;
            }

            error_message(NETLIST_ERROR, node->line_number, node->file_number, "Missing declaration of this symbol %s\n", node->types.identifier);
        } break;

        case ARRAY_REF: {
            ast_node_t* sym_node = resolve_hierarchical_name_reference(local_ref, node->children[0]->types.identifier);
            if (sym_node != NULL) {
                var_declare = sym_node;
                break;
            }

            error_message(NETLIST_ERROR, node->children[0]->line_number, node->children[0]->file_number, "Missing declaration of this symbol %s\n", node->children[0]->types.identifier);
        } break;

        case RANGE_REF: {
            var_declare = node;
        } break;

        case CONCATENATE: {
            assignment_size = resolve_concat_sizes(node, local_ref);
            return assignment_size;
        }

        default:
            oassert(false);
            break;
    }

    if (var_declare && !(var_declare->children[1])) {
        assignment_size = 1;
    } else if (var_declare && var_declare->children[1] && var_declare->children[2]) {
        ast_node_t* node_max = var_declare->children[1];
        ast_node_t* node_min = var_declare->children[2];

        oassert(node_min->type == NUMBERS && node_max->type == NUMBERS);
        long range_max = node_max->types.vnumber->get_value();
        long range_min = node_min->types.vnumber->get_value();

        assignment_size = (range_max - range_min) + 1;
    }

    oassert(assignment_size != 0);
    return assignment_size;
}

/*---------------------------------------------------------------------------------------------
 * (function: move_ast_node)
 * move node from src to dest
 *-------------------------------------------------------------------------------------------*/
void move_ast_node(ast_node_t* src, ast_node_t* dest, ast_node_t* node) {
    int i, j;
    int number;
    number = src->num_children;
    for (i = 0; i < number; i++) {
        if (src->children[i]->unique_count == node->unique_count) {
            number = number - 1;
            src->num_children = number;
            for (j = i; j < number; j++) {
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
ast_node_t* ast_node_deep_copy(ast_node_t* node) {
    ast_node_t* node_copy = ast_node_copy(node);

    if (node && node_copy) {
        //Create a new child list;
        node_copy->children = (ast_node_t**)vtr::calloc(node->num_children, sizeof(ast_node_t*));

        //Recursively copy its children
        for (long i = 0; i < node->num_children; i++) {
            oassert(node != node->children[i]);
            node_copy->children[i] = ast_node_deep_copy(node->children[i]);
        }
    }

    return node_copy;
}

/*---------------------------------------------------------------------------------------------
 * (function: ast_node_copy)
 * copy node; return new node
 *-------------------------------------------------------------------------------------------*/
ast_node_t* ast_node_copy(ast_node_t* node) {
    ast_node_t* node_copy;

    if (node == NULL) {
        return NULL;
    }

    //Copy node
    node_copy = (ast_node_t*)vtr::calloc(1, sizeof(ast_node_t));
    memcpy(node_copy, node, sizeof(ast_node_t));

    //Copy contents
    if (node->type == NUMBERS && node->types.vnumber)
        node_copy->types.vnumber = new VNumber((*node->types.vnumber));

    node_copy->types.identifier = vtr::strdup(node->types.identifier);
    node_copy->children = NULL;

    return node_copy;
}

/*---------------------------------------------------------------------------
 * (function: expand_power)
 * expand power operation into multiplication
 *-------------------------------------------------------------------------*/
static void expand_power(ast_node_t** node) {
    /* create a node for this array reference */
    ast_node_t* new_node = NULL;

    ast_node_t* expression1 = (*node)->children[0];
    ast_node_t* expression2 = (*node)->children[1];

    /* allocate child nodes to this node */
    int len = expression2->types.vnumber->get_value();
    if (expression1->type == NUMBERS) {
        int len1 = expression1->types.vnumber->get_value();
        long powRes = pow(len1, len);
        new_node = create_tree_node_number(powRes, (*node)->line_number, (*node)->file_number);
    } else {
        if (len == 0) {
            new_node = create_tree_node_number(1L, (*node)->line_number, (*node)->file_number);
        } else {
            new_node = ast_node_deep_copy(expression1);
            for (int i = 1; i < len; i++) {
                ast_node_t* temp_node = create_node_w_type(BINARY_OPERATION, (*node)->line_number, (*node)->file_number);
                temp_node->types.operation.op = MULTIPLY;

                allocate_children_to_node(temp_node, {ast_node_deep_copy(expression1), new_node});
                new_node = temp_node;
            }
        }
    }
    free_whole_tree(*node);
    *node = new_node;
}

/*---------------------------------------------------------------------------
 * (function: change_ast_node)
 * check if the number is the power of 2
 *-------------------------------------------------------------------------*/
static void check_node_number(ast_node_t* parent, ast_node_t* child, int flag) {
    long power = 0;
    long number = child->types.vnumber->get_value();
    if (number <= 1)
        return;
    while (((number % 2) == 0) && number > 1) // While number is even and > 1
    {
        number >>= 1;
        power++;
    }
    if (number == 1) // the previous number is a power of 2
    {
        change_to_number_node(child, power);
        if (flag == 1) // multiply
            parent->types.operation.op = SL;
        else if (flag == 2) // multiply and needs to move children nodes
        {
            parent->types.operation.op = SL;
            parent->children[0] = parent->children[1];
            parent->children[1] = child;
        } else if (flag == 3) // divide
            parent->types.operation.op = SR;
    }
}

/*---------------------------------------------------------------------------
 * (function: change_ast_node)
 *  check the children nodes of an operation node
 *-------------------------------------------------------------------------*/
static void check_binary_operation(ast_node_t** node) {
    if ((*node) && (*node)->type == BINARY_OPERATION) {
        switch ((*node)->types.operation.op) {
            case MULTIPLY:
                if ((*node)->children[0]->type == IDENTIFIERS && (*node)->children[1]->type == NUMBERS)
                    check_node_number((*node), (*node)->children[1], 1); // 1 means multiply and don't need to move children nodes
                if ((*node)->children[0]->type == NUMBERS && (*node)->children[1]->type == IDENTIFIERS)
                    check_node_number((*node), (*node)->children[0], 2); // 2 means multiply and needs to move children nodes
                break;
            case DIVIDE:
                if (!node_is_constant((*node)->children[1]))
                    error_message(NETLIST_ERROR, (*node)->line_number, (*node)->file_number, "%s", "Odin only supports constant expressions as divisors\n");
                if ((*node)->children[0]->type == IDENTIFIERS && (*node)->children[1]->type == NUMBERS)
                    check_node_number((*node), (*node)->children[1], 3); // 3 means divide
                break;
            case POWER:
                if (!node_is_constant((*node)->children[1]))
                    error_message(NETLIST_ERROR, (*node)->line_number, (*node)->file_number, "%s", "Odin only supports constant expressions as exponents\n");
                expand_power(node);
                break;
            default:
                break;
        }
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: calculate_unary)
 * TODO ! what does verilog say about !d'00001 ??
 * we currently make it as small as possible(+1) such that d'00001 becomes (in bin) 01
 * so it would become 10, is that what is defined?
 *-------------------------------------------------------------------------------------------*/
ast_node_t* fold_unary(ast_node_t** node) {
    if (!node || !(*node))
        return NULL;

    operation_list op_id = (*node)->types.operation.op;
    ast_node_t* child_0 = (*node)->children[0];

    if (node_is_constant(child_0)) {
        bool success = false;
        VNumber voperand_0 = *(child_0->types.vnumber);
        VNumber vresult;

        switch (op_id) {
            case LOGICAL_NOT:
                vresult = V_LOGICAL_NOT(voperand_0);
                success = true;
                break;

            case BITWISE_NOT:
                vresult = V_BITWISE_NOT(voperand_0);
                success = true;
                break;

            case BUF_NODE:
                vresult = voperand_0;
                success = true;
                break;

            case MINUS:
                vresult = V_MINUS(voperand_0);
                success = true;
                break;

            case ADD:
                vresult = V_ADD(voperand_0);
                success = true;
                break;

            case BITWISE_OR:
                vresult = V_BITWISE_OR(voperand_0);
                success = true;
                break;

            case BITWISE_NAND:
                vresult = V_BITWISE_NAND(voperand_0);
                success = true;
                break;

            case BITWISE_NOR:
                vresult = V_BITWISE_NOR(voperand_0);
                success = true;
                break;

            case BITWISE_XNOR:
                vresult = V_BITWISE_XNOR(voperand_0);
                success = true;
                break;

            case BITWISE_XOR:
                vresult = V_BITWISE_XOR(voperand_0);
                success = true;
                break;

            case CLOG2:
                if (voperand_0.size() > ODIN_STD_BITWIDTH)
                    warning_message(PARSE_ERROR, (*node)->line_number, (*node)->file_number, "argument is %ld-bits but ODIN limit is %lu-bits \n", voperand_0.size(), ODIN_STD_BITWIDTH);

                vresult = VNumber(clog2(voperand_0.get_value(), voperand_0.size()));
                success = true;
                break;

            case UNSIGNED:
                vresult = V_UNSIGNED(voperand_0);
                success = true;
                break;

            case SIGNED:
                vresult = V_SIGNED(voperand_0);
                success = true;
                break;

            default:
                break;
        }

        if (success) {
            ast_node_t* new_num = create_tree_node_number(vresult, (*node)->line_number, (*node)->file_number);
            return new_num;
        }
    } else if (op_id == CLOG2) {
        /* $clog2() argument must be a constant expression */
        error_message(PARSE_ERROR, (*node)->line_number, current_parse_file, "%s", "Argument must be constant\n");
    }

    return NULL;
}

/*---------------------------------------------------------------------------------------------
 * (function: calculate_binary)
 *-------------------------------------------------------------------------------------------*/
ast_node_t* fold_binary(ast_node_t** node) {
    if (!node || !(*node))
        return NULL;

    operation_list op_id = (*node)->types.operation.op;
    ast_node_t* child_0 = (*node)->children[0];
    ast_node_t* child_1 = (*node)->children[1];

    if (node_is_constant(child_0) && node_is_constant(child_1)) {
        bool success = false;
        VNumber voperand_0 = *(child_0->types.vnumber);
        VNumber voperand_1 = *(child_1->types.vnumber);
        VNumber vresult;

        switch (op_id) {
            case ADD:
                vresult = V_ADD(voperand_0, voperand_1);
                success = true;
                break;

            case MINUS:
                vresult = V_MINUS(voperand_0, voperand_1);
                success = true;
                break;

            case MULTIPLY:
                vresult = V_MULTIPLY(voperand_0, voperand_1);
                success = true;
                break;

            case POWER:
                vresult = V_POWER(voperand_0, voperand_1);
                success = true;
                break;

            case DIVIDE:
                vresult = V_DIV(voperand_0, voperand_1);
                success = true;
                break;

            case BITWISE_XOR:
                vresult = V_BITWISE_XOR(voperand_0, voperand_1);
                success = true;
                break;

            case BITWISE_XNOR:
                vresult = V_BITWISE_XNOR(voperand_0, voperand_1);
                success = true;
                break;

            case BITWISE_AND:
                vresult = V_BITWISE_AND(voperand_0, voperand_1);
                success = true;
                break;

            case BITWISE_NAND:
                vresult = V_BITWISE_NAND(voperand_0, voperand_1);
                success = true;
                break;

            case BITWISE_OR:
                vresult = V_BITWISE_OR(voperand_0, voperand_1);
                success = true;
                break;

            case BITWISE_NOR:
                vresult = V_BITWISE_NOR(voperand_0, voperand_1);
                success = true;
                break;

            case SL:
                vresult = V_SHIFT_LEFT(voperand_0, voperand_1);
                success = true;
                break;

            case SR:
                vresult = V_SHIFT_RIGHT(voperand_0, voperand_1);
                success = true;
                break;

            case ASR: {
                vresult = V_SIGNED_SHIFT_RIGHT(voperand_0, voperand_1);
                success = true;
                break;
            }
            case LOGICAL_AND:
                vresult = V_LOGICAL_AND(voperand_0, voperand_1);
                success = true;
                break;

            case LOGICAL_OR:
                vresult = V_LOGICAL_OR(voperand_0, voperand_1);
                success = true;
                break;

            case MODULO:
                vresult = V_MOD(voperand_0, voperand_1);
                success = true;
                break;

            case LT:
                vresult = V_LT(voperand_0, voperand_1);
                success = true;
                break;

            case GT:
                vresult = V_GT(voperand_0, voperand_1);
                success = true;
                break;

            case LOGICAL_EQUAL:
                vresult = V_EQUAL(voperand_0, voperand_1);
                success = true;
                break;

            case NOT_EQUAL:
                vresult = V_NOT_EQUAL(voperand_0, voperand_1);
                success = true;
                break;

            case LTE:
                vresult = V_LE(voperand_0, voperand_1);
                success = true;
                break;

            case GTE:
                vresult = V_GE(voperand_0, voperand_1);
                success = true;
                break;

            case CASE_EQUAL:
                vresult = V_CASE_EQUAL(voperand_0, voperand_1);
                success = true;
                break;

            case CASE_NOT_EQUAL:
                vresult = V_CASE_NOT_EQUAL(voperand_0, voperand_1);
                success = true;
                break;

            default:
                break;
        }

        if (success) {
            ast_node_t* new_num = create_tree_node_number(vresult, (*node)->line_number, (*node)->file_number);
            return new_num;
        }
    } else {
        check_binary_operation(node);
        oassert(!((*node)->type == BINARY_OPERATION && (*node)->types.operation.op == POWER));
    }
    return NULL;
}

/*---------------------------------------------------------------------------------------------
 * (function: node_is_constant)
 *-------------------------------------------------------------------------------------------*/
bool node_is_constant(ast_node_t* node) {
    if (node && node->type == NUMBERS && node->types.vnumber != nullptr) {
        return true;
    }
    return false;
}

/*---------------------------------------------------------------------------
 * (function: initial_node)
 *-------------------------------------------------------------------------*/
void initial_node(ast_node_t* new_node, ids id, int line_number, int file_number, int unique_counter) {
    new_node->type = id;
    new_node->children = NULL;
    new_node->num_children = 0;
    new_node->unique_count = unique_counter; //++count_id;
    new_node->line_number = line_number;
    new_node->file_number = file_number;
    new_node->far_tag = 0;
    new_node->high_number = 0;
    new_node->hb_port = 0;
    new_node->net_node = 0;
    new_node->types.vnumber = nullptr;
    new_node->types.identifier = NULL;
    new_node->types.hierarchy = NULL;
    new_node->chunk_size = 1;
}

/*---------------------------------------------------------------------------
 * (function: clog2)
 *-------------------------------------------------------------------------*/
long clog2(long value_in, int length) {
    if (value_in == 0) return 0;

    long result;

    /* negative numbers may be larger than they need to be */
    if (value_in < 0 && value_in >= std::numeric_limits<int32_t>::min()) return 32;

    if (length > 32) {
        uint64_t unsigned_val = (uint64_t)value_in;
        result = (long)ceil(log2((double)unsigned_val));
    } else {
        uint32_t unsigned_val = (uint32_t)value_in;
        result = (long)ceil(log2((double)unsigned_val));
    }

    return result;
}

/*---------------------------------------------------------------------------
 * (function: resolve_concat_sizes)
 *-------------------------------------------------------------------------*/
long resolve_concat_sizes(ast_node_t* node_top, sc_hierarchy* local_ref) {
    long concatenation_size = 0;

    if (node_top) {
        switch (node_top->type) {
            case CONCATENATE: {
                for (int i = 0; i < node_top->num_children; i++) {
                    concatenation_size += resolve_concat_sizes(node_top->children[i], local_ref);
                }
            } break;

            case IDENTIFIERS:
            case ARRAY_REF:
            case RANGE_REF: {
                concatenation_size += get_size_of_variable(node_top, local_ref);
            } break;

            case BINARY_OPERATION:
            case UNARY_OPERATION: {
                long max_size = 0;
                for (int i = 0; i < node_top->num_children; i++) {
                    long this_size = resolve_concat_sizes(node_top->children[i], local_ref);
                    if (this_size > max_size) max_size = this_size;
                }
                concatenation_size += max_size;
            } break;

            case IF_Q: {
                /* check true/false expressions */
                long true_length = resolve_concat_sizes(node_top->children[1], local_ref);
                long false_length = resolve_concat_sizes(node_top->children[2], local_ref);
                concatenation_size += (true_length > false_length) ? true_length : false_length;
            } break;

            case NUMBERS: {
                /* verify that the number that this represents is sized */
                if (!(node_top->types.vnumber->is_defined_size())) {
                    error_message(NETLIST_ERROR, node_top->line_number, node_top->file_number, "%s", "Unsized constants cannot be concatenated.\n");
                }
                concatenation_size += node_top->types.vnumber->size();
            } break;

            default: {
                error_message(NETLIST_ERROR, node_top->line_number, node_top->file_number, "%s", "Unsupported operation within a concatenation.\n");
            }
        }
    }

    return concatenation_size;
}
