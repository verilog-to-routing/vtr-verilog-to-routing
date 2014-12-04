/*
Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

int simplify_ast();
void optimize_for_tree();
void search_for_node(ast_node_t *root, ast_node_t *list_for_node[], ast_node_t *list_parent[]);
void copy_tree(ast_node_t *node, ast_node_t *new_node);
void complete_node(ast_node_t *node, ast_node_t *new_node);
void record_expression(ast_node_t *node, char *array[]);
int calculation(char *post_exp[]);
void translate_expression(char *exp[],char *post_exp[]);
void check_and_replace(ast_node_t *node, char *p[]);
void modify_expression(char *exp[], char *infix_exp[], char *value);
void free_whole_tree(ast_node_t *node);
void initial_node(ast_node_t *node, ids id, int line_number, int file_number);
void change_to_number_node(ast_node_t *node, char *number);
void reallocate_node(ast_node_t *node, int idx);
void find_most_unique_count();
void mark_node_write(ast_node_t *node, char list[10][20]);
void mark_node_read(ast_node_t *node, char list[10][20]);
void remove_intermediate_variable(ast_node_t *node, char list[10][20]);
void search_marked_node(ast_node_t *node, int is, char *temp, ast_node_t **p);
void free_single_node(ast_node_t *node);
void reduce_assignment_expression();
void find_assign_node(ast_node_t *t, ast_node_t *list[]);
ast_node_t *find_top_module();

typedef struct exp_node
{
	union
	{
		char operation;
		int data;
		char variable[1024];
	}type;
	int id;
	int flag;
	int priority;
	struct exp_node *next;
	struct exp_node *pre;

}enode;

void record_tree_info(ast_node_t *node);
void store_exp_list(ast_node_t *node);
void create_enode(ast_node_t *node);
void simplify_expression(int *build);
void adjoin_constant(int *build);
enode *replace_enode(int data, enode *t, int mark);
void combine_constant(int *build);
void delete_continuous_multiply(int *build);
void construct_new_tree(enode *tail, ast_node_t *node, int line_num, int file_num);
void reduce_enode_list();
enode *find_tail(enode *node);
int check_exp_list(enode *tail);
void create_ast_node(enode *temp, ast_node_t *node, int line_num, int file_num);
void create_op_node(ast_node_t *node, enode *temp, int line_num, int file_num);
void free_exp_list();
int deal_with_bracket(ast_node_t *node);
void recursive_tree(ast_node_t *node, int list_brackets[], int *count_bracket);
void find_leaf_node(ast_node_t *node, int list_brackets[], int *count_bracket, int ids);
void delete_bracket(int beign, int end);
void delete_bracket_head(enode *begin, enode *end);
void change_exp_list(enode *beign, enode *end, enode *s, int flag);
enode *copy_enode_list(enode *new_head, enode *begin, enode *end);
void copy_enode(enode *node, enode *new_node);
void delete_bracket_tail(enode *begin, enode *end);
void delete_bracket_body(enode *begin, enode *end);
void check_tree_operation(ast_node_t *node, int *mark);
void reduce_parameter();
void find_parameter(ast_node_t *top, ast_node_t *para[], int *count);
void remove_para_node(ast_node_t *top, ast_node_t *para[], int num);
void change_para_node(ast_node_t *node, char *name, int value);
void change_ast_node(ast_node_t *node, int value);
void check_operation(enode *begin, enode *end);
void shift_operation();
void search_certain_operation(ast_node_t *node, int module_num);
void check_binary_operation(ast_node_t *node, int module_num);
void check_node_number(ast_node_t *parent, ast_node_t *child, int flag, int module_num);
int check_mult_bracket(int list[], int num_bracket);
void change_bit_size(ast_node_t *node, int module_num, long long size);
void search_var_declare_list(ast_node_t *top, char *name, long long size);
void check_intermediate_variable(ast_node_t *node, int *mark);
void keep_all_branch(ast_node_t *temp_node, ast_node_t *for_parent, int mark);
int check_index(ast_node_t *parent, ast_node_t *for_node);
