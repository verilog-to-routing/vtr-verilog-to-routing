#include "ast_environment.hpp"
#include "odin_types.h"
#include "odin_error.h"
#include "parse_making_ast.h"
#include "ast_util.h"

static bool is_integer_decl(ast_node_t* node)
{
	bool is_int_decl = node; 
	is_int_decl = is_int_decl && node->type == VAR_DECLARE;
	is_int_decl = is_int_decl && node->types.variable.is_integer;
	return is_int_decl;
}

static bool is_integer_decl_list(ast_node_t* node)
{
	bool is_int_decl = node; 
	is_int_decl = is_int_decl && node->type == VAR_DECLARE_LIST;
	for(int i=0; i<node->num_children; i++){
		is_int_decl = is_int_decl && is_integer_decl(node->children[i]);
	}
	return is_int_decl;
}

static void add_int_to_env(ast_node_t* node, Env& env)
{
	if(!node)
		return;
	if(is_integer_decl(node)) {
		env[std::string(node->children[0]->types.identifier)] 
			= std::nullopt;
		remove_ast_node(node->parent, node);
	} else if(is_integer_decl_list(node)) {
		for(int i=0; i<node->num_children; i++){
			env[std::string(node->children[i]->children[0]->types.identifier)] 
				= std::nullopt;
		}
		remove_ast_node(node->parent, node);
	} else {
		for(int i=0; i<node->num_children; i++){	
			add_int_to_env(node->children[i], env);
		}
	}
}

Environment::Environment(ast_node_t* module)
	:env(Env())
	, is_init(false)
{
	add_int_to_env(module, env);
	is_init = true;
}

/* Copy Constructor */
Environment::Environment(const Environment& to_copy) 
	:env(to_copy.env)
	, is_init(to_copy.is_init)
{}

/* Destructor */
Environment::~Environment()
{
	//Free all nodes in the environment (they are not part of the ast)
}

/* (function: update_value) */
int Environment::update_value(std::string symbol, ast_node_t* new_value){
	if(!is_in_env(symbol))
		return UNKNOWN_SYMBOL;
	env[symbol] = new_value;
	return SUCCESS;
}

/* (function: get_value) */
Value Environment::get_value(std::string symbol){
	if(!is_in_env(symbol))
		return std::nullopt;
	return env[symbol];
}

/* (function: is_in_env) */
bool Environment::is_in_env(std::string symbol){
	bool contains_symbol = false;
	for(auto it = env.begin(); it != env.end() && !contains_symbol; ++it)
		contains_symbol = (it->first == symbol);
	return contains_symbol;
}

static ast_node_t* reduce_binary_operation(ast_node_t* node)
{
	oassert(node);
	oassert(node->type == BINARY_OPERATION);
	ast_node_t* to_return = ast_node_deep_copy(node->children[0]);
	switch(node->types.operation.op){
		/* These _should_ always be removed by the time we get 
		 * to net list generation; so don't worry about some sort
		 * of boolean type for logical operations and just use
		 * long long for bool (which is fine as long as we stay in C/C++
		 */

		//TODO: Do not assume that all numbers are the same format
		//in the future, this is temporary for prototyping
		case LT:
			to_return->types.number.value = 
				node->children[0]->types.number.value < node->children[1]->types.number.value;
			break;
		case GT:
			to_return->types.number.value = 
				node->children[0]->types.number.value > node->children[1]->types.number.value;
			break;
		case LTE:
			to_return->types.number.value = 
				node->children[0]->types.number.value <= node->children[1]->types.number.value;
			break;
		case GTE:
			to_return->types.number.value = 
				node->children[0]->types.number.value >= node->children[1]->types.number.value;
			break;
		case NOT_EQUAL:
			to_return->types.number.value = 
				node->children[0]->types.number.value != node->children[1]->types.number.value;
			break;
		case LOGICAL_EQUAL:
			to_return->types.number.value = 
				node->children[0]->types.number.value == node->children[1]->types.number.value;
			break;
		case LOGICAL_OR:
			to_return->types.number.value = 
				(node->children[0]->types.number.value)? 
				(node->children[0]->types.number.value): 
				(node->children[1]->types.number.value);
			break;
		case LOGICAL_AND:
			/* Forgive me for this formating, I just don't know what to do
			 * to make this clean without adding more uinitialized variables
			 * outside the switch statement.
			 */
			to_return->types.number.value = 
				(node->children[0]->types.number.value &&
				 node->children[1]->types.number.value) ? 0 : 1;
			break;
		case LOGICAL_NOT:
			to_return->types.number.value = 
				(node->children[0]->types.number.value) ? 0 : 1;
			break;
		case ADD:
			to_return->types.number.value += node->children[1]->types.number.value;
			break;
		case MINUS:
			to_return->types.number.value -= node->children[1]->types.number.value;
			break;
		case MULTIPLY:
			to_return->types.number.value *= node->children[1]->types.number.value;
			break;
		case DIVIDE:
			to_return->types.number.value /= node->children[1]->types.number.value;
			break;
		default:
			//Shouldn't have missed any cases but this is here to catch it if we do
			oassert(0);
	}
	return to_return;
}

/**
 *  (function: evaluate_node)
 *  returns a (possibly null) pointer to an ast_node_t.
 *  If the return value is the same as the given `node`,
 *  no simplification could occur.
 *  If the return value is a pointer to a different ast_node_t than `node`
 *  then the subtree was simplified.
 *  If the return value is a `nullptr` then the given node updated a 
 *  compile time variable, which is now updated in the provided Environment.
 */
ast_node_t* evaluate_node(ast_node_t* node, Environment& env)
{
	if(!node)
		return nullptr;
	ast_node_t* to_return = nullptr;
	Value v = std::nullopt;
	ast_node_t* temp = nullptr;
	ast_node_t* child_node =nullptr ;
	ast_node_t* body_parent = nullptr;
	std::string symbol;
	switch(node->type){
		case IDENTIFIERS:
			v = env.get_value(std::string(node->types.identifier));
			to_return = (v != std::nullopt) ? ast_node_deep_copy(*v) : node;
			break;
		case BINARY_OPERATION:
			to_return = ast_node_deep_copy(node);
			temp = to_return->children[0];
			assign_child_to_node(to_return, evaluate_node(ast_node_deep_copy(to_return->children[0]), env), 0);
			if(temp != to_return->children[0])
				free_whole_tree(temp);
			temp = to_return->children[1];
			assign_child_to_node(to_return, evaluate_node(ast_node_deep_copy(to_return->children[1]), env), 1);
			if(temp != to_return->children[1])
				free_whole_tree(temp);
			if(children_are_numbers(to_return, to_return->num_children)){
				temp = reduce_binary_operation(to_return);
				free_whole_tree(to_return);
				to_return = temp;
			}
			break;
		case BLOCKING_STATEMENT:
			if(node->children[0]->type == IDENTIFIERS) {
				if(env.is_in_env(std::string(node->children[0]->types.identifier))){
					env.update_value(
						node->children[0]->types.identifier, 
						evaluate_node(ast_node_deep_copy(node->children[1]), env));
					free_whole_tree(*v);
				} else {
					goto default_case; //forgive me father for I have sinned
				}
			} else {
				goto default_case; //forgive me father for I have sinned
			}
			break;
		case WHILE:
			to_return = ast_node_deep_copy(node);
			temp = evaluate_node(ast_node_deep_copy(to_return->children[0]), env);
			if(!is_a_number_node(temp)){
				error_message(
					PARSE_ERROR, 
					node->line_number, 
					node->file_number,
					"%s",
					"Unable to simplify condition of while loop at compile time."
				);
			}
			while(temp->types.number.value){
				for(int i=0; i<to_return->children[1]->num_children; i++){
					child_node = evaluate_node(ast_node_deep_copy(to_return->children[1]->children[i]), env);
					if(child_node) {
						body_parent = body_parent ? 
							newList_entry(body_parent, child_node):
							newList(BLOCK, child_node);
					}
				}
				free_whole_tree(temp);
				temp = evaluate_node(ast_node_deep_copy(to_return->children[0]), env);
			}
			free_whole_tree(to_return);
			to_return = body_parent;
			break;
		default:
		default_case:
			to_return = ast_node_deep_copy(node);
			for(int i=0; i<to_return->num_children; i++){
				child_node = evaluate_node(ast_node_deep_copy(to_return->children[i]), env);
				if(child_node){
					assign_child_to_node(to_return, child_node, i);
				} else if(to_return->children[i]){
					remove_ast_node(to_return, to_return->children[i]);
					i--;
				}
			}
			break;
	}
	if(node != to_return)
		free_whole_tree(node);
	return to_return;
}
