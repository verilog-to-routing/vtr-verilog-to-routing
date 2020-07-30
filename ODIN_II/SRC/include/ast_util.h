#ifndef AST_UTIL_H
#define AST_UTIL_H

#include "odin_types.h"

void add_tag_data(ast_t* ast);

ast_t* allocate_ast();
ast_t* free_ast(ast_t* to_delete);
void add_top_module_to_ast(ast_t* verilog_ast, ast_node_t* to_add);

ast_node_t* create_node_w_type_no_count(ids id, loc_t loc);
ast_node_t* create_node_w_type(ids id, loc_t loc);
ast_node_t* create_tree_node_id(char* string, loc_t loc);

ast_node_t* create_tree_node_string(char* input_number, loc_t loc);
ast_node_t* create_tree_node_number(char* input_number, loc_t loc);
ast_node_t* create_tree_node_number(VNumber& input_number, loc_t loc);
ast_node_t* create_tree_node_number(long input_number, loc_t loc);

void allocate_children_to_node(ast_node_t* node, std::vector<ast_node_t*> children_list);
void add_child_to_node(ast_node_t* node, ast_node_t* child);
void add_child_to_node_at_index(ast_node_t* node, ast_node_t* child, int index);
void remove_child_from_node_at_index(ast_node_t* node, int index);
ast_node_t** expand_node_list_at(ast_node_t** list, long old_size, long to_add, long start_idx);
void move_ast_node(ast_node_t* src, ast_node_t* dest, ast_node_t* node);
ast_node_t* ast_node_deep_copy(ast_node_t* node);
ast_node_t* ast_node_copy(ast_node_t* node);

void free_all_children(ast_node_t* node);
ast_node_t* free_whole_tree(ast_node_t* node);
void free_resolved_children(ast_node_t* node);
ast_node_t* free_resolved_tree(ast_node_t* node);
ast_node_t* free_single_node(ast_node_t* node);
void free_assignement_of_node_keep_tree(ast_node_t* node);

void make_concat_into_list_of_strings(ast_node_t* concat_top, char* instance_name_prefix, sc_hierarchy* local_ref);
void change_to_number_node(ast_node_t* node, long number);
void change_to_number_node(ast_node_t* node, VNumber number);

char* get_identifier(ast_node_t* node);

char* get_name_of_pin_at_bit(ast_node_t* var_node, int bit, char* instance_name_prefix, sc_hierarchy* local_ref);
char* get_name_of_var_declare_at_bit(ast_node_t* var_declare, int bit);
char_list_t* get_name_of_pins(ast_node_t* var_node, char* instance_name_prefix, sc_hierarchy* local_ref);
char_list_t* get_name_of_pins_with_prefix(ast_node_t* var_node, char* instance_name_prefix, sc_hierarchy* local_ref);
long get_size_of_variable(ast_node_t* node, sc_hierarchy* local_ref);

bool node_is_constant(ast_node_t* node);
ast_node_t* fold_binary(ast_node_t** node);
ast_node_t* fold_unary(ast_node_t** node);

long clog2(long value_in, int length);
void c_display(ast_node_t* node);
void c_finish(ast_node_t* node);
long resolve_concat_sizes(ast_node_t* node_top, sc_hierarchy* local_ref);

#endif
