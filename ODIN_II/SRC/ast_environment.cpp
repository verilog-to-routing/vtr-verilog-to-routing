#include "ast_environment.hpp"
#include "odin_types.h"
#include "ast_util.h"

ast_node_t* pop_and_grab_back(Path path)
{
	ast_node_t* back = path.back();
	path.pop_back();
	return back;
}

Path make_path(const ast_node_t* node, const ast_node_t* module)
{
	Path path();
	ast_node_t* current = node;
	while(current != module){
		path.push_back(current);
		current = node.parent;
	}
	path.push_back(module);
	return path;
}

bool is_integer_decl(ast_node_t* node)
{
	/* 
	 * Per the C++ standard, `&=` 
	 * is safe to use as `logical and` for bool
	 * See sections 4.7.4 and 4.12.1 
	 */
	bool is_int_decl = node 
	is_int_decl &= node->type == VAR_DECLARE;
	is_int_decl &= node->num_children == 2;
	is_int_decl &= node->children[0]->type == INTEGER;

}

bool is_integer_decl_list(ast_node_t* node)
{
	/* 
	 * Per the C++ standard, `&=` 
	 * is safe to use as `logical and` for bool
	 * See sections 4.7.4 and 4.12.1 
	 */
	bool is_int_decl = node 
	is_int_decl &= node->type == VAR_DECLARE_LIST;
	is_int_decl &= node->num_children >= 3;
	is_int_decl &= node->children[0]->type == INTEGER;

}

bool is_integer_assign(ast_node_t* node, Env env)
{
	/* 
	 * Per the C++ standard, `&=` 
	 * is safe to use as `logical and` for bool
	 * See sections 4.7.4 and 4.12.1 
	 */
	bool is_int_assign = node && node->type == BLOCKING_STATEMENT;
	is_int_assign &= node->children[0] && node->children[0]->type == IDENTIFIERS;

	/* 
	 * We have now confirmed that this is an 
	 * assignment with a symbol on the left. 
	 * So we now check that that symbol is in
	 * our integer list
	 */
	if(is_int_assign){
		std::string id = std::string(node->children[0]->types.identifier);
		is_int_assign &= (env.find(id) == env.end());	
	}
}

ast_node_t* reduce_binary_operation(ast_node_t* node, Environment& env)
{
	oassert(node);
	oassert(node->type == BINARY_OPERATION);
	ast_node_t* to_return = ast_node_deep_copy(node->children[0]);
	switch(node->types.operation.op){
		//These _should_ always be removed by the time we get 
		//to net list generation; so don't worry about some sort
		//of boolean type for logical operations and just use
		//long long for bool (which is fine as long as we stay in C/C++
		//
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
		case LOGICAL AND:
			//Forgive me for this formating, I just don't know what to do
			//to make this clean without adding more uinitialized variables
			//outside the switch statement.
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

ast_node_t* evaluate_node(ast_node_t* node, Environment& env)
{
	oassert(node);
	ast_node_t* to_return = nullptr;
	Value v;
	ast_node_t* temp;
	std::string symbol;
	switch(node->type){
		case IDENTIFIERS:
			v = env.get_value(node->types.identifier);
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
			if(children_are_numbers(to_return)){
				temp = reduce_binary_operation(to_return);
				free_whole_tree(to_return);
				to_return = temp;
			}
			break;
		case BLOCKING_STATEMENT:
			v = env.get_value(node->types.identifier);
			to_return = (v != std::nullopt) ? nullptr : node;
			symbol = node->types.identifier;
			if(env.is_in_env(symbol)){
				env.update_value(symbol, evaluate_node(to_return->children[1]));
				free_whole_tree(*v);
			}
			break;
		case WHILE:

		default:
			to_return = node;
			break;
	}
	if(node != to_return)
		free_whole_tree(node);
	return to_return;
}

/* Construct from Path */
Environment::Environment(Path path)
	:env(Env())
	:is_init(false)
{
	/* Get the top unsearched scope of the Environment */
	ast_node_t* top = pop_and_grab_back(path);

	/* While there are still scopes to search */
	while(top != nullptr){

		/* Check all children upto (but not including) the next scope level */
		ast_node_t* child = top->children[0]; //Don't need to check as this must be non-null
		while(child != path.back()){
			
			/* 
			 * If there is an integer declaration, add it to the Environment
			 * Else if there is an assignment to an integer, update value in Environment 
			 */
			if(is_integer_decl(child)) {
				env[child->children[1]->types.identifier] = std::nullopt;
			} else if(is_integer_decl_list(child)) {
				for(int i=1; i<child->num_children; i++){
					env[child->children[i]->types.identifier] = std::nullopt;
				}
			} else if(is_integer_assign(child)) {
				env[child->children[0]->types.identify] = 
					child->children[1]->types.number.value;
			}

		}

	}
	to_init - true;
}

/* Copy Constructor */
Environment::Environment(const Environment& to_copy) 
	:env(to_copy.env)
	:is_init(to_copy.init)
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
