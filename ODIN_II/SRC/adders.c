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
#include "netlist_utils.h"
#include "partial_map.h"
#include "util.h"
#include "read_xml_arch_file.h"
#include "globals.h"
#include "errors.h"

t_model *hard_adders = NULL;
struct s_linked_vptr *adder_list = NULL;
struct s_linked_vptr *add_list = NULL;
int min_add = 0;
int *adder = NULL;


void instantiate_simple_soft_adder(nnode_t *node, short mark, netlist_t *netlist)
{
	int width_a;
	int width_b;
	int width_cout;
	int width_sumout;
	int i;

	/* need for an carry-ripple-adder for each of the bits of port B. */
	/* good question of which is better to put on the bottom of adder.  Larger means more smaller adds, or small is
	 * less large adds */
	oassert(node->num_output_pins > 0);
	oassert(node->num_input_pins > 0);
	oassert(node->num_input_port_sizes == 2);

	width_a = node->input_port_sizes[0];
	width_b = node->input_port_sizes[1];
	width_sumout = node->output_port_sizes[0];
	width_cout = 1;

	nnode_t **new_add_cells;
	nnode_t **new_carry_cells;

	new_add_cells  = (nnode_t**)malloc(sizeof(nnode_t*)*(width_sumout + width_cout));
	new_carry_cells = (nnode_t**)malloc(sizeof(nnode_t*)*(width_sumout + width_cout));

		/* create the adder units and the zero unit */
		for (i = 0; i < width_sumout + width_cout; i++)
		{
			new_add_cells[i] = make_3port_gate(ADDER_FUNC, 1, 1, 1, 1, node, mark);
			// The last carry cell will be connected to an output pin, if one is available
			new_carry_cells[i] = make_3port_gate(CARRY_FUNC, 1, 1, 1, 1, node, mark);

		}

	    	/* ground first carry in */
		add_input_pin_to_node(new_add_cells[0], get_zero_pin(netlist), 0);
		if (i > 1)
		{
			add_input_pin_to_node(new_carry_cells[0], get_zero_pin(netlist), 0);
		}

		/* connect inputs */
		for(i = 0; i < width_sumout + width_cout; i++)
		{
			if (i < width_a)
			{
				/* join the A port up to adder */
				remap_pin_to_new_node(node->input_pins[i], new_add_cells[i], 1);
				if (i < width_sumout - 1)
					add_input_pin_to_node(new_carry_cells[i], copy_input_npin(new_add_cells[i]->input_pins[1]), 1);
			}
			else
			{
				add_input_pin_to_node(new_add_cells[i], get_zero_pin(netlist), 1);
				if (i < width_sumout - 1)
					add_input_pin_to_node(new_carry_cells[i], get_zero_pin(netlist), 1);
			}

			if (i < width_b)
			{
				/* join the B port up to adder */
				remap_pin_to_new_node(node->input_pins[i+width_a], new_add_cells[i], 2);
				if (i < width_sumout - 1)
					add_input_pin_to_node(new_carry_cells[i], copy_input_npin(new_add_cells[i]->input_pins[2]), 2);
			}
			else
			{
				add_input_pin_to_node(new_add_cells[i], get_zero_pin(netlist), 2);
				if (i < width_sumout - 1)
					add_input_pin_to_node(new_carry_cells[i], get_zero_pin(netlist), 2);
			}

			/* join that gate to the output */
			remap_pin_to_new_node(node->output_pins[i], new_add_cells[i], 0);
		}

		/* connect carry outs with carry ins */
		for(i = 1; i < width_sumout+width_cout; i++)
		{
			connect_nodes(new_carry_cells[i-1], 0, new_add_cells[i], 0);
			if (i < width_sumout)
				connect_nodes(new_carry_cells[i-1], 0, new_carry_cells[i], 0);
		}

		free(new_add_cells);
		free(new_carry_cells);
}

#ifdef VPR6
/*---------------------------------------------------------------------------
 * (function: init_mult_distribution)
 *  For adder, the output will only be input size + 1
 *-------------------------------------------------------------------------*/
void init_add_distribution()
{
	int i;

	oassert(hard_adders != NULL);
	adder = (int *)malloc(sizeof(int) * (hard_adders->inputs->size + 1));
	for (i = 0; i <= hard_adders->inputs->size; i++)
		adder[i] = 0;
	return;
}

/*---------------------------------------------------------------------------
 * (function: record_mult_distribution)
 *-------------------------------------------------------------------------*/
void record_add_distribution(nnode_t *node)
{
	//int a, b;

	oassert(hard_adders != NULL);
	oassert(node != NULL);

	//a = node->input_port_sizes[0];
	//b = node->input_port_sizes[1];

	adder[hard_adders->inputs->size] += 1;
	return;
}

/*---------------------------------------------------------------------------
 * (function: report_add_distribution)
 *-------------------------------------------------------------------------*/

void report_add_distribution()
{
	int i, j;
	int total = 0;

	if(hard_adders == NULL)
		return;

	printf("\nHard adder Distribution\n");
	printf("============================\n");
	for (i = 0; i <= hard_adders->inputs->size; i++)
	{
		for (j = 1; j <= hard_adders->inputs->next->size; j++)
		{
			if (adder[i * hard_adders->inputs->size + j] != 0)
			{
				total += adder[i * hard_adders->inputs->size + j];
				printf("%d X %d => %d\n", i, j, adder[i * hard_adders->inputs->size + j]);
			}
		}
	}
	printf("\n");
	printf("\nTotal # of adders = %d\n", total);
	return;
}

/*---------------------------------------------------------------------------
 * (function: find_hard_adders)
 *-------------------------------------------------------------------------*/
void find_hard_adders()
{
	hard_adders = Arch.models;
	min_add = configuration.min_hard_adder;
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
	int width_a, width_b, width_sumout, swap;

	/* See if this size instance of adder exists? */
	if (hard_adders == NULL)
	{
		printf(ERRTAG "Instantiating adder where adders do not exist\n");
	}
	tmp = (t_adder *)hard_adders->instances;
	width_a = node->input_port_sizes[0];
	width_b = node->input_port_sizes[1];
	//width_cin = node->input_port_sizes[2];
	width_sumout = node->output_port_sizes[1];
	//width_sumout = node->output_port_sizes[1];
	if (width_a < width_b) /* Make sure a is bigger than b */
	{
		swap = width_b;
		width_b = width_a;
		width_a = swap;
	}
	while (tmp != NULL)
	{
		if ((tmp->size_a == width_a) && (tmp->size_b == width_b) && (tmp->size_sumout == width_sumout))
			return;
		else
			tmp = tmp->next;
	}

	/* Does not exist - must create an instance */
	tmp = (t_adder *)malloc(sizeof(t_adder));
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
void instantiate_hard_adder(nnode_t *node, short mark, netlist_t *netlist)
{
	char *new_name;
	int len, sanity, i;

	declare_hard_adder(node);

	/* Need to give node proper name */
	len = strlen(node->name);
	len = len + 20; /* 20 chars should hold mul specs */
	new_name = (char*)malloc(len);

	/* wide input first :) */
	if (node->input_port_sizes[0] > node->input_port_sizes[1])
		sanity = sprintf(new_name, "%s_%d_%d_%d", node->name, node->input_port_sizes[0], node->input_port_sizes[1], node->output_port_sizes[0]);
	else
		sanity = sprintf(new_name, "%s_%d_%d_%d", node->name, node->input_port_sizes[1], node->input_port_sizes[0], node->output_port_sizes[0]);

	if (len <= sanity) /* buffer not large enough */
		oassert(FALSE);

	/* Give names to the output pins */
	for (i = 0; i < node->num_output_pins;  i++)
	{
		if (node->output_pins[i]->name)
		{
			free(node->output_pins[i]->name);
		}
		len = strlen(node->name) + 6; /* 6 chars for pin idx */
		new_name = (char*)malloc(len);
		sprintf(new_name, "%s[%d]", node->name, node->output_pins[i]->pin_node_idx);
		node->output_pins[i]->name = new_name;
	}
	free(node->name);
	node->name = new_name;
	node->traverse_visited = mark;
	return;
}


/*----------------------------------------------------------------------------
 * function: add_the_blackbox_for_mults()
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
			count = fprintf(out, ".model adder_%d_%d_%d_%d_%d\n", adds->size_a, adds->size_b, adds->size_cin, adds->size_sumout, adds->size_cout);

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
			if (i < adds->size_sumout)
			{
				count = count + sprintf(buffer, " %s[%d]", psumout, i);
			}
			else
			{
				count = count + sprintf(buffer, " %s[%d]", pcout, i - adds->size_sumout);
			}


			//count = count + sprintf(buffer, " %s[%d]", po, i);
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
void define_add_function(nnode_t *node, short type, FILE *out)
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

	if (configuration.fixed_hard_adder != 0)
	{
		count += fprintf(out, " adder");
	}
	else
	{
		count += fprintf(out, " adder_%d_%d_%d_%d_%d", node->input_port_sizes[0],
			node->input_port_sizes[1], node->input_port_sizes[2], node->output_port_sizes[1], node->output_port_sizes[0]);
	}


	for (i = 0;  i < node->num_input_pins; i++)
	{
		if (i < node->input_port_sizes[0])
		{
			npin_t *driver_pin = node->input_pins[i]->net->driver_pin;

			if (!driver_pin->name)
				j = sprintf(buffer, " %s[%d]=%s", hard_adders->inputs->next->next->name, i, driver_pin->node->name);
			else
				j = sprintf(buffer, " %s[%d]=%s", hard_adders->inputs->next->next->name, i, driver_pin->name);
		}
		else if(i >= node->input_port_sizes[0] && i < node->input_port_sizes[1] + node->input_port_sizes[0])
		{
			npin_t *driver_pin = node->input_pins[i]->net->driver_pin;

			if (!driver_pin->name)
				j = sprintf(buffer, " %s[%d]=%s", hard_adders->inputs->next->name, i - node->input_port_sizes[0], driver_pin->node->name);
			else
				j = sprintf(buffer, " %s[%d]=%s", hard_adders->inputs->next->name, i - node->input_port_sizes[0], driver_pin->name);
		}
		else
		{
			npin_t *driver_pin = node->input_pins[i]->net->driver_pin;
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


	for (i = node->num_output_pins - 1; i >= 0; i--)
	{
		if(i > node->output_port_sizes[0] - 1)
		{
			j = sprintf(buffer, " %s[%d]=%s", hard_adders->outputs->name, node->output_port_sizes[1] - i , node->output_pins[(node->output_port_sizes[1] - i)]->name);
		}
		else
		{
			j = sprintf(buffer, " %s[%d]=%s", hard_adders->outputs->next->name, i, node->output_pins[(i + node->output_port_sizes[1])]->name);
		}
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
 *	with temp pins to connect cascading adders/adders.
 *---------------------------------------------------------------------*/
void init_split_adder(nnode_t *node, nnode_t *ptr, int a, int sizea, int b, int sizeb, int cin, int cout, int index)
{
	int i;
	int flaga = 0, flagb = 0;
	int current_sizea, current_sizeb;

	/* Copy properties from original node */
	ptr->type = node->type;
	ptr->related_ast_node = node->related_ast_node;
	ptr->traverse_visited = node->traverse_visited;
	ptr->node_data = NULL;

	current_sizea = a - sizea * index;
	current_sizeb = b - sizeb * index;
	if(current_sizea > sizea)
		current_sizea = sizea;
	else if(current_sizea <= 0)
	{
		current_sizea = 1;
		flaga = 1;
	}

	if(current_sizeb > sizeb)
		current_sizeb = sizeb;
	else if(current_sizeb <= 0)
	{
		current_sizeb = 1;
		flagb = 1;
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
	if(current_sizea > current_sizeb)
		ptr->output_port_sizes[1] = current_sizea;
	else
		ptr->output_port_sizes[1] = current_sizeb;

	/* Set the number of pins and re-locate previous pin entries */
	ptr->num_input_pins = current_sizea + current_sizeb + cin;
	ptr->input_pins = (npin_t**)malloc(sizeof(void *) * (current_sizea + current_sizeb + cin));
	//if flaga or flagb = 1, the input pins should be empty.
	if(flaga == 1)
	{
		for (i = 0; i < current_sizea; i++)
			ptr->input_pins[i] = NULL;
	}
	else
	{
		for (i = 0; i < current_sizea; i++)
		{
			ptr->input_pins[i] = node->input_pins[i + index * sizea];
			ptr->input_pins[i]->node = ptr;
		}
	}

	if(flagb == 1)
	{
		for (i = 0; i < current_sizeb; i++)
			ptr->input_pins[i+current_sizea] = NULL;
	}
	else
	{
		for (i = 0; i < current_sizeb; i++)
		{
			ptr->input_pins[i+current_sizea] = node->input_pins[i + a + index * sizeb];
			ptr->input_pins[i+current_sizea]->node = ptr;
		}
	}

	for (i = 0; i < cin; i++)
	{
		ptr->input_pins[i+current_sizea+current_sizeb] = NULL;
	}

	int output;
	if(current_sizea > current_sizeb)
	{
		output = current_sizea + cout;
	}
	else
	{
		output = current_sizeb + cout;
	}
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
 *  a adder and is told how to split it. The end result is:
 *
 *  a1a0 * b1b0 => a0 * b0 + a0 * b1 + a1 * b0 + a1 * b1 => c1c0 => c
 *  
 * If we "balance" the additions, we can actually remove one of the
 * addition operations since we know that a0 * b0 and a1 * b1 will
 * not overlap in bits. This afind_hard_addersllows us to skip the addition between
 * these two terms and simply concat the results together. Giving us
 * the resulting logic:
 *
 * ((a1 * b1) . (a0 * b0)) + ((a0 * b1) + (a1 * b0)) ==> Result
 *
 * Note that for some of the additions we need to perform sign extensions,
 * but this should not be a problem since the sign extension is always
 * extending NOT contracting.
 * 		spit_adder(node, sizea, a, b, cin, count);
 *-----------------------------------------------------------------------*/

void split_adder(nnode_t *nodeo, int a, int b, int sizea, int sizeb, int cin, int cout, int count, netlist_t *netlist)
{
	nnode_t **node;
	int i,j;
	int num;

	/* Check for a legitimate split */
	oassert(nodeo->input_port_sizes[0] == a);
	oassert(nodeo->input_port_sizes[1] == b);

	node  = (nnode_t**)malloc(sizeof(nnode_t*)*(count + 1));

	for(i = 0; i < count; i++)
	{
		node[i] = allocate_nnode();
		node[i]->name = (char *)malloc(strlen(nodeo->name) + 4);
		//strcpy(node[i]->name, nodeo->name);
		//strcat(node[i]->name, "-");
		sprintf(node[i]->name, "%s-%d", nodeo->name, i);
		init_split_adder(nodeo, node[i], a, sizea, b, sizeb, cin, cout, i);
		add_list = insert_in_vptr_list(add_list, node[i]);
	}

	//connect the first cin pin to gnd
	if(a > sizea && b > sizeb)
		connect_nodes(netlist->gnd_node, 0, node[0], (sizea + sizeb));
	else
		connect_nodes(netlist->gnd_node, 0, node[0], (a + b));

	//if any input pins beside first cin pins are NULL, connect those pins to gnd
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

	//remap the output pins of each adder to nodeo
	for(i = 0; i < count; i++)
	{
		for(j = 0; j < node[i]->num_output_pins - 1; j ++)
			remap_pin_to_new_node(nodeo->output_pins[i * sizea + j], node[i], j);
	}

	// the last node's cout should be remapped to the most significant bits of nodeo
	remap_pin_to_new_node(nodeo->output_pins[(nodeo->num_output_pins - 1)], node[(count - 1)], (node[(count - 1)]->num_output_pins - 1));

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
 *	hard_adders data.
 * node, a, b, sizea, sizeb, 1, 1, count, netlist
 * NOTE: The inputs are extended based on adder padding setting.
 *-----------------------------------------------------------------------*/
void pad_adder(nnode_t *node, int a, int b, int sizea, int sizeb, int cin, int cout, int count, netlist_t *netlist)
{
	int diffa, diffb, diffout, i;
	int sizeout;

	int testa, testb;

	static int pad_pin_number = 0;

	oassert(node->type == ADD);
	oassert(hard_adders != NULL);

	sizea = node->input_port_sizes[0];
	sizeb = node->input_port_sizes[1];
	sizeout = node->output_port_sizes[0];
	record_add_distribution(node);

	/* Calculate the BEST fit hard adder to use */
	diffa = hard_adders->inputs->size - sizea;
	diffb = hard_adders->inputs->next->size - sizeb;
	diffout = hard_adders->outputs->size - sizeout;

	if (configuration.split_hard_adder == 1)
	{
		struct s_linked_vptr *plist = hard_adders->pb_types;
		while ((diffa + diffb) && plist)
		{
			t_pb_type *physical = (t_pb_type *)(plist->data_vptr);
			plist = plist->next;
			testa = physical->ports[0].num_pins;
			testb = physical->ports[1].num_pins;
			if ((testa >= sizea) && (testb >= sizeb) &&
				((testa - sizea + testb - sizeb) < (diffa + diffb)))
			{
				diffa = testa - sizea;
				diffb = testb - sizeb;
				diffout = physical->ports[2].num_pins - sizeout;
			}
		}
	}

	/* Expand the inputs */
	if ((diffa != 0) || (diffb != 0))
	{
		allocate_more_input_pins(node, diffa + diffb);

		/* Shift pins for expansion of first input pins */
		if (diffa != 0)
		{
			for (i = 1; i <= sizeb; i++)
			{
				move_input_pin(node, sizea + sizeb - i, node->num_input_pins - diffb - i);
			}

			/* Connect unused first input pins to zero/pad pin */
			for (i = 0; i < diffa; i++)
			{
				if (configuration.add_padding == 0)
					add_input_pin_to_node(node, get_zero_pin(netlist), i + sizea);
				else
					add_input_pin_to_node(node, get_pad_pin(netlist), i + sizea);
			}

			node->input_port_sizes[0] = sizea + diffa;
		}

		if (diffb != 0)
		{
			/* Connect unused second input pins to zero/pad pin */
			for (i = 1; i <= diffb; i++)
			{
				if (configuration.add_padding == 0)
					add_input_pin_to_node(node, get_zero_pin(netlist), node->num_input_pins - i);
				else
					add_input_pin_to_node(node, get_pad_pin(netlist), node->num_input_pins - i);
			}

			node->input_port_sizes[1] = sizeb + diffb;
		}
	}

	/* Expand the outputs */
	if (diffout != 0)
	{
		allocate_more_output_pins(node, diffout);
		for (i = 0; i < diffout; i++)
		{
			// Add new pins to the higher order spots.
			npin_t *new_pin = allocate_npin();
			// Pad outputs with a unique and descriptive name to avoid collisions.
			new_pin->name = append_string("", "unconnected_adder_output~%d", pad_pin_number++);
			add_output_pin_to_node(node, new_pin, i + sizeout);
		}
		node->output_port_sizes[0] = sizeout + diffout;
	}

	return;
}

/*-------------------------------------------------------------------------
 * (function: iterate_adders)
 *
 * This function will iterate over all of the multiply operations that
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
	nnode_t *node;

	/* Can only perform the optimisation if hard adders exist! */
	if (hard_adders == NULL)
		return;
	//In hard block adder, the summand and addend are same size.
	sizecin = hard_adders->inputs->size;
	sizeb = hard_adders->inputs->next->size;
	sizea = hard_adders->inputs->next->size;

	while(add_list != NULL)
	{
		node = (nnode_t *)add_list->data_vptr;
		add_list = delete_in_vptr_list(add_list);
		adder_list = insert_in_vptr_list(adder_list, node);
	}

	while (adder_list != NULL)
	{
		node = (nnode_t *)adder_list->data_vptr;
		adder_list = delete_in_vptr_list(adder_list);

		oassert(node != NULL);
		oassert(node->type == ADD);

		a = node->input_port_sizes[0];
		b = node->input_port_sizes[1];
		//fixed_hard_adder = 0 then the use hard block for extra bits
		if(configuration.fixed_hard_adder == 0){
			// how many adders a can split
			if(a % sizea == 0)
				counta = a / sizea;
			else
				counta = a / sizea + 1;
			// how many adders b can split
			if(b % sizeb == 0)
				countb = b / sizeb;
			else
				countb = b / sizeb + 1;
			// how many adders need to be split
			if(counta >= countb)
				count = counta;
			else
				count = countb;

			split_adder(node, a, b, sizea, sizeb, 1, 1, count, netlist);
			//add_list = insert_in_vptr_list(add_list, node);
		}
		else
		{
			counta = a / sizea;
			countb = b / sizeb;
			if(counta > countb)
				count = countb;
			else
				count = counta;
			pad_adder(node, a, b, sizea, sizeb, 1, 1, count, netlist);
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
void clean_adders()
{
	while (add_list != NULL)
		add_list = delete_in_vptr_list(add_list);
	return;
}

#endif
