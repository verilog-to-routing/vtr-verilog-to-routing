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
#include <math.h>
#include "odin_types.h"
#include "ast_util.h"
#include "ast_elaborate.h"
#include "ast_loop_unroll.h"
#include "parse_making_ast.h"
#include "verilog_bison.h"
#include "netlist_create_from_ast.h"
#include "ctype.h"
#include "vtr_util.h"
#include "vtr_memory.h"
#include <string>
#include <iostream>
#include <vector>
#include <stack>

#define read_node  1
#define write_node 2

#define e_data  1
#define e_operation 2
#define e_variable 3

#define N 64
#define Max_size 64

long count_id = 0;
long count_assign = 0;
long count;
long count_write;
enode *head, *p;

void remove_generate(ast_node_t *node)
{
	for(int i=0; i<node->num_children; i++){
		if(!node->children[i])
			continue;
		ast_node_t* candidate = node->children[i];
		ast_node_t* body_parent = nullptr;

		if(candidate->type == GENERATE){
			for(int j = 0; j<candidate->num_children; j++){
				ast_node_t* new_child = ast_node_deep_copy(candidate->children[j]);
				body_parent = body_parent ? newList_entry(body_parent, new_child) : newList(BLOCK, new_child);
			}
			node->children[i] = body_parent;
			free_whole_tree(candidate);
		} else {
			remove_generate(candidate);
		}
	}
}

int simplify_ast_module(ast_node_t **ast_module)
{
	/* reduce parameters with their values if they have been set */
	reduce_parameter(*ast_module);
	/* simplify assignment expressions */
	reduce_assignment_expression(*ast_module);
	/* for loop support */
	unroll_loops(ast_module);
	/* remove unused node preventing module instantiation */
	remove_generate(*ast_module);
	/* find multiply or divide operation that can be replaced with shift operation */
	shift_operation(*ast_module);

	return 1;
}

/*---------------------------------------------------------------------------
 * (function: reduce_assignment_expression)
 * reduce the number nodes which can be calculated to optimize the AST
 *-------------------------------------------------------------------------*/
void reduce_assignment_expression(ast_node_t *ast_module)
{
	head = NULL;
	p = NULL;
	ast_node_t *T = NULL;

	if (count_id < ast_module->unique_count)
		count_id = ast_module->unique_count;

	count_assign = 0;
	std::vector<ast_node_t *> list_assign;
	find_assign_node(ast_module, list_assign, ast_module->children[0]->types.identifier);
	for (long j = 0; j < count_assign; j++)
	{
		if (check_tree_operation(list_assign[j]->children[1]) && (list_assign[j]->children[1]->num_children > 0))
		{
			store_exp_list(list_assign[j]->children[1]);
			if (deal_with_bracket(list_assign[j]->children[1])) // there are multiple brackets multiplying -- ()*(), stop expanding brackets which may not simplify AST but make it mroe complex
				return;

			if (simplify_expression())
			{
				enode *tail = find_tail(head);
				free_whole_tree(list_assign[j]->children[1]);
				T = (ast_node_t*)vtr::malloc(sizeof(ast_node_t));
				construct_new_tree(tail, T, list_assign[j]->line_number, list_assign[j]->file_number);
				list_assign[j]->children[1] = T;
			}
			free_exp_list();
		}
	}
}

/*---------------------------------------------------------------------------
 * (function: search_assign_node)
 * find the top of the assignment expression
 *-------------------------------------------------------------------------*/
void find_assign_node(ast_node_t *node, std::vector<ast_node_t *> list, char *module_name)
{
	if (node && node->num_children > 0 && (node->type == BLOCKING_STATEMENT || node->type == NON_BLOCKING_STATEMENT))
	{
		list.push_back(node);
	}

	for (long i = 0; node && i < node->num_children; i++)
		find_assign_node(node->children[i], list, module_name);
}

/*---------------------------------------------------------------------------
 * (function: simplify_expression)
 * simplify the expression stored in the linked_list
 *-------------------------------------------------------------------------*/
bool simplify_expression()
{
	bool build = adjoin_constant();
	reduce_enode_list();

	if(delete_continuous_multiply())
		build = TRUE;

	if(combine_constant())
		build = TRUE;

	reduce_enode_list();
	return build;
}

/*---------------------------------------------------------------------------
 * (function: find_tail)
 *-------------------------------------------------------------------------*/
enode *find_tail(enode *node)
{
	enode *temp;
	enode *tail = NULL;
	for (temp = node; temp != NULL; temp = temp->next)
		if (temp->next == NULL)
			tail = temp;

	return tail;
}

/*---------------------------------------------------------------------------
 * (function: reduce_enode_list)
 *-------------------------------------------------------------------------*/
void reduce_enode_list()
{
	enode *temp;
	int a;

	while(head != NULL && (head->type.data == 0) && (head->next->priority == 2) && (head->next->type.operation == '+')){
		temp=head;
		head = head->next->next;
		head->pre = NULL;

		vtr::free(temp->next);
		vtr::free(temp);
	}

	if(head == NULL){
		return;
	}

	temp = head->next;
	while (temp != NULL)
	{
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
				vtr::free(temp->pre);

				enode *toBeDeleted = temp;
				temp = temp->next;
				vtr::free(toBeDeleted);
			} else if (temp->type.data < 0)
			{
				if (temp->pre->type.operation == '+')
					temp->pre->type.operation = '-';
				else
					temp->pre->type.operation = '+';
				a = temp->type.data;
				temp->type.data = -a;

				temp = temp->next;
			} else {
				temp = temp->next;
			}
		} else {
			temp = temp->next;
		}
	}
}

/*---------------------------------------------------------------------------
 * (function: store_exp_list)
 *-------------------------------------------------------------------------*/
void store_exp_list(ast_node_t *node)
{
	enode *temp;
	head = (enode*)vtr::malloc(sizeof(enode));
	p = head;
	record_tree_info(node);
	temp = head;
	head = head->next;
	head->pre = NULL;
	p->next = NULL;
	vtr::free(temp);

}

/*---------------------------------------------------------------------------
 * (function: record_tree_info)
 *-------------------------------------------------------------------------*/
void record_tree_info(ast_node_t *node)
{
	if (node){
		if (node->num_children > 0)
		{
			record_tree_info(node->children[0]);
			create_enode(node);
			if (node->num_children > 1)
				record_tree_info(node->children[1]);
		}else{
			create_enode(node);
		}
	}
}

/*---------------------------------------------------------------------------
 * (function: create_enode)
 * store elements of an expression in nodes consisting the linked_list
 *-------------------------------------------------------------------------*/
void create_enode(ast_node_t *node)
{
	enode *s;
	s = (enode*)vtr::malloc(sizeof(enode));
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
			s->type.variable = node->types.identifier;
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

/*---------------------------------------------------------------------------
 * (function: adjoin_constant)
 * compute the constant numbers in the linked_list
 *-------------------------------------------------------------------------*/
bool adjoin_constant()
{
	bool build = FALSE;
	enode *t, *replace;
	int a, b, result = 0;
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
					build = TRUE;
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
						build = TRUE;
						mark = 1;
					}
					break;
                default:
                    //pass
                    break;
			}
		}
		if (mark == 0)
			t = t->next;
		else
			if (mark == 1)
				t = replace;

	}
	return build;
}

/*---------------------------------------------------------------------------
 * (function: replace_enode)
 *-------------------------------------------------------------------------*/
enode *replace_enode(int data, enode *t, int mark)
{
	enode *replace;
	replace = (enode*)vtr::malloc(sizeof(enode));
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
		vtr::free(t->next->next);
		vtr::free(t->next);
	}
	else
	{
		replace->next = t->next;
		t->next->pre = replace;
	}

	vtr::free(t);
	return replace;

}

/*---------------------------------------------------------------------------
 * (function: combine_constant)
 *-------------------------------------------------------------------------*/
bool combine_constant()
{
	enode *temp, *m, *s1, *s2, *replace;
	int a, b, result;
	int mark;
	bool build = FALSE;

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
						if ((s1 == head) || (s1->pre->type.operation == '+'))
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

						vtr::free(s2->pre);
						vtr::free(s2);
						if (replace == head)
						{
							temp = replace;
							mark = 1;
						}
						else
							temp = replace->pre;
						build = TRUE;

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
	return build;

}

/*---------------------------------------------------------------------------
 * (function: construct_new_tree)
 *-------------------------------------------------------------------------*/
void construct_new_tree(enode *tail, ast_node_t *node, int line_num, int file_num)
{
	enode *temp, *tail1 = NULL, *tail2 = NULL;
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
		node->children = (ast_node_t**)vtr::malloc(2*sizeof(ast_node_t*));
		node->children[0] = (ast_node_t*)vtr::malloc(sizeof(ast_node_t));
		node->children[1] = (ast_node_t*)vtr::malloc(sizeof(ast_node_t));
		construct_new_tree(tail1, node->children[0], line_num, file_num);
		construct_new_tree(tail2, node->children[1], line_num, file_num);
	}

	return;
}

/*---------------------------------------------------------------------------
 * (function: check_exp_list)
 *-------------------------------------------------------------------------*/
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

/*---------------------------------------------------------------------------
 * (function: delete_continuous_multiply)
 *-------------------------------------------------------------------------*/
bool delete_continuous_multiply()
{
	enode *temp, *t, *m, *replace;
	bool build = FALSE;
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
						build = TRUE;
						vtr::free(t->pre);
						vtr::free(t);
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
	return build;
}

/*---------------------------------------------------------------------------
 * (function: create_ast_node)
 *-------------------------------------------------------------------------*/
void create_ast_node(enode *temp, ast_node_t *node, int line_num, int file_num)
{
	switch(temp->flag)
	{
		case 1:
			initial_node(node, NUMBERS, line_num, file_num, ++count_id);
			change_to_number_node(node, temp->type.data);
		break;

		case 2:
			create_op_node(node, temp, line_num, file_num);
		break;

		case 3:
			initial_node(node, IDENTIFIERS, line_num, file_num, ++count_id);
			node->types.identifier = vtr::strdup(temp->type.variable.c_str());
		break;

		default:
		break;
	}

}

/*---------------------------------------------------------------------------
 * (function: create_op_node)
 *-------------------------------------------------------------------------*/
void create_op_node(ast_node_t *node, enode *temp, int line_num, int file_num)
{
	initial_node(node, BINARY_OPERATION, line_num, file_num, ++count_id);
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
        default:
            oassert(FALSE);
        break;
	}

}

/*---------------------------------------------------------------------------
 * (function: free_exp_list)
 *-------------------------------------------------------------------------*/
void free_exp_list()
{
	enode *next, *temp;
	for (temp = head; temp != NULL; temp = next)
	{
		next = temp->next;
		vtr::free(temp);
	}
}

/*---------------------------------------------------------------------------
 * (function: deal_with_bracket)
 *-------------------------------------------------------------------------*/
bool deal_with_bracket(ast_node_t *node)
{
	std::vector<int> list_bracket;

	recursive_tree(node, list_bracket);
	if (!check_mult_bracket(list_bracket)) //if there are brackets multiplying continuously ()*(), stop expanding brackets which may not simplify AST but make it mroe complex
	{
		for (int i = list_bracket.size()-2; i >= 0; i = i - 2)
			delete_bracket(list_bracket[i], list_bracket[i+1]);
		return TRUE;
	}

	return FALSE;
}

/*---------------------------------------------------------------------------
 * (function: check_mult_bracket)
 * check if the brackets are continuously multiplying
 *-------------------------------------------------------------------------*/
bool check_mult_bracket(std::vector<int> list)
{
	for (long i = 1; i < list.size() - 2; i = i + 2)
	{
		enode *node = head;
		while (node != NULL && node->next != NULL && node->next->next != NULL)
		{
			if (node->id == list[i] && node->next->next->id == list[i+1] &&
				node->next->flag == 2 &&
				(node->next->type.operation == '*' || node->next->type.operation == '/'))

				return TRUE;

			node = node->next;
		}
	}
	return FALSE;

}

/*---------------------------------------------------------------------------
 * (function: recursive_tree)
 * search the AST recursively to find brackets
 *-------------------------------------------------------------------------*/
void recursive_tree(ast_node_t *node, std::vector<int> list_bracket)
{
	if (node && (node->type == BINARY_OPERATION) && (node->types.operation.op == MULTIPLY))
	{
		for (long i = 0; i < node->num_children; i++)
		{
			if ((node->children[i]->type == BINARY_OPERATION) && (node->children[i]->types.operation.op == ADD || node->children[i]->types.operation.op == MINUS))
			{
				find_leaf_node(node->children[i], list_bracket, 0);
				find_leaf_node(node->children[i], list_bracket, 1);
			}
		}

	}

	for (long i = 0; node && i < node->num_children; i++)
		recursive_tree(node->children[i], list_bracket);

}

/*---------------------------------------------------------------------------
 * (function: find_lead_node)
 *-------------------------------------------------------------------------*/
void find_leaf_node(ast_node_t *node, std::vector<int> list_bracket, int ids2)
{
	if (node){
		if (node->num_children > 0){
			find_leaf_node(node->children[ids2], list_bracket, ids2);
		}else{
			list_bracket.push_back(node->unique_count);
		}
	}
}

/*---------------------------------------------------------------------------
 * (function: delete_bracket)
 *-------------------------------------------------------------------------*/
void delete_bracket(int begin, int end)
{
	enode *s1 = NULL, *s2 = NULL, *temp, *p2;
	int mark = 0;
	for (temp = head; temp != NULL; temp = temp->next)
	{
		if (temp->id == begin)
		{
			s1 = temp;
			for (p2 = temp; p2 != NULL; p2 = p2->next)
				if (p2->id == end)
				{
					s2 = p2;
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
	if (s1 != head)
		check_operation(s1, s2);

}

/*---------------------------------------------------------------------------
 * (function: delete_bracket_head)
 *-------------------------------------------------------------------------*/
void delete_bracket_head(enode *begin, enode *end)
{
	enode *temp;
    enode *s = NULL;
	for (temp = end; temp != NULL; temp = temp->next)
	{
		if ((temp->flag == 2) && (temp->priority == 2))
		{
			s = temp->pre;
			break;
		}
		if (temp->next == NULL)
			s = temp;
	}
	change_exp_list(begin, end, s, 1);
}

/*---------------------------------------------------------------------------
 * (function: change_exp_list)
 *-------------------------------------------------------------------------*/
void change_exp_list(enode *begin, enode *end, enode *s, int flag)
{
	enode *temp, *new_head, *tail, *p2, *partial = NULL, *start = NULL;
	int mark;
	switch (flag)
	{
		case 1:
		{
			for (temp = begin; temp != end; temp = p2->next)
			{
				mark = 0;
				for (p2 = temp; p2 != end->next; p2 = p2->next)
				{
					if (p2 == end)
					{
						mark = 2;
						break;
					}
					if ((p2->flag == 2) && (p2->priority == 2))
					{
						partial = p2->pre;
						mark = 1;
						break;
					}
				}
				if (mark == 1)
				{
					new_head = (enode*)vtr::malloc(sizeof(enode));
					tail = copy_enode_list(new_head, end->next, s);
					tail = tail->pre;
					vtr::free(tail->next);
					partial->next = new_head;
					new_head->pre = partial;
					tail->next = p2;
					p2->pre = tail;
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
				for (p2 = temp; p2 != end->next; p2 = p2->next)
				{
					if (p2 == end)
					{
						mark = 2;
						break;
					}
					if ((p2->flag == 2) && (p2->priority == 2))
					{
						partial = p2->next;
						mark = 1;
						break;
					}
				}
				if (mark == 1)
				{
					new_head = (enode*)vtr::malloc(sizeof(enode));
					tail = copy_enode_list(new_head, s, begin->pre);
					tail = tail->pre;
					vtr::free(tail->next);
					p2->next = new_head;
					new_head->pre = p2;
					tail->next = partial;
					partial->pre = tail;

				}
				if (mark == 2)
					break;
			}
			break;
		}
        default:
            break;

	}
}

/*---------------------------------------------------------------------------
 * (function: copy_enode_list)
 *-------------------------------------------------------------------------*/
enode *copy_enode_list(enode *new_head, enode *begin, enode *end)
{
	enode *temp, *new_enode, *next_enode;
	new_enode = new_head;
	for (temp = begin; temp != end->next; temp = temp->next)
	{
		copy_enode(temp, new_enode);
		next_enode = (enode*)vtr::malloc(sizeof(enode));
		new_enode->next = next_enode;
		next_enode->pre = new_enode;
		new_enode = next_enode;
	}

	return new_enode;
}

/*---------------------------------------------------------------------------
 * (function: copy_enode)
 *-------------------------------------------------------------------------*/
void copy_enode(enode *node, enode *new_node)
{
	new_node->type = node->type;
	new_node->flag = node->flag;
	new_node->priority = node->priority;
	new_node->id = -1;
}

/*---------------------------------------------------------------------------
 * (function: deleted_bracket_tail)
 *-------------------------------------------------------------------------*/
void delete_bracket_tail(enode *begin, enode *end)
{
	enode *temp, *s = NULL;
	for (temp = begin; temp != NULL; temp = temp->pre)
		if ((temp->flag == 2) && (temp->priority == 2)) // '+' or '-'
		{
			s = temp->next;
			break;
		}
	change_exp_list(begin, end, s, 2);
}

/*---------------------------------------------------------------------------
 * (function: delete_bracket_body)
 *-------------------------------------------------------------------------*/
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

/*---------------------------------------------------------------------------
 * (function: check_tree_operation)
 *-------------------------------------------------------------------------*/
bool check_tree_operation(ast_node_t *node)
{
	if (node && (node->type == BINARY_OPERATION) && ((node->types.operation.op == ADD ) || (node->types.operation.op == MINUS) || (node->types.operation.op == MULTIPLY)))
		return TRUE;

	for (long i = 0; node &&  i < node->num_children; i++)
		if(check_tree_operation(node->children[i]))
			return TRUE;

	return FALSE;
}

/*---------------------------------------------------------------------------
 * (function: check_operation)
 *-------------------------------------------------------------------------*/
void check_operation(enode *begin, enode *end)
{
	enode *temp, *op;
	for (temp = begin; temp != head; temp = temp->pre)
	{
		if (temp->flag == 2 && temp->priority == 2 && temp->type.operation == '-')
		{
			for (op = begin; op != end; op = op->next)
			{
				if (op->flag == 2 && op->priority == 2)
				{
					if (op->type.operation == '+')
						op->type.operation = '-';
					else
						op->type.operation = '+';
				}
			}
		}
	}
}

/*---------------------------------------------------------------------------
 * (function: reduce_parameter)
 * replace parameters with their values in the AST
 *-------------------------------------------------------------------------*/
void reduce_parameter(ast_node_t *ast_module)
{
	std::vector<ast_node_t *>para;
	find_parameter(ast_module, para);
	if (ast_module->types.module.is_instantiated == 0 && !para.empty())
		remove_para_node(ast_module, para);


}

/*---------------------------------------------------------------------------
 * (function: find_parameter)
 *-------------------------------------------------------------------------*/
void find_parameter(ast_node_t *node, std::vector<ast_node_t *>para)
{
	for (long i = 0; node && i < node->num_children; i++)
	{
		if (node->children[i] && node->children[i]->type == VAR_DECLARE && node->children[i]->types.variable.is_parameter == 1)
				para.push_back(node->children[i]);

		find_parameter(node->children[i], para);
	}
}

/*---------------------------------------------------------------------------
 * (function: remove_para_node)
 *-------------------------------------------------------------------------*/
void remove_para_node(ast_node_t *top, std::vector<ast_node_t *> para)
{
	std::vector<ast_node_t *> list;

	count_assign = 0;
	find_assign_node(top, list, top->children[0]->types.identifier);
	if (count_assign!= 0){
		for (long i = 0; i < para.size(); i++)
		{
			std::string name;
			long value = 0;
			if (para[i] && para[i]->children[0])
				name = para[i]->children[0]->types.identifier;

			if (node_is_constant(para[i]->children[5]))
				value = para[i]->children[5]->types.number.value;

			for (long j = 0; j < count_assign; j++){
				change_para_node(list[j], name, value);
			}

		}
	}
}

/*---------------------------------------------------------------------------
 * (function: change_para_node)
 *-------------------------------------------------------------------------*/
void change_para_node(ast_node_t *node, std::string name, long value)
{
	if (node && node->type == IDENTIFIERS && name == (std::string) node->types.identifier)
		change_to_number_node(node, value);

	for (long i = 0; node && i < node->num_children; i++)
		change_para_node(node->children[i], name, value);
}

/*---------------------------------------------------------------------------
 * (function: copy_tree)
 * find multiply or divide operation that can be replaced with shift operation
 *-------------------------------------------------------------------------*/
void shift_operation(ast_node_t *ast_module)
{
	if(ast_module){
		search_certain_operation(ast_module);
	}

}

/*---------------------------------------------------------------------------
 * (function: change_ast_node)
 *  search all AST to find multiply or divide operations
 *-------------------------------------------------------------------------*/
void search_certain_operation(ast_node_t *node)
{
	for (long i = 0; node && i < node->num_children; i++){
		check_binary_operation(node->children[i]);
		search_certain_operation(node->children[i]);
	}
}

/*---------------------------------------------------------------------------
 * (function: change_ast_node)
 *  check the children nodes of an operation node
 *-------------------------------------------------------------------------*/
void check_binary_operation(ast_node_t *node)
{
	if(node && node->type == BINARY_OPERATION){
		switch(node->types.operation.op){
			case MULTIPLY:
				if (node->children[0]->type == IDENTIFIERS && node->children[1]->type == NUMBERS)
					check_node_number(node, node->children[1], 1); // 1 means multiply and don't need to move children nodes
				if (node->children[0]->type == NUMBERS && node->children[1]->type == IDENTIFIERS)
					check_node_number(node, node->children[0], 2); // 2 means multiply and needs to move children nodes
				break;
			case DIVIDE:
				if (node->children[0]->type == IDENTIFIERS && node->children[1]->type == NUMBERS)
					check_node_number(node, node->children[1], 3); // 3 means divide
				break;
			default:
				break;
		}
	}
}

/*---------------------------------------------------------------------------
 * (function: change_ast_node)
 * check if the number is the power of 2
 *-------------------------------------------------------------------------*/
void check_node_number(ast_node_t *parent, ast_node_t *child, int flag)
{
	long power = 0;
	long number = child->types.number.value;
	if (number <= 1)
		return;
	while (((number % 2) == 0) && number > 1) // While number is even and > 1
	{
		number >>= 1;
		power++;
	}
	if (number == 1) // the previous number is a power of 2
	{
		change_to_number_node(child, power);
		if (flag == 1) // multiply
			parent->types.operation.op = SL;
		else if (flag == 2) // multiply and needs to move children nodes
		{
			parent->types.operation.op = SL;
			parent->children[0] = parent->children[1];
			parent->children[1] = child;
		}
		else if (flag == 3) // divide
			parent->types.operation.op = SR;
	}
}


/*---------------------------------------------------------------------------
 * (function: check_intermediate_variable)
 * check if there are intermediate variables
 *-------------------------------------------------------------------------*/
short has_intermediate_variable(ast_node_t *node){
	if (node && (node->is_read_write == 1 || node->is_read_write == 2))
		return TRUE;

	for (long i = 0; node && i < node->num_children; i++){
		if(has_intermediate_variable(node->children[i]))
			return TRUE;
	}

	return FALSE;
}


/*---------------------------------------------------------------------------
 * (function: find_proc_assign_nodes)
 * find a list of the assignment expressions recursively
 *-------------------------------------------------------------------------*/
void find_proc_assign_nodes(ast_node_t *node, std::vector<ast_node_t *> &list)
{
    if (node && node->num_children>0
        && (node->type == BLOCKING_STATEMENT
            || node->type == NON_BLOCKING_STATEMENT
            || node->type == PROC_CONT_ASSIGN))
        list.push_back(node);

    for (long i = 0; node && i < node->num_children; i++)
        find_proc_assign_nodes(node->children[i], list);
}

/*---------------------------------------------------------------------------
 * (function: find_children_by_type)
 * Finds all children with the same type recursively
 *-------------------------------------------------------------------------*/
void find_children_by_type(ast_node_t *node, std::vector<ast_node_t *> &list, ids type) {
    if (node && node->num_children>0 && (node->type == type))
        list.push_back(node);

    for (long i = 0; node && i < node->num_children; i++)
        find_children_by_type(node->children[i], list, type);
}

/*---------------------------------------------------------------------------
 * (function: find_children_by_type)
 * Simplifies the procedural continuous assignments
 *-------------------------------------------------------------------------*/
void simplify_pc_assignments2() {
    struct assignment_info {
        std::string name;
        ast_node_t *id;
        ast_node_t *pc;
        ast_node_t *p;
        ast_node_t *condition;
        ast_node_t *p_always;
        ast_node_t *pc_always;
    };

    for (long m = 0; m < num_modules; m++)
    {

        auto module = ast_modules[m];
        
        std::map<std::string, assignment_info> assignments;

        for (long i = 0; i < module->children[2]->num_children; i++) {
            if (module->children[2]->children[i]->type != ALWAYS)
                continue;

            auto always = module->children[2]->children[i];
            std::vector<ast_node_t *> list_assign;
            find_proc_assign_nodes(always, list_assign);

            for (auto assignment : list_assign) {
                char * name;

                switch (assignment->children[0]->type) {
                    case IDENTIFIERS:
                        name = assignment->children[0]->types.identifier;
                        break;
                    default:
                        
                        continue;
                }

                assignment_info info = assignment_info();
                if (assignments[name].id != nullptr)
                    info = assignments[name];

                info.id = assignment->children[0];
                if (assignment->type == PROC_CONT_ASSIGN)
                {
                    info.pc = assignment;
                    info.pc_always = always;
                }
                else
                {
                    info.p = assignment;
                    info.p_always = always;
                }

                std::vector<ast_node_t*> if_statements;
                find_children_by_type(always, if_statements, IF);
                //oassert(if_statements.size() <= 1);
                if (!if_statements.empty())
                    info.condition = if_statements[0];

                assignments[name] = info;
            }
        }
        for (auto &info: assignments)
        {
            
            auto second = info.second;
            if (second.pc == nullptr || second.p == nullptr || second.condition == nullptr)
                continue;

            auto ifQ_node = newIfQuestion(second.condition->children[0], second.pc->children[1], second.p->children[1], 0);
            if (second.condition->children[0]->types.operation.op == LOGICAL_NOT)
            {
                second.condition->children[0]->types.operation.op = BITWISE_NOT;
            }
            ast_node_t *node = nullptr;
            if (second.p->type == BLOCKING_STATEMENT)
                node = newBlocking(second.id, ifQ_node, 0);
            else if (second.p->type == NON_BLOCKING_STATEMENT)
                node = newNonBlocking(second.id, ifQ_node, 0);
            else
                throw;

            //TODO: cleanup old assignment nodes **PROPERLY**
            for (long i = 0; i < module->children[2]->num_children; i++)
            {
                if (module->children[2]->children[i] == second.pc_always)
                {
                    module->children[2]->children[i] = module->children[2]->children[i+1];
                    break;
                }
            }
            module->children[2]->num_children--;
            second.p_always->children[1]->children[0] = node; 
            second.p_always->children[1]->num_children = 1;
        }
        //graphVizOutputAst("./temp",module);
    }

}

void simplify_pc_assignments() {
    simplify_pc_assignments2();
    return;
    for (long m = 0; m < num_modules; m++)
    {
        auto module = ast_modules[m];
        std::vector<ast_node_t*> always_blocks;
        find_children_by_type(module, always_blocks, ALWAYS);

        if (always_blocks.empty())
            continue;

        for (auto always : always_blocks){
            std::vector<ast_node_t *> assignments;
            find_children_by_type(always, assignments, PROC_CONT_ASSIGN);
			if (assignments.empty())
				continue;

			/* Convert Procedural Continuous Assignment to Continuous assignment */
			for (auto assignment : assignments) {
				if (assignment->type == PROC_CONT_ASSIGN)
					assignment->type = BLOCKING_STATEMENT;
			}
        }
    }

}
