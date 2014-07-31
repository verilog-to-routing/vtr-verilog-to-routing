/*
Copyright (c) 2009 Peter Andrew Jamieson (jamieson.peter@gmail.com)

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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "ast_util.h"
#include "ast_elaborate.h"
#include "parse_making_ast.h"
#include "verilog_bison.h"
#include "verilog_bison_user_defined.h"
#include "util.h"
#include "netlist_create_from_ast.h"
#include "ctype.h"

#define read_node  1
#define write_node 2

#define e_data  1
#define e_operation 2
#define e_variable 3

#define N 64
#define Max_size 64

int num_for = 0;
char *v_name;
long long v_value;
int count_id = 0;
int count_assign = 0;
int count;
int count_write;
enode *head, *p;

int simplify_ast()
{
	//reduce_parameter();
	//reduce_assignment_expression();
	optimize_for_tree();
	reduce_assignment_expression();
	//ast_node_t *top = find_top_module();

	return 1;
}

/*---------------------------------------------------------------------------
 * (function: optimize_for_tree)
 * simplify the FOR loop syntax tree
 *-------------------------------------------------------------------------*/
void optimize_for_tree()
{
	ast_node_t *top;
	ast_node_t *T;
	int i, j;

	ast_node_t *list_for_node[N] = {0};
	ast_node_t *list_parent[N] = {0};
	int idx[N] = {0};

	/* we will find the top module */
	for (i = 0; i < num_modules; i++)
	{
		top = ast_modules[i];
		/* search the tree looking for FOR node */
		search_for_node(top, list_for_node, list_parent, idx);

		find_most_unique_count(top);

		/* simplify every FOR node */
		for (j = 0; j < num_for; j++)
		{
			int initial, terminal, from;
			char *expression[Max_size]= {0};
			char infix_expression[Max_size] = {0};
			char suffix_expression[Max_size] = {0};
			char node_write[10][20];
			count_write = 0;
			count = 0;
			memset(node_write, 0, sizeof(node_write));
			v_name = (char*)malloc(sizeof(list_for_node[j]->children[0]->children[0]->types.identifier)+1);
			sprintf(v_name, "%s", list_for_node[j]->children[0]->children[0]->types.identifier);
			initial = list_for_node[j]->children[0]->children[1]->types.number.value;
			v_value = initial;
			terminal = list_for_node[j]->children[1]->children[1]->types.number.value;
			record_expression(list_for_node[j]->children[2], expression);
			mark_node_write(list_for_node[j]->children[3], node_write);
			mark_node_read(list_for_node[j]->children[3], node_write);
			from = list_parent[j]->num_children - 1;

			while(v_value < terminal)
			{
				T  = (ast_node_t*)malloc(sizeof(ast_node_t));
				copy_tree((list_for_node[j])->children[3], T);
				add_child_to_node(list_parent[j], T);
				modify_expression(expression, infix_expression);
				translate(infix_expression, suffix_expression);
				v_value = calculation(suffix_expression);
				memset(infix_expression, 0, Max_size);
				memset(suffix_expression, 0, Max_size);

			}

			reallocate_node(list_parent[j], idx[j]);
			remove_intermediate_variable(list_parent[j], node_write, from);
			free(v_name);
		}
	}
}

/*---------------------------------------------------------------------------
 * (function: search_for_node)
 * search the tree looking for FOR node
 *-------------------------------------------------------------------------*/
void search_for_node(ast_node_t *root, ast_node_t *list_for_node[], ast_node_t *list_parent[], int idx[])
{
	int i, j;
	if ((root == NULL)||(root->num_children == 0))
		return;

	for(i = 0; i < root->num_children; i++)
		search_for_node(root->children[i], list_for_node, list_parent, idx);

	for (j = 0; j < root->num_children; j++)
	{
		if (root->children[j] == NULL)
			continue;
		else
		{
			if (root->children[j]->type == FOR)
			{
				list_for_node[num_for] = root->children[j];
				list_parent[num_for] = root;
				idx[num_for] = j;
				num_for++;
			}
		}
	}


}

/*---------------------------------------------------------------------------
 * (function: copy_tree)
 *-------------------------------------------------------------------------*/
void copy_tree(ast_node_t *node, ast_node_t *new_node)
{
	int i, n, len;
	char number[12] = {0};
	if (node == NULL)
		new_node = NULL;

	else
	{
		n = node->num_children;
		switch (node->type)
		{
			case IDENTIFIERS:
				if(strcmp(node->types.identifier, v_name) == 0)
				{
					sprintf(number, "%lld", v_value);
					initial_node(new_node, NUMBERS, node->line_number, node->file_number);
					complete_node(node, new_node);
					change_to_number_node(new_node, number);
				}
				else
				{
					len = sizeof(node->types.identifier);
					new_node->types.identifier = (char*)malloc(len+1);
					initial_node(new_node, node->type, node->line_number, node->file_number);
					strcpy(new_node->types.identifier, node->types.identifier);
					complete_node(node, new_node);
				}
				break;

			case NUMBERS:
				initial_node(new_node, node->type, node->line_number, node->file_number);
				new_node->types.number = node->types.number;
				len = sizeof(node->types.number.number);
				new_node->types.number.number = (char*)malloc(len+1);
				strcpy(new_node->types.number.number, node->types.number.number);
				len = sizeof(node->types.number.binary_string);
				new_node->types.number.binary_string = (char*)malloc(len+1);
				strcpy(new_node->types.number.binary_string, node->types.number.binary_string);
				complete_node(node, new_node);
				break;

			default:
				initial_node(new_node, node->type, node->line_number, node->file_number);
				new_node->types = node->types;
				complete_node(node, new_node);
				break;
		}

		if (n == 0)
		new_node->children = NULL;

		else
		{
			new_node->children = (ast_node_t**)malloc(n*sizeof(ast_node_t*));
			for(i=0; i<n; i++)
			{
				if (node->children[i] != NULL)
				{
					new_node->children[i] = (ast_node_t*)malloc(sizeof(ast_node_t));
					copy_tree(node->children[i], new_node->children[i]);
				}
				else
					new_node->children[i] = NULL;
			}
		}
	}

}

/*---------------------------------------------------------------------------
 * (function: complete_node)
 *-------------------------------------------------------------------------*/
void complete_node(ast_node_t *node, ast_node_t *new_node)
{
	new_node->num_children = node->num_children;
	new_node->far_tag = node->far_tag;
	new_node->high_number = node->high_number;
	new_node->shared_node = node->shared_node;
	new_node->hb_port = node->hb_port;
	new_node->net_node = node->net_node;
	new_node->is_read_write = node->is_read_write;
	new_node->additional_data = node->additional_data;
}

/*---------------------------------------------------------------------------
 * (function: expression)
 *-------------------------------------------------------------------------*/
void record_expression(ast_node_t *node, char *array[])
{
	if (node == NULL)
		return;
	if (node->num_children == 0)
	{
		check_and_replace(node, array);
		return;
	}
	record_expression(node->children[0], array);
	check_and_replace(node, array);
	record_expression(node->children[1], array);

}

/*---------------------------------------------------------------------------
 * (function: calculation)
 *-------------------------------------------------------------------------*/
int calculation(char expression[])
{
	struct
	{
		int data[Max_size];
		int top;
	}st;
	int d;
	char ch;
	int t = 0;
	st.top = -1;
	ch = expression[t];
	t++;
	while(ch != '\0')
	{
		switch(ch)
		{
			case '+':
				st.data[st.top-1] = st.data[st.top-1]+st.data[st.top];
				st.top--;
				break;
			case '-':
				st.data[st.top-1] = st.data[st.top-1]-st.data[st.top];
				st.top--;
				break;
			case '*':
				st.data[st.top-1] = st.data[st.top-1]*st.data[st.top];
				st.top--;
				break;
			case '/':
				if(st.data[st.top] != 0)
				st.data[st.top-1]=st.data[st.top-1]/st.data[st.top];
				st.top--;
				break;
			   	default:
			    d=0;
			    while(ch >= '0'&&ch <= '9')
			    {
			    	d = 10*d+ch-'0';
			    	ch = expression[t];
			    	t++;
			    }
				  	st.top++;
				  	st.data[st.top] = d;
				  	break;
		}
	            ch = expression[t];
	            t++;
	}
	return st.data[st.top];
}

/*---------------------------------------------------------------------------
 * (function: translate)
 * translate infix expression into suffix notation
 *-------------------------------------------------------------------------*/
void translate(char str[],char exp[])
{
	struct
	{
		char data[Max_size];
		int top;
	}op;
	char ch;
	int i = 0,t = 0;
	op.top = -1;
	ch = str[i];
	i++;
	while(ch != '\000')
	{
		switch(ch)
		{
			case '(':
				op.top++;op.data[op.top]=ch;
				break;
			case ')':
				while(op.data[op.top] != '(')
				{
					exp[t]=op.data[op.top];
					op.top--;
					t++;
				}
				op.top--;
				break;
			case '+':
			case '-':
				while(op.top != -1&&op.data[op.top] != '(')
				{
					exp[t] = op.data[op.top];
					op.top--;
					t++;
				}
				op.top++;
				op.data[op.top] = ch;
				break;
			case '*':
			case '/':
				while(op.top == '/'||op.top == '*')
				{
					exp[t] = op.data[op.top];
					op.top--;
					t++;
				}
				op.top++;
				op.data[op.top] = ch;
				break;
			default:
			while(ch >= '0'&&ch <= '9')
			{
				exp[t] = ch;t++;
				ch = str[i];i++;
			}
			i--;
			exp[t] = '#';
			t++;
			break;
		}
		  ch = str[i];
		  i++;
	}
	while(op.top != -1)
	{
		exp[t] = op.data[op.top];
		t++;
		op.top--;
	}
	exp[t] = '\000';
}

/*---------------------------------------------------------------------------
 * (function: check_and_replace)
 *-------------------------------------------------------------------------*/
void check_and_replace(ast_node_t *node, char *p[])
{
	switch(node->type)
	{
		case IDENTIFIERS:
				//sprintf((p+count++), "%s", node->types.identifier);
				p[count++] = node->types.identifier;
				break;

		case BLOCKING_STATEMENT:
			//sprintf((p+count++), "%s", "=");
			p[count++] = strdup("=");
			break;

		case NUMBERS:
			//sprintf((p+count++), "%lld", node->types.number.value);
			p[count++] = node->types.number.number;
			break;

		case BINARY_OPERATION:
			switch(node->types.operation.op)
			{
				case ADD:
					//sprintf((p+count++), "%s", "+");
					p[count++] = strdup("+");
					break;
				case MINUS:
					//sprintf((p+count++), "%s", "-");
					p[count++] = strdup("-");
					break;
				case MULTIPLY:
					//sprintf((p+count++), "%s", "*");
					p[count++] = strdup("*");
					break;
				case DIVIDE:
					//sprintf((p+count++), "%s", "/");
					p[count++] = strdup("/");
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}

}

/*---------------------------------------------------------------------------
 * (function: modify_expression)
 *-------------------------------------------------------------------------*/
void modify_expression(char *exp[], char s_exp[])
{
	int i = 0;
	int j;
	int k = 0;

	while((strcmp(exp[i], "=") != 0) && i < Max_size)
		i++;

	for (j = i + 1; exp[j] != NULL; j++)
	{
		if (strcmp(exp[j], v_name) == 0)
			sprintf(&s_exp[k++], "%lld", v_value);

		else
			sprintf(&s_exp[k++], "%s", exp[j]);

	}

}

/*---------------------------------------------------------------------------
 * (function: free_whole_tree)
 *-------------------------------------------------------------------------*/
void free_whole_tree(ast_node_t *node)
{
	int i;

	if (node == NULL)
		return;

	if (node->num_children != 0)
	{
		for (i = 0; i < node->num_children; i++)
			free_whole_tree(node->children[i]);
	}

	if (node->children != NULL)
		free(node->children);

	switch(node->type)
		{
			case IDENTIFIERS:
				if (node->types.identifier != NULL)
					free(node->types.identifier);
				break;
			case NUMBERS:
				if (node->types.number.number != NULL)
					free(node->types.number.number);
				if (node->types.number.binary_string != NULL)
					free(node->types.number.binary_string);
				break;
			default:
				break;
		}

	free(node);

}

/*---------------------------------------------------------------------------
 * (function: initial_node)
 *-------------------------------------------------------------------------*/
void initial_node(ast_node_t *new_node, ids id, int line_number, int file_number)
{
	new_node->type = id;
	new_node->children = NULL;
	new_node->num_children = 0;
	new_node->unique_count = ++count_id;
	new_node->line_number = line_number;
	new_node->file_number = file_number;
	new_node->far_tag = 0;
	new_node->high_number = 0;
	new_node->shared_node = 0;
	new_node->hb_port = 0;
	new_node->net_node = 0;
	new_node->is_read_write = 0;
	new_node->additional_data = 0;

}

/*---------------------------------------------------------------------------
 * (function: change_to_number_node)
 *-------------------------------------------------------------------------*/
void change_to_number_node(ast_node_t *node, char *number)
{
	char *string_pointer = number;
	int index_string_pointer = 0;
	short flag_constant_decimal = FALSE;

	for (string_pointer=number; *string_pointer; string_pointer++)
	{
		if (*string_pointer == '\'')
		{
			break;
		}
		index_string_pointer++;
	}

	if (index_string_pointer == (int) strlen(number))
	{
		flag_constant_decimal = TRUE;
		node->types.number.base = DEC;
		string_pointer = number;
		node->types.number.size = strlen((string_pointer));
		node->types.number.number = strdup((string_pointer));
	}

	if (flag_constant_decimal == FALSE)
	{
		node->types.number.binary_size = node->types.number.size;
	}

	else
	{
		if (strcmp(node->types.number.number, "0") != 0)
		{
			node->types.number.binary_size = ceil((log(convert_dec_string_of_size_to_long_long(node->types.number.number, node->types.number.size)+1))/log(2));
		}
		else
		{
			node->types.number.binary_size = 1;
		}
	}

	node->types.number.value = convert_dec_string_of_size_to_long_long(node->types.number.number, node->types.number.size);
	node->types.number.binary_string = convert_long_long_to_bit_string(node->types.number.value, node->types.number.binary_size);

}

/*---------------------------------------------------------------------------
 * (function: reallocate_node)
 *-------------------------------------------------------------------------*/
void reallocate_node(ast_node_t *node, int idx)
{
	int i;

	free_whole_tree(node->children[idx]);

	for (i = idx; i < node->num_children; i++)
		node->children[i] = node->children[i+1];

	node->num_children = node->num_children - 1;
}

/*---------------------------------------------------------------------------
 * (function: find_most_unique_count)
 *-------------------------------------------------------------------------*/
void find_most_unique_count(ast_node_t *node)
{
	int i;
	if (node == NULL)
		return;

	if (count_id < node->unique_count)
		count_id = node->unique_count;

	if (node->num_children != 0)
	{
		for (i = 0; i < node->num_children; i++)
			find_most_unique_count(node->children[i]);
	}

}

/*---------------------------------------------------------------------------
 * (function: mark_node_write)
 * mark the node that is write
 *-------------------------------------------------------------------------*/
void mark_node_write(ast_node_t *node, char list[10][20])
{
	int i;
	if (node == NULL)
		return;

	if (node->type == BLOCKING_STATEMENT && node->types.operation.op == NO_OP)
		if (node->children[0]->type == IDENTIFIERS)
		{
			node->children[0]->is_read_write = 2;
			sprintf(list[count_write++], "%s", node->children[0]->types.identifier);

		}

    if (node->num_children != 0)
    	for (i = 0; i < node->num_children; i++)
    		mark_node_write(node->children[i], list);

}

/*---------------------------------------------------------------------------
 * (function: mark_node_read)
 * mark the node that is read
 *-------------------------------------------------------------------------*/
void mark_node_read(ast_node_t *node, char list[10][20])
{
	int i, j;
	if (node == NULL)
		return;

	if (node->type == IDENTIFIERS)
	{
		for (i = 0; i < 10; i++)
			if (strcmp(node->types.identifier, list[i]) == 0 && node->is_read_write != 2)
				node->is_read_write = 1;
	}

	if (node->num_children != 0)
		for (j = 0; j < node->num_children; j++)
			mark_node_read(node->children[j], list);

}

/*---------------------------------------------------------------------------
 * (function: remove_intermediate_variable)
 * remove the intermediate variables, and prune the syntax tree of FOR loop
 *-------------------------------------------------------------------------*/
void remove_intermediate_variable(ast_node_t *node, char list[10][20], int from)
{
	int i, j, k, n;
	ast_node_t *write = NULL;
	ast_node_t *read = NULL;
	ast_node_t *new_node;
	char *temp;
	k = 0;

	for (i =  from; i < node->num_children-1; i++)
	{
		for (j = 0; j < count_write; j++)
		{
			temp = (char*)malloc(sizeof(char)*20);
			new_node = (ast_node_t *)malloc(sizeof(ast_node_t));
			sprintf(temp, "%s", list[j]);
			search_marked_node(node->children[i], 2, temp, &write);
			search_marked_node(node->children[i+1], 1, temp, &read);
			for (n = 0; n < read->num_children; n++)
				if (read->children[n]->is_read_write == 1)
				{
					copy_tree(write->children[1], new_node);
					free_single_node(read->children[n]);
					read->children[n] = new_node;

				}

			free(temp);

		}

		free_whole_tree(node->children[i]);
		node->children[i] = NULL;
	}

	for (i = 0; i < node->num_children; i++)
		if (node->children[i] != NULL)
			node->children[k++] = node->children[i];

	for (i = k; i < node->num_children; i++)
		node->children[i] = NULL;

	node->num_children = k;

}

/*---------------------------------------------------------------------------
 * (function: search_marked_node)
 * search the marked nodes as requirement
 *-------------------------------------------------------------------------*/
void search_marked_node(ast_node_t *node, int is, char *temp, ast_node_t **p)
{
	int i;
	if (node == NULL)
		return;

	if(node->num_children != 0)
	{
		for (i = 0; i < node->num_children; i++)
		{
			if (node->children[i]->type == IDENTIFIERS)
			{
				if( strcmp(node->children[0]->types.identifier, temp) == 0 && node->children[0]->is_read_write == is)
					*p = node;
			}

			else
				search_marked_node(node->children[i], is, temp, p);

		}
	}

}

/*---------------------------------------------------------------------------
 * (function: free_single_node)
 * free a node whose type is IDENTIFERS
 *-------------------------------------------------------------------------*/
void free_single_node(ast_node_t *node)
{
	if (node == NULL)
		return;

	if (node->children != NULL)
		free(node->children);

	if (node->type == IDENTIFIERS)
		if (node->types.identifier != NULL)
			free(node->types.identifier);

	free(node);

}

/*---------------------------------------------------------------------------
 * (function: reduce_assignment_expression)
 * reduce the number nodes which can be calculated to optimize the AST
 *-------------------------------------------------------------------------*/
void reduce_assignment_expression()
{
	int i, j, build, mark, *n;
	n = &mark;
	int line_num, file_num;
	head = NULL;
	p = NULL;
	int *bd = &build;
	enode *tail;
	ast_node_t *T = NULL;
	ast_node_t *list_assign[10240] = {0};
	ast_node_t *top = NULL;
	for (i = 0; i < num_modules; i++)
	{
		top = ast_modules[i];
		count_assign = 0;
		find_assign_node(top, list_assign);
		count_id = 0;
		find_most_unique_count(top);
		for (j = 0; j < count_assign; j++)
		{
			mark = 0;
			check_tree_operation(list_assign[j]->children[1], n);
			if ((list_assign[j]->children[1]->num_children != 0) && (mark == 1))
			{
				build = 0;
				store_exp_list(list_assign[j]->children[1]);
				deal_with_bracket(list_assign[j]->children[1]);
				simplify_expression(bd);
				if (build == 1)
				{
					tail = find_tail(head);
					free_whole_tree(list_assign[j]->children[1]);
					line_num = list_assign[j]->line_number;
					file_num = list_assign[j]->file_number;
					T = (ast_node_t*)malloc(sizeof(ast_node_t));
					construct_new_tree(tail, T, line_num, file_num);
					list_assign[j]->children[1] = T;
				}
				free_exp_list();
			}
		}
	}
}

/*---------------------------------------------------------------------------
 * (function: search_assign_node)
 * find the top of the assignment expression
 *-------------------------------------------------------------------------*/
void find_assign_node(ast_node_t *node, ast_node_t *list[])
{
	int i;
	if ((node == NULL) || (node->num_children == 0))
		return;

	if (node->type == BLOCKING_STATEMENT)
		list[count_assign++] = node;

	for (i = 0; i < node->num_children; i++)
		find_assign_node(node->children[i], list);

}

void simplify_expression(int *build)
{
	adjoin_constant(build);
	reduce_enode_list();
	delete_continuous_multiply(build);
	combine_constant(build);
	reduce_enode_list();

}

enode *find_tail(enode *node)
{
	enode *temp;
	enode *tail = NULL;
	for (temp = node; temp != NULL; temp = temp->next)
		if (temp->next == NULL)
			tail = temp;

	return tail;
}

void reduce_enode_list()
{
	enode *temp;
	int a;
	for (temp = head; temp != NULL; temp = temp->next)
	{
		if (temp == head)
		{
			if ((temp->type.data == 0) && (temp->next->priority == 2))
			{
				head = temp->next->next;
				free(temp->next);
				free(temp);
			}
		}
		else
			if ((temp->flag == 1) && (temp->pre->priority == 2))
			{
				if (temp->type.data == 0)
				{
					if (temp->next == NULL)
						temp->pre->pre->next = NULL;
					else
					{
						temp->pre->pre->next = temp->next;
						temp->next->pre = temp->pre->pre;
					}
					free(temp->pre);
					free(temp);
				}
				if (temp->type.data < 0)
				{
					if (temp->pre->type.operation == '+')
						temp->pre->type.operation = '-';
					else
						temp->pre->type.operation = '+';
					a = temp->type.data;
					temp->type.data = -a;
				}
			}
	}
}

void store_exp_list(ast_node_t *node)
{
	enode *temp;
	head = (enode*)malloc(sizeof(enode));
	p = head;
	record_tree_info(node);
	temp = head;
	head = head->next;
	head->pre = NULL;
	p->next = NULL;
	free(temp);

}

void record_tree_info(ast_node_t *node)
{
	if (node == NULL)
		return;

	else
	{
		if (node->num_children == 0)
		{
			create_enode(node);
			return;
		}

		else
		{
			record_tree_info(node->children[0]);
			create_enode(node);
			if (node->num_children == 2)
				record_tree_info(node->children[1]);
		}

	}

}

void create_enode(ast_node_t *node)
{
	enode *s;
	s = (enode*)malloc(sizeof(enode));
	memset(s->type.variable, 0, 10) ;
	s->flag = -1;
	s->priority = -1;
	s->id = node->unique_count;
	switch (node->type)
	{
		case NUMBERS:
			s->type.data = node->types.number.value;
			s->flag = 1;
			s->priority = 0;
		break;

		case IDENTIFIERS:
			sprintf(s->type.variable, "%s", node->types.identifier);
			s->flag = 3;
			s->priority = 0;
		break;

		case BINARY_OPERATION:
		{
			switch(node->types.operation.op)
			{
				case ADD:
					s->type.operation = '+';
					s->flag = 2;
					s->priority = 2;
				break;

				case MINUS:
					s->type.operation = '-';
					s->flag =2;
					s->priority = 2;
				break;

				case MULTIPLY:
					s->type.operation = '*';
					s->flag = 2;
					s->priority = 1;
					break;

				case DIVIDE:
					s->type.operation = '/';
					s->flag = 2;
					s->priority = 1;
				break;

				default:
				break;
			}

		default:
			break;
		}


	}

	p->next = s;
	s->pre = p;
	p = s;

}

void adjoin_constant(int *build)
{
	enode *t, *replace;
	int a, b, result;
	int mark;
	for (t = head; t->next!= NULL; )
	{
		mark = 0;
		if ((t->flag == 1) && (t->next->next != NULL) && (t->next->next->flag ==1))
		{
			switch (t->next->priority)
			{
				case 1:
					a = t->type.data;
					b = t->next->next->type.data;
					if (t->next->type.operation == '*')
						result = a * b;
					else
						result = a / b;
					replace = replace_enode(result, t, 1);
					*build = 1;
					mark = 1;
					break;

				case 2:
					if (((t == head) || (t->pre->priority == 2)) && ((t->next->next->next == NULL) ||(t->next->next->next->priority ==2)))
					{
						a = t->type.data;
						b = t->next->next->type.data;
						if (t->pre->type.operation == '+')
						{
							if (t->next->type.operation == '+')
								result = a + b;
							else
								result = a - b;
						}
						if (t->pre->type.operation == '-')
						{
							if (t->next->type.operation == '+')
								result = a - b;
							else
								result = a + b;
						}

						replace = replace_enode(result, t, 1);
						*build = 1;
						mark = 1;
					}
					break;

			}
		}
		if (mark == 0)
			t = t->next;
		else
			if (mark == 1)
				t = replace;

	}
}

enode *replace_enode(int data, enode *t, int mark)
{
	enode *replace;
	replace = (enode*)malloc(sizeof(enode));
	memset(replace->type.variable, 0, 10);
	replace->type.data = data;
	replace->flag = 1;
	replace->priority = 0;

	if (t == head)
	{
		replace->pre = NULL;
		head = replace;
	}

	else
	{
		replace->pre = t->pre;
		t->pre->next = replace;
	}

	if (mark == 1)
	{
		replace->next = t->next->next->next;
		if (t->next->next->next == NULL)
			replace->next = NULL;
		else
			t->next->next->next->pre = replace;
		free(t->next->next);
		free(t->next);
	}
	else
	{
		replace->next = t->next;
		t->next->pre = replace;
	}

	free(t);
	return replace;

}

void combine_constant(int *build)
{
	enode *temp, *m, *s1, *s2, *replace;
	int a, b, result;
	int mark;

	for (temp = head; temp != NULL; )
	{
		mark = 0;
		if ((temp->flag == 1) && (temp->next != NULL) && (temp->next->priority == 2))
		{
			if ((temp == head) || (temp->pre->priority == 2))
			{
				for (m = temp->next; m != NULL; m = m->next)
				{
					if((m->flag == 1) && (m->pre->priority == 2) && ((m->next == NULL) || (m->next->priority == 2)))
					{
						s1 = temp;
						s2 = m;
						a = s1->type.data;
						b = s2->type.data;
						if ((temp = head) || (temp->pre->type.operation == '+'))
						{
							if (s2->pre->type.operation == '+')
								result = a + b;
							else
								result = a - b;
						}
						else
						{
							if (s2->pre->type.operation == '+')
								result = a - b;
							else
								result = a + b;
						}
						replace = replace_enode(result, s1, 2);
						if (s2->next == NULL)
						{
							s2->pre->pre->next = NULL;
							mark = 2;
						}
						else
						{
							s2->pre->pre->next = s2->next;
							s2->next->pre = s2->pre->pre;
						}

						free(s2->pre);
						free(s2);
						if (replace == head)
						{
							temp = replace;
							mark = 1;
						}
						else
							temp = replace->pre;
						*build = 1;

						break;
					}
				}
			}
		}
		if (mark == 0)
			temp = temp->next;
		else
			if (mark == 1)
				continue;
			else
				break;
	}

}

void construct_new_tree(enode *tail, ast_node_t *node, int line_num, int file_num)
{
	enode *temp, *tail1, *tail2;
	int prio = 0;

	if (tail == NULL)
		return;

	prio = check_exp_list(tail);
	if (prio == 1)
	{
		for (temp = tail; temp != NULL; temp = temp->pre)
			if ((temp->flag == 2) && (temp->priority == 2))
			{
				create_ast_node(temp, node, line_num, file_num);
				tail1 = temp->pre;
				tail2 = find_tail(temp->next);
				tail1->next = NULL;
				temp->next->pre = NULL;
				break;
			}

	}
	else
		if(prio == 2)
		{
			for (temp = tail; temp != NULL; temp = temp->pre)
				if ((temp->flag ==2) && (temp->priority == 1))
				{
					create_ast_node(temp, node, line_num, file_num);
					tail1 = temp->pre;
					tail2 = temp->next;
					tail2->pre = NULL;
					tail2->next = NULL;
					break;
				}
		}
		else
			if (prio == 3)
				create_ast_node(tail, node, line_num, file_num);

	if (prio == 1 || prio == 2)
	{
		node->children = (ast_node_t**)malloc(2*sizeof(ast_node_t*));
		node->children[0] = (ast_node_t*)malloc(sizeof(ast_node_t));
		node->children[1] = (ast_node_t*)malloc(sizeof(ast_node_t));
		construct_new_tree(tail1, node->children[0], line_num, file_num);
		construct_new_tree(tail2, node->children[1], line_num, file_num);
	}

	return;
}

int check_exp_list(enode *tail)
{
	enode *temp;
	for (temp = tail; temp != NULL; temp = temp->pre)
		if ((temp->flag == 2) && (temp->priority == 2))
			return 1;

	for (temp = tail; temp != NULL; temp = temp->pre)
		if ((temp->flag == 2) && (temp->priority ==1))
			return 2;

	return 3;
}

void delete_continuous_multiply(int *build)
{
	enode *temp, *t, *m, *replace;
	int a, b, result;
	int mark;
	for (temp = head; temp != NULL; )
	{
		mark = 0;
		if ((temp->flag == 1) && (temp->next != NULL) && (temp->next->priority == 1))
		{
			for (t = temp->next; t != NULL; t = t->next)
			{
				if ((t->flag == 1) && (t->pre->priority ==1))
				{
					for (m = temp->next; m != t; m = m->next)
					{
						if ((m->flag == 2) && (m->priority != 1))
						{
							mark = 1;
							break;
						}

					}
					if (mark == 0)
					{
						a = temp->type.data;
						b = t->type.data;
						if (t->pre->type.operation == '*')
							result = a * b;
						else
							result = a / b;
						replace = replace_enode(result, temp, 3);
						if (t->next == NULL)
							t->pre->pre->next = NULL;
						else
						{
							t->pre->pre->next = t->next;
							t->next->pre = t->pre->pre;
						}
						mark = 2;
						*build = 1;
						free(t->pre);
						free(t);
						break;
					}
					break;
				}
			}
		}
		if ((mark == 0) || (mark == 1))
			temp = temp->next;
		else
			if (mark == 2)
				temp = replace;

	}

}

void create_ast_node(enode *temp, ast_node_t *node, int line_num, int file_num)
{
	char num[12] = {0};
	int len;

	switch(temp->flag)
	{
		case 1:
			sprintf(num, "%d", temp->type.data);
			initial_node(node, NUMBERS, line_num, file_num);
			change_to_number_node(node, num);
		break;

		case 2:
			create_op_node(node, temp, line_num, file_num);
		break;

		case 3:
			len = strlen(temp->type.variable);
			initial_node(node, IDENTIFIERS, line_num, file_num);
			node->types.identifier = (char*)malloc(len+1);
			strcpy(node->types.identifier, temp->type.variable);
		break;

		default:
		break;
	}

}

void create_op_node(ast_node_t *node, enode *temp, int line_num, int file_num)
{
	initial_node(node, BINARY_OPERATION, line_num, file_num);
	node->num_children = 2;
	switch(temp->type.operation)
	{
		case '+':
			node->types.operation.op = ADD;
		break;

		case '-':
			node->types.operation.op = MINUS;
		break;

		case '*':
			node->types.operation.op = MULTIPLY;
		break;

		case '/':
			node->types.operation.op = DIVIDE;
		break;
	}

}

void free_exp_list()
{
	enode *next, *temp;
	for (temp = head; temp != NULL; temp = next)
	{
		next = temp->next;
		free(temp);
	}
}

void deal_with_bracket(ast_node_t *node)
{
	int i, begin, end;
	int list_bracket[Max_size] = {0};
	int num_bracket = 0;
	int *count_bracket = &num_bracket;
	recursive_tree(node, list_bracket, count_bracket);
	for (i = num_bracket-2; i >= 0; i = i - 2)
	{
		begin = list_bracket[i];
		end = list_bracket[i+1];
		delete_bracket(begin, end);
	}
}

void recursive_tree(ast_node_t *node, int list_bracket[], int *count_bracket)
{
	int i;
	if (node == NULL)
		return;

	if ((node->type == BINARY_OPERATION) && (node->types.operation.op == MULTIPLY))
	{
		for (i = 0; i < 2; i++)
		{
			if ((node->children[i]->type == BINARY_OPERATION) && (node->children[i]->types.operation.op == ADD || node->children[i]->types.operation.op == MINUS))
			{
				find_leaf_node(node->children[i], list_bracket, count_bracket, 0);
				find_leaf_node(node->children[i], list_bracket, count_bracket, 1);
			}
		}

	}
	if (node->num_children != 0)
		for (i = 0; i < node->num_children; i++)
			recursive_tree(node->children[i], list_bracket, count_bracket);

}

void find_leaf_node(ast_node_t *node, int list_bracket[], int *count_bracket, int ids)
{
	if (node == NULL)
		return;

	if (node->num_children == 0)
		list_bracket[(*count_bracket)++] = node->unique_count;

	if (node->num_children != 0)
		find_leaf_node(node->children[ids], list_bracket, count_bracket, ids);

}

void delete_bracket(int begin, int end)
{
	enode *s1, *s2, *temp, *p;
	int mark = 0;
	for (temp = head; temp != NULL; temp = temp->next)
	{
		if (temp->id == begin)
		{
			s1 = temp;
			for (p = temp; p != NULL; p = p->next)
				if (p->id == end)
				{
					s2 = p;
					mark = 1;
					break;
				}
		}
		if (mark == 1)
			break;
	}

	if (s1 == head)
		delete_bracket_head(s1, s2);
	else
		if (s2->next == NULL)
			delete_bracket_tail(s1, s2);
		else
			delete_bracket_body(s1, s2);

}

void delete_bracket_head(enode *begin, enode *end)
{
	enode *temp, *s = NULL;
	for (temp = end; temp != NULL; temp = temp->next)
	{
		if ((temp->flag == 2) && (temp->priority == 2))
		{
			s = temp->pre;
			break;
		}
	}
	change_exp_list(begin, end, s, 1);
}

void change_exp_list(enode *begin, enode *end, enode *s, int flag)
{
	enode *temp, *new_head, *tail, *p, *partial, *start = NULL;
	int mark;
	switch (flag)
	{
		case 1:
		{
			for (temp = begin; temp != end; temp = p->next)
			{
				mark = 0;
				for (p = temp; p != end->next; p = p->next)
				{
					if (p == end)
					{
						mark = 2;
						break;
					}
					if ((p->flag == 2) && (p->priority == 2))
					{
						partial = p->pre;
						mark = 1;
						break;
					}
				}
				if (mark == 1)
				{
					new_head = (enode*)malloc(sizeof(enode));
					tail = copy_enode_list(new_head, end->next, s);
					tail = tail->pre;
					free(tail->next);
					partial->next = new_head;
					new_head->pre = partial;
					tail->next = p;
					p->pre = tail;
				}
				if (mark == 2)
					break;
			}
			break;
		}

		case 2:
		{
			for (temp = begin; temp != end->next; temp = temp->next)
				if ((temp->flag == 2) && (temp->priority == 2))
				{
					start = temp;
					break;
				}
			for (temp = start; temp != end->next; temp = partial->next)
			{
				mark = 0;
				for (p = temp; p != end->next; p = p->next)
				{
					if (p == end)
					{
						mark = 2;
						break;
					}
					if ((p->flag == 2) && (p->priority == 2))
					{
						partial = p->next;
						mark = 1;
						break;
					}
				}
				if (mark == 1)
				{
					new_head = (enode*)malloc(sizeof(enode));
					tail = copy_enode_list(new_head, s, begin->pre);
					tail = tail->pre;
					free(tail->next);
					p->next = new_head;
					new_head->pre = p;
					tail->next = partial;
					partial->pre = tail;

				}
				if (mark == 2)
					break;
			}
			break;
		}

	}
}


enode *copy_enode_list(enode *new_head, enode *begin, enode *end)
{
	enode *temp, *new_enode, *next_enode;
	new_enode = new_head;
	for (temp = begin; temp != end->next; temp = temp->next)
	{
		copy_enode(temp, new_enode);
		next_enode = (enode*)malloc(sizeof(enode));
		new_enode->next = next_enode;
		next_enode->pre = new_enode;
		new_enode = next_enode;
	}

	return new_enode;
}

void copy_enode(enode *node, enode *new_node)
{
	new_node->type = node->type;
	new_node->flag = node->flag;
	new_node->priority = node->priority;
	new_node->id = -1;
}

void delete_bracket_tail(enode *begin, enode *end)
{
	enode *temp, *s = NULL;
	for (temp = begin; temp != NULL; temp = temp->pre)
		if ((temp->flag == 2) && (temp->priority == 2))
		{
			s = temp->next;
			break;
		}
	change_exp_list(begin, end, s, 2);
}


void delete_bracket_body(enode *begin, enode *end)
{
	enode *temp;
	if ((begin->pre->priority == 1) && (end->next->priority == 2))
		delete_bracket_tail(begin, end);
	if ((begin->pre->priority == 2) && (end->next->priority == 1))
		delete_bracket_head(begin, end);
	if ((begin->pre->priority == 1) && (end->next->priority == 1))
	{
		delete_bracket_tail(begin, end);
		for (temp = begin; temp != NULL; temp = temp->pre)
			if ((temp->flag == 2) && (temp->priority == 2))
			{
				begin = temp->next;
				break;
			}
		delete_bracket_head(begin, end);
	}

}

void check_tree_operation(ast_node_t *node, int *mark)
{
	int i;
	if (node == NULL)
		return;

	if ((node->type == BINARY_OPERATION) && ((node->types.operation.op == ADD ) || (node->types.operation.op == MINUS) || (node->types.operation.op == MULTIPLY)))
	{
		*mark = 1;
		return;
	}
	else
		if (node->num_children != 0)
			for (i = 0; i < node->num_children; i++)
				check_tree_operation(node->children[i], mark);

}

void reduce_parameter()
{
	ast_node_t *top;
	ast_node_t *para[1024];
	int num = 0;
	int *count = &num;
	int i;
	for (i = 0; i < num_modules; i++)
	{
		top = ast_modules[i];
		*count = 0;
		memset(para, 0, sizeof(para));
		find_parameter(top, para, count);
		if (top->types.module.is_instantiated == 0 && num != 0)
			remove_para_node(top, para, num);

	}

}

void find_parameter(ast_node_t *top, ast_node_t *para[], int *count)
{
	int i;
	ast_node_t *node;

	if (top == NULL)
		return;

	for (i = 0; i < top->num_children; i++)
	{
		node = top->children[i];
		if (node == NULL)
			return;
		else
			if (node->type == VAR_DECLARE && node->types.variable.is_parameter == 1)
				para[(*count)++] = node;

		find_parameter(node, para, count);
	}

}


void remove_para_node(ast_node_t *top, ast_node_t *para[], int num)
{
	int i, j;
	ast_node_t *para_node;
	ast_node_t *list[1024] = {0};
	ast_node_t *node;
	char *name;
	long value;

	count_assign = 0;
	find_assign_node(top, list);
	if (count_assign!= 0)
	{
		for (i = 0; i < num; i++)
		{
			if (para[i] != NULL && para[i]->children[0] != NULL)
			{
				para_node = para[i]->children[0];
				name = para_node->types.identifier;
			}
			if (para[i]->children[5] != NULL && para[i]->children[5]->type == NUMBERS)
				value = para[i]->children[5]->types.number.value;

			for (j = 0; j < count_assign; j++)
			{
				node = list[j];
				change_para_node(node, name, value);
			}

		}
	}
}

void change_para_node(ast_node_t *node, char *name, int value)
{
	int i;

	if (node == NULL)
		return;

	else
	{
		if (node->type == IDENTIFIERS && strcmp(name, node->types.identifier) == 0)
			change_ast_node(node, value);
	}
	if (node->num_children != 0)
		for (i = 0; i < node->num_children; i++)
			change_para_node(node->children[i], name, value);

}

void change_ast_node(ast_node_t *node, int value)
{
	char num[12] = {0};
	if (node->types.identifier != NULL)
			free(node->types.identifier);
	node->type = NUMBERS;
	sprintf(num, "%d", value);
	change_to_number_node(node, num);

}

















