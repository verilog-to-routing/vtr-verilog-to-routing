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
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "node_creation_library.h"
#include "adders.h"
#include "netlist_utils.h"
#include "partial_map.h"
#include "read_xml_arch_file.h"
#include "globals.h"
#include "errors.h"
#include "subtractions.h"
#include "print_tags.h"

#include "vtr_list.h"

using vtr::t_linked_vptr;

t_model *hard_adders = NULL;
t_linked_vptr *add_list = NULL;
t_linked_vptr *processed_adder_list = NULL;
t_linked_vptr *chain_list = NULL;
int total = 0;
int *adder = NULL;
int min_add = 0;
int min_threshold_adder = 0;

netlist_t *the_netlist;

#ifdef VPR6

void record_add_distribution(nnode_t *node);
void init_split_adder(nnode_t *node, nnode_t *ptr, int a, int sizea, int b, int sizeb, int cin, int cout, int index, int flag, netlist_t *netlist);

/*---------------------------------------------------------------------------
 * (function: init_add_distribution)
 *  For adder, the output will only be the maxim input size + 1
 *-------------------------------------------------------------------------*/
void init_add_distribution()
{
	int i, j;
	oassert(hard_adders != NULL);
	j = hard_adders->inputs->size + hard_adders->inputs->next->size;
	adder = (int *)calloc((j + 1),sizeof(int));
	for (i = 0; i <= j; i++)
		adder[i] = 0;
	return;
}

/*---------------------------------------------------------------------------
 * (function: record_add_distribution)
 *-------------------------------------------------------------------------*/
void record_add_distribution(nnode_t *node)
{
	oassert(hard_adders != NULL);
	oassert(node != NULL);
	adder[hard_adders->inputs->size] += 1;
	return;
}

/*---------------------------------------------------------------------------
 * (function: report_add_distribution)
 *-------------------------------------------------------------------------*/

/* These values are collected during the unused logic removal sweep */
extern int adder_chain_count;
extern int longest_adder_chain;
extern int total_adders;

extern double geomean_addsub_length;
extern double sum_of_addsub_logs;
extern int total_addsub_chain_count;

void report_add_distribution()
{
	if(hard_adders == NULL)
		return;

	printf("\nHard adder Distribution\n");
	printf("============================\n");
	printf("\n");
	printf("\nTotal # of chains = %d\n", adder_chain_count);

	printf("\nHard adder chain Details\n");
	printf("============================\n");

	printf("\n");
	printf("\nThe Number of Hard Block adders in the Longest Chain: %d\n", longest_adder_chain);

	printf("\n");
	printf("\nThe Total Number of Hard Block adders: %d\n", total_adders);

	printf("\n");
	printf("\nGeometric mean adder/subtractor chain length: %.2f\n", geomean_addsub_length);

	return;
}

/*---------------------------------------------------------------------------
 * (function: find_hard_adders)
 *-------------------------------------------------------------------------*/
void find_hard_adders()
{
	hard_adders = Arch.models;
	//Disable the size in configuration file.(The threshold for the extra bits).
	//min_add = configuration.min_hard_adder;
	min_threshold_adder = configuration.min_threshold_adder;

	while (hard_adders != NULL)
	{
		if (strcmp(hard_adders->name, "adder") == 0)
		{
			init_add_distribution();
			return;
		}
		else
		{
			hard_adders = hard_adders->next;
		}
	}

	return;
}

/*---------------------------------------------------------------------------
 * (function: declare_hard_adder)
 *-------------------------------------------------------------------------*/
void declare_hard_adder(nnode_t *node)
{
	t_adder *tmp;
	int width_a, width_b, width_sumout;

	/* See if this size instance of adder exists? */
	if (!hard_adders){
		printf(ERRTAG "Instantiating adder where adders do not exist\n");
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

	/* Does not exist - must create an instance */
	tmp = (t_adder *)calloc(1,sizeof(t_adder));
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
 * (function: instantiate_hard_addier )
 *-------------------------------------------------------------------------*/
void instantiate_hard_adder(nnode_t *node, short mark, netlist_t * /*netlist*/)
{
	int i;
	declare_hard_adder(node);

	/* Give names to the output pins */
	for (i = 0; i < node->num_output_pins;  i++)
	{
		if (node->output_pins[i]->name){
			free_me(node->output_pins[i]->name);
		}
		size_t length = snprintf(NULL,0,"%s[%d]", node->name, node->output_pins[i]->pin_node_idx);
		node->output_pins[i]->name = (char*)calloc(length +1,sizeof(char));
		sprintf(node->output_pins[i]->name, "%s[%d]", node->name, node->output_pins[i]->pin_node_idx);
	}

	node->traverse_visited = mark;
	return;
}

/*----------------------------------------------------------------------------
 * function: add_the_blackbox_for_adds()
 *--------------------------------------------------------------------------*/
void add_the_blackbox_for_adds(FILE *out)
{
	int i;
	int count;
	int hard_add_inputs, hard_add_outputs;
	t_adder *adds;
	t_model_ports *ports;
	char buffer[MAX_BUF];
	char *pa, *pb, *psumout, *pcin, *pcout;

	/* Check to make sure this target architecture has hard adders */
	if (hard_adders == NULL)
		return;

	/* Get the names of the ports for the adder */
	ports = hard_adders->inputs;
	pcin = ports->name;
	ports = ports->next;
	pb = ports->name;
	ports = ports->next;
	pa = ports->name;

	ports = hard_adders->outputs;
	psumout = ports->name;
	ports = ports->next;
	pcout = ports->name;

	/* find the adder devices in the tech library */
	adds = (t_adder *)(hard_adders->instances);
	if (adds == NULL) /* No adders instantiated */
		return;

	/* simplified way of getting the multsize, but fine for quick example */
	while (adds != NULL)
	{
		/* write out this adder model */
		if (configuration.fixed_hard_adder != 0)
			count = fprintf(out, ".model adder\n");
		else
			count = fprintf(out, ".model adder\n");

		/* add the inputs */
		count = count + fprintf(out, ".inputs");
		hard_add_inputs = adds->size_a + adds->size_b + adds->size_cin;
		for (i = 0; i < hard_add_inputs; i++)
		{
			if (i < adds->size_a)
			{
				count = count + sprintf(buffer, " %s[%d]", pa, i);
			}
			else if(i < hard_add_inputs - adds->size_cin && i >= adds->size_a)
			{
				count = count + sprintf(buffer, " %s[%d]", pb, i - adds->size_a);
			}
			else
			{
				count = count + sprintf(buffer, " %s[%d]", pcin, i - adds->size_a - adds->size_b);
			}
			if (count > 78)
				count = fprintf(out, " \\\n %s", buffer) - 3;
			else
				fprintf(out, " %s", buffer);
		}
		fprintf(out, "\n");

		/* add the outputs */
		count = fprintf(out, ".outputs");
		hard_add_outputs = adds->size_cout + adds->size_sumout;
		for (i = 0; i < hard_add_outputs; i++)
		{
			if (i < adds->size_cout)
			{
				count = count + sprintf(buffer, " %s[%d]", pcout, i);
			}
			else
			{
				count = count + sprintf(buffer, " %s[%d]", psumout, i - adds->size_cout);
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

		adds = adds->next;
	}
}

/*-------------------------------------------------------------------------
 * (function: define_add_function)
 *-----------------------------------------------------------------------*/
void define_add_function(nnode_t *node, short /*type*/, FILE *out)
{
	int i, j;
	int count;
	char buffer[MAX_BUF];

	count = fprintf(out, "\n.subckt");
	count--;
	oassert(node->input_port_sizes[0] > 0);
	oassert(node->input_port_sizes[1] > 0);
	oassert(node->input_port_sizes[2] > 0);
	oassert(node->output_port_sizes[0] > 0);
	oassert(node->output_port_sizes[1] > 0);

	/* Write out the model adder  */
	if (configuration.fixed_hard_adder != 0)
	{
		count += fprintf(out, " adder");
	}
	else
	{
		count += fprintf(out, " adder");
	}

	/* Write the input pins*/
	for (i = 0;  i < node->num_input_pins; i++)
	{
			npin_t *driver_pin = node->input_pins[i]->net->driver_pin;

			if (i < node->input_port_sizes[0])
			{
				if (!driver_pin->name)
					j = sprintf(buffer, " %s[%d]=%s", hard_adders->inputs->next->next->name, i, driver_pin->node->name);
				else
					j = sprintf(buffer, " %s[%d]=%s", hard_adders->inputs->next->next->name, i, driver_pin->name);
			}
			else if(i >= node->input_port_sizes[0] && i < node->input_port_sizes[1] + node->input_port_sizes[0])
			{
				if (!driver_pin->name)
					j = sprintf(buffer, " %s[%d]=%s", hard_adders->inputs->next->name, i - node->input_port_sizes[0], driver_pin->node->name);
				else
					j = sprintf(buffer, " %s[%d]=%s", hard_adders->inputs->next->name, i - node->input_port_sizes[0], driver_pin->name);
			}
			else
			{
				if (!driver_pin->name)
					j = sprintf(buffer, " %s[%d]=%s", hard_adders->inputs->name, i - (node->input_port_sizes[0] + node->input_port_sizes[1]), driver_pin->node->name);
				else
					j = sprintf(buffer, " %s[%d]=%s", hard_adders->inputs->name, i - (node->input_port_sizes[0] + node->input_port_sizes[1]), driver_pin->name);
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
			j = sprintf(buffer, " %s[%d]=%s", hard_adders->outputs->next->name, i , node->output_pins[i]->name);
		else
			j = sprintf(buffer, " %s[%d]=%s", hard_adders->outputs->name, i - node->output_port_sizes[0], node->output_pins[i]->name);
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
 *	flag = 0: all adders are hard logic block; flag = 1: the last adder in the chain is soft logic block
 *---------------------------------------------------------------------*/
void init_split_adder(nnode_t *node, nnode_t *ptr, int a, int sizea, int b, int sizeb, int cin, int cout, int index, int flag, netlist_t *netlist)
{
	int i;
	int flaga = 0, flagb = 0;
	int current_sizea, current_sizeb;
	int aa = 0, bb = 0, num = 0;

	/* Copy properties from original node */
	ptr->type = node->type;
	ptr->bit_width = node->bit_width;
	ptr->related_ast_node = node->related_ast_node;
	ptr->traverse_visited = node->traverse_visited;
	ptr->node_data = NULL;

	/* decide the current size of input a and b */
	if(flag == 0)
	{
		current_sizea = (a + 1) - sizea * index;
		current_sizeb = (b + 1) - sizeb * index;
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
	ptr->input_port_sizes = (int *)calloc(3,sizeof(int));
	ptr->input_port_sizes[0] = current_sizea;
	ptr->input_port_sizes[1] = current_sizeb;
	ptr->input_port_sizes[2] = cin;
	ptr->num_output_port_sizes = 2;
	ptr->output_port_sizes = (int *)calloc(2,sizeof(int));
	ptr->output_port_sizes[0] = cout;

	/* The size of output port sumout equals the maxim size of sizea and sizeb  */
	if(current_sizea > current_sizeb)
		ptr->output_port_sizes[1] = current_sizea;
	else
		ptr->output_port_sizes[1] = current_sizeb;

	/* Set the number of pins and re-locate previous pin entries */
	ptr->num_input_pins = current_sizea + current_sizeb + cin;
	ptr->input_pins = (npin_t**)calloc((current_sizea + current_sizeb + cin),sizeof(npin_t *));
	//if flaga or flagb = 1, the input pins should be empty.
	if(flaga == 1)
	{
		for (i = 0; i < current_sizea; i++)
			ptr->input_pins[i] = NULL;
	}
	else if(flaga == 2)
	{
		if(index == 0)
		{
			ptr->input_pins[0] = NULL;
			if(sizea > 1){
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
		if(index == 0)
		{
			if(flag == 0)
			{
				ptr->input_pins[0] = NULL;
				if(current_sizea > 1)
				{
					for (i = 1; i < current_sizea; i++)
					{
						ptr->input_pins[i] = node->input_pins[i - 1];
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
				for (i = 0; i < current_sizea; i++)
				{
					ptr->input_pins[i] = node->input_pins[i + index * sizea - 1];
					ptr->input_pins[i]->node = ptr;
					ptr->input_pins[i]->pin_node_idx = i;
				}
			}
			else
			{
				if(sizea == 0)
					connect_nodes(netlist->gnd_node, 0, ptr, 0);
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

	}

	if(flagb == 1)
	{
		for (i = 0; i < current_sizeb; i++)
			ptr->input_pins[i + current_sizeb] = NULL;
	}
	else if(flagb == 2)
	{
		if(index == 0)
		{
			ptr->input_pins[sizea] = NULL;
			if(current_sizeb > 1)
			{
				for (i = 1; i < bb; i++)
				{
					ptr->input_pins[i + current_sizea] = node->input_pins[i + a + index * sizeb - 1];
					ptr->input_pins[i + current_sizea]->node = ptr;
					ptr->input_pins[i + current_sizea]->pin_node_idx = i + current_sizea;
				}
				for (i = 0; i < (sizeb - bb); i++)
					ptr->input_pins[i + current_sizea + bb] = NULL;
			}
		}
		else
		{
			for (i = 0; i < bb; i++)
			{
				ptr->input_pins[i + current_sizea] = node->input_pins[i + a + index * sizeb - 1];
				ptr->input_pins[i + current_sizea]->node = ptr;
				ptr->input_pins[i + current_sizea]->pin_node_idx = i + current_sizea;
			}
			for (i = 0; i < (sizeb - bb); i++)
				ptr->input_pins[i + current_sizea + bb] = NULL;
		}
	}
	else
	{
		if(index == 0)
		{
			if(flag == 0)
			{
				ptr->input_pins[sizea] = NULL;
				if(current_sizeb > 1)
				{
					for (i = 1; i < current_sizeb; i++)
					{
						ptr->input_pins[i + current_sizea] = node->input_pins[i + a + index * sizeb - 1];
						ptr->input_pins[i + current_sizea]->node = ptr;
						ptr->input_pins[i + current_sizea]->pin_node_idx = i + current_sizea;
					}
				}
			}
			else
			{
				for (i = 0; i < current_sizeb; i++)
				{
					ptr->input_pins[i + current_sizea] = node->input_pins[i + a];
					ptr->input_pins[i + current_sizea]->node = ptr;
					ptr->input_pins[i + current_sizea]->pin_node_idx = i + current_sizea;
				}
			}
		}
		else
		{
			if(flag == 0)
			{
				for (i = 0; i < current_sizeb; i++)
				{
					ptr->input_pins[i + current_sizea] = node->input_pins[i + a + index * sizeb - 1];
					ptr->input_pins[i + current_sizea]->node = ptr;
					ptr->input_pins[i + current_sizea]->pin_node_idx = i + current_sizea;
				}
			}
			else
			{
				if(sizeb == 0)
					connect_nodes(netlist->gnd_node, 0, ptr, current_sizea);
				else
				{
					num = node->input_port_sizes[0] +  node->input_port_sizes[1];
					for (i = 0; i < current_sizeb; i++)
					{
						ptr->input_pins[i + current_sizea] = node->input_pins[i + num - current_sizeb];
						ptr->input_pins[i + current_sizea]->node = ptr;
						ptr->input_pins[i + current_sizea]->pin_node_idx = i + current_sizea;
					}
				}
			}
		}
	}

	/* Carry_in should be NULL*/
	for (i = 0; i < cin; i++)
	{
		ptr->input_pins[i + current_sizea + current_sizeb] = NULL;
	}

	/* output pins */
	int output;
	if(current_sizea > current_sizeb)
		output = current_sizea + cout;
	else
		output = current_sizeb + cout;

	ptr->num_output_pins = output;
	ptr->output_pins = (npin_t**)calloc(output,sizeof(npin_t *));
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
 * Note: In this function, we can do padding(default -1), fix the size of hard block adder.
 *-----------------------------------------------------------------------*/

void split_adder(nnode_t *nodeo, int a, int b, int sizea, int sizeb, int cin, int cout, int count, netlist_t *netlist)
{
	nnode_t **node;
	int i,j;
	int num, lefta = 0, leftb = 0;
	int max_num = 0;
	int flag = 0;

	/* Check for a legitimate split */
	oassert(nodeo->input_port_sizes[0] == a);
	oassert(nodeo->input_port_sizes[1] == b);

	node  = (nnode_t**)calloc((count),sizeof(nnode_t*));

	for(i = 0; i < count; i++)
	{
		node[i] = (nnode_t *)my_malloc_struct(sizeof(nnode_t));
		allocate_nnode(node[i]);
		node[i]->name = (char *)calloc(strlen(nodeo->name) + 20,sizeof(char));
		sprintf(node[i]->name, "%s-%d", nodeo->name, i);
		if(i == count - 1)
		{
			//fixed_hard_adder = 1 then adder need to be exact size;
			if(configuration.fixed_hard_adder == 1)
				init_split_adder(nodeo, node[i], a, sizea, b, sizeb, cin, cout, i, flag, netlist);
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
				if(max_num >= min_add)
					init_split_adder(nodeo, node[i], a, sizea, b, sizeb, cin, cout, i, flag, netlist);
				else
				{
					// Using soft logic to do the addition, No need to pad as the same size
					flag = 1;
					init_split_adder(nodeo, node[i], a, lefta, b, leftb, cin, cout, i, flag, netlist);
				}
			}
		}
		else
			init_split_adder(nodeo, node[i], a, sizea, b, sizeb, cin, cout, i, flag, netlist);
			
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
	chain_list = insert_in_vptr_list(chain_list, adder_chain);

	if(flag == 0 || count > 1)
	{
		//connect the a[0] and b[0] of first adder node to ground
		connect_nodes(netlist->gnd_node, 0, node[0], 0);
		connect_nodes(netlist->gnd_node, 0, node[0], sizea);
		//hang the first sumout
		
		node[0]->output_pins[1] = (npin_t *)my_malloc_struct(sizeof(npin_t));
		allocate_npin(node[0]->output_pins[1]);
		node[0]->output_pins[1]->name = append_string("", "%s~dummy_output~%d~%d", node[0]->name, 0, 1);
	}

	if(nodeo->num_input_port_sizes == 2)
	{
		//connect the first cin pin to unconn
		connect_nodes(netlist->pad_node, 0, node[0], node[0]->num_input_pins - 1);
	}
	else if(nodeo->num_input_port_sizes == 3)
	{
		//remap the first cin pins)
		remap_pin_to_new_node(nodeo->input_pins[nodeo->num_input_pins - 1], node[0], (node[0]->num_input_pins - 1));
	}
	//if (a + 1) % sizea == 0, the a[0] and b[0] of node[count-1] should connect to gound
	if((a + 1) % sizea == 0 && (b + 1) % sizeb == 0)
	{
		if(flag == 0)
		{
			connect_nodes(netlist->gnd_node, 0, node[count-1], 0);
			connect_nodes(netlist->gnd_node, 0, node[count-1], sizea);
		}
	}

	//if any input pins beside first cin pins are NULL, connect those pins to unconn
	for(i = 0; i < count; i++)
	{
		num = node[i]->num_input_pins;
		for(j = 0; j < num - 1; j++)
		{
			if(node[i]->input_pins[j] == NULL)
				connect_nodes(netlist->pad_node, 0, node[i],j);
		}
	}

	//connect cout to next cin
	for(i = 1; i < count; i++)
		connect_nodes(node[i-1], 0, node[i], (node[i]->num_input_pins - 1));

	//remap the output pins of each adder to nodeo
	if(count == 1)
	{
		if(flag == 0)
		{
			for(j = 0; j < node[0]->num_output_pins - 2; j++)
			{
				if(j < nodeo->num_output_pins)
					remap_pin_to_new_node(nodeo->output_pins[j], node[0], j + 2);
				else
				{
					node[0]->output_pins[j + 2] = (npin_t *)my_malloc_struct(sizeof(npin_t));
					allocate_npin(node[0]->output_pins[j + 2]);
					node[0]->output_pins[j + 2]->name = append_string("", "%s~dummy_output~%d~%d", node[0]->name, 0, j + 2);
				}
				//hang the first cout
				node[0]->output_pins[0] = (npin_t *)my_malloc_struct(sizeof(npin_t));
				allocate_npin(node[0]->output_pins[0]);
				node[0]->output_pins[0]->name = append_string("", "%s~dummy_output~%d~%d", node[0]->name, 0, 0);
			}
		}
		else
		{
			for(j = 0; j < node[0]->num_output_pins - 1; j ++)
				remap_pin_to_new_node(nodeo->output_pins[j], node[0], j + 1);
			remap_pin_to_new_node(nodeo->output_pins[nodeo->num_output_pins - 1], node[0], 0);
		}
	}
	else
	{
		//First adder
		for(j = 0; j < node[0]->num_output_pins - 2; j++)
			remap_pin_to_new_node(nodeo->output_pins[j], node[0], j + 2);
		for(i = 1; i < count - 1; i ++)
		{
			for(j = 0; j < node[i]->num_output_pins - 1; j ++)
				remap_pin_to_new_node(nodeo->output_pins[i * sizea + j - 1], node[i], j + 1);
		}
		//Last adder
		if(flag == 0)
		{
			for(j = 0; j < node[count-1]->num_output_pins - 1; j ++)
			{
				if(((count - 1) * sizea + j - 1) < nodeo->num_output_pins)
					remap_pin_to_new_node(nodeo->output_pins[(count - 1) * sizea + j - 1], node[count - 1], j + 1);
				else
				{
					node[count - 1]->output_pins[j + 1] = (npin_t *)my_malloc_struct(sizeof(npin_t));
					allocate_npin(node[count - 1]->output_pins[j + 1]);
					// Pad outputs with a unique and descriptive name to avoid collisions.
					node[count - 1]->output_pins[j + 1]->name = append_string("", "%s~dummy_output~%d~%d", node[count - 1]->name, count - 1, j + 1);
				}
			}
			//Hang the last cout
			node[count - 1]->output_pins[0] = (npin_t *)my_malloc_struct(sizeof(npin_t));
			allocate_npin(node[count - 1]->output_pins[0]);
			// Pad outputs with a unique and descriptive name to avoid collisions.
			node[count - 1]->output_pins[0]->name = append_string("", "%s~dummy_output~%d~%d", node[count - 1]->name, count - 1, 0);
		}
		else
		{
			for(j = 0; j < node[count - 1]->num_output_pins - 1; j ++)
				//if(((count - 1) * sizea + j - 1) < nodeo->num_output_pins)
					remap_pin_to_new_node(nodeo->output_pins[(count - 1) * sizea + j - 1], node[count - 1], j + 1);
			if(nodeo->output_pins[nodeo->num_output_pins - 1] != NULL)
				remap_pin_to_new_node(nodeo->output_pins[nodeo->num_output_pins - 1], node[count - 1], 0);
			else
			{
				node[count - 1]->output_pins[0] = (npin_t *)my_malloc_struct(sizeof(npin_t));
				allocate_npin(node[count - 1]->output_pins[0]);
				// Pad outputs with a unique and descriptive name to avoid collisions.
				node[count - 1]->output_pins[0]->name = append_string("", "%s~dummy_output~%d~%d", node[count - 1]->name, count - 1, 0);
			}
		}
	}

	/* Probably more to do here in freeing the old node! */
	free_me(nodeo->name);
	free_me(nodeo->input_port_sizes);
	free_me(nodeo->output_port_sizes);

	/* Free arrays NOT the pins since relocated! */
	free_me(nodeo->input_pins);
	free_me(nodeo->output_pins);
	free_me(nodeo);
	free_me(node);
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
void iterate_adders(netlist_t *netlist)
{
	int sizea, sizeb, sizecin;//the size of
	int a, b;
	int count,counta,countb;
	int num = 0;
	nnode_t *node;

	/* Can only perform the optimisation if hard adders exist! */
	if (hard_adders == NULL)
		return;
	//In hard block adder, the summand and addend are same size.
	sizecin = hard_adders->inputs->size;
	sizeb = hard_adders->inputs->next->size;
	sizea = hard_adders->inputs->next->size;

	oassert(sizecin == 1);

	while (add_list != NULL)
	{
		node = (nnode_t *)add_list->data_vptr;
		add_list = delete_in_vptr_list(add_list);

		oassert(node != NULL);
		if (node->type == HARD_IP)
			node->type = ADD;

		oassert(node->type == ADD);

		a = node->input_port_sizes[0];
		b = node->input_port_sizes[1];
		num = (a >= b)? a : b;
		node->bit_width = num;
		if(num >= min_threshold_adder && num >= min_add)
		{
			// how many adders a can split
			counta = (a + 1) / sizea + 1;
			// how many adders b can split
			countb = (b + 1) / sizeb + 1;
			// how many adders need to be split
			if(counta >= countb)
				count = counta;
			else
				count = countb;
			total++;
			split_adder(node, a, b, sizea, sizeb, 1, 1, count, netlist);
		}
		// Store the node into processed_adder_list if the threshold is bigger than num
		else
			processed_adder_list = insert_in_vptr_list(processed_adder_list, node);
	}
	return;
}

/*-------------------------------------------------------------------------
 * (function: clean_adders)
 *
 * Clean up the memory by deleting the list structure of adders
 *	during optimization
 *-----------------------------------------------------------------------*/
void clean_adders()
{
	while (add_list != NULL)
		add_list = delete_in_vptr_list(add_list);
	return;
}

#endif


/*-------------------------------------------------------------------------
 * (function: reduce_operations)
 *
 * reduce the operations that are redundant
 *-----------------------------------------------------------------------*/
void reduce_operations(netlist_t * /*netlist*/, operation_list op)
{
	t_linked_vptr *place = NULL;
	operation_list oper;
	switch (op)
	{
		case ADD:
			place = add_list;
			oper = ADD;
		break;

		case MULTIPLY:
			place = mult_list;
			oper = MULTIPLY;
		break;

		case MINUS:
			place = sub_list;
			oper = MINUS;
		break;

		default:
			oper = NO_OP;
		break;

	}

	traverse_list(oper, place);

}

/*-------------------------------------------------------------------------
 * (function: traverse_list)
 *
 * traverse the operation lists
 *-----------------------------------------------------------------------*/
void traverse_list(operation_list oper, t_linked_vptr *place)
{
		while (place != NULL && place->next != NULL)
		{
			match_node(place, oper);
			place = place->next;
		}
}

/*---------------------------------------------------------------------------
 * (function: match_node)
 *-------------------------------------------------------------------------*/
void match_node(t_linked_vptr *place, operation_list oper)
{
	int flag = 0;
    int mark = 0;
	nnode_t *node = NULL;
	nnode_t *next_node = NULL;
	node = (nnode_t*)place->data_vptr;
	t_linked_vptr *pre = place;
	t_linked_vptr *next = NULL;
	if (place->next != NULL)
		next = place->next;
	while (next != NULL)
	{
		flag = 0;
		mark = 0;
		next_node = (nnode_t*)next->data_vptr;
		if (node->type == next_node->type)
		{
			if (node->num_input_pins == next_node->num_input_pins)
			{
				flag = match_ports(node, next_node, oper);
				if (flag == 1)
				{
					mark = match_pins(node, next_node);
					if (mark == 1)
					{
						merge_nodes(node, next_node);
						remove_list_node(pre, next);
					}
				}
			}
		}
		if (mark == 1)
			next = pre->next;
		else
		{
			pre = next;
			next= next->next;
		}
	}

}

/*---------------------------------------------------------------------------
 * (function: match_ports)
 *-------------------------------------------------------------------------*/
int match_ports(nnode_t *node, nnode_t *next_node, operation_list oper)
{
	int flag = 0;
	int sign = 0;
	int *mark = &sign;
	int mark1 = 1;
	int mark2 = 1;
	ast_node_t *ast_node, *ast_node_next;
	char *component_s[2] = {0};
	char *component_o[2] = {0};
	ast_node = node->related_ast_node;
	ast_node_next = next_node->related_ast_node;
	if (ast_node->types.operation.op == oper)
	{
		traverse_operation_node(ast_node, component_s, oper, mark);
		if (sign != 1)
		{
			traverse_operation_node(ast_node_next, component_o, oper, mark);
			if (sign != 1)
			{
				switch (oper)
				{
					case ADD:
					case MULTIPLY:
					{
						mark1 = strcmp(component_s[0], component_o[0]);
						if (mark1 == 0)
							mark2 = strcmp(component_s[1], component_o[1]);
						else
						{
                            assert(component_s[0] && component_o[1] && component_s[1] && component_o[0]);
							mark1 = strcmp(component_s[0], component_o[1]);
							mark2 = strcmp(component_s[1], component_o[0]);
						}
						if (mark1 == 0 && mark2 == 0)
							flag = 1;
					}
					break;

					case MINUS:
						mark1 = strcmp(component_s[0], component_o[0]);
						if (mark1 == 0)
							mark2 = strcmp(component_s[1], component_s[1]);
						if (mark2 == 0)
							flag = 1;
					break;

					default:

					break;
				}
			}
		}
	}

	return flag;
}

/*-------------------------------------------------------------------------
 * (function: traverse_operation_node)
 *
 * search the ast find the couple of components
 *-----------------------------------------------------------------------*/
void traverse_operation_node(ast_node_t *node, char *component[], operation_list op, int *mark)
{
	int i;

	if (node == NULL)
		return;

	if (node->types.operation.op == op)
	{
		for (i = 0; i < node->num_children; i++)
		{
			*mark = 0;
			if (node->children[i]->type != IDENTIFIERS && node->children[i]->type != NUMBERS)
			{
				*mark = 1;
				break;
			}
			else
			{
				if (node->children[i]->type == IDENTIFIERS)
					component[i] = node->children[i]->types.identifier;
				else
					if (node->children[i]->type == NUMBERS)
						component[i] = node->children[i]->types.number.number;
			}

		}
	}

}

/*---------------------------------------------------------------------------
 * (function: merge_node)
 *-------------------------------------------------------------------------*/
void merge_nodes(nnode_t *node, nnode_t *next_node)
{

	remove_fanout_pins(next_node);
	reallocate_pins(node, next_node);
	free_op_nodes(next_node);

}

/*---------------------------------------------------------------------------
 * (function: remove_list_node)
 *-------------------------------------------------------------------------*/
void remove_list_node(t_linked_vptr *pre, t_linked_vptr *next)
{
	if (next->next != NULL)
		pre->next = next->next;
	else
		pre->next = NULL;
	free_me(next);

}

/*---------------------------------------------------------------------------
 * (function: remove_fanout_pins)
 *-------------------------------------------------------------------------*/
void remove_fanout_pins(nnode_t *node)
{
	int i, j, k, idx;
	for (i = 0; i < node->num_input_pins; i++)
	{
		idx = node->input_pins[i]->unique_id;
		for (j = 0; j < node->input_pins[i]->net->num_fanout_pins; j++)
		{
			if (node->input_pins[i]->net->fanout_pins[j]->unique_id == idx)
				break;
		}
		for (k = j; k < node->input_pins[i]->net->num_fanout_pins - 1; k++)
		{
			node->input_pins[i]->net->fanout_pins[k] = node->input_pins[i]->net->fanout_pins[k+1];
			node->input_pins[i]->net->fanout_pins[k]->pin_net_idx = k;
		}
		node->input_pins[i]->net->fanout_pins[k] = NULL;
		node->input_pins[i]->net->num_fanout_pins--;

	}
}

/*---------------------------------------------------------------------------
 * (function: reallocate_pins)
 *-------------------------------------------------------------------------*/
void reallocate_pins(nnode_t *node, nnode_t *next_node)
{
	int i, j;
	int pin_idx;
	nnode_t *input_node = NULL;
	nnet_t *net = NULL;
	npin_t *pin = NULL;
	for (i = 0; i < next_node->num_output_pins; i++)
	{
		for (j = 0; j < next_node->output_pins[i]->net->num_fanout_pins; j++)
		{
			if (next_node->output_pins[i]->net->fanout_pins[j]->node != NULL)
			{
				input_node = next_node->output_pins[i]->net->fanout_pins[j]->node;
				net = node->output_pins[i]->net;
				pin_idx = next_node->output_pins[i]->net->fanout_pins[j]->pin_node_idx;
				pin = input_node->input_pins[pin_idx];
				add_fanout_pin_to_net(net, pin);
			}

		}
	}
}

/*---------------------------------------------------------------------------
 * (function: free_op_nodes)
 *-------------------------------------------------------------------------*/
void free_op_nodes(nnode_t *node)
{
	int i;
	nnet_t *net = NULL;
	for (i = 0; i < node->num_output_pins; i++)
	{
		if (node->output_pins[i]->net != NULL)
		{
			net = node->output_pins[i]->net;
			if (net->fanout_pins != NULL)
				free_me(net->fanout_pins);
			free_me(net);
			node->output_pins[i]->net = NULL;
		}
	}
	free_nnode(node);

}

/*---------------------------------------------------------------------------
 * (function: match_pins)
 *-------------------------------------------------------------------------*/
int match_pins(nnode_t *node, nnode_t *next_node)
{
	int flag = 0;
	int i, j;
	long id;
	for (i = 0; i < node->num_input_pins; i++)
	{
		flag = 0;
		id = node->input_pins[i]->net->driver_pin->unique_id;
		for (j = 0; j < next_node->num_input_pins; j++)
		{
			if (id == next_node->input_pins[j]->net->driver_pin->unique_id)
			{
				flag = 1;
				break;
			}
			if (j == next_node->num_input_pins - 1)
			{
				flag = -1;
				break;
			}
		}
		if (flag == -1)
			break;
	}

	return flag;
}
