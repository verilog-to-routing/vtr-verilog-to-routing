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

struct e_data {
    int pass;
    int index;

    struct
    {
        ast_node_t** defparams;
        sc_hierarchy** sc_hierarchies; // for the correct scope
        int num_defparams;
    } defparam;

    struct
    {
        ast_node_t** generate_constructs;
        ast_node_t** generate_parents;
        int* generate_indexes;         // for the correct location of the parent's child
        sc_hierarchy** sc_hierarchies; // for the correct scope
        int num_generate_constructs;
    } generate;
};

ast_node_t* build_hierarchy(ast_node_t* node, ast_node_t* parent, int index, sc_hierarchy* local_ref, bool is_generate_region, bool fold_expressions, e_data* data);
ast_node_t* finalize_ast(ast_node_t* node, ast_node_t* parent, sc_hierarchy* local_ref, bool is_generate_region, bool fold_expressions);
ast_node_t* reduce_expressions(ast_node_t* node, sc_hierarchy* local_ref, long* max_size, long assignment_size);

void update_string_caches(sc_hierarchy* local_ref);
void update_instance_parameter_table_direct_instances(ast_node_t* instance, STRING_CACHE* instance_param_table_sc);
void update_instance_parameter_table_defparams(ast_node_t* instance, STRING_CACHE* instance_param_table_sc);
void override_parameters_for_all_instances(ast_node_t* module, sc_hierarchy* local_ref);

void convert_2D_to_1D_array(ast_node_t** var_declare);
void convert_2D_to_1D_array_ref(ast_node_t** node, sc_hierarchy* local_ref);
char* make_chunk_size_name(char* instance_name_prefix, char* array_name);
ast_node_t* get_chunk_size_node(char* instance_name_prefix, char* array_name, sc_hierarchy* local_ref);

bool verify_terminal(ast_node_t* top, ast_node_t* iterator);
void verify_genvars(ast_node_t* node, sc_hierarchy* local_ref, char*** other_genvars, int num_genvars);

ast_node_t* look_for_matching_hard_block(ast_node_t* node, char* hard_block_name, sc_hierarchy* local_ref);
ast_node_t* look_for_matching_soft_logic(ast_node_t* node, char* hard_block_name);

/*---------------------------------------------------------------------------------------------
 * (function: find_top_module)
 * 	Finds the top module based on that it is not called by anyone else
 * 	Assumes there is only one top
 *-------------------------------------------------------------------------------------------*/
ast_node_t* find_top_module(ast_t* ast) {
    ast_node_t* top_entry = NULL;

    long number_of_top_modules = 0;

    /* check for which module wasn't marked as instantiated...this one will be the top */
    std::string module_name_list("");
    std::string desired_module("");
    bool found_desired_module = true;

    if (global_args.top_level_module_name.provenance() == argparse::Provenance::SPECIFIED) {
        found_desired_module = false;
        desired_module = global_args.top_level_module_name;
        printf("Using Top Level Module: %s\n", desired_module.c_str());
    }

    if (ast->top_modules_count < 1) {
        module_name_list = "N/A";
    } else {
        for (long i = 0; i < ast->top_modules_count; i++) {
            std::string current_module = "";

            if (ast->top_modules[i]->identifier_node->types.identifier) {
                current_module = ast->top_modules[i]->identifier_node->types.identifier;
            }

            if (current_module != "") {
                if (desired_module != "" && current_module == desired_module) {
                    // append the name
                    module_name_list = std::string("\t") + current_module;
                    top_entry = ast->top_modules[i];
                    found_desired_module = true;
                    number_of_top_modules = 1;
                    break;
                } else if (!(ast->top_modules[i]->types.module.is_instantiated)) {
                    /**
                     * Check to see if the module is a hard block 
                     * a hard block is never the top level!
                     */
                    long sc_spot;

                    if (-1 == (sc_spot = sc_lookup_string(hard_block_names, current_module.c_str()))) {
                        // append the name
                        module_name_list += std::string("\t") + current_module;

                        if (number_of_top_modules > 0)
                            module_name_list += "\n";

                        number_of_top_modules += 1;
                        top_entry = ast->top_modules[i];
                    }
                }
            }
        }
    }

    /* check atleast one module is top ... and only one */
    if (!found_desired_module) {
        warning_message(AST, unknown_location, "Could not find the desired top level module: %s\n", desired_module.c_str());
    }

    if (number_of_top_modules > 1) {
        error_message(AST, unknown_location, "Found multiple top level modules\n%s", module_name_list.c_str());
    } else {
        printf("==========================\nDetected Top Level Module: %s\n==========================\n", module_name_list.c_str());
    }

    return top_entry;
}

int simplify_ast_module(ast_node_t** ast_module, sc_hierarchy* local_ref) {
    /* resolve constant expressions in string caches */
    bool is_module = (*ast_module)->type == MODULE ? true : false;

    /* set up elaboration data */
    e_data* data = (e_data*)vtr::calloc(1, sizeof(e_data));

    data->generate.generate_constructs = NULL;
    data->generate.num_generate_constructs = 0;
    data->generate.sc_hierarchies = NULL;

    data->defparam.defparams = NULL;
    data->defparam.num_defparams = 0;
    data->defparam.sc_hierarchies = NULL;

    data->index = -1;
    /* TODO need to also store some kind of info about what defparams have been applied to... */

    /* elaborate AST */
    /* pass 1: elaborate everything except generate constructs */
    data->pass = 1;
    *ast_module = build_hierarchy(*ast_module, NULL, -1, local_ref, true, true, data);

    /* apply defparam overrides for instances found during this pass */
    override_parameters_for_all_instances(*ast_module, local_ref);

    /* pass 2: elaborate generate constructs */
    data->pass = 2;
    for (int i = 0; i < data->generate.num_generate_constructs; i++) {
        ast_node_t* generate_node = data->generate.generate_constructs[i];
        ast_node_t* parent = data->generate.generate_parents[i];
        int index = data->generate.generate_indexes[i];
        sc_hierarchy* sc_hierarchy = data->generate.sc_hierarchies[i];

        generate_node = build_hierarchy(generate_node, parent, index, sc_hierarchy, true, true, data);
        parent->children[index] = generate_node;
    }

    /* pass 3: finish AST elaboration */
    update_string_caches(local_ref);
    *ast_module = finalize_ast(*ast_module, NULL, local_ref, is_module, true);
    *ast_module = reduce_expressions(*ast_module, local_ref, NULL, 0);

    if (data->generate.num_generate_constructs > 0) {
        vtr::free(data->generate.generate_constructs);
        vtr::free(data->generate.generate_parents);
        vtr::free(data->generate.generate_indexes);
        vtr::free(data->generate.sc_hierarchies);
    }
    vtr::free(data);
    return 1;
}

/* -----------------------------------------------------------------------------------------------------
 * (function: build_hierarchy)
 *
 *  this function finds all instances in two passes and:
 *	1. builds the official string cache list hierarchy so we can resolve identifiers
 *	2. resolves defparams so that we properly override parameters
 * ----------------------------------------------------------------------------------------------------- */
ast_node_t* build_hierarchy(ast_node_t* node, ast_node_t* parent, int index, sc_hierarchy* local_ref, bool is_generate_region, bool fold_expressions, e_data* data) {
    short* child_skip_list = NULL; // list of children not to traverse into
    short skip_children = false;   // skips the DFS completely if true

    STRING_CACHE* local_defparam_table_sc = local_ref->local_defparam_table_sc;
    STRING_CACHE* local_param_table_sc = local_ref->local_param_table_sc;

    if (node) {
        if (node->num_children > 0) {
            child_skip_list = (short*)vtr::calloc(node->num_children, sizeof(short));
        }

        /* pre-amble */
        switch (node->type) {
            case MODULE: {
                break;
            }
            case MODULE_INSTANCE: {
                /* flip hard blocks */

                if (node->num_children == 1) {
                    // check ports
                    ast_node_t* connect_list = node->children[0]->children[0]->children[0];
                    bool is_ordered_list;
                    if (connect_list->children[0]->identifier_node) {
                        is_ordered_list = false; // name was specified
                    } else {
                        is_ordered_list = true;
                    }

                    for (int i = 0; i < connect_list->num_children; i++) {
                        if ((connect_list->children[i]->identifier_node && is_ordered_list)
                            || (!connect_list->children[i]->identifier_node && !is_ordered_list)) {
                            error_message(AST, node->loc,
                                          "%s", "Cannot mix port connections by name and port connections by ordered list\n");
                        }
                    }

                    char* module_ref_name = node->children[0]->identifier_node->types.identifier;
                    long sc_spot = sc_lookup_string(hard_block_names, module_ref_name);

                    /* TODO: strcmp on "multiply", "adder" for soft logic implementation? */
                    if (
                        sc_spot != -1
                        || !strcmp(module_ref_name, SINGLE_PORT_RAM_string)
                        || !strcmp(module_ref_name, DUAL_PORT_RAM_string)) {
                        ast_node_t* hb_node = look_for_matching_hard_block(node->children[0], module_ref_name, local_ref);
                        if (hb_node != node->identifier_node) {
                            free_whole_tree(node);
                            node = hb_node;

                            break;
                        }
                    }
                }
                break;
            }
            case FOR: {
                if (data->pass == 1 && is_generate_region) {
                    // parameters haven't been resolved yet; don't elaborate until the second pass
                    data->generate.generate_constructs = (ast_node_t**)vtr::realloc(data->generate.generate_constructs, sizeof(ast_node_t*) * (data->generate.num_generate_constructs + 1));
                    data->generate.generate_constructs[data->generate.num_generate_constructs] = node;

                    data->generate.generate_parents = (ast_node_t**)vtr::realloc(data->generate.generate_parents, sizeof(ast_node_t*) * (data->generate.num_generate_constructs + 1));
                    data->generate.generate_parents[data->generate.num_generate_constructs] = parent;

                    data->generate.generate_indexes = (int*)vtr::realloc(data->generate.generate_indexes, sizeof(int) * (data->generate.num_generate_constructs + 1));
                    data->generate.generate_indexes[data->generate.num_generate_constructs] = index;

                    data->generate.sc_hierarchies = (sc_hierarchy**)vtr::realloc(data->generate.sc_hierarchies, sizeof(sc_hierarchy*) * (data->generate.num_generate_constructs + 1));
                    data->generate.sc_hierarchies[data->generate.num_generate_constructs] = local_ref;

                    data->generate.num_generate_constructs++;
                    skip_children = true;

                    break;
                } else if (!(data->pass == 2)) {
                    break;
                }

                oassert(child_skip_list);

                // look ahead for parameters in loop conditions
                node->children[0] = build_hierarchy(node->children[0], node, 0, local_ref, is_generate_region, true, data);
                node->children[1] = build_hierarchy(node->children[1], node, 1, local_ref, is_generate_region, true, data);
                node->children[2] = build_hierarchy(node->children[2], node, 2, local_ref, is_generate_region, true, data);

                // update skip list
                child_skip_list[0] = true;
                child_skip_list[1] = true;
                child_skip_list[2] = true;

                // if this is a loop generate construct, verify constant expressions
                if (is_generate_region) {
                    ast_node_t* iterator = NULL;
                    ast_node_t* initial = node->children[0];
                    ast_node_t* compare_expression = node->children[1];
                    ast_node_t* terminal = node->children[2];

                    char** genvar_list = NULL;
                    verify_genvars(node, local_ref, &genvar_list, 0);
                    if (genvar_list) vtr::free(genvar_list);

                    iterator = resolve_hierarchical_name_reference(local_ref, initial->children[0]->types.identifier);
                    oassert(iterator != NULL);

                    if (!(node_is_constant(initial->children[1]))
                        || !(node_is_constant(compare_expression->children[1]))
                        || !(verify_terminal(terminal->children[1], iterator))) {
                        error_message(AST, node->loc,
                                      "%s", "Loop generate construct conditions must be constant expressions");
                    }
                }

                int num_unrolled = 0;
                node = unroll_for_loop(node, parent, &num_unrolled, local_ref, is_generate_region);
                for (int i = index; i < (num_unrolled + index); i++) {
                    parent->children[i] = build_hierarchy(parent->children[i], parent, i, local_ref, is_generate_region, true, data);
                }
                break;
            }
            case IF: {
                if (data->pass == 1 && is_generate_region) {
                    // parameters haven't been resolved yet; don't elaborate until the second pass
                    data->generate.generate_constructs = (ast_node_t**)vtr::realloc(data->generate.generate_constructs, sizeof(ast_node_t*) * (data->generate.num_generate_constructs + 1));
                    data->generate.generate_constructs[data->generate.num_generate_constructs] = node;

                    data->generate.generate_parents = (ast_node_t**)vtr::realloc(data->generate.generate_parents, sizeof(ast_node_t*) * (data->generate.num_generate_constructs + 1));
                    data->generate.generate_parents[data->generate.num_generate_constructs] = parent;

                    data->generate.generate_indexes = (int*)vtr::realloc(data->generate.generate_indexes, sizeof(int) * (data->generate.num_generate_constructs + 1));
                    data->generate.generate_indexes[data->generate.num_generate_constructs] = index;

                    data->generate.sc_hierarchies = (sc_hierarchy**)vtr::realloc(data->generate.sc_hierarchies, sizeof(sc_hierarchy*) * (data->generate.num_generate_constructs + 1));
                    data->generate.sc_hierarchies[data->generate.num_generate_constructs] = local_ref;

                    data->generate.num_generate_constructs++;
                    skip_children = true;

                    break;
                } else if (!(data->pass == 2)) {
                    break;
                }

                ast_node_t* to_return = NULL;

                node->children[0] = build_hierarchy(node->children[0], node, 0, local_ref, is_generate_region, true, data);
                ast_node_t* child_condition = node->children[0];
                if (node_is_constant(child_condition)) {
                    VNumber condition = *(child_condition->types.vnumber);

                    if (V_TRUE(condition)) {
                        to_return = node->children[1];
                        node->children[1] = NULL;
                    } else if (V_FALSE(condition)) {
                        to_return = node->children[2];
                        node->children[2] = NULL;
                    } else if (is_generate_region) {
                        error_message(AST, node->loc,
                                      "%s", "Could not resolve conditional generate construct");
                    }
                    free_whole_tree(node);
                    node = to_return;
                    // otherwise we keep it as is to build the circuitry
                } else if (is_generate_region) {
                    error_message(AST, node->loc,
                                  "%s", "Could not resolve conditional generate construct");
                }

                if (to_return != NULL) {
                    if (to_return->type == BLOCK && to_return->children[0]->type != IF && (is_generate_region || to_return->types.identifier != NULL)) {
                        // must create scope; parent has access to named block but not unnamed,
                        // and child always has access to parent
                        sc_hierarchy* new_hierarchy = init_sc_hierarchy();
                        to_return->types.hierarchy = new_hierarchy;

                        new_hierarchy->top_node = to_return;
                        new_hierarchy->parent = local_ref;

                        if (to_return->types.identifier != NULL) {
                            // parent's children
                            local_ref->block_children = (sc_hierarchy**)vtr::realloc(local_ref->block_children, sizeof(sc_hierarchy*) * (local_ref->num_block_children + 1));
                            local_ref->block_children[local_ref->num_block_children] = new_hierarchy;
                            local_ref->num_block_children++;

                            // scope id/instance name prefix
                            new_hierarchy->scope_id = to_return->types.identifier;
                            new_hierarchy->instance_name_prefix = make_full_ref_name(local_ref->instance_name_prefix, NULL, to_return->types.identifier, NULL, -1);
                        } else if (is_generate_region) {
                            // create a unique scope id/instance name prefix for internal use
                            local_ref->num_unnamed_genblks++;
                            std::string new_scope_id("genblk" + std::to_string(local_ref->num_unnamed_genblks));
                            new_hierarchy->scope_id = vtr::strdup(new_scope_id.c_str());
                            new_hierarchy->instance_name_prefix = make_full_ref_name(local_ref->instance_name_prefix, NULL, new_hierarchy->scope_id, NULL, -1);
                        }

                        /* string caches */
                        create_param_table_for_scope(to_return, new_hierarchy);
                        create_symbol_table_for_scope(to_return, new_hierarchy);
                    }
                }

                break;
            }
            case CASE: {
                if (data->pass == 1 && is_generate_region) {
                    // parameters haven't been resolved yet; don't elaborate until the second pass
                    data->generate.generate_constructs = (ast_node_t**)vtr::realloc(data->generate.generate_constructs, sizeof(ast_node_t*) * (data->generate.num_generate_constructs + 1));
                    data->generate.generate_constructs[data->generate.num_generate_constructs] = node;

                    data->generate.generate_parents = (ast_node_t**)vtr::realloc(data->generate.generate_parents, sizeof(ast_node_t*) * (data->generate.num_generate_constructs + 1));
                    data->generate.generate_parents[data->generate.num_generate_constructs] = parent;

                    data->generate.generate_indexes = (int*)vtr::realloc(data->generate.generate_indexes, sizeof(int) * (data->generate.num_generate_constructs + 1));
                    data->generate.generate_indexes[data->generate.num_generate_constructs] = index;

                    data->generate.sc_hierarchies = (sc_hierarchy**)vtr::realloc(data->generate.sc_hierarchies, sizeof(sc_hierarchy*) * (data->generate.num_generate_constructs + 1));
                    data->generate.sc_hierarchies[data->generate.num_generate_constructs] = local_ref;

                    data->generate.num_generate_constructs++;
                    skip_children = true;

                    break;
                } else if (!(data->pass == 2)) {
                    break;
                }

                ast_node_t* to_return = NULL;

                node->children[0] = build_hierarchy(node->children[0], node, 0, local_ref, is_generate_region, true, data);
                ast_node_t* child_condition = node->children[0];
                if (node_is_constant(child_condition)) {
                    ast_node_t* case_list = node->children[1];
                    for (int i = 0; i < case_list->num_children; i++) {
                        ast_node_t* item = case_list->children[i];
                        if (i == case_list->num_children - 1 && item->type == CASE_DEFAULT) {
                            to_return = item->children[0];
                            item->children[0] = NULL;
                        } else {
                            if (item->type != CASE_ITEM) {
                                error_message(AST, node->loc,
                                              "%s", "Default case must only be the last case");
                            }

                            item->children[0] = build_hierarchy(item->children[0], item, 0, local_ref, is_generate_region, true, data);
                            if (node_is_constant(item->children[0])) {
                                VNumber eval = V_CASE_EQUAL(*(child_condition->types.vnumber), *(item->children[0]->types.vnumber));
                                if (V_TRUE(eval)) {
                                    to_return = item->children[1];
                                    item->children[1] = NULL;
                                    break;
                                }
                            } else if (is_generate_region) {
                                error_message(AST, node->loc,
                                              "%s", "Could not resolve conditional generate construct");
                            } else {
                                /* encountered non-constant item - don't continue searching */
                                break;
                            }
                        }
                    }

                    if (to_return) {
                        if (to_return->type == BLOCK && (is_generate_region || to_return->types.identifier != NULL)) {
                            // must create scope; parent has access to named block but not unnamed,
                            // and child always has access to parent
                            sc_hierarchy* new_hierarchy = init_sc_hierarchy();
                            to_return->types.hierarchy = new_hierarchy;

                            new_hierarchy->top_node = to_return;
                            new_hierarchy->parent = local_ref;

                            if (to_return->types.identifier != NULL) {
                                // parent's children
                                local_ref->block_children = (sc_hierarchy**)vtr::realloc(local_ref->block_children, sizeof(sc_hierarchy*) * (local_ref->num_block_children + 1));
                                local_ref->block_children[local_ref->num_block_children] = new_hierarchy;
                                local_ref->num_block_children++;

                                // scope id/instance name prefix
                                new_hierarchy->scope_id = to_return->types.identifier;
                                new_hierarchy->instance_name_prefix = make_full_ref_name(local_ref->instance_name_prefix, NULL, to_return->types.identifier, NULL, -1);
                            } else if (is_generate_region) {
                                // create a unique scope id/instance name prefix for internal use
                                local_ref->num_unnamed_genblks++;
                                std::string new_scope_id("genblk" + std::to_string(local_ref->num_unnamed_genblks));
                                new_hierarchy->scope_id = vtr::strdup(new_scope_id.c_str());
                                new_hierarchy->instance_name_prefix = make_full_ref_name(local_ref->instance_name_prefix, NULL, new_hierarchy->scope_id, NULL, -1);
                            }

                            /* string caches */
                            create_param_table_for_scope(to_return, new_hierarchy);
                            create_symbol_table_for_scope(to_return, new_hierarchy);
                        }

                        free_whole_tree(node);
                        node = to_return;
                    } else if (is_generate_region) {
                        /* no case */
                        error_message(AST, node->loc,
                                      "%s", "Could not resolve conditional generate construct");
                    }
                } else if (is_generate_region) {
                    error_message(AST, node->loc,
                                  "%s", "Could not resolve conditional generate construct");
                }

                break;
            }
            case REPLICATE: {
                if (data->pass == 2) {
                    node->children[0] = build_hierarchy(node->children[0], node, 0, local_ref, is_generate_region, true, data);
                    if (!(node_is_constant(node->children[0]))) {
                        error_message(AST, node->loc,
                                      "%s", "Replication constant must be a constant expression");
                    } else if (node->children[0]->types.vnumber->has_unknown()) {
                        error_message(AST, node->loc,
                                      "%s", "Replication constant cannot contain x or z");
                    }

                    ast_node_t* new_node = NULL;
                    int64_t value = node->children[0]->types.vnumber->get_value();
                    if (value > 0) {
                        new_node = create_node_w_type(CONCATENATE, node->loc);
                        for (int64_t i = 0; i < value; i++) {
                            add_child_to_node(new_node, ast_node_deep_copy(node->children[1]));
                        }
                    }

                    node->children[1] = free_whole_tree(node->children[1]);
                    node->children[1] = NULL;
                    node->num_children--;

                    node->children[0] = free_whole_tree(node->children[0]);
                    node->children[0] = new_node;
                }
                break;
            }
            case IDENTIFIERS: {
                if (data->pass == 2 && local_param_table_sc != NULL && node->types.identifier) {
                    long sc_spot = sc_lookup_string(local_param_table_sc, node->types.identifier);
                    if (sc_spot != -1) {
                        ast_node_t* newNode = ast_node_deep_copy((ast_node_t*)local_param_table_sc->data[sc_spot]);

                        if (newNode->type != NUMBERS) {
                            newNode = build_hierarchy(newNode, parent, index, local_ref, is_generate_region, true, data);
                            oassert(newNode->type == NUMBERS);

                            /* this forces parameter values as unsigned, since we don't currently support signed keyword...
                             * must be changed once support is added */
                            VNumber* temp = newNode->types.vnumber;
                            VNumber* to_unsigned = new VNumber(V_UNSIGNED(*temp));
                            newNode->types.vnumber = to_unsigned;
                            delete temp;

                            if (newNode->type != NUMBERS) {
                                error_message(AST, node->loc, "Parameter %s is not a constant expression\n", node->types.identifier);
                            }
                        }

                        node = free_whole_tree(node);
                        node = newNode;
                    }
                }
                break;
            }
            case BLOCKING_STATEMENT: // fallthrough
            case NON_BLOCKING_STATEMENT: {
                // size of resolved expressions can depend on left-hand side of assignment
                fold_expressions = false;
                break;
            }
            case RANGE_REF: // fallthrough
            case ARRAY_REF: {
                // sizes are independent of whether these are contained in blocking/non-blocking statements
                fold_expressions = true;
                break;
            }
            case TASK:      //fallthrough
            case STATEMENT: //fallthough
            case FUNCTION:  // fallthrough
            case INITIAL:   // fallthrough
            case ALWAYS: {
                is_generate_region = false;
            }
            default: {
                break;
            }
        }
    }
    if (node) {
        if (child_skip_list && node->num_children > 0 && skip_children == false) {
            /* use while loop and boolean to prevent optimizations
             * since number of children may change during recursion */

            bool done = false;
            int i = 0;

            sc_hierarchy* new_ref = NULL;
            if (node->type == BLOCK) {
                if (node->types.hierarchy) {
                    new_ref = node->types.hierarchy;
                }
            }

            if (!new_ref) {
                new_ref = local_ref;
            }

            /* traverse all the children */
            while (done == false) {
                int num_original_children = node->num_children;
                if (child_skip_list[i] == false) {
                    /* recurse */
                    ast_node_t* old_child = node->children[i];
                    ast_node_t* new_child = build_hierarchy(old_child, node, i, new_ref, is_generate_region, fold_expressions, data);
                    node->children[i] = new_child;

                    if (old_child != new_child) {
                        /* recurse on this child again in case it can be further reduced 
                         * (e.g. resolved generate constructs) */
                        i--;
                    }
                }

                if (num_original_children != node->num_children) {
                    child_skip_list = (short*)vtr::realloc(child_skip_list, sizeof(short) * node->num_children);
                    for (int j = num_original_children; j < node->num_children; j++) {
                        child_skip_list[j] = false;
                    }
                }

                i++;
                if (i >= node->num_children) {
                    done = true;
                }
            }
        }
    }
    if (node) {
        /* post-amble */
        switch (node->type) {
            case MODULE: {
                int i, j;

                /* update module instance parameters */
                for (i = 0; i < node->types.module.size_module_instantiations; i++) {
                    ast_node_t* instance_node = node->types.module.module_instantiations_instance[i];
                    std::string instance_name(instance_node->children[0]->identifier_node->types.identifier);

                    // check defparams for a match
                    if (local_defparam_table_sc) {
                        for (j = 0; j < local_defparam_table_sc->free; j++) {
                            // a matching parameter at this scope will just have the instance name
                            // and parameter, e.g. defparam inst_1.a = 4
                            std::string defparam_ref(local_defparam_table_sc->string[j]);
                            size_t param_loc = defparam_ref.find_last_of('.');
                            std::string param_ref = defparam_ref.substr(param_loc + 1, std::string::npos);
                            defparam_ref.erase(param_loc, std::string::npos);

                            if (defparam_ref.find_first_of('.') != std::string::npos) {
                                continue;
                            }

                            // if this point is reached then the defparam is referencing an instance
                            // that was created in this module (or an instance in an encompassing module...
                            // but we'll add that functionality later (TODO))

                            if (strcmp(defparam_ref.c_str(), instance_name.c_str()) == 0) {
                                ast_node_t* instance_param_list = instance_node->children[0]->children[1];
                                ast_node_t* param_node = ast_node_deep_copy((ast_node_t*)local_defparam_table_sc->data[j]);
                                // identifier expected to be parameter name only
                                vtr::free(param_node->identifier_node->types.identifier);
                                param_node->identifier_node->types.identifier = vtr::strdup(param_ref.c_str());

                                if (!instance_param_list) {
                                    instance_param_list = create_node_w_type(MODULE_PARAMETER_LIST, instance_node->children[0]->loc);
                                    instance_node->children[0]->children[1] = instance_param_list;
                                }

                                add_child_to_node(instance_param_list, param_node);
                                // set to null to find conflicts later
                                local_defparam_table_sc->data[j] = NULL;
                                break;
                            }
                        }
                    }
                }

                for (i = 0; i < node->types.function.size_function_instantiations; i++) {
                    /* make the stringed up module instance name - instance name is
                     * MODULE_INSTANCE->MODULE_NAMED_INSTANCE(child[1])->IDENTIFIER(child[0]).
                     * module name is MODULE_INSTANCE->IDENTIFIER(child[0])
                     */
                    ast_node_t* temp_instance = node->types.function.function_instantiations_instance[i];
                    char* instance_name_prefix = local_ref->instance_name_prefix;

                    char* temp_instance_name = make_full_ref_name(instance_name_prefix,
                                                                  temp_instance->identifier_node->types.identifier,
                                                                  temp_instance->children[0]->identifier_node->types.identifier,
                                                                  NULL, -1);

                    long sc_spot;
                    /* lookup the name of the function associated with this instantiated point */
                    if ((sc_spot = sc_lookup_string(module_names_to_idx, temp_instance->identifier_node->types.identifier)) == -1) {
                        error_message(AST, node->loc,
                                      "Can't find function name %s\n", temp_instance->identifier_node->types.identifier);
                    }

                    /* make a unique copy of this function */
                    ast_node_t* instance = ast_node_deep_copy((ast_node_t*)module_names_to_idx->data[sc_spot]);

                    long sc_spot_2 = sc_add_string(module_names_to_idx, temp_instance_name);
                    oassert(sc_spot_2 > -1 && module_names_to_idx->data[sc_spot_2] == NULL);
                    module_names_to_idx->data[sc_spot_2] = (void*)instance;
                    add_top_module_to_ast(verilog_ast, instance);

                    /* create the string cache list for the instantiated module */
                    sc_hierarchy* original_sc_hierarchy = ((ast_node_t*)module_names_to_idx->data[sc_spot])->types.hierarchy;
                    sc_hierarchy* function_sc_hierarchy = copy_sc_hierarchy(original_sc_hierarchy);
                    function_sc_hierarchy->parent = local_ref;
                    function_sc_hierarchy->instance_name_prefix = vtr::strdup(temp_instance_name);
                    function_sc_hierarchy->scope_id = vtr::strdup(temp_instance->children[0]->identifier_node->types.identifier);
                    function_sc_hierarchy->top_node = instance;

                    /* update parent string cache list */
                    int num_function_children = local_ref->num_function_children;
                    local_ref->function_children = (sc_hierarchy**)vtr::realloc(local_ref->function_children, sizeof(sc_hierarchy*) * (num_function_children + 1));
                    local_ref->function_children[i] = function_sc_hierarchy;
                    local_ref->num_function_children++;

                    /* elaboration */
                    instance = build_hierarchy(instance, NULL, -1, function_sc_hierarchy, false, true, data);

                    /* clean up */
                    vtr::free(temp_instance_name);
                }

                for (i = 0; i < node->types.task.size_task_instantiations; i++) {
                    /* make the stringed up module instance name - instance name is
                     * MODULE_INSTANCE->MODULE_NAMED_INSTANCE(child[1])->IDENTIFIER(child[0]).
                     * module name is MODULE_INSTANCE->IDENTIFIER(child[0])
                     */
                    ast_node_t* temp_instance = node->types.task.task_instantiations_instance[i];
                    char* instance_name_prefix = local_ref->instance_name_prefix;

                    char* temp_instance_name = make_full_ref_name(instance_name_prefix,
                                                                  temp_instance->identifier_node->types.identifier,
                                                                  temp_instance->children[0]->identifier_node->types.identifier,
                                                                  NULL, -1);

                    long sc_spot;
                    /* lookup the name of the task associated with this instantiated point */
                    if ((sc_spot = sc_lookup_string(module_names_to_idx, temp_instance->identifier_node->types.identifier)) == -1) {
                        error_message(AST, node->loc,
                                      "Can't find task name %s\n", temp_instance->identifier_node->types.identifier);
                    }

                    /* make a unique copy of this task */
                    ast_node_t* instance = ast_node_deep_copy((ast_node_t*)module_names_to_idx->data[sc_spot]);

                    long sc_spot_2 = sc_add_string(module_names_to_idx, temp_instance_name);
                    oassert(sc_spot_2 > -1 && module_names_to_idx->data[sc_spot_2] == NULL);
                    module_names_to_idx->data[sc_spot_2] = (void*)instance;
                    add_top_module_to_ast(verilog_ast, instance);

                    /* create the string cache list for the instantiated module */
                    sc_hierarchy* original_sc_hierarchy = ((ast_node_t*)module_names_to_idx->data[sc_spot])->types.hierarchy;
                    sc_hierarchy* task_sc_hierarchy = copy_sc_hierarchy(original_sc_hierarchy);
                    task_sc_hierarchy->parent = local_ref;
                    task_sc_hierarchy->instance_name_prefix = vtr::strdup(temp_instance_name);
                    task_sc_hierarchy->scope_id = vtr::strdup(temp_instance->children[0]->identifier_node->types.identifier);

                    /* update parent string cache list */
                    int num_task_children = local_ref->num_task_children;
                    local_ref->task_children = (sc_hierarchy**)vtr::realloc(local_ref->task_children, sizeof(sc_hierarchy*) * (num_task_children + 1));
                    local_ref->task_children[i] = task_sc_hierarchy;
                    local_ref->num_task_children++;

                    /* elaboration */
                    instance = build_hierarchy(instance, NULL, -1, task_sc_hierarchy, false, true, data);

                    /* clean up */
                    vtr::free(temp_instance_name);
                }

                break;
            }
            case MODULE_INSTANCE: {
                /* this should be encountered once generate constructs are resolved, so we don't have
                 * stray instances that never get instantiated */

                if (node->num_children == 1 && node->identifier_node) {
                    sc_hierarchy* this_ref = local_ref;
                    std::string new_id(node->children[0]->identifier_node->types.identifier);
                    while (this_ref->top_node->type != MODULE) {
                        std::string this_scope(this_ref->scope_id);
                        new_id = this_scope + "." + new_id;
                        this_ref = this_ref->parent;
                        oassert(this_ref);
                    }
                    ast_node_t* module = this_ref->top_node;
                    oassert(module->type == MODULE);

                    int num_instances = module->types.module.size_module_instantiations;
                    char* new_instance_name = vtr::strdup(new_id.c_str());
                    bool found = false;
                    for (int i = 0; i < num_instances; i++) {
                        ast_node_t* temp_instance_node = module->types.module.module_instantiations_instance[i];
                        char* temp_instance_name = temp_instance_node->children[0]->identifier_node->types.identifier;
                        if (strcmp(node->children[0]->identifier_node->types.identifier, temp_instance_name) == 0
                            || strcmp(new_instance_name, temp_instance_name) == 0) {
                            found = true;
                        }
                    }

                    if (found) {
                        vtr::free(new_instance_name);
                        break;
                    }

                    vtr::free(node->children[0]->identifier_node->types.identifier);
                    node->children[0]->identifier_node->types.identifier = new_instance_name;

                    module->types.module.module_instantiations_instance = (ast_node_t**)vtr::realloc(module->types.module.module_instantiations_instance, sizeof(ast_node_t*) * (num_instances + 1));
                    module->types.module.module_instantiations_instance[num_instances] = node;
                    module->types.module.size_module_instantiations++;

                    /* make the stringed up module instance name - instance name is
                     * MODULE_INSTANCE->MODULE_NAMED_INSTANCE(child[1])->IDENTIFIER(child[0]).
                     * module name is MODULE_INSTANCE->IDENTIFIER(child[0])
                     */
                    ast_node_t* temp_instance = node;
                    char* instance_name_prefix = this_ref->instance_name_prefix;

                    char* temp_instance_name = make_full_ref_name(instance_name_prefix,
                                                                  temp_instance->identifier_node->types.identifier,
                                                                  temp_instance->children[0]->identifier_node->types.identifier,
                                                                  NULL, -1);

                    long sc_spot;
                    /* lookup the name of the module associated with this instantiated point */
                    if ((sc_spot = sc_lookup_string(module_names_to_idx, temp_instance->identifier_node->types.identifier)) == -1) {
                        error_message(AST, node->loc,
                                      "Can't find module name %s\n", temp_instance->identifier_node->types.identifier);
                    }

                    /* make a unique copy of this module */
                    ast_node_t* instance = ast_node_deep_copy((ast_node_t*)module_names_to_idx->data[sc_spot]);

                    long sc_spot_2 = sc_add_string(module_names_to_idx, temp_instance_name);
                    oassert(sc_spot_2 > -1 && module_names_to_idx->data[sc_spot_2] == NULL);
                    module_names_to_idx->data[sc_spot_2] = (void*)instance;

                    add_top_module_to_ast(verilog_ast, instance);

                    /* create the string cache list for the instantiated module */
                    sc_hierarchy* original_sc_hierarchy = ((ast_node_t*)module_names_to_idx->data[sc_spot])->types.hierarchy;
                    sc_hierarchy* module_sc_hierarchy = copy_sc_hierarchy(original_sc_hierarchy);
                    instance->types.hierarchy = module_sc_hierarchy;

                    module_sc_hierarchy->parent = local_ref;
                    module_sc_hierarchy->instance_name_prefix = vtr::strdup(temp_instance_name);
                    module_sc_hierarchy->scope_id = vtr::strdup(temp_instance->children[0]->identifier_node->types.identifier);
                    module_sc_hierarchy->top_node = instance;

                    /* update parent string cache list */
                    int num_module_children = local_ref->num_module_children;
                    local_ref->module_children = (sc_hierarchy**)vtr::realloc(local_ref->module_children, sizeof(sc_hierarchy*) * (num_module_children + 1));
                    local_ref->module_children[num_module_children] = module_sc_hierarchy;
                    local_ref->num_module_children++;

                    /* elaboration */
                    if (data->pass == 2) {
                        /* make sure parameters are updated */
                        /* TODO apply remaining defparams and check for conflicts */
                        update_instance_parameter_table_direct_instances(temp_instance, module_sc_hierarchy->local_param_table_sc);
                        update_instance_parameter_table_defparams(temp_instance, module_sc_hierarchy->local_param_table_sc);
                    }
                    instance = build_hierarchy(instance, NULL, -1, module_sc_hierarchy, true, true, data);

                    /* clean up */
                    vtr::free(temp_instance_name);
                }
                break;
            }
            case TERNARY_OPERATION: {
                if (node->children[0] == NULL) {
                    /* resulting from replication of zero */
                    error_message(AST, node->loc,
                                  "%s", "Cannot perform operation with nonexistent value");
                }
                ast_node_t* child_condition = node->children[0];
                if (node_is_constant(child_condition)) {
                    VNumber condition = *(child_condition->types.vnumber);
                    ast_node_t* tmp = NULL;
                    if (V_TRUE(condition)) {
                        tmp = node->children[1];
                        node->children[1] = NULL;
                    } else {
                        tmp = node->children[2];
                        node->children[2] = NULL;
                    }
                    free_whole_tree(node);
                    node = tmp;
                }
                break;
            }
            case BINARY_OPERATION: {
                if (node->children[0] == NULL || node->children[1] == NULL) {
                    /* resulting from replication of zero */
                    error_message(AST, node->loc,
                                  "%s", "Cannot perform operation with nonexistent value");
                }

                /* only fold if size can be self-determined */
                if (data->pass == 2 && fold_expressions) {
                    ast_node_t* new_node = fold_binary(&node);
                    if (node_is_constant(new_node)) {
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
            case UNARY_OPERATION: {
                if (node->children[0] == NULL) {
                    /* resulting from replication of zero */
                    error_message(AST, node->loc,
                                  "%s", "Cannot perform operation with nonexistent value");
                }

                /* only fold if size can be self-determined */
                if (data->pass == 2 && fold_expressions) {
                    ast_node_t* new_node = fold_unary(&node);
                    if (node_is_constant(new_node)) {
                        /* clean up */
                        free_resolved_children(node);

                        change_to_number_node(node, VNumber(*(new_node->types.vnumber), new_node->types.vnumber->size()));
                    }
                    new_node = free_whole_tree(new_node);
                }
                break;
            }
            case REPLICATE: {
                /* remove intermediate REPLICATE node */
                if (data->pass == 2) {
                    ast_node_t* child = node->children[0];
                    node->children[0] = NULL;
                    node = free_whole_tree(node);
                    node = child;
                }

                break;
            }
            case CONCATENATE: {
                if (data->pass == 2 && node->num_children > 0) {
                    long current = 0;
                    long previous = -1;
                    bool previous_is_constant = false;

                    while (current < node->num_children) {
                        bool current_is_constant = node_is_constant(node->children[current]);

                        if (previous_is_constant && current_is_constant) {
                            resolve_concat_sizes(node->children[previous], local_ref);
                            resolve_concat_sizes(node->children[current], local_ref);

                            VNumber new_value = V_CONCAT({*(node->children[previous]->types.vnumber), *(node->children[current]->types.vnumber)});

                            node->children[current] = free_whole_tree(node->children[current]);

                            delete node->children[previous]->types.vnumber;
                            node->children[previous]->types.vnumber = new VNumber(new_value);
                        } else if (node->children[current] != NULL) {
                            previous += 1;
                            previous_is_constant = current_is_constant;
                            node->children[previous] = node->children[current];
                        }
                        current += 1;
                    }

                    node->num_children = previous + 1;

                    if (node->num_children == 0) {
                        // could we simply warn and continue ? any ways we can recover rather than fail?
                        /* resulting replication(s) of zero */
                        error_message(AST, node->loc,
                                      "%s", "Cannot concatenate zero bitstrings");
                    }

                    // node was all constant
                    if (node->num_children == 1) {
                        ast_node_t* tmp = node->children[0];
                        node->children[0] = NULL;
                        free_whole_tree(node);
                        node = tmp;
                    }
                }

                break;
            }
            case STATEMENT: //fallthrough
            case TASK:      //fallthrough
            case FUNCTION:  // fallthrough
            case INITIAL:   // fallthrough
            case ALWAYS: {
                is_generate_region = true;
            }
            default: {
                break;
            }
        }
    }

    if (child_skip_list) {
        child_skip_list = (short*)vtr::free(child_skip_list);
    }

    return node;
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
ast_node_t* finalize_ast(ast_node_t* node, ast_node_t* parent, sc_hierarchy* local_ref, bool is_generate_region, bool fold_expressions) {
    short* child_skip_list = NULL; // list of children not to traverse into
    short skip_children = false;   // skips the DFS completely if true

    STRING_CACHE* local_param_table_sc = local_ref->local_param_table_sc;
    STRING_CACHE* local_symbol_table_sc = local_ref->local_symbol_table_sc;

    if (node) {
        if (node->num_children > 0) {
            child_skip_list = (short*)vtr::calloc(node->num_children, sizeof(short));
        }

        /* pre-amble */
        switch (node->type) {
            case MODULE: {
                oassert(child_skip_list);
                break;
            }
            case FUNCTION: {
                if (parent != NULL) {
                    skip_children = true;
                }
                break;
            }
            case FUNCTION_INSTANCE: {
                // check ports
                ast_node_t* connect_list = node->children[0]->children[0];
                bool is_ordered_list;
                if (connect_list->children[1]->identifier_node) // skip first connection
                {
                    is_ordered_list = false; // name was specified
                } else {
                    is_ordered_list = true;
                }

                for (int i = 1; i < connect_list->num_children; i++) {
                    if ((connect_list->children[i]->identifier_node && is_ordered_list)
                        || (!connect_list->children[i]->identifier_node && !is_ordered_list)) {
                        error_message(AST, node->loc,
                                      "%s", "Cannot mix port connections by name and port connections by ordered list\n");
                    }
                }
                break;
            }
            case TASK: {
                if (parent != NULL) {
                    skip_children = true;
                }
                break;
            }
            case TASK_INSTANCE: {
                // check ports
                ast_node_t* connect_list = node->children[0]->children[0];
                bool is_ordered_list;
                if (connect_list->children[1]->children[0]) // skip first connection
                {
                    is_ordered_list = false; // name was specified
                } else {
                    is_ordered_list = true;
                }

                for (int i = 1; i < connect_list->num_children; i++) {
                    if ((connect_list->children[i]
                         && connect_list->children[i]->children)
                        && ((connect_list->children[i]->children[0] && is_ordered_list)
                            || (!connect_list->children[i]->children[0] && !is_ordered_list))) {
                        error_message(AST, node->loc,
                                      "%s", "Cannot mix port connections by name and port connections by ordered list\n");
                    }
                }
                break;
            }
            case MODULE_ITEMS: {
                /* look in the string cache for in-line continuous assignments */
                ast_node_t** local_symbol_table = local_ref->local_symbol_table;
                int num_local_symbol_table = local_ref->num_local_symbol_table;

                for (int i = 0; i < num_local_symbol_table; i++) {
                    ast_node_t* var_declare = local_symbol_table[i];
                    if (var_declare->types.variable.is_wire && var_declare->children[4] != NULL) {
                        /* in-line assignment; split into its own continuous assignment */
                        ast_node_t* id = ast_node_copy(var_declare->identifier_node);
                        ast_node_t* val = var_declare->children[4];
                        var_declare->children[4] = NULL;

                        ast_node_t* blocking_node = newBlocking(id, val, var_declare->loc);
                        ast_node_t* assign_node = newList(ASSIGN, blocking_node, var_declare->loc);

                        add_child_to_node(node, assign_node);
                    }
                }

                break;
            }
            case VAR_DECLARE: {
                oassert(child_skip_list);
                break;
            }
            case IDENTIFIERS: {
                if (local_param_table_sc != NULL && node->types.identifier) {
                    long sc_spot = sc_lookup_string(local_param_table_sc, node->types.identifier);
                    if (sc_spot != -1) {
                        ast_node_t* newNode = ast_node_deep_copy((ast_node_t*)local_param_table_sc->data[sc_spot]);

                        if (newNode->type != NUMBERS) {
                            newNode = finalize_ast(newNode, parent, local_ref, is_generate_region, true);
                            oassert(newNode->type == NUMBERS);

                            /* this forces parameter values as unsigned, since we don't currently support signed keyword...
                             * must be changed once support is added */
                            VNumber* temp = newNode->types.vnumber;
                            VNumber* to_unsigned = new VNumber(V_UNSIGNED(*temp));
                            newNode->types.vnumber = to_unsigned;
                            delete temp;

                            if (newNode->type != NUMBERS) {
                                error_message(AST, node->loc, "Parameter %s is not a constant expression\n", node->types.identifier);
                            }
                        }

                        node = free_whole_tree(node);
                        node = newNode;
                    }
                }
                break;
            }
            case FOR: {
                oassert(child_skip_list);

                // look ahead for parameters in loop conditions
                node->children[0] = finalize_ast(node->children[0], node, local_ref, is_generate_region, true);
                node->children[1] = finalize_ast(node->children[1], node, local_ref, is_generate_region, true);
                node->children[2] = finalize_ast(node->children[2], node, local_ref, is_generate_region, true);

                // update skip list
                child_skip_list[0] = true;
                child_skip_list[1] = true;
                child_skip_list[2] = true;

                // if this is a loop generate construct, verify constant expressions
                if (is_generate_region) {
                    ast_node_t* iterator = NULL;
                    ast_node_t* initial = node->children[0];
                    ast_node_t* compare_expression = node->children[1];
                    ast_node_t* terminal = node->children[2];

                    char** genvar_list = NULL;
                    verify_genvars(node, local_ref, &genvar_list, 0);
                    if (genvar_list) vtr::free(genvar_list);

                    iterator = resolve_hierarchical_name_reference(local_ref, initial->children[0]->types.identifier);
                    oassert(iterator != NULL);

                    if (!(node_is_constant(initial->children[1]))
                        || !(node_is_constant(compare_expression->children[1]))
                        || !(verify_terminal(terminal->children[1], iterator))) {
                        error_message(AST, node->loc,
                                      "%s", "Loop generate construct conditions must be constant expressions");
                    }
                }

                int num_unrolled = 0;
                node = unroll_for_loop(node, parent, &num_unrolled, local_ref, is_generate_region);
                break;
            }
            case STATEMENT: //fallthrough
            case ALWAYS:    // fallthrough
            case INITIAL: {
                is_generate_region = false;
                break;
            }
            case BLOCKING_STATEMENT:
            case NON_BLOCKING_STATEMENT: {
                /* fill out parameters and try to resolve self-determined expressions */
                if (node->children[1]->type != FUNCTION_INSTANCE) {
                    node->children[0] = finalize_ast(node->children[0], node, local_ref, is_generate_region, true);

                    if (node->children[1]->type != NUMBERS) {
                        node->children[1] = finalize_ast(node->children[1], node, local_ref, is_generate_region, false);
                        if (node->children[1] == NULL) {
                            /* resulting from replication of zero */
                            error_message(AST, node->loc,
                                          "%s", "Cannot perform assignment with nonexistent value");
                        }
                    }

                    skip_children = true;
                }
                break;
            }
            case REPLICATE: {
                node->children[0] = finalize_ast(node->children[0], node, local_ref, is_generate_region, true);
                if (!(node_is_constant(node->children[0]))) {
                    error_message(AST, node->loc,
                                  "%s", "Replication constant must be a constant expression");
                } else if (node->children[0]->types.vnumber->has_unknown()) {
                    error_message(AST, node->loc,
                                  "%s", "Replication constant cannot contain x or z");
                }

                ast_node_t* new_node = NULL;
                int64_t value = node->children[0]->types.vnumber->get_value();
                if (value > 0) {
                    new_node = create_node_w_type(CONCATENATE, node->loc);
                    for (int64_t i = 0; i < value; i++) {
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
            case ARRAY_REF: {
                /* these need to be folded if they are binary expressions... regardless of fold_expressions */
                for (int i = 0; i < node->num_children; i++) {
                    node->children[i] = finalize_ast(node->children[i], node, local_ref, is_generate_region, true);
                }
                skip_children = true;
                break;
            }
            default: {
                break;
            }
        }

        if (child_skip_list && node->num_children > 0 && skip_children == false) {
            /* use while loop and boolean to prevent optimizations
             * since number of children may change during recursion */

            bool done = false;
            int i = 0;

            sc_hierarchy* new_ref = NULL;
            if (node->type == BLOCK) {
                if (node->types.hierarchy) {
                    new_ref = node->types.hierarchy;
                }
            }

            if (!new_ref) {
                new_ref = local_ref;
            }

            /* traverse all the children */
            while (done == false) {
                int num_original_children = node->num_children;
                if (child_skip_list[i] == false) {
                    /* recurse */
                    ast_node_t* old_child = node->children[i];
                    ast_node_t* new_child = finalize_ast(old_child, node, new_ref, is_generate_region, fold_expressions);
                    node->children[i] = new_child;

                    if (old_child != new_child) {
                        /* recurse on this child again in case it can be further reduced 
                         * (e.g. resolved generate constructs) */
                        i--;
                    }
                }

                if (num_original_children != node->num_children) {
                    child_skip_list = (short*)vtr::realloc(child_skip_list, sizeof(short) * node->num_children);
                    for (int j = num_original_children; j < node->num_children; j++) {
                        child_skip_list[j] = false;
                    }
                }

                i++;
                if (i >= node->num_children) {
                    done = true;
                }
            }
        }

        /* post-amble */
        switch (node->type) {
            case MODULE: {
                int i;

                for (i = 0; i < node->types.module.size_module_instantiations; i++) {
                    ast_node_t* instance;
                    /* make the stringed up module instance name - instance name is
                     * MODULE_INSTANCE->MODULE_NAMED_INSTANCE(child[1])->IDENTIFIER(child[0]).
                     * module name is MODULE_INSTANCE->IDENTIFIER(child[0])
                     */
                    ast_node_t* temp_instance = node->types.module.module_instantiations_instance[i];
                    char* instance_name_prefix = local_ref->instance_name_prefix;

                    char* temp_instance_name = make_full_ref_name(instance_name_prefix,
                                                                  temp_instance->identifier_node->types.identifier,
                                                                  temp_instance->children[0]->identifier_node->types.identifier,
                                                                  NULL, -1);

                    long sc_spot = sc_lookup_string(module_names_to_idx, temp_instance_name);

                    // TODO this part is temporary
                    if (sc_spot == -1) {
                        // this was from a generate construct
                        long sc_spot_2;

                        /* lookup the name of the module associated with this instantiated point */
                        if ((sc_spot_2 = sc_lookup_string(module_names_to_idx, temp_instance->identifier_node->types.identifier)) == -1) {
                            error_message(AST, node->loc,
                                          "Can't find module name %s\n", temp_instance->identifier_node->types.identifier);
                        }

                        /* make a unique copy of this module */
                        instance = ast_node_deep_copy((ast_node_t*)module_names_to_idx->data[sc_spot_2]);

                        sc_spot = sc_add_string(module_names_to_idx, temp_instance_name);
                        module_names_to_idx->data[sc_spot] = (void*)instance;
                        add_top_module_to_ast(verilog_ast, instance);

                        /* create the string cache list for the instantiated module */
                        sc_hierarchy* original_sc_hierarchy = ((ast_node_t*)module_names_to_idx->data[sc_spot])->types.hierarchy;
                        sc_hierarchy* module_sc_hierarchy = copy_sc_hierarchy(original_sc_hierarchy);
                        module_sc_hierarchy->parent = local_ref;
                        module_sc_hierarchy->instance_name_prefix = vtr::strdup(temp_instance_name);
                        module_sc_hierarchy->scope_id = vtr::strdup(temp_instance->children[0]->identifier_node->types.identifier);

                        /* update parent string cache list */
                        int num_module_children = local_ref->num_module_children;
                        local_ref->module_children = (sc_hierarchy**)vtr::realloc(local_ref->module_children, sizeof(sc_hierarchy*) * (num_module_children + 1));
                        local_ref->module_children[i] = module_sc_hierarchy;
                        local_ref->num_module_children++;
                    } else {
                        instance = (ast_node_t*)module_names_to_idx->data[sc_spot];
                    }

                    sc_hierarchy* module_sc_hierarchy = instance->types.hierarchy;

                    /* update the parameter table for the instantiated module */
                    STRING_CACHE* instance_param_table_sc = module_sc_hierarchy->local_param_table_sc;
                    update_instance_parameter_table_direct_instances(temp_instance, instance_param_table_sc);
                    update_instance_parameter_table_defparams(temp_instance, instance_param_table_sc);

                    /* elaboration */
                    update_string_caches(module_sc_hierarchy);
                    instance = finalize_ast(instance, NULL, module_sc_hierarchy, true, true);

                    /* clean up */
                    vtr::free(temp_instance_name);
                }

                for (i = 0; i < node->types.function.size_function_instantiations; i++) {
                    /* make the stringed up module instance name - instance name is
                     * MODULE_INSTANCE->MODULE_NAMED_INSTANCE(child[1])->IDENTIFIER(child[0]).
                     * module name is MODULE_INSTANCE->IDENTIFIER(child[0])
                     */
                    ast_node_t* temp_instance = node->types.function.function_instantiations_instance[i];
                    char* instance_name_prefix = local_ref->instance_name_prefix;

                    char* temp_instance_name = make_full_ref_name(instance_name_prefix,
                                                                  temp_instance->identifier_node->types.identifier,
                                                                  temp_instance->children[0]->identifier_node->types.identifier,
                                                                  NULL, -1);

                    long sc_spot = sc_lookup_string(module_names_to_idx, temp_instance_name);
                    oassert(sc_spot > -1 && module_names_to_idx->data[sc_spot] != NULL);
                    ast_node_t* instance = (ast_node_t*)module_names_to_idx->data[sc_spot];

                    sc_hierarchy* function_sc_hierarchy = local_ref->function_children[i];

                    /* update the parameter table for the instantiated module */
                    STRING_CACHE* instance_param_table_sc = function_sc_hierarchy->local_param_table_sc;
                    update_instance_parameter_table_direct_instances(temp_instance, instance_param_table_sc);
                    update_instance_parameter_table_defparams(temp_instance, instance_param_table_sc);

                    /* elaboration */
                    update_string_caches(function_sc_hierarchy);
                    instance = finalize_ast(instance, NULL, function_sc_hierarchy, false, true);

                    /* clean up */
                    vtr::free(temp_instance_name);
                }

                for (i = 0; i < node->types.task.size_task_instantiations; i++) {
                    /* make the stringed up module instance name - instance name is
                     * MODULE_INSTANCE->MODULE_NAMED_INSTANCE(child[1])->IDENTIFIER(child[0]).
                     * module name is MODULE_INSTANCE->IDENTIFIER(child[0])
                     */
                    ast_node_t* temp_instance = node->types.task.task_instantiations_instance[i];
                    char* instance_name_prefix = local_ref->instance_name_prefix;

                    char* temp_instance_name = make_full_ref_name(instance_name_prefix,
                                                                  temp_instance->identifier_node->types.identifier,
                                                                  temp_instance->children[0]->identifier_node->types.identifier,
                                                                  NULL, -1);

                    /* lookup the name of the task associated with this instantiated point */
                    long sc_spot = sc_lookup_string(module_names_to_idx, temp_instance->identifier_node->types.identifier);
                    oassert(sc_spot > -1 && module_names_to_idx->data[sc_spot] != NULL);
                    ast_node_t* instance = (ast_node_t*)module_names_to_idx->data[sc_spot];

                    sc_hierarchy* task_sc_hierarchy = local_ref->task_children[i];

                    /* update the parameter table for the instantiated module */
                    STRING_CACHE* instance_param_table_sc = task_sc_hierarchy->local_param_table_sc;
                    update_instance_parameter_table_direct_instances(temp_instance, instance_param_table_sc);
                    update_instance_parameter_table_defparams(temp_instance, instance_param_table_sc);

                    /* elaboration */
                    update_string_caches(task_sc_hierarchy);
                    instance = finalize_ast(instance, NULL, task_sc_hierarchy, false, true);

                    /* clean up */
                    vtr::free(temp_instance_name);
                }

                break;
            }
            case MODULE_INSTANCE: {
                /* this should be encountered once generate constructs are resolved, so we don't have
                 * stray instances that never get instantiated */

                if (node->num_children == 1 && node->identifier_node) {
                    sc_hierarchy* this_ref = local_ref;
                    std::string new_id(node->children[0]->identifier_node->types.identifier);
                    while (this_ref->top_node->type != MODULE) {
                        std::string this_scope(this_ref->scope_id);
                        new_id = this_scope + "." + new_id;
                        this_ref = this_ref->parent;
                        oassert(this_ref);
                    }
                    ast_node_t* module = this_ref->top_node;
                    oassert(module->type == MODULE);

                    int num_instances = module->types.module.size_module_instantiations;
                    char* new_instance_name = vtr::strdup(new_id.c_str());
                    bool found = false;
                    for (int i = 0; i < num_instances; i++) {
                        ast_node_t* temp_instance_node = module->types.module.module_instantiations_instance[i];
                        char* temp_instance_name = temp_instance_node->children[0]->identifier_node->types.identifier;
                        if (strcmp(node->children[0]->identifier_node->types.identifier, temp_instance_name) == 0
                            || strcmp(new_instance_name, temp_instance_name) == 0) {
                            found = true;
                        }
                    }

                    if (found) {
                        vtr::free(new_instance_name);
                        break;
                    }

                    oassert(false); // module instances should be resolved at this point
                }
                break;
            }
            case VAR_DECLARE: {
                // pack 2d array into 1d
                if (node->num_children == 7
                    && node->children[4]
                    && node->children[5]) {
                    convert_2D_to_1D_array(&node);
                }

                break;
            }
            case BINARY_OPERATION: {
                if (node->children[0] == NULL || node->children[1] == NULL) {
                    /* resulting from replication of zero */
                    error_message(AST, node->loc,
                                  "%s", "Cannot perform operation with nonexistent value");
                }

                /* only fold if size can be self-determined */
                if (fold_expressions) {
                    ast_node_t* new_node = fold_binary(&node);
                    if (node_is_constant(new_node)) {
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
            case UNARY_OPERATION: {
                if (node->children[0] == NULL) {
                    /* resulting from replication of zero */
                    error_message(AST, node->loc,
                                  "%s", "Cannot perform operation with nonexistent value");
                }

                /* only fold if size can be self-determined */
                if (fold_expressions) {
                    ast_node_t* new_node = fold_unary(&node);
                    if (node_is_constant(new_node)) {
                        /* clean up */
                        free_resolved_children(node);

                        change_to_number_node(node, VNumber(*(new_node->types.vnumber), new_node->types.vnumber->size()));
                    }
                    new_node = free_whole_tree(new_node);
                }
                break;
            }
            case REPLICATE: {
                /* remove intermediate REPLICATE node */
                ast_node_t* child = node->children[0];
                node->children[0] = NULL;
                node = free_whole_tree(node);
                node = child;
                break;
            }
            case CONCATENATE: {
                if (node->num_children > 0) {
                    long current = 0;
                    long previous = -1;
                    bool previous_is_constant = false;

                    while (current < node->num_children) {
                        bool current_is_constant = node_is_constant(node->children[current]);

                        if (previous_is_constant && current_is_constant) {
                            resolve_concat_sizes(node->children[previous], local_ref);
                            resolve_concat_sizes(node->children[current], local_ref);

                            VNumber new_value = V_CONCAT({*(node->children[previous]->types.vnumber), *(node->children[current]->types.vnumber)});

                            node->children[current] = free_whole_tree(node->children[current]);

                            delete node->children[previous]->types.vnumber;
                            node->children[previous]->types.vnumber = new VNumber(new_value);
                        } else if (node->children[current] != NULL) {
                            previous += 1;
                            previous_is_constant = current_is_constant;
                            node->children[previous] = node->children[current];
                        }
                        current += 1;
                    }

                    node->num_children = previous + 1;

                    if (node->num_children == 0) {
                        // could we simply warn and continue ? any ways we can recover rather than fail?
                        /* resulting replication(s) of zero */
                        error_message(AST, node->loc,
                                      "%s", "Cannot concatenate zero bitstrings");
                    }

                    // node was all constant
                    if (node->num_children == 1) {
                        ast_node_t* tmp = node->children[0];
                        node->children[0] = NULL;
                        free_whole_tree(node);
                        node = tmp;
                    }
                }

                break;
            }
            case RANGE_REF: {
                bool is_constant_ref = node_is_constant(node->children[0]);
                if (is_constant_ref) {
                    is_constant_ref = is_constant_ref && !(node->children[0]->types.vnumber->has_unknown());
                }

                is_constant_ref = is_constant_ref && node_is_constant(node->children[1]);
                if (is_constant_ref) {
                    is_constant_ref = is_constant_ref && !(node->children[1]->types.vnumber->has_unknown());
                }

                if (!is_constant_ref) {
                    /* note: indexed part-selects (-: and +:) should support non-constant base expressions, 
                     * e.g. my_var[some_input+:3] = 4'b0101, but we don't support it right now... */

                    error_message(AST, node->loc,
                                  "%s", "Part-selects can only contain constant expressions");
                }

                break;
            }
            case ARRAY_REF: {
                bool is_constant_ref = node_is_constant(node->children[0]);
                if (is_constant_ref) {
                    is_constant_ref = is_constant_ref && !(node->children[0]->types.vnumber->has_unknown());
                }

                if (node->num_children == 2) {
                    is_constant_ref = is_constant_ref && node_is_constant(node->children[1]);
                    if (is_constant_ref) {
                        is_constant_ref = is_constant_ref && !(node->children[1]->types.vnumber->has_unknown());
                    }
                }

                if (!is_constant_ref) {
                    // this must be an implicit memory
                    long sc_spot = sc_lookup_string(local_symbol_table_sc, node->identifier_node->types.identifier);
                    oassert(sc_spot > -1);
                    ((ast_node_t*)local_symbol_table_sc->data[sc_spot])->types.variable.is_memory = true;
                }

                if (node->num_children == 2) convert_2D_to_1D_array_ref(&node, local_ref);
                break;
            }
            case STATEMENT: //fallthrough
            case ALWAYS:    // fallthrough
            case INITIAL: {
                is_generate_region = true;
                break;
            }
            default: {
                break;
            }
        }

        if (child_skip_list) {
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
ast_node_t* reduce_expressions(ast_node_t* node, sc_hierarchy* local_ref, long* max_size, long assignment_size) {
    short* child_skip_list = NULL; // list of children not to traverse into
    short skip_children = false;   // skips the DFS completely if true

    if (node) {
        oassert(node->type != NO_ID);

        STRING_CACHE* local_symbol_table_sc = local_ref->local_symbol_table_sc;

        if (node->num_children > 0) {
            child_skip_list = (short*)vtr::calloc(node->num_children, sizeof(short));
        }

        switch (node->type) {
            case MODULE: {
                break;
            }
            case FUNCTION: {
                skip_children = true;
                break;
            }
            case TASK: {
                skip_children = true;
                break;
            }
            case MODULE_INSTANCE: {
                skip_children = true;
                break;
            }
            case BLOCKING_STATEMENT:
            case NON_BLOCKING_STATEMENT: {
                /* try to resolve */
                if (node->children[1]->type != FUNCTION_INSTANCE) {
                    node->children[0] = reduce_expressions(node->children[0], local_ref, NULL, 0);

                    assignment_size = get_size_of_variable(node->children[0], local_ref);
                    max_size = (long*)calloc(1, sizeof(long));

                    if (node->children[1]->type != NUMBERS) {
                        node->children[1] = reduce_expressions(node->children[1], local_ref, max_size, assignment_size);
                        if (node->children[1] == NULL) {
                            /* resulting from replication of zero */
                            error_message(AST, node->loc,
                                          "%s", "Cannot perform assignment with nonexistent value");
                        }
                    } else {
                        VNumber* temp = node->children[1]->types.vnumber;
                        node->children[1]->types.vnumber = new VNumber(*temp, assignment_size);
                        delete temp;
                    }

                    vtr::free(max_size);

                    /* 
                     * cast to unsigned if necessary
                     * Concatenate results are unsigned, regardless of the operands. IEEE.1364-2005 pp.65
                     */
                    if (node->children[0]->type != CONCATENATE && node_is_constant(node->children[1])) {
                        char* id = NULL;
                        if (node->children[0]->type == IDENTIFIERS) {
                            id = node->children[0]->types.identifier;
                        } else {
                            id = node->children[0]->identifier_node->types.identifier;
                        }

                        long sc_spot = sc_lookup_string(local_symbol_table_sc, id);
                        if (sc_spot > -1) {
                            operation_list signedness = ((ast_node_t*)local_symbol_table_sc->data[sc_spot])->types.variable.signedness;
                            VNumber* old_value = node->children[1]->types.vnumber;
                            if (signedness == UNSIGNED) {
                                node->children[1]->types.vnumber = new VNumber(V_UNSIGNED(*old_value));
                            } else {
                                node->children[1]->types.vnumber = new VNumber(V_SIGNED(*old_value));
                            }
                            delete old_value;
                        }
                    } else {
                        /* signed keyword is not supported, meaning unresolved values will already be handled as
                         * unsigned at the netlist level... must update once signed support is added */
                    }

                    assignment_size = 0;
                    skip_children = true;
                }
                break;
            }
            case VAR_DECLARE_LIST: {
                skip_children = true;
                break;
            }
            case ARRAY_REF: // fallthrough
            case RANGE_REF: {
                /* resolve the identifier in case it's in a different scope */
                oassert(child_skip_list);
                break;
            }
            case IDENTIFIERS: {
                if (node->types.identifier) {
                    // resolve the source identifier
                    long sc_spot = sc_lookup_string(local_symbol_table_sc, node->types.identifier);
                    char* id = vtr::strdup(node->types.identifier);
                    ast_node_t* var_declare = resolve_hierarchical_name_reference(local_ref, id);
                    vtr::free(id);

                    if (var_declare) {
                        if (sc_spot == -1) {
                            // resolve the local identifier with it's source identifier
                            sc_spot = sc_add_string(local_symbol_table_sc, node->types.identifier);
                            local_symbol_table_sc->data[sc_spot] = (void*)ast_node_deep_copy(var_declare);
                        } else {
                            /* add the source identifier contents if the 
                             * local identifier has been already resolved */
                            if (var_declare->types.variable.initial_value)
                                node->types.variable.initial_value = new VNumber((*var_declare->types.variable.initial_value));

                            node->types.variable.signedness = var_declare->types.variable.signedness;
                            node->identifier_node = ast_node_deep_copy(var_declare->identifier_node);
                        }
                    } else {
                        /* error - symbol either doesn't exist or is not accessible to this scope */
                        error_message(AST, node->loc,
                                      "Missing declaration of this symbol %s\n", node->types.identifier);
                    }
                }
                break;
            }
            default: {
                break;
            }
        }

        if (child_skip_list && node->num_children > 0 && skip_children == false) {
            /* use while loop and boolean to prevent optimizations
             * since number of children may change during recursion */

            bool done = false;
            int i = 0;

            /* traverse all the children */
            while (done == false) {
                int num_original_children = node->num_children;
                if (child_skip_list[i] == false) {
                    /* recurse */
                    ast_node_t* old_child = node->children[i];
                    ast_node_t* new_child = reduce_expressions(old_child, local_ref, max_size, assignment_size);
                    node->children[i] = new_child;

                    if (old_child != new_child) {
                        /* recurse on this child again in case it can be further reduced */
                        i--;
                    }
                }

                if (num_original_children != node->num_children) {
                    child_skip_list = (short*)vtr::realloc(child_skip_list, sizeof(short) * node->num_children);
                    for (int j = num_original_children; j < node->num_children; j++) {
                        child_skip_list[j] = false;
                    }
                }

                i++;
                if (i >= node->num_children) {
                    done = true;
                }
            }
        }

        /* post-amble */
        switch (node->type) {
            case MODULE: {
                int i;

                /* sort of the same thing as we do in finalize_ast - call reduce_expressions on children */
                for (i = 0; i < node->types.module.size_module_instantiations; i++) {
                    /* make the stringed up module instance name - instance name is
                     * MODULE_INSTANCE->MODULE_NAMED_INSTANCE(child[0])->IDENTIFIER(identifier_node).
                     * module name is MODULE_INSTANCE->IDENTIFIER(identifier_node)
                     */
                    ast_node_t* temp_instance = node->types.module.module_instantiations_instance[i];
                    char* instance_name_prefix = local_ref->instance_name_prefix;

                    char* temp_instance_name = make_full_ref_name(instance_name_prefix,
                                                                  temp_instance->identifier_node->types.identifier,
                                                                  temp_instance->children[0]->identifier_node->types.identifier,
                                                                  NULL, -1);

                    long sc_spot = sc_lookup_string(module_names_to_idx, temp_instance_name);
                    /* lookup the name of the module associated with this instantiated point */
                    oassert(sc_spot != -1);
                    ast_node_t* instance = (ast_node_t*)module_names_to_idx->data[sc_spot];

                    /* elaboration */
                    sc_hierarchy* module_sc_hierarchy = instance->types.hierarchy;
                    instance = reduce_expressions(instance, module_sc_hierarchy, NULL, 0);

                    /* clean up */
                    vtr::free(temp_instance_name);
                }

                for (i = 0; i < node->types.function.size_function_instantiations; i++) {
                    /* make the stringed up module instance name - instance name is
                     * MODULE_INSTANCE->MODULE_NAMED_INSTANCE(child[0])->IDENTIFIER(identifier_node).
                     * module name is MODULE_INSTANCE->IDENTIFIER(identifier_node)
                     */
                    ast_node_t* temp_instance = node->types.function.function_instantiations_instance[i];
                    char* instance_name_prefix = local_ref->instance_name_prefix;

                    char* temp_instance_name = make_full_ref_name(instance_name_prefix,
                                                                  temp_instance->identifier_node->types.identifier,
                                                                  temp_instance->children[0]->identifier_node->types.identifier,
                                                                  NULL, -1);

                    long sc_spot = sc_lookup_string(module_names_to_idx, temp_instance_name);
                    /* lookup the name of the module associated with this instantiated point */
                    oassert(sc_spot != -1);
                    ast_node_t* instance = (ast_node_t*)module_names_to_idx->data[sc_spot];

                    /* elaboration */
                    sc_hierarchy* function_sc_hierarchy = local_ref->function_children[i];
                    instance = reduce_expressions(instance, function_sc_hierarchy, NULL, 0);

                    /* clean up */
                    vtr::free(temp_instance_name);
                }

                break;
            }
            case CONCATENATE: {
                resolve_concat_sizes(node, local_ref);
                break;
            }
            case BINARY_OPERATION: {
                if (node->children[0] == NULL || node->children[1] == NULL) {
                    /* resulting from replication of zero */
                    error_message(AST, node->loc,
                                  "%s", "Cannot perform operation with nonexistent value");
                }

                ast_node_t* new_node = fold_binary(&node);
                if (node_is_constant(new_node)) {
                    /* resize as needed */
                    long new_size;
                    long this_size = new_node->types.vnumber->size();

                    if (assignment_size > 0) {
                        new_size = assignment_size;
                    } else if (max_size) {
                        new_size = *max_size;
                    } else {
                        new_size = this_size;
                    }

                    /* clean up */
                    free_resolved_children(node);

                    change_to_number_node(node, VNumber(*(new_node->types.vnumber), new_size));
                }
                new_node = free_whole_tree(new_node);
                break;
            }
            case UNARY_OPERATION: {
                if (node->children[0] == NULL) {
                    /* resulting from replication of zero */
                    error_message(AST, node->loc,
                                  "%s", "Cannot perform operation with nonexistent value");
                }

                ast_node_t* new_node = fold_unary(&node);
                if (node_is_constant(new_node)) {
                    /* resize as needed */
                    long new_size;
                    long this_size = new_node->types.vnumber->size();

                    if (assignment_size > 0) {
                        new_size = assignment_size;
                    } else if (max_size) {
                        new_size = *max_size;
                    } else {
                        new_size = this_size;
                    }

                    /* clean up */
                    free_resolved_children(node);

                    change_to_number_node(node, VNumber(*(new_node->types.vnumber), new_size));
                }
                new_node = free_whole_tree(new_node);
                break;
            }
            case IDENTIFIERS: {
                // look up to resolve unresolved range refs
                ast_node_t* var_node = NULL;

                if (node->types.identifier) {
                    long sc_spot = sc_lookup_string(local_symbol_table_sc, node->types.identifier);
                    if (sc_spot > -1) {
                        var_node = (ast_node_t*)local_symbol_table_sc->data[sc_spot];

                        if (var_node->children[0] != NULL) {
                            oassert(node_is_constant(var_node->children[0]) && node_is_constant(var_node->children[1]));
                        }
                        if (var_node->children[2] != NULL) {
                            oassert(node_is_constant(var_node->children[2]) && node_is_constant(var_node->children[3]));
                        }
                        if (var_node->num_children == 7 && var_node->children[4]) {
                            oassert(node_is_constant(var_node->children[4]) && node_is_constant(var_node->children[5]));
                        }

                        local_symbol_table_sc->data[sc_spot] = (void*)var_node;
                    }

                    if (max_size) {
                        long var_size = get_size_of_variable(node, local_ref);
                        if (var_size > *max_size) {
                            *max_size = var_size;
                        }
                    }
                }

                break;
            }
            case NUMBERS: {
                if (max_size) {
                    long current_size = static_cast<long>(node->types.vnumber->size());
                    if (current_size > (*max_size)) {
                        *max_size = current_size;
                    }
                }

                break;
            }
            default: {
                break;
            }
        }

        if (child_skip_list) {
            child_skip_list = (short*)vtr::free(child_skip_list);
        }
    }

    return node;
}

void update_string_caches(sc_hierarchy* local_ref) {
    STRING_CACHE* local_param_table_sc = local_ref->local_param_table_sc;
    ast_node_t** local_symbol_table = local_ref->local_symbol_table;
    int num_local_symbol_table = local_ref->num_local_symbol_table;

    int i = 0;

    if (local_param_table_sc) {
        while (i < local_param_table_sc->size && local_param_table_sc->data[i] != NULL) {
            ast_node_t* param_val = (ast_node_t*)local_param_table_sc->data[i];
            if (param_val->type == VAR_DECLARE && !node_is_constant(param_val->children[4])) {
                ast_node_t* resolved_val = finalize_ast(param_val->children[4], NULL, local_ref, true, true);
                oassert(resolved_val);
                param_val->children[4] = NULL;
                free_whole_tree(param_val);
                param_val = resolved_val;
            } else {
                sc_hierarchy* this_ref = local_ref;
                while (this_ref && !node_is_constant(param_val)) {
                    param_val = finalize_ast(param_val, NULL, this_ref, true, true);
                    this_ref = this_ref->parent;
                }
            }
            oassert(node_is_constant(param_val));

            /* this forces parameter values as unsigned, since we don't currently support signed keyword...
             * must be changed once support is added */
            VNumber* temp = param_val->types.vnumber;
            VNumber* to_unsigned = new VNumber(V_UNSIGNED(*temp));
            param_val->types.vnumber = to_unsigned;
            delete temp;

            local_param_table_sc->data[i] = (void*)param_val;
            i++;
        }
    }

    for (i = 0; i < num_local_symbol_table; i++) {
        ast_node_t* var_declare = local_symbol_table[i];
        if ((var_declare->children[0] && !node_is_constant(var_declare->children[0]))
            || (var_declare->children[1] && !node_is_constant(var_declare->children[1]))
            || (var_declare->children[2] && !node_is_constant(var_declare->children[2]))
            || (var_declare->children[3] && !node_is_constant(var_declare->children[3]))
            || var_declare->num_children == 7) {
            local_symbol_table[i] = finalize_ast(var_declare, NULL, local_ref, true, true);
        }
    }
}

void update_instance_parameter_table_direct_instances(ast_node_t* instance, STRING_CACHE* instance_param_table_sc) {
    if (instance->children[0]->children[1] && instance->children[0]->children[1]->num_children > 0) {
        long sc_spot;
        ast_node_t* parameter_override_list = instance->children[0]->children[1];
        int param_count = 0;

        bool is_by_name = parameter_override_list->children[0]->types.variable.is_parameter
                          && parameter_override_list->children[0]->identifier_node != NULL;

        /* error checks for overrides passed directly into instance */
        for (int j = 0; j < parameter_override_list->num_children; j++) {
            if (parameter_override_list->children[j]->types.variable.is_parameter) {
                param_count++;
                if (is_by_name != (parameter_override_list->children[j]->identifier_node != NULL)) {
                    error_message(AST, parameter_override_list->loc,
                                  "%s", "Cannot mix parameters passed by name and parameters passed by ordered list\n");
                } else if (is_by_name) {
                    char* param_id = parameter_override_list->children[j]->identifier_node->types.identifier;

                    for (int k = 0; k < j; k++) {
                        if (!strcmp(param_id, parameter_override_list->children[k]->identifier_node->types.identifier)) {
                            error_message(AST, parameter_override_list->loc,
                                          "Cannot override the same parameter twice (%s) within a module instance\n",
                                          parameter_override_list->children[j]->identifier_node->types.identifier);
                        }
                    }

                    sc_spot = sc_lookup_string(instance_param_table_sc, param_id);
                    if (sc_spot == -1) {
                        error_message(AST, parameter_override_list->loc,
                                      "Can't find parameter name %s in module %s\n",
                                      param_id, instance->identifier_node->types.identifier);
                    }
                } else {
                    char* param_id = vtr::strdup(instance_param_table_sc->string[j]);
                    ast_node_t* param_symbol = newSymbolNode(param_id, parameter_override_list->children[j]->loc);
                    parameter_override_list->children[j]->identifier_node = param_symbol;
                }
            } else {
                oassert(parameter_override_list->children[j]->types.variable.is_defparam);
                break;
            }
        }

        if (!is_by_name && param_count > instance_param_table_sc->free) {
            error_message(AST, parameter_override_list->loc,
                          "There are more parameters (%d) passed into %s than there are specified in the module (%ld)!",
                          param_count, instance->children[1]->children[0]->types.identifier, instance_param_table_sc->free);
        }
    }
}

void update_instance_parameter_table_defparams(ast_node_t* instance, STRING_CACHE* instance_param_table_sc) {
    if (instance->children[0]->children[1] && instance->children[0]->children[1]->num_children > 0) {
        long sc_spot;
        ast_node_t* parameter_override_list = instance->children[0]->children[1];

        /* fill out the overrides */
        for (int j = (parameter_override_list->num_children - 1); j >= 0; j--) {
            char* param_id = parameter_override_list->children[j]->identifier_node->types.identifier;
            sc_spot = sc_lookup_string(instance_param_table_sc, param_id);
            if (sc_spot == -1) {
                warning_message(AST, parameter_override_list->loc,
                                "Can't find parameter name %s in module %s\n",
                                param_id, instance->identifier_node->types.identifier);
            } else {
                ast_node_t* param_node = (ast_node_t*)instance_param_table_sc->data[sc_spot];
                if (param_node == NULL) {
                    instance_param_table_sc->data[sc_spot] = (void*)parameter_override_list->children[j]->children[4];
                } else if (!param_node->types.variable.is_parameter) {
                    warning_message(AST, parameter_override_list->loc,
                                    "Defparam can only override parameters: %s\n",
                                    parameter_override_list->children[j]->identifier_node->types.identifier);
                } else {
                    /* update this parameter and remove all other overrides with this name */
                    free_whole_tree(((ast_node_t*)instance_param_table_sc->data[sc_spot]));
                    instance_param_table_sc->data[sc_spot] = (void*)parameter_override_list->children[j]->children[4];
                    parameter_override_list->children[j]->children[4] = NULL;
                }

                for (int k = 0; k < j; k++) {
                    if (!strcmp(parameter_override_list->children[k]->identifier_node->types.identifier, param_id)) {
                        remove_child_from_node_at_index(parameter_override_list, k);
                        j--;
                        k--;
                    }
                }
                remove_child_from_node_at_index(parameter_override_list, j);
            }
        }
        // check if there are still overrides in the list that haven't been deleted
        oassert(parameter_override_list->num_children == 0);
    }
}

void override_parameters_for_all_instances(ast_node_t* module, sc_hierarchy* local_ref) {
    ast_node_t** module_instantiations_instance = module->types.module.module_instantiations_instance;
    int size_module_instantiations = module->types.module.size_module_instantiations;

    for (int i = 0; i < size_module_instantiations; i++) {
        ast_node_t* instance;
        /* make the stringed up module instance name - instance name is
         * MODULE_INSTANCE->MODULE_NAMED_INSTANCE(child[0])->identifier_node.
         * module name is MODULE_INSTANCE->identifier_node
         */
        ast_node_t* temp_instance = module_instantiations_instance[i];
        char* instance_name_prefix = local_ref->instance_name_prefix;

        char* temp_instance_name = make_full_ref_name(instance_name_prefix,
                                                      temp_instance->identifier_node->types.identifier,
                                                      temp_instance->children[0]->identifier_node->types.identifier,
                                                      NULL, -1);

        long sc_spot = sc_lookup_string(module_names_to_idx, temp_instance_name);

        vtr::free(temp_instance_name);

        instance = (ast_node_t*)module_names_to_idx->data[sc_spot];

        sc_hierarchy* module_sc_hierarchy = local_ref->module_children[i];

        /* update the parameter table for the instantiated module */
        STRING_CACHE* instance_param_table_sc = module_sc_hierarchy->local_param_table_sc;
        update_instance_parameter_table_direct_instances(temp_instance, instance_param_table_sc);
        update_instance_parameter_table_defparams(temp_instance, instance_param_table_sc);
        /* go through this instance's children */
        override_parameters_for_all_instances(instance, module_sc_hierarchy);
    }
}

void convert_2D_to_1D_array(ast_node_t** var_declare) {
    ast_node_t* node_max2 = (*var_declare)->children[2];
    ast_node_t* node_min2 = (*var_declare)->children[3];

    ast_node_t* node_max3 = (*var_declare)->children[4];
    ast_node_t* node_min3 = (*var_declare)->children[5];

    oassert(node_min2->type == NUMBERS && node_max2->type == NUMBERS);
    oassert(node_min3->type == NUMBERS && node_max3->type == NUMBERS);

    long addr_min = node_min2->types.vnumber->get_value();
    long addr_max = node_max2->types.vnumber->get_value();

    long addr_min1 = node_min3->types.vnumber->get_value();
    long addr_max1 = node_max3->types.vnumber->get_value();

    if ((addr_min > addr_max) || (addr_min1 > addr_max1)) {
        error_message(AST, (*var_declare)->loc, "%s",
                      "Odin doesn't support arrays declared [m:n] where m is less than n.");
    } else if ((addr_min < 0 || addr_max < 0) || (addr_min1 < 0 || addr_max1 < 0)) {
        error_message(AST, (*var_declare)->loc, "%s",
                      "Odin doesn't support negative number in index.");
    }

    char* name = (*var_declare)->identifier_node->types.identifier;

    if (addr_min != 0 || addr_min1 != 0) {
        error_message(AST, (*var_declare)->loc,
                      "%s: right memory address index must be zero\n", name);
    }

    long addr_chunk_size = (addr_max1 - addr_min1 + 1);

    (*var_declare)->chunk_size = addr_chunk_size;

    long new_address_max = (addr_max - addr_min + 1) * addr_chunk_size - 1;

    change_to_number_node((*var_declare)->children[2], new_address_max);
    change_to_number_node((*var_declare)->children[3], 0);

    (*var_declare)->children[4] = free_whole_tree((*var_declare)->children[4]);
    (*var_declare)->children[5] = free_whole_tree((*var_declare)->children[5]);

    (*var_declare)->children[4] = (*var_declare)->children[6];
    (*var_declare)->children[6] = NULL;

    (*var_declare)->num_children -= 2;
}

void convert_2D_to_1D_array_ref(ast_node_t** node, sc_hierarchy* local_ref) {
    STRING_CACHE* local_symbol_table_sc = local_ref->local_symbol_table_sc;

    char* temp_string = make_full_ref_name(NULL, NULL, NULL, (*node)->identifier_node->types.identifier, -1);

    // look up chunk size
    long sc_spot = sc_lookup_string(local_symbol_table_sc, temp_string);
    oassert(sc_spot != -1);

    ast_node_t* array_size = ast_node_deep_copy((ast_node_t*)local_symbol_table_sc->data[sc_spot]);

    change_to_number_node(array_size, array_size->chunk_size);

    ast_node_t* array_row = (*node)->children[0];
    ast_node_t* array_col = (*node)->children[1];

    // build the new AST
    ast_node_t* new_node_1 = newBinaryOperation(MULTIPLY, array_row, array_size, (*node)->loc);
    ast_node_t* new_node_2 = newBinaryOperation(ADD, new_node_1, array_col, (*node)->loc);

    vtr::free(temp_string);

    (*node)->children[0] = new_node_2;
    (*node)->children[1] = NULL;
    (*node)->num_children -= 1;
}

bool verify_terminal(ast_node_t* top, ast_node_t* iterator) {
    if (top) {
        if (top->type == BINARY_OPERATION) {
            return verify_terminal(top->children[0], iterator) && verify_terminal(top->children[1], iterator);
        } else if (top->type == UNARY_OPERATION) {
            return verify_terminal(top->children[0], iterator);
        } else if (top->type == IDENTIFIERS) {
            return (strcmp(top->types.identifier, iterator->identifier_node->types.identifier) == 0);
        } else {
            return node_is_constant(top);
        }
    } else {
        return false;
    }
}

void verify_genvars(ast_node_t* node, sc_hierarchy* local_ref, char*** other_genvars, int num_genvars) {
    if (node) {
        if (node->type == FOR) {
            ast_node_t* initial = node->children[0];
            ast_node_t* terminal = node->children[2];
            ast_node_t* body = node->children[3];
            ast_node_t* iterator = initial->children[0];

            if (strcmp(initial->children[0]->types.identifier, terminal->children[0]->types.identifier) != 0) {
                /* must match */
                error_message(AST, node->loc,
                              "%s", "Must use the same genvar for initial condition and iteration condition");
            }

            ast_node_t* var_declare = resolve_hierarchical_name_reference(local_ref, iterator->types.identifier);

            if (var_declare == NULL) {
                error_message(AST, iterator->loc,
                              "Missing declaration of this symbol %s\n", iterator->types.identifier);
            } else if (!var_declare->types.variable.is_genvar) {
                error_message(AST, node->loc,
                              "%s", "Iterator for loop generate construct must be declared as a genvar");
            }

            // check genvars that were already found
            for (int i = 0; i < num_genvars; i++) {
                if (!strcmp(iterator->types.identifier, (*other_genvars)[i])) {
                    error_message(AST, node->loc,
                                  "%s", "Cannot reuse a genvar in a nested loop sequence");
                }
            }

            (*other_genvars) = (char**)vtr::realloc((*other_genvars), sizeof(char*) * (num_genvars + 1));
            (*other_genvars)[num_genvars] = iterator->types.identifier;
            num_genvars++;

            // look for nested loops to verify that each genvar is used in only one loop
            for (int i = 0; i < body->num_children; i++) {
                verify_genvars(body->children[i], local_ref, other_genvars, num_genvars);
            }
        } else {
            // look for nested loops to verify that each genvar is used in only one loop
            for (int i = 0; i < node->num_children; i++) {
                verify_genvars(node->children[i], local_ref, other_genvars, num_genvars);
            }
        }
    }
}

ast_node_t* look_for_matching_hard_block(ast_node_t* node, char* hard_block_name, sc_hierarchy* local_ref) {
    t_model* hb_model = find_hard_block(hard_block_name);
    bool is_hb = true;

    if (!hb_model) {
        /* check for soft logic RAM */
        return look_for_matching_soft_logic(node, hard_block_name);
    } else {
        t_model_ports* hb_input_ports = hb_model->inputs;
        t_model_ports* hb_output_ports = hb_model->outputs;

        int num_hb_inputs = 0;
        int num_hb_outputs = 0;

        while (hb_input_ports != NULL) {
            num_hb_inputs++;
            hb_input_ports = hb_input_ports->next;
        }

        while (hb_output_ports != NULL) {
            num_hb_outputs++;
            hb_output_ports = hb_output_ports->next;
        }

        ast_node_t* connect_list = node->children[0]->children[0];

        /* first check if the number of ports match up */
        if (connect_list->num_children != (num_hb_inputs + num_hb_outputs)) {
            is_hb = false;
        }

        if (is_hb && connect_list->children[0]->identifier_node != NULL) {
            /* number of ports match and ports were passed in by name;
             * if all port names match up, this is a hard block */

            for (int i = 0; i < connect_list->num_children && is_hb; i++) {
                oassert(connect_list->children[i]->identifier_node);
                char* id = connect_list->children[i]->identifier_node->types.identifier;
                hb_input_ports = hb_model->inputs;
                hb_output_ports = hb_model->outputs;

                while ((hb_input_ports != NULL) && (strcmp(hb_input_ports->name, id) != 0)) {
                    hb_input_ports = hb_input_ports->next;
                }

                if (hb_input_ports == NULL) {
                    while ((hb_output_ports != NULL) && (strcmp(hb_output_ports->name, id) != 0)) {
                        hb_output_ports = hb_output_ports->next;
                    }
                } else {
                    /* matching input was found; move on to the next port connection */
                    continue;
                }

                if (hb_output_ports == NULL) {
                    /* name doesn't match up with any of the defined ports; this is not a hard block */
                    is_hb = false;
                } else {
                    /* matching output was found; move on to the next port connection */
                    continue;
                }
            }
        } else if (is_hb) {
            /* number of ports match and ports were passed in by ordered list; 
             * this is risky, but we will try to do some "smart" mapping to mark inputs and outputs 
             * by evaluating the port connections to determine the order */

            warning_message(AST, connect_list->loc,
                            "Attempting to convert this instance to a hard block (%s) -	unnamed port connections will be matched according to hard block specification and may produce unexpected results\n", hard_block_name);

            t_model_ports *hb_ports_1 = NULL, *hb_ports_2 = NULL;
            bool is_input, is_output;
            int num_ports;

            hb_input_ports = hb_model->inputs;
            hb_output_ports = hb_model->outputs;

            /* decide whether to look for inputs or outputs based on what there are less of */
            if (num_hb_inputs <= num_hb_outputs) {
                is_input = true;
                is_output = false;
                num_ports = num_hb_inputs;
            } else {
                is_input = false;
                is_output = true;
                num_ports = num_hb_outputs;
            }

            STRING_CACHE* local_symbol_table_sc = local_ref->local_symbol_table_sc;

            int i;

            /* look through the first N (num_ports) port connections to look for a match */
            for (i = 0; i < num_ports; i++) {
                char* port_id = connect_list->children[i]->children[0]->types.identifier;
                long sc_spot = sc_lookup_string(local_symbol_table_sc, port_id);
                oassert(sc_spot > -1);
                ast_node_t* var_declare = (ast_node_t*)local_symbol_table_sc->data[sc_spot];

                if (var_declare->types.variable.is_output == is_output
                    && var_declare->types.variable.is_input == is_input) {
                    /* found a match! check if it's an input or output */
                    if (is_input) {
                        hb_ports_1 = hb_model->inputs;
                        hb_ports_2 = hb_model->outputs;
                    } else {
                        hb_ports_1 = hb_model->outputs;
                        hb_ports_2 = hb_model->inputs;
                    }
                    break;
                } else if (var_declare->types.variable.is_output != is_output
                           && var_declare->types.variable.is_input != is_input) {
                    /* found the opposite of what we were looking for */
                    if (is_input) {
                        hb_ports_1 = hb_model->outputs;
                        hb_ports_2 = hb_model->inputs;
                    } else {
                        hb_ports_1 = hb_model->inputs;
                        hb_ports_2 = hb_model->outputs;
                    }
                    break;
                }
            }

            /* if a match hasn't been found yet, look through the last N (num_ports) port connections */
            if (!(hb_ports_1 && hb_ports_2)) {
                for (i = connect_list->num_children - num_ports; i < connect_list->num_children; i++) {
                    char* port_id = connect_list->children[i]->children[1]->types.identifier;
                    long sc_spot = sc_lookup_string(local_symbol_table_sc, port_id);
                    oassert(sc_spot > -1);
                    ast_node_t* var_declare = (ast_node_t*)local_symbol_table_sc->data[sc_spot];

                    if (var_declare->types.variable.is_output == is_output
                        && var_declare->types.variable.is_input == is_input) {
                        /* found a match! since we're at the other end, inputs/outputs should be reversed */
                        if (is_input) {
                            hb_ports_1 = hb_model->outputs;
                            hb_ports_2 = hb_model->inputs;
                        } else {
                            hb_ports_1 = hb_model->inputs;
                            hb_ports_2 = hb_model->outputs;
                        }
                        break;
                    } else if (var_declare->types.variable.is_output != is_output
                               && var_declare->types.variable.is_input != is_input) {
                        /* found the opposite of what we were looking for */
                        if (is_input) {
                            hb_ports_1 = hb_model->inputs;
                            hb_ports_2 = hb_model->outputs;
                        } else {
                            hb_ports_1 = hb_model->outputs;
                            hb_ports_2 = hb_model->inputs;
                        }
                        break;
                    }
                }
            }

            /* if a match hasn't been found, then there is no way to tell what should be done first;
             * we will default to inputs first, then outputs after (this is an arbitrary decision) */
            if (!(hb_ports_1 && hb_ports_2)) {
                hb_ports_1 = hb_model->inputs;
                hb_ports_2 = hb_model->outputs;
            }

            /* attach new port identifiers for later reference when building the hard block */
            i = 0;
            while (hb_ports_1) {
                oassert(connect_list->children[i] && !connect_list->children[i]->identifier_node);
                connect_list->children[i]->identifier_node = newSymbolNode(vtr::strdup(hb_ports_1->name), connect_list->loc);
                hb_ports_1 = hb_ports_1->next;
                i++;
            }
            while (hb_ports_2) {
                oassert(connect_list->children[i] && !connect_list->children[i]->identifier_node);
                connect_list->children[i]->identifier_node = newSymbolNode(vtr::strdup(hb_ports_2->name), connect_list->loc);
                hb_ports_2 = hb_ports_2->next;
                i++;
            }
        }
    }

    if (is_hb) {
        ast_node_t* instance = ast_node_deep_copy(node->children[0]);
        char* new_hard_block_name = vtr::strdup(hard_block_name);

        return newHardBlockInstance(new_hard_block_name, instance, instance->loc);
    } else {
        return look_for_matching_soft_logic(node, hard_block_name);
    }
}

ast_node_t* look_for_matching_soft_logic(ast_node_t* node, char* hard_block_name) {
    bool is_hb = true;

    if (!is_ast_sp_ram(node) && !is_ast_dp_ram(node) && !is_ast_adder(node) && !is_ast_multiplier(node)) {
        is_hb = false;
    }

    if (is_hb) {
        ast_node_t* instance = ast_node_deep_copy(node->children[0]);
        char* new_hard_block_name = vtr::strdup(hard_block_name);
        return newHardBlockInstance(new_hard_block_name, instance, instance->loc);
    } else {
        return node;
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: create_param_table_for_scope)
 * 	Creates a lookup of the parameters declared here.
 *
 * (NOTE: this is intended for named or generate blocks only)
 *-------------------------------------------------------------------------------------------*/
void create_param_table_for_scope(ast_node_t* module_items, sc_hierarchy* local_ref) {
    /* with the top module we need to visit the entire ast tree */
    long i, j;
    char* temp_string;
    long sc_spot;
    oassert(module_items->type == BLOCK);

    STRING_CACHE* local_param_table_sc = local_ref->local_param_table_sc;
    STRING_CACHE* local_defparam_table_sc = local_ref->local_defparam_table_sc;

    /* search for VAR_DECLARE_LISTS */
    if (module_items->num_children > 0) {
        for (i = 0; i < module_items->num_children; i++) {
            if (module_items->children[i]->type == VAR_DECLARE_LIST) {
                /* go through the vars in this declare list */
                for (j = 0; j < module_items->children[i]->num_children; j++) {
                    ast_node_t* var_declare = module_items->children[i]->children[j];

                    /* symbols are already dealt with */
                    if (var_declare->types.variable.is_input
                        || var_declare->types.variable.is_output
                        || var_declare->types.variable.is_reg
                        || var_declare->types.variable.is_genvar
                        || var_declare->types.variable.is_wire
                        || var_declare->types.variable.is_defparam)

                        continue;

                    oassert(module_items->children[i]->children[j]->type == VAR_DECLARE);

                    oassert(var_declare->types.variable.is_parameter
                            || var_declare->types.variable.is_localparam
                            || var_declare->types.variable.is_defparam);

                    /* make the string to add to the string cache */
                    temp_string = make_full_ref_name(NULL, NULL, NULL, var_declare->identifier_node->types.identifier, -1);

                    if (var_declare->types.variable.is_parameter || var_declare->types.variable.is_localparam) {
                        /* look for that element */
                        sc_spot = sc_add_string(local_param_table_sc, temp_string);

                        if (local_param_table_sc->data[sc_spot] != NULL) {
                            error_message(AST, var_declare->loc,
                                          "Block already has parameter with this name (%s)\n", var_declare->identifier_node->types.identifier);
                        }

                        /* store the data which is an idx here */
                        local_param_table_sc->data[sc_spot] = (void*)var_declare;
                    } else {
                        /* look for that element */
                        sc_spot = sc_add_string(local_defparam_table_sc, temp_string);

                        if (local_defparam_table_sc->data[sc_spot] != NULL) {
                            error_message(AST, var_declare->loc,
                                          "Block already has defparam with this name (%s)\n", var_declare->identifier_node->types.identifier);
                        }

                        /* store the data which is an idx here */
                        local_defparam_table_sc->data[sc_spot] = (void*)var_declare;
                    }

                    vtr::free(temp_string);

                    module_items->children[i]->children[j] = NULL;
                    remove_child_from_node_at_index(module_items->children[i], j);
                    j--;
                }

                if (module_items->children[i]->num_children == 0) {
                    remove_child_from_node_at_index(module_items, i);
                    i--;
                }
            }
        }
    } else {
        error_message(AST, module_items->loc, "%s", "Empty module\n");
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
// 				construct_new_tree(tail, T,list_assign[j]->loc);
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
