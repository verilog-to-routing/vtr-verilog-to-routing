#include "types.h"

ast_node_t* create_node_w_type(ids id, int line_number, int file_number);
void free_child_in_tree(ast_node_t *from, int idx_removal);
void free_ast_node(ast_node_t *child);
void free_ast_node_only(ast_node_t *child);

ast_node_t* create_tree_node_id(char* string, int line_number, int file_number);
ast_node_t *create_tree_node_long_long_number(long long number, int constant_bit_size, int line_number, int file_number);
ast_node_t *create_tree_node_number(char* number, int line_number, int file_number);

void allocate_children_to_node(ast_node_t* node, int num_children, ...);
void add_child_to_node(ast_node_t* node, ast_node_t *child);

int get_range(ast_node_t* first_node);

void make_concat_into_list_of_strings(ast_node_t *concat_top, char *instance_name_prefix);
char *get_name_of_pin_at_bit(ast_node_t *var_node, int bit, char *instance_name_prefix);
char *get_name_of_var_declare_at_bit(ast_node_t *var_declare, int bit);
char_list_t *get_name_of_pins(ast_node_t *var_node, char *instance_name_prefix);
char_list_t *get_name_of_pins_with_prefix(ast_node_t *var_node, char *instance_name_prefix);

ast_node_t *resolve_node(char *module_name, ast_node_t *node);
char *make_module_param_name(ast_node_t *module_param_list, char *module_name);
long calculate(long operand0, long operand1, short type);
void move_ast_node(ast_node_t *src, ast_node_t *dest, ast_node_t *node);
