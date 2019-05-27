#ifndef LOOP_UNROLL_AST_H
#define LOOP_UNROLL_AST_H

#include "odin_globals.h"
#include "odin_types.h"
#include <stdlib.h>
#include <functional>

/* resolve_pre_condition error codes */
#define UNSUPPORTED_PRE_CONDITION_NODE 1
#define UNSUPPORTED_CONDITION_NODE 2
#define UNSUPPORTED_POST_CONDITION_NODE 3

/* Function Pointer Types */
typedef std::function<bool(long)> condition_function;
typedef std::function<long(long)> post_condition_function;

void unroll_loops(ast_node_t **ast_module);

inline bool is_for_node(ast_node_t* node)
{
    return node && node->type == FOR;
}

/* IMPORTANT: as support for more complex pre conditions is added, update this function. */
inline bool is_unsupported_pre(ast_node_t* node){
    return  (node == nullptr ||
            node->type != BLOCKING_STATEMENT ||
            node->num_children != 2 ||
            node->children[1] == nullptr ||
            node->children[1]->type != NUMBERS);
}
bool is_unsupported_post(ast_node_t* node, ast_node_t* symbol);
bool is_unsupported_condition(ast_node_t* node, ast_node_t* symbol);

ast_node_t* for_preprocessor(ast_node_t *ast_module, ast_node_t* node, ast_node_t ****instances, int *num_unrolled, int *num_original);
ast_node_t* replace_fors(ast_node_t *ast_module, ast_node_t* node, ast_node_t ****instances, int *num_unrolled, int *num_original);
ast_node_t* resolve_for(ast_node_t *ast_module, ast_node_t* node, ast_node_t ****instances, int *num_unrolled, int *num_original);
int resolve_pre_condition(ast_node_t* node, ast_node_t** number);
condition_function resolve_condition(ast_node_t* node, ast_node_t* symbol, int* error_code);
post_condition_function resolve_binary_operation(ast_node_t* node);
post_condition_function resolve_post_condition(ast_node_t* assignment, ast_node_t* symbol, int* error_code);
ast_node_t* dup_and_fill_body(ast_node_t *ast_module, ast_node_t* body, ast_node_t* pre, ast_node_t** value, int* error_code, ast_node_t ****instances, int *num_unrolled, int *num_original);
ast_node_t *replace_named_module(ast_node_t* module, ast_node_t** value);

#endif
