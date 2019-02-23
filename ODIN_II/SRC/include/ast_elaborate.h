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
void search_for_node(ast_node_t *root, std::vector<ast_node_t *> list_for_node, std::vector<ast_node_t *> list_parent);
ast_node_t *get_copy_tree(ast_node_t *node, long virtual_value, std::string virtual_name);
void record_expression(ast_node_t *node, std::vector<std::string> expressions, bool *found_statement);
long  calculation(std::vector<std::string> post_exp);
void translate_expression(std::vector<std::string> infix_exp, std::vector<std::string> postfix_exp);
void reallocate_node(ast_node_t *node, int idx);
void find_most_unique_count();
void mark_node_write(ast_node_t *node, std::vector<std::string> list);
void mark_node_read(ast_node_t *node, std::vector<std::string> list);
void remove_intermediate_variable(ast_node_t *node, std::vector<std::string> list, long virtual_value, std::string virtual_name);
ast_node_t *search_marked_node(ast_node_t *node, int is, std::string temp);
void reduce_assignment_expression();
void find_assign_node(ast_node_t *t, std::vector<ast_node_t *>list);
ast_node_t *find_top_module();

typedef struct exp_node
{
	struct
	{
		short operation;
		int data;
		std::string variable;
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
bool simplify_expression();
bool adjoin_constant();
enode *replace_enode(int data, enode *t, int mark);
bool combine_constant();
bool delete_continuous_multiply();
void construct_new_tree(enode *tail, ast_node_t *node, int line_num, int file_num);
void reduce_enode_list();
enode *find_tail(enode *node);
int check_exp_list(enode *tail);
void create_ast_node(enode *temp, ast_node_t *node, int line_num, int file_num);
void create_op_node(ast_node_t *node, enode *temp, int line_num, int file_num);
void free_exp_list();
bool deal_with_bracket(ast_node_t *node);
void recursive_tree(ast_node_t *node, std::vector<int> list_bracket);
void find_leaf_node(ast_node_t *node, std::vector<int> list_bracket, int ids2);
void delete_bracket(int beign, int end);
void delete_bracket_head(enode *begin, enode *end);
void change_exp_list(enode *beign, enode *end, enode *s, int flag);
enode *copy_enode_list(enode *new_head, enode *begin, enode *end);
void copy_enode(enode *node, enode *new_node);
void delete_bracket_tail(enode *begin, enode *end);
void delete_bracket_body(enode *begin, enode *end);
bool check_tree_operation(ast_node_t *node);
void reduce_parameter();
void find_parameter(ast_node_t *top, std::vector<ast_node_t *>para);
void remove_para_node(ast_node_t *top, std::vector<ast_node_t *>para);
void change_para_node(ast_node_t *node, std::string name, long value);
void check_operation(enode *begin, enode *end);
void shift_operation();
void search_certain_operation(ast_node_t *node);
void check_binary_operation(ast_node_t *node);
void check_node_number(ast_node_t *parent, ast_node_t *child, int flag);
bool check_mult_bracket(std::vector<int> list);
short has_intermediate_variable(ast_node_t *node);
void keep_all_branch(ast_node_t *temp_node, ast_node_t *for_parent, int mark);

