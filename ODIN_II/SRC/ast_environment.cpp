#include "ast_environment.hpp"
#include "odin_types.h"
#include "odin_error.h"
#include "parse_making_ast.h"
#include "ast_util.h"

static ast_node_t* pop_and_grab_back(Path& path)
{
	ast_node_t* back = path.back();
	path.pop_back();
	return back;
}

Path make_path(ast_node_t* node, ast_node_t* module)
{
	Path path;
	ast_node_t* current = node;
	while(current != module){
		path.push_back(current);
		current = current->parent;
	}
	path.push_back(module);
	return path;
}

static bool is_integer_decl(ast_node_t* node)
{
	bool is_int_decl = node; 
	is_int_decl = is_int_decl && node->type == VAR_DECLARE;
	is_int_decl = is_int_decl && node->num_children == 2;
	is_int_decl = is_int_decl && node->children[0] != nullptr;
	is_int_decl = is_int_decl && node->children[0]->type == INTEGER;
	return is_int_decl;
}

static bool is_integer_decl_list(ast_node_t* node)
{
	bool is_int_decl = node; 
	is_int_decl = is_int_decl && node->type == VAR_DECLARE_LIST;
	is_int_decl = is_int_decl && node->num_children >= 3;
	is_int_decl = is_int_decl && node->children[0] != nullptr;
	is_int_decl = is_int_decl && node->children[0]->type == INTEGER;
	return is_int_decl;
}

static bool is_integer_assign(ast_node_t* node, Env env)
{
	bool is_int_assign = node && node->type == BLOCKING_STATEMENT;
	is_int_assign = is_int_assign && node->children[0] != nullptr;
	is_int_assign = is_int_assign && node->children[0]->type == IDENTIFIERS;

	if(is_int_assign){
		std::string id = std::string(node->children[0]->types.identifier);
		is_int_assign = is_int_assign && (env.find(id) == env.end());	
	}
	return is_int_assign;
}

/* Construct from Path */
Environment::Environment(Path path)
	:env(Env())
	, is_init(false)
{
	/* Get the top unsearched scope of the Environment */
	ast_node_t* top = pop_and_grab_back(path);

	/* While there are still scopes to search */
	while(!path.empty()){

		/* Check all children upto (but not including) the next scope level */
		ast_node_t* child = top->children[0];
		for(int i = 1; child != path.back() && i<top->num_children; i++){
			
			/* 
			 * If there is an integer declaration, add it to the Environment
			 * Else if there is an assignment to an integer, update value in Environment 
			 */
			if(is_integer_decl(child)) {
				env[std::string(child->children[1]->types.identifier)] 
					= std::nullopt;
			} else if(is_integer_decl_list(child)) {
				for(int j=1; i<child->num_children; i++){
					env[std::string(child->children[j]->types.identifier)] 
						= std::nullopt;
				}
			} else if(is_integer_assign(child, env)) {
				env[std::string(child->children[0]->types.identifier)] 
					= ast_node_deep_copy(child->children[1]);
			}
			child = top->children[i];
		}
		top = pop_and_grab_back(path);

	}
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
	if(env.find(symbol) == env.end())
		return UNKNOWN_SYMBOL;
	env[symbol] = new_value;
	return SUCCESS;
}

/* (function: get_value) */
Value Environment::get_value(std::string symbol){
	return env[symbol];
}

/* (function: is_in_env) */
bool Environment::is_in_env(std::string symbol){
	return env.find(symbol) == env.end();
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
	oassert(node);
	ast_node_t* to_return = nullptr;
	Value v;
	ast_node_t* temp = nullptr;
	ast_node_t* child_node =nullptr ;
	ast_node_t* body_parent = nullptr;
	int loop_index;
	std::string symbol;
	switch(node->type){
		case IDENTIFIERS:
			v = env.get_value(std::string(node->types.identifier));
			to_return = (v != std::nullopt) ? ast_node_deep_copy(*v) : node;
			break;
		case BINARY_OPERATION:
			to_return = ast_node_deep_copy(node);
			temp = to_return->children[0];
			assign_child_to_node(evaluate_node(to_return->children[0], env), to_return, 0);
			free_whole_tree(temp);
			temp = to_return->children[1];
			assign_child_to_node(evaluate_node(to_return->children[1], env), to_return, 1);
			free_whole_tree(temp);
			if(children_are_numbers(to_return, to_return->num_children)){
				temp = reduce_binary_operation(to_return);
				free_whole_tree(to_return);
				to_return = temp;
			}
			break;
		case BLOCKING_STATEMENT:
			v = env.get_value(node->children[0]->types.identifier);
			to_return = (v != std::nullopt) ? nullptr : node;
			symbol = node->children[0]->types.identifier;
			if(env.is_in_env(symbol)){
				env.update_value(symbol, evaluate_node(to_return->children[1], env));
				free_whole_tree(*v);
			}
			break;
		case WHILE:
			to_return = ast_node_deep_copy(node);
			temp = evaluate_node(to_return->children[0], env);
			if(!is_a_number_node(temp)){
				error_message(
					PARSE_ERROR, 
					temp->line_number, 
					temp->file_number,
					"%s",
					"Unable to simplify condition of while loop at compile time."
				);
			}
			while(temp->types.number.value){
				free_whole_tree(temp);
				for(int i=0; i<to_return->children[1]->num_children; i++){
					child_node = evaluate_node(to_return->children[1]->children[i], env);
					body_parent = body_parent ? 
						newList_entry(body_parent, child_node):
						newList(BLOCK, child_node);
				}

				temp = evaluate_node(to_return->children[0], env);
			}
		case BLOCK:
			to_return = ast_node_deep_copy(node);
			for(loop_index = 0; loop_index < to_return->num_children; loop_index++){
				temp = evaluate_node(to_return->children[loop_index], env);
				if(temp != to_return->children[loop_index]){
					assign_child_to_node(to_return, temp, loop_index);
				}
			}
		default:
			to_return = node;
			break;
	}
	if(node != to_return)
		free_whole_tree(node);
	return to_return;
}
