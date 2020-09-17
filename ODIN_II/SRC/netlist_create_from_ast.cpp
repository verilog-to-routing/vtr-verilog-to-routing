/*
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
#include <string.h>
#include <math.h>
#include <string>

#include "odin_types.h"
#include "odin_globals.h"

#include "netlist_utils.h"
#include "odin_util.h"
#include "ast_util.h"
#include "netlist_create_from_ast.h"
#include "string_cache.h"
#include "netlist_visualizer.h"
#include "parse_making_ast.h"
#include "node_creation_library.h"
#include "multipliers.h"
#include "hard_blocks.h"
#include "memories.h"
#include "implicit_memory.h"
#include "adders.h"
#include "subtractions.h"
#include "ast_elaborate.h"
#include "vtr_util.h"
#include "vtr_memory.h"

/* NAMING CONVENTIONS
 * {previous_string}.instance_name
 * {previous_string}.instance_name^signal_name
 * {previous_string}.instance_name^signal_name~bit
 */

#define INSTANTIATE_DRIVERS 1
#define ALIAS_INPUTS 2
#define RECURSIVE_LIMIT 256

STRING_CACHE* output_nets_sc;
STRING_CACHE* input_nets_sc;

signal_list_t* local_clock_list;
short local_clock_found;
int local_clock_idx;

/* CONSTANT NET ELEMENTS */
char* one_string;
char* zero_string;
char* pad_string;

netlist_t* verilog_netlist;

circuit_type_e type_of_circuit;
edge_type_e circuit_edge;

/* PROTOTYPES */
void convert_ast_to_netlist_recursing_via_modules(ast_node_t** current_module, char* instance_name, sc_hierarchy* local_ref, int level);
signal_list_t* netlist_expand_ast_of_module(ast_node_t** node, char* instance_name_prefix, sc_hierarchy* local_ref, long assignment_size);

void create_all_driver_nets_in_this_scope(char* instance_name_prefix, sc_hierarchy* local_ref);

void create_top_driver_nets(ast_node_t* module, char* instance_name_prefix, sc_hierarchy* local_ref);
void create_top_output_nodes(ast_node_t* module, char* instance_name_prefix, sc_hierarchy* local_ref);
nnet_t* define_nets_with_driver(ast_node_t* var_declare, char* instance_name_prefix);
nnet_t* define_nodes_and_nets_with_driver(ast_node_t* var_declare, char* instance_name_prefix);
ast_node_t* resolve_top_module_parameters(ast_node_t* node, sc_hierarchy* top_sc_list);
ast_node_t* resolve_top_parameters_defined_by_parameters(ast_node_t* node, sc_hierarchy* top_sc_list, int count);

void connect_hard_block_and_alias(ast_node_t* hb_instance, char* instance_name_prefix, int outport_size, sc_hierarchy* local_ref);
void connect_module_instantiation_and_alias(short PASS, ast_node_t* module_instance, char* instance_name_prefix, sc_hierarchy* local_ref);
signal_list_t* connect_function_instantiation_and_alias(short PASS, ast_node_t* module_instance, char* instance_name_prefix, sc_hierarchy* local_ref);
signal_list_t* connect_task_instantiation_and_alias(short PASS, ast_node_t* task_instance, char* instance_name_prefix, sc_hierarchy* local_ref);
VNumber* get_init_value(ast_node_t* node);
void define_latchs_initial_value_inside_initial_statement(ast_node_t* initial_node, sc_hierarchy* local_ref);

signal_list_t* concatenate_signal_lists(signal_list_t** signal_lists, int num_signal_lists);

signal_list_t* create_gate(ast_node_t* gate, char* instance_name_prefix, sc_hierarchy* local_ref, long assignment_size);
signal_list_t* create_hard_block(ast_node_t* block, char* instance_name_prefix, sc_hierarchy* local_ref);
signal_list_t* create_pins(ast_node_t* var_declare, char* name, char* instance_name_prefix, sc_hierarchy* local_ref);
signal_list_t* create_output_pin(ast_node_t* var_declare, char* instance_name_prefix, sc_hierarchy* local_ref);
signal_list_t* assignment_alias(ast_node_t* assignment, char* instance_name_prefix, sc_hierarchy* local_ref, long assignment_size);
signal_list_t* create_operation_node(ast_node_t* op, signal_list_t** input_lists, int list_size, char* instance_name_prefix, long assignment_size);

void terminate_continuous_assignment(ast_node_t* node, signal_list_t* assignment, char* instance_name_prefix);
void terminate_registered_assignment(ast_node_t* always_node, signal_list_t* assignment, signal_list_t* potential_clocks, sc_hierarchy* local_ref);

signal_list_t* create_case(ast_node_t* case_ast, char* instance_name_prefix, sc_hierarchy* local_ref, long assignment_size);
void create_case_control_signals(ast_node_t* case_list_of_items, ast_node_t** compare_against, nnode_t* case_node, char* instance_name_prefix, sc_hierarchy* local_ref, long assignment_size);
signal_list_t* create_case_mux_statements(ast_node_t* case_list_of_items, nnode_t* case_node, char* instance_name_prefix, sc_hierarchy* local_ref, long assignment_size);
signal_list_t* create_if(ast_node_t* if_ast, char* instance_name_prefix, sc_hierarchy* local_ref, long assignment_size);
void create_if_control_signals(ast_node_t** if_expression, nnode_t* if_node, char* instance_name_prefix, sc_hierarchy* local_ref, long assignment_size);
signal_list_t* create_if_mux_statements(ast_node_t* if_ast, nnode_t* if_node, char* instance_name_prefix, sc_hierarchy* local_ref, long assignment_size);
signal_list_t* create_if_for_question(ast_node_t* if_ast, char* instance_name_prefix, sc_hierarchy* local_ref, long assignment_size);
signal_list_t* create_if_question_mux_expressions(ast_node_t* if_ast, nnode_t* if_node, char* instance_name_prefix, sc_hierarchy* local_ref, long assignment_size);
signal_list_t* create_mux_statements(signal_list_t** statement_lists, nnode_t* case_node, int num_statement_lists, char* instance_name_prefix, sc_hierarchy* local_ref);
signal_list_t* create_mux_expressions(signal_list_t** expression_lists, nnode_t* mux_node, int num_expression_lists, char* instance_name_prefix);

signal_list_t* evaluate_sensitivity_list(ast_node_t* delay_control, char* instance_name_prefix, sc_hierarchy* local_ref);

int alias_output_assign_pins_to_inputs(char_list_t* output_list, signal_list_t* input_list, ast_node_t* node);

int find_smallest_non_numerical(ast_node_t* node, signal_list_t** input_list, int num_input_lists);
void pad_with_zeros(ast_node_t* node, signal_list_t* list, int pad_size, char* instance_name_prefix);
signal_list_t* create_dual_port_ram_block(ast_node_t* block, char* instance_name_prefix, t_model* hb_model, sc_hierarchy* local_ref);
signal_list_t* create_single_port_ram_block(ast_node_t* block, char* instance_name_prefix, t_model* hb_model, sc_hierarchy* local_ref);
signal_list_t* create_soft_single_port_ram_block(ast_node_t* block, char* instance_name_prefix, sc_hierarchy* local_ref);
signal_list_t* create_soft_dual_port_ram_block(ast_node_t* block, char* instance_name_prefix, sc_hierarchy* local_ref);

void look_for_clocks(netlist_t* netlist);
void reorder_connections_from_name(ast_node_t* instance_node, ast_node_t* instanciated_instance, ids type);

/*---------------------------------------------------------------------------------------------
 * (function: create_netlist)
 *--------------------------------------------------------------------------*/
void create_netlist(ast_t* ast) {
    /* just build all the fundamental elements, and then hookup with port definitions...every net has
     * a named net as a variable instance...even modules...the only trick with modules will
     * be instance names.  There are a few implied nets as in Muxes for cases, signals
     * for flip-flops and memories */

    /* define string cache lists for modules */
    for (int i = 0; i < module_names_to_idx->free; i++) {
        sc_hierarchy* local_ref = init_sc_hierarchy();
        local_ref->local_symbol_table_sc = sc_new_string_cache();
        create_symbol_table_for_scope(((ast_node_t*)module_names_to_idx->data[i])->children[1], local_ref);

        ((ast_node_t*)module_names_to_idx->data[i])->types.hierarchy = local_ref;

        if (((ast_node_t*)module_names_to_idx->data[i])->type == MODULE) {
            local_ref->local_param_table_sc = ((ast_node_t*)module_names_to_idx->data[i])->types.scope->param_sc;
            local_ref->local_defparam_table_sc = ((ast_node_t*)module_names_to_idx->data[i])->types.scope->defparam_sc;
        }
    }

    /* we will find the top module */
    ast_node_t* top_module = find_top_module(ast);
    if (top_module) {
        top_module->types.hierarchy->top_node = top_module;

        /* Since the modules are in a tree, we will bottom up build the netlist.  Essentially,
         * we will go to the leafs of the module tree, build them upwards such that when we search for the nets,
         * we will find them and can hook them up at that point */

        /* PASS 1 - we make all the nets based on registers defined in modules */

        /* initialize the string caches that hold the aliasing of module nets and input pins */
        output_nets_sc = sc_new_string_cache();
        input_nets_sc = sc_new_string_cache();
        /* initialize the storage of the top level drivers.  Assigned in create_top_driver_nets */
        verilog_netlist = allocate_netlist();

        sc_hierarchy* top_sc_list = top_module->types.hierarchy;
        oassert(top_sc_list);
        top_sc_list->instance_name_prefix = vtr::strdup(top_module->identifier_node->types.identifier);
        top_sc_list->scope_id = vtr::strdup(top_module->identifier_node->types.identifier);
        verilog_netlist->identifier = vtr::strdup(top_module->identifier_node->types.identifier);
        /* elaboration */
        resolve_top_module_parameters(top_module, top_sc_list);
        simplify_ast_module(&top_module, top_sc_list);

        /* now recursively parse the modules by going through the tree of modules starting at top */
        create_top_driver_nets(top_module, top_sc_list->instance_name_prefix, top_sc_list);
        init_implicit_memory_index();
        convert_ast_to_netlist_recursing_via_modules(&top_module, top_sc_list->instance_name_prefix, top_sc_list, 0);
        free_implicit_memory_index_and_finalize_memories();
        create_top_output_nodes(top_module, top_sc_list->instance_name_prefix, top_sc_list);

        /* clean up string caches */
        free_sc_hierarchy(top_sc_list);

        /* now look for high-level signals */
        look_for_clocks(verilog_netlist);
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: look_for_clocks)
 *-------------------------------------------------------------------------------------------*/
void look_for_clocks(netlist_t* netlist) {
    int i;

    for (i = 0; i < netlist->num_ff_nodes; i++) {
        oassert(netlist->ff_nodes[i]->input_pins[1]->net->num_driver_pins == 1);
        if (netlist->ff_nodes[i]->input_pins[1]->net->driver_pins[0]->node->type != CLOCK_NODE) {
            netlist->ff_nodes[i]->input_pins[1]->net->driver_pins[0]->node->type = CLOCK_NODE;
        }
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: convert_ast_to_netlist_recursing_via_modules)
 * 	Recurses through modules by depth first traversal of the tree of modules.  Expands
 * 	the netlists at each level.
 *-------------------------------------------------------------------------------------------*/
void convert_ast_to_netlist_recursing_via_modules(ast_node_t** current_module, char* instance_name, sc_hierarchy* local_ref, int level) {
    signal_list_t* list = NULL;

    /* BASE CASE is when there are no other instantiations of modules in this module */
    if ((*current_module)->types.module.size_module_instantiations == 0 && (*current_module)->types.function.size_function_instantiations == 0 && (*current_module)->types.task.size_task_instantiations == 0) {
        list = netlist_expand_ast_of_module(current_module, instance_name, local_ref, 0);
    } else {
        /* ELSE - we need to visit all the children before */
        int k;

        for (k = 0; k < (*current_module)->types.module.size_module_instantiations; k++) {
            /* make the stringed up module instance name - instance name is
             * MODULE_INSTANCE->MODULE_NAMED_INSTANCE(child[1])->IDENTIFIER(child[0]).
             * module name is MODULE_INSTANCE->IDENTIFIER(child[0])
             */

            char* temp_instance_name = make_full_ref_name(instance_name,
                                                          (*current_module)->types.module.module_instantiations_instance[k]->identifier_node->types.identifier,
                                                          (*current_module)->types.module.module_instantiations_instance[k]->children[0]->identifier_node->types.identifier,
                                                          NULL, -1);

            long sc_spot;
            /* lookup the name of the module associated with this instantiated point */
            if ((sc_spot = sc_lookup_string(module_names_to_idx, temp_instance_name)) == -1) {
                error_message(NETLIST, (*current_module)->loc,
                              "Can't find instance name %s\n", temp_instance_name);
            }

            ast_node_t* instance = (ast_node_t*)module_names_to_idx->data[sc_spot];

            sc_hierarchy* module_sc_hierarchy = instance->types.hierarchy;
            ;
            oassert(!strcmp(module_sc_hierarchy->instance_name_prefix, temp_instance_name));

            /* recursive call point */
            convert_ast_to_netlist_recursing_via_modules(&instance, temp_instance_name, module_sc_hierarchy, level + 1);

            /* clean up */
            vtr::free(temp_instance_name);
        }

        for (k = 0; k < (*current_module)->types.function.size_function_instantiations; k++) {
            /* make the stringed up module instance name - instance name is
             * MODULE_INSTANCE->MODULE_NAMED_INSTANCE(child[1])->IDENTIFIER(child[0]).
             * module name is MODULE_INSTANCE->IDENTIFIER(child[0])
             */
            char* temp_instance_name = make_full_ref_name(instance_name,
                                                          (*current_module)->types.function.function_instantiations_instance[k]->identifier_node->types.identifier,
                                                          (*current_module)->types.function.function_instantiations_instance[k]->children[0]->identifier_node->types.identifier,
                                                          NULL, -1);

            long sc_spot;
            /* lookup the name of the module associated with this instantiated point */
            if ((sc_spot = sc_lookup_string(module_names_to_idx, temp_instance_name)) == -1) {
                error_message(NETLIST, (*current_module)->loc,
                              "Can't find instance name %s\n", temp_instance_name);
            }

            sc_hierarchy* function_sc_hierarchy = local_ref->function_children[k];
            oassert(!strcmp(function_sc_hierarchy->instance_name_prefix, temp_instance_name));

            /* recursive call point */
            convert_ast_to_netlist_recursing_via_modules(((ast_node_t**)&module_names_to_idx->data[sc_spot]), temp_instance_name, function_sc_hierarchy, level + 1);

            /* clean up */
            vtr::free(temp_instance_name);
        }

        for (k = 0; k < (*current_module)->types.task.size_task_instantiations; k++) {
            /* make the stringed up task instance name - instance name is
             * MODULE_INSTANCE->MODULE_NAMED_INSTANCE(child[1])->IDENTIFIER(child[0]).
             * module name is MODULE_INSTANCE->IDENTIFIER(child[0])
             */

            char* temp_instance_name = make_full_ref_name(instance_name,
                                                          (*current_module)->types.task.task_instantiations_instance[k]->identifier_node->types.identifier,
                                                          (*current_module)->types.task.task_instantiations_instance[k]->children[0]->identifier_node->types.identifier,
                                                          NULL, -1);

            long sc_spot;
            /* lookup the name of the module associated with this instantiated point */
            if ((sc_spot = sc_lookup_string(module_names_to_idx, temp_instance_name)) == -1) {
                error_message(NETLIST, (*current_module)->loc,
                              "Can't find instance name %s\n", temp_instance_name);
            }

            sc_hierarchy* task_sc_hierarchy = local_ref->task_children[k];
            oassert(!strcmp(task_sc_hierarchy->instance_name_prefix, temp_instance_name));

            /* recursive call point */
            convert_ast_to_netlist_recursing_via_modules(((ast_node_t**)&module_names_to_idx->data[sc_spot]), temp_instance_name, task_sc_hierarchy, level + 1);

            /* clean up */
            vtr::free(temp_instance_name);
        }

        /* once we've done everyone lower, we can do this module */
        list = netlist_expand_ast_of_module(current_module, instance_name, local_ref, 0);
    }

    if (list) free_signal_list(list);
}

/*---------------------------------------------------------------------------
 * (function: netlist_expand_ast_of_module)
 * 	Main recursion point that looks at the abstract syntax tree.
 * 	Allows for a pre amble, looks at children (dfs), then does post amble
 * 	Can skip children traverse and the ambles...
 *-------------------------------------------------------------------------*/
signal_list_t* netlist_expand_ast_of_module(ast_node_t** node_ref, char* instance_name_prefix, sc_hierarchy* local_ref, long assignment_size) {
    /* with the top module we need to visit the entire ast tree */
    long i;
    short* child_skip_list = NULL; // list of children not to traverse into
    short skip_children = false;   // skips the DFS completely if true
    signal_list_t* return_sig_list = NULL;
    signal_list_t** children_signal_list = NULL;

    ast_node_t* node = *node_ref;

    if (node == NULL) {
        /* print out the node and label details */
    } else {
        /* make a skip list of nodes in the tree we don't need to visit for this node.
         * For example, a module we know that the 0th entry is the identifier of the module */
        if (node->num_children > 0) {
            child_skip_list = (short*)vtr::calloc(node->num_children, sizeof(short));
            children_signal_list = (signal_list_t**)vtr::calloc(node->num_children, sizeof(signal_list_t*));
        }

        /* ------------------------------------------------------------------------------*/
        /* PREAMBLE */
        switch (node->type) {
            case FILE_ITEMS:
                oassert(false);
                break;
            case MODULE:
            case FUNCTION:
            case TASK:
                /* set the skip list */
                if (node->num_children >= 1) {
                    oassert(child_skip_list);
                    child_skip_list[0] = true; /* skip portlist ... we'll use where they're defined */
                }
                break;

            case MODULE_ITEMS:
                /* items include: wire, reg, input, outputs, assign, gate, module_instance, always */

                local_clock_found = false;

                /* check for initial register values set in initial block.*/
                for (i = 0; i < node->num_children; i++) {
                    if (node->children[i] && node->children[i]->type == INITIAL) {
                        define_latchs_initial_value_inside_initial_statement(node->children[i]->children[0], local_ref);
                    }
                }

                /* create all the driven nets based on the "reg" registers */
                create_all_driver_nets_in_this_scope(instance_name_prefix, local_ref);

                /* process elements in the list */
                if (node->num_children > 0) {
                    oassert(child_skip_list);
                    for (i = 0; i < node->num_children; i++) {
                        if (!node->children[i]) {
                            child_skip_list[i] = true;
                        } else if (node->children[i]->type == VAR_DECLARE_LIST) {
                            child_skip_list[i] = true;
                        } else if (node->children[i]->type == MODULE_INSTANCE) {
                            /* ELSE IF - we deal with instantiations of modules twice to alias input and output nets.  In this
                             * pass we are looking for any drivers emerging from a module */
                            long j;
                            for (j = 0; j < node->children[i]->num_children; j++) {
                                /* make the aliases for all the drivers as they're passed through modules */
                                connect_module_instantiation_and_alias(INSTANTIATE_DRIVERS, node->children[i]->children[j], instance_name_prefix, local_ref);
                            }

                            /* is a call site for another module.  Alias names to nets and pins */
                            child_skip_list[i] = true;
                        } else if (node->children[i]->type == FUNCTION) {
                            child_skip_list[i] = true;
                        } else if (node->children[i]->type == TASK) {
                            child_skip_list[i] = true;
                        }
                    }
                }
                break;
            case BLOCK:
                if (node->num_children > 0) {
                    oassert(child_skip_list);
                    for (i = 0; i < node->num_children; i++) {
                        if (node->children[i]->type == MODULE_INSTANCE) {
                            /* we deal with instantiations of modules twice to alias input and output nets.  In this
                             * pass we are looking for any drivers emerging from a module */
                            long j;
                            for (j = 0; j < node->children[i]->num_children; j++) {
                                /* make the aliases for all the drivers as they're passed through modules */
                                connect_module_instantiation_and_alias(INSTANTIATE_DRIVERS, node->children[i]->children[j], instance_name_prefix, local_ref);
                            }

                            /* is a call site for another module.  Alias names to nets and pins */
                            child_skip_list[i] = true;
                        }
                    }
                }
                break;
            case FUNCTION_ITEMS:
                /* items include: wire, reg, input, outputs, assign, gate, always */

                local_clock_found = false;

                /* create all the driven nets based on the "reg" registers */
                create_all_driver_nets_in_this_scope(instance_name_prefix, local_ref);

                /* process elements in the list */
                if (node->num_children > 0) {
                    oassert(child_skip_list);
                    for (i = 0; i < node->num_children; i++) {
                        if (node->children[i]->type == VAR_DECLARE_LIST) {
                            /* IF - The port lists of this module are handled else where */
                            child_skip_list[i] = true;
                        }
                    }
                }
                break;
            case TASK_ITEMS:
                /* items include: wire, reg, input, outputs, assign, gate, always */

                local_clock_found = false;

                /* create all the driven nets based on the "reg" registers */
                create_all_driver_nets_in_this_scope(instance_name_prefix, local_ref);

                /* process elements in the list */
                if (node->num_children > 0) {
                    oassert(child_skip_list);
                    for (i = 0; i < node->num_children; i++) {
                        if (node->children[i]->type == VAR_DECLARE_LIST) {
                            /* IF - The port lists of this task are handled else where */
                            child_skip_list[i] = true;
                        } else if (node->children[i]->type == FUNCTION) {
                            child_skip_list[i] = true;
                        } else if (node->children[i]->type == TASK) {
                            child_skip_list[i] = true;
                        }
                    }
                }
                break;

            case INITIAL:
                /* define initial value of latchs */
                //define_latchs_initial_value_inside_initial_statement(node->children[0], instance_name_prefix);
                skip_children = true;
                break;
            case FUNCTION_INSTANCE:
                return_sig_list = connect_function_instantiation_and_alias(INSTANTIATE_DRIVERS, node, instance_name_prefix, local_ref);
                skip_children = true;
                break;
            case TASK_INSTANCE:
                return_sig_list = connect_task_instantiation_and_alias(INSTANTIATE_DRIVERS, node, instance_name_prefix, local_ref);
                skip_children = true;
                break;
            case GATE:
                /* create gate instances */
                return_sig_list = create_gate(node, instance_name_prefix, local_ref, assignment_size);
                /* create_gate does it's own instantiations so skip the children in the traverse */
                skip_children = true;
                break;
            /* ---------------------- */
            /* All these are input references that we need to grab their pins from by create_pin */
            case ARRAY_REF: // fallthrough
            {
                // skip_children = true;
            }
            case IDENTIFIERS:
            case RANGE_REF:
            case NUMBERS: {
                return_sig_list = create_pins(node, NULL, instance_name_prefix, local_ref);
                break;
            }
            /* ---------------------- */
            case ASSIGN:
                /* combinational path */
                type_of_circuit = COMBINATIONAL;
                circuit_edge = UNDEFINED_SENSITIVITY;
                break;
            case BLOCKING_STATEMENT:
            case NON_BLOCKING_STATEMENT: {
                assignment_size = get_size_of_variable(node->children[0], local_ref);
                return_sig_list = assignment_alias(node, instance_name_prefix, local_ref, assignment_size);
                skip_children = true;
                break;
            }
            case ALWAYS:
                if (node->num_children >= 1) {
                    oassert(child_skip_list);
                    child_skip_list[0] = true;
                    /* evaluate if this is a sensitivity list with posedges/negedges (=SEQUENTIAL) or none (=COMBINATIONAL) */
                    local_clock_list = evaluate_sensitivity_list(node->children[0], instance_name_prefix, local_ref);
                }
                break;
            case CASE:
                return_sig_list = create_case(node, instance_name_prefix, local_ref, assignment_size);
                skip_children = true;
                break;
            case DISPLAY:
                c_display(node);
                skip_children = true;
                break;
            case FINISH:
                c_finish(node);
                skip_children = true;
                break;
            case IF:
                return_sig_list = create_if(node, instance_name_prefix, local_ref, assignment_size);
                skip_children = true;
                break;
            case TERNARY_OPERATION:
                return_sig_list = create_if_for_question(node, instance_name_prefix, local_ref, assignment_size);
                skip_children = true;
                break;
            case HARD_BLOCK:
                /* set the skip list */
                if (node->num_children >= 1) {
                    oassert(child_skip_list);
                    child_skip_list[0] = true; /* skip portlist ... we'll use where they're defined */
                }
                return_sig_list = create_hard_block(node, instance_name_prefix, local_ref);
                break;
            default:
                break;
        }
        /* ------------------------------------------------------------------------------*/
        /* This is the depth first traverse (DFT aka DFS) of the ast nodes that make up the netlist. */
        if (skip_children == false) {
            /* traverse all the children */
            for (i = 0; i < node->num_children; i++) {
                if (child_skip_list && child_skip_list[i] == false) {
                    oassert(node->children[i]);
                    sc_hierarchy* child_ref = local_ref;
                    if (node->children[i]->types.hierarchy != NULL) {
                        child_ref = node->children[i]->types.hierarchy;
                    }

                    /* recursively call through the tree going to each instance.  This is depth first traverse. */
                    children_signal_list[i] = netlist_expand_ast_of_module(&(node->children[i]), instance_name_prefix, child_ref, assignment_size);
                }
            }
        }

        /* ------------------------------------------------------------------------------*/
        /* POST AMBLE - process the children */
        switch (node->type) {
            case FILE_ITEMS:
                error_message(NETLIST, node->loc, "%s",
                              "FILE_ITEMS are not supported by Odin.\n");
                break;
            case CONCATENATE:
                return_sig_list = concatenate_signal_lists(children_signal_list, node->num_children);
                break;
            case MODULE_ITEMS: {
                if (node->num_children > 0) {
                    for (i = 0; i < node->num_children; i++) {
                        if (node->children[i] && node->children[i]->type == MODULE_INSTANCE) {
                            long j;
                            for (j = 0; j < node->children[i]->num_children; j++) {
                                /* make the aliases for all the drivers as they're passed through modules */
                                connect_module_instantiation_and_alias(ALIAS_INPUTS, node->children[i]->children[j], instance_name_prefix, local_ref);
                            }
                        }
                    }
                }
                break;
            }
            case FUNCTION_INSTANCE: {
                signal_list_t* temp_list = connect_function_instantiation_and_alias(ALIAS_INPUTS, node, instance_name_prefix, local_ref);
                free_signal_list(temp_list); // list is unused; discard
                break;
            }
            case TASK_INSTANCE: {
                signal_list_t* temp_list = connect_task_instantiation_and_alias(ALIAS_INPUTS, node, instance_name_prefix, local_ref);
                free_signal_list(temp_list); // list is unused; discard
                break;
            }
            case ASSIGN:
                //oassert(node->num_children == 1);
                /* attach the drivers to the driver nets */
                for (i = 0; i < node->num_children; i++) {
                    terminate_continuous_assignment(node, children_signal_list[i], instance_name_prefix);
                }
                break;
            case STATEMENT:
                terminate_continuous_assignment(node, children_signal_list[0], instance_name_prefix);
                break;
            case ALWAYS:
                /* attach the drivers to the driver nets */
                switch (circuit_edge) {
                    case FALLING_EDGE_SENSITIVITY: //fallthrough
                    case RISING_EDGE_SENSITIVITY: {
                        terminate_registered_assignment(node, children_signal_list[1], local_clock_list, local_ref);
                        break;
                    }
                    case ASYNCHRONOUS_SENSITIVITY: {
                        /* idx 1 element since always has DELAY Control first */
                        terminate_continuous_assignment(node, children_signal_list[1], instance_name_prefix);
                        break;
                    }
                    default: {
                        oassert(false && "Assignment outside of always block.");
                        break;
                    }
                }

                if (local_clock_list)
                    free_signal_list(local_clock_list);

                break;
            case BINARY_OPERATION:
                oassert(node->num_children == 2);
                return_sig_list = create_operation_node(node, children_signal_list, node->num_children, instance_name_prefix, assignment_size);
                break;
            case UNARY_OPERATION:
                oassert(node->num_children == 1);
                return_sig_list = create_operation_node(node, children_signal_list, node->num_children, instance_name_prefix, assignment_size);
                break;
            case BLOCK:
                if (node->num_children > 0) {
                    for (i = 0; i < node->num_children; i++) {
                        if (node->children[i]->type == MODULE_INSTANCE) {
                            long j;
                            for (j = 0; j < node->children[i]->num_children; j++) {
                                /* make the aliases for all the drivers as they're passed through modules */
                                connect_module_instantiation_and_alias(ALIAS_INPUTS, node->children[i]->children[j], instance_name_prefix, local_ref);
                            }
                        }
                    }
                }
                return_sig_list = combine_lists(children_signal_list, node->num_children);
                break;
            case RAM:
                connect_memory_and_alias(node, instance_name_prefix, local_ref);
                break;
            case HARD_BLOCK:
                connect_hard_block_and_alias(node, instance_name_prefix, return_sig_list->count, local_ref);
                break;
            case IF:
            default:
                break;
        }
        /* ------------------------------------------------------------------------------*/
    }

    /* cleaning */
    if (child_skip_list != NULL) {
        vtr::free(child_skip_list);
    }
    if (children_signal_list != NULL) {
        for (i = 0; i < node->num_children; i++) {
            /* skip lists that have already been freed */
            if (children_signal_list[i]
                && node->type != BINARY_OPERATION
                && node->type != UNARY_OPERATION
                && node->type != BLOCK
                && node->type != VAR_DECLARE_LIST
                && node->type != ASSIGN
                && node->type != ALWAYS
                && node->type != STATEMENT
                && node->type != CONCATENATE) {
                free_signal_list(children_signal_list[i]);
            }
        }
        vtr::free(children_signal_list);
    }

    *node_ref = node;
    return return_sig_list;
}

/*
 * Concatenates the given signal lists together.
 *
 * Ignores the carry out from adders to maintain width expectations.
 */
signal_list_t* concatenate_signal_lists(signal_list_t** signal_lists, int num_signal_lists) {
    signal_list_t* return_list = init_signal_list();

    int i;
    for (i = num_signal_lists - 1; i >= 0; i--) {
        // When concatenating, ignore the carry out from adders so that they occupy the expected width.
        if (signal_lists[i]->is_adder)
            signal_lists[i]->count--;

        int j;
        for (j = 0; j < signal_lists[i]->count; j++) {
            npin_t* pin = signal_lists[i]->pins[j];
            add_pin_to_signal_list(return_list, pin);
        }
        free_signal_list(signal_lists[i]);
    }

    return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function: create_all_driver_nets_in_this_scope)
 * 	Use to define all the nets that will connect to instantiated nodes.  This means we go
 * 	through the var_declare_list of module variables and all registers are made
 * 	as drivers.
 *-------------------------------------------------------------------------------------------*/
void create_all_driver_nets_in_this_scope(char* instance_name_prefix, sc_hierarchy* local_ref) {
    /* with the top module we need to visit the entire ast tree */
    int i;

    ast_node_t** local_symbol_table = local_ref->local_symbol_table;
    int num_local_symbol_table = local_ref->num_local_symbol_table;

    /* use the symbol table to decide if driver */
    for (i = 0; i < num_local_symbol_table; i++) {
        oassert(local_symbol_table[i]->type == VAR_DECLARE || local_symbol_table[i]->type == BLOCKING_STATEMENT);
        if (
            /* all registers are drivers */
            (local_symbol_table[i]->types.variable.is_reg)
            /* a wire that is an input can be a driver */
            || ((local_symbol_table[i]->types.variable.is_wire)
                && (!local_symbol_table[i]->types.variable.is_input))
            /* any output that has nothing else is an implied wire driver */
            || ((local_symbol_table[i]->types.variable.is_output)
                && (!local_symbol_table[i]->types.variable.is_reg)
                && (!local_symbol_table[i]->types.variable.is_wire))) {
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
void create_top_driver_nets(ast_node_t* module, char* instance_name_prefix, sc_hierarchy* local_ref) {
    /* with the top module we need to visit the entire ast tree */
    long i;
    long sc_spot;
    ast_node_t** local_symbol_table = local_ref->local_symbol_table;
    long num_local_symbol_table = local_ref->num_local_symbol_table;
    ast_node_t* module_items = module->children[1];
    npin_t* new_pin;

    oassert(module_items->type == MODULE_ITEMS);

    /* search the symbol table for inputs to make drivers */
    if (local_symbol_table && num_local_symbol_table > 0) {
        for (i = 0; i < num_local_symbol_table; i++) {
            if (local_symbol_table[i]->types.variable.is_input) {
                define_nodes_and_nets_with_driver(local_symbol_table[i], instance_name_prefix);
            }
        }
    } else {
        error_message(NETLIST, module_items->loc, "%s", "Module is empty\n");
    }

    /* create the constant nets */
    verilog_netlist->zero_net = allocate_nnet();
    verilog_netlist->gnd_node = allocate_nnode(module->loc);
    verilog_netlist->gnd_node->type = GND_NODE;
    allocate_more_output_pins(verilog_netlist->gnd_node, 1);
    add_output_port_information(verilog_netlist->gnd_node, 1);
    new_pin = allocate_npin();
    add_output_pin_to_node(verilog_netlist->gnd_node, new_pin, 0);
    add_driver_pin_to_net(verilog_netlist->zero_net, new_pin);

    verilog_netlist->one_net = allocate_nnet();
    verilog_netlist->vcc_node = allocate_nnode(module->loc);
    verilog_netlist->vcc_node->type = VCC_NODE;
    allocate_more_output_pins(verilog_netlist->vcc_node, 1);
    add_output_port_information(verilog_netlist->vcc_node, 1);
    new_pin = allocate_npin();
    add_output_pin_to_node(verilog_netlist->vcc_node, new_pin, 0);
    add_driver_pin_to_net(verilog_netlist->one_net, new_pin);

    verilog_netlist->pad_net = allocate_nnet();
    verilog_netlist->pad_node = allocate_nnode(module->loc);
    verilog_netlist->pad_node->type = PAD_NODE;
    allocate_more_output_pins(verilog_netlist->pad_node, 1);
    add_output_port_information(verilog_netlist->pad_node, 1);
    new_pin = allocate_npin();
    add_output_pin_to_node(verilog_netlist->pad_node, new_pin, 0);
    add_driver_pin_to_net(verilog_netlist->pad_net, new_pin);

    /* CREATE the driver for the ZERO */
    verilog_netlist->gnd_node->name = make_full_ref_name(instance_name_prefix, NULL, NULL, zero_string, -1);
    vtr::free(zero_string);
    zero_string = verilog_netlist->gnd_node->name;

    sc_spot = sc_add_string(output_nets_sc, zero_string);
    if (output_nets_sc->data[sc_spot] != NULL) {
        error_message(NETLIST, module_items->loc, "%s", "Error in Odin\n");
    }
    /* store the data which is an idx here */
    output_nets_sc->data[sc_spot] = (void*)verilog_netlist->zero_net;
    verilog_netlist->zero_net->name = zero_string;

    /* CREATE the driver for the ONE and store twice */
    verilog_netlist->vcc_node->name = make_full_ref_name(instance_name_prefix, NULL, NULL, one_string, -1);
    vtr::free(one_string);
    one_string = verilog_netlist->vcc_node->name;

    sc_spot = sc_add_string(output_nets_sc, one_string);
    if (output_nets_sc->data[sc_spot] != NULL) {
        error_message(NETLIST, module_items->loc, "%s", "Error in Odin\n");
    }
    /* store the data which is an idx here */
    output_nets_sc->data[sc_spot] = (void*)verilog_netlist->one_net;
    verilog_netlist->one_net->name = one_string;

    /* CREATE the driver for the PAD */
    verilog_netlist->pad_node->name = make_full_ref_name(instance_name_prefix, NULL, NULL, pad_string, -1);
    vtr::free(pad_string);
    pad_string = verilog_netlist->pad_node->name;

    sc_spot = sc_add_string(output_nets_sc, pad_string);
    if (output_nets_sc->data[sc_spot] != NULL) {
        error_message(NETLIST, module_items->loc, "%s", "Error in Odin\n");
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
void create_top_output_nodes(ast_node_t* module, char* instance_name_prefix, sc_hierarchy* local_ref) {
    /* with the top module we need to visit the entire ast tree */
    long i;
    int k;
    long sc_spot;
    long num_local_symbol_table = local_ref->num_local_symbol_table;
    ast_node_t** local_symbol_table = local_ref->local_symbol_table;
    ast_node_t* module_items = module->children[1];
    nnode_t* new_node;

    oassert(module_items->type == MODULE_ITEMS);

    /* search the symbol table for outputs */
    if (local_symbol_table && num_local_symbol_table > 0) {
        for (i = 0; i < num_local_symbol_table; i++) {
            if (local_symbol_table[i]->types.variable.is_output) {
                char* full_name;
                ast_node_t* var_declare = local_symbol_table[i];
                npin_t* new_pin;

                /* decide what type of variable this is */
                if (var_declare->children[0] == NULL) {
                    /* IF - this is a identifier then find it in the output_nets and hook it up to a newly created node */
                    /* get the name of the pin */
                    full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, var_declare->identifier_node->types.identifier, -1);

                    /* check if the instantiation pin exists. */
                    if ((sc_spot = sc_lookup_string(output_nets_sc, full_name)) == -1) {
                        warning_message(NETLIST, var_declare->loc,
                                        "Output pin (%s) is not hooked up!!!\n", full_name);
                    }

                    new_pin = allocate_npin();
                    /* hookup this pin to this net */
                    add_fanout_pin_to_net((nnet_t*)output_nets_sc->data[sc_spot], new_pin);

                    /* create the node */
                    new_node = allocate_nnode(var_declare->loc);
                    new_node->related_ast_node = ast_node_deep_copy(var_declare);
                    new_node->name = full_name;
                    new_node->type = OUTPUT_NODE;
                    /* allocate the pins needed */
                    allocate_more_input_pins(new_node, 1);
                    add_input_port_information(new_node, 1);

                    /* hookup the pin, and node */
                    add_input_pin_to_node(new_node, new_pin, 0);

                    /* record this node */
                    verilog_netlist->top_output_nodes = (nnode_t**)vtr::realloc(verilog_netlist->top_output_nodes, sizeof(nnode_t*) * (verilog_netlist->num_top_output_nodes + 1));
                    verilog_netlist->top_output_nodes[verilog_netlist->num_top_output_nodes] = new_node;
                    verilog_netlist->num_top_output_nodes++;
                } else if (var_declare->children[2] == NULL) {
                    ast_node_t* node_max = var_declare->children[0];
                    ast_node_t* node_min = var_declare->children[1];

                    oassert(node_min->type == NUMBERS && node_max->type == NUMBERS);
                    long max_value = node_max->types.vnumber->get_value();
                    long min_value = node_min->types.vnumber->get_value();

                    if (min_value > max_value) {
                        error_message(NETLIST, var_declare->loc, "%s",
                                      "Odin doesn't support arrays declared [m:n] where m is less than n.");
                    }
                    //ODIN doesn't support negative number in index now.
                    if (min_value < 0 || max_value < 0) {
                        warning_message(NETLIST, var_declare->loc, "%s",
                                        "Odin doesn't support negative number in index.");
                    }

                    /* assume digit 1 is largest */
                    for (k = min_value; k <= max_value; k++) {
                        /* get the name of the pin */
                        full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, var_declare->identifier_node->types.identifier, k);

                        /* check if the instantiation pin exists. */
                        if ((sc_spot = sc_lookup_string(output_nets_sc, full_name)) == -1) {
                            warning_message(NETLIST, var_declare->loc,
                                            "Output pin (%s) is not hooked up!!!\n", full_name);
                        }
                        new_pin = allocate_npin();
                        /* hookup this pin to this net */
                        add_fanout_pin_to_net((nnet_t*)output_nets_sc->data[sc_spot], new_pin);

                        /* create the node */
                        new_node = allocate_nnode(var_declare->loc);
                        new_node->related_ast_node = ast_node_deep_copy(var_declare);
                        new_node->name = full_name;
                        new_node->type = OUTPUT_NODE;
                        /* allocate the pins needed */
                        allocate_more_input_pins(new_node, 1);
                        add_input_port_information(new_node, 1);

                        /* hookup the pin, and node */
                        add_input_pin_to_node(new_node, new_pin, 0);

                        /* record this node */
                        verilog_netlist->top_output_nodes = (nnode_t**)vtr::realloc(verilog_netlist->top_output_nodes, sizeof(nnode_t*) * (verilog_netlist->num_top_output_nodes + 1));
                        verilog_netlist->top_output_nodes[verilog_netlist->num_top_output_nodes] = new_node;
                        verilog_netlist->num_top_output_nodes++;
                    }
                } else {
                    /* Implicit memory */
                    error_message(NETLIST, var_declare->loc, "%s",
                                  "Memory not handled ... yet in create_top_output_nodes!\n");
                }
            }
        }

    } else {
        error_message(NETLIST, module_items->loc, "%s", "Empty module\n");
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: define_nets_with_driver)
 * Given a register declaration, this function defines it as a driver.
 *-------------------------------------------------------------------------------------------*/
nnet_t* define_nets_with_driver(ast_node_t* var_declare, char* instance_name_prefix) {
    int i;
    char* temp_string = NULL;
    long sc_spot;
    nnet_t* new_net = NULL;

    if (var_declare->children[0] == NULL || var_declare->type == BLOCKING_STATEMENT) {
        /* FOR single driver signal since spots 1, 2, 3, 4 are NULL */

        /* create the net */
        new_net = allocate_nnet();

        /* make the string to add to the string cache */
        temp_string = make_full_ref_name(instance_name_prefix, NULL, NULL, var_declare->identifier_node->types.identifier, -1);

        /* look for that element */
        sc_spot = sc_add_string(output_nets_sc, temp_string);
        if (output_nets_sc->data[sc_spot] != NULL) {
            error_message(NETLIST, var_declare->loc,
                          "Net (%s) with the same name already created\n", temp_string);
        }
        /* store the data which is an idx here */
        output_nets_sc->data[sc_spot] = (void*)new_net;
        new_net->name = temp_string;

        if (var_declare->types.variable.initial_value) {
            new_net->initial_value = (init_value_e)var_declare->types.variable.initial_value->get_bit_from_lsb(0);
        }

    } else if (var_declare->children[2] == NULL) {
        ast_node_t* node_max = var_declare->children[0];
        ast_node_t* node_min = var_declare->children[1];

        /* FOR array driver  since sport 3 and 4 are NULL */
        oassert(node_min->type == NUMBERS && node_max->type == NUMBERS);
        long min_value = node_min->types.vnumber->get_value();
        long max_value = node_max->types.vnumber->get_value();

        if (min_value > max_value) {
            error_message(NETLIST, var_declare->loc, "%s",
                          "Odin doesn't support arrays declared [m:n] where m is less than n.");
        }
        //ODIN doesn't support negative number in index now.
        if (min_value < 0 || max_value < 0) {
            warning_message(NETLIST, var_declare->loc, "%s",
                            "Odin doesn't support negative number in index.");
        }

        /* This register declaration is a range as opposed to a single bit so we need to define each element */
        /* assume digit 1 is largest */
        for (i = min_value; i <= max_value; i++) {
            /* create the net */
            new_net = allocate_nnet();

            /* create the string to add to the cache */
            temp_string = make_full_ref_name(instance_name_prefix, NULL, NULL, var_declare->identifier_node->types.identifier, i);

            sc_spot = sc_add_string(output_nets_sc, temp_string);
            if (output_nets_sc->data[sc_spot] != NULL) {
                error_message(NETLIST, var_declare->loc,
                              "Net (%s) with the same name already created\n", temp_string);
            }
            /* store the data which is an idx here */
            output_nets_sc->data[sc_spot] = (void*)new_net;
            new_net->name = temp_string;

            if (var_declare->types.variable.initial_value) {
                new_net->initial_value = (init_value_e)var_declare->types.variable.initial_value->get_bit_from_lsb(i);
            }
        }
    }
    /* Implicit memory */
    else if (var_declare->children[2] != NULL && var_declare->types.variable.is_memory) {
        ast_node_t* node_max1 = var_declare->children[0];
        ast_node_t* node_min1 = var_declare->children[1];

        oassert(node_min1->type == NUMBERS && node_max1->type == NUMBERS);
        long data_min = node_min1->types.vnumber->get_value();
        long data_max = node_max1->types.vnumber->get_value();

        if (data_min > data_max) {
            error_message(NETLIST, var_declare->loc, "%s",
                          "Odin doesn't support arrays declared [m:n] where m is less than n.");
        }
        //ODIN doesn't support negative number in index now.
        if (data_min < 0 || data_max < 0) {
            warning_message(NETLIST, var_declare->loc, "%s",
                            "Odin doesn't support negative number in index.");
        }

        ast_node_t* node_max2 = var_declare->children[2];
        ast_node_t* node_min2 = var_declare->children[3];

        oassert(node_min2->type == NUMBERS && node_max2->type == NUMBERS);
        long addr_min = node_min2->types.vnumber->get_value();
        long addr_max = node_max2->types.vnumber->get_value();

        if (addr_min > addr_max) {
            error_message(NETLIST, var_declare->loc, "%s",
                          "Odin doesn't support arrays declared [m:n] where m is less than n.");
        }
        //ODIN doesn't support negative number in index now.
        if (addr_min < 0 || addr_max < 0) {
            warning_message(NETLIST, var_declare->loc, "%s",
                            "Odin doesn't support negative number in index.");
        }

        char* name = var_declare->identifier_node->types.identifier;

        if (data_min != 0)
            error_message(NETLIST, var_declare->loc,
                          "%s: right memory index must be zero\n", name);

        oassert(data_min <= data_max);

        if (addr_min != 0)
            error_message(NETLIST, var_declare->loc,
                          "%s: right memory address index must be zero\n", name);

        oassert(addr_min <= addr_max);

        long data_width = data_max - data_min + 1;
        long address_width = addr_max - addr_min + 1;

        create_implicit_memory_block(data_width, address_width, name, instance_name_prefix, var_declare->loc);
    }

    return new_net;
}

/*---------------------------------------------------------------------------------------------
 * (function:  define_nodes_and_nets_with_driver)
 * 	Similar to define_nets_with_driver except this one is for top level nodes and
 * 	is making the input pins into nodes and drivers.
 *-------------------------------------------------------------------------------------------*/
nnet_t* define_nodes_and_nets_with_driver(ast_node_t* var_declare, char* instance_name_prefix) {
    int i;
    char* temp_string;
    long sc_spot;
    nnet_t* new_net = NULL;
    npin_t* new_pin;
    nnode_t* new_node;

    if (var_declare->children[0] == NULL) {
        /* FOR single driver signal since spots 1, 2, 3, 4 are NULL */

        /* create the net */
        new_net = allocate_nnet();

        temp_string = make_full_ref_name(instance_name_prefix, NULL, NULL, var_declare->identifier_node->types.identifier, -1);

        sc_spot = sc_add_string(output_nets_sc, temp_string);
        if (output_nets_sc->data[sc_spot] != NULL) {
            error_message(NETLIST, var_declare->loc,
                          "Net (%s) with the same name already created\n", temp_string);
        }
        /* store the data which is an idx here */
        output_nets_sc->data[sc_spot] = (void*)new_net;
        new_net->name = temp_string;

        /* create this node and make the pin connection to the net */
        new_pin = allocate_npin();
        new_node = allocate_nnode(var_declare->loc);
        new_node->name = vtr::strdup(temp_string);

        new_node->related_ast_node = var_declare;
        new_node->type = INPUT_NODE;
        /* allocate the pins needed */
        allocate_more_output_pins(new_node, 1);
        add_output_port_information(new_node, 1);

        /* hookup the pin, net, and node */
        add_output_pin_to_node(new_node, new_pin, 0);
        add_driver_pin_to_net(new_net, new_pin);

        /* store it in the list of input nodes */
        verilog_netlist->top_input_nodes = (nnode_t**)vtr::realloc(verilog_netlist->top_input_nodes, sizeof(nnode_t*) * (verilog_netlist->num_top_input_nodes + 1));
        verilog_netlist->top_input_nodes[verilog_netlist->num_top_input_nodes] = new_node;
        verilog_netlist->num_top_input_nodes++;
    } else if (var_declare->children[2] == NULL) {
        /* FOR array driver  since sport 3 and 4 are NULL */
        ast_node_t* node_max = var_declare->children[0];
        ast_node_t* node_min = var_declare->children[1];

        oassert(node_min->type == NUMBERS && node_max->type == NUMBERS);
        long min_value = node_min->types.vnumber->get_value();
        long max_value = node_max->types.vnumber->get_value();

        if (min_value > max_value) {
            error_message(NETLIST, var_declare->loc, "%s",
                          "Odin doesn't support arrays declared [m:n] where m is less than n.");
        }
        //ODIN doesn't support negative number in index now.
        if (min_value < 0 || max_value < 0) {
            warning_message(NETLIST, var_declare->loc, "%s",
                            "Odin doesn't support negative number in index.");
        }

        /* assume digit 1 is largest */
        for (i = min_value; i <= max_value; i++) {
            /* create the net */
            new_net = allocate_nnet();

            /* FOR single driver signal */
            temp_string = make_full_ref_name(instance_name_prefix, NULL, NULL, var_declare->identifier_node->types.identifier, i);

            sc_spot = sc_add_string(output_nets_sc, temp_string);
            if (output_nets_sc->data[sc_spot] != NULL) {
                error_message(NETLIST, var_declare->loc,
                              "Net (%s) with the same name already created\n", temp_string);
            }
            /* store the data which is an idx here */
            output_nets_sc->data[sc_spot] = (void*)new_net;
            new_net->name = temp_string;

            /* create this node and make the pin connection to the net */
            new_pin = allocate_npin();
            new_node = allocate_nnode(var_declare->loc);

            new_node->related_ast_node = var_declare;
            new_node->name = vtr::strdup(temp_string);
            new_node->type = INPUT_NODE;
            /* allocate the pins needed */
            allocate_more_output_pins(new_node, 1);
            add_output_port_information(new_node, 1);

            /* hookup the pin, net, and node */
            add_output_pin_to_node(new_node, new_pin, 0);
            add_driver_pin_to_net(new_net, new_pin);

            /* store it in the list of input nodes */
            verilog_netlist->top_input_nodes = (nnode_t**)vtr::realloc(verilog_netlist->top_input_nodes, sizeof(nnode_t*) * (verilog_netlist->num_top_input_nodes + 1));
            verilog_netlist->top_input_nodes[verilog_netlist->num_top_input_nodes] = new_node;
            verilog_netlist->num_top_input_nodes++;
        }
    } else if (var_declare->types.variable.is_memory) {
        /* Implicit memory */
        error_message(NETLIST, var_declare->children[2]->loc, "%s\n", "Unhandled implicit memory in define_nodes_and_nets_with_driver");
    }

    return new_net;
}

/*---------------------------------------------------------------------------------------------
 * (function: create_symbol_table_for_scope)
 * 	Creates a lookup of the variables declared here so that in the analysis we can look
 * 	up the definition of it to decide what to do.
 *-------------------------------------------------------------------------------------------*/
void create_symbol_table_for_scope(ast_node_t* module_items, sc_hierarchy* local_ref) {
    /* with the top module we need to visit the entire ast tree */
    long i, j;
    char* temp_string;
    long sc_spot;
    oassert(module_items->type == MODULE_ITEMS || module_items->type == FUNCTION_ITEMS || module_items->type == TASK_ITEMS || module_items->type == BLOCK);

    STRING_CACHE* local_symbol_table_sc = local_ref->local_symbol_table_sc;
    ast_node_t** local_symbol_table = local_ref->local_symbol_table;
    int num_local_symbol_table = local_ref->num_local_symbol_table;

    ast_node_t** implicit_declarations = NULL;
    int num_implicit_declarations = 0;

    /* search for VAR_DECLARE_LISTS */
    if (module_items->num_children > 0) {
        for (i = 0; i < module_items->num_children; i++) {
            if (module_items->children[i]->type == VAR_DECLARE_LIST) {
                /* go through the vars in this declare list */
                for (j = 0; j < module_items->children[i]->num_children; j++) {
                    ast_node_t* var_declare = module_items->children[i]->children[j];

                    /* parameters are already dealt with */
                    if (var_declare->types.variable.is_parameter
                        || var_declare->types.variable.is_localparam
                        || var_declare->types.variable.is_defparam)

                        continue;

                    oassert(var_declare->type == VAR_DECLARE);
                    oassert((var_declare->types.variable.is_input) || (var_declare->types.variable.is_output) || (var_declare->types.variable.is_reg) || (var_declare->types.variable.is_genvar) || (var_declare->types.variable.is_wire));

                    if (var_declare->types.variable.is_input
                        && var_declare->types.variable.is_reg) {
                        error_message(NETLIST, var_declare->loc, "%s",
                                      "Input cannot be defined as a reg\n");
                    }

                    /* make the string to add to the string cache */
                    temp_string = make_full_ref_name(NULL, NULL, NULL, var_declare->identifier_node->types.identifier, -1);
                    /* look for that element */
                    sc_spot = sc_add_string(local_symbol_table_sc, temp_string);

                    if (local_symbol_table_sc->data[sc_spot] != NULL) {
                        /* ERROR checks here
                         * output with reg is fine
                         * output with wire is fine
                         * input with wire is fine
                         * Then update the stored string cache entry with information */
                        if (var_declare->types.variable.is_input
                            && ((ast_node_t*)local_symbol_table_sc->data[sc_spot])->types.variable.is_reg) {
                            error_message(NETLIST, var_declare->loc, "%s",
                                          "Input cannot be defined as a reg\n");
                        }
                        /* MORE ERRORS ... could check for same declaration name ... */
                        else if (var_declare->types.variable.is_output) {
                            /* copy all the reg and wire info over */
                            ((ast_node_t*)local_symbol_table_sc->data[sc_spot])->types.variable.is_output = true;
                        } else if ((var_declare->types.variable.is_reg) || (var_declare->types.variable.is_wire)
                                   || (var_declare->types.variable.is_genvar)) {
                            /* copy the output status over */
                            ((ast_node_t*)local_symbol_table_sc->data[sc_spot])->types.variable.is_wire = var_declare->types.variable.is_wire;
                            ((ast_node_t*)local_symbol_table_sc->data[sc_spot])->types.variable.is_reg = var_declare->types.variable.is_reg;
                        } else {
                            oassert(false && "invalid type");
                        }
                    } else {
                        /* store the data which is an idx here */
                        local_symbol_table_sc->data[sc_spot] = (void*)var_declare;

                        /* store the symbol */
                        local_symbol_table = (ast_node_t**)vtr::realloc(local_symbol_table, sizeof(ast_node_t*) * (num_local_symbol_table + 1));
                        local_symbol_table[num_local_symbol_table] = (ast_node_t*)var_declare;
                        num_local_symbol_table++;

                        module_items->children[i]->children[j] = NULL;
                    }

                    /* check for an initial value and copy it over if found */
                    VNumber* init_value = get_init_value(var_declare->children[4]);
                    ((ast_node_t*)local_symbol_table_sc->data[sc_spot])->types.variable.initial_value = init_value;

                    vtr::free(temp_string);

                    remove_child_from_node_at_index(module_items->children[i], j);
                    j--;
                }

                if (module_items->children[i]->num_children == 0) {
                    remove_child_from_node_at_index(module_items, i);
                    i--;
                }
            } else if (module_items->children[i]->type == ASSIGN) {
                /* might be an implicit declaration */
                if ((module_items->children[i]->children[0]) && (module_items->children[i]->children[0]->type == BLOCKING_STATEMENT)) {
                    if ((module_items->children[i]->children[0]->identifier_node) && (module_items->children[i]->children[0]->identifier_node->type == IDENTIFIERS)) {
                        temp_string = make_full_ref_name(NULL, NULL, NULL, module_items->children[i]->children[0]->identifier_node->types.identifier, -1);
                        /* look for that element */
                        sc_spot = sc_lookup_string(local_symbol_table_sc, temp_string);
                        if (sc_spot == -1) {
                            implicit_declarations = (ast_node_t**)vtr::realloc(implicit_declarations, sizeof(ast_node_t*) * (num_implicit_declarations + 1));
                            implicit_declarations[num_implicit_declarations] = module_items->children[i]->children[0]->identifier_node;
                            num_implicit_declarations++;
                        }
                        vtr::free(temp_string);
                    }
                }
            }
        }

        /* add implicit declarations to string cache */
        for (i = 0; i < num_implicit_declarations; i++) {
            ast_node_t* node = implicit_declarations[i];
            ast_node_t* potential_var_declare = resolve_hierarchical_name_reference(local_ref, node->types.identifier);

            if (potential_var_declare == NULL) {
                sc_spot = sc_add_string(local_symbol_table_sc, node->types.identifier);
                oassert(sc_spot > -1);

                if (local_symbol_table_sc->data[sc_spot] == NULL) {
                    ast_node_t* var_declare = create_node_w_type(VAR_DECLARE, node->loc);

                    /* copy the output status over */
                    var_declare->types.variable.is_wire = true;
                    var_declare->types.variable.is_reg = false;

                    var_declare->types.variable.is_input = false;

                    allocate_children_to_node(var_declare, {node, NULL, NULL, NULL, NULL, NULL});

                    /* store the data which is an idx here */
                    local_symbol_table_sc->data[sc_spot] = (void*)var_declare;

                    /* store the symbol */
                    local_symbol_table = (ast_node_t**)vtr::realloc(local_symbol_table, sizeof(ast_node_t*) * (num_local_symbol_table + 1));
                    local_symbol_table[num_local_symbol_table] = (ast_node_t*)var_declare;
                    num_local_symbol_table++;
                } else {
                    /* net was declared later; do nothing */
                }
            } else {
                /* var_declare was defined in encompassing scope */
            }
        }

        if (implicit_declarations) {
            vtr::free(implicit_declarations);
        }

        local_ref->local_symbol_table = local_symbol_table;
        local_ref->num_local_symbol_table = num_local_symbol_table;
    } else {
        error_message(NETLIST, module_items->loc, "%s", "Empty module\n");
    }
}

/*--------------------------------------------------------------------------
 * (function: check_for_initial_reg_value)
 * 	This function looks at a VAR_DECLARE AST node and checks to see
 *  if the register declaration includes an initial value.
 *  Returns the initial value in *value if one is found.
 *  Added by Conor
 *-------------------------------------------------------------------------*/

VNumber* get_init_value(ast_node_t* node) {
    // by default it is always undefined
    VNumber* init_value = nullptr;
    // Initial value is always the last child, if one exists
    if (node != NULL && node->types.vnumber != nullptr) {
        init_value = new VNumber(node->types.vnumber);
    }
    return init_value;
}

/*--------------------------------------------------------------------------
 * (function: connect_memory_and_alias)
 * 	This function looks at the memory instantiation points and checks
 * 		if there are any inputs that don't have a driver at this level.
 *  	If they don't then name needs to be aliased for higher levels to
 *  	understand.  If there exists a driver, then we connect the two.
 *
 * 	Assume that ports are written in the same order as the instantiation.
 *-------------------------------------------------------------------------*/
void connect_memory_and_alias(ast_node_t* hb_instance, char* instance_name_prefix, sc_hierarchy* local_ref) {
    ast_node_t* hb_connect_list;
    long i;
    int j;
    long sc_spot_output;
    long sc_spot_input_new, sc_spot_input_old;

    /* Note: Hard Block port matching is checked in earlier node processing */
    hb_connect_list = hb_instance->children[0]->children[0];
    for (i = 0; i < hb_connect_list->num_children; i++) {
        int port_size;

        ast_node_t* hb_var_node = hb_connect_list->children[i]->identifier_node;
        ast_node_t* hb_instance_var_node = hb_connect_list->children[i]->children[0];
        char* ip_name = hb_var_node->types.identifier;

        /* We can ignore inputs since they are already resolved */
        enum PORTS port_dir = (!strcmp(ip_name, "out") || !strcmp(ip_name, "out1") || !strcmp(ip_name, "out2"))
                                  ? OUT_PORT
                                  : IN_PORT;
        if ((port_dir == OUT_PORT) || (port_dir == INOUT_PORT)) {
            char* name_of_hb_input;
            char* full_name;
            char* alias_name;

            /*this will crash because of an oassert.we keep it but we work around the problem here*/
            if (hb_instance_var_node->type == RANGE_REF) {
                full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, NULL, -1);
            } else {
                name_of_hb_input = get_name_of_pin_at_bit(hb_instance_var_node, -1, instance_name_prefix, local_ref);
                full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_hb_input, -1);
                vtr::free(name_of_hb_input);
            }

            alias_name = make_full_ref_name(instance_name_prefix,
                                            hb_instance->identifier_node->types.identifier,
                                            hb_instance->children[0]->identifier_node->types.identifier,
                                            hb_connect_list->children[i]->identifier_node->types.identifier, -1);

            /* Lookup port size in cache */
            port_size = get_memory_port_size(alias_name);
            vtr::free(alias_name);
            vtr::free(full_name);
            oassert(port_size != 0);

            for (j = 0; j < port_size; j++) {
                /* This is an output pin from the hard block. We need to
                 * alias this output pin with it's calling name here so that
                 * everyone can see it at this level.
                 *
                 * Make the new string for the alias name */
                if (port_size > 1) {
                    name_of_hb_input = get_name_of_pin_at_bit(hb_instance_var_node, j, instance_name_prefix, local_ref);
                    full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_hb_input, -1);
                    vtr::free(name_of_hb_input);

                    alias_name = make_full_ref_name(instance_name_prefix,
                                                    hb_instance->identifier_node->types.identifier,
                                                    hb_instance->children[0]->identifier_node->types.identifier,
                                                    hb_connect_list->children[i]->identifier_node->types.identifier, j);
                } else {
                    oassert(j == 0);
                    /* this will crash because of an oassert. we keep it but we work around the problem here */
                    if (hb_instance_var_node->type == RANGE_REF) {
                        full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, NULL, -1);
                    } else {
                        name_of_hb_input = get_name_of_pin_at_bit(hb_instance_var_node, -1, instance_name_prefix, local_ref);
                        full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_hb_input, j);
                        vtr::free(name_of_hb_input);
                    }

                    alias_name = make_full_ref_name(instance_name_prefix,
                                                    hb_instance->identifier_node->types.identifier,
                                                    hb_instance->children[0]->identifier_node->types.identifier,
                                                    hb_connect_list->children[i]->identifier_node->types.identifier, -1);
                }

                /* Search for the old_input name */
                sc_spot_input_old = sc_lookup_string(input_nets_sc, alias_name);

                /* check if the instantiation pin exists */
                if ((sc_spot_output = sc_lookup_string(output_nets_sc, full_name)) == -1) {
                    /* IF - no driver, then assume that it needs to be
                     * aliased to move up as an input */
                    if ((sc_spot_input_new = sc_lookup_string(input_nets_sc, full_name)) == -1) {
                        /* if this input is not yet used in this module
                         * then we'll add it */
                        sc_spot_input_new = sc_add_string(input_nets_sc, full_name);
                        /* copy the pin to the old spot */
                        input_nets_sc->data[sc_spot_input_new] = input_nets_sc->data[sc_spot_input_old];
                    } else {
                        /* already exists so we'll join the nets */
                        combine_nets((nnet_t*)input_nets_sc->data[sc_spot_input_old], (nnet_t*)input_nets_sc->data[sc_spot_input_new], verilog_netlist);
                        input_nets_sc->data[sc_spot_input_old] = NULL;
                    }
                } else {
                    /* ELSE - we've found a matching net,
                     * so add this pin to the net */
                    nnet_t* net = (nnet_t*)output_nets_sc->data[sc_spot_output];
                    nnet_t* in_net = (nnet_t*)input_nets_sc->data[sc_spot_input_old];

                    /* if they haven't been combined already,
                     * then join the inputs and output */
                    in_net->name = net->name;
                    combine_nets(net, in_net, verilog_netlist);
                    net = NULL;

                    /* since the driver net is deleted,
                     * copy the spot of the in_net over */
                    input_nets_sc->data[sc_spot_input_old] = (void*)in_net;
                    output_nets_sc->data[sc_spot_output] = (void*)in_net;

                    /* if this input is not yet used in this module
                     * then we'll add it */
                    sc_spot_input_new = sc_add_string(input_nets_sc, full_name);
                    /* copy the pin to the old spot */
                    input_nets_sc->data[sc_spot_input_new] = (void*)in_net;
                }

                vtr::free(full_name);
                vtr::free(alias_name);
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
void connect_hard_block_and_alias(ast_node_t* hb_instance, char* instance_name_prefix, int outport_size, sc_hierarchy* local_ref) {
    t_model* hb_model;
    ast_node_t* hb_connect_list;
    long i;
    int j;
    long sc_spot_output;
    long sc_spot_input_new, sc_spot_input_old;

    /* See if the hard block declared is supported by FPGA architecture */
    char* identifier = hb_instance->identifier_node->types.identifier;
    hb_model = find_hard_block(identifier);
    if (!hb_model) {
        error_message(NETLIST, hb_instance->loc, "Found Hard Block \"%s\": Not supported by FPGA Architecture\n", identifier);
    }

    /* Note: Hard Block port matching is checked in earlier node processing */
    hb_connect_list = hb_instance->children[0]->children[0];
    for (i = 0; i < hb_connect_list->num_children; i++) {
        int port_size;
        int flag = 0;
        enum PORTS port_dir;
        ast_node_t* hb_var_node = hb_connect_list->children[i]->identifier_node;
        ast_node_t* hb_instance_var_node = hb_connect_list->children[i]->children[0];

        /* We can ignore inputs since they are already resolved */
        port_dir = hard_block_port_direction(hb_model, hb_var_node->types.identifier);
        if ((port_dir == OUT_PORT) || (port_dir == INOUT_PORT)) {
            port_size = hard_block_port_size(hb_model, hb_var_node->types.identifier);
            oassert(port_size != 0);
            port_size = outport_size;

            if (strcmp(hb_model->name, "adder") == 0) {
                if (strcmp(hb_var_node->types.identifier, "cout") == 0 && flag == 0)
                    port_size = 1;
                else if (strcmp(hb_var_node->types.identifier, "sumout") == 0 && flag == 0)
                    port_size = port_size - 1;
            }

            for (j = 0; j < port_size; j++) {
                /* This is an output pin from the hard block. We need to
                 * alias this output pin with it's calling name here so that
                 * everyone can see it at this level
                 */
                char* name_of_hb_input;
                char* full_name;
                char* alias_name;

                /* make the new string for the alias name */
                if (port_size > 1) {
                    name_of_hb_input = get_name_of_pin_at_bit(hb_instance_var_node, j, instance_name_prefix, local_ref);
                    full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_hb_input, -1);
                    vtr::free(name_of_hb_input);

                    alias_name = make_full_ref_name(instance_name_prefix,
                                                    hb_instance->identifier_node->types.identifier,
                                                    hb_instance->children[0]->identifier_node->types.identifier,
                                                    hb_connect_list->children[i]->identifier_node->types.identifier, j);
                } else {
                    oassert(j == 0);
                    /* this will crash because of an oassert. we keep it but we work around the problem here */
                    if (hb_instance_var_node->type == RANGE_REF) {
                        full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, NULL, -1);
                    } else {
                        name_of_hb_input = get_name_of_pin_at_bit(hb_instance_var_node, -1, instance_name_prefix, local_ref);
                        full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_hb_input, -1);
                        vtr::free(name_of_hb_input);
                    }

                    alias_name = make_full_ref_name(instance_name_prefix,
                                                    hb_instance->identifier_node->types.identifier,
                                                    hb_instance->children[0]->identifier_node->types.identifier,
                                                    hb_connect_list->children[i]->identifier_node->types.identifier, -1);
                }

                /* Search for the old_input name */
                sc_spot_input_old = sc_lookup_string(input_nets_sc, alias_name);
                if (sc_spot_input_old == -1) {
                    sc_spot_input_old = sc_add_string(input_nets_sc, alias_name);
                    input_nets_sc->data[sc_spot_input_old] = NULL;
                }

                /* check if the instantiation pin exists */
                if ((sc_spot_output = sc_lookup_string(output_nets_sc, full_name)) == -1) {
                    /* IF - no driver, then assume that it needs to be
                     * aliased to move up as an input */
                    if ((sc_spot_input_new = sc_lookup_string(input_nets_sc, full_name)) == -1) {
                        /* if this input is not yet used in this module
                         * then we'll add it */
                        sc_spot_input_new = sc_add_string(input_nets_sc, full_name);
                        /* copy the pin to the old spot */
                        input_nets_sc->data[sc_spot_input_new] = input_nets_sc->data[sc_spot_input_old];
                    } else {
                        /* already exists so we'll join the nets */
                        combine_nets((nnet_t*)input_nets_sc->data[sc_spot_input_old], (nnet_t*)input_nets_sc->data[sc_spot_input_new], verilog_netlist);
                        input_nets_sc->data[sc_spot_input_old] = NULL;
                    }
                } else {
                    /* ELSE - we've found a matching net,
                     * so add this pin to the net */
                    nnet_t* net = (nnet_t*)output_nets_sc->data[sc_spot_output];
                    nnet_t* in_net = (nnet_t*)input_nets_sc->data[sc_spot_input_old];

                    /* if they haven't been combined already,
                     * then join the inputs and output */
                    if (in_net != NULL) {
                        in_net->name = net->name;
                        combine_nets(net, in_net, verilog_netlist);
                        net = NULL;

                        /* since the driver net is deleted,
                         * copy the spot of the in_net over */
                        input_nets_sc->data[sc_spot_input_old] = (void*)in_net;
                        output_nets_sc->data[sc_spot_output] = (void*)in_net;

                        /* if this input is not yet used in this module
                         * then we'll add it */
                        sc_spot_input_new = sc_add_string(input_nets_sc, full_name);
                        /* copy the pin to the old spot */
                        input_nets_sc->data[sc_spot_input_new] = (void*)in_net;
                    }
                }

                vtr::free(full_name);
                vtr::free(alias_name);
            }
        }
    }

    return;
}

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
void connect_module_instantiation_and_alias(short PASS, ast_node_t* module_instance, char* instance_name_prefix, sc_hierarchy* local_ref) {
    long i;
    int j;
    ast_node_t* module_node = NULL;
    ast_node_t* module_list = NULL;
    ast_node_t* module_instance_list = module_instance->children[0]->children[0]; // MODULE_INSTANCE->MODULE_INSTANCE_NAME(child[0])->MODULE_CONNECT_LIST(child[0])
    long sc_spot;
    long sc_spot_output;
    long sc_spot_input_old;
    long sc_spot_input_new;

    char* module_instance_name = make_full_ref_name(instance_name_prefix,
                                                    module_instance->identifier_node->types.identifier,
                                                    module_instance->children[0]->identifier_node->types.identifier,
                                                    NULL, -1);

    /* lookup the node of the module associated with this instantiated module */
    if ((sc_spot = sc_lookup_string(module_names_to_idx, module_instance_name)) == -1) {
        error_message(NETLIST, module_instance->loc,
                      "Can't find module %s\n", module_instance_name);
    }

    vtr::free(module_instance_name);

    module_node = (ast_node_t*)module_names_to_idx->data[sc_spot];
    module_list = module_node->children[0]; // MODULE->VAR_DECLARE_LIST(child[0])

    if (module_list->num_children != module_instance_list->num_children) {
        error_message(NETLIST, module_instance->loc,
                      "Module instantiation (%s) and definition don't match in terms of ports\n",
                      module_instance->identifier_node->types.identifier);
    }

    //reordering if the variable is instantiated by name
    if (module_instance_list->num_children > 0 && module_instance_list->children[0]->identifier_node != NULL) {
        reorder_connections_from_name(module_list, module_instance_list, MODULE_ITEMS);
    }

    for (i = 0; i < module_list->num_children; i++) {
        int port_size = 0;
        // VAR_DECLARE_LIST(child[i])->VAR_DECLARE_PORT(child[0])->VAR_DECLARE_input-or-output(child[0])
        ast_node_t* module_var_node = module_list->children[i];
        // MODULE_CONNECT_LIST(child[i])->MODULE_CONNECT(child[0])
        ast_node_t* module_instance_var_node = module_instance_list->children[i]->children[0];

        if (
            // skip inputs on pass 1
            ((PASS == INSTANTIATE_DRIVERS) && (module_list->children[i]->types.variable.is_output))
            // skip outputs on pass 2
            || ((PASS == ALIAS_INPUTS) && (module_list->children[i]->types.variable.is_input))) {
            /* calculate the port details */
            if (module_var_node->children[0] == NULL) {
                port_size = 1;
            } else if (module_var_node->children[2] == NULL) {
                ast_node_t* node1 = module_var_node->children[0];
                ast_node_t* node2 = module_var_node->children[1];

                oassert(node2->type == NUMBERS && node1->type == NUMBERS);
                /* assume all arrays declared [largest:smallest] */
                oassert(node2->types.vnumber->get_value() <= node1->types.vnumber->get_value());
                port_size = node1->types.vnumber->get_value() - node2->types.vnumber->get_value() + 1;
            } else if (module_var_node->children[4] == NULL) {
                ast_node_t* node1 = module_var_node->children[0];
                ast_node_t* node2 = module_var_node->children[1];
                ast_node_t* node3 = module_var_node->children[2];
                ast_node_t* node4 = module_var_node->children[3];

                oassert(node2->type == NUMBERS && node1->type == NUMBERS && node3->type == NUMBERS && node4->type == NUMBERS);
                /* assume all arrays declared [largest:smallest] */
                oassert(node2->types.vnumber->get_value() <= node1->types.vnumber->get_value());
                oassert(node4->types.vnumber->get_value() <= node3->types.vnumber->get_value());
                int port_1 = node1->types.vnumber->get_value() - node2->types.vnumber->get_value() + 1;
                int port_2 = node3->types.vnumber->get_value() - node4->types.vnumber->get_value() + 1;
                port_size = port_1 * port_2;
            }

            //---------------------------------------------------------------------------
            else if (module_var_node->children[4] != NULL) {
                /* Implicit memory */
                error_message(NETLIST, module_var_node->children[4]->loc, "%s\n", "Unhandled implicit memory in connect_module_instantiation_and_alias");
            }
            for (j = 0; j < port_size; j++) {
                if (module_var_node->types.variable.is_input) {
                    /* IF - this spot in the module list is an input, then we need to find it in the
                     * string cache (as its old name), check if the new_name (the instantiation name)
                     * is already a driver at this level.  IF it is a driver then we can hook the two up.
                     * IF it's not we need to alias the pin name as it is used here since it
                     * must be driven at a higher level of hierarchy in the tree of modules */
                    char* name_of_module_instance_of_input = NULL;
                    char* full_name = NULL;
                    char* alias_name = NULL;

                    if (port_size > 1) {
                        /* Get the name of the module instantiation pin */
                        name_of_module_instance_of_input = get_name_of_pin_at_bit(module_instance_var_node, j, instance_name_prefix, local_ref);
                        full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_module_instance_of_input, -1);
                        vtr::free(name_of_module_instance_of_input);

                        /* make the new string for the alias name - has to be a identifier in the instantiated modules old names */
                        name_of_module_instance_of_input = get_name_of_var_declare_at_bit(module_var_node, j);
                        alias_name = make_full_ref_name(instance_name_prefix,
                                                        module_instance->identifier_node->types.identifier,
                                                        module_instance->children[0]->identifier_node->types.identifier,
                                                        name_of_module_instance_of_input, -1);

                        vtr::free(name_of_module_instance_of_input);
                    } else {
                        oassert(j == 0);

                        /* Get the name of the module instantiation pin */
                        name_of_module_instance_of_input = get_name_of_pin_at_bit(module_instance_var_node, -1, instance_name_prefix, local_ref);
                        full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_module_instance_of_input, -1);
                        vtr::free(name_of_module_instance_of_input);

                        name_of_module_instance_of_input = get_name_of_var_declare_at_bit(module_var_node, 0);
                        alias_name = make_full_ref_name(instance_name_prefix,
                                                        module_instance->identifier_node->types.identifier,
                                                        module_instance->children[0]->identifier_node->types.identifier,
                                                        name_of_module_instance_of_input, -1);

                        vtr::free(name_of_module_instance_of_input);
                    }

                    /* search for the old_input name */
                    if ((sc_spot_input_old = sc_lookup_string(input_nets_sc, alias_name)) == -1) {
                        /* doesn it have to exist since might only be used in module */
                        if (port_size > 1) {
                            warning_message(NETLIST, module_instance_var_node->loc, "This module port %s[%d] is unused in module %s\n", module_instance_var_node->types.identifier, j, module_node->identifier_node->types.identifier);
                        } else {
                            warning_message(NETLIST, module_instance_var_node->loc, "This module port %s is unused in module %s\n", module_instance_var_node->types.identifier, module_node->identifier_node->types.identifier);
                        }

                    } else {
                        /* CMM - Check if this pin should be driven by the top level VCC or GND drivers	*/
                        if (strstr(full_name, ONE_VCC_CNS)) {
                            join_nets(verilog_netlist->one_net, (nnet_t*)input_nets_sc->data[sc_spot_input_old]);
                            free_nnet((nnet_t*)input_nets_sc->data[sc_spot_input_old]);
                            input_nets_sc->data[sc_spot_input_old] = (void*)verilog_netlist->one_net;
                        } else if (strstr(full_name, ZERO_GND_ZERO)) {
                            join_nets(verilog_netlist->zero_net, (nnet_t*)input_nets_sc->data[sc_spot_input_old]);
                            free_nnet((nnet_t*)input_nets_sc->data[sc_spot_input_old]);
                            input_nets_sc->data[sc_spot_input_old] = (void*)verilog_netlist->zero_net;
                        }
                        /* check if the instantiation pin exists. */
                        else if ((sc_spot_output = sc_lookup_string(output_nets_sc, full_name)) == -1) {
                            /* IF - no driver, then assume that it needs to be aliased to move up as an input */
                            if ((sc_spot_input_new = sc_lookup_string(input_nets_sc, full_name)) == -1) {
                                /* if this input is not yet used in this module then we'll add it */
                                sc_spot_input_new = sc_add_string(input_nets_sc, full_name);

                                /* copy the pin to the old spot */
                                input_nets_sc->data[sc_spot_input_new] = input_nets_sc->data[sc_spot_input_old];
                            } else {
                                /* already exists so we'll join the nets */
                                combine_nets((nnet_t*)input_nets_sc->data[sc_spot_input_old], (nnet_t*)input_nets_sc->data[sc_spot_input_new], verilog_netlist);
                                input_nets_sc->data[sc_spot_input_old] = NULL;
                            }
                        } else {
                            /* ELSE - we've found a matching net, so add this pin to the net */
                            nnet_t* net = (nnet_t*)output_nets_sc->data[sc_spot_output];
                            nnet_t* in_net = (nnet_t*)input_nets_sc->data[sc_spot_input_old];

                            if ((net != in_net) && (net->combined == true)) {
                                /* if they haven't been combined already, then join the inputs and output */
                                join_nets(net, in_net);
                                in_net = free_nnet(in_net);
                                /* since the driver net is deleted, copy the spot of the in_net over */
                                input_nets_sc->data[sc_spot_input_old] = (void*)net;
                            } else if ((net != in_net) && (net->combined == false)) {
                                /* if they haven't been combined already, then join the inputs and output */
                                combine_nets(net, in_net, verilog_netlist);
                                net = NULL;
                                /* since the driver net is deleted, copy the spot of the in_net over */
                                output_nets_sc->data[sc_spot_output] = (void*)in_net;
                            }
                        }
                    }

                    /* IF the designer uses port names then make sure they line up */
                    if (module_instance_list->children[i]->identifier_node != NULL) {
                        if (strcmp(module_instance_list->children[i]->identifier_node->types.identifier,
                                   module_var_node->identifier_node->types.identifier)
                            != 0) {
                            error_message(NETLIST, module_var_node->loc,
                                          "This module entry does not match up correctly (%s != %s).  Odin expects the order of ports to be the same\n",
                                          module_instance_list->children[i]->identifier_node->types.identifier,
                                          module_var_node->identifier_node->types.identifier);
                        }
                    }

                    vtr::free(full_name);
                    vtr::free(alias_name);
                } else if (module_var_node->types.variable.is_output) {
                    /* ELSE IF - this is an output pin from the module.  We need to alias this output
                     * pin with it's calling name here so that everyone can see it at this level */
                    char* name_of_module_instance_of_input = NULL;
                    char* full_name = NULL;
                    char* alias_name = NULL;

                    /* make the new string for the alias name - has to be a identifier in the
                     * instantiated modules old names */
                    if (port_size > 1) {
                        /* Get the name of the module instantiation pin */
                        name_of_module_instance_of_input = get_name_of_pin_at_bit(module_instance_var_node, j, instance_name_prefix, local_ref);
                        full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_module_instance_of_input, -1);
                        vtr::free(name_of_module_instance_of_input);

                        name_of_module_instance_of_input = get_name_of_var_declare_at_bit(module_var_node, j);
                        alias_name = make_full_ref_name(instance_name_prefix,
                                                        module_instance->identifier_node->types.identifier,
                                                        module_instance->children[0]->identifier_node->types.identifier, name_of_module_instance_of_input, -1);
                        vtr::free(name_of_module_instance_of_input);
                    } else {
                        oassert(j == 0);
                        /* Get the name of the module instantiation pin */
                        name_of_module_instance_of_input = get_name_of_pin_at_bit(module_instance_var_node, -1, instance_name_prefix, local_ref);
                        full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_module_instance_of_input, -1);
                        vtr::free(name_of_module_instance_of_input);

                        name_of_module_instance_of_input = get_name_of_var_declare_at_bit(module_var_node, 0);
                        alias_name = make_full_ref_name(instance_name_prefix,
                                                        module_instance->identifier_node->types.identifier,
                                                        module_instance->children[0]->identifier_node->types.identifier, name_of_module_instance_of_input, -1);
                        vtr::free(name_of_module_instance_of_input);
                    }

                    /* check if the instantiation pin exists. */
                    if ((sc_spot_output = sc_lookup_string(output_nets_sc, alias_name)) == -1) {
                        error_message(NETLIST, module_var_node->loc,
                                      "This output (%s) must exist...must be an error\n", alias_name);
                    }

                    /* can already be there */
                    sc_spot_input_new = sc_add_string(output_nets_sc, full_name);

                    /* Copy over the initial value data from the net alias to the corresponding
                     * flip-flop node if one exists. This is necessary if an initial value is
                     * assigned on a higher-level module since the flip-flop node will have
                     * already been instantiated without any initial value. */
                    nnet_t* output_net = (nnet_t*)output_nets_sc->data[sc_spot_output];
                    nnet_t* input_new_net = (nnet_t*)output_nets_sc->data[sc_spot_input_new];
                    for (int k = 0; k < output_net->num_driver_pins; k++) {
                        if (output_net->driver_pins[k] && output_net->driver_pins[k]->node && input_new_net) {
                            output_net->driver_pins[k]->node->initial_value = input_new_net->initial_value;
                        }
                    }

                    /* clean up input_new_net */
                    if (!(input_new_net) || !(input_new_net->num_driver_pins))
                        input_new_net = free_nnet(input_new_net);

                    /* add this alias for the net */
                    output_nets_sc->data[sc_spot_input_new] = output_nets_sc->data[sc_spot_output];

                    /* IF the designer users port names then make sure they line up */
                    if (module_instance_list->children[i]->identifier_node != NULL) {
                        if (strcmp(module_instance_list->children[i]->identifier_node->types.identifier, module_var_node->identifier_node->types.identifier) != 0) {
                            error_message(NETLIST, module_var_node->loc,
                                          "This module entry does not match up correctly (%s != %s).  Odin expects the order of ports to be the same\n",
                                          module_instance_list->children[i]->identifier_node->types.identifier,
                                          module_var_node->identifier_node->types.identifier);
                        }
                    }

                    vtr::free(full_name);
                    vtr::free(alias_name);
                }
            }
        }
    }
}

signal_list_t* connect_function_instantiation_and_alias(short PASS, ast_node_t* module_instance, char* instance_name_prefix, sc_hierarchy* local_ref) {
    signal_list_t* return_list = init_signal_list();

    long i;
    int j;
    //signal_list_t *aux_node = NULL;
    ast_node_t* module_node = NULL;
    ast_node_t* module_list = NULL;
    ast_node_t* module_instance_list = module_instance->children[0]->children[0]; // MODULE_INSTANCE->MODULE_INSTANCE_NAME(child[0])->MODULE_CONNECT_LIST(child[0])
    long sc_spot;
    long sc_spot_output;
    long sc_spot_input_old;
    long sc_spot_input_new;

    char* module_instance_name = module_instance->identifier_node->types.identifier;

    /* lookup the node of the module associated with this instantiated module */
    if ((sc_spot = sc_lookup_string(module_names_to_idx, module_instance_name)) == -1) {
        error_message(NETLIST, module_instance->loc,
                      "Can't find function %s\n", module_instance_name);
    }

    if (module_instance_name != module_instance->identifier_node->types.identifier)
        vtr::free(module_instance_name);

    module_node = (ast_node_t*)module_names_to_idx->data[sc_spot];
    module_list = module_node->children[0]; // MODULE->VAR_DECLARE_LIST(child[0])

    if (module_list->num_children != module_instance_list->num_children) {
        error_message(NETLIST, module_instance->loc,
                      "Function instantiation (%s) and definition don't match in terms of ports\n",
                      module_instance->identifier_node->types.identifier);
    }

    //reordering if the variable is instantiated by name
    if (module_instance_list->num_children > 0 && module_instance_list->children[1]->identifier_node != NULL) {
        reorder_connections_from_name(module_list, module_instance_list, FUNCTION);
    }

    for (i = 0; i < module_list->num_children; i++) {
        int port_size = 0;
        // VAR_DECLARE_LIST(child[i])->VAR_DECLARE_PORT(child[0])->VAR_DECLARE_input-or-output(child[0])
        ast_node_t* module_var_node = module_list->children[i];
        ast_node_t* module_instance_var_node = NULL;

        if (i > 0) module_instance_var_node = module_instance_list->children[i]->children[0];

        if (
            // skip inputs on pass 1
            ((PASS == INSTANTIATE_DRIVERS) && (module_var_node->types.variable.is_input))
            // skip outputs on pass 2
            || ((PASS == ALIAS_INPUTS) && (module_var_node->types.variable.is_output))) {
            continue;
        }

        /* IF the designer users port names then make sure they line up */
        // TODO: This code may need to be moved down the line
        if (i > 0 && module_instance_list->children[i]->identifier_node != NULL) {
            if (strcmp(module_instance_list->children[i]->identifier_node->types.identifier, module_var_node->identifier_node->types.identifier) != 0) {
                // TODO: This error message may not be correct. If assigning by name, the order should not matter
                error_message(NETLIST, module_var_node->loc,
                              "This function entry does not match up correctly (%s != %s).  Odin expects the order of ports to be the same\n",
                              module_instance_list->children[i]->identifier_node->types.identifier,
                              module_var_node->identifier_node->types.identifier);
            }
        }

        /* calculate the port details */
        if (module_var_node->children[0] == NULL) {
            port_size = 1;
        } else if (module_var_node->children[2] == NULL) {
            char* module_name = make_full_ref_name(instance_name_prefix,
                                                   // module_name
                                                   module_instance->identifier_node->types.identifier,
                                                   // instance name
                                                   module_instance->children[0]->identifier_node->types.identifier,
                                                   NULL, -1);

            ast_node_t* node1 = module_var_node->children[0];
            ast_node_t* node2 = module_var_node->children[1];

            vtr::free(module_name);
            oassert(node2->type == NUMBERS && node1->type == NUMBERS);
            /* assume all arrays declared [largest:smallest] */
            oassert(node2->types.vnumber->get_value() <= node1->types.vnumber->get_value());
            port_size = node1->types.vnumber->get_value() - node2->types.vnumber->get_value() + 1;
        } else if (module_var_node->children[4] == NULL) {
            char* module_name = make_full_ref_name(instance_name_prefix,
                                                   // module_name
                                                   module_instance->identifier_node->types.identifier,
                                                   // instance name
                                                   module_instance->children[0]->identifier_node->types.identifier,
                                                   NULL, -1);

            ast_node_t* node1 = module_var_node->children[0];
            ast_node_t* node2 = module_var_node->children[1];
            ast_node_t* node3 = module_var_node->children[2];
            ast_node_t* node4 = module_var_node->children[3];

            free(module_name);
            oassert(node2->type == NUMBERS && node1->type == NUMBERS && node3->type == NUMBERS && node4->type == NUMBERS);
            /* assume all arrays declared [largest:smallest] */
            oassert(node2->types.vnumber->get_value() <= node1->types.vnumber->get_value());
            oassert(node4->types.vnumber->get_value() <= node3->types.vnumber->get_value());
            port_size = node1->types.vnumber->get_value() * node3->types.vnumber->get_value() - 1;
        }

        //-----------------------------------------------------------------------------------
        else if (module_var_node->children[4] != NULL) {
            /* Implicit memory */
            error_message(NETLIST, module_var_node->children[4]->loc, "%s\n", "Unhandled implicit memory in connect_module_instantiation_and_alias");
        }
        for (j = 0; j < port_size; j++) {
            if (i > 0 && module_var_node->types.variable.is_input) {
                /* IF - this spot in the module list is an input, then we need to find it in the
                 * string cache (as its old name), check if the new_name (the instantiation name)
                 * is already a driver at this level.  IF it is a driver then we can hook the two up.
                 * IF it's not we need to alias the pin name as it is used here since it
                 * must be driven at a higher level of hierarchy in the tree of modules */
                char* name_of_module_instance_of_input = NULL;
                char* full_name = NULL;
                char* alias_name = NULL;

                if (port_size > 1) {
                    /* Get the name of the module instantiation pin */
                    name_of_module_instance_of_input = get_name_of_pin_at_bit(module_instance_var_node, j, instance_name_prefix, local_ref);
                    full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_module_instance_of_input, -1);
                    vtr::free(name_of_module_instance_of_input);

                    /* make the new string for the alias name - has to be a identifier in the instantiated modules old names */
                    name_of_module_instance_of_input = get_name_of_var_declare_at_bit(module_var_node, j);
                    alias_name = make_full_ref_name(instance_name_prefix,
                                                    module_instance->identifier_node->types.identifier,
                                                    module_instance->children[0]->identifier_node->types.identifier,
                                                    name_of_module_instance_of_input, -1);

                    vtr::free(name_of_module_instance_of_input);
                } else {
                    oassert(j == 0);

                    /* Get the name of the module instantiation pin */
                    name_of_module_instance_of_input = get_name_of_pin_at_bit(module_instance_var_node, -1, instance_name_prefix, local_ref);

                    full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_module_instance_of_input, -1);
                    vtr::free(name_of_module_instance_of_input);

                    name_of_module_instance_of_input = get_name_of_var_declare_at_bit(module_var_node, 0);
                    alias_name = make_full_ref_name(instance_name_prefix,
                                                    module_instance->identifier_node->types.identifier,
                                                    module_instance->children[0]->identifier_node->types.identifier,
                                                    name_of_module_instance_of_input, -1);

                    vtr::free(name_of_module_instance_of_input);
                }

                /* search for the old_input name */
                if ((sc_spot_input_old = sc_lookup_string(input_nets_sc, alias_name)) == -1) {
                    /* doesn it have to exist since might only be used in module */
                    if (port_size > 1)
                        warning_message(NETLIST, module_instance_var_node->loc, "This function port %s[%d] is unused in module %s\n", module_instance_var_node->types.identifier, j, module_node->identifier_node->types.identifier);
                    else
                        warning_message(NETLIST, module_instance_var_node->loc, "This function port %s is unused in module %s\n", module_instance_var_node->types.identifier, module_node->identifier_node->types.identifier);

                } else {
                    /* CMM - Check if this pin should be driven by the top level VCC or GND drivers	*/
                    if (strstr(full_name, ONE_VCC_CNS)) {
                        join_nets(verilog_netlist->one_net, (nnet_t*)input_nets_sc->data[sc_spot_input_old]);
                        free_nnet((nnet_t*)input_nets_sc->data[sc_spot_input_old]);
                        input_nets_sc->data[sc_spot_input_old] = (void*)verilog_netlist->one_net;
                    } else if (strstr(full_name, ZERO_GND_ZERO)) {
                        join_nets(verilog_netlist->zero_net, (nnet_t*)input_nets_sc->data[sc_spot_input_old]);
                        free_nnet((nnet_t*)input_nets_sc->data[sc_spot_input_old]);
                        input_nets_sc->data[sc_spot_input_old] = (void*)verilog_netlist->zero_net;
                    }
                    /* check if the instantiation pin exists. */
                    else if ((sc_spot_output = sc_lookup_string(output_nets_sc, full_name)) == -1) {
                        /* IF - no driver, then assume that it needs to be aliased to move up as an input */
                        if ((sc_spot_input_new = sc_lookup_string(input_nets_sc, full_name)) == -1) {
                            /* if this input is not yet used in this module then we'll add it */
                            sc_spot_input_new = sc_add_string(input_nets_sc, full_name);

                            /* copy the pin to the old spot */
                            input_nets_sc->data[sc_spot_input_new] = input_nets_sc->data[sc_spot_input_old];
                        } else {
                            /* already exists so we'll join the nets */
                            combine_nets((nnet_t*)input_nets_sc->data[sc_spot_input_old], (nnet_t*)input_nets_sc->data[sc_spot_input_new], verilog_netlist);
                            input_nets_sc->data[sc_spot_input_old] = NULL;
                        }
                    } else {
                        /* ELSE - we've found a matching net, so add this pin to the net */
                        nnet_t* net = (nnet_t*)output_nets_sc->data[sc_spot_output];
                        nnet_t* in_net = (nnet_t*)input_nets_sc->data[sc_spot_input_old];

                        if ((net != in_net) && (net->combined == true)) {
                            /* if they haven't been combined already, then join the inputs and output */
                            join_nets(net, in_net);
                            in_net = free_nnet(in_net);
                            /* since the driver net is deleted, copy the spot of the in_net over */
                            input_nets_sc->data[sc_spot_input_old] = (void*)net;
                        } else if ((net != in_net) && (net->combined == false)) {
                            /* if they haven't been combined already, then join the inputs and output */
                            combine_nets(net, in_net, verilog_netlist);
                            net = NULL;
                            /* since the driver net is deleted, copy the spot of the in_net over */
                            output_nets_sc->data[sc_spot_output] = (void*)in_net;
                        }
                    }
                }

                vtr::free(full_name);
                vtr::free(alias_name);
            } else if (i == 0 && module_var_node->types.variable.is_output) {
                /* ELSE IF - this is an output pin from the module.  We need to alias this output
                 * pin with it's calling name here so that everyone can see it at this level */
                char* name_of_module_instance_of_input = NULL;
                char* full_name = NULL;
                char* alias_name = NULL;

                /* make the new string for the alias name - has to be a identifier in the
                 * instantiated modules old names */
                if (port_size > 1) {
                    /* Get the name of the module instantiation pin */

                    //name_of_module_instance_of_input = get_name_of_pin_at_bit(module_instance_var_node, j, instance_name_prefix);

                    full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, NULL, -1);

                    //vtr::free(name_of_module_instance_of_input);

                    name_of_module_instance_of_input = get_name_of_var_declare_at_bit(module_var_node, j);

                    alias_name = make_full_ref_name(instance_name_prefix,
                                                    module_instance->identifier_node->types.identifier,
                                                    module_instance->children[0]->identifier_node->types.identifier, name_of_module_instance_of_input, -1);

                    vtr::free(name_of_module_instance_of_input);

                } else {
                    //oassert(j == 0);
                    /* Get the name of the module instantiation pin */

                    //name_of_module_instance_of_input = get_name_of_pin_at_bit(module_instance_var_node, -1, instance_name_prefix);

                    full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, NULL, -1);

                    //vtr::free(name_of_module_instance_of_input);

                    name_of_module_instance_of_input = get_name_of_var_declare_at_bit(module_var_node, 0);

                    alias_name = make_full_ref_name(instance_name_prefix,
                                                    module_instance->identifier_node->types.identifier,
                                                    module_instance->children[0]->identifier_node->types.identifier, name_of_module_instance_of_input, -1);

                    vtr::free(name_of_module_instance_of_input);
                }

                /* check if the instantiation pin exists. */
                if ((sc_spot_output = sc_lookup_string(output_nets_sc, alias_name)) == -1) {
                    error_message(NETLIST, module_var_node->loc,
                                  "This output (%s) must exist...must be an error\n", alias_name);
                }

                /* can already be there */
                sc_spot_input_new = sc_add_string(output_nets_sc, full_name);

                /* Copy over the initial value data from the net alias to the corresponding
                 * flip-flop node if one exists. This is necessary if an initial value is
                 * assigned on a higher-level module since the flip-flop node will have
                 * already been instantiated without any initial value. */

                npin_t* new_pin2;
                nnet_t* output_net = (nnet_t*)output_nets_sc->data[sc_spot_output];
                new_pin2 = allocate_npin();
                add_fanout_pin_to_net(output_net, new_pin2);
                add_pin_to_signal_list(return_list, new_pin2);
                nnet_t* input_new_net = (nnet_t*)output_nets_sc->data[sc_spot_input_new];

                for (int k = 0; k < output_net->num_driver_pins; k++) {
                    if (output_net->driver_pins[k] && output_net->driver_pins[k]->node && input_new_net) {
                        output_net->driver_pins[k]->node->initial_value = input_new_net->initial_value;
                    }
                }

                /* clean up input_new_net */
                if (!(input_new_net) || !(input_new_net->num_driver_pins))
                    input_new_net = free_nnet(input_new_net);

                /* add this alias for the net */
                output_nets_sc->data[sc_spot_input_new] = output_nets_sc->data[sc_spot_output];

                /* make the inplicit output list and hook up the outputs */
                vtr::free(full_name);
                vtr::free(alias_name);
            }
        }
    }

    return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function: connect_task_instantiation_and_alias)
 *-------------------------------------------------------------------------------------------*/

signal_list_t* connect_task_instantiation_and_alias(short PASS, ast_node_t* task_instance, char* instance_name_prefix, sc_hierarchy* local_ref) {
    signal_list_t* return_list = init_signal_list();

    long i;
    int j;
    //signal_list_t *aux_node = NULL;
    ast_node_t* task_node = NULL;
    ast_node_t* task_list = NULL;
    ast_node_t* task_instance_list = task_instance->children[0]->children[0]; // TASK_INSTANCE->TASK_NAMED_INSTANCE->MODULE_CONNECT_LIST(child[0])
    long sc_spot;
    long sc_spot_output;
    long sc_spot_input_old;
    long sc_spot_input_new;

    char* task_instance_name = task_instance->identifier_node->types.identifier;

    /* lookup the node of the task associated with this instantiated task */
    if ((sc_spot = sc_lookup_string(module_names_to_idx, task_instance_name)) == -1) {
        error_message(NETLIST, task_instance->loc,
                      "Can't find task %s\n", task_instance_name);
    }

    if (task_instance_name != task_instance->identifier_node->types.identifier)
        vtr::free(task_instance_name);

    task_node = (ast_node_t*)module_names_to_idx->data[sc_spot];
    task_list = task_node->children[0]; // TASK->VAR_DECLARE_LIST(child[0])

    if (task_list->num_children != task_instance_list->num_children) {
        error_message(NETLIST, task_instance->loc,
                      "Task instantiation (%s) and definition don't match in terms of ports\n",
                      task_instance->identifier_node->types.identifier);
    }

    //reordering if the variable is instantiated by name
    if (task_instance_list->num_children > 0 && task_instance_list->children[1]->identifier_node != NULL) {
        reorder_connections_from_name(task_list, task_instance_list, TASK);
    }

    for (i = 0; i < task_list->num_children; i++) {
        int port_size = 0;
        // VAR_DECLARE_LIST(child[i])->VAR_DECLARE_PORT(child[0])->VAR_DECLARE_input-or-output(child[0])
        ast_node_t* task_var_node = task_list->children[i];
        ast_node_t* task_instance_var_node = task_instance_list->children[i]->children[0];

        if (
            // skip inputs on pass 1
            ((PASS == INSTANTIATE_DRIVERS) && (task_var_node->types.variable.is_input))
            // skip outputs on pass 2
            || ((PASS == ALIAS_INPUTS) && (task_var_node->types.variable.is_output))) {
            continue;
        }

        /* IF the designer users port names then make sure they line up */
        // TODO: This code may need to be moved down the line
        if (i > 0 && task_instance_list->children[i]->identifier_node != NULL) {
            if (strcmp(task_instance_list->children[i]->identifier_node->types.identifier, task_var_node->identifier_node->types.identifier) != 0) {
                // TODO: This error message may not be correct. If assigning by name, the order should not matter
                error_message(NETLIST, task_var_node->loc,
                              "This task entry does not match up correctly (%s != %s).  Odin expects the order of ports to be the same\n",
                              task_instance_list->children[i]->identifier_node->types.identifier,
                              task_var_node->identifier_node->types.identifier);
            }
        }

        /* calculate the port details */
        if (task_var_node->children[0] == NULL) {
            port_size = 1;
        } else if (task_var_node->children[2] == NULL) {
            char* task_name = make_full_ref_name(instance_name_prefix,
                                                 // task_name
                                                 task_instance->identifier_node->types.identifier,
                                                 // instance name
                                                 task_instance->children[0]->identifier_node->types.identifier,
                                                 NULL, -1);

            ast_node_t* node1 = task_var_node->children[0];
            ast_node_t* node2 = task_var_node->children[1];

            vtr::free(task_name);
            oassert(node2->type == NUMBERS && node1->type == NUMBERS);
            /* assume all arrays declared [largest:smallest] */
            oassert(node2->types.vnumber->get_value() <= node1->types.vnumber->get_value());
            port_size = node1->types.vnumber->get_value() - node2->types.vnumber->get_value() + 1;
        } else if (task_var_node->children[4] == NULL) {
            char* task_name = make_full_ref_name(instance_name_prefix,
                                                 // task_name
                                                 task_instance->identifier_node->types.identifier,
                                                 // instance name
                                                 task_instance->children[0]->identifier_node->types.identifier,
                                                 NULL, -1);

            ast_node_t* node1 = task_var_node->children[0];
            ast_node_t* node2 = task_var_node->children[1];
            ast_node_t* node3 = task_var_node->children[2];
            ast_node_t* node4 = task_var_node->children[3];

            vtr::free(task_name);
            oassert(node2->type == NUMBERS && node1->type == NUMBERS && node3->type == NUMBERS && node4->type == NUMBERS);
            /* assume all arrays declared [largest:smallest] */
            oassert(node2->types.vnumber->get_value() <= node1->types.vnumber->get_value());
            oassert(node4->types.vnumber->get_value() <= node3->types.vnumber->get_value());
            port_size = node1->types.vnumber->get_value() * node3->types.vnumber->get_value() - 1;
        }

        //-----------------------------------------------------------------------------------
        else if (task_var_node->children[4] != NULL) {
            /* Implicit memory */
            error_message(NETLIST, task_var_node->children[4]->loc, "%s\n", "Unhandled implicit memory in connect_task_instantiation_and_alias");
        }
        if (task_var_node->types.variable.is_input) {
            for (j = 0; j < port_size; j++) {
                /* IF - this spot in the task list is an input, then we need to find it in the
                 * string cache (as its old name), check if the new_name (the instantiation name)
                 * is already a driver at this level.  IF it is a driver then we can hook the two up.
                 * IF it's not we need to alias the pin name as it is used here since it
                 * must be driven at a higher level of hierarchy in the tree of tasks */
                char* name_of_task_instance_of_input = NULL;
                char* full_name = NULL;
                char* alias_name = NULL;

                if (port_size > 1) {
                    /* Get the name of the task instantiation pin */
                    name_of_task_instance_of_input = get_name_of_pin_at_bit(task_instance_var_node, j, instance_name_prefix, local_ref);
                    full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_task_instance_of_input, -1);
                    vtr::free(name_of_task_instance_of_input);

                    /* make the new string for the alias name - has to be a identifier in the instantiated tasks old names */
                    name_of_task_instance_of_input = get_name_of_var_declare_at_bit(task_var_node, j);
                    alias_name = make_full_ref_name(instance_name_prefix,
                                                    task_instance->identifier_node->types.identifier,
                                                    task_instance->children[0]->identifier_node->types.identifier,
                                                    name_of_task_instance_of_input, -1);

                    vtr::free(name_of_task_instance_of_input);
                } else {
                    oassert(j == 0);

                    /* Get the name of the task instantiation pin */
                    name_of_task_instance_of_input = get_name_of_pin_at_bit(task_instance_var_node, -1, instance_name_prefix, local_ref);

                    full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_task_instance_of_input, -1);
                    vtr::free(name_of_task_instance_of_input);

                    name_of_task_instance_of_input = get_name_of_var_declare_at_bit(task_var_node, 0);
                    alias_name = make_full_ref_name(instance_name_prefix,
                                                    task_instance->identifier_node->types.identifier,
                                                    task_instance->children[0]->identifier_node->types.identifier,
                                                    name_of_task_instance_of_input, -1);

                    vtr::free(name_of_task_instance_of_input);
                }

                /* search for the old_input name */
                if ((sc_spot_input_old = sc_lookup_string(input_nets_sc, alias_name)) == -1) {
                    if (port_size > 1)
                        warning_message(NETLIST, task_instance_var_node->loc, "This task port %s[%d] is unused in module %s\n", task_instance_var_node->types.identifier, j, task_node->identifier_node->types.identifier);
                    else
                        warning_message(NETLIST, task_instance_var_node->loc, "This task port %s is unused in module %s\n", task_instance_var_node->types.identifier, task_node->identifier_node->types.identifier);

                } else {
                    /* CMM - Check if this pin should be driven by the top level VCC or GND drivers	*/
                    if (strstr(full_name, ONE_VCC_CNS)) {
                        join_nets(verilog_netlist->one_net, (nnet_t*)input_nets_sc->data[sc_spot_input_old]);
                        free_nnet((nnet_t*)input_nets_sc->data[sc_spot_input_old]);
                        input_nets_sc->data[sc_spot_input_old] = (void*)verilog_netlist->one_net;
                    } else if (strstr(full_name, ZERO_GND_ZERO)) {
                        join_nets(verilog_netlist->zero_net, (nnet_t*)input_nets_sc->data[sc_spot_input_old]);
                        free_nnet((nnet_t*)input_nets_sc->data[sc_spot_input_old]);
                        input_nets_sc->data[sc_spot_input_old] = (void*)verilog_netlist->zero_net;
                    }
                    /* check if the instantiation pin exists. */
                    else if ((sc_spot_output = sc_lookup_string(output_nets_sc, full_name)) == -1) {
                        /* IF - no driver, then assume that it needs to be aliased to move up as an input */
                        if ((sc_spot_input_new = sc_lookup_string(input_nets_sc, full_name)) == -1) {
                            /* if this input is not yet used in this task then we'll add it */
                            sc_spot_input_new = sc_add_string(input_nets_sc, full_name);

                            /* copy the pin to the old spot */
                            input_nets_sc->data[sc_spot_input_new] = input_nets_sc->data[sc_spot_input_old];
                        } else {
                            /* already exists so we'll join the nets */
                            combine_nets((nnet_t*)input_nets_sc->data[sc_spot_input_old], (nnet_t*)input_nets_sc->data[sc_spot_input_new], verilog_netlist);
                            input_nets_sc->data[sc_spot_input_old] = NULL;
                        }
                    } else {
                        /* ELSE - we've found a matching net, so add this pin to the net */
                        nnet_t* net = (nnet_t*)output_nets_sc->data[sc_spot_output];
                        nnet_t* in_net = (nnet_t*)input_nets_sc->data[sc_spot_input_old];

                        if ((net != in_net) && (net->combined == true)) {
                            /* if they haven't been combined already, then join the inputs and output */
                            join_nets(net, in_net);
                            in_net = free_nnet(in_net);
                            /* since the driver net is deleted, copy the spot of the in_net over */
                            input_nets_sc->data[sc_spot_input_old] = (void*)net;
                        } else if ((net != in_net) && (net->combined == false)) {
                            /* if they haven't been combined already, then join the inputs and output */
                            combine_nets(net, in_net, verilog_netlist);
                            net = NULL;
                            /* since the driver net is deleted, copy the spot of the in_net over */
                            output_nets_sc->data[sc_spot_output] = (void*)in_net;
                        }
                    }
                }

                vtr::free(full_name);
                vtr::free(alias_name);
            }
        } else if (task_var_node->types.variable.is_output) {
            for (j = 0; j < port_size; j++) {
                /* ELSE IF - this is an output pin from the module.  We need to alias this output
                 * pin with it's calling name here so that everyone can see it at this level */
                char* name_of_task_instance_of_input = NULL;
                char* full_name = NULL;
                char* alias_name = NULL;

                /* make the new string for the alias name - has to be a identifier in the
                 * instantiated modules old names */
                if (port_size > 1) {
                    /* Get the name of the module instantiation pin */
                    name_of_task_instance_of_input = get_name_of_pin_at_bit(task_instance_var_node, j, instance_name_prefix, local_ref);
                    full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_task_instance_of_input, -1);
                    vtr::free(name_of_task_instance_of_input);

                    name_of_task_instance_of_input = get_name_of_var_declare_at_bit(task_var_node, j);
                    alias_name = make_full_ref_name(instance_name_prefix,
                                                    task_instance->identifier_node->types.identifier,
                                                    task_instance->children[0]->identifier_node->types.identifier, name_of_task_instance_of_input, -1);
                    vtr::free(name_of_task_instance_of_input);
                } else {
                    oassert(j == 0);
                    /* Get the name of the module instantiation pin */
                    name_of_task_instance_of_input = get_name_of_pin_at_bit(task_instance_var_node, -1, instance_name_prefix, local_ref);
                    full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_task_instance_of_input, -1);
                    vtr::free(name_of_task_instance_of_input);
                    name_of_task_instance_of_input = NULL;

                    name_of_task_instance_of_input = get_name_of_var_declare_at_bit(task_var_node, 0);
                    alias_name = make_full_ref_name(instance_name_prefix,
                                                    task_instance->identifier_node->types.identifier,
                                                    task_instance->children[0]->identifier_node->types.identifier, name_of_task_instance_of_input, -1);
                    vtr::free(name_of_task_instance_of_input);
                }

                /* check if the instantiation pin exists. */
                if ((sc_spot_output = sc_lookup_string(output_nets_sc, alias_name)) == -1) {
                    error_message(NETLIST, task_var_node->loc,
                                  "This output (%s) must exist...must be an error\n", alias_name);
                }

                sc_spot_input_new = sc_add_string(output_nets_sc, full_name);

                /* Copy over the initial value data from the net alias to the corresponding
                 * flip-flop node if one exists. This is necessary if an initial value is
                 * assigned on a higher-level module since the flip-flop node will have
                 * already been instantiated without any initial value. */
                name_of_task_instance_of_input = get_name_of_pin_at_bit(task_instance_var_node, j, instance_name_prefix, local_ref);
                char* pin_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_task_instance_of_input, -1);

                nnet_t* output_net = (nnet_t*)output_nets_sc->data[sc_spot_output];
                npin_t* new_pin2 = allocate_npin();
                new_pin2->name = pin_name;
                add_fanout_pin_to_net(output_net, new_pin2);
                add_pin_to_signal_list(return_list, new_pin2);

                vtr::free(full_name);
                vtr::free(alias_name);
                vtr::free(name_of_task_instance_of_input);
            }
        }
    }
    return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function: create_pins)
 * 	Create pin creates the pins representing this naming instance, adds them to the input
 * 	nets list (if not already there), checks if a driver already exists (and hooks input
 * 	to output if needed), and adds the pin to the list.
 * 	Note: only for input paths...
 *-------------------------------------------------------------------------------------------*/
signal_list_t* create_pins(ast_node_t* var_declare, char* name, char* instance_name_prefix, sc_hierarchy* local_ref) {
    int i;
    signal_list_t* return_sig_list = init_signal_list();
    long sc_spot;
    long sc_spot_output;
    npin_t* new_pin;
    nnet_t* new_in_net;
    char_list_t* pin_lists = NULL;

    oassert((!name || !var_declare)
            && "Invalid state or internal error");
    if (name == NULL) {
        /* get all the pins */
        pin_lists = get_name_of_pins_with_prefix(var_declare, instance_name_prefix, local_ref);
    } else if (var_declare == NULL) {
        /* if you have the name and just want a pin then use this method */
        pin_lists = (char_list_t*)vtr::malloc(sizeof(char_list_t) * 1);
        pin_lists->strings = (char**)vtr::malloc(sizeof(char*));
        pin_lists->strings[0] = name;
        pin_lists->num_strings = 1;
    }

    for (i = 0; pin_lists && i < pin_lists->num_strings; i++) {
        new_pin = allocate_npin();
        new_pin->name = pin_lists->strings[i];

        /* check if the instantiation pin exists. */
        if (strstr(pin_lists->strings[i], ONE_VCC_CNS)) {
            add_fanout_pin_to_net(verilog_netlist->one_net, new_pin);
            sc_spot = sc_add_string(input_nets_sc, pin_lists->strings[i]);
            input_nets_sc->data[sc_spot] = (void*)verilog_netlist->one_net;
        } else if (strstr(pin_lists->strings[i], ZERO_GND_ZERO)) {
            add_fanout_pin_to_net(verilog_netlist->zero_net, new_pin);
            sc_spot = sc_add_string(input_nets_sc, pin_lists->strings[i]);
            input_nets_sc->data[sc_spot] = (void*)verilog_netlist->zero_net;
        } else {
            /* search for the input name  if already exists...needs to be added to
             * string cache in case it's an input pin */
            sc_spot = sc_lookup_string(input_nets_sc, pin_lists->strings[i]);
            sc_spot_output = sc_lookup_string(output_nets_sc, pin_lists->strings[i]);
            if (sc_spot != -1 && sc_spot_output != -1) {
                /* store the pin in this net */
                add_fanout_pin_to_net((nnet_t*)input_nets_sc->data[sc_spot], new_pin);

                nnet_t* net = (nnet_t*)output_nets_sc->data[sc_spot_output];

                if ((net != (nnet_t*)input_nets_sc->data[sc_spot]) && net->combined) {
                    /* IF - the input and output nets don't match, then they need to be joined */
                    join_nets(net, (nnet_t*)input_nets_sc->data[sc_spot]);
                    free_nnet((nnet_t*)input_nets_sc->data[sc_spot]);
                    /* since the driver net is deleted, copy the spot of the in_net over */
                    input_nets_sc->data[sc_spot] = (void*)net;
                } else if ((net != (nnet_t*)input_nets_sc->data[sc_spot]) && !net->combined) {
                    /* IF - the input and output nets don't match, then they need to be joined */

                    combine_nets(net, (nnet_t*)input_nets_sc->data[sc_spot], verilog_netlist);
                    net = NULL;
                    /* since the driver net is deleted, copy the spot of the in_net over */
                    output_nets_sc->data[sc_spot_output] = (void*)input_nets_sc->data[sc_spot];
                }
            } else if (sc_spot_output != -1) {
                sc_spot = sc_add_string(input_nets_sc, pin_lists->strings[i]);
                input_nets_sc->data[sc_spot] = output_nets_sc->data[sc_spot_output];
                add_fanout_pin_to_net((nnet_t*)input_nets_sc->data[sc_spot], new_pin);
            } else {
                if (sc_spot == -1) {
                    new_in_net = allocate_nnet();
                    sc_spot = sc_add_string(input_nets_sc, pin_lists->strings[i]);
                    input_nets_sc->data[sc_spot] = (void*)new_in_net;
                }

                /* store the pin in this net */
                add_fanout_pin_to_net((nnet_t*)input_nets_sc->data[sc_spot], new_pin);
            }
        }

        /* add the pin now stored in the string chache to the returned signal list */
        add_pin_to_signal_list(return_sig_list, new_pin);
    }

    if (pin_lists != NULL) {
        vtr::free(pin_lists->strings);
        vtr::free(pin_lists);
    }
    return return_sig_list;
}

/*---------------------------------------------------------------------------------------------
 * (function: create_output_pin)
 * 	Create OUTPUT pin creates a pin representing this naming isntance, adds it to the input
 * 	nets list (if not already there) and adds the pin to the list.
 * 	Note: only for output drivers...
 *-------------------------------------------------------------------------------------------*/
signal_list_t* create_output_pin(ast_node_t* var_declare, char* instance_name_prefix, sc_hierarchy* local_ref) {
    char* name_of_pin;
    char* full_name;
    signal_list_t* return_sig_list = init_signal_list();
    npin_t* new_pin;

    /* get the name of the pin */
    name_of_pin = get_name_of_pin_at_bit(var_declare, -1, instance_name_prefix, local_ref);
    full_name = make_full_ref_name(instance_name_prefix, NULL, NULL, name_of_pin, -1);
    vtr::free(name_of_pin);

    new_pin = allocate_npin();
    new_pin->name = full_name;

    add_pin_to_signal_list(return_sig_list, new_pin);

    return return_sig_list;
}

/*---------------------------------------------------------------------------------------------
 * (function: assignment_alias)
 *-------------------------------------------------------------------------------------------*/
signal_list_t* assignment_alias(ast_node_t* assignment, char* instance_name_prefix, sc_hierarchy* local_ref, long assignment_size) {
    ast_node_t* left = assignment->children[0];
    ast_node_t* right = assignment->children[1];

    implicit_memory* left_memory = lookup_implicit_memory_reference_ast(instance_name_prefix, left);
    implicit_memory* right_memory = lookup_implicit_memory_reference_ast(instance_name_prefix, right);

    signal_list_t* in_1 = NULL;

    signal_list_t* right_outputs = NULL;
    signal_list_t* right_inputs = NULL;

    /* process the signal for the input gate */
    if (right_memory) {
        if (!is_valid_implicit_memory_reference_ast(instance_name_prefix, right))
            error_message(NETLIST, assignment->loc,
                          "Invalid addressing mode for implicit memory %s.\n", right_memory->name);

        signal_list_t* address = netlist_expand_ast_of_module(&(right->children[0]), instance_name_prefix, local_ref, assignment_size);
        // Pad/shrink the address to the depth of the memory.

        if (address->count == 0) {
            warning_message(NETLIST, assignment->loc,
                            "indexing into memory with %s has address of length 0, skipping memory read", instance_name_prefix);
        } else {
            if (address->count != right_memory->addr_width) {
                if (address->count > right_memory->addr_width) {
                    std::string unused_pins_name = "";
                    for (long i = right_memory->addr_width; i < address->count; i++) {
                        if (address->pins && address->pins[i] && address->pins[i]->name)
                            unused_pins_name = unused_pins_name + " " + address->pins[i]->name;
                    }
                    warning_message(NETLIST, assignment->loc,
                                    "indexing into memory with %s has larger input than memory. Unused pins: %s", instance_name_prefix, unused_pins_name.c_str());
                } else {
                    warning_message(NETLIST, assignment->loc,
                                    "indexing into memory with %s has smaller input than memory. Padding with GND", instance_name_prefix);
                }

                while (address->count < right_memory->addr_width)
                    add_pin_to_signal_list(address, get_zero_pin(verilog_netlist));

                address->count = right_memory->addr_width;
            }

            add_input_port_to_implicit_memory(right_memory, address, "addr1");
            // Right inputs are the inputs to the memory. This will contain the address only.
            right_inputs = init_signal_list();
            char* name = right->identifier_node->types.identifier;
            for (int i = 0; i < address->count; i++) {
                npin_t* pin = address->pins[i];
                if (pin->name)
                    vtr::free(pin->name);
                pin->name = make_full_ref_name(instance_name_prefix, NULL, NULL, name, i);
                add_pin_to_signal_list(right_inputs, pin);
            }
            free_signal_list(address);

            // Right outputs will be the outputs of the memory. They will be
            // treated the same as the outputs from the RHS of any assignment.
            right_outputs = init_signal_list();
            signal_list_t* outputs = init_signal_list();
            for (int i = 0; i < right_memory->data_width; i++) {
                npin_t* pin = allocate_npin();
                add_pin_to_signal_list(outputs, pin);
                pin->name = make_full_ref_name("", NULL, NULL, right_memory->node->name, i);
                nnet_t* net = allocate_nnet();
                add_driver_pin_to_net(net, pin);
                pin = allocate_npin();
                add_fanout_pin_to_net(net, pin);
                //right_outputs->pins[i] = pin;
                add_pin_to_signal_list(right_outputs, pin);
            }
            add_output_port_to_implicit_memory(right_memory, outputs, "out1");
            free_signal_list(outputs);
        }

    } else {
        in_1 = netlist_expand_ast_of_module(&(assignment->children[1]), instance_name_prefix, local_ref, assignment_size);
        oassert(in_1 != NULL);
        right = assignment->children[1];
    }

    char_list_t* out_list;

    if (left_memory) {
        if (!is_valid_implicit_memory_reference_ast(instance_name_prefix, left))
            error_message(NETLIST, assignment->loc,
                          "Invalid addressing mode for implicit memory %s.\n", left_memory->name);

        // A memory can only be written from a clocked rising edge block.
        if (type_of_circuit != SEQUENTIAL) {
            out_list = NULL;
            error_message(NETLIST, assignment->loc, "%s",
                          "Assignment to implicit memories is only supported within sequential circuits.\n");
        } else {
            // Make sure the memory is addressed.
            signal_list_t* address = netlist_expand_ast_of_module(&(left->children[0]), instance_name_prefix, local_ref, assignment_size);

            // Pad/shrink the address to the depth of the memory.
            if (address->count == 0) {
                warning_message(NETLIST, assignment->loc,
                                "indexing into memory with %s has address of length 0, skipping memory write", instance_name_prefix);
            } else {
                if (address->count != left_memory->addr_width) {
                    if (address->count > left_memory->addr_width) {
                        std::string unused_pins_name = "";
                        for (long i = left_memory->addr_width; i < address->count; i++) {
                            if (address->pins && address->pins[i] && address->pins[i]->name)
                                unused_pins_name = unused_pins_name + " " + address->pins[i]->name;
                        }
                        warning_message(NETLIST, assignment->loc,
                                        "indexing into memory with %s has larger input than memory. Unused pins: %s", instance_name_prefix, unused_pins_name.c_str());
                    } else {
                        warning_message(NETLIST, assignment->loc,
                                        "indexing into memory with %s has smaller input than memory. Padding with GND", instance_name_prefix);
                    }

                    while (address->count < left_memory->addr_width)
                        add_pin_to_signal_list(address, get_zero_pin(verilog_netlist));

                    address->count = left_memory->addr_width;
                }

                add_input_port_to_implicit_memory(left_memory, address, "addr2");

                signal_list_t* data;
                if (right_memory)
                    data = right_outputs;
                else
                    data = in_1;

                // Pad/shrink the data to the width of the memory.
                if (data) {
                    while (data->count < left_memory->data_width)
                        add_pin_to_signal_list(data, get_zero_pin(verilog_netlist));

                    data->count = left_memory->data_width;

                    add_input_port_to_implicit_memory(left_memory, data, "data2");

                    signal_list_t* we = init_signal_list();
                    add_pin_to_signal_list(we, get_one_pin(verilog_netlist));
                    add_input_port_to_implicit_memory(left_memory, we, "we2");

                    in_1 = init_signal_list();
                    char* name = left->identifier_node->types.identifier;
                    int i;
                    int pin_index = left_memory->data_width + left_memory->data_width + left_memory->addr_width + 2;
                    for (i = 0; i < address->count; i++) {
                        npin_t* pin = address->pins[i];
                        if (pin->name)
                            vtr::free(pin->name);
                        pin->name = make_full_ref_name(instance_name_prefix, NULL, NULL, name, pin_index++);
                        add_pin_to_signal_list(in_1, pin);
                    }
                    free_signal_list(address);

                    for (i = 0; i < data->count; i++) {
                        npin_t* pin = data->pins[i];
                        if (pin->name)
                            vtr::free(pin->name);
                        pin->name = make_full_ref_name(instance_name_prefix, NULL, NULL, name, pin_index++);
                        add_pin_to_signal_list(in_1, pin);
                    }
                    free_signal_list(data);

                    for (i = 0; i < we->count; i++) {
                        npin_t* pin = we->pins[i];
                        if (pin->name)
                            vtr::free(pin->name);
                        pin->name = make_full_ref_name(instance_name_prefix, NULL, NULL, name, pin_index++);
                        add_pin_to_signal_list(in_1, pin);
                    }
                    free_signal_list(we);

                    out_list = NULL;
                }
            }
        }
    } else {
        out_list = get_name_of_pins_with_prefix(left, instance_name_prefix, local_ref);
    }

    signal_list_t* return_list;
    return_list = init_signal_list();

    if (!right_memory && !left_memory) {
        int output_size = alias_output_assign_pins_to_inputs(out_list, in_1, assignment);

        if (in_1 && output_size < in_1->count) {
            /* need to shrink the output list */
            int i;
            for (i = 0; i < output_size; i++) {
                add_pin_to_signal_list(return_list, in_1->pins[i]);
            }
            free_signal_list(in_1);
        } else {
            free_signal_list(return_list);
            return_list = in_1;

            // /* TODO must check if output_size > in_1->count... pad accordingly */
            // if (output_size > return_list->count)
            // {
            // 	int i;
            // 	for (i = return_list->count; i < output_size; i++)
            // 	{
            // 		add_pin_to_signal_list(return_list, return_list->pins[i-1]);
            // 	}
            // }
        }

        vtr::free(out_list->strings);
        vtr::free(out_list);
    } else {
        if (right_memory) {
            // Register inputs for later assignment directly to the memory.
            oassert(right_inputs);
            int i;
            for (i = 0; i < right_inputs->count; i++) {
                add_pin_to_signal_list(return_list, right_inputs->pins[i]);
                register_implicit_memory_input(right_inputs->pins[i]->name, right_memory);
            }
            free_signal_list(right_inputs);

            // Alias the outputs like a regular assignment if the thing on the left isn't a memory.
            if (!left_memory) {
                int output_size = alias_output_assign_pins_to_inputs(out_list, right_outputs, assignment);
                for (i = 0; i < output_size; i++) {
                    add_pin_to_signal_list(return_list, right_outputs->pins[i]);
                }
                free_signal_list(right_outputs);
                vtr::free(out_list->strings);
                vtr::free(out_list);
            }
        }

        if (left_memory) {
            // Index all inputs to the left memory for implicit memory direct assignment.
            oassert(in_1);
            int i;
            for (i = 0; i < in_1->count; i++) {
                add_pin_to_signal_list(return_list, in_1->pins[i]);
                register_implicit_memory_input(in_1->pins[i]->name, left_memory);
            }
            free_signal_list(in_1);
        }
    }

    return return_list;
}

void define_latchs_initial_value_inside_initial_statement(ast_node_t* initial_node, sc_hierarchy* local_ref) {
    long i;
    long sc_spot;
    STRING_CACHE* local_symbol_table_sc = local_ref->local_symbol_table_sc;
    ast_node_t** local_symbol_table = local_ref->local_symbol_table;

    for (i = 0; i < initial_node->num_children; i++) {
        /*check to see if, for each member of the initial block, if the assignment to a given variable is a number and not
         * a complex statement*/
        if ((initial_node->children[i]->type == BLOCKING_STATEMENT || initial_node->children[i]->type == NON_BLOCKING_STATEMENT)
            && initial_node->children[i]->children[1]->type == NUMBERS) {
            //Find corresponding register, set it's members to reflect initialization.
            if (initial_node->children[i]) {
                /*if the identifier we found in the table matches the identifier of our blocking statement*/
                sc_spot = sc_lookup_string(local_symbol_table_sc, initial_node->children[i]->children[0]->types.identifier);
                if (sc_spot == -1) {
                    warning_message(NETLIST, initial_node->children[i]->loc, "Register [%s] used in initial block is not declared.\n", initial_node->children[i]->identifier_node->types.identifier);
                } else {
                    local_symbol_table[sc_spot]->types.variable.initial_value = get_init_value(initial_node->children[i]->children[1]);
                }
            }
        }
    }
}

/*---------------------------------------------------------------------------------------------
 * (function: terminate_registered_assignment)
 *-------------------------------------------------------------------------------------------*/
void terminate_registered_assignment(ast_node_t* always_node, signal_list_t* assignment, signal_list_t* potential_clocks, sc_hierarchy* local_ref) {
    oassert(potential_clocks != NULL);

    npin_t** list_dependence_pin = (npin_t**)vtr::calloc(assignment->count, sizeof(npin_t*));
    ids* list_dependence_type = (ids*)vtr::calloc(assignment->count, sizeof(ids));
    /* figure out which one is the clock */
    if (local_clock_found == false) {
        int i;
        for (i = 0; i < potential_clocks->count; i++) {
            nnet_t* temp_net = NULL;
            /* searching for the clock with no net */
            long sc_spot = sc_lookup_string(output_nets_sc, potential_clocks->pins[i]->name);
            if (sc_spot == -1) {
                sc_spot = sc_lookup_string(input_nets_sc, potential_clocks->pins[i]->name);
                if (sc_spot == -1) {
                    error_message(NETLIST, always_node->loc,
                                  "Sensitivity list element (%s) is not a driver or net ... must be\n", potential_clocks->pins[i]->name);
                }
                temp_net = (nnet_t*)input_nets_sc->data[sc_spot];
            } else {
                temp_net = (nnet_t*)output_nets_sc->data[sc_spot];
            }

            if ((((temp_net->num_fanout_pins == 1) && (temp_net->fanout_pins[0]->node == NULL)) || (temp_net->num_fanout_pins == 0))
                && (local_clock_found == true)) {
                error_message(NETLIST, always_node->loc,
                              "Suspected second clock (%s).  In a sequential sensitivity list, Odin expects the "
                              "clock not to drive anything and any other signals in this list to drive stuff.  "
                              "For example, a reset in the sensitivy list has to be hooked up to something in the always block.\n",
                              potential_clocks->pins[i]->name);
            } else if (temp_net->num_fanout_pins == 0) {
                /* If this element is in the sensitivity list and doesn't drive anything it's the clock */
                local_clock_found = true;
                local_clock_idx = i;
            } else if ((temp_net->num_fanout_pins == 1) && (temp_net->fanout_pins[0]->node == NULL)) {
                /* If this element is in the sensitivity list and doesn't drive anything it's the clock */
                local_clock_found = true;
                local_clock_idx = i;
            }
        }
    }

    nnet_t* clock_net = potential_clocks->pins[local_clock_idx]->net;

    signal_list_t* memory_inputs = init_signal_list();
    char* ref_string;
    int i, j;
    for (i = 0; i < assignment->count; i++) {
        npin_t* pin = assignment->pins[i];
        implicit_memory* memory = lookup_implicit_memory_input(pin->name);
        if (memory) {
            add_pin_to_signal_list(memory_inputs, pin);
        } else {
            /* look up the net */
            int sc_spot = sc_lookup_string(output_nets_sc, pin->name);
            if (sc_spot == -1) {
                error_message(NETLIST, always_node->loc,
                              "Assignment is missing driver (%s)\n", pin->name);
            }
            nnet_t* net = (nnet_t*)output_nets_sc->data[sc_spot];
            //looking for dependence according to with the type of statement (non-blocking or blocking)
            list_dependence_pin[i] = (pin);
            if (pin->node)
                list_dependence_type[i] = pin->node->related_ast_node->type;

            /* clean up non-blocking */
            if (pin->node && pin->node->related_ast_node->type == NON_BLOCKING_STATEMENT) {
                pin->node = free_nnode(pin->node);
            }

            /* HERE create the ff node and hookup everything */
            nnode_t* ff_node = allocate_nnode(always_node->loc);
            ff_node->related_ast_node = always_node;

            ff_node->type = FF_NODE;
            ff_node->edge_type = potential_clocks->pins[local_clock_idx]->sensitivity;
            /* create the unique name for this gate */
            //ff_node->name = node_name(ff_node, instance_name_prefix);
            /* Name the flipflop based on the name of its output pin */
            const char* ff_base_name = node_name_based_on_op(ff_node);
            ff_node->name = (char*)vtr::malloc(sizeof(char) * (strlen(pin->name) + strlen(ff_base_name) + 2));
            odin_sprintf(ff_node->name, "%s_%s", pin->name, ff_base_name);

            /* Copy over the initial value information from the net */
            ref_string = (char*)vtr::calloc(strlen(pin->name) + 100, sizeof(char));
            strcpy(ref_string, pin->name);
            strcat(ref_string, "_latch_initial_value");

            sc_spot = sc_lookup_string(local_ref->local_symbol_table_sc, ref_string);
            if (sc_spot == -1) {
                sc_spot = sc_add_string(local_ref->local_symbol_table_sc, ref_string);
                local_ref->local_symbol_table_sc->data[sc_spot] = (void*)net->initial_value;
            }
            init_value_e init_value = (init_value_e)(uintptr_t)local_ref->local_symbol_table_sc->data[sc_spot];
            oassert(ff_node->initial_value == init_value_e::undefined || ff_node->initial_value == init_value);
            ff_node->initial_value = init_value;

            /* free the reference string */
            vtr::free(ref_string);

            /* allocate the pins needed */
            allocate_more_input_pins(ff_node, 2);
            add_input_port_information(ff_node, 1);
            allocate_more_output_pins(ff_node, 1);
            add_output_port_information(ff_node, 1);

            /* add the clock to the flip_flop */
            /* add a fanout pin */
            npin_t* fanout_pin_of_clock = allocate_npin();
            add_fanout_pin_to_net(clock_net, fanout_pin_of_clock);
            add_input_pin_to_node(ff_node, fanout_pin_of_clock, 1);

            /* hookup the driver pin (the in_1) to to this net (the lookup) */
            add_input_pin_to_node(ff_node, pin, 0);

            /* finally hookup the output pin of the flip flop to the orginal driver net */
            npin_t* ff_output_pin = allocate_npin();
            add_output_pin_to_node(ff_node, ff_output_pin, 0);

            add_driver_pin_to_net(net, ff_output_pin);

            verilog_netlist->ff_nodes = (nnode_t**)vtr::realloc(verilog_netlist->ff_nodes, sizeof(nnode_t*) * (verilog_netlist->num_ff_nodes + 1));
            verilog_netlist->ff_nodes[verilog_netlist->num_ff_nodes] = ff_node;
            verilog_netlist->num_ff_nodes++;
        }
    }

    for (i = 0; i < assignment->count; i++) {
        npin_t* pin = assignment->pins[i];
        int dependence_variable_position = -1;

        if (pin->net->num_driver_pins) {
            ref_string = pin->net->driver_pins[0]->node->name;

            for (j = i - 1; j >= 0; j--) {
                if (list_dependence_pin[j] && list_dependence_pin[j]->net->num_driver_pins && list_dependence_type[j] == BLOCKING_STATEMENT && strcmp(ref_string, assignment->pins[j]->node->name) == 0) {
                    if (list_dependence_pin[j]->net->num_driver_pins > 1 || pin->net->num_driver_pins > 1) {
                        error_message(NETLIST, unknown_location, "%s", "Multiple registered assignments to the same variable not supported for this use case");
                    }
                    dependence_variable_position = j;
                    ref_string = list_dependence_pin[j]->net->driver_pins[0]->node->name;
                }
            }

            if (dependence_variable_position > -1) {
                pin->net = list_dependence_pin[dependence_variable_position]->net;
            }
        }
    }
    vtr::free(list_dependence_pin);
    vtr::free(list_dependence_type);
    for (i = 0; i < memory_inputs->count; i++) {
        npin_t* pin = memory_inputs->pins[i];
        if (pin->name) {
            implicit_memory* memory = lookup_implicit_memory_input(pin->name);

            if (memory) {
                nnode_t* node = memory->node;

                for (j = 0; j < node->num_input_pins; j++) {
                    npin_t* original_pin = node->input_pins[j];
                    if (original_pin->name && !strcmp(original_pin->name, pin->name)) {
                        pin->mapping = original_pin->mapping;
                        add_input_pin_to_node(node, pin, j);
                        break;
                    }
                }

                if (!memory->clock_added) {
                    npin_t* clock_pin = allocate_npin();
                    add_fanout_pin_to_net(clock_net, clock_pin);
                    signal_list_t* clock = init_signal_list();
                    add_pin_to_signal_list(clock, clock_pin);
                    add_input_port_to_implicit_memory(memory, clock, "clk");
                    free_signal_list(clock);
                    memory->clock_added = true;
                }
            }
        }
    }
    free_signal_list(memory_inputs);

    free_signal_list(assignment);
}

/*---------------------------------------------------------------------------------------------
 * (function: terminate_continuous_assignment)
 *-------------------------------------------------------------------------------------------*/
void terminate_continuous_assignment(ast_node_t* node, signal_list_t* assignment, char* instance_name_prefix) {
    signal_list_t* memory_inputs = init_signal_list();
    int i;
    for (i = 0; i < assignment->count; i++) {
        npin_t* pin = assignment->pins[i];
        implicit_memory* memory = lookup_implicit_memory_input(pin->name);
        if (memory) {
            add_pin_to_signal_list(memory_inputs, pin);
        } else {
            /* look up the net */
            long sc_spot = sc_lookup_string(output_nets_sc, pin->name);
            if (sc_spot == -1) {
                warning_message(NETLIST, node->loc,
                                "Assignment (%s) is missing driver\n", pin->name);
            } else {
                nnet_t* net = (nnet_t*)output_nets_sc->data[sc_spot];

                if (net->name == NULL)
                    net->name = pin->name;

                nnode_t* buf_node = allocate_nnode(node->loc);
                buf_node->type = BUF_NODE;
                /* create the unique name for this gate */
                buf_node->name = node_name(buf_node, instance_name_prefix);

                buf_node->related_ast_node = node;
                /* allocate the pins needed */
                allocate_more_input_pins(buf_node, 1);
                add_input_port_information(buf_node, 1);
                allocate_more_output_pins(buf_node, 1);
                add_output_port_information(buf_node, 1);

                npin_t* buf_input_pin = pin;
                add_input_pin_to_node(buf_node, buf_input_pin, 0);

                /* finally hookup the output pin of the buffer to the orginal driver net */
                npin_t* buf_output_pin = allocate_npin();
                add_output_pin_to_node(buf_node, buf_output_pin, 0);

                add_driver_pin_to_net(net, buf_output_pin);
            }
        }
    }

    for (i = 0; i < memory_inputs->count; i++) {
        npin_t* pin = memory_inputs->pins[i];
        if (pin->name) {
            implicit_memory* memory = lookup_implicit_memory_input(pin->name);
            if (memory) {
                nnode_t* node2 = memory->node;

                int j;
                for (j = 0; j < node2->num_input_pins; j++) {
                    npin_t* original_pin = node2->input_pins[j];
                    if (original_pin->name && !strcmp(original_pin->name, pin->name)) {
                        pin->mapping = original_pin->mapping;
                        add_input_pin_to_node(node2, pin, j);
                        break;
                    }
                }
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
int alias_output_assign_pins_to_inputs(char_list_t* output_list, signal_list_t* input_list, ast_node_t* node) {
    for (int i = 0; i < output_list->num_strings; i++) {
        if (i >= input_list->count) {
            if (global_args.all_warnings)
                warning_message(NETLIST, node->loc,
                                "More nets to drive than drivers, padding with ZEROs for driver %s\n", output_list->strings[i]);

            add_pin_to_signal_list(input_list, get_zero_pin(verilog_netlist));
        }

        if (input_list->pins[i]->name)
            vtr::free(input_list->pins[i]->name);

        input_list->pins[i]->name = output_list->strings[i];
        free_nnode(input_list->pins[i]->node);
    }

    if (global_args.all_warnings && output_list->num_strings < input_list->count)
        warning_message(NETLIST, node->loc, "%s",
                        "Alias: More driver pins than nets to drive: sometimes using decimal numbers causes this problem\n");

    return output_list->num_strings;
}

/*--------------------------------------------------------------------------
 * (function: create_gate)
 * 	This function creates a gate node in the netlist and hooks up the inputs
 * 	and outputs.
 *------------------------------------------------------------------------*/
signal_list_t* create_gate(ast_node_t* gate, char* instance_name_prefix, sc_hierarchy* local_ref, long assignment_size) {
    signal_list_t* out_1 = NULL;

    for (long j = 0; j < gate->children[0]->num_children; j++) {
        /* free the previous list since it was not the last itteration */
        if (out_1 != NULL) {
            free_signal_list(out_1);
            out_1 = NULL;
        }

        ast_node_t* gate_instance = gate->children[0]->children[j];

        if (gate_instance->children[2] == NULL) {
            /* IF one input gate */

            /* process the signal for the input gate */
            signal_list_t* in_1 = netlist_expand_ast_of_module(&(gate_instance->children[1]), instance_name_prefix, local_ref, assignment_size);
            /* process the signal for the input ga$te */
            out_1 = create_output_pin(gate_instance->children[0], instance_name_prefix, local_ref);

            oassert(in_1 != NULL);
            oassert(out_1 != NULL);

            /* create the node */
            nnode_t* gate_node = allocate_nnode(gate->loc);
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
        } else {
            /* ELSE 2 input gate */

            /* process the signal for the input gate */

            signal_list_t** in = (signal_list_t**)vtr::calloc(gate_instance->num_children - 1, sizeof(signal_list_t*));
            for (long i = 0; i < gate_instance->num_children - 1; i++) {
                in[i] = netlist_expand_ast_of_module(&(gate_instance->children[i + 1]), instance_name_prefix, local_ref, assignment_size);
            }

            /* process the signal for the input gate */
            out_1 = create_output_pin(gate_instance->children[0], instance_name_prefix, local_ref);

            for (long i = 0; i < gate_instance->num_children - 1; i++) {
                oassert((in[i] != NULL));
            }

            oassert((out_1 != NULL));

            /* create the node */
            nnode_t* gate_node = allocate_nnode(gate->loc);
            /* store all the relevant info */
            gate_node->related_ast_node = gate;
            gate_node->type = gate->types.operation.op;

            oassert(gate_node->type > 0);

            gate_node->name = node_name(gate_node, instance_name_prefix);

            /* allocate the needed pins */
            allocate_more_input_pins(gate_node, gate_instance->num_children - 1);
            for (long i = 0; i < gate_instance->num_children - 1; i++) {
                add_input_port_information(gate_node, 1);
            }

            allocate_more_output_pins(gate_node, 1);
            add_output_port_information(gate_node, 1);

            /* hookup the input pins */
            for (long i = 0; i < gate_instance->num_children - 1; i++) {
                hookup_input_pins_from_signal_list(gate_node, i, in[i], 0, 1, verilog_netlist);
            }

            /* hookup the output pins */
            hookup_output_pins_from_signal_list(gate_node, 0, out_1, 0, 1);

            for (long i = 0; i < gate_instance->num_children - 1; i++) {
                free_signal_list(in[i]);
            }
            vtr::free(in);
        }
    }

    return out_1;
}

/*----------------------------------------------------------------------------
 * (function: create_operation_node)
 *--------------------------------------------------------------------------*/
signal_list_t* create_operation_node(ast_node_t* op, signal_list_t** input_lists, int list_size, char* instance_name_prefix, long assignment_size) {
    long i;
    signal_list_t* return_list = init_signal_list();
    nnode_t* operation_node;
    long max_input_port_width = -1;
    long output_port_width = -1;
    long input_port_width = -1;
    long current_idx;

    /* create the node */
    operation_node = allocate_nnode(op->loc);
    /* store all the relevant info */
    operation_node->related_ast_node = op;
    operation_node->type = op->types.operation.op;
    operation_node->name = node_name(operation_node, instance_name_prefix);

    current_idx = 0;

    /* analyse the inputs */
    for (i = 0; i < list_size; i++) {
        if (max_input_port_width < input_lists[i]->count) {
            max_input_port_width = input_lists[i]->count;
        }
    }

    switch (operation_node->type) {
        case BITWISE_NOT: // ~
            /* only one input port */
            output_port_width = max_input_port_width;
            input_port_width = output_port_width;
            break;
        case BUF_NODE:
            output_port_width = max_input_port_width;
            input_port_width = output_port_width;
            break;
        case ADD: // +
            /* add the largest bit width + the other input padded with 0's */
            return_list->is_adder = true;
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
        case BITWISE_AND:  // &
        case BITWISE_OR:   // |
        case BITWISE_NAND: // ~&
        case BITWISE_NOR:  // ~|
        case BITWISE_XNOR: // ~^
        case BITWISE_XOR:  // ^
            /* we'll padd the other inputs with 0, do it for the largest and throw a warning */
            if (list_size == 2) {
                output_port_width = max_input_port_width;
                input_port_width = output_port_width;
            } else {
                oassert(list_size == 1);
                /* Logical reduction - same as a logic function */
                output_port_width = 1;
                input_port_width = max_input_port_width;
            }
            break;
        case SR:  // >>
        case ASR: // >>>
            output_port_width = input_lists[0]->count;
            input_port_width = input_lists[0]->count;
            break;
        case ASL: // <<<
        case SL:  // <<
            //output_port_width = op->types.operation.output_port_width;
            output_port_width = assignment_size;
            input_port_width = (input_lists[0]->count > input_lists[1]->count) ? input_lists[0]->count : input_lists[1]->count;

            if (output_port_width > input_port_width)
                input_port_width = output_port_width;
            /*
             * This condition checks the output_port_width has been resolved in ast phase if it hasn't 
             * that means there was no LHS operand size or in another word it was not a statement! 
             * As a result, its value would set to the first RHS operand. 
             * e.x. array[(addr/4)*2] => (addr/4)*2 => output_port_width = addr_port_width
             */
            if (output_port_width == 0)
                output_port_width = input_lists[0]->count;
            break;
        case LOGICAL_NOT: // !
        case LOGICAL_OR:  // ||
        case LOGICAL_AND: // &&
            /* only one input port */
            output_port_width = 1;
            input_port_width = max_input_port_width;
            break;
        case LT:            // <
        case GT:            // >
        case LOGICAL_EQUAL: // ==
        case NOT_EQUAL:     // !=
        case LTE:           // <=
        case GTE:           // >=
        {
            if (input_lists[0]->count != input_lists[1]->count) {
                int index_of_smallest;

                /*if (op->num_children != 0 && op->children[0]->type == NUMBERS && op->children[1]->type == NUMBERS)
                 * {
                 * if (input_lists[0]->count < input_lists[1]->count)
                 * index_of_smallest = 0;
                 * else
                 * index_of_smallest = 1;
                 * }
                 *
                 * else*/
                index_of_smallest = find_smallest_non_numerical(op, input_lists, 2);

                input_port_width = input_lists[index_of_smallest]->count;

                /* pad the input lists */
                pad_with_zeros(op, input_lists[0], input_port_width, instance_name_prefix);
                pad_with_zeros(op, input_lists[1], input_port_width, instance_name_prefix);
            } else {
                input_port_width = input_lists[0]->count;
            }
            output_port_width = 1;
            break;
        }
        case DIVIDE: // /
            error_message(NETLIST, op->loc, "%s", "Divide operation not supported by Odin\n");
            break;
        case MODULO: // %
            error_message(NETLIST, op->loc, "%s", "Modulo operation not supported by Odin\n");
            break;
        default:
            error_message(NETLIST, op->loc, "%s", "Operation not supported by Odin\n");
            break;
    }

    oassert(input_port_width != -1);
    oassert(output_port_width != -1);

    /* allocate the pins for the ouput port */
    allocate_more_output_pins(operation_node, output_port_width);
    add_output_port_information(operation_node, output_port_width);

    if ((operation_node->type == SR) || (operation_node->type == ASL) || (operation_node->type == SL) || (operation_node->type == ASR)) {
        if (op->children[1]->type != NUMBERS) {
            for (int k = 0; k < list_size; k++) {
                /* allocate the pins needed */
                allocate_more_input_pins(operation_node, input_port_width);
                /* record this port size */
                add_input_port_information(operation_node, input_port_width);
                /* hookup the input pins */
                hookup_input_pins_from_signal_list(operation_node, current_idx, input_lists[k], 0, input_port_width, verilog_netlist);
                current_idx += input_port_width;
            }

        } else {
            /* record the size of the shift */
            input_port_width = input_lists[0]->count;
            int pad_bit = input_port_width - 1;
            int shift_size = op->children[1]->types.vnumber->get_value();

            switch (operation_node->type) {
                case SL:
                case ASL: {
                    /* connect ZERO to outputs that don't have inputs connected */
                    for (i = 0; i < shift_size; i++) {
                        if (i < output_port_width) {
                            // connect 0 to lower outputs
                            npin_t* zero_pin = allocate_npin();
                            add_fanout_pin_to_net(verilog_netlist->zero_net, zero_pin);

                            add_pin_to_signal_list(return_list, zero_pin);
                            zero_pin->node = NULL;
                        }
                    }

                    /* connect inputs to outputs */
                    for (i = 0; i < output_port_width - shift_size; i++) {
                        if (i < input_port_width) {
                            // connect higher output pin to lower input pin
                            add_pin_to_signal_list(return_list, input_lists[0]->pins[i]);
                            input_lists[0]->pins[i]->node = NULL;
                        } else {
                            npin_t* extension_pin = NULL;
                            if (op->children[1]->types.variable.signedness == SIGNED) {
                                extension_pin = copy_input_npin(input_lists[0]->pins[pad_bit]);
                            } else {
                                extension_pin = get_zero_pin(verilog_netlist);
                            }

                            add_pin_to_signal_list(return_list, extension_pin);
                            extension_pin->node = NULL;
                        }
                    }
                    break;
                }
                case SR: //fallthrough
                case ASR: {
                    // This IF condition has been set to prevent showing error for SR
                    if (operation_node->type == ASR)
                        error_message(NETLIST, op->loc, "%s", "TODO: test that ASR works with signed number\n");

                    for (i = shift_size; i < input_port_width; i++) {
                        // connect higher output pin to lower input pin
                        if (i - shift_size < output_port_width) {
                            add_pin_to_signal_list(return_list, input_lists[0]->pins[i]);
                            input_lists[0]->pins[i]->node = NULL;
                        }
                    }

                    /* Extend pad_bit to outputs that don't have inputs connected */
                    for (i = output_port_width - 1; i >= input_port_width - shift_size; i--) {
                        npin_t* extension_pin = NULL;
                        if (op->children[1]->types.variable.signedness == SIGNED && operation_node->type == ASR) {
                            extension_pin = copy_input_npin(input_lists[0]->pins[pad_bit]);
                        } else {
                            extension_pin = get_zero_pin(verilog_netlist);
                        }

                        add_pin_to_signal_list(return_list, extension_pin);
                        extension_pin->node = NULL;
                    }
                    break;
                }
                default:
                    error_message(NETLIST, op->loc, "%s", "Operation not supported by Odin\n");
                    break;
            }

            //CLEAN UP
            for (i = 0; i < list_size; i++) {
                free_signal_list(input_lists[i]);
            }
            free_nnode(operation_node);

            return return_list;
        }

    } else {
        for (i = 0; i < list_size; i++) {
            if (input_port_width != -2) {
                /* IF taking port widths based on preset */
                /* allocate the pins needed */
                allocate_more_input_pins(operation_node, input_port_width);
                /* record this port size */
                add_input_port_information(operation_node, input_port_width);

                /* hookup the input pins = will do padding of zeros for smaller port */
                hookup_input_pins_from_signal_list(operation_node, current_idx, input_lists[i], 0, input_port_width, verilog_netlist);
                current_idx += input_port_width;
            } else {
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
    }

    for (i = 0; i < output_port_width; i++) {
        npin_t* new_pin1;
        npin_t* new_pin2;
        nnet_t* new_net;
        new_pin1 = allocate_npin();
        new_pin2 = allocate_npin();
        new_net = allocate_nnet();
        new_net->name = vtr::strdup(operation_node->name);
        /* hook the output pin into the node */
        add_output_pin_to_node(operation_node, new_pin1, i);
        /* hook up new pin 1 into the new net */
        add_driver_pin_to_net(new_net, new_pin1);
        /* hook up the new pin 2 to this new net */
        add_fanout_pin_to_net(new_net, new_pin2);

        /* add the new_pin 2 to the list of outputs */
        add_pin_to_signal_list(return_list, new_pin2);
    }

    for (i = 0; i < list_size; i++) {
        free_signal_list(input_lists[i]);
    }

    return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function: evaluate_sensitivity_list)
 *-------------------------------------------------------------------------------------------*/
signal_list_t* evaluate_sensitivity_list(ast_node_t* delay_control, char* instance_name_prefix, sc_hierarchy* local_ref) {
    long i;
    circuit_edge = UNDEFINED_SENSITIVITY;
    signal_list_t* return_sig_list = init_signal_list();

    if (delay_control == NULL) {
        /* Assume always @* */
        circuit_edge = ASYNCHRONOUS_SENSITIVITY;
    } else {
        oassert(delay_control->type == DELAY_CONTROL);

        for (i = 0; i < delay_control->num_children; i++) {
            /* gather edge sensitivity */
            edge_type_e child_sensitivity = UNDEFINED_SENSITIVITY;
            switch (delay_control->children[i]->type) {
                case NEGEDGE:
                    child_sensitivity = FALLING_EDGE_SENSITIVITY;
                    break;
                case POSEDGE:
                    child_sensitivity = RISING_EDGE_SENSITIVITY;
                    break;
                default:
                    child_sensitivity = ASYNCHRONOUS_SENSITIVITY;
                    break;
            }

            if (circuit_edge == UNDEFINED_SENSITIVITY)
                circuit_edge = child_sensitivity;

            if (circuit_edge != child_sensitivity) {
                if (circuit_edge == ASYNCHRONOUS_SENSITIVITY || child_sensitivity == ASYNCHRONOUS_SENSITIVITY) {
                    error_message(NETLIST, delay_control->loc, "%s",
                                  "Sensitivity list switches between edge sensitive to asynchronous.  You can't define something like always @(posedge clock or a).\n");
                }
            }

            switch (child_sensitivity) {
                case FALLING_EDGE_SENSITIVITY: //falltrhough
                case RISING_EDGE_SENSITIVITY: {
                    signal_list_t* temp_list = create_pins(delay_control->children[i]->children[0], NULL, instance_name_prefix, local_ref);
                    oassert(temp_list->count == 1);
                    temp_list->pins[0]->sensitivity = child_sensitivity;
                    add_pin_to_signal_list(return_sig_list, temp_list->pins[0]);
                    free_signal_list(temp_list);
                    break;
                }
                default: /* nothing to do */
                    break;
            }
        }
    }

    /* update the analysis type of this block of statements */
    if (circuit_edge == UNDEFINED_SENSITIVITY) {
        // TODO: empty always block will probably appear here
        error_message(NETLIST, delay_control->loc, "%s", "Sensitivity list error...looks empty?\n");
    } else if (circuit_edge == ASYNCHRONOUS_SENSITIVITY) {
        /* @(*) or @* is here */
        free_signal_list(return_sig_list);
        return_sig_list = NULL;
    }

    type_of_circuit = SEQUENTIAL;

    return return_sig_list;
}

/*---------------------------------------------------------------------------------------------
 * (function: create_if_for_question)
 *-------------------------------------------------------------------------------------------*/
signal_list_t* create_if_for_question(ast_node_t* if_ast, char* instance_name_prefix, sc_hierarchy* local_ref, long assignment_size) {
    signal_list_t* return_list;
    nnode_t* if_node;

    /* create the node */
    if_node = allocate_nnode(if_ast->loc);
    /* store all the relevant info */
    if_node->related_ast_node = if_ast;
    if_node->type = MULTI_PORT_MUX; // port 1 = control, port 2+ = mux options
    if_node->name = node_name(if_node, instance_name_prefix);

    /* create the control structure for the if node */
    create_if_control_signals(&(if_ast->children[0]), if_node, instance_name_prefix, local_ref, assignment_size);

    /* create the statements and integrate them into the mux */
    return_list = create_if_question_mux_expressions(if_ast, if_node, instance_name_prefix, local_ref, assignment_size);

    return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function:  create_if_question_mux_expressions)
 *-------------------------------------------------------------------------------------------*/
signal_list_t* create_if_question_mux_expressions(ast_node_t* if_ast, nnode_t* if_node, char* instance_name_prefix, sc_hierarchy* local_ref, long assignment_size) {
    signal_list_t** if_expressions;
    signal_list_t* return_list;
    int i;

    /* make storage for statements and expressions */
    if_expressions = (signal_list_t**)vtr::malloc(sizeof(signal_list_t*) * 2);

    /* now we will process the statements and add to the other ports */
    for (i = 0; i < 2; i++) {
        if (if_ast->children[i + 1] != NULL) // checking to see if expression exists.  +1 since first child is control expression
        {
            /* IF - this is a normal case item, then process the case match and the details of the statement */
            if_expressions[i] = netlist_expand_ast_of_module(&(if_ast->children[i + 1]), instance_name_prefix, local_ref, assignment_size);
        } else {
            error_message(NETLIST, if_ast->loc, "%s", "No such thing as a a = b ? z;\n");
        }
    }

    /* now with all the lists sorted, we do the matching and proper propogation */
    return_list = create_mux_expressions(if_expressions, if_node, 2, instance_name_prefix);
    vtr::free(if_expressions);

    return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function: create_if)
 *-------------------------------------------------------------------------------------------*/
signal_list_t* create_if(ast_node_t* if_ast, char* instance_name_prefix, sc_hierarchy* local_ref, long assignment_size) {
    signal_list_t* return_list;
    nnode_t* if_node;

    /* create the node */
    if_node = allocate_nnode(if_ast->loc);
    /* store all the relevant info */
    if_node->related_ast_node = if_ast;
    if_node->type = MULTI_PORT_MUX; // port 1 = control, port 2+ = mux options
    if_node->name = node_name(if_node, instance_name_prefix);

    /* create the control structure for the if node */
    create_if_control_signals(&(if_ast->children[0]), if_node, instance_name_prefix, local_ref, assignment_size);

    /* create the statements and integrate them into the mux */
    return_list = create_if_mux_statements(if_ast, if_node, instance_name_prefix, local_ref, assignment_size);

    return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function:  create_if_control_signals)
 *-------------------------------------------------------------------------------------------*/
void create_if_control_signals(ast_node_t** if_expression, nnode_t* if_node, char* instance_name_prefix, sc_hierarchy* local_ref, long assignment_size) {
    signal_list_t* if_logic_expression;
    signal_list_t* if_logic_expression_final;
    nnode_t* not_node;
    npin_t* not_pin;
    signal_list_t* out_pin_list;

    /* reserve the first 2 pins of the mux for the control signals */
    allocate_more_input_pins(if_node, 2);
    /* record this port size */
    add_input_port_information(if_node, 2);

    /* get the logic */
    if_logic_expression = netlist_expand_ast_of_module(if_expression, instance_name_prefix, local_ref, assignment_size);
    oassert(if_logic_expression != NULL);

    if (if_logic_expression->count != 1) {
        nnode_t* or_gate;
        signal_list_t* default_expression;

        or_gate = make_1port_logic_gate_with_inputs(LOGICAL_OR, if_logic_expression->count, if_logic_expression, if_node, -1);
        default_expression = make_output_pins_for_existing_node(or_gate, 1);

        /* copy that output pin to be put into the default */
        add_input_pin_to_node(if_node, default_expression->pins[0], 0);

        if_logic_expression_final = default_expression;
        free_signal_list(if_logic_expression);
    } else {
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
    out_pin_list->pins[0]->is_default = true;

    /* copy that output pin to be put into the default */
    add_input_pin_to_node(if_node, out_pin_list->pins[0], 1);

    free_signal_list(out_pin_list);
    free_signal_list(if_logic_expression_final);
}

/*---------------------------------------------------------------------------------------------
 * (function:  create_if_mux_statements)
 *-------------------------------------------------------------------------------------------*/
signal_list_t* create_if_mux_statements(ast_node_t* if_ast, nnode_t* if_node, char* instance_name_prefix, sc_hierarchy* local_ref, long assignment_size) {
    signal_list_t** if_statements;
    signal_list_t* return_list;
    int i, j;

    /* make storage for statements and expressions */
    if_statements = (signal_list_t**)vtr::malloc(sizeof(signal_list_t*) * 2);

    /* now we will process the statements and add to the other ports */
    for (i = 0; i < 2; i++) {
        if (if_ast->children[i + 1] != NULL) // checking to see if statement exists.  +1 since first child is control expression
        {
            /* IF - this is a normal case item, then process the case match and the details of the statement */
            if_statements[i] = netlist_expand_ast_of_module(&(if_ast->children[i + 1]), instance_name_prefix, local_ref, assignment_size);
            sort_signal_list_alphabetically(if_statements[i]);

            /* free unused nnodes */
            for (j = 0; j < if_statements[i]->count; j++) {
                nnode_t* temp_node = if_statements[i]->pins[j]->node;
                if (temp_node != NULL && temp_node->related_ast_node->type == NON_BLOCKING_STATEMENT) {
                    if_statements[i]->pins[j]->node = free_nnode(temp_node);
                }
            }
        } else {
            /* ELSE - there is no if/else and we need to make a place holder since it means there will be implied signals */
            if_statements[i] = init_signal_list();
        }
    }

    /* now with all the lists sorted, we do the matching and proper propagation */
    return_list = create_mux_statements(if_statements, if_node, 2, instance_name_prefix, local_ref);
    vtr::free(if_statements);

    return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function: create_case)
 *-------------------------------------------------------------------------------------------*/
signal_list_t* create_case(ast_node_t* case_ast, char* instance_name_prefix, sc_hierarchy* local_ref, long assignment_size) {
    signal_list_t* return_list;
    nnode_t* case_node;
    ast_node_t* case_list_of_items;

    /* create the node */
    case_node = allocate_nnode(case_ast->loc);
    /* store all the relevant info */
    case_node->related_ast_node = case_ast;
    case_node->type = MULTI_PORT_MUX; // port 1 = control, port 2+ = mux options
    case_node->name = node_name(case_node, instance_name_prefix);

    /* point to the case list */
    case_list_of_items = case_ast->children[1];

    /* create all the control structures for the case mux ... each bit will turn on one of the paths ... one hot mux */
    create_case_control_signals(case_list_of_items, &(case_ast->children[0]), case_node, instance_name_prefix, local_ref, assignment_size);

    /* create the statements and integrate them into the mux */
    return_list = create_case_mux_statements(case_list_of_items, case_node, instance_name_prefix, local_ref, assignment_size);

    return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function:  create_case_control_signals)
 *-------------------------------------------------------------------------------------------*/
void create_case_control_signals(ast_node_t* case_list_of_items, ast_node_t** compare_against, nnode_t* case_node, char* instance_name_prefix, sc_hierarchy* local_ref, long assignment_size) {
    long i;
    signal_list_t* other_expressions_pin_list = init_signal_list();

    /* reserve the first X pins of the mux for the control signals where X is the number of items */
    allocate_more_input_pins(case_node, case_list_of_items->num_children);
    /* record this port size */
    add_input_port_information(case_node, case_list_of_items->num_children);

    /* now go through each of the case items and build the comparison expressions */
    for (i = 0; i < case_list_of_items->num_children; i++) {
        if (case_list_of_items->children[i]->type == CASE_ITEM) {
            /* IF - this is a normal case item, then process the case match and the details of the statement */
            signal_list_t* case_compare_expression;
            signal_list_t** case_compares = (signal_list_t**)vtr::malloc(sizeof(signal_list_t*) * 2);
            ast_node_t* logical_equal = create_node_w_type(BINARY_OPERATION, case_list_of_items->children[i]->loc);
            logical_equal->types.operation.op = LOGICAL_EQUAL;

            /* get the signals to compare against */
            case_compares[0] = netlist_expand_ast_of_module(compare_against, instance_name_prefix, local_ref, assignment_size);
            case_compares[1] = netlist_expand_ast_of_module(&(case_list_of_items->children[i]->children[0]), instance_name_prefix, local_ref, assignment_size);

            /* make a LOGIC_EQUAL gate that collects all the other signals and if they're all off */
            case_compare_expression = create_operation_node(logical_equal, case_compares, 2, instance_name_prefix, assignment_size);
            oassert(case_compare_expression->count == 1);

            /* hookup this pin to the spot in the case_node */
            add_input_pin_to_node(case_node, case_compare_expression->pins[0], i);

            /* copy that output pin to be put into the default */
            add_pin_to_signal_list(other_expressions_pin_list, copy_input_npin(case_compare_expression->pins[0]));

            /* clean up */
            free_signal_list(case_compare_expression);

            vtr::free(case_compares);
        } else if (case_list_of_items->children[i]->type == CASE_DEFAULT) {
            /* take all the other pins from the case expressions and intstall in the condition */
            nnode_t* default_node;
            signal_list_t* default_expression;

            oassert(i == case_list_of_items->num_children - 1); // has to be at the end

            /* make a NOR gate that collects all the other signals and if they're all off */
            default_node = make_1port_logic_gate_with_inputs(LOGICAL_NOR, case_list_of_items->num_children - 1, other_expressions_pin_list, case_node, -1);
            default_expression = make_output_pins_for_existing_node(default_node, 1);

            // Mark the "default" case for simulation.
            default_expression->pins[0]->is_default = true;

            /* copy that output pin to be put into the default */
            add_input_pin_to_node(case_node, default_expression->pins[0], i);

            free_signal_list(default_expression);
        } else {
            oassert(false);
        }
    }

    free_signal_list(other_expressions_pin_list);
}

/*---------------------------------------------------------------------------------------------
 * (function:  create_case_mux_statements)
 *-------------------------------------------------------------------------------------------*/
signal_list_t* create_case_mux_statements(ast_node_t* case_list_of_items, nnode_t* case_node, char* instance_name_prefix, sc_hierarchy* local_ref, long assignment_size) {
    signal_list_t** case_statement;
    signal_list_t* return_list;
    long i, j;

    /* make storage for statements and expressions */
    case_statement = (signal_list_t**)vtr::malloc(sizeof(signal_list_t*) * (case_list_of_items->num_children));

    /* now we will process the statements and add to the other ports */
    for (i = 0; i < case_list_of_items->num_children; i++) {
        if (case_list_of_items->children[i]->type == CASE_ITEM) {
            /* IF - this is a normal case item, then process the case match and the details of the statement */
            case_statement[i] = netlist_expand_ast_of_module(&(case_list_of_items->children[i]->children[1]), instance_name_prefix, local_ref, assignment_size);
            sort_signal_list_alphabetically(case_statement[i]);

            /* free unused nnodes */
            for (j = 0; j < case_statement[i]->count; j++) {
                nnode_t* temp_node = case_statement[i]->pins[j]->node;
                if (temp_node != NULL && temp_node->related_ast_node->type == NON_BLOCKING_STATEMENT) {
                    case_statement[i]->pins[j]->node = free_nnode(temp_node);
                }
            }
        } else if (case_list_of_items->children[i]->type == CASE_DEFAULT) {
            oassert(i == case_list_of_items->num_children - 1); // has to be at the end
            case_statement[i] = netlist_expand_ast_of_module(&(case_list_of_items->children[i]->children[0]), instance_name_prefix, local_ref, assignment_size);
            sort_signal_list_alphabetically(case_statement[i]);

            /* free unused nnodes */
            for (j = 0; j < case_statement[i]->count; j++) {
                nnode_t* temp_node = case_statement[i]->pins[j]->node;
                if (temp_node != NULL && temp_node->related_ast_node->type == NON_BLOCKING_STATEMENT) {
                    case_statement[i]->pins[j]->node = free_nnode(temp_node);
                }
            }
        } else {
            oassert(false);
        }
    }

    /* now with all the lists sorted, we do the matching and proper propogation */
    return_list = create_mux_statements(case_statement, case_node, case_list_of_items->num_children, instance_name_prefix, local_ref);
    vtr::free(case_statement);

    return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function:  create_mux_statements)
 *-------------------------------------------------------------------------------------------*/
signal_list_t* create_mux_statements(signal_list_t** statement_lists, nnode_t* mux_node, int num_statement_lists, char* instance_name_prefix, sc_hierarchy* local_ref) {
    int i, j;
    signal_list_t* combined_lists;
    int* per_case_statement_idx;
    signal_list_t* return_list = init_signal_list();
    int in_index = 1;
    int out_index = 0;

    /* allocate and initialize indexes */
    per_case_statement_idx = (int*)vtr::calloc(sizeof(int), num_statement_lists);

    /* make the uber list and sort it */
    combined_lists = combine_lists_without_freeing_originals(statement_lists, num_statement_lists);
    sort_signal_list_alphabetically(combined_lists);

    for (i = 0; i < combined_lists->count; i++) {
        int i_skip = 0; // iskip is the number of statemnts that do have this signal so we can skip in the combine list
        npin_t* new_pin1;
        npin_t* new_pin2;
        nnet_t* new_net;
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
        for (j = 0; j < num_statement_lists; j++) {
            int pin_index = in_index * num_statement_lists + j;

            /* check if the current element for this case statement is defined */
            if (
                (per_case_statement_idx[j] < (statement_lists[j] ? statement_lists[j]->count : 0))
                && (statement_lists[j] ? (strcmp(combined_lists->pins[i]->name, statement_lists[j]->pins[per_case_statement_idx[j]]->name) == 0) : 0)) {
                /* If they match then we have a signal with this name and we can attach the pin */
                add_input_pin_to_node(mux_node, statement_lists[j]->pins[per_case_statement_idx[j]], pin_index);

                per_case_statement_idx[j]++; // increment the local index
                i_skip++;                    // it's a match so the combo list will have atleast +1 entries the same
            } else {
                /* Don't match, so this signal is an IMPLIED SIGNAL !!! */
                npin_t* pin = combined_lists->pins[i];

                switch (circuit_edge) {
                    case RISING_EDGE_SENSITIVITY: //fallthrough
                    case FALLING_EDGE_SENSITIVITY: {
                        /* implied signal for mux */
                        if (lookup_implicit_memory_input(pin->name)) {
                            // If the mux feeds an implicit memory, imply zero.
                            add_input_pin_to_node(mux_node, get_zero_pin(verilog_netlist), pin_index);
                        } else {
                            /* lookup this driver name */
                            signal_list_t* this_pin_list = create_pins(NULL, pin->name, instance_name_prefix, local_ref);
                            oassert(this_pin_list->count == 1);
                            //add_a_input_pin_to_node_spot_idx(mux_node, get_zero_pin(verilog_netlist), pin_index);
                            add_input_pin_to_node(mux_node, this_pin_list->pins[0], pin_index);
                            /* clean up */
                            free_signal_list(this_pin_list);
                        }
                        break;
                    }
                    case ASYNCHRONOUS_SENSITIVITY: {
                        /* DON'T CARE - so hookup zero */
                        add_input_pin_to_node(mux_node, get_zero_pin(verilog_netlist), pin_index);
                        // Allows the simulator to be aware of the implied nature of this signal.
                        mux_node->input_pins[pin_index]->is_implied = true;
                        break;
                    }
                    default: {
                        oassert(false && "No circuit sensitivity for mux !!");
                        break;
                    }
                }
            }
        }

        i += i_skip - 1; // for every match move index i forward except this one wihich is handled by for i++
        in_index++;
        out_index++;
    }

    /* clean up */
    for (i = 0; i < num_statement_lists; i++) {
        free_signal_list(statement_lists[i]);
    }
    free_signal_list(combined_lists);
    vtr::free(per_case_statement_idx);

    return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function:  create_mux_expressions)
 *-------------------------------------------------------------------------------------------*/
signal_list_t* create_mux_expressions(signal_list_t** expression_lists, nnode_t* mux_node, int num_expression_lists, char* /*instance_name_prefix*/) {
    int i, j;
    signal_list_t* return_list = init_signal_list();
    int max_index = -1;

    /* find the biggest element */
    for (i = 0; i < num_expression_lists; i++) {
        if (max_index < expression_lists[i]->count) {
            max_index = expression_lists[i]->count;
        }
    }

    for (i = 0; i < max_index; i++) {
        npin_t* new_pin1;
        npin_t* new_pin2;
        nnet_t* new_net;
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
        for (j = 0; j < num_expression_lists; j++) {
            int pin_index = (i + 1) * num_expression_lists + j;

            /* check if the current element for this case statement is defined */
            if (i < expression_lists[j]->count) {
                /* If there is a signal */
                add_input_pin_to_node(mux_node, expression_lists[j]->pins[i], pin_index);

            } else {
                /* Don't match, so this signal is an IMPLIED SIGNAL !!! */
                /* implied signal for mux */
                /* DON'T CARE - so hookup zero */
                add_input_pin_to_node(mux_node, get_zero_pin(verilog_netlist), pin_index);
            }
        }
    }

    /* clean up */
    for (i = 0; i < num_expression_lists; i++) {
        free_signal_list(expression_lists[i]);
    }

    return return_list;
}

/*---------------------------------------------------------------------------------------------
 * (function:  pad_compares_to_smallest_non_numerical_implementation)
 *-------------------------------------------------------------------------------------------*/
int find_smallest_non_numerical(ast_node_t* node, signal_list_t** input_list, int num_input_lists) {
    int i;
    int smallest;
    int smallest_idx;
    short* tested = (short*)vtr::calloc(sizeof(short), num_input_lists);
    short found_non_numerical = false;

    while (found_non_numerical == false) {
        smallest_idx = -1;
        smallest = -1;

        /* find the smallest width, now verify that it's not a number */
        for (i = 0; i < num_input_lists; i++) {
            if (tested[i] == 1) {
                /* skip the ones we've already tried */
                continue;
            }
            if ((smallest == -1) || (smallest >= input_list[i]->count)) {
                smallest = input_list[i]->count;
                smallest_idx = i;
            }
        }

        if (smallest_idx == -1) {
            error_message(NETLIST, node->loc, "%s", "all numbers in padding non numericals\n");
        } else {
            /* mark that we're evaluating this input */
            tested[smallest_idx] = true;

            /* check if the smallest is not a number */
            for (i = 0; i < input_list[smallest_idx]->count; i++) {
                found_non_numerical = !(
                    input_list[smallest_idx]->pins[i]->name
                    && (strstr(input_list[smallest_idx]->pins[i]->name, ONE_VCC_CNS)
                        || strstr(input_list[smallest_idx]->pins[i]->name, ZERO_GND_ZERO)));
                /* Not a number so this is the smallest */
                if (found_non_numerical)
                    break;
            }
        }
    }

    vtr::free(tested);

    return smallest_idx;
}

/*---------------------------------------------------------------------------------------------
 * (function: pad_with_zeros)
 *-------------------------------------------------------------------------------------------*/
void pad_with_zeros(ast_node_t* node, signal_list_t* list, int pad_size, char* /*instance_name_prefix*/) {
    int i;

    if (pad_size > list->count) {
        for (i = list->count; i < pad_size; i++) {
            if (global_args.all_warnings)
                warning_message(NETLIST, node->loc, "%s",
                                "Padding an input port with 0 for operation (likely compare)\n");
            add_pin_to_signal_list(list, get_zero_pin(verilog_netlist));
        }
    } else if (pad_size < list->count) {
        if (global_args.all_warnings)
            warning_message(NETLIST, node->loc, "%s",
                            "More driver pins than nets to drive.  This means that for this operation you are losing some of the most significant bits\n");
    }
}

/*--------------------------------------------------------------------------
 * (function: create_dual_port_ram_block)
 * 	This function creates a dual port ram block node in the netlist
 *	and hooks up the inputs and outputs.
 *------------------------------------------------------------------------*/
signal_list_t* create_dual_port_ram_block(ast_node_t* block, char* instance_name_prefix, t_model* hb_model, sc_hierarchy* local_ref) {
    if (!hb_model || !is_ast_dp_ram(block))
        error_message(NETLIST, block->loc, "%s", "Error in creating dual port ram\n");

    block->type = RAM;

    ast_node_t* block_instance = block->children[0];
    ast_node_t* block_list = block_instance->children[0];
    ast_node_t* block_connect;

    /* create the node */
    nnode_t* block_node = allocate_nnode(block->loc);
    /* store all of the relevant info */
    block_node->related_ast_node = block;
    block_node->type = HARD_IP;
    block_node->name = hard_node_name(block_node, instance_name_prefix, block->identifier_node->types.identifier, block_instance->identifier_node->types.identifier);

    /* Declare the hard block as used for the blif generation */
    hb_model->used = 1;

    /* Need to do a sanity check to make sure ports line up */
    t_model_ports* hb_ports;
    long i;
    for (i = 0; i < block_list->num_children; i++) {
        block_connect = block_list->children[i];
        char* ip_name = block_connect->identifier_node->types.identifier;
        hb_ports = hb_model->inputs;

        while (hb_ports && strcmp(hb_ports->name, ip_name))
            hb_ports = hb_ports->next;

        if (!hb_ports) {
            hb_ports = hb_model->outputs;
            while ((hb_ports != NULL) && (strcmp(hb_ports->name, ip_name) != 0))
                hb_ports = hb_ports->next;
        }

        if (!hb_ports)
            error_message(NETLIST, block->loc, "Non-existant port %s in hard block %s\n", ip_name, block->identifier_node->types.identifier);

        /* Link the signal to the port definition */
        block_connect->children[0]->hb_port = (void*)hb_ports;
    }

    signal_list_t** in_list = (signal_list_t**)vtr::malloc(sizeof(signal_list_t*) * block_list->num_children);
    int out_port_size1 = 0;
    int out_port_size2 = 0;
    int current_idx = 0;
    for (i = 0; i < block_list->num_children; i++) {
        int port_size;
        ast_node_t* block_port_connect;

        in_list[i] = NULL;
        block_connect = block_list->children[i]->identifier_node;
        block_port_connect = block_list->children[i]->children[0];
        hb_ports = (t_model_ports*)block_list->children[i]->children[0]->hb_port;
        char* ip_name = block_connect->types.identifier;

        if (hb_ports->dir == IN_PORT) {
            /* Create the pins for port if needed */
            in_list[i] = create_pins(block_port_connect, NULL, instance_name_prefix, local_ref);
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
                block_node->input_pins[current_idx + j]->mapping = vtr::strdup(ip_name);
            current_idx += port_size;
        }
    }

    if (out_port_size2 != 0)
        oassert(out_port_size2 == out_port_size1);

    int current_out_idx = 0;
    signal_list_t* return_list = init_signal_list();
    for (i = 0; i < block_list->num_children; i++) {
        block_connect = block_list->children[i]->identifier_node;
        hb_ports = (t_model_ports*)block_list->children[i]->children[0]->hb_port;
        char* ip_name = block_connect->types.identifier;

        int out_port_size;
        if (strcmp(hb_ports->name, "out1") == 0)
            out_port_size = out_port_size1;
        else
            out_port_size = out_port_size2;

        if (hb_ports->dir != IN_PORT) {
            allocate_more_output_pins(block_node, out_port_size);
            add_output_port_information(block_node, out_port_size);

            char* alias_name = make_full_ref_name(
                instance_name_prefix,
                block->identifier_node->types.identifier,
                block->children[0]->identifier_node->types.identifier,
                ip_name, -1);

            t_memory_port_sizes* ps = (t_memory_port_sizes*)vtr::calloc(1, sizeof(t_memory_port_sizes));
            ps->size = out_port_size;
            ps->name = alias_name;
            memory_port_size_list = insert_in_vptr_list(memory_port_size_list, ps);

            /* make the implicit output list and hook up the outputs */
            int j;
            for (j = 0; j < out_port_size; j++) {
                char* pin_name = make_full_ref_name(
                    instance_name_prefix,
                    block->identifier_node->types.identifier,
                    block->children[0]->identifier_node->types.identifier,
                    ip_name,
                    (out_port_size > 1) ? j : -1);

                npin_t* new_pin1 = allocate_npin();
                new_pin1->mapping = make_signal_name(hb_ports->name, -1);
                new_pin1->name = pin_name;
                npin_t* new_pin2 = allocate_npin();
                nnet_t* new_net = allocate_nnet();
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
    vtr::free(in_list);

    dp_memory_list = insert_in_vptr_list(dp_memory_list, block_node);
    block_node->type = MEMORY;

    return return_list;
}

/*--------------------------------------------------------------------------
 * (function: create_single_port_ram_block)
 * 	This function creates a single port ram block node in the netlist
 *	and hooks up the inputs and outputs.
 *------------------------------------------------------------------------*/
signal_list_t* create_single_port_ram_block(ast_node_t* block, char* instance_name_prefix, t_model* hb_model, sc_hierarchy* local_ref) {
    if (!hb_model || !is_ast_sp_ram(block))
        error_message(NETLIST, block->loc, "%s", "Error in creating single port ram\n");

    // EDDIE: Uses new enum in ids: RAM (opposed to MEMORY from operation_t previously)
    block->type = RAM;
    signal_list_t* return_list = init_signal_list();
    int current_idx = 0;

    /* create the node */
    nnode_t* block_node = allocate_nnode(block->loc);
    /* store all of the relevant info */
    block_node->related_ast_node = block;
    block_node->type = HARD_IP;
    ast_node_t* block_instance = block->children[0];
    ast_node_t* block_list = block_instance->children[0];
    block_node->name = hard_node_name(block_node,
                                      instance_name_prefix,
                                      block->identifier_node->types.identifier,
                                      block_instance->identifier_node->types.identifier);

    /* Declare the hard block as used for the blif generation */
    hb_model->used = 1;

    /* Need to do a sanity check to make sure ports line up */
    ast_node_t* block_connect;
    char* ip_name = NULL;
    long i;
    t_model_ports* hb_ports;
    for (i = 0; i < block_list->num_children; i++) {
        block_connect = block_list->children[i];
        ip_name = block_connect->identifier_node->types.identifier;
        hb_ports = hb_model->inputs;

        while (hb_ports && strcmp(hb_ports->name, ip_name))
            hb_ports = hb_ports->next;

        if (!hb_ports) {
            hb_ports = hb_model->outputs;
            while (hb_ports && strcmp(hb_ports->name, ip_name))
                hb_ports = hb_ports->next;
        }

        if (!hb_ports)
            error_message(NETLIST, block->loc, "Non-existant port %s in hard block %s\n", ip_name, block->identifier_node->types.identifier);

        /* Link the signal to the port definition */
        block_connect->children[0]->hb_port = (void*)hb_ports;
    }

    /* Need to make sure ALL ports are defined */
    hb_ports = hb_model->inputs;
    i = 0;
    while (hb_ports) {
        i++;
        hb_ports = hb_ports->next;
    }

    hb_ports = hb_model->outputs;
    while (hb_ports) {
        i++;
        hb_ports = hb_ports->next;
    }

    if (i != block_list->num_children)
        error_message(NETLIST, block->loc, "Not all ports defined in hard block %s\n", ip_name);

    signal_list_t** in_list = (signal_list_t**)vtr::malloc(sizeof(signal_list_t*) * block_list->num_children);
    int out_port_size = 0;
    for (i = 0; i < block_list->num_children; i++) {
        int port_size;
        ast_node_t* block_port_connect;

        in_list[i] = NULL;
        block_connect = block_list->children[i]->identifier_node;
        block_port_connect = block_list->children[i]->children[0];
        hb_ports = (t_model_ports*)block_list->children[i]->children[0]->hb_port;
        ip_name = block_connect->types.identifier;

        if (hb_ports->dir == IN_PORT) {
            /* Create the pins for port if needed */
            in_list[i] = create_pins(block_port_connect, NULL, instance_name_prefix, local_ref);
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
                block_node->input_pins[current_idx + j]->mapping = vtr::strdup(ip_name);
            current_idx += port_size;
        }
    }

    int current_out_idx = 0;
    for (i = 0; i < block_list->num_children; i++) {
        block_connect = block_list->children[i]->identifier_node;
        hb_ports = (t_model_ports*)block_list->children[i]->children[0]->hb_port;
        ip_name = block_connect->types.identifier;

        if (hb_ports->dir != IN_PORT) {
            allocate_more_output_pins(block_node, out_port_size);
            add_output_port_information(block_node, out_port_size);

            char* alias_name = make_full_ref_name(
                instance_name_prefix,
                block->identifier_node->types.identifier,
                block->children[0]->identifier_node->types.identifier,
                ip_name, -1);
            t_memory_port_sizes* ps = (t_memory_port_sizes*)vtr::calloc(1, sizeof(t_memory_port_sizes));
            ps->size = out_port_size;
            ps->name = alias_name;
            memory_port_size_list = insert_in_vptr_list(memory_port_size_list, ps);

            /* make the implicit output list and hook up the outputs */
            int j;
            for (j = 0; j < out_port_size; j++) {
                char* pin_name = make_full_ref_name(instance_name_prefix,
                                                    block->identifier_node->types.identifier,
                                                    block->children[0]->identifier_node->types.identifier,
                                                    ip_name,
                                                    (out_port_size > 1) ? j : -1);

                npin_t* new_pin1 = allocate_npin();
                new_pin1->mapping = make_signal_name(hb_ports->name, -1);
                new_pin1->name = pin_name;
                npin_t* new_pin2 = allocate_npin();

                nnet_t* new_net = allocate_nnet();
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

    for (i = 0; i < block_list->num_children; i++) {
        free_signal_list(in_list[i]);
    }

    vtr::free(in_list);

    sp_memory_list = insert_in_vptr_list(sp_memory_list, block_node);
    block_node->type = MEMORY;
    block->net_node = block_node;

    return return_list;
}

/*
 * Creates an architecture independent memory block which will be mapped
 * to soft logic during the partial map.
 */
signal_list_t* create_soft_single_port_ram_block(ast_node_t* block, char* instance_name_prefix, sc_hierarchy* local_ref) {
    char* identifier = block->identifier_node->types.identifier;

    if (!is_ast_sp_ram(block))
        error_message(NETLIST, block->loc, "%s", "Error in creating soft single port ram\n");

    block->type = RAM;

    // create the node
    nnode_t* block_node = allocate_nnode(block->loc);
    // store all of the relevant info
    block_node->related_ast_node = block;
    block_node->type = HARD_IP;
    ast_node_t* block_instance = block->children[0];
    ast_node_t* block_list = block_instance->children[0];
    block_node->name = hard_node_name(block_node,
                                      instance_name_prefix,
                                      identifier,
                                      block_instance->identifier_node->types.identifier);

    long i;
    signal_list_t** in_list = (signal_list_t**)vtr::malloc(sizeof(signal_list_t*) * block_list->num_children);
    int out_port_size = 0;
    int current_idx = 0;
    for (i = 0; i < block_list->num_children; i++) {
        in_list[i] = NULL;
        ast_node_t* block_connect = block_list->children[i]->identifier_node;
        ast_node_t* block_port_connect = block_list->children[i]->children[0];

        char* ip_name = block_connect->types.identifier;

        if (strcmp(ip_name, "out")) {
            // Create the pins for port if needed
            in_list[i] = create_pins(block_port_connect, NULL, instance_name_prefix, local_ref);
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
                block_node->input_pins[current_idx + j]->mapping = vtr::strdup(ip_name);
            current_idx += port_size;
        }
    }

    int current_out_idx = 0;
    signal_list_t* return_list = init_signal_list();
    for (i = 0; i < block_list->num_children; i++) {
        ast_node_t* block_connect = block_list->children[i]->identifier_node;
        char* ip_name = block_connect->types.identifier;

        if (!strcmp(ip_name, "out")) {
            allocate_more_output_pins(block_node, out_port_size);
            add_output_port_information(block_node, out_port_size);

            char* alias_name = make_full_ref_name(
                instance_name_prefix,
                identifier,
                block->children[0]->identifier_node->types.identifier,
                block_connect->types.identifier, -1);
            t_memory_port_sizes* ps = (t_memory_port_sizes*)vtr::calloc(1, sizeof(t_memory_port_sizes));
            ps->size = out_port_size;
            ps->name = alias_name;
            memory_port_size_list = insert_in_vptr_list(memory_port_size_list, ps);

            // make the implicit output list and hook up the outputs
            int j;
            for (j = 0; j < out_port_size; j++) {
                char* pin_name;
                if (out_port_size > 1) {
                    pin_name = make_full_ref_name(instance_name_prefix,
                                                  identifier,
                                                  block->children[0]->identifier_node->types.identifier,
                                                  block_connect->types.identifier, j);
                } else {
                    pin_name = make_full_ref_name(
                        instance_name_prefix,
                        identifier,
                        block->children[0]->identifier_node->types.identifier,
                        block_connect->types.identifier, -1);
                }

                npin_t* new_pin1 = allocate_npin();
                new_pin1->mapping = ip_name;
                new_pin1->name = pin_name;

                npin_t* new_pin2 = allocate_npin();

                nnet_t* new_net = allocate_nnet();
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

    vtr::free(in_list);

    block_node->type = MEMORY;
    block->net_node = block_node;

    return return_list;
}

/*
 * Creates an architecture independent memory block which will be mapped
 * to soft logic during the partial map.
 */
signal_list_t* create_soft_dual_port_ram_block(ast_node_t* block, char* instance_name_prefix, sc_hierarchy* local_ref) {
    char* identifier = block->identifier_node->types.identifier;
    char* instance_name = block->children[0]->identifier_node->types.identifier;

    if (!is_ast_dp_ram(block))
        error_message(NETLIST, block->loc, "%s", "Error in creating soft dual port ram\n");

    block->type = RAM;

    // create the node
    nnode_t* block_node = allocate_nnode(block->loc);
    // store all of the relevant info
    block_node->related_ast_node = block;
    block_node->type = HARD_IP;
    ast_node_t* block_instance = block->children[0];
    ast_node_t* block_list = block_instance->children[0];
    block_node->name = hard_node_name(block_node,
                                      instance_name_prefix,
                                      identifier,
                                      block_instance->identifier_node->types.identifier);

    long i;
    signal_list_t** in_list = (signal_list_t**)vtr::malloc(sizeof(signal_list_t*) * block_list->num_children);
    int out1_size = 0;
    int out2_size = 0;
    int current_idx = 0;
    for (i = 0; i < block_list->num_children; i++) {
        in_list[i] = NULL;
        ast_node_t* block_connect = block_list->children[i]->identifier_node;
        ast_node_t* block_port_connect = block_list->children[i]->children[0];

        char* ip_name = block_connect->types.identifier;

        int is_output = !strcmp(ip_name, "out1") || !strcmp(ip_name, "out2");

        if (!is_output) {
            // Create the pins for port if needed
            in_list[i] = create_pins(block_port_connect, NULL, instance_name_prefix, local_ref);
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
                block_node->input_pins[current_idx + j]->mapping = vtr::strdup(ip_name);

            current_idx += port_size;
        }
    }

    oassert(out1_size == out2_size);

    int current_out_idx = 0;
    signal_list_t* return_list = init_signal_list();
    for (i = 0; i < block_list->num_children; i++) {
        ast_node_t* block_connect = block_list->children[i]->identifier_node;
        char* ip_name = block_connect->types.identifier;

        int is_out1 = !strcmp(ip_name, "out1");
        int is_out2 = !strcmp(ip_name, "out2");
        int is_output = is_out1 || is_out2;

        if (is_output) {
            char* alias_name = make_full_ref_name(
                instance_name_prefix,
                identifier,
                instance_name,
                ip_name, -1);

            int port_size = is_out1 ? out1_size : out2_size;

            allocate_more_output_pins(block_node, port_size);
            add_output_port_information(block_node, port_size);

            t_memory_port_sizes* ps = (t_memory_port_sizes*)vtr::calloc(1, sizeof(t_memory_port_sizes));
            ps->size = port_size;
            ps->name = alias_name;
            memory_port_size_list = insert_in_vptr_list(memory_port_size_list, ps);

            // make the implicit output list and hook up the outputs
            int j;
            for (j = 0; j < port_size; j++) {
                char* pin_name = make_full_ref_name(
                    instance_name_prefix,
                    identifier,
                    instance_name,
                    ip_name,
                    (port_size > 1) ? j : -1);

                npin_t* new_pin1 = allocate_npin();
                new_pin1->mapping = ip_name;
                new_pin1->name = pin_name;

                npin_t* new_pin2 = allocate_npin();

                nnet_t* new_net = allocate_nnet();
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

    vtr::free(in_list);

    block_node->type = MEMORY;
    block->net_node = block_node;

    return return_list;
}

/*--------------------------------------------------------------------------
 * (function: create_hard_block)
 * 	This function creates a hard block node in the netlist and hooks up the
 * 	inputs and outputs.
 *------------------------------------------------------------------------*/

signal_list_t* create_hard_block(ast_node_t* block, char* instance_name_prefix, sc_hierarchy* local_ref) {
    signal_list_t **in_list, *return_list;
    nnode_t* block_node;
    ast_node_t* block_instance = block->children[0];
    ast_node_t* block_list = block_instance->children[0];
    ast_node_t* block_connect;
    char* ip_name;
    t_model_ports* hb_ports = NULL;
    long i;
    int j, current_idx, current_out_idx;
    int is_mult = 0;
    int mult_size = 0;
    int adder_size = 0;
    int is_adder = 0;

    char* identifier = block->identifier_node->types.identifier;

    /* See if the hard block declared is supported by FPGA architecture */
    t_model* hb_model = find_hard_block(identifier);

    /* single_port_ram's are a special case due to splitting */
    if (is_ast_sp_ram(block)) {
        if (hb_model)
            return create_single_port_ram_block(block, instance_name_prefix, hb_model, local_ref);
        else
            return create_soft_single_port_ram_block(block, instance_name_prefix, local_ref);
    }

    /* dual_port_ram's are a special case due to splitting */
    if (is_ast_dp_ram(block)) {
        if (hb_model)
            return create_dual_port_ram_block(block, instance_name_prefix, hb_model, local_ref);
        else
            return create_soft_dual_port_ram_block(block, instance_name_prefix, local_ref);
    }

    /* TODO: create_soft_adder_block()/create_soft_multiplier_block()??? */

    if (!hb_model) {
        error_message(NETLIST, block->loc,
                      "Found Hard Block \"%s\": Not supported by FPGA Architecture\n", identifier);
    }

    /* memory's are a special case due to splitting */
    if (strcmp(hb_model->name, "multiply") == 0) {
        is_mult = 1;
    } else if (strcmp(hb_model->name, "adder") == 0) {
        is_adder = 1;
    }

    return_list = init_signal_list();
    current_idx = 0;
    current_out_idx = 0;

    /* create the node */
    block_node = allocate_nnode(block->loc);
    /* store all of the relevant info */
    block_node->related_ast_node = block;
    block_node->type = HARD_IP;
    block_node->name = hard_node_name(
        block_node,
        instance_name_prefix,
        block->identifier_node->types.identifier,
        block_instance->identifier_node->types.identifier);

    /* Declare the hard block as used for the blif generation */
    hb_model->used = 1;

    /* Need to do a sanity check to make sure ports line up */
    for (i = 0; i < block_list->num_children; i++) {
        block_connect = block_list->children[i];
        ip_name = block_connect->identifier_node->types.identifier;
        hb_ports = hb_model->inputs;
        while ((hb_ports != NULL) && (strcmp(hb_ports->name, ip_name) != 0))
            hb_ports = hb_ports->next;
        if (hb_ports == NULL) {
            hb_ports = hb_model->outputs;
            while ((hb_ports != NULL) && (strcmp(hb_ports->name, ip_name) != 0))
                hb_ports = hb_ports->next;
        }

        if (hb_ports == NULL) {
            error_message(NETLIST, block->loc, "Non-existant port %s in hard block %s\n", ip_name, block->identifier_node->types.identifier);
        }

        /* Link the signal to the port definition */
        block_connect->children[0]->hb_port = (void*)hb_ports;
    }

    in_list = (signal_list_t**)vtr::malloc(sizeof(signal_list_t*) * block_list->num_children);
    for (i = 0; i < block_list->num_children; i++) {
        int port_size;
        ast_node_t* block_port_connect;

        in_list[i] = NULL;
        block_connect = block_list->children[i]->identifier_node;
        block_port_connect = block_list->children[i]->children[0];
        hb_ports = (t_model_ports*)block_list->children[i]->children[0]->hb_port;
        ip_name = block_connect->types.identifier;

        if (hb_ports->dir == IN_PORT) {
            int min_size;

            /* Create the pins for port if needed */
            in_list[i] = create_pins(block_port_connect, NULL, instance_name_prefix, local_ref);

            /* Only map the required number of pins to match port size */
            port_size = hb_ports->size;
            if (in_list[i]->count < port_size)
                min_size = in_list[i]->count;
            else
                min_size = port_size;

            /* IF a multiplier - leave input size arbitrary with no padding */
            if (is_mult == 1) {
                min_size = in_list[i]->count;
                port_size = in_list[i]->count;
                mult_size = mult_size + min_size;
            }

            /* IF a adder -*/
            if (is_adder == 1) {
                min_size = in_list[i]->count;
                port_size = in_list[i]->count;
                if (min_size > adder_size) {
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
                block_node->input_pins[current_idx + j]->mapping = vtr::strdup(ip_name);
            current_idx += port_size;
        } else {
            /* IF a multiplier - need to process the output pins last!!! */
            /* Makes the assumption that a multiplier has only 1 output */
            if (is_mult == 0 && is_adder == 0) {
                allocate_more_output_pins(block_node, hb_ports->size);
                add_output_port_information(block_node, hb_ports->size);

                /* make the implicit output list and hook up the outputs */
                for (j = 0; j < hb_ports->size; j++) {
                    npin_t* new_pin1;
                    npin_t* new_pin2;
                    nnet_t* new_net;
                    char* pin_name;
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

    hb_ports = hb_model->outputs;

    /* IF a multiplier - need to process the output pins now */
    /* Size of the output is estimated to be size of the inputs added */
    if (is_mult == 1) {
        allocate_more_output_pins(block_node, mult_size);
        add_output_port_information(block_node, mult_size);

        /* make the implicit output list and hook up the outputs */
        for (j = 0; j < mult_size; j++) {
            npin_t* new_pin1;
            npin_t* new_pin2;
            nnet_t* new_net;
            char* pin_name;
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

            vtr::free(pin_name);
        }
        current_out_idx += j;
    }
    /* IF a multiplier - need to process the output pins now */
    else if (is_adder == 1) {
        /*adder_size is the size of sumout*/
        allocate_more_output_pins(block_node, adder_size + 1);
        add_output_port_information(block_node, adder_size + 1);

        for (j = 0; j < adder_size + 1; j++) {
            npin_t* new_pin1;
            npin_t* new_pin2;
            nnet_t* new_net;
            char* pin_name;
            long sc_spot;

            new_pin1 = allocate_npin();
            new_pin2 = allocate_npin();
            new_net = allocate_nnet();
            /*For sumout - make the implicit output list and hook up the outputs */
            if (j < adder_size) {
                if (adder_size > 1) {
                    pin_name = make_full_ref_name(block_node->name, NULL, NULL, hb_ports->name, j);
                    new_pin1->mapping = make_signal_name(hb_ports->name, j);
                } else {
                    pin_name = make_full_ref_name(block_node->name, NULL, NULL, hb_ports->name, -1);
                    new_pin1->mapping = make_signal_name(hb_ports->name, -1);
                }
                new_net->name = hb_ports->name;
            }
            /*For cout - make the implicit output list and hook up the outputs */
            else {
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

            vtr::free(pin_name);
        }
        current_out_idx += j;
    }

    for (i = 0; i < block_list->num_children; i++) {
        free_signal_list(in_list[i]);
    }
    vtr::free(in_list);

    /* Add multiplier to list for later splitting and optimizing */
    if (is_mult == 1)
        mult_list = insert_in_vptr_list(mult_list, block_node);
    /* Add adder to list for later splitting and optimizing */
    if (is_adder == 1)
        add_list = insert_in_vptr_list(add_list, block_node);

    return return_list;
}

void reorder_connections_from_name(ast_node_t* instance_node_list, ast_node_t* instantiated_instance_list, ids type) {
    int i, j;
    bool has_matched;
    int* arr_index = new int[instantiated_instance_list->num_children];
    int start_index = type == FUNCTION ? 1 : 0;

    for (i = start_index; i < instantiated_instance_list->num_children; i++) {
        has_matched = false;
        arr_index[i] = -1;
        for (j = start_index; j < instance_node_list->num_children; j++) {
            if (strcmp(instance_node_list->children[j]->identifier_node->types.identifier,
                       instantiated_instance_list->children[i]->identifier_node->types.identifier)
                == 0) {
                if (i != j) {
                    arr_index[i] = j;
                }

                has_matched = true;
            }
        }
        if (!has_matched) {
            error_message(NETLIST, instantiated_instance_list->children[i]->loc,
                          "Module entry %s does not exist\n",
                          instantiated_instance_list->children[i]->identifier_node->types.identifier);
        }
    }

    for (i = start_index; i < instantiated_instance_list->num_children; i++) {
        if (arr_index[i] != -1) {
            ast_node_t* temp = instantiated_instance_list->children[arr_index[i]];
            instantiated_instance_list->children[arr_index[i]] = instantiated_instance_list->children[i];
            instantiated_instance_list->children[i] = temp;
            arr_index[arr_index[i]] = -1;
        }
    }

    delete[] arr_index;
}
/*--------------------------------------------------------------------------
 * Resolves top module parameters and defparams defined by it's own parameters as those parameters cannot be overriden
 * Technically the top module parameters can be overriden by defparams in a seperate module however that cannot be supported until
 * Odin has full hierarchy support. Even Quartus doesn't allow defparams to override the top module parameters but the Verilog Standard 2005
 * doesn't say its not allowed. It's not good practice to override top module parameters and Odin could choose not to allow it.
 *-------------------------------------------------------------------------*/

ast_node_t* resolve_top_module_parameters(ast_node_t* node, sc_hierarchy* top_sc_list) {
    if (node->type == MODULE_PARAMETER) {
        ast_node_t* child = node->children[4];
        if (child->type != NUMBERS) {
            node->children[4] = resolve_top_parameters_defined_by_parameters(child, top_sc_list, 0);
        }
        return (node);
    }

    int i = 0;

    while (node->num_children > i) {
        ast_node_t* new_child = node->children[i];
        if (new_child != NULL) {
            node->children[i] = resolve_top_module_parameters(new_child, top_sc_list);
        }
        i++;
    }
    return (node);
}

ast_node_t* resolve_top_parameters_defined_by_parameters(ast_node_t* node, sc_hierarchy* top_sc_list, int count) {
    STRING_CACHE* local_param_table_sc = top_sc_list->local_param_table_sc;
    STRING_CACHE* local_symbol_table_sc = top_sc_list->local_symbol_table_sc;

    count++;

    if (count > RECURSIVE_LIMIT) {
        error_message(NETLIST, node->loc, "Exceeds recursion count limit of %d", RECURSIVE_LIMIT);
    }

    if (node->type == IDENTIFIERS) {
        const char* string = node->types.identifier;
        long sc_spot = sc_lookup_string(local_param_table_sc, string);
        long sc_spot_symbol = sc_lookup_string(local_symbol_table_sc, string);

        if ((sc_spot == -1) && (sc_spot_symbol == -1)) {
            error_message(NETLIST, node->loc, "%s is not a valid parameter", string);
        } else if (sc_spot_symbol != -1) {
            return (node);
        } else {
            ast_node_t* newNode = ast_node_deep_copy((ast_node_t*)local_param_table_sc->data[sc_spot]);
            if (newNode->type != NUMBERS) {
                newNode = resolve_top_parameters_defined_by_parameters(newNode, top_sc_list, count);
            }
            node = free_whole_tree(node);
            node = newNode;
        }
    }
    if (node->type == BINARY_OPERATION) {
        for (int i = 0; i < 2; i++) {
            ast_node_t* child = node->children[i];
            if (child->type != NUMBERS) {
                node->children[i] = resolve_top_parameters_defined_by_parameters(node->children[i], top_sc_list, count);
            }
        }
    }
    return (node);
}
