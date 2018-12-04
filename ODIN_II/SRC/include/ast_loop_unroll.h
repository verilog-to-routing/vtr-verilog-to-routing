#ifndef LOOP_UNROLL_AST_H
#define LOOP_UNROLL_AST_H

#include "globals.h"
#include "types.h"
#include <stdlib.h>
#include <functional>

/* resolve_pre_condition error codes */
#define UNSUPPORTED_PRE_CONDITION_NODE 1
#define UNSUPPORTED_CONDITION_NODE 2
#define UNSUPPORTED_POST_CONDITION_NODE 3

/* Function Pointer Types */
typedef std::function<bool(long long)> condition_function;
typedef std::function<long long(long long)> post_condition_function;

void unroll_loops();

inline bool is_for_node(ast_node_t* node)
{
    return node && node->type == FOR;
}

ast_node_t* for_preprocessor(ast_node_t* node);
ast_node_t* replace_fors(ast_node_t* node);
ast_node_t* resolve_for(ast_node_t* node);
int resolve_pre_condition(ast_node_t* node, ast_node_t** number);
condition_function resolve_condition(ast_node_t* node, ast_node_t* symbol, int* error_code);
post_condition_function resolve_post_condition(ast_node_t* assignment, ast_node_t* symbol, int* error_code);
ast_node_t* dup_and_fill_body(ast_node_t* body, ast_node_t* symbol, ast_node_t** value, int* error_code);

#endif
