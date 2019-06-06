#ifndef AST_ENVIRONMENT_HPP
#define AST_ENVIRONMENT_HPP

#include <unordered_map>
#include "odin_types.h"

#define SUCCESS 0
#define NO_VALUE_IN_ENV 1
#define UNKNOWN_SYMBOL 2
#define KNOWN_SYMBOL 3

using Value = ast_node_t*;
using Env = std::unordered_map<std::string, Value>;

inline bool is_a_number_node(ast_node_t* node){
	return node->type == NUMBERS;
}

inline bool children_are_numbers(ast_node_t* node, unsigned int children){
	bool are_numbers = true;
	for(unsigned int i = 0; are_numbers && i < children; i++)
		are_numbers &= is_a_number_node(node->children[i]);
	return are_numbers;
}

class Environment{
	private:
		Env env;
		bool is_init;

	public:
		/* Constructors */
		Environment() = delete; // Always build Environments from modules
		Environment(ast_node_t* module);
		Environment(Environment&& to_move) = delete; // Don't move Environments, there is no need.
		Environment(const Environment& to_copy);
		~Environment();
		
		/* Member functions */
		int update_value(std::string, ast_node_t*);
		Value get_value(std::string);
		bool is_in_env(std::string);
};

ast_node_t* evaluate_node(ast_node_t*, Environment&);
#endif /* AST_ENVIRONMENT_HPP */
