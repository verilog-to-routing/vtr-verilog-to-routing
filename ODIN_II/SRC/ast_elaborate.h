int simplify_ast();
void optimize_for_tree();
void search_for_node(ast_node_t *root, ast_node_t *list_for_node[], ast_node_t *list_parent[], int idx[]);
void copy_tree(ast_node_t *node, ast_node_t *new_node);
void complete_node(ast_node_t *node, ast_node_t *new_node);
void record_expression(ast_node_t *node, char *array[]);
int calculation(char list[]);
void translate(char str[],char exp[]);
void check_and_replace(ast_node_t *node, char *p[]);
void modify_expression(char *exp[], char s_exp[]);
void free_whole_tree(ast_node_t *node);
void initial_node(ast_node_t *node, ids id, int line_number, int file_number);
void change_to_number_node(ast_node_t *node, char *number);
void reallocate_node(ast_node_t *node, int idx);
void find_most_unique_count(ast_node_t *node);
void mark_node_write(ast_node_t *node, char list[10][20]);
void mark_node_read(ast_node_t *node, char list[10][20]);
void remove_intermediate_variable(ast_node_t *node, char list[10][20], int from);
void search_marked_node(ast_node_t *node, int is, char *temp, ast_node_t **p);
void free_single_node(ast_node_t *node);
void reduce_assignment_expression();
void find_assign_node(ast_node_t *t, ast_node_t *list[]);

typedef struct exp_node
{
	union
	{
		char operation;
		int data;
		char variable[10];
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
void deal_with_bracket(ast_node_t *node);
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


