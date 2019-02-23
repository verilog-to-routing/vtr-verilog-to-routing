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

int simplify_ast()
{
	/* for loop support */
	unroll_loops();
	/* reduce parameters with their values if they have been set */
	reduce_parameter();
	/* simplify assignment expressions */
	reduce_assignment_expression();
	/* find multiply or divide operation that can be replaced with shift operation */
	shift_operation();

	//ast_node_t *top = find_top_module();

	return 1;
}

/*---------------------------------------------------------------------------
 * (function: optimize_for_tree)
 * simplify the FOR loop syntax tree
 * TODO fix this.. leaking memmory
 *-------------------------------------------------------------------------*/
void optimize_for_tree()
{
	// find out the most unique_count prepared for new AST nodes
	for (long i = 0; i < num_modules; i++)
		if (count_id < ast_modules[i]->unique_count)
			count_id = ast_modules[i]->unique_count;

	/* we will find the top module */
	for (long i = 0; i < num_modules; i++)
	{
		std::vector<ast_node_t *> list_for_node;
		std::vector<ast_node_t *> list_parent;
		/* search the tree looking for FOR node */
		search_for_node(ast_modules[i], list_for_node, list_parent);

		/* simplify every FOR node */
		for (long j = 0; j < list_for_node.size() ; j++)
		{

			long v_value = list_for_node[j]->children[0]->children[1]->types.number.value;
			long terminal = list_for_node[j]->children[1]->children[1]->types.number.value;
			std::vector<std::string> expression;
			std::vector<std::string> node_write;

			ast_node_t *temp_parent_node = (ast_node_t*)vtr::malloc(sizeof(ast_node_t)); //used to connect copied branches from the for loop
			count_write = 0;
			count = 0;
			bool found = FALSE;
			record_expression(list_for_node[j]->children[2], expression, &found);
			mark_node_write(list_for_node[j]->children[3], node_write);
			mark_node_read(list_for_node[j]->children[3], node_write);

			while(v_value < terminal)
			{
				add_child_to_node(temp_parent_node,
									get_copy_tree((list_for_node[j])->children[3],
										v_value,
										(std::string )list_for_node[j]->children[0]->children[0]->types.identifier
										)
									);

				std::vector<std::string> infix_exp;
				std::vector<std::string> postfix_exp;

				for (long k = 0; expression[k].size() ; k++)
				{
					if (expression[k] == (std::string)list_for_node[j]->children[0]->children[0]->types.identifier){
						std::string valued_at;
						valued_at = v_value;
						infix_exp.push_back(valued_at);
					}else{
						infix_exp.push_back(expression[k]);
					}
				}

				translate_expression(infix_exp, postfix_exp);
				v_value = calculation(postfix_exp);
			}
			if (has_intermediate_variable(temp_parent_node)) //there are intermediate variables involved in operations so remove the last branch from the node
				remove_intermediate_variable(temp_parent_node, node_write, v_value, (std::string) list_for_node[j]->children[0]->children[0]->types.identifier);

			//the index of the FOR node belonging to its parent node may change after every for loop support iteration, so it needs to be checked again
			int idx;
			for (idx = list_parent[j]->num_children ; idx >= 0 ; idx--)
				if (list_parent[j]->children[i] == list_for_node[j])
					break;

			keep_all_branch(temp_parent_node, list_parent[j], idx);
			reallocate_node(list_parent[j], idx);
			vtr::free(temp_parent_node);
		}
	}
}

/*---------------------------------------------------------------------------
 * (function: search_for_node)
 * search the tree looking for FOR node
 *-------------------------------------------------------------------------*/
void search_for_node(ast_node_t *root, std::vector<ast_node_t *> list_for_node, std::vector<ast_node_t *> list_parent)
{
	for(long i = 0; root && i < root->num_children; i++)
		search_for_node(root->children[i], list_for_node, list_parent);

	for (long j = 0; root && j < root->num_children; j++)
	{
		if (root->children[j] && root->children[j]->type == FOR)
		{
			list_for_node.push_back(root->children[j]);
			list_parent.push_back(root);
		}
	}

}

/*---------------------------------------------------------------------------
 * (function: get_copy_tree)
 *-------------------------------------------------------------------------*/
ast_node_t *get_copy_tree(ast_node_t *node, long virtual_value, std::string virtual_name)
{
	if (!node)
		return NULL;

	ast_node_t *new_node = (ast_node_t *)vtr::malloc(sizeof(ast_node_t));
	memcpy(new_node, node, sizeof(ast_node_t));
		//Copy contents
	new_node->types.identifier = vtr::strdup(node->types.identifier);
	new_node->types.number.number = vtr::strdup(node->types.number.number);
	new_node->types.number.binary_string = vtr::strdup(node->types.number.binary_string);

	new_node->unique_count = ++count_id;

	if (node->type == IDENTIFIERS && (std::string) node->types.identifier == virtual_name)
		change_to_number_node(new_node, virtual_value);

	new_node->children = (ast_node_t**)vtr::malloc(node->num_children*sizeof(ast_node_t*));
	for(long i=0; i<node->num_children; i++){
		new_node->children[i] = get_copy_tree(node->children[i],virtual_value,virtual_name);
	}
	return new_node;
}

/*---------------------------------------------------------------------------
 * (function: expression)
 * in order traversal
 *-------------------------------------------------------------------------*/
void record_expression(ast_node_t *node, std::vector<std::string> expressions, bool *found_statement)
{
	if (node){

		if(node->num_children > 0)
			record_expression(node->children[0], expressions, found_statement);


		if(!(*found_statement) && (/*node->type == BLOCKING_STATEMENT || */node->type == BLOCKING_STATEMENT)){
			*found_statement = TRUE;

		}else if(*found_statement){
			switch(node->type)
			{
				case IDENTIFIERS:
					expressions.push_back(node->types.identifier);
					break;

				case NUMBERS:
					expressions.push_back(node->types.number.number);
					break;

				case BINARY_OPERATION:
					switch(node->types.operation.op)
					{
						case ADD:
							expressions.push_back("+");
							break;
						case MINUS:
							expressions.push_back("-");
							break;
						case MULTIPLY:
							expressions.push_back("*");
							break;
						case DIVIDE:
							expressions.push_back("/");
							break;

						default:
							break;
					}
					break;

				default:
					break;
			}
		}

		if(node->num_children > 1)
			record_expression(node->children[1], expressions, found_statement);
	}
}

/*---------------------------------------------------------------------------
 * (function: calculate)
 *-------------------------------------------------------------------------*/
long  calculation(std::vector<std::string> post_exp)
{
	std::stack<long> stacked;
	for (long i = 0; i< post_exp.size(); i++)
	{
		long second_exp(stacked.top());
		stacked.pop();

		long first_exp(stacked.top());
		stacked.pop();

	    if (post_exp[i] == "+"){
			 stacked.push(first_exp + second_exp);

		}else if (post_exp[i] == "-"){
			stacked.push(first_exp - second_exp);

		}else if (post_exp[i] == "*"){
			stacked.push(first_exp * second_exp);

		}else if (post_exp[i] == "/"){
			stacked.push(first_exp / second_exp);

		}else{
			stacked.push(first_exp);
			stacked.push(second_exp);
			stacked.push(std::strtoll(post_exp[i].c_str(),NULL,10));
		}
    }
	return stacked.top();
}

/*---------------------------------------------------------------------------
 * (function: translate_expression)
 * translate infix expression into postfix expression
 *-------------------------------------------------------------------------*/
void translate_expression(std::vector<std::string> infix_exp, std::vector<std::string> postfix_exp)
{
	std::stack<std::string> stacked;
	for (long i = 0; i < infix_exp.size() ; i++)
	{
		if(infix_exp[i] == "+" || infix_exp[i] == "-" ||  infix_exp[i] == "*" || infix_exp[i] == "/" ){
			if (	(infix_exp[i] == "+" || infix_exp[i] == "-")	 &&
					!stacked.empty()	&&
					( stacked.top() == "*" || stacked.top() == "/" ) )
			{
				while (!stacked.empty()){
			    	postfix_exp.push_back(stacked.top());
			    	stacked.pop();
			  	}
			}
			stacked.push(infix_exp[i]);
		}else{// number
			postfix_exp.push_back(infix_exp[i]);
		}
	}
	while (!stacked.empty()){
    	postfix_exp.push_back(stacked.top());
    	stacked.pop();
  	}
}

/*---------------------------------------------------------------------------
 * (function: reallocate_node)
 *-------------------------------------------------------------------------*/
void reallocate_node(ast_node_t *node, int idx)
{
	node->children[idx] = free_whole_tree(node->children[idx]);

	for (long i = idx; i < node->num_children; i++)
		node->children[i] = node->children[i+1];

	node->num_children = node->num_children - 1;

}

/*---------------------------------------------------------------------------
 * (function: find_most_unique_count)
 *-------------------------------------------------------------------------*/
void find_most_unique_count()
{
	for (long i = 0; i < num_modules; i++)
		if (count_id < ast_modules[i]->unique_count)
			count_id = ast_modules[i]->unique_count;
}

/*---------------------------------------------------------------------------
 * (function: mark_node_write)
 * mark the node that is write
 *-------------------------------------------------------------------------*/
void mark_node_write(ast_node_t *node, std::vector<std::string> list)
{
	if (node && ( node->type == BLOCKING_STATEMENT || node->type == NON_BLOCKING_STATEMENT ) && node->children[0]->type == IDENTIFIERS){
		node->children[0]->is_read_write = 2;
		list.push_back(node->children[0]->types.identifier);
	}

    for (long i = 0; node && i < node->num_children; i++)
    	mark_node_write(node->children[i], list);

}

/*---------------------------------------------------------------------------
 * (function: mark_node_read)
 * mark the node that is read
 *-------------------------------------------------------------------------*/
void mark_node_read(ast_node_t *node, std::vector<std::string> list)
{
	for (long i = 0; node && node->type == IDENTIFIERS && i < list.size() ; i++)
		if ((std::string)node->types.identifier == list[i] && node->is_read_write != 2)
			node->is_read_write = 1;

	for (long j = 0; node && j < node->num_children; j++)
		mark_node_read(node->children[j], list);

}

/*---------------------------------------------------------------------------
 * (function: remove_intermediate_variable)
 * remove the intermediate variables, and prune the syntax tree of FOR loop
 *-------------------------------------------------------------------------*/
void remove_intermediate_variable(ast_node_t *node, std::vector<std::string> list, long virtual_value, std::string virtual_name)
{
	long k = 0;

	for (long i =  0; i < node->num_children-1; i++)
	{
		for (long j = 0; j < count_write; j++)
		{
			ast_node_t *write = search_marked_node(node->children[i], 2, list[j]); //search for "write" nodes
			ast_node_t *read = search_marked_node(node->children[i+1], 1, list[j]); // search for "read" nodes
			for (long n = 0; n < read->num_children; n++){
				if (read->children[n]->is_read_write == 1)
				{
					free_single_node(read->children[n]);
					read->children[n] = get_copy_tree(write->children[1],virtual_value,virtual_name);
				}
			}
		}

		node->children[i] = free_whole_tree(node->children[i]);
	}

	for (long i = 0; i < node->num_children; i++)
		if (node->children[i])
			node->children[k++] = node->children[i];

	for (long i = k; i < node->num_children; i++)
		node->children[i] = NULL;

	node->num_children = k;

}

/*---------------------------------------------------------------------------
 * (function: search_marked_node)
 * search the marked nodes as requirement
 *-------------------------------------------------------------------------*/
ast_node_t *search_marked_node(ast_node_t *node, int is, std::string temp)
{
	for (long i = 0; node && i < node->num_children; i++)
	{
		if(node->children[i]->type == IDENTIFIERS && (std::string)node->children[0]->types.identifier == temp && node->children[0]->is_read_write == is)
			return node;

		ast_node_t *to_return = search_marked_node(node->children[i], is, temp);
		if(to_return){
			return to_return;
		}
	}
	return NULL;

}

/*---------------------------------------------------------------------------
 * (function: reduce_assignment_expression)
 * reduce the number nodes which can be calculated to optimize the AST
 *-------------------------------------------------------------------------*/
void reduce_assignment_expression()
{
	head = NULL;
	p = NULL;
	ast_node_t *T = NULL;


	// find out most unique_count prepared for new AST nodes
	for (long i = 0; i < num_modules; i++)
		if (count_id < ast_modules[i]->unique_count)
			count_id = ast_modules[i]->unique_count;

	for (long i = 0; i < num_modules; i++)
	{
		count_assign = 0;
		std::vector<ast_node_t *> list_assign;
		find_assign_node(ast_modules[i], list_assign);
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
}

/*---------------------------------------------------------------------------
 * (function: search_assign_node)
 * find the top of the assignment expression
 *-------------------------------------------------------------------------*/
void find_assign_node(ast_node_t *node, std::vector<ast_node_t *> list)
{
	if (node && node->num_children>0 && (node->type == BLOCKING_STATEMENT || node->type == NON_BLOCKING_STATEMENT))
		list.push_back(node);

	for (long i = 0; node && i < node->num_children; i++)
		find_assign_node(node->children[i], list);
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
void reduce_parameter()
{
	for (long i = 0; i < num_modules; i++)
	{
		std::vector<ast_node_t *>para;
		find_parameter(ast_modules[i], para);
		if (ast_modules[i]->types.module.is_instantiated == 0 && !para.empty())
			remove_para_node(ast_modules[i], para);

	}

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
	find_assign_node(top, list);
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
void shift_operation()
{
	long i;
	if(ast_modules){
		for (i = 0; i < num_modules; i++){
			search_certain_operation(ast_modules[i]);
		}
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
 * (function: keep_all_branch)
 * keep all the branches and allocate them to the parent node of the FOR node
 TODO ########## POINTER DEREFERENCE!!! ###############
 *-------------------------------------------------------------------------*/
void keep_all_branch(ast_node_t *temp_node, ast_node_t *for_parent, int mark)
{
	long index = mark;
	for (long i = 0; i < temp_node->num_children; i++)
	{
		for_parent->children = (ast_node_t**)vtr::realloc(for_parent->children, sizeof(ast_node_t*)*(for_parent->num_children+1));
		for_parent->num_children ++;
		if (temp_node->children[i]){
			for (long j = for_parent->num_children - 1; j > index; j--)
				for_parent->children[j] = for_parent->children[j-1];

			for_parent->children[++index] = temp_node->children[i];
		}
	}
}
