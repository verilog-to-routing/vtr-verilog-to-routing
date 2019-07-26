/* Standard libraries */
#include <stdlib.h>
#include <string.h>

/* Odin_II libraries */
#include "odin_globals.h"
#include "odin_types.h"
#include "ast_util.h"
#include "parse_making_ast.h"
#include "odin_util.h"

/* This files header */
#include "ast_loop_unroll.h"

/*
 *  (function: unroll_loops)
 */
void unroll_loops(ast_node_t **ast_module)
{
    ast_node_t* module = for_preprocessor(*ast_module);
    if(module != *ast_module)
        free_whole_tree(*ast_module);
    *ast_module = module;
}

/*
 *  (function: for_preprocessor)
 */
ast_node_t* for_preprocessor(ast_node_t* node)
{
    if(!node)
        return nullptr;
    /* If this is a for node, something has gone wrong */
    oassert(!is_for_node(node));
    
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
        error_message(PARSE_ERROR, pre->line_number, pre->file_number, "%s", "Unsupported pre-condition node in for loop");
    }

    int error_code = 0;
    condition_function cond_func = resolve_condition(cond, pre->children[0], &error_code);
    if(error_code)
    {
        error_message(PARSE_ERROR, cond->line_number, cond->file_number, "%s", "Unsupported condition node in for loop");
    }

    post_condition_function post_func = resolve_post_condition(post, pre->children[0], &error_code);
    if(error_code)
    {
        error_message(PARSE_ERROR, post->line_number, post->file_number, "%s", "Unsupported post-condition node in for loop");
    }
    bool dup_body = cond_func(value->types.number.value);
    while(dup_body)
    {
        ast_node_t* new_body = dup_and_fill_body(body, pre->children[0], &value, &error_code);
        if(error_code)
        {
            error_message(PARSE_ERROR, pre->line_number, pre->file_number, "%s", "Unsupported pre-condition node in for loop");
        }
        value->types.number.value = post_func(value->types.number.value);
        body_parent = body_parent ? newList_entry(body_parent, new_body) : newList(BLOCK, new_body);
        dup_body = cond_func(value->types.number.value);
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
     *     for(VAR = NUM; ...; ...) ...
     * TODO:
     *     for(VAR = function(PARAMS...); ...; ...) ...
     */
    /* IMPORTANT: if support for more complex continue conditions is added, update this inline function. */
    if(is_unsupported_pre(node))
    {
        return UNSUPPORTED_PRE_CONDITION_NODE;
    }
    *number_node = ast_node_deep_copy(node->children[1]);
    return 0;
}

/** IMPORTANT: as support for more complex continue conditions is added, update this function. 
 *  (function: is_unsupported_condition)
 *  returns true if, given the supplied symbol, the node can be simplifed 
 *  to true or false if the symbol is replaced with some value.
 */
bool is_unsupported_condition(ast_node_t* node, ast_node_t* symbol){
    bool invalid_inequality = ( node->type != BINARY_OPERATION ||
            !(  node->types.operation.op == LT ||
	            node->types.operation.op == GT ||
        	    node->types.operation.op == LOGICAL_EQUAL ||
        	    node->types.operation.op == NOT_EQUAL ||
        	    node->types.operation.op == LTE ||
        	    node->types.operation.op == GTE) ||
            node->num_children != 2 ||
            node->children[1] == nullptr ||
            !(  node->children[1]->type == NUMBERS ||
                node->children[1]->type == IDENTIFIERS ) ||
            node->children[0] == nullptr ||
            !(  node->children[0]->type == NUMBERS ||
                node->children[0]->type == IDENTIFIERS ));

    bool invalid_logical_concatenation = ( node->type != BINARY_OPERATION ||
            !(  node->types.operation.op == LOGICAL_OR ||
                node->types.operation.op == LOGICAL_AND) ||
            node->num_children != 2 ||
            node->children[1] == nullptr ||
            is_unsupported_condition(node->children[1], symbol) ||
            node->children[0] == nullptr ||
            is_unsupported_condition(node->children[0], symbol));

    bool invalid_negation = ( node->type != UNARY_OPERATION ||
            node->types.operation.op != LOGICAL_NOT ||
            node->num_children != 1 ||
            node->children[0] == nullptr ||
            is_unsupported_condition(node->children[0], symbol));

    bool contains_unknown_symbols = !(invalid_inequality) && (
            (   node->children[0]->type == IDENTIFIERS &&
                strcmp(node->children[0]->types.identifier, symbol->types.identifier)) ||
            (   node->children[1]->type == IDENTIFIERS &&
                strcmp(node->children[1]->types.identifier, symbol->types.identifier)));

    return ((invalid_inequality || contains_unknown_symbols) && invalid_logical_concatenation && invalid_negation);
}

/*
 *  (function: resolve_condition)
 *  return a lambda which tests the loop condition for a given value
 */
condition_function resolve_condition(ast_node_t* node, ast_node_t* symbol, int* error_code)
{
    /* Add new for loop support here. Keep current work in the TODO
     * Currently supporting:
     *     for(...; VAR {<, >, ==, !=, <=, >=} NUM; ...) ...
     *     for(...; CONDITION_A {&&, ||} CONDITION_B;...) ...
     *     for(...; !(CONDITION);...) ...
     * TODO:
     *     for(...; {EXPRESSION_OF_VAR, NUM} {<, >, ==, !=, <=, >=} {EXPRESSION_OF_VAR, NUM}; ...) ...
     */
    /* IMPORTANT: if support for more complex continue conditions is added, update this inline function. */
    if(is_unsupported_condition(node, symbol))
    {
        *error_code = UNSUPPORTED_CONDITION_NODE;
        return nullptr;
    }
    *error_code = 0;
    /* Resursive calls need to report errors before returning a lambda */
    condition_function left = nullptr;
    condition_function right = nullptr;
    condition_function inner = nullptr;
    switch(node->types.operation.op){
        case LOGICAL_OR:
            left = resolve_condition(node->children[0], symbol, error_code);
            if(*error_code)
                return nullptr;
            right = resolve_condition(node->children[1], symbol, error_code);
            if(*error_code)
                return nullptr;
            return [=](long value) {
                return (left(value) || right(value));
            };
        case LOGICAL_AND:
            left = resolve_condition(node->children[0], symbol, error_code);
            if(*error_code)
                return nullptr;
            right = resolve_condition(node->children[1], symbol, error_code);
            if(*error_code)
                return nullptr;
            return [=](long value) {
                return (left(value) && right(value));
            };
        case LOGICAL_NOT:
            inner = resolve_condition(node->children[0], symbol, error_code);
            if(*error_code) 
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


/* IMPORTANT: as support for more complex post conditions is added, update this function. 
 * (function: is_unsupported_post)
 * returns true if the post condition blocking assignment is more complex than
 * can currently be unrolled statically
 */
bool is_unsupported_post(ast_node_t* node, ast_node_t* symbol){
    return  (node == nullptr ||
            node->type != BINARY_OPERATION ||
            !(  node->types.operation.op == ADD ||
                node->types.operation.op == MINUS ||
                node->types.operation.op == MULTIPLY ||
                node->types.operation.op == DIVIDE) ||
            node->num_children != 2 ||
            node->children[1] == nullptr ||
            !(  (   node->children[1]->type == IDENTIFIERS &&
                    !strcmp(node->children[1]->types.identifier, symbol->types.identifier))||
                node->children[1]->type == NUMBERS ||
                !is_unsupported_post(node->children[0], symbol)) ||
            node->children[0] == nullptr ||
            !(  (   node->children[0]->type == IDENTIFIERS &&
                    !strcmp(node->children[0]->types.identifier, symbol->types.identifier))||
                node->children[0]->type == NUMBERS ||
                !is_unsupported_post(node->children[0], symbol)));
}

post_condition_function resolve_binary_operation(ast_node_t* node)
{
    if(node->type == NUMBERS){
        return [=](long value){
            /* 
             * this lambda triggers a warning for unused variable unless
             * we use value to generate a 0
             */
            return node->types.number.value + (value - value);
        };
    } else if (node->type == IDENTIFIERS) {
        return [=](long value){
            return value;
        };
    } else {
        return [=](long value) {
            post_condition_function left_func = resolve_binary_operation(node->children[0]);
            post_condition_function right_func = resolve_binary_operation(node->children[1]);
            switch(node->types.operation.op){
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
post_condition_function resolve_post_condition(ast_node_t* assignment, ast_node_t* symbol, int* error_code)
{
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
    if( assignment != nullptr &&
        assignment->type == BLOCKING_STATEMENT &&
        assignment->num_children == 2 &&
        assignment->children[0] != nullptr &&
        assignment->children[1] != nullptr)
    {
        node = assignment->children[1];
    }
    /* IMPORTANT: If support for more complex post conditions is added, update this inline function */
    if(is_unsupported_post(node, symbol)) 
    {
        *error_code = UNSUPPORTED_POST_CONDITION_NODE;
        return nullptr;
    }
    *error_code = 0;
    return resolve_binary_operation(node);
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
