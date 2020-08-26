/* Standard libraries */
#include <stdlib.h>
#include <string.h>

/* Odin_II libraries */
#include "odin_globals.h"
#include "odin_types.h"
#include "ast_util.h"
#include "ast_elaborate.h"
#include "parse_making_ast.h"
#include "netlist_create_from_ast.h"
#include "odin_util.h"
#include "vtr_memory.h"
#include "vtr_util.h"

/* This files header */
#include "ast_loop_unroll.h"

ast_node_t* unroll_for_loop(ast_node_t* node, ast_node_t* parent, int* num_unrolled, sc_hierarchy* local_ref, bool is_generate) {
    oassert(node && node->type == FOR);

    ast_node_t* unrolled_for = resolve_for(node);
    oassert(unrolled_for != nullptr);

    *num_unrolled = unrolled_for->num_children;

    /* update parent */
    int i;
    int this_genblk = 0;
    for (i = 0; i < parent->num_children; i++) {
        if (node == parent->children[i]) {
            int j;
            for (j = i; j < (unrolled_for->num_children + i); j++) {
                ast_node_t* child = unrolled_for->children[j - i];
                add_child_to_node_at_index(parent, child, j);
                unrolled_for->children[j - i] = NULL;

                /* create scopes as necessary */
                if (is_generate) {
                    oassert(child->type == BLOCK);

                    /*  generate blocks always have scopes; parent has access to named block 
                     * but not unnamed, and child always has access to parent */
                    sc_hierarchy* child_hierarchy = init_sc_hierarchy();
                    child->types.hierarchy = child_hierarchy;

                    child_hierarchy->top_node = child;
                    child_hierarchy->parent = local_ref;

                    if (child->types.identifier != NULL) {
                        local_ref->block_children = (sc_hierarchy**)vtr::realloc(local_ref->block_children, sizeof(sc_hierarchy*) * (local_ref->num_block_children + 1));
                        local_ref->block_children[local_ref->num_block_children] = child_hierarchy;
                        local_ref->num_block_children++;

                        /* add an array reference to this label */
                        std::string new_id(child->types.identifier);
                        new_id = new_id + "[" + std::to_string(j - i) + "]";
                        vtr::free(child->types.identifier);
                        child->types.identifier = vtr::strdup(new_id.c_str());

                        child_hierarchy->scope_id = node->types.identifier;
                        child_hierarchy->instance_name_prefix = make_full_ref_name(local_ref->instance_name_prefix, NULL, child->types.identifier, NULL, -1);
                    } else {
                        /* create a unique scope id/instance name prefix for internal use */
                        this_genblk = local_ref->num_unnamed_genblks + 1;
                        std::string new_scope_id("genblk");
                        new_scope_id = new_scope_id + std::to_string(this_genblk) + "[" + std::to_string(j - i) + "]";
                        child_hierarchy->scope_id = vtr::strdup(new_scope_id.c_str());
                        child_hierarchy->instance_name_prefix = make_full_ref_name(local_ref->instance_name_prefix, NULL, child_hierarchy->scope_id, NULL, -1);
                    }

                    /* string caches */
                    create_param_table_for_scope(child, child_hierarchy);
                    create_symbol_table_for_scope(child, child_hierarchy);
                } else if (child->type == BLOCK && child->types.identifier != NULL) {
                    /* only create scope if child is named block */
                    sc_hierarchy* child_hierarchy = init_sc_hierarchy();
                    child->types.hierarchy = child_hierarchy;

                    child_hierarchy->top_node = child;
                    child_hierarchy->parent = local_ref;

                    local_ref->block_children = (sc_hierarchy**)vtr::realloc(local_ref->block_children, sizeof(sc_hierarchy*) * (local_ref->num_block_children + 1));
                    local_ref->block_children[local_ref->num_block_children] = child_hierarchy;
                    local_ref->num_block_children++;

                    /* add an array reference to this label */
                    std::string new_id(child->types.identifier);
                    new_id = new_id + "[" + std::to_string(j - i) + "]";
                    vtr::free(child->types.identifier);
                    child->types.identifier = vtr::strdup(new_id.c_str());

                    child_hierarchy->scope_id = node->types.identifier;
                    child_hierarchy->instance_name_prefix = make_full_ref_name(local_ref->instance_name_prefix, NULL, child->types.identifier, NULL, -1);

                    /* string caches */
                    create_param_table_for_scope(child, child_hierarchy);
                    create_symbol_table_for_scope(child, child_hierarchy);
                }
            }

            oassert(j == (unrolled_for->num_children + i) && parent->children[j] == node);
            remove_child_from_node_at_index(parent, j);

            break;
        }
    }

    if (this_genblk > 0) {
        local_ref->num_unnamed_genblks++;
    }

    free_whole_tree(unrolled_for);
    return parent->children[i];
}

/*
 *  (function: resolve_for)
 */
ast_node_t* resolve_for(ast_node_t* node) {
    oassert(is_for_node(node));
    oassert(node != nullptr);
    ast_node_t* body_parent = nullptr;

    ast_node_t* pre = node->children[0];
    ast_node_t* cond = node->children[1];
    ast_node_t* post = node->children[2];
    ast_node_t* body = node->children[3];

    ast_node_t* value = 0;
    if (resolve_pre_condition(pre, &value)) {
        error_message(AST, pre->loc, "%s", "Unsupported pre-condition node in for loop");
    }

    int error_code = 0;
    condition_function cond_func = resolve_condition(cond, pre->children[0], &error_code);
    if (error_code) {
        error_message(AST, cond->loc, "%s", "Unsupported condition node in for loop");
    }

    post_condition_function post_func = resolve_post_condition(post, pre->children[0], &error_code);
    if (error_code) {
        error_message(AST, post->loc, "%s", "Unsupported post-condition node in for loop");
    }

    bool dup_body = cond_func(value->types.vnumber->get_value());
    while (dup_body) {
        ast_node_t* new_body = dup_and_fill_body(body, pre, &value, &error_code);
        if (error_code) {
            error_message(AST, pre->loc, "%s", "Unsupported pre-condition node in for loop");
        }

        VNumber* temp_vnum = value->types.vnumber;
        value->types.vnumber = new VNumber(post_func(temp_vnum->get_value()));
        delete temp_vnum;

        body_parent = body_parent ? newList_entry(body_parent, new_body) : newList(BLOCK, new_body, new_body->loc);

        dup_body = cond_func(value->types.vnumber->get_value());
    }

    free_whole_tree(value);
    return body_parent;
}

/*
 *  (function: resolve_pre_condition)
 *  return 0 if the first value of the variable set
 *  in the pre condition of a `for` node has been put in location 
 *  pointed to by the number pointer.
 *
 *  return a non-zero number on failure.
 *  define failure constants in header.
 */
int resolve_pre_condition(ast_node_t* node, ast_node_t** number_node) {
    /* Add new for loop support here. Keep current work in the TODO
     * Currently supporting:
     *     for(VAR = NUM; ...; ...) ...
     * TODO:
     *     for(VAR = function(PARAMS...); ...; ...) ...
     */
    /* IMPORTANT: if support for more complex continue conditions is added, update this inline function. */
    if (is_unsupported_pre(node)) {
        return UNSUPPORTED_PRE_CONDITION_NODE;
    }
    if (*number_node) free_whole_tree(*number_node);
    *number_node = ast_node_deep_copy(node->children[1]);
    return 0;
}

/** IMPORTANT: as support for more complex continue conditions is added, update this function. 
 *  (function: is_unsupported_condition)
 *  returns true if, given the supplied symbol, the node can be simplifed 
 *  to true or false if the symbol is replaced with some value.
 */
bool is_unsupported_condition(ast_node_t* node, ast_node_t* symbol) {
    bool invalid_inequality = (node->type != BINARY_OPERATION || !(node->types.operation.op == LT || node->types.operation.op == GT || node->types.operation.op == LOGICAL_EQUAL || node->types.operation.op == NOT_EQUAL || node->types.operation.op == LTE || node->types.operation.op == GTE) || node->num_children != 2 || node->children[1] == nullptr || !(node->children[1]->type == NUMBERS || node->children[1]->type == IDENTIFIERS) || node->children[0] == nullptr || !(node->children[0]->type == NUMBERS || node->children[0]->type == IDENTIFIERS));

    bool invalid_logical_concatenation = (node->type != BINARY_OPERATION || !(node->types.operation.op == LOGICAL_OR || node->types.operation.op == LOGICAL_AND) || node->num_children != 2 || node->children[1] == nullptr || is_unsupported_condition(node->children[1], symbol) || node->children[0] == nullptr || is_unsupported_condition(node->children[0], symbol));

    bool invalid_negation = (node->type != UNARY_OPERATION || node->types.operation.op != LOGICAL_NOT || node->num_children != 1 || node->children[0] == nullptr || is_unsupported_condition(node->children[0], symbol));

    bool contains_unknown_symbols = !(invalid_inequality) && ((node->children[0]->type == IDENTIFIERS && strcmp(node->children[0]->types.identifier, symbol->types.identifier)) || (node->children[1]->type == IDENTIFIERS && strcmp(node->children[1]->types.identifier, symbol->types.identifier)));

    return ((invalid_inequality || contains_unknown_symbols) && invalid_logical_concatenation && invalid_negation);
}

/*
 *  (function: resolve_condition)
 *  return a lambda which tests the loop condition for a given value
 */
condition_function resolve_condition(ast_node_t* node, ast_node_t* symbol, int* error_code) {
    /* Add new for loop support here. Keep current work in the TODO
     * Currently supporting:
     *     for(...; VAR {<, >, ==, !=, <=, >=} NUM; ...) ...
     *     for(...; CONDITION_A {&&, ||} CONDITION_B;...) ...
     *     for(...; !(CONDITION);...) ...
     * TODO:
     *     for(...; {EXPRESSION_OF_VAR, NUM} {<, >, ==, !=, <=, >=} {EXPRESSION_OF_VAR, NUM}; ...) ...
     */
    /* IMPORTANT: if support for more complex continue conditions is added, update this inline function. */
    if (is_unsupported_condition(node, symbol)) {
        *error_code = UNSUPPORTED_CONDITION_NODE;
        return nullptr;
    }
    *error_code = 0;
    /* Resursive calls need to report errors before returning a lambda */
    condition_function left = nullptr;
    condition_function right = nullptr;
    condition_function inner = nullptr;
    switch (node->types.operation.op) {
        case LOGICAL_OR:
            left = resolve_condition(node->children[0], symbol, error_code);
            if (*error_code)
                return nullptr;
            right = resolve_condition(node->children[1], symbol, error_code);
            if (*error_code)
                return nullptr;
            return [=](long value) {
                return (left(value) || right(value));
            };
        case LOGICAL_AND:
            left = resolve_condition(node->children[0], symbol, error_code);
            if (*error_code)
                return nullptr;
            right = resolve_condition(node->children[1], symbol, error_code);
            if (*error_code)
                return nullptr;
            return [=](long value) {
                return (left(value) && right(value));
            };
        case LOGICAL_NOT:
            inner = resolve_condition(node->children[0], symbol, error_code);
            if (*error_code)
                return nullptr;
            return [=](long value) {
                bool inner_true = inner(value);
                return !inner_true;
            };
        default:
            break;
    }
    /* Non-recursive calls can type check in the lambda to save copy/paste */
    return [=](long value) {
        switch (node->types.operation.op) {
            case LT:
                return value < node->children[1]->types.vnumber->get_value();
            case GT:
                return value > node->children[1]->types.vnumber->get_value();
            case NOT_EQUAL:
                return value != node->children[1]->types.vnumber->get_value();
            case LOGICAL_EQUAL:
                return value == node->children[1]->types.vnumber->get_value();
            case LTE:
                return value <= node->children[1]->types.vnumber->get_value();
            case GTE:
                return value >= node->children[1]->types.vnumber->get_value();
            default:
                return false;
        }
    };
}

/* IMPORTANT: as support for more complex post conditions is added, update this function. 
 * (function: is_unsupported_post)
 * returns true if the post condition blocking assignment is more complex than
 * can currently be unrolled statically
 */
bool is_unsupported_post(ast_node_t* node, ast_node_t* symbol) {
    return (node == nullptr || node->type != BINARY_OPERATION || !(node->types.operation.op == ADD || node->types.operation.op == MINUS || node->types.operation.op == MULTIPLY || node->types.operation.op == DIVIDE) || node->num_children != 2 || node->children[1] == nullptr || !((node->children[1]->type == IDENTIFIERS && !strcmp(node->children[1]->types.identifier, symbol->types.identifier)) || node->children[1]->type == NUMBERS || !is_unsupported_post(node->children[0], symbol)) || node->children[0] == nullptr || !((node->children[0]->type == IDENTIFIERS && !strcmp(node->children[0]->types.identifier, symbol->types.identifier)) || node->children[0]->type == NUMBERS || !is_unsupported_post(node->children[0], symbol)));
}

post_condition_function resolve_binary_operation(ast_node_t* node) {
    if (node->type == NUMBERS) {
        return [=](long value) {
            /* 
             * this lambda triggers a warning for unused variable unless
             * we use value to generate a 0
             */
            return node->types.vnumber->get_value() + (value - value);
        };
    } else if (node->type == IDENTIFIERS) {
        return [=](long value) {
            return value;
        };
    } else {
        return [=](long value) {
            post_condition_function left_func = resolve_binary_operation(node->children[0]);
            post_condition_function right_func = resolve_binary_operation(node->children[1]);
            switch (node->types.operation.op) {
                case ADD:
                    return left_func(value) + right_func(value);
                case MINUS:
                    return left_func(value) - right_func(value);
                case MULTIPLY:
                    return left_func(value) * right_func(value);
                case DIVIDE:
                    return left_func(value) / right_func(value);
                default:
                    return 0x0L;
            }
        };
    }
}

/*
 *  (function: resolve_post_condition)
 *  return a lambda which gives the next value
 *  of the loop variable given the current value
 *  of the loop variable
 */
post_condition_function resolve_post_condition(ast_node_t* assignment, ast_node_t* symbol, int* error_code) {
    /* Add new for loop support here. Keep current work in the TODO
     * Given iteration t, and VAR[t] is the value of VAR at iteration t,
     * VAR[0] is init, EXPRESSION_OF_VAR[t] is the value of the post 
     * expression evaluated at iteration t, and VAR[t+1] is the 
     * value of VAR after the current iteration:
     *     Currently supporting:
     *         for(...; ...; VAR = VAR {+, -, *, /} NUM) ...
     *         for(...; ...; VAR[t+1] = EXPRESSION_OF_VAR[t])
     *     TODO:
     *         for(...; ...; VAR[t+1] = function_call(VAR[t]))
     */
    ast_node_t* node = nullptr;
    /* Check that the post condition assignment node is a valid assignment */
    if (assignment != nullptr && assignment->type == BLOCKING_STATEMENT && assignment->num_children == 2 && assignment->children[0] != nullptr && assignment->children[1] != nullptr) {
        node = assignment->children[1];
    }
    /* IMPORTANT: If support for more complex post conditions is added, update this inline function */
    if (is_unsupported_post(node, symbol)) {
        *error_code = UNSUPPORTED_POST_CONDITION_NODE;
        return nullptr;
    }
    *error_code = 0;
    return resolve_binary_operation(node);
}

ast_node_t* dup_and_fill_body(ast_node_t* body, ast_node_t* pre, ast_node_t** value, int* error_code) {
    ast_node_t* copy = ast_node_deep_copy(body);
    for (long i = 0; i < copy->num_children; i++) {
        ast_node_t* child = copy->children[i];
        if (child) {
            bool is_unrolled = false;

            if (child->type == IDENTIFIERS) {
                if (!strcmp(child->types.identifier, pre->children[0]->types.identifier)) {
                    ast_node_t* new_num = ast_node_copy(*value);
                    child = free_whole_tree(child);
                    copy->children[i] = new_num;
                }
            } else if (child->type == MODULE_INSTANCE && child->children[0]->type != MODULE_INSTANCE) {
                /* find and replace iteration symbol for port connections and parameters */
                ast_node_t* named_instance = child->children[0];
                copy->children[i]->children[0] = dup_and_fill_body(named_instance, pre, value, error_code);
                free_whole_tree(named_instance);

                is_unrolled = true;
            }

            if (child && child->num_children > 0) {
                if (!is_unrolled) {
                    for (int j = 0; j < copy->children[i]->num_children; j++) {
                        if (copy->children[i]->children[j] != child->children[j]) free_whole_tree(copy->children[i]->children[j]);
                    }

                    copy->children[i] = dup_and_fill_body(child, pre, value, error_code);
                    free_whole_tree(child);
                }
            }
        }
    }
    return copy;
}
