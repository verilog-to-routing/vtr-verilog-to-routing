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
#include "types.h"
#include "node_creation_library.h"
#include "adders.h"
#include "subtractions.h"
#include "netlist_utils.h"
#include "partial_map.h"
#include "util.h"
#include "read_xml_arch_file.h"
#include "globals.h"
#include "errors.h"

t_model *hard_subs = NULL;
struct s_linked_vptr *subs_list = NULL;
struct s_linked_vptr *sub_list = NULL;
struct s_linked_vptr *sub_one_node_list = NULL;
struct s_linked_vptr *subs_one_node_list = NULL;
struct s_linked_vptr *sub_chain_list = NULL;
int subchaintotal = 0;
int *sub = NULL;

#ifdef VPR6

/*---------------------------------------------------------------------------
 * (function: init_add_distribution)
 *  For adder, the output will only be the maxim input size + 1
 *-------------------------------------------------------------------------*/
void init_sub_distribution()
{
	int i;
	oassert(hard_subs != NULL);
	sub = (int *)malloc(sizeof(int) * (hard_subs->inputs->size + 1));
	for (i = 0; i <= (hard_subs->inputs->size + hard_subs->inputs->next->size); i++)
		sub[i] = 0;
	return;
}

/*---------------------------------------------------------------------------
 * (function: report_add_distribution)
 *-------------------------------------------------------------------------*/

void report_sub_distribution()
{
	int max = 0;
	int totalsub = 0;
	chain_information_t *sub_chain;

	if(hard_subs == NULL)
		return;

	printf("\nHard MINUS Distribution\n");
	printf("============================\n");
	printf("\n");
	printf("\nTotal # of chains = %d\n", subchaintotal);

	printf("\nHard sub chain Details\n");
	printf("============================\n");

	while(sub_chain_list != NULL)
	{
		sub_chain = (chain_information_t *)sub_chain_list->data_vptr;
		sub_chain_list = delete_in_vptr_list(sub_chain_list);
		totalsub = totalsub + sub_chain->count;
		if(max < sub_chain->count)
			max = sub_chain->count;
	}
	printf("\n");
	printf("\nThe Number of Hard Block subs in the Longest Chain: %d\n", max);

	printf("\n");
	printf("\nThe Total Number of Hard Block subs: %d\n", totalsub);


	return;
}

/*---------------------------------------------------------------------------
 * (function: find_hard_adders_for_sub)
 *-------------------------------------------------------------------------*/
void find_hard_adders_for_sub()
{

	hard_subs = Arch.models;

	while (hard_subs != NULL)
	{
		if (strcmp(hard_subs->name, "sub") == 0)
		{
			init_sub_distribution();
			return;
		}
		else
		{
			hard_subs = hard_subs->next;
		}
	}
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
	if (hard_subs == NULL)
	{
		printf(ERRTAG "Instantiating subtraction where subs do not exist\n");
	}
	tmp = (t_adder *)hard_subs->instances;
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
	tmp = (t_adder *)malloc(sizeof(t_adder));
	tmp->next = (t_adder *)hard_subs->instances;
	hard_subs->instances = tmp;
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
void instantiate_hard_adder_subtraction(nnode_t *node, short mark, netlist_t *netlist, int type)
{
	char *new_name;
	int len, sanity, i;

	declare_hard_adder_for_sub(node);

	/* Need to give node proper name */
	len = strlen(node->name);
	len = len + 20; /* 20 chars should hold mul specs */
	new_name = (char*)malloc(len);

	/* wide input first :) */
	if (node->input_port_sizes[0] > node->input_port_sizes[1])
		sanity = sprintf(new_name, "%s", node->name);
	else
		sanity = sprintf(new_name, "%s", node->name);

	if (len <= sanity) /* buffer not large enough */
		oassert(FALSE);

	/* Give names to the output pins */
	for (i = 0; i < node->num_output_pins;  i++)
	{
		if (node->output_pins[i]->name ==NULL)
		{
			len = strlen(node->name) + 6; /* 6 chars for pin idx */
			new_name = (char*)malloc(len);
			sprintf(new_name, "%s[%d]", node->name, node->output_pins[i]->pin_node_idx);
			node->output_pins[i]->name = new_name;
		}
	}

	free(node->name);
	node->name = new_name;
	node->traverse_visited = mark;
	return;
}

/*----------------------------------------------------------------------------
 * function: add_the_blackbox_for_subs()
 *--------------------------------------------------------------------------*/
void add_the_blackbox_for_subs(FILE *out)
{
	int i;
	int count;
	int hard_add_inputs, hard_add_outputs;
	t_adder *subs;
	t_model_ports *ports;
	char buffer[MAX_BUF];
	char *pa, *pb, *psumout, *pcin, *pcout;

	/* Check to make sure this target architecture has hard adders */
	if (hard_subs == NULL)
		return;

	/* Get the names of the ports for the adder */
	ports = hard_subs->inputs;
	pcin = ports->name;
	ports = ports->next;
	pb = ports->name;
	ports = ports->next;
	pa = ports->name;

	ports = hard_subs->outputs;
	psumout = ports->name;
	ports = ports->next;
	pcout = ports->name;

	/* find the adder devices in the tech library */
	subs = (t_adder *)(hard_subs->instances);
	if (subs == NULL) /* No adders instantiated */
		return;

	/* simplified way of getting the multsize, but fine for quick example */
	while (subs != NULL)
	{
		/* write out this adder model */
		if (configuration.fixed_hard_adder != 0)
			count = fprintf(out, ".model sub\n");
		else
			count = fprintf(out, ".model sub\n");

		/* add the inputs */
		count = count + fprintf(out, ".inputs");
		hard_add_inputs = subs->size_a + subs->size_b + subs->size_cin;
		for (i = 0; i < hard_add_inputs; i++)
		{
			if (i < subs->size_a)
			{
				count = count + sprintf(buffer, " %s[%d]", pa, i);
			}
			else if(i < hard_add_inputs - subs->size_cin && i >= subs->size_a)
			{
				count = count + sprintf(buffer, " %s[%d]", pb, i - subs->size_a);
			}
			else
			{
				count = count + sprintf(buffer, " %s[%d]", pcin, i - subs->size_a - subs->size_b);
			}
			if (count > 78)
				count = fprintf(out, " \\\n %s", buffer) - 3;
			else
				fprintf(out, " %s", buffer);
		}
		fprintf(out, "\n");

		/* add the outputs */
		count = fprintf(out, ".outputs");
		hard_add_outputs = subs->size_cout + subs->size_sumout;
		for (i = 0; i < hard_add_outputs; i++)
		{
			if (i < subs->size_sumout)
			{
				count = count + sprintf(buffer, " %s[%d]", psumout, i);
			}
			else
			{
				count = count + sprintf(buffer, " %s[%d]", pcout, i - subs->size_sumout);
			}

			if (count > 78)
			{
				fprintf(out, " \\\n%s", buffer);
				count = strlen(buffer);
			}
			else
				fprintf(out, "%s", buffer);
		}
		fprintf(out, "\n");
		fprintf(out, ".blackbox\n");
		fprintf(out, ".end\n");
		fprintf(out, "\n");

		subs = subs->next;
	}
}


/*-------------------------------------------------------------------------
 * (function: define_sub_function)
 *-----------------------------------------------------------------------*/
void define_sub_function(nnode_t *node, short type, FILE *out)
{
	int i, j;
	int count;
	char buffer[MAX_BUF];

	count = fprintf(out, "\n.subckt");
	count--;
	oassert(node->input_port_sizes[0] > 0);
	oassert(node->input_port_sizes[1] > 0);
	//oassert(node->input_port_sizes[2] > 0);
	oassert(node->output_port_sizes[0] > 0);
	oassert(node->output_port_sizes[1] > 0);

	/* Write out the model adder  */
	if (configuration.fixed_hard_adder != 0)
	{
		count += fprintf(out, " sub");
	}
	else
	{
		count += fprintf(out, " sub");
	}

	/* Write the input pins*/
	for (i = 0;  i < node->num_input_pins; i++)
	{
		npin_t *driver_pin = node->input_pins[i]->net->driver_pin;

		if (i < node->input_port_sizes[0])
		{
			if (!driver_pin->name)
				j = sprintf(buffer, " %s[%d]=%s", hard_subs->inputs->next->next->name, i, driver_pin->node->name);
			else
				j = sprintf(buffer, " %s[%d]=%s", hard_subs->inputs->next->next->name, i, driver_pin->name);
		}
		else if(i >= node->input_port_sizes[0] && i < node->input_port_sizes[1] + node->input_port_sizes[0])
		{
			if (!driver_pin->name)
				j = sprintf(buffer, " %s[%d]=%s", hard_subs->inputs->next->name, i - node->input_port_sizes[0], driver_pin->node->name);
			else
				j = sprintf(buffer, " %s[%d]=%s", hard_subs->inputs->next->name, i - node->input_port_sizes[0], driver_pin->name);
		}
		else
		{
			if (!driver_pin->name)
				j = sprintf(buffer, " %s[%d]=%s", hard_subs->inputs->name, i - (node->input_port_sizes[0] + node->input_port_sizes[1]), driver_pin->node->name);
			else
				j = sprintf(buffer, " %s[%d]=%s", hard_subs->inputs->name, i - (node->input_port_sizes[0] + node->input_port_sizes[1]), driver_pin->name);
		}

		if (count + j > 79)
		{
			fprintf(out, "\\\n");
			count = 0;
		}
		count += fprintf(out, "%s", buffer);
	}

	/* Write the output pins*/
	for (i = 0; i < node->num_output_pins; i++)
	{
		if(i < node->output_port_sizes[0])
			j = sprintf(buffer, " %s[%d]=%s", hard_subs->outputs->next->name, i , node->output_pins[i]->name);
		else
			j = sprintf(buffer, " %s[%d]=%s", hard_subs->outputs->name, i - node->output_port_sizes[0], node->output_pins[i]->name);
		if (count + j > 79)
		{
			fprintf(out, "\\\n");
			count = 0;
		}
		count += fprintf(out, "%s", buffer);
	}

	fprintf(out, "\n\n");
	return;
}


/*-----------------------------------------------------------------------
 * (function: init_split_adder)
 *	Create a carry chain adder when spliting. Inputs are connected
 *	to original pins, output pins are set to NULL for later connecting
 *---------------------------------------------------------------------*/
void init_split_adder_for_sub(nnode_t *node, nnode_t *ptr, int a, int sizea, int b, int sizeb, int cin, int cout, int index)
{
	int i;
	int flaga = 0, flagb = 0;
	int current_sizea, current_sizeb;
	int aa = 0, bb = 0;

	/* Copy properties from original node */
	ptr->type = node->type;
	ptr->related_ast_node = node->related_ast_node;
	ptr->traverse_visited = node->traverse_visited;
	ptr->node_data = NULL;

	/* decide the current size of input a and b */
	current_sizea = a - sizea * index;
	current_sizeb = b - sizeb * index;
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

	if(current_sizeb >= sizeb)
		current_sizeb = sizeb;
	else if(current_sizeb <= 0)
	{
		current_sizeb = sizeb;
		flagb = 1;
	}
	else
	{
		bb = current_sizeb;
		current_sizeb = sizeb;
		flagb = 2;
	}

	/* Set new port sizes and parameters */
	ptr->num_input_port_sizes = 3;
	ptr->input_port_sizes = (int *)malloc(3 * sizeof(int));
	ptr->input_port_sizes[0] = current_sizea;
	ptr->input_port_sizes[1] = current_sizeb;
	ptr->input_port_sizes[2] = cin;
	ptr->num_output_port_sizes = 2;
	ptr->output_port_sizes = (int *)malloc(2 * sizeof(int));
	ptr->output_port_sizes[0] = cout;

	/* The size of output port sumout equals the maxim size of a and b  */
	if(current_sizea > current_sizeb)
		ptr->output_port_sizes[1] = current_sizea;
	else
		ptr->output_port_sizes[1] = current_sizeb;

	/* Set the number of pins and re-locate previous pin entries */
	ptr->num_input_pins = current_sizea + current_sizeb + cin;
	ptr->input_pins = (npin_t**)malloc(sizeof(void *) * (current_sizea + current_sizeb + cin));
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
		for (i = 0; i < aa; i++)
		{
			ptr->input_pins[i] = node->input_pins[i + index * sizea];
			ptr->input_pins[i]->node = ptr;
		}
		for (i = 0; i < (sizea - aa); i++)
			ptr->input_pins[i + aa] = NULL;
	}
	else
	{
		for (i = 0; i < current_sizea; i++)
		{
			ptr->input_pins[i] = node->input_pins[i + index * sizea];
			ptr->input_pins[i]->node = ptr;
		}
	}
	if(node->num_input_port_sizes == 1)
	{
		if(flagb == 1)
		{
			for (i = 0; i < current_sizeb; i++)
				ptr->input_pins[i+current_sizeb] = NULL;
		}
		else if(flaga == 2)
		{
			for (i = 0; i < bb; i++)
			{
				ptr->input_pins[i+current_sizea] = node->input_pins[i + index * sizeb];
				ptr->input_pins[i+current_sizea]->node = ptr;
			}
			for (i = 0; i < (sizeb - bb); i++)
				ptr->input_pins[i+current_sizea + bb] = NULL;
		}
		else
		{
			for (i = 0; i < current_sizeb; i++)
			{
				ptr->input_pins[i + current_sizea] = node->input_pins[i + index * sizeb];
				ptr->input_pins[i + current_sizea]->node = ptr;
			}
		}
	}
	else
	{
		if(flagb == 1)
		{
			for (i = 0; i < current_sizeb; i++)
				ptr->input_pins[i+current_sizeb] = NULL;
		}
		else if(flaga == 2)
		{
			for (i = 0; i < bb; i++)
			{
				ptr->input_pins[i+current_sizea] = node->input_pins[i + a + index * sizeb];
				ptr->input_pins[i+current_sizea]->node = ptr;
			}
			for (i = 0; i < (sizeb - bb); i++)
				ptr->input_pins[i+current_sizea + bb] = NULL;
		}
		else
		{
			for (i = 0; i < current_sizeb; i++)
			{
				ptr->input_pins[i + current_sizea] = node->input_pins[i + a + index * sizeb];
				ptr->input_pins[i + current_sizea]->node = ptr;
			}
		}
	}

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
	ptr->output_pins = (npin_t**)malloc(sizeof(void *) * output);
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
	int i,j;
	int num;

	/* Check for a legitimate split */
	if(nodeo->num_input_port_sizes == 3)
	{
		oassert(nodeo->input_port_sizes[0] == a);
		oassert(nodeo->input_port_sizes[1] == b);
	}
	else
	{
		oassert(nodeo->input_port_sizes[0] == a);
		oassert(nodeo->input_port_sizes[0] == b);
	}

	chain_information_t *adder_chain = allocate_chain_info();
	adder_chain->count = count;
	adder_chain->name = nodeo->name;
	sub_chain_list = insert_in_vptr_list(sub_chain_list, adder_chain);

	node  = (nnode_t**)malloc(sizeof(nnode_t*)*(count + 1));

	for(i = 0; i < count; i++)
	{
		node[i] = allocate_nnode();
		node[i]->name = (char *)malloc(strlen(nodeo->name) + 4);
		sprintf(node[i]->name, "%s-%d", nodeo->name, i);
		init_split_adder_for_sub(nodeo, node[i], a, sizea, b, sizeb, cin, cout, i);
		sub_list = insert_in_vptr_list(sub_list, node[i]);
	}

	//connect the first cin pin to unconn
	connect_nodes(netlist->pad_node, 0, node[0], (sizea + sizeb));

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

	//if(count * sizea == a)
	//{
		//remap the output pins of each adder to nodeo
	//	for(i = 0; i < count; i++)
	//	{
	//		for(j = 0; j < node[i]->num_output_pins - 1; j ++)
	//			remap_pin_to_new_node(nodeo->output_pins[i * sizea + j], node[i], j + 1);
	//	}
		// the last node's cout should be remapped to the most significant bits of nodeo
		//remap_pin_to_new_node(nodeo->output_pins[(nodeo->num_output_pins - 1)], node[(count - 1)], 0);
	//	node[count - 1]->output_pins[0] = allocate_npin();
	//	node[count - 1]->output_pins[0]->name = append_string("", "%s~dummy_output~%d~%d", node[(count - 1)]->name, (count - 1), (node[(count - 1)]->num_output_pins - 1));
	//}
	//else
	//{
		//remap the output pins of each adder to nodeo
		for(i = 0; i < count; i++)
		{
			for(j = 0; j < node[i]->num_output_pins - 1; j ++)
			{
				if((i * sizea + j) < nodeo->num_output_pins)
					remap_pin_to_new_node(nodeo->output_pins[i * sizea + j], node[i], j + 1);
				else
				{
					node[i]->output_pins[j + 1] = allocate_npin();
					// Pad outputs with a unique and descriptive name to avoid collisions.
				   node[i]->output_pins[j + 1]->name = append_string("", "%s~dummy_output~%d~%d", node[i]->name, i, j + 1);
				}
			}
		}
		node[count - 1]->output_pins[0] = allocate_npin();
		// Pad outputs with a unique and descriptive name to avoid collisions.
		node[count - 1]->output_pins[0]->name = append_string("", "%s~dummy_output~%d~%d", node[(count - 1)]->name, (count - 1), 0);
		//connect_nodes(node[count - 1], (node[(count - 1)]->num_output_pins - 1), netlist->gnd_node, 0);
	//}


	/* Probably more to do here in freeing the old node! */
	free(nodeo->name);
	free(nodeo->input_port_sizes);
	free(nodeo->output_port_sizes);

	/* Free arrays NOT the pins since relocated! */
	free(nodeo->input_pins);
	free(nodeo->output_pins);
	free(nodeo);
	return;
}


/*-------------------------------------------------------------------------
 * (function: pad_adder)
 *
 * Fill out a adder to a fixed size. Size is retrieved from global
 *	hard_subs data.
 * node, a, b, sizea, sizeb, 1, 1, count, netlist
 * NOTE: The inputs are extended based on adder padding setting.
 *-----------------------------------------------------------------------*/
void pad_adder_for_sub(nnode_t *nodeo, int a, int b, int sizea, int sizeb, int cin, int cout, int count, netlist_t *netlist)
{
		nnode_t **node;
		int i,j;
		int num, max, mark;
		nnode_t **new_add_cells;
		nnode_t **new_carry_cells;
		int aleft, bleft;

		/* Check for a legitimate split*/
		oassert(nodeo->input_port_sizes[0] == a);
		oassert(nodeo->input_port_sizes[1] == b);

		mark = 10;
		aleft = a - sizea * count;
		bleft = b - sizea * count;
		if(aleft > bleft)
			max = aleft;
		else
			max = bleft;

		new_add_cells  = (nnode_t**)malloc(sizeof(nnode_t*)*(max + cout));
		new_carry_cells = (nnode_t**)malloc(sizeof(nnode_t*)*(max + cout));
		node  = (nnode_t**)malloc(sizeof(nnode_t*)*(count + 1));

		//connect the first cin pin to ground
		if(count > 0)
		{
			for(i = 0; i < count; i++)
			{
				node[i] = allocate_nnode();
				node[i]->name = (char *)malloc(strlen(nodeo->name) + 4);
				sprintf(node[i]->name, "%s-%d", nodeo->name, i);
				init_split_adder_for_sub(nodeo, node[i], a, sizea, b, sizeb, cin, cout, i);
				sub_list = insert_in_vptr_list(sub_list, node[i]);
			}

			/* create the adder units and the zero unit*/
			for (i = 0; i < max; i++)
			{
				new_add_cells[i] = make_3port_gate(ADDER_FUNC, 1, 1, 1, 1, nodeo, mark);
				new_carry_cells[i] = make_3port_gate(CARRY_FUNC, 1, 1, 1, 1, nodeo, mark);
			}

			connect_nodes(netlist->gnd_node, 0, node[0], (sizea + sizeb));

			//if any input pins beside first cin pins are NULL, connect those pins to ground
			for(i = 0; i < count; i++)
			{
				num = node[i]->num_input_pins;
				for(j = 0; j < num - 1; j++)
				{
					if(node[i]->input_pins[j] == NULL)
						connect_nodes(netlist->gnd_node, 0, node[i], j);
				}
			}

			//connect cout to next node's cin
			if(sizea > sizeb)
			{
				for(i = 1; i < count; i++)
					connect_nodes(node[i-1], sizea, node[i], (node[i]->num_input_pins - 1));
			}
			else
			{
				for(i = 1; i < count; i++)
					connect_nodes(node[i-1], sizeb, node[i], (node[i]->num_input_pins - 1));
			}

			//connect adders cout to add_cells first cin
			add_input_pin_to_node(new_add_cells[0], node[count - 1]->output_pins[(node[(count - 1)]->num_output_pins - 1)], 0);
			//connect adders cout to carry_cells first cin
			add_input_pin_to_node(new_carry_cells[0], node[count - 1]->output_pins[(node[(count - 1)]->num_output_pins - 1)], 0);

			//remap the input pins of add_cells and carry_cells
			for(i = 0; i < max; i++)
			{
				if(i < aleft && aleft > 0)
				{
					remap_pin_to_new_node(nodeo->input_pins[i + sizea * count], new_add_cells[i], 1);
					if (i < max - 1)
						add_input_pin_to_node(new_carry_cells[i], copy_input_npin(new_add_cells[i]->input_pins[1]), 1);
				}
				else
				{
					add_input_pin_to_node(new_add_cells[i], get_zero_pin(netlist), 1);
					if (i < max - 1)
						add_input_pin_to_node(new_carry_cells[i], get_zero_pin(netlist), 1);
				}

				if(i < bleft && bleft > 0)
				{
					remap_pin_to_new_node(nodeo->input_pins[i + a + sizea * count], new_add_cells[i], 2);
					if (i < max - 1)
						add_input_pin_to_node(new_carry_cells[i], copy_input_npin(new_add_cells[i]->input_pins[2]), 2);
				}
				else
				{
					add_input_pin_to_node(new_add_cells[i], get_zero_pin(netlist), 2);
					if (i < max - 1)
						add_input_pin_to_node(new_carry_cells[i], get_zero_pin(netlist), 2);
				}

				/* join that gate to the output*/
				remap_pin_to_new_node(nodeo->output_pins[i + sizea * count], new_add_cells[i], 0);
			}


			//remap the output pins of each adder to nodeo
			for(i = 0; i < count; i++)
			{
				for(j = 0; j < node[i]->num_output_pins - 1; j ++)
					remap_pin_to_new_node(nodeo->output_pins[i * sizea + j], node[i], j);
			}

			/* connect carry outs with carry ins*/
			for(i = 1; i < max; i++)
			{
				connect_nodes(new_carry_cells[i-1], 0, new_add_cells[i], 0);
				if (i < max - 1)
					connect_nodes(new_carry_cells[i-1], 0, new_carry_cells[i], 0);
			}
		}
		else
		{
			/* create the adder units and the zero unit*/
			for (i = 0; i < max; i++)
			{
				//new_add_cells[i] = make_3port_gate(ADDER_FUNC, 1, 1, 1, 1, node, mark);
				// The last carry cell will be connected to an output pin, if one is available
				//new_carry_cells[i] = make_3port_gate(CARRY_FUNC, 1, 1, 1, 1, node, mark);
			}

			/* ground first carry in*/
			add_input_pin_to_node(new_add_cells[0], get_zero_pin(netlist), 0);
			if (i > 1)
			{
				add_input_pin_to_node(new_carry_cells[0], get_zero_pin(netlist), 0);
			}

			/* connect inputs*/
			for(i = 0; i < max; i++)
			{
				if (i < aleft)
				{
					/* join the A port up to adder*/
					remap_pin_to_new_node(nodeo->input_pins[i], new_add_cells[i], 1);
					if (i < max - 1)
						add_input_pin_to_node(new_carry_cells[i], copy_input_npin(new_add_cells[i]->input_pins[1]), 1);
				}
				else
				{
					add_input_pin_to_node(new_add_cells[i], get_zero_pin(netlist), 1);
					if (i < max - 1)
						add_input_pin_to_node(new_carry_cells[i], get_zero_pin(netlist), 1);
				}

				if (i < bleft)
				{
					/* join the B port up to adder*/
					remap_pin_to_new_node(nodeo->input_pins[i+a], new_add_cells[i], 2);
					if (i < max - 1)
						add_input_pin_to_node(new_carry_cells[i], copy_input_npin(new_add_cells[i]->input_pins[2]), 2);
				}
				else
				{
					add_input_pin_to_node(new_add_cells[i], get_zero_pin(netlist), 2);
					if (i < max - 1)
						add_input_pin_to_node(new_carry_cells[i], get_zero_pin(netlist), 2);
				}

				/* join that gate to the output*/
				remap_pin_to_new_node(nodeo->output_pins[i], new_add_cells[i], 0);
			}

			/* connect carry outs with carry ins*/
			for(i = 1; i < max; i++)
			{
				connect_nodes(new_carry_cells[i-1], 0, new_add_cells[i], 0);
				if (i < max - 1)
					connect_nodes(new_carry_cells[i-1], 0, new_carry_cells[i], 0);
			}
		}

		/* Probably more to do here in freeing the old node!*/
		free(nodeo->name);
		free(nodeo->input_port_sizes);
		free(nodeo->output_port_sizes);

		/* Free arrays NOT the pins since relocated!*/
		free(new_add_cells);
		free(new_carry_cells);
		free(nodeo->input_pins);
		free(nodeo->output_pins);
		free(nodeo);
		return;

}


/*-------------------------------------------------------------------------
 * (function: iterate_adders)
 *
 * This function will iterate over all of the add operations that
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
	nnode_t *node;

	/* Can only perform the optimisation if hard adders exist! */
	if (hard_subs == NULL)
		return;
	else
	{
		//In hard block adder, the summand and addend are same size.
		sizecin = hard_subs->inputs->size;
		sizeb = hard_subs->inputs->next->size;
		sizea = hard_subs->inputs->next->size;

		oassert(sizecin == 1);

		while(sub_list != NULL)
		{
			node = (nnode_t *)sub_list->data_vptr;
			sub_list = delete_in_vptr_list(sub_list);
			subs_list = insert_in_vptr_list(subs_list, node);
		}

		while (subs_list != NULL)
		{
			node = (nnode_t *)subs_list->data_vptr;
			subs_list = delete_in_vptr_list(subs_list);

			oassert(node != NULL);
			oassert(node->type == MINUS);

			subchaintotal++;

			a = node->input_port_sizes[0];
			if(node->num_input_port_sizes == 2)
				b = node->input_port_sizes[1];
			else
				b = node->input_port_sizes[0];

			//fixed_hard_adder = 0 then the use hard block for extra bits
			if(configuration.fixed_hard_adder == 0){
				// how many subtractors base on a can split
				if(a % sizea == 0)
					counta = a / sizea;
				else
					counta = a / sizea + 1;
				// how many subtractors base on b can split
				if(b % sizeb == 0)
					countb = b / sizeb;
				else
					countb = b / sizeb + 1;
				// how many subtractors need to be split
				if(counta >= countb)
					count = counta;
				else
					count = countb;

				split_adder_for_sub(node, a, b, sizea, sizeb, 1, 1, count, netlist);
			}
			else
			{
				counta = a / sizea;
				countb = b / sizeb;
				if(counta > countb)
					count = countb;
				else
					count = counta;

				//pad_adder_for_sub(node, a, b, sizea, sizeb, 1, 1, count, netlist);
			}
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
	return;
}

#endif
