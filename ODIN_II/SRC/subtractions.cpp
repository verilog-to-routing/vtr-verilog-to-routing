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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "odin_types.h"
#include "node_creation_library.h"
#include "adders.h"
#include "subtractions.h"
#include "netlist_utils.h"
#include "partial_map.h"
#include "read_xml_arch_file.h"
#include "odin_globals.h"


#include "vtr_memory.h"
#include "vtr_util.h"


using vtr::t_linked_vptr;

t_linked_vptr *sub_list = NULL;
t_linked_vptr *sub_chain_list = NULL;
int subchaintotal = 0;
int *sub = NULL;

void init_split_adder_for_sub(nnode_t *node, nnode_t *ptr, int a, int sizea, int b, int sizeb, int cin, int cout, int index, int flag);

/*---------------------------------------------------------------------------
 * (function: report_sub_distribution)
 *-------------------------------------------------------------------------*/

/* These values are collected during the unused logic removal sweep */
extern long subtractor_chain_count;
extern long longest_subtractor_chain;
extern long total_subtractors;

void report_sub_distribution()
{
	if(hard_adders == NULL)
		return;

	printf("\nHard MINUS Distribution\n");
	printf("============================\n");
	printf("\n");
	printf("\nTotal # of chains = %ld\n", subtractor_chain_count);

	printf("\nHard sub chain Details\n");
	printf("============================\n");

	printf("\n");
	printf("\nThe Number of Hard Block subs in the Longest Chain: %ld\n", longest_subtractor_chain);

	printf("\n");
	printf("\nThe Total Number of Hard Block subs: %ld\n", total_subtractors);

	return;
}

/*---------------------------------------------------------------------------
 * (function: declare_hard_adder_for_sub)
 *-------------------------------------------------------------------------*/
void declare_hard_adder_for_sub(nnode_t *node)
{
	t_adder *tmp;
	int width_a, width_b, width_sumout;

	/* See if this size instance of adder exists?*/
	if (hard_adders == NULL)
	{
		warning_message(NETLIST_ERROR, node->related_ast_node->line_number, node->related_ast_node->file_number, "%s\n", "Instantiating Substraction where hard adders do not exist");
	}
	tmp = (t_adder *)hard_adders->instances;
	width_a = node->input_port_sizes[0];
	width_b = node->input_port_sizes[1];
	width_sumout = node->output_port_sizes[1];

	while (tmp != NULL)
	{
		if ((tmp->size_a == width_a) && (tmp->size_b == width_b) && (tmp->size_sumout == width_sumout))
			return;
		else
			tmp = tmp->next;
	}

	/* Does not exist - must create an instance*/
	tmp = (t_adder *)vtr::malloc(sizeof(t_adder));
	tmp->next = (t_adder *)hard_adders->instances;
	hard_adders->instances = tmp;
	tmp->size_a = width_a;
	tmp->size_b = width_b;
	tmp->size_cin = 1;
	tmp->size_cout = 1;
	tmp->size_sumout = width_sumout;
	return;
}

/*---------------------------------------------------------------------------
 * (function: instantiate_hard_adder_subtraction )
 *-------------------------------------------------------------------------*/
void instantiate_hard_adder_subtraction(nnode_t *node, short mark, netlist_t * /*netlist*/)
{
	char *new_name;
	int len, sanity, i;

	declare_hard_adder_for_sub(node);

	/* Need to give node proper name */
	len = strlen(node->name);
	len = len + 20; /* 20 chars should hold mul specs */
	new_name = (char*)vtr::malloc(len);

	/* wide input first :) identical branching! ? */
	//if (node->input_port_sizes[0] > node->input_port_sizes[1])
		sanity = odin_sprintf(new_name, "%s", node->name);
	// else
	// 	sanity = odin_sprintf(new_name, "%s", node->name);

	if (len <= sanity) /* buffer not large enough */
		oassert(FALSE);

	/* Give names to the output pins */
	for (i = 0; i < node->num_output_pins;  i++)
	{
		if (node->output_pins[i]->name ==NULL)
		{
			len = strlen(node->name) + 20; /* 6 chars for pin idx */
			new_name = (char*)vtr::malloc(len);
			odin_sprintf(new_name, "%s[%ld]", node->name, node->output_pins[i]->pin_node_idx);
			node->output_pins[i]->name = new_name;
		}
	}

	node->traverse_visited = mark;
	return;
}

/*-----------------------------------------------------------------------
 * (function: init_split_adder)
 *	Create a carry chain adder when spliting. Inputs are connected
 *	to original pins, output pins are set to NULL for later connecting
 *---------------------------------------------------------------------*/
void init_split_adder_for_sub(nnode_t *node, nnode_t *ptr, int a, int sizea, int b, int sizeb, int cin, int cout, int index, int flag)
{
	int i;
	int flaga = 0;
	int current_sizea, current_sizeb;
	int aa = 0;
	int num = 0;

    // if the input of the first cin is generated by a dummy adder added
    // to the start of the chain, then an offset is needed to compensate
    // for that in various positions in the code, otherwise the offset is 0
    const int offset = (configuration.adder_cin_global)? 0 : 1;

	/* Copy properties from original node */
	ptr->type = node->type;
	ptr->related_ast_node = node->related_ast_node;
	ptr->traverse_visited = node->traverse_visited;
	ptr->node_data = NULL;

	/* decide the current size of input a and b */
	if(flag == 0)
	{
		current_sizea = (a + offset) - sizea * index;
		current_sizeb = (b + offset) - sizeb * index;
		if(current_sizea >= sizea)
			current_sizea = sizea;
		else if(current_sizea <= 0)
		{
			current_sizea = sizea;
			flaga = 1;
		}
		else
		{
			aa = current_sizea;
			current_sizea = sizea;
			flaga = 2;
		}
		current_sizeb = sizeb;
	}
	else
	{
		if(sizea != 0)
			current_sizea = sizea;
		else
			current_sizea = 1;
		if(sizeb != 0)
			current_sizeb = sizeb;
		else
			current_sizeb = 1;
	}


	/* Set new port sizes and parameters */
	ptr->num_input_port_sizes = 3;
	ptr->input_port_sizes = (int *)vtr::malloc(3 * sizeof(int));
	ptr->input_port_sizes[0] = current_sizea;
	ptr->input_port_sizes[1] = current_sizeb;
	ptr->input_port_sizes[2] = cin;
	ptr->num_output_port_sizes = 2;
	ptr->output_port_sizes = (int *)vtr::malloc(2 * sizeof(int));
	ptr->output_port_sizes[0] = cout;

	/* The size of output port sumout equals the maxim size of a and b  */
	if(current_sizea > current_sizeb)
		ptr->output_port_sizes[1] = current_sizea;
	else
		ptr->output_port_sizes[1] = current_sizeb;

	/* Set the number of pins and re-locate previous pin entries */
	ptr->num_input_pins = current_sizea + current_sizeb + cin;
	ptr->input_pins = (npin_t**)vtr::malloc(sizeof(void *) * (current_sizea + current_sizeb + cin));
	//the normal sub: if flaga or flagb = 1, the input pins should be empty.
	//the unary sub: all input pins for a should be null, input pins for b should be connected to node
	if(node->num_input_port_sizes == 1)
	{
		for (i = 0; i < current_sizea; i++)
			ptr->input_pins[i] = NULL;
	}
	else if((flaga == 1) && (node->num_input_port_sizes == 2))
	{
		for (i = 0; i < current_sizea; i++)
			ptr->input_pins[i] = NULL;
	}
	else if((flaga == 2) && (node->num_input_port_sizes == 2))
	{
		if(index == 0)
		{
			ptr->input_pins[0] = NULL;
			if(sizea > 1)
			{
				for (i = 1; i < aa; i++)
				{
					ptr->input_pins[i] = node->input_pins[i + index * sizea - 1];
					ptr->input_pins[i]->node = ptr;
					ptr->input_pins[i]->pin_node_idx = i;
				}
				for (i = 0; i < (sizea - aa); i++)
					ptr->input_pins[i + aa] = NULL;
			}
		}
		else
		{
			for (i = 0; i < aa; i++)
			{
				ptr->input_pins[i] = node->input_pins[i + index * sizea - 1];
				ptr->input_pins[i]->node = ptr;
				ptr->input_pins[i]->pin_node_idx = i;
			}
			for (i = 0; i < (sizea - aa); i++)
				ptr->input_pins[i + aa] = NULL;
		}
	}
	else
	{
		if(index == 0 && !configuration.adder_cin_global)
		{
			if(flag == 0)
			{
				ptr->input_pins[0] = NULL;
				if(sizea > 1)
				{
					for (i = 1; i < sizea; i++)
					{
						ptr->input_pins[i] = node->input_pins[i + index * sizea - 1];
						ptr->input_pins[i]->node = ptr;
						ptr->input_pins[i]->pin_node_idx = i;
					}
				}
			}
			else
			{
				for (i = 0; i < current_sizea; i++)
				{
					ptr->input_pins[i] = node->input_pins[i];
					ptr->input_pins[i]->node = ptr;
					ptr->input_pins[i]->pin_node_idx = i;
				}
			}
		}
		else
		{
			if(flag == 0)
			{
				for (i = 0; i < sizea; i++)
				{
					ptr->input_pins[i] = node->input_pins[i + index * sizea - offset];
					ptr->input_pins[i]->node = ptr;
					ptr->input_pins[i]->pin_node_idx = i;
				}
			}
			else
			{
				num = node->input_port_sizes[0];
				for (i = 0; i < current_sizea; i++)
				{
					ptr->input_pins[i] = node->input_pins[i + num - current_sizea];
					ptr->input_pins[i]->node = ptr;
					ptr->input_pins[i]->pin_node_idx = i;
				}
			}
		}
	}

	for (i = 0; i < current_sizeb; i++)
		ptr->input_pins[i + current_sizeb] = NULL;

	/* Carry_in should be NULL*/
	for (i = 0; i < cin; i++)
	{
		ptr->input_pins[i+current_sizea+current_sizeb] = NULL;
	}

	/* output pins */
	int output;
	if(current_sizea > current_sizeb)
		output = current_sizea + cout;
	else
		output = current_sizeb + cout;

	ptr->num_output_pins = output;
	ptr->output_pins = (npin_t**)vtr::malloc(sizeof(void *) * output);
	for (i = 0; i < output; i++)
		ptr->output_pins[i] = NULL;

	return;
}

/*-------------------------------------------------------------------------
 * (function: split_adder)
 *
 * This function works to split a adder into several smaller
 *  adders to better "fit" with the available resources in a
 *  targeted FPGA architecture.
 *
 * This function is at the lowest level since it simply receives
 *  a adder and is told how to split it.
 *
 * Note that for some of the additions we need to perform sign extensions,
 * but this should not be a problem since the sign extension is always
 * extending NOT contracting.
 *-----------------------------------------------------------------------*/

void split_adder_for_sub(nnode_t *nodeo, int a, int b, int sizea, int sizeb, int cin, int cout, int count, netlist_t *netlist)
{
	nnode_t **node;
	nnode_t **not_node;
	int i,j;
	int num;
	int max_num = 0;
	int flag = 0, lefta = 0, leftb = 0;

    // if the input of the first cin is generated by a dummy adder added
    // to the start of the chain, then an offset is needed to compensate
    // for that in various positions in the code, otherwise the offset is 0
    const int offset = (configuration.adder_cin_global)? 0 : 1;

	/* Check for a legitimate split */
	if(nodeo->num_input_port_sizes == 2)
	{
		oassert(nodeo->input_port_sizes[0] == a);
		oassert(nodeo->input_port_sizes[1] == b);
	}
	else
	{
		oassert(nodeo->input_port_sizes[0] == a);
		oassert(nodeo->input_port_sizes[0] == b);
	}

	node  = (nnode_t**)vtr::malloc(sizeof(nnode_t*)*(count));
	not_node = (nnode_t**)vtr::malloc(sizeof(nnode_t*)*(b));

	for(i = 0; i < b; i++)
	{
		not_node[i] = allocate_nnode();
		if(nodeo->num_input_port_sizes == 2)
			not_node[i] = make_not_gate_with_input(nodeo->input_pins[a + i], not_node[i], -1);
		else
			not_node[i] = make_not_gate_with_input(nodeo->input_pins[i], not_node[i], -1);
	}

	for(i = 0; i < count; i++)
	{
		node[i] = allocate_nnode();
		node[i]->name = (char *)vtr::malloc(strlen(nodeo->name) + 20);
		odin_sprintf(node[i]->name, "%s-%ld", nodeo->name, i);
		if(i == count - 1)
		{
			if(configuration.fixed_hard_adder == 1)
				init_split_adder_for_sub(nodeo, node[i], a, sizea, b, sizeb, cin, cout, i, flag);
			else
			{
				if(count == 1)
				{
					lefta = a;
					leftb = b;
				}
				else
				{
					lefta = (a + 1) % sizea;
					leftb = (b + 1) % sizeb;
				}

				max_num = (lefta >= leftb)? lefta: leftb;
				// if fixed_hard_adder = 0, and the left of a and b is more than min_add, then adder need to be remain the same size.
				if(max_num >= min_add || lefta + leftb == 0)
					init_split_adder_for_sub(nodeo, node[i], a, sizea, b, sizeb, cin, cout, i, flag);
				else
				{
					// Using soft logic to do the addition, No need to pad as the same size
					flag = 1;
					init_split_adder_for_sub(nodeo, node[i], a, lefta, b, leftb, cin, cout, i, flag);
				}
			}
		}
		else
			init_split_adder_for_sub(nodeo, node[i], a, sizea, b, sizeb, cin, cout, i, flag);

		//store the processed hard adder node for optimization
		processed_adder_list = insert_in_vptr_list(processed_adder_list, node[i]);
	}

	chain_information_t *adder_chain = allocate_chain_info();
	//if flag = 0, the last adder use soft logic, so the count of the chain should be one less
	if(flag == 0)
		adder_chain->count = count;
	else
		adder_chain->count = count - 1;
	adder_chain->num_bits = a + b;
	adder_chain->name = nodeo->name;
	sub_chain_list = insert_in_vptr_list(sub_chain_list, adder_chain);

	if(flag == 1 && count == 1)
	{
		for(i = 0; i < b; i ++)
		{
			/* If the input pin of not gate connects to gnd, replacing the input pin and the not gate with vcc;
			 * if the input pin of not gate connects to vcc, replacing the input pin and the not gate with gnd.*/
			if(not_node[i]->input_pins[0]->net->driver_pin->node->type == GND_NODE)
			{
				connect_nodes(netlist->vcc_node, 0, node[0], (lefta + i));
				remove_fanout_pins_from_net(not_node[i]->input_pins[0]->net, not_node[i]->input_pins[0], not_node[i]->input_pins[0]->pin_net_idx);
				free_nnode(not_node[i]);
			}
			else if(not_node[i]->input_pins[0]->net->driver_pin->node->type == VCC_NODE)
			{
				connect_nodes(netlist->gnd_node, 0, node[0], (lefta + i));
				remove_fanout_pins_from_net(not_node[i]->input_pins[0]->net, not_node[i]->input_pins[0], not_node[i]->input_pins[0]->pin_net_idx);
				free_nnode(not_node[i]);
			}
			else
				connect_nodes(not_node[i], 0, node[0], (lefta + i));
		}
	}
	else
	{
		if(sizeb > 1)
		{
			if((b + 1) < sizeb)
				num = b;
			else
				num = sizeb - 1;
			for(i = 0; i < num; i++)
			{
				/* If the input pin of not gate connects to gnd, replacing the input pin and the not gate with vcc;
				 * if the input pin of not gate connects to vcc, replacing the input pin and the not gate with gnd.*/
				if(not_node[i]->input_pins[0]->net->driver_pin->node->type == GND_NODE)
				{
					connect_nodes(netlist->vcc_node, 0, node[0], (sizea + i + 1));
					remove_fanout_pins_from_net(not_node[i]->input_pins[0]->net, not_node[i]->input_pins[0], not_node[i]->input_pins[0]->pin_net_idx);
					free_nnode(not_node[i]);
				}
				else if(not_node[i]->input_pins[0]->net->driver_pin->node->type == VCC_NODE)
				{
					connect_nodes(netlist->gnd_node, 0, node[0], (sizea + i + 1));
					remove_fanout_pins_from_net(not_node[i]->input_pins[0]->net, not_node[i]->input_pins[0], not_node[i]->input_pins[0]->pin_net_idx);
					free_nnode(not_node[i]);
				}
				else
					connect_nodes(not_node[i], 0, node[0], (sizea + i + 1));
			}
		}

		for(i = offset; i<count; i++)
		{
			num = (b + 1) - i * sizeb;
			if(num > sizeb)
				num = sizeb;

			for(j = 0; j < num; j++)
			{
				if(i == count - 1 && flag == 1)
				{
					/* If the input pin of not gate connects to gnd, replacing the input pin and the not gate with vcc;
					 * if the input pin of not gate connects to vcc, replacing the input pin and the not gate with gnd.*/
					if(not_node[(i * sizeb + j - 1)]->input_pins[0]->net->driver_pin->node->type == GND_NODE)
					{
						connect_nodes(netlist->vcc_node, 0, node[i], (lefta + j));
						remove_fanout_pins_from_net(not_node[(i * sizeb + j - 1)]->input_pins[0]->net, not_node[(i * sizeb + j - 1)]->input_pins[0], not_node[(i * sizeb + j - 1)]->input_pins[0]->pin_net_idx);
						free_nnode(not_node[(i * sizeb + j - 1)]);
					}
					else if(not_node[(i * sizeb + j - 1)]->input_pins[0]->net->driver_pin->node->type == VCC_NODE)
					{
						connect_nodes(netlist->gnd_node, 0, node[i], (lefta + j));
						remove_fanout_pins_from_net(not_node[(i * sizeb + j - 1)]->input_pins[0]->net, not_node[(i * sizeb + j - 1)]->input_pins[0], not_node[(i * sizeb + j - 1)]->input_pins[0]->pin_net_idx);
						free_nnode(not_node[(i * sizeb + j - 1)]);
					}
					else
						connect_nodes(not_node[(i * sizeb + j - 1)], 0, node[i], (lefta + j));
				}
				else
				{
					/* If the input pin of not gate connects to gnd, replacing the input pin and the not gate with vcc;
					 * if the input pin of not gate connects to vcc, replacing the input pin and the not gate with gnd.*/
                    const int index = i *sizeb + j - offset;
					if(not_node[index]->input_pins[0]->net->driver_pin->node->type == GND_NODE)
					{
						connect_nodes(netlist->vcc_node, 0, node[i], (sizea + j));
						remove_fanout_pins_from_net(not_node[index]->input_pins[0]->net, not_node[index]->input_pins[0], not_node[index]->input_pins[0]->pin_net_idx);
						free_nnode(not_node[index]);
					}
					else if(not_node[index]->input_pins[0]->net->driver_pin->node->type == VCC_NODE)
					{
						connect_nodes(netlist->gnd_node, 0, node[i], (sizea + j));
						remove_fanout_pins_from_net(not_node[index]->input_pins[0]->net, not_node[index]->input_pins[0], not_node[index]->input_pins[0]->pin_net_idx);
						free_nnode(not_node[index]);
					}
					else
						connect_nodes(not_node[index], 0, node[i], (sizea + j));
				}
			}
		}
	}

	if((flag == 0 || count > 1) && !configuration.adder_cin_global)
	{
		//connect the a[0] of first adder node to ground, and b[0] of first adder node to vcc
		connect_nodes(netlist->gnd_node, 0, node[0], 0);
		connect_nodes(netlist->vcc_node, 0, node[0], sizea);
		//hang the first sumout
		node[0]->output_pins[1] = allocate_npin();
		node[0]->output_pins[1]->name = append_string("", "%s~dummy_output~%ld~%ld", node[0]->name, 0, 1);
	}

	// connect the first cin pin to vcc or unconn depending on configuration
	if((flag == 1 && count == 1) || configuration.adder_cin_global)
		connect_nodes(netlist->vcc_node, 0, node[0], node[0]->num_input_pins - 1);
	else
		connect_nodes(netlist->pad_node, 0, node[0], node[0]->num_input_pins - 1);

	//for normal subtraction: if any input pins beside intial cin is NULL, it should connect to unconn
	//for unary subtraction: the first number should has the number of a input pins connected to gnd. The others are as same as normal subtraction
	for(i = 0; i < count; i++)
	{
		num = node[i]->num_input_pins;
		for(j = 0; j < num - 1; j++)
		{
			if(node[i]->input_pins[j] == NULL)
			{
				if(nodeo->num_input_port_sizes != 3 && i * sizea + j < a)
					connect_nodes(netlist->gnd_node, 0, node[i], j);
				else
					connect_nodes(netlist->pad_node, 0, node[i], j);
			}
		}
	}

	//connect cout to next node's cin
	for(i = 1; i < count; i++)
		connect_nodes(node[i-1], 0, node[i], (node[i]->num_input_pins - 1));

	if(flag == 1 && count == 1)
	{
		for(j = 0; j < node[0]->num_output_pins - 1; j ++)
		{
			if(j < nodeo->num_output_pins)
				remap_pin_to_new_node(nodeo->output_pins[j], node[0], j + 1);
			else
			{
				node[0]->output_pins[j + 1] = allocate_npin();
				// Pad outputs with a unique and descriptive name to avoid collisions.
				node[0]->output_pins[j + 1]->name = append_string("", "%s~dummy_output~%ld~%ld", node[0]->name, 0, j + 1);
			}
		}
	}
	else
	{
		for(j = 0; j < node[0]->num_output_pins - 2; j ++)
		{
			if(j < nodeo->num_output_pins)
				remap_pin_to_new_node(nodeo->output_pins[j], node[0], j + 2);
			else
			{
				node[0]->output_pins[j + 2] = allocate_npin();
				// Pad outputs with a unique and descriptive name to avoid collisions.
				node[0]->output_pins[j + 2]->name = append_string("", "%s~dummy_output~%ld~%ld", node[0]->name, 0, j + 2);
			}
		}
	}

	if(count > 1 || configuration.adder_cin_global)
	{
		//remap the output pins of each adder to nodeo
		for(i = offset; i < count; i++)
		{
			for(j = 0; j < node[i]->num_output_pins - 1; j ++)
			{
				if((i * sizea + j - offset) < nodeo->num_output_pins)
					remap_pin_to_new_node(nodeo->output_pins[i * sizea + j - offset], node[i], j + 1);
				else
				{
					node[i]->output_pins[j + 1] = allocate_npin();
					// Pad outputs with a unique and descriptive name to avoid collisions.
				    node[i]->output_pins[j + 1]->name = append_string("", "%s~dummy_output~%ld~%ld", node[i]->name, i, j + 2);
				}
			}
		}
	}
		node[count - 1]->output_pins[0] = allocate_npin();
		// Pad outputs with a unique and descriptive name to avoid collisions.
		node[count - 1]->output_pins[0]->name = append_string("", "%s~dummy_output~%ld~%ld", node[(count - 1)]->name, (count - 1), 0);
		//connect_nodes(node[count - 1], (node[(count - 1)]->num_output_pins - 1), netlist->gnd_node, 0);
	//}


	/* Probably more to do here in freeing the old node! */
	vtr::free(nodeo->name);
	vtr::free(nodeo->input_port_sizes);
	vtr::free(nodeo->output_port_sizes);

	/* Free arrays NOT the pins since relocated! */
	vtr::free(nodeo->input_pins);
	vtr::free(nodeo->output_pins);
	vtr::free(nodeo);
	vtr::free(node);
	vtr::free(not_node);
	return;
}


/*-------------------------------------------------------------------------
 * (function: iterate_adders_for_sub)
 *
 * This function will iterate over all of the minus operations that
 *	exist in the netlist and perform a splitting so that they can
 *	fit into a basic hard adder block that exists on the FPGA.
 *	If the proper option is set, then it will be expanded as well
 *	to just use a fixed size hard adder.
 *-----------------------------------------------------------------------*/
void iterate_adders_for_sub(netlist_t *netlist)
{
	int sizea, sizeb, sizecin;//the size of
	int a, b;
	int count,counta,countb;
	int num;
	nnode_t *node;

    const int offset = (configuration.adder_cin_global)? 0 : 1;

	/* Can only perform the optimisation if hard adders exist! */
	if (hard_adders == NULL)
		return;
	else
	{
		//In hard block adder, the summand and addend are same size.
		sizecin = hard_adders->inputs->size;
		sizeb = hard_adders->inputs->next->size;
		sizea = hard_adders->inputs->next->size;

		oassert(sizecin == 1);

		while (sub_list != NULL)
		{
			node = (nnode_t *)sub_list->data_vptr;
			sub_list = delete_in_vptr_list(sub_list);

			oassert(node != NULL);
			oassert(node->type == MINUS);

			a = node->input_port_sizes[0];
			if(node->num_input_port_sizes == 2)
				b = node->input_port_sizes[1];
			else
				b = node->input_port_sizes[0];
			num = (a >= b)? a : b;

			if(num >= min_threshold_adder)
			{
				// how many subtractors base on a can split
				if((a + 1) % sizea == 0)
					counta = (a + offset) / sizea;
				else
					counta = (a + 1) / sizea + 1;
				// how many subtractors base on b can split
				if((b + 1) % sizeb == 0)
					countb = (b + offset) / sizeb;
				else
					countb = (b + 1) / sizeb + 1;
				// how many subtractors need to be split
				if(counta >= countb)
					count = counta;
				else
					count = countb;
				subchaintotal++;

				split_adder_for_sub(node, a, b, sizea, sizeb, 1, 1, count, netlist);
			}
			// Store the node into processed_adder_list if the threshold is bigger than num
			else
				processed_adder_list = insert_in_vptr_list(processed_adder_list, node);
		}
	}

	return;
}

/*-------------------------------------------------------------------------
 * (function: clean_adders)
 *
 * Clean up the memory by deleting the list structure of adders
 *	during optimization
 *-----------------------------------------------------------------------*/
void clean_adders_for_sub()
{
	while (sub_list != NULL)
		sub_list = delete_in_vptr_list(sub_list);
	while (processed_adder_list != NULL)
			processed_adder_list = delete_in_vptr_list(processed_adder_list);
	return;
}

