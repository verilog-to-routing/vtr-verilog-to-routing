/* Standard libraries */
#include <stdlib.h>
#include <string.h>

/* Odin_II libraries */
#include "globals.h"
#include "types.h"
#include "ast_util.h"
#include "parse_making_ast.h"
#include "odin_util.h"

/* This files header */
#include "ast_loop_unroll.h"

/*
 *  (function: unroll_loops)
 */
void unroll_loops()
{
    /* preprocess each module */
	size_t i;
	for (i = 0; i < num_modules; i++)
	{
        ast_node_t* module = for_preprocessor(ast_modules[i]);
        if(module != ast_modules[i])
            free_whole_tree(ast_modules[i]);
        ast_modules[i] = module;
    }
}

/*
 *  (function: for_preprocessor)
 */
ast_node_t* for_preprocessor(ast_node_t* node)
{
    if(!node)
        return nullptr;
    /* If this is a for node, something has gone wrong */
    oassert(!is_for_node(node))
    
    /* If this node has for loops as children, replace them */
    bool for_loops = false;
    for(int i=0; i<node->num_children && !for_loops; i++){
        for_loops = is_for_node(node->children[i]);
    }
    ast_node_t* new_node = for_loops ? replace_fors(node) : node;
    
    /* Run this function recursively on the children */
    for(int i=0; i<new_node->num_children; i++){
        ast_node_t* new_child = for_preprocessor(new_node->children[i]);

        /* Cleanup replaced child */
        if(new_node->children[i] != new_child){
            free_whole_tree(new_node->children[i]);
            new_node->children[i] = new_child;
        }
    }
    return new_node;
}

/*
 *  (function: replace_fors)
 */
ast_node_t* replace_fors(ast_node_t* node)
{
    oassert(!is_for_node(node));
    oassert(node != nullptr);

    ast_node_t* new_node = ast_node_deep_copy(node);
    if(!new_node)
        return nullptr;

    /* process children one at a time */
    for(int i=0; i<new_node->num_children; i++){
        /* unroll `for` children */
        if(is_for_node(new_node->children[i])){
            ast_node_t* unrolled_for = resolve_for(new_node->children[i]);
            oassert(unrolled_for != nullptr);
            free_whole_tree(new_node->children[i]);
            new_node->children[i] = unrolled_for;
        }
    }
    return new_node;
}

/*
 *  (function: resolve_for)
 */
ast_node_t* resolve_for(ast_node_t* node)
{
    oassert(is_for_node(node));
    oassert(node != nullptr);
    ast_node_t* body_parent = nullptr;

    ast_node_t* pre  = node->children[0];
    ast_node_t* cond = node->children[1];
    ast_node_t* post = node->children[2];
    ast_node_t* body = node->children[3];
    
    ast_node_t* value = 0;
    if(resolve_pre_condition(pre, &value))
    {
        error_message(PARSE_ERROR, pre->line_number, pre->file_number, "Unsupported pre-condition node in for loop");
    }

    int error_code = 0;
    condition_function cond_func = resolve_condition(cond, pre->children[0], &error_code);
    if(error_code)
    {
        error_message(PARSE_ERROR, cond->line_number, cond->file_number, "Unsupported condition node in for loop");
    }

    post_condition_function post_func = resolve_post_condition(post, pre->children[0], &error_code);
    if(error_code)
    {
        error_message(PARSE_ERROR, post->line_number, post->file_number, "Unsupported post-condition node in for loop");
    }

    while(cond_func(value->types.number.value))
    {
        ast_node_t* new_body = dup_and_fill_body(body, pre->children[0], &value, &error_code);
        if(error_code)
        {
            error_message(PARSE_ERROR, pre->line_number, pre->file_number, "Unsupported pre-condition node in for loop");
        }
        value->types.number.value = post_func(value->types.number.value);
        body_parent = body_parent ? newList_entry(body_parent, new_body) : newList(BLOCK, new_body);
    }

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
int resolve_pre_condition(ast_node_t* node, ast_node_t** number_node)
{
    /* Add new for loop support here. Keep current work in the TODO
     * Currently supporting:
     * TODO:
     *  for(VAR = NUM; VAR {<, >, ==, <=, >=} NUM; VAR = VAR {+, -, *, /} NUM) STATEMENT
     */
    if( node == nullptr ||
        node->type != BLOCKING_STATEMENT ||
        node->num_children != 2 ||
        node->children[1] == nullptr ||
        node->children[1]->type != NUMBERS)
    {
        return UNSUPPORTED_PRE_CONDITION_NODE;
    }
    *number_node = ast_node_deep_copy(node->children[1]);
    return 0;
}

/*
 *  (function: resolve_condition)
 *  return a lambda which tests the loop condition for a given value
 */
condition_function resolve_condition(ast_node_t* node, ast_node_t* symbol, int* error_code)
{
    /* Add new for loop support here. Keep current work in the TODO
     * Currently supporting:
     * TODO:
     *  for(VAR = NUM; VAR {<, >, ==, !=, <=, >=} NUM; VAR = VAR {+, -, *, /} NUM) STATEMENT
     */
    if( node->type != BINARY_OPERATION ||
        !(  node->types.operation.op == LT ||
	        node->types.operation.op == GT ||
        	node->types.operation.op == LOGICAL_EQUAL ||
        	node->types.operation.op == NOT_EQUAL ||
        	node->types.operation.op == LTE ||
        	node->types.operation.op == GTE ) ||
        node->num_children != 2 ||
        node->children[1] == nullptr ||
        node->children[1]->type != NUMBERS ||
        node->children[0] == nullptr ||
        node->children[0]->type != IDENTIFIERS ||
        strcmp(node->children[0]->types.identifier, symbol->types.identifier))
    {
        *error_code = UNSUPPORTED_CONDITION_NODE;
        return nullptr;
    }
    *error_code = 0;
    return [=](long long value) {
       switch(node->types.operation.op){
           case LT:
                return value <  node->children[1]->types.number.value;
           case GT:
                return value >  node->children[1]->types.number.value;
           case NOT_EQUAL:
                return value != node->children[1]->types.number.value;
           case LOGICAL_EQUAL:
                return value == node->children[1]->types.number.value;
           case LTE:
                return value <= node->children[1]->types.number.value;
           case GTE:
                return value >= node->children[1]->types.number.value;
           default:
               return false;
       }
    };
}

/*
 *  (function: resolve_post_condition)
 *  return a lambda which gives the next value
 *  of the loop variable given the current value
 *  of the loop variable
 */
post_condition_function resolve_post_condition(ast_node_t* assignment, ast_node_t* symbol, int* error_code)
{
    /* Add new for loop support here. Keep current work in the TODO
     * Currently supporting:
     * TODO:
     *  for(VAR = NUM; VAR {<, >, ==, <=, >=} NUM; VAR = VAR {+, -, *, /} NUM) STATEMENT
     */
    ast_node_t* node = nullptr;
    if( assignment != nullptr &&
        assignment->type == BLOCKING_STATEMENT &&
        assignment->num_children == 2 &&
        assignment->children[0] != nullptr &&
        assignment->children[1] != nullptr)
    {
        node = assignment->children[1];
    }
    if( node == nullptr ||
        node->type != BINARY_OPERATION ||
        !(  node->types.operation.op == ADD ||
	        node->types.operation.op == MINUS ||
        	node->types.operation.op == MULTIPLY ||
        	node->types.operation.op == DIVIDE) ||
        node->num_children != 2 ||
        node->children[1] == nullptr ||
        node->children[1]->type != NUMBERS ||
        node->children[0] == nullptr ||
        node->children[0]->type != IDENTIFIERS ||
        strcmp(node->children[0]->types.identifier, symbol->types.identifier))
    {
        *error_code = UNSUPPORTED_POST_CONDITION_NODE;
        return nullptr;
    }
    *error_code = 0;
    return [=](long long value) {
       switch(node->types.operation.op){
           case ADD:
                return value + node->children[1]->types.number.value;
           case MINUS:
                return value - node->children[1]->types.number.value;
           case MULTIPLY:
                return value * node->children[1]->types.number.value;
           case DIVIDE:
                return value / node->children[1]->types.number.value;
           default:
               return 0ll;
       }
    };    
}

ast_node_t* dup_and_fill_body(ast_node_t* body, ast_node_t* symbol, ast_node_t** value, int* error_code)
{
    ast_node_t* copy = ast_node_deep_copy(body);
    for(int i = 0; i<copy->num_children; i++){
        ast_node_t* child = copy->children[i];
        oassert(child);
        if(child->type == IDENTIFIERS){
            if(!strcmp(child->types.identifier, symbol->types.identifier)){
                ast_node_t* new_num = ast_node_deep_copy(*value);
                free_whole_tree(child);
                copy->children[i] = new_num;
            }
        } else if(child->num_children > 0){
            copy->children[i] = dup_and_fill_body(child, symbol, value, error_code);
            oassert(copy->children[i]);
            free_whole_tree(child);
        }
    }
    return copy;
}
