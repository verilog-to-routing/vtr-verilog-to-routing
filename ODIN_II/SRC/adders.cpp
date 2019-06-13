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
#include <math.h>

#include "odin_types.h"
#include "node_creation_library.h"
#include "soft_logic_def_parser.h"
#include "adders.h"
#include "netlist_utils.h"
#include "partial_map.h"
#include "read_xml_arch_file.h"
#include "odin_globals.h"

#include "subtractions.h"

#include "vtr_memory.h"

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
	adder = (int *)vtr::malloc(sizeof(int) * (j + 1));
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
extern long adder_chain_count;
extern long longest_adder_chain;
extern long total_adders;

extern double geomean_addsub_length;
extern double sum_of_addsub_logs;

void report_add_distribution()
{
	if(hard_adders == NULL)
		return;

	printf("\nHard adder Distribution\n");
	printf("============================\n");
	printf("\n");
	printf("\nTotal # of chains = %ld\n", adder_chain_count);

	printf("\nHard adder chain Details\n");
	printf("============================\n");

	printf("\n");
	printf("\nThe Number of Hard Block adders in the Longest Chain: %ld\n", longest_adder_chain);

	printf("\n");
	printf("\nThe Total Number of Hard Block adders: %ld\n", total_adders);

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
	if (hard_adders == NULL)
		warning_message(NETLIST_ERROR, node->related_ast_node->line_number, node->related_ast_node->file_number, "%s\n", "Instantiating adder where adders do not exist");
	
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
 * (function: instantiate_hard_addier )
 *-------------------------------------------------------------------------*/
void instantiate_hard_adder(nnode_t *node, short mark, netlist_t * /*netlist*/)
{
	char *new_name;
	int len, sanity, i;

	declare_hard_adder(node);

	/* Need to give node proper name */
	len = strlen(node->name);
	len = len + 20; /* 20 chars should hold mul specs */
	new_name = (char*)vtr::malloc(len);

	/* wide input first :) identical branches! */
	// if (node->input_port_sizes[0] > node->input_port_sizes[1])
	// 	sanity = odin_sprintf(new_name, "%s", node->name);
	// else
		sanity = odin_sprintf(new_name, "%s", node->name);

	if (len <= sanity) /* buffer not large enough */
		oassert(FALSE);

	/* Give names to the output pins */
	for (i = 0; i < node->num_output_pins;  i++)
	{
		if (node->output_pins[i]->name == NULL)
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
		/* write out this adder model TODO identical branches ?*/
		// if (configuration.fixed_hard_adder != 0)
		// 	count = fprintf(out, ".model adder\n");
		// else
			count = fprintf(out, ".model adder\n");

		/* add the inputs */
		count = count + fprintf(out, ".inputs");
		hard_add_inputs = adds->size_a + adds->size_b + adds->size_cin;
		for (i = 0; i < hard_add_inputs; i++)
		{
			if (i < adds->size_a)
			{
				count = count + odin_sprintf(buffer, " %s[%ld]", pa, i);
			}
			else if(i < hard_add_inputs - adds->size_cin && i >= adds->size_a)
			{
				count = count + odin_sprintf(buffer, " %s[%ld]", pb, i - adds->size_a);
			}
			else
			{
				count = count + odin_sprintf(buffer, " %s[%ld]", pcin, i - adds->size_a - adds->size_b);
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
				count = count + odin_sprintf(buffer, " %s[%ld]", pcout, i);
			}
			else
			{
				count = count + odin_sprintf(buffer, " %s[%ld]", psumout, i - adds->size_cout);
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
void define_add_function(nnode_t *node, FILE *out)
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

	/* Write out th	bec_csla 	= 3 	// binary to excess carry Select Addere model adder  */
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
					j = odin_sprintf(buffer, " %s[%ld]=%s", hard_adders->inputs->next->next->name, i, driver_pin->node->name);
				else
					j = odin_sprintf(buffer, " %s[%ld]=%s", hard_adders->inputs->next->next->name, i, driver_pin->name);
			}
			else if(i >= node->input_port_sizes[0] && i < node->input_port_sizes[1] + node->input_port_sizes[0])
			{
				if (!driver_pin->name)
					j = odin_sprintf(buffer, " %s[%ld]=%s", hard_adders->inputs->next->name, i - node->input_port_sizes[0], driver_pin->node->name);
				else
					j = odin_sprintf(buffer, " %s[%ld]=%s", hard_adders->inputs->next->name, i - node->input_port_sizes[0], driver_pin->name);
			}
			else
			{
				if (!driver_pin->name)
					j = odin_sprintf(buffer, " %s[%ld]=%s", hard_adders->inputs->name, i - (node->input_port_sizes[0] + node->input_port_sizes[1]), driver_pin->node->name);
				else
					j = odin_sprintf(buffer, " %s[%ld]=%s", hard_adders->inputs->name, i - (node->input_port_sizes[0] + node->input_port_sizes[1]), driver_pin->name);
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
			j = odin_sprintf(buffer, " %s[%ld]=%s", hard_adders->outputs->next->name, i , node->output_pins[i]->name);
		else
			j = odin_sprintf(buffer, " %s[%ld]=%s", hard_adders->outputs->name, i - node->output_port_sizes[0], node->output_pins[i]->name);
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
 * ##################################
 * TODO the soft logic adders can now be splitted at the source, we could tap onto that and merge these function for
 * simplicicity and would also make sure to keep the allocation at one place
 *###################################
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

    // if the input of the first cin is generated by a dummy adder added
    // to the start of the chain, then an offset is needed to compensate
    // for that in various positions in the code, otherwise the offset is 0
    const int offset = (configuration.adder_cin_global)? 0 : 1;

	/* Copy properties from original node */
	ptr->type = node->type;
	ptr->bit_width = node->bit_width;
	ptr->related_ast_node = node->related_ast_node;
	ptr->traverse_visited = node->traverse_visited;
	ptr->node_data = NULL;

	/* decide the current size of input a and b */
	if(flag == 0)
	{
        // increase input sizes by one if a dummy adder is
        // added to feed the first cin in the chain
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
	ptr->input_port_sizes = (int *)vtr::malloc(3 * sizeof(int));
	ptr->input_port_sizes[0] = current_sizea;
	ptr->input_port_sizes[1] = current_sizeb;
	ptr->input_port_sizes[2] = cin;
	ptr->num_output_port_sizes = 2;
	ptr->output_port_sizes = (int *)vtr::malloc(2 * sizeof(int));
	ptr->output_port_sizes[0] = cout;

	/* The size of output port sumout equals the maxim size of sizea and sizeb  */
	if(current_sizea > current_sizeb)
		ptr->output_port_sizes[1] = current_sizea;
	else
		ptr->output_port_sizes[1] = current_sizeb;

	/* Set the number of pins and re-locate previous pin entries */
	ptr->num_input_pins = current_sizea + current_sizeb + cin;
	ptr->input_pins = (npin_t**)vtr::malloc(sizeof(void *) * (current_sizea + current_sizeb + cin));
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
		if(index == 0 && !configuration.adder_cin_global)
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
                    // use the offset to compensate for the dummy adder added at start of the chain
					ptr->input_pins[i] = node->input_pins[i + index * sizea - offset];
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
		if(index == 0 && !configuration.adder_cin_global)
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
					ptr->input_pins[i + current_sizea] = node->input_pins[i + a + index * sizeb - offset];
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
 * Note: In this function, we can do padding(default -1), fix the size of hard block adder.
 *-----------------------------------------------------------------------*/

void split_adder(nnode_t *nodeo, int a, int b, int sizea, int sizeb, int cin, int cout, int count, netlist_t *netlist)
{
	nnode_t **node;
	int i,j;
	int num, lefta = 0, leftb = 0;
	int max_num = 0;
	int flag = 0;

    // if the input of the first cin is generated by a dummy adder added
    // to the start of the chain, then an offset is needed to compensate
    // for that in various positions in the code, otherwise the offset is 0
    const int offset = (configuration.adder_cin_global)? 0 : 1;

	/* Check for a legitimate split */
	oassert(nodeo->input_port_sizes[0] == a);
	oassert(nodeo->input_port_sizes[1] == b);

	node  = (nnode_t**)vtr::malloc(sizeof(nnode_t*)*(count));

	for(i = 0; i < count; i++)
	{
		node[i] = allocate_nnode();
		node[i]->name = (char *)vtr::malloc(strlen(nodeo->name) + 20);
		odin_sprintf(node[i]->name, "%s-%ld", nodeo->name, i);
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

    // don't add a dummy adder in the beginning of the chain if the first cin will be connected to a global gnd
	if((flag == 0 || count > 1) && !configuration.adder_cin_global)
	{
		//connect the a[0] and b[0] of first adder node to ground
		connect_nodes(netlist->gnd_node, 0, node[0], 0);
		connect_nodes(netlist->gnd_node, 0, node[0], sizea);
		//hang the first sumout
		node[0]->output_pins[1] = allocate_npin();
		node[0]->output_pins[1]->name = append_string("", "%s~dummy_output~%ld~%ld", node[0]->name, 0, 1);
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

    if (configuration.adder_cin_global) {
        // connect first cin to gnd
        connect_nodes(netlist->gnd_node, 0, node[0], (node[0]->num_input_pins - 1));
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
					node[0]->output_pins[j + 2] = allocate_npin();
					node[0]->output_pins[j + 2]->name = append_string("", "%s~dummy_output~%ld~%ld", node[0]->name, 0, j + 2);
				}
				//hang the first cout
				node[0]->output_pins[0] = allocate_npin();
				node[0]->output_pins[0]->name = append_string("", "%s~dummy_output~%ld~%ld", node[0]->name, 0, 0);
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
        // if a dummy adder is added (offset = 1) start from the second adder)
		for(i = offset; i < count - 1; i ++)
		{
			for(j = 0; j < node[i]->num_output_pins - 1; j ++)
				remap_pin_to_new_node(nodeo->output_pins[i * sizea + j - offset], node[i], j + 1);
		}
		//Last adder
		if(flag == 0)
		{
			for(j = 0; j < node[count-1]->num_output_pins - 1; j ++)
			{
                // if a dummy adder is added to this chain (offset = 1), adjust the index of the adder using the offset constant
				if(((count - 1) * sizea + j - offset) < nodeo->num_output_pins)
					remap_pin_to_new_node(nodeo->output_pins[(count - 1) * sizea + j - offset], node[count - 1], j + 1);
				else
				{
					node[count - 1]->output_pins[j + 1] = allocate_npin();
					// Pad outputs with a unique and descriptive name to avoid collisions.
					node[count - 1]->output_pins[j + 1]->name = append_string("", "%s~dummy_output~%ld~%ld", node[count - 1]->name, count - 1, j + 1);
				}
			}
			//Hang the last cout
			node[count - 1]->output_pins[0] = allocate_npin();
			// Pad outputs with a unique and descriptive name to avoid collisions.
			node[count - 1]->output_pins[0]->name = append_string("", "%s~dummy_output~%ld~%ld", node[count - 1]->name, count - 1, 0);
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
				node[count - 1]->output_pins[0] = allocate_npin();
				// Pad outputs with a unique and descriptive name to avoid collisions.
				node[count - 1]->output_pins[0]->name = append_string("", "%s~dummy_output~%ld~%ld", node[count - 1]->name, count - 1, 0);
			}
		}
	}

	/* Probably more to do here in freeing the old node! */
	vtr::free(nodeo->name);
	vtr::free(nodeo->input_port_sizes);
	vtr::free(nodeo->output_port_sizes);

	/* Free arrays NOT the pins since relocated! */
	vtr::free(nodeo->input_pins);
	vtr::free(nodeo->output_pins);
	vtr::free(nodeo);
	vtr::free(node);
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

    // offset to the adder size in case a dummy adder is added to
    // start of the adder chain to feed the first cin with gnd
    const int offset = (configuration.adder_cin_global)? 0 : 1;

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
            // if the first cin in a chain is fed by a global input (offset = 0) the adder width is the
            // input width + 1 (to pass the last cout -> sumout) divided by size of the adder input ports
            // otherwise (offset = 1) a dummy adder is added to the chain to feed the first cin with gnd
            // how many adders a can split
            counta = (a + 1) / sizea + offset;
            // how many adders b can split
            countb = (b + 1) / sizeb + offset;
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
              oassert(component_s[0] && component_o[1] && component_s[1] && component_o[0]);
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
	long i;

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
	vtr::free(next);

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
	for (i = 0; i < node->num_output_pins; i++)
	{
		if (node->output_pins[i]->net != NULL)
			vtr::free(node->output_pins[i]->net->fanout_pins);
		vtr::free(node->output_pins[i]->net);
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


/*---------------------------------------------------------------------------------------------
 * ###########################################################
 * These are function for the soft_logic adders only
 *-------------------------------------------------------------------------------------------*/
void connect_output_pin_to_node(int *width, int current_pin, int output_pin_id, nnode_t *node, nnode_t *current_adder, short subtraction);
nnode_t *make_mux_2to1(nnode_t *select, nnode_t *port_a, nnode_t *port_b, nnode_t *node, short mark);
nnode_t *make_adder(operation_list funct, nnode_t *current_adder, nnode_t *previous_carry, int *width, int current_pin, netlist_t *netlist, nnode_t *node, short subtraction, short mark);
nnode_t *make_bec(operation_list funct, nnode_t *current_adder, nnode_t *previous_carry, int *width, int current_pin, netlist_t *netlist, nnode_t *node, short subtraction, short mark);

/*---------------------------------------------------------------------------------------------
 * connect adder type output pin to a node
 *-------------------------------------------------------------------------------------------*/
void connect_output_pin_to_node(int *width, int current_pin, int output_pin_id, nnode_t *node, nnode_t *current_adder, short subtraction)
{
	// output
	if(subtraction)
	{
		remap_pin_to_new_node(node->output_pins[current_pin], current_adder, output_pin_id);
	}
	else
	{
		npin_t *node_pin_select = node->output_pins[(node->num_input_port_sizes == 2)? current_pin : (current_pin < width[output_pin_id]-1)? current_pin+1 : 0];
		if(node_pin_select->type != NO_ID || (node->num_input_port_sizes == 2))
		{
			remap_pin_to_new_node(node_pin_select, current_adder, output_pin_id);
		}
		else
		{
			current_adder->output_pins[output_pin_id] = allocate_npin();
			current_adder->output_pins[output_pin_id]->name = append_string("", "%s~dummy_output~%ld", current_adder->name, output_pin_id);
		}
	}
}

/*---------------------------------------------------------------------------------------------
 * makes a 2 to 1 mux (select == 1)? port_a : port_b
 *-------------------------------------------------------------------------------------------*/
nnode_t *make_mux_2to1(nnode_t *select, nnode_t *port_a, nnode_t *port_b, nnode_t *node, short mark)
{
	nnode_t *mux_2 = make_2port_gate(MUX_2, 2, 2, 1, node, mark);

	//driver
	nnode_t *notted_gate = make_not_gate(node,mark);
	connect_nodes(select,0,notted_gate,0);


	connect_nodes(select,0,mux_2,0);
	connect_nodes(notted_gate,0,mux_2,1);

	//connect carry skip to mux
	connect_nodes(port_a,0,mux_2,2);
	connect_nodes(port_b,0,mux_2,3);
	return mux_2;
}

nnode_t *make_bec(operation_list funct, nnode_t *current_adder, nnode_t *previous_carry, int* /* width */, int /* current_pin */, netlist_t *netlist, nnode_t *node, short /* subtraction */, short mark)
{
	nnode_t *new_funct = NULL;
	if(previous_carry == netlist->vcc_node)
	{
		if(funct == ADDER_FUNC)
		{
			new_funct = make_not_gate(node, mark);
			connect_nodes(current_adder, 0, new_funct, 0);
		}
		else if(funct == CARRY_FUNC)
		{
			new_funct = current_adder;
		}
	}
	else if(previous_carry == netlist->gnd_node)
	{
		if(funct == ADDER_FUNC)
		{
			new_funct = current_adder;
		}
		else if(funct == CARRY_FUNC)
		{
			new_funct = netlist->gnd_node;
		}
	}
	else
	{
		if(funct == ADDER_FUNC)
		{
			new_funct = make_2port_gate(LOGICAL_XOR, 1, 1, 1, node, mark);
			connect_nodes(previous_carry,0, new_funct, 0);
			connect_nodes(current_adder,0, new_funct, 1);
		}
		else if(funct == CARRY_FUNC)
		{
			new_funct = make_2port_gate(LOGICAL_AND, 1, 1, 1, node, mark);
			connect_nodes(previous_carry,0, new_funct, 0);
			connect_nodes(current_adder,0, new_funct, 1);
		}
	}
	return new_funct;
}

/*---------------------------------------------------------------------------------------------
 * make a single half-adder (can do unary subtraction, binary subtraction and addition)
 *-------------------------------------------------------------------------------------------*/
nnode_t *make_adder(operation_list funct, nnode_t *current_adder, nnode_t *previous_carry, int *width, int current_pin, netlist_t *netlist, nnode_t *node, short subtraction, short mark)
{
	//make a 2 bit 0r 3 bit sum or carry based on previous carry
	nnode_t *new_funct = NULL;
	short is_three_port_gate = 0;


	if(previous_carry == netlist->gnd_node)
	{
		if(funct == ADDER_FUNC)
			new_funct = make_2port_gate(LOGICAL_XOR, 1, 1, 1, node, mark);
		else if(funct == CARRY_FUNC)
			new_funct = make_2port_gate(LOGICAL_AND, 1, 1, 1, node, mark);
	}
	else if(previous_carry == netlist->vcc_node)
	{
		if(funct == ADDER_FUNC)
			new_funct = make_2port_gate(LOGICAL_XNOR, 1, 1, 1, node, mark);
		else if(funct == CARRY_FUNC)
			new_funct = make_2port_gate(LOGICAL_OR, 1, 1, 1, node, mark);
	}
	else
	{
		if(funct == ADDER_FUNC)
			new_funct = make_3port_gate(ADDER_FUNC, 1, 1, 1, 1, node, mark);
		else if(funct == CARRY_FUNC)
			new_funct = make_3port_gate(CARRY_FUNC, 1, 1, 1, 1, node, mark);
		connect_nodes(previous_carry, 0, new_funct, 0);
		is_three_port_gate = 1;
	}

	//copy the input pin of a half-adder to another function (CARRY or ADDER)
	if(current_adder != NULL)
	{
		add_input_pin_to_node(new_funct, copy_input_npin(current_adder->input_pins[0+is_three_port_gate]), 0+is_three_port_gate);
		add_input_pin_to_node(new_funct, copy_input_npin(current_adder->input_pins[1+is_three_port_gate]), 1+is_three_port_gate);
	}
	// create one from scratch
	else
	{
		//connect input a
		if (current_pin < width[1])
		{
			npin_t *temp_pin = node->input_pins[current_pin];
			if(temp_pin->net->driver_pin == NULL || temp_pin->net->driver_pin->node->type == GND_NODE)
			{
				connect_nodes(netlist->gnd_node,0, new_funct, 0+is_three_port_gate);
				remove_fanout_pins_from_net(temp_pin->net, temp_pin, temp_pin->pin_net_idx);
			}
			else if(temp_pin->net->driver_pin->node->type == VCC_NODE)
			{
				connect_nodes(netlist->vcc_node,0, new_funct, 0+is_three_port_gate);
				remove_fanout_pins_from_net(temp_pin->net, temp_pin, temp_pin->pin_net_idx);
			}
			else
			{
				remap_pin_to_new_node(temp_pin, new_funct, 0+is_three_port_gate);
			}
		}
		else
		{
			connect_nodes(netlist->gnd_node,0, new_funct, 0+is_three_port_gate);
		}

		//connect input b
		if(current_pin < width[2])
		{
			//pin a is neighbor to pin b
			npin_t *temp_pin = node->input_pins[current_pin+width[1]];
			if(temp_pin->net->driver_pin == NULL || temp_pin->net->driver_pin->node->type == GND_NODE)
			{
				nnode_t* attach_to=(subtraction)? netlist->vcc_node: netlist->gnd_node;
				connect_nodes(attach_to,0, new_funct, 1+is_three_port_gate);
				remove_fanout_pins_from_net(temp_pin->net, temp_pin, temp_pin->pin_net_idx);
			}
			else if(temp_pin->net->driver_pin->node->type == VCC_NODE)
			{
				nnode_t* attach_to=(subtraction)? netlist->gnd_node: netlist->vcc_node;
				connect_nodes(attach_to,0, new_funct, 1+is_three_port_gate);
				remove_fanout_pins_from_net(temp_pin->net, temp_pin, temp_pin->pin_net_idx);
			}
			else
			{
				if(subtraction)
				{
					nnode_t *new_not_cells = make_not_gate(node, mark);
					remap_pin_to_new_node(temp_pin, new_not_cells, 0);
					connect_nodes(new_not_cells, 0, new_funct, 1+is_three_port_gate);
				}
				else
				{
					remap_pin_to_new_node(temp_pin, new_funct, 1+is_three_port_gate);
				}
			}
		}
		else
		{
			nnode_t* attach_to=(subtraction)? netlist->vcc_node: netlist->gnd_node;
			connect_nodes(attach_to,0, new_funct, 1+is_three_port_gate);
		}
	}
	return new_funct;
}

/***---------------------------------------------------------------------------------------------
 *
 * bit # :  	| 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
 * blk_type:	  R->[heterogenous      		]
 *                   [blk size and blk type		]
 *
 * the first bit is built from an/xor gate, everything between could be anything
 * create a single adder block using adder_type_definition file input to select blk size and type
 *-------------------------------------------------------------------------------------------*/
void instantiate_add_w_carry_block(int *width, nnode_t *node, short mark, netlist_t *netlist, short subtraction)
{
	//set the default
	int blk_size 							= width[0];
	nnode_t *initial_carry 		= (subtraction)? netlist->vcc_node: netlist->gnd_node;
	for(int start_pin=0, current_counter=1 ; start_pin<width[0]; start_pin+=blk_size, current_counter++)
	{
		std::string my_type 			= "soft";
		std::string sub_structure = "default";
		blk_size 									= width[0]-start_pin;

		soft_sub_structure *def = fetch_blk("+",width[0]-start_pin);
		if(def)
		{
			blk_size = (def->bitsize > blk_size)? blk_size: def->bitsize;

			//don't optimize small circuit
			if(blk_size != 1)
			{
				my_type 			= def->type;
				sub_structure = def->name;
			}

			/* pretty print info */
			if(current_counter == 1)
				printf("\n::%s\n[target-bit_size:<%d>START]~", node->name, width[0]-start_pin);

			printf("~[<%s><%s>::%d]~",my_type.c_str(), sub_structure.c_str(), blk_size);

			if(blk_size+start_pin == width[0]){
				printf("~[END::sub_module_count:<%d>] \n", current_counter);
				fflush(stdout);
			}
		}

		nnode_t *previous_carry = initial_carry;
		nnode_t *previous_carry_gnd = netlist->gnd_node;
		nnode_t *previous_carry_vcc = netlist->vcc_node;

		for(int i = start_pin; i < start_pin+blk_size; i++)
		{
			/* set of flags for building purposes */
			short construct_last_carry_flag = (i != width[0]-1 || !subtraction)? 1:0;
			short last_pin_on_blk_flag 			= (i == start_pin+blk_size-1)? 1:0;

			if(my_type == "soft")
			{
				// Ripple Carry Adder
				if(sub_structure == "default"){
					//build adder
					nnode_t *current_adder = make_adder(ADDER_FUNC, NULL, previous_carry, width, i, netlist, node, subtraction, mark);
					if(construct_last_carry_flag)
						 previous_carry = make_adder(CARRY_FUNC, current_adder, previous_carry, width, i, netlist, node, subtraction, mark);

					connect_output_pin_to_node(width, i, 0, node, current_adder, subtraction);
				}
				//Carry Select Adder
				else if(sub_structure == "csla")
				{
					nnode_t *current_adder_gnd = make_adder(ADDER_FUNC, NULL, previous_carry_gnd, width, i, netlist, node, subtraction, mark);
					if(construct_last_carry_flag)
						previous_carry_gnd = make_adder(CARRY_FUNC, current_adder_gnd, previous_carry_gnd, width, i, netlist, node, subtraction, mark);

					nnode_t *current_adder_vcc = make_adder(ADDER_FUNC, current_adder_gnd, previous_carry_vcc, width, i, netlist, node, subtraction, mark);
					if(construct_last_carry_flag)
						previous_carry_vcc = make_adder(CARRY_FUNC, current_adder_gnd, previous_carry_vcc, width, i, netlist, node, subtraction, mark);

					nnode_t *current_adder = make_mux_2to1(previous_carry, current_adder_vcc, current_adder_gnd, node, mark);

					if(last_pin_on_blk_flag && construct_last_carry_flag)
						previous_carry = make_mux_2to1(previous_carry, previous_carry_vcc, previous_carry_gnd, node, mark);

					connect_output_pin_to_node(width, i, 0, node, current_adder, subtraction);
				}
				//binary to excess Carry Select Adder
				else if(sub_structure == "bec_csla")
				{
					nnode_t *current_adder_gnd = make_adder(ADDER_FUNC, NULL, previous_carry_gnd, width, i, netlist, node, subtraction, mark);
					if(construct_last_carry_flag)
						 previous_carry_gnd = make_adder(CARRY_FUNC, current_adder_gnd, previous_carry_gnd, width, i, netlist, node, subtraction, mark);

					nnode_t *current_adder_vcc = make_bec(ADDER_FUNC, current_adder_gnd, previous_carry_vcc, width, i, netlist, node, subtraction, mark);
					if(construct_last_carry_flag)
						previous_carry_vcc = make_bec(CARRY_FUNC, current_adder_gnd, previous_carry_vcc, width, i, netlist, node, subtraction, mark);

					nnode_t *current_adder = make_mux_2to1(previous_carry, current_adder_vcc, current_adder_gnd, node, mark);

					if(last_pin_on_blk_flag && construct_last_carry_flag)
						previous_carry = make_mux_2to1(previous_carry, previous_carry_vcc, previous_carry_gnd, node, mark);

					connect_output_pin_to_node(width, i, 0, node, current_adder, subtraction);
				}
				else
				{
					error_message(NETLIST_ERROR, -1, -1, "( %s )is not a valid substructure name", sub_structure.c_str());
					return;
				}
			}
			else // if(my_type == "hard")
			{
				// carry lookahead
				// if(sub_structure == "cla")
				// {
				// 	error_message(NETLIST_ERROR, -1, -1, "( %s )is not a valid substructure name", sub_structure.c_str());
				// }
				// else
				// {
					error_message(NETLIST_ERROR, -1, -1, "( %s )is not a valid substructure name", sub_structure.c_str());
				// }
			}
		}
		initial_carry = previous_carry;
	}
}
