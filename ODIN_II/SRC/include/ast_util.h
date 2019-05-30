#include "odin_types.h"

void add_tag_data();

ast_node_t* create_node_w_type_no_count(ids id, int line_number, int file_number);
ast_node_t* create_node_w_type(ids id, int line_number, int file_number);
ast_node_t* create_tree_node_id(char* string, int line_number, int file_number);
ast_node_t *create_tree_node_long_number(long number, int constant_bit_size, int line_number, int file_number);
ast_node_t *create_tree_node_number(std::string number, bases base, signedness sign, int line_number, int file_number);

void initial_node(ast_node_t *node, ids id, int line_number, int file_number, int counter);

void allocate_children_to_node(ast_node_t* node, int num_children, ...);
void add_child_to_node(ast_node_t* node, ast_node_t *child);
void add_child_at_the_beginning_of_the_node(ast_node_t* node, ast_node_t *child);
ast_node_t **expand_node_list_at(ast_node_t **list, long old_size, long to_add, long start_idx);
void move_ast_node(ast_node_t *src, ast_node_t *dest, ast_node_t *node);
ast_node_t *ast_node_deep_copy(ast_node_t *node);

void free_all_children(ast_node_t *node);
ast_node_t *free_whole_tree(ast_node_t *node);
ast_node_t *free_single_node(ast_node_t *node);
void free_assignement_of_node_keep_tree(ast_node_t *node);

char *make_module_param_name(STRING_CACHE *defines_for_module_sc, ast_node_t *module_param_list, char *module_name);
void make_concat_into_list_of_strings(ast_node_t *concat_top, char *instance_name_prefix);
void change_to_number_node(ast_node_t *node, long number);

char *get_name_of_pin_at_bit(ast_node_t *var_node, int bit, char *instance_name_prefix);
char *get_name_of_var_declare_at_bit(ast_node_t *var_declare, int bit);
char_list_t *get_name_of_pins(ast_node_t *var_node, char *instance_name_prefix);
char_list_t *get_name_of_pins_with_prefix(ast_node_t *var_node, char *instance_name_prefix);

ast_node_t *resolve_node(STRING_CACHE *local_param_table_sc, char *module_name, ast_node_t *node);
ast_node_t *resolve_ast_node(STRING_CACHE *local_param_table_sc, short initial, char *module_name, ast_node_t *node);
ast_node_t *node_is_constant(ast_node_t *node);
ast_node_t *node_is_ast_constant(ast_node_t *node);
ast_node_t *node_is_ast_constant(ast_node_t *node, STRING_CACHE *defines_for_module_sc);
ast_node_t * fold_binary(ast_node_t *node);
ast_node_t *fold_unary(ast_node_t *node);

long clog2(long value_in, int length);
