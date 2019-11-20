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
#include "multipliers.h"
#include "netlist_utils.h"
#include "partial_map.h"
#include "util.h"
#include "read_xml_arch_file.h"
#include "globals.h"
#include "errors.h"
#include "adders.h"

t_model *hard_multipliers = NULL;
struct s_linked_vptr *mult_list = NULL;
int min_mult = 0;
int *mults = NULL;

/*---------------------------------------------------------------------------
 * (function: instantiate_simple_soft_multiplier )
 * Sample 4x4 multiplier to help understand logic.
 *
 * 					a3 	a2	a1	a0 
 *					b3 	b2 	b1 	b0
 *					---------------------------
 *					c03	c02	c01	c00
 *			+	c13	c12	c11	c10
 *			-----------------------------------	
 *			r14	r13	r12	r11	r10
 *		+	c23	c22	c21	c20
 *		-----------------------------------
 *		r24	r23	r22	r21	r20
 *	+	c33	c32	c31	c30
 *	------------------------------------
 *	o7	o6	o5	o4	o3	o2	o1	o0
 *
 *	In the first case will be c01
 *-------------------------------------------------------------------------*/
void instantiate_simple_soft_multiplier(nnode_t *node, short mark, netlist_t *netlist)
{
	int width_a;
	int width_b;
	int width;
	int multiplier_width;
	int multiplicand_width;
	nnode_t **adders_for_partial_products;
	nnode_t ***partial_products;
	int multiplicand_offset_index;
	int multiplier_offset_index;
	int current_index;
	int i, j;

	/* need for an carry-ripple-adder for each of the bits of port B. */
	/* good question of which is better to put on the bottom of multiplier.  Larger means more smaller adds, or small is 
	 * less large adds */
	oassert(node->num_output_pins > 0);
	oassert(node->num_input_pins > 0);
	oassert(node->num_input_port_sizes == 2);
	oassert(node->num_output_port_sizes == 1);
	width_a = node->input_port_sizes[0];
	width_b = node->input_port_sizes[1];
	width = node->output_port_sizes[0];
	multiplicand_width = width_b;
	multiplier_width = width_a;
	/* offset is related to which multport is chosen as the multiplicand */
	multiplicand_offset_index = width_a;
	multiplier_offset_index = 0;

	adders_for_partial_products = (nnode_t**)malloc(sizeof(nnode_t*)*multiplicand_width-1);

	/* need to generate partial products for each bit in width B. */
	partial_products = (nnode_t***)malloc(sizeof(nnode_t**)*multiplicand_width);

	/* generate the AND partial products */
	for (i = 0; i < multiplicand_width; i++)
	{
		/* create the memory for each AND gate needed for the levels of partial products */
		partial_products[i] = (nnode_t**)malloc(sizeof(nnode_t*)*multiplier_width);
		
		if (i < multiplicand_width - 1)
		{
			adders_for_partial_products[i] = make_2port_gate(ADD, multiplier_width+1, multiplier_width+1, multiplier_width+1, node, mark);
		}

		for (j = 0; j < multiplier_width; j++)
		{
			/* create each one of the partial products */
			partial_products[i][j] = make_1port_logic_gate(LOGICAL_AND, 2, node, mark);
		}
	}

	/* generate the coneections to the AND gates */
	for (i = 0; i < multiplicand_width; i++)
	{
		for (j = 0; j < multiplier_width; j++)
		{
			/* hookup the input of B to each AND gate */
			if (j == 0)
			{
				/* IF - this is the first time we are mapping multiplicand port then can remap */ 
				remap_pin_to_new_node(node->input_pins[i+multiplicand_offset_index], partial_products[i][j], 0);
			}
			else
			{
				/* ELSE - this needs to be a new ouput of the multiplicand port */
				add_input_pin_to_node(partial_products[i][j], copy_input_npin(partial_products[i][0]->input_pins[0]), 0);
			}

			/* hookup the input of the multiplier to each AND gate */
			if (i == 0)
			{
				/* IF - this is the first time we are mapping multiplier port then can remap */ 
				remap_pin_to_new_node(node->input_pins[j+multiplier_offset_index], partial_products[i][j], 1);
			}
			else
			{
				/* ELSE - this needs to be a new ouput of the multiplier port */
				add_input_pin_to_node(partial_products[i][j], copy_input_npin(partial_products[0][j]->input_pins[1]), 1);
			}
		}
	}

	/* hookup each of the adders */
	for (i = 0; i < multiplicand_width-1; i++) // -1 since the first stage is a combo of partial products while all others are part of tree
	{
		for (j = 0; j < multiplier_width+1; j++) // +1 since adders are one greater than multwidth to pass carry
		{
			/* join to port 1 of the add one of the partial products.  */
			if (i == 0)
			{
				/* IF - this is the first addition row, then adding two sets of partial products and first set is from the c0* */
				if (j < multiplier_width-1)
				{
					/* IF - we just take an element of the first list c[0][j+1]. */
					connect_nodes(partial_products[i][j+1], 0, adders_for_partial_products[i], j);
				}
				else
				{
					/* ELSE - this is the last input to the first adder, then we pass in 0 since no carry yet */
					add_input_pin_to_node(adders_for_partial_products[i], get_zero_pin(netlist), j);
				}
			}
			else if (j < multiplier_width)
			{
				/* ELSE - this is the standard situation when we need to hookup this adder with a previous adder, r[i-1][j+1] */
				connect_nodes(adders_for_partial_products[i-1], j+1, adders_for_partial_products[i], j);
			}
			else
			{
				add_input_pin_to_node(adders_for_partial_products[i], get_zero_pin(netlist), j);
			}
			
			if (j < multiplier_width)
			{
				/* IF - this is not most significant bit then just add current partial product */
				connect_nodes(partial_products[i+1][j], 0, adders_for_partial_products[i], j+multiplier_width+1);
			}
			else
			{
				add_input_pin_to_node(adders_for_partial_products[i], get_zero_pin(netlist), j+multiplier_width+1);
			}
		}
	}

	current_index = 0;
	/* hookup the outputs */
	for (i = 0; i < width; i++)
	{
		if (multiplicand_width == 1)
		{
			// this is undealt with
			error_message(1,-1,-1,"Cannot create soft multiplier with multiplicand width of 1.\n");
		}
		else if (i == 0)
		{
			/* IF - this is the LSbit, then we use a pass through from the partial product */
			remap_pin_to_new_node(node->output_pins[i], partial_products[0][0], 0);
		}
		else if (i < multiplicand_width - 1)
		{
			/* ELSE IF - these are the middle values that come from the LSbit of partial adders */
			remap_pin_to_new_node(node->output_pins[i], adders_for_partial_products[i-1], 0);
		}
		else 
		{
			/* ELSE - the final outputs are straight from the outputs of the last adder */
			remap_pin_to_new_node(node->output_pins[i], adders_for_partial_products[multiplicand_width-2], current_index);
			current_index++;
		}
	}

	/* soft map the adders if they need to be mapped */
	for (i = 0; i < multiplicand_width - 1; i++)
	{
		instantiate_add_w_carry(adders_for_partial_products[i], mark, netlist);
	}

	/* Cleanup everything */
	if (adders_for_partial_products != NULL)
	{
		free(adders_for_partial_products);
	}
	/* generate the AND partial products */
	for (i = 0; i < multiplicand_width; i++)
	{
		/* create the memory for each AND gate needed for the levels of partial products */
		if (partial_products[i] != NULL)
		{
			free(partial_products[i]);
		}
	}
	if (partial_products != NULL)
	{
		free(partial_products);
	}
}

#ifdef VPR6
/*---------------------------------------------------------------------------
 * (function: init_mult_distribution)
 *-------------------------------------------------------------------------*/
void init_mult_distribution()
{
	int i, j;

	oassert(hard_multipliers != NULL);
	mults = (int *)malloc(sizeof(int) * (hard_multipliers->inputs->size + 1) * (1 + hard_multipliers->inputs->next->size));
	for (i = 0; i <= hard_multipliers->inputs->size; i++)
		for (j = 0; j <= hard_multipliers->inputs->next->size; j++)
			mults[i * hard_multipliers->inputs->size + j] = 0;
	return;
}

/*---------------------------------------------------------------------------
 * (function: record_mult_distribution)
 *-------------------------------------------------------------------------*/
void record_mult_distribution(nnode_t *node)
{
	int a, b;

	oassert(hard_multipliers != NULL);
	oassert(node != NULL);

	a = node->input_port_sizes[0];
	b = node->input_port_sizes[1];

	mults[a * hard_multipliers->inputs->size + b] += 1;
	return;
}

/*---------------------------------------------------------------------------
 * (function: report_mult_distribution)
 *-------------------------------------------------------------------------*/
void report_mult_distribution()
{
	int i, j;
	int total = 0;

	if(hard_multipliers == NULL)
		return;

	printf("\nHard Multiplier Distribution\n");
	printf("============================\n");
	for (i = 0; i <= hard_multipliers->inputs->size; i++)
	{
		for (j = 1; j <= hard_multipliers->inputs->next->size; j++)
		{
			if (mults[i * hard_multipliers->inputs->size + j] != 0)
			{
				total += mults[i * hard_multipliers->inputs->size + j];
				printf("%d X %d => %d\n", i, j, mults[i * hard_multipliers->inputs->size + j]);
			}
		}
	}
	printf("\n");
	printf("\nTotal # of multipliers = %d\n", total);
	return;
}

/*---------------------------------------------------------------------------
 * (function: find_hard_multipliers)
 *-------------------------------------------------------------------------*/
void find_hard_multipliers()
{
	hard_multipliers = Arch.models;
	min_mult = configuration.min_hard_multiplier;
	while (hard_multipliers != NULL)
	{
		if (strcmp(hard_multipliers->name, "multiply") == 0)
		{
			init_mult_distribution();
			return;
		}
		else
		{
			hard_multipliers = hard_multipliers->next;
		}
	}

	return;
}

/*---------------------------------------------------------------------------
 * (function: declare_hard_multiplier)
 *-------------------------------------------------------------------------*/
void declare_hard_multiplier(nnode_t *node)
{
	t_multiplier *tmp;
	int width_a, width_b, width, swap;

	/* See if this size instance of multiplier exists? */
	if (hard_multipliers == NULL)
	{
		printf(ERRTAG "Instantiating multiplier where multipliers do not exist\n");
	}
	tmp = (t_multiplier *)hard_multipliers->instances;
	width_a = node->input_port_sizes[0];
	width_b = node->input_port_sizes[1];
	width = node->output_port_sizes[0];
	if (width_a < width_b) /* Make sure a is bigger than b */
	{
		swap = width_b;
		width_b = width_a;
		width_a = swap;
	}
	while (tmp != NULL)
	{
		if ((tmp->size_a == width_a) && (tmp->size_b == width_b) && (tmp->size_out == width))
			return;
		else
			tmp = tmp->next;
	}

	/* Does not exist - must create an instance */
	tmp = (t_multiplier *)malloc(sizeof(t_multiplier));
	tmp->next = (t_multiplier *)hard_multipliers->instances;
	hard_multipliers->instances = tmp;
	tmp->size_a = width_a;
	tmp->size_b = width_b;
	tmp->size_out = width;
	return;
}

/*---------------------------------------------------------------------------
 * (function: instantiate_hard_multiplier )
 *-------------------------------------------------------------------------*/
void instantiate_hard_multiplier(nnode_t *node, short mark, netlist_t *netlist)
{
	char *new_name;
	int len, sanity, i;

	declare_hard_multiplier(node);

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
void add_the_blackbox_for_mults(FILE *out)
{
	int i;
	int count;
	int hard_mult_inputs;
	t_multiplier *muls;
	t_model_ports *ports;
	char buffer[MAX_BUF];
	char *pa, *pb, *po;

	/* Check to make sure this target architecture has hard multipliers */
	if (hard_multipliers == NULL)
		return;

	/* Get the names of the ports for the multiplier */
	ports = hard_multipliers->inputs;
	pb = ports->name;
	ports = ports->next;
	pa = ports->name;
	po = hard_multipliers->outputs->name;

	/* find the multiplier devices in the tech library */
	muls = (t_multiplier *)(hard_multipliers->instances);
	if (muls == NULL) /* No multipliers instantiated */
		return;

	/* simplified way of getting the multsize, but fine for quick example */
	while (muls != NULL)
	{
		/* write out this multiplier model */
		if (configuration.fixed_hard_multiplier != 0)
			count = fprintf(out, ".model multiply\n");
		else
			count = fprintf(out, ".model mult_%d_%d_%d\n", muls->size_a, muls->size_b, muls->size_out);

		/* add the inputs */
		count = count + fprintf(out, ".inputs");
		hard_mult_inputs = muls->size_a + muls->size_b;
		for (i = 0; i < hard_mult_inputs; i++)
		{
			if (i < muls->size_a)
			{
				count = count + sprintf(buffer, " %s[%d]", pa, i);
			}
			else
			{
				count = count + sprintf(buffer, " %s[%d]", pb, i - muls->size_a);
			}

			if (count > 78)
				count = fprintf(out, " \\\n %s", buffer) - 3;
			else
				fprintf(out, " %s", buffer);
		}
		fprintf(out, "\n");

		/* add the outputs */
		count = fprintf(out, ".outputs");
		for (i = 0; i < muls->size_out; i++)
		{
			count = count + sprintf(buffer, " %s[%d]", po, i);
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

		muls = muls->next;
	}
}

/*-------------------------------------------------------------------------
 * (function: define_mult_function)
 *-----------------------------------------------------------------------*/
void define_mult_function(nnode_t *node, short type, FILE *out)
{
	int i, j;
	int count;
	char buffer[MAX_BUF];

	count = fprintf(out, "\n.subckt");
	count--;
	oassert(node->input_port_sizes[0] > 0);
	oassert(node->input_port_sizes[1] > 0);
	oassert(node->output_port_sizes[0] > 0);

	int flip = FALSE;

	if (configuration.fixed_hard_multiplier != 0)
	{
		count += fprintf(out, " multiply");
	}
	else
	{
		if (node->input_port_sizes[0] > node->input_port_sizes[1])
		{
			count += fprintf(out, " mult_%d_%d_%d", node->input_port_sizes[0],
				node->input_port_sizes[1], node->output_port_sizes[0]);

			flip = FALSE;
		}
		else
		{
			count += fprintf(out, " mult_%d_%d_%d", node->input_port_sizes[1],
				node->input_port_sizes[0], node->output_port_sizes[0]);

			flip = TRUE;
		}
	}

	for (i = 0;  i < node->num_input_pins; i++)
	{
		if (i < node->input_port_sizes[flip?1:0])
		{
			npin_t *driver_pin = flip
						?node->input_pins[i+node->input_port_sizes[0]]->net->driver_pin
						:node->input_pins[i                          ]->net->driver_pin;

			if (!driver_pin->name)
				j = sprintf(buffer, " %s[%d]=%s", hard_multipliers->inputs->next->name, i, driver_pin->node->name);
			else
				j = sprintf(buffer, " %s[%d]=%s", hard_multipliers->inputs->next->name, i, driver_pin->name);
		}
		else
		{
			npin_t *driver_pin = flip
						?node->input_pins[i-node->input_port_sizes[1]]->net->driver_pin
						:node->input_pins[i                          ]->net->driver_pin;

			int index = flip
						?i - node->input_port_sizes[1]
						:i - node->input_port_sizes[0];

			if (!driver_pin->name)
				j = sprintf(buffer, " %s[%d]=%s", hard_multipliers->inputs->name, index, driver_pin->node->name);
			else
				j = sprintf(buffer, " %s[%d]=%s", hard_multipliers->inputs->name, index, driver_pin->name);
		}

		if (count + j > 79)
		{
			fprintf(out, "\\\n");
			count = 0;
		}
		count += fprintf(out, "%s", buffer);
	}


	for (i = 0; i < node->num_output_pins; i++)
	{
		j = sprintf(buffer, " %s[%d]=%s", hard_multipliers->outputs->name, i, node->output_pins[i]->name);
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
 * (function: init_split_multiplier)
 *	Create a dummy multiplier when spliting. Inputs are connected
 *	to original pins, output pins are set to NULL for later connecting
 *	with temp pins to connect cascading multipliers/adders.
 *---------------------------------------------------------------------*/
void init_split_multiplier(nnode_t *node, nnode_t *ptr, int offa, int a, int offb, int b)
{
	int i;

	/* Copy properties from original node */
	ptr->type = node->type;
	ptr->related_ast_node = node->related_ast_node;
	ptr->traverse_visited = node->traverse_visited;
	ptr->node_data = NULL;

	/* Set new port sizes and parameters */
	ptr->num_input_port_sizes = 2;
	ptr->input_port_sizes = (int *)malloc(2 * sizeof(int));
	ptr->input_port_sizes[0] = a;
	ptr->input_port_sizes[1] = b;
	ptr->num_output_port_sizes = 1;
	ptr->output_port_sizes = (int *)malloc(sizeof(int));
	ptr->output_port_sizes[0] = a + b;

	/* Set the number of pins and re-locate previous pin entries */
	ptr->num_input_pins = a + b;
	ptr->input_pins = (npin_t**)malloc(sizeof(void *) * (a + b));
	for (i = 0; i < a; i++)
	{
		ptr->input_pins[i] = node->input_pins[i+offa];
		ptr->input_pins[i]->node = ptr;
	}
	for (i = 0; i < b; i++)
	{
		ptr->input_pins[i+a] = node->input_pins[i + offa + a + offb];
		ptr->input_pins[i+a]->node = ptr;
	}

	/* Prep output pins for connecting to cascaded multipliers */
	ptr->num_output_pins = a + b;
	ptr->output_pins = (npin_t**)malloc(sizeof(void *) * (a + b));
	for (i = 0; i < a + b; i++)
		ptr->output_pins[i] = NULL;

	return;
}

/*-------------------------------------------------------------------------
 * (function: init_cascade_adder)
 *
 * This function is used to initialize an adder that is within
 *	a split multiplier.
 *-----------------------------------------------------------------------*/
void init_cascade_adder(nnode_t *node, nnode_t *a, int b)
{
	int i, size;

	node->type = ADD;
	node->related_ast_node = a->related_ast_node;
	node->traverse_visited = a->traverse_visited;
	node->node_data = NULL;

	/* Set size to be the maximum input size */
	size = a->output_port_sizes[0];
	size = (size < b) ? b : size;

	/* Set new port sizes and parameters */
	node->num_input_port_sizes = 2;
	node->input_port_sizes = (int *)malloc(2 * sizeof(int));
	node->input_port_sizes[0] = a->output_port_sizes[0];
	node->input_port_sizes[1] = b;
	node->num_output_port_sizes = 1;
	node->output_port_sizes = (int *)malloc(sizeof(int));
	node->output_port_sizes[0] = size;

	/* Set the number of input pins and clear pin entries */
	node->num_input_pins = a->output_port_sizes[0] + b;
	node->input_pins = (npin_t**)malloc(sizeof(void *) * 
		(a->output_port_sizes[0] + b));
	for (i = 0; i < a->output_port_sizes[0] + b; i++)
		node->input_pins[i] = NULL;

	/* Set the number of output pins and clear pin entries */
	node->num_output_pins = size;
	node->output_pins = (npin_t**)malloc(sizeof(void *) * size);
	for (i = 0; i < size; i++)
		node->output_pins[i] = NULL;

	add_list = insert_in_vptr_list(add_list, node);
	return;
}

/*-------------------------------------------------------------------------
 * (function: split_multiplier)
 *
 * This function works to split a multiplier into several smaller
 *  multipliers to better "fit" with the available resources in a
 *  targeted FPGA architecture.
 *
 * This function is at the lowest level since it simply receives
 *  a multiplier and is told how to split it. The end result is:
 *
 *  a1a0 * b1b0 => a0 * b0 + a0 * b1 + a1 * b0 + a1 * b1 => c1c0 => c
 *  
 * If we "balance" the additions, we can actually remove one of the
 * addition operations since we know that a0 * b0 and a1 * b1 will
 * not overlap in bits. This allows us to skip the addition between
 * these two terms and simply concat the results together. Giving us
 * the resulting logic:
 *
 * ((a1 * b1) . (a0 * b0)) + ((a0 * b1) + (a1 * b0)) ==> Result
 *
 * Note that for some of the additions we need to perform sign extensions,
 * but this should not be a problem since the sign extension is always
 * extending NOT contracting.
 *
 *-----------------------------------------------------------------------*/
void split_multiplier(nnode_t *node, int a0, int b0, int a1, int b1)
{
	nnode_t *a0b0, *a0b1, *a1b0, *a1b1, *addsmall, *addbig;
	int i, size;

	/* Check for a legitimate split */
	oassert(node->input_port_sizes[0] == (a0 + a1));
	oassert(node->input_port_sizes[1] == (b0 + b1));
	
	/* New node for small multiply */
	a0b0 = allocate_nnode();
	a0b0->name = (char *)malloc(strlen(node->name) + 3);
	strcpy(a0b0->name, node->name);
	strcat(a0b0->name, "-0");
	init_split_multiplier(node, a0b0, 0, a0, 0, b0);
	mult_list = insert_in_vptr_list(mult_list, a0b0);

	/* New node for big multiply */
	a1b1 = allocate_nnode();
	a1b1->name = (char *)malloc(strlen(node->name) + 3);
	strcpy(a1b1->name, node->name);
	strcat(a1b1->name, "-3");
	init_split_multiplier(node, a1b1, a0, a1, b0, b1);
	mult_list = insert_in_vptr_list(mult_list, a1b1);

	/* New node for 2nd multiply */
	a0b1 = allocate_nnode();
	a0b1->name = (char *)malloc(strlen(node->name) + 3);
	strcpy(a0b1->name, node->name);
	strcat(a0b1->name, "-1");
	init_split_multiplier(node, a0b1, 0, a0, b0, b1);
	mult_list = insert_in_vptr_list(mult_list, a0b1);

	/* New node for 3rd multiply */
	a1b0 = allocate_nnode();
	a1b0->name = (char *)malloc(strlen(node->name) + 3);
	strcpy(a1b0->name, node->name);
	strcat(a1b0->name, "-2");
	init_split_multiplier(node, a1b0, a0, a1, 0, b0);
	mult_list = insert_in_vptr_list(mult_list, a1b0);

	/* New node for the initial add */
	addsmall = allocate_nnode();
	addsmall->name = (char *)malloc(strlen(node->name) + 6);
	strcpy(addsmall->name, node->name);
	strcat(addsmall->name, "-add0");
	init_cascade_adder(addsmall, a1b0, a0b1->output_port_sizes[0]);
	
	/* New node for the BIG add */
	addbig = allocate_nnode();
	addbig->name = (char *)malloc(strlen(node->name) + 6);
	strcpy(addbig->name, node->name);
	strcat(addbig->name, "-add1");
	init_cascade_adder(addbig, addsmall,
		a0b0->output_port_sizes[0] + a1b1->output_port_sizes[0]);
	
	/* Insert temporary pins for addsmall */
	for (i = 0; i < a0b1->output_port_sizes[0]; i++)
		connect_nodes(a0b1, i, addsmall, i);
	for (i = 0; i < a1b0->output_port_sizes[0]; i++)
		connect_nodes(a1b0, i, addsmall, i+a0b1->output_port_sizes[0]);

	/* Insert temporary pins for addbig */
	size = addsmall->output_port_sizes[0];
	for (i = 0; i < size; i++)
		connect_nodes(addsmall, i, addbig, i);
	for (i = 0; i < a1b1->output_port_sizes[0]; i++)
		connect_nodes(a1b1, i, addbig, i + size);
	size = size + a1b1->output_port_sizes[0];
	for (i = 0; i < a0b0->output_port_sizes[0]; i++)
		connect_nodes(a0b0, i, addbig, i + size);

	/* Move original output pins for multiply to addbig */
	for (i = 0; i < addbig->num_output_pins; i++)
		remap_pin_to_new_node(node->output_pins[i], addbig, i);

	/* Probably more to do here in freeing the old node! */
	free(node->name);
	free(node->input_port_sizes);
	free(node->output_port_sizes);

	/* Free arrays NOT the pins since relocated! */
	free(node->input_pins); 
	free(node->output_pins); 
	free(node);

	return;
}

/*-------------------------------------------------------------------------
 * (function: split_multiplier_a)
 *
 * This function works to split the "a" input of a multiplier into 
 *  several smaller multipliers to better "fit" with the available 
 *  resources in a targeted FPGA architecture.
 *
 * This function is at the lowest level since it simply receives
 *  a multiplier and is told how to split it. The end result is:
 *
 *  a1a0 * b => a0 * b + a1 * b => c
 *  
 * Note that for the addition we need to perform sign extension,
 * but this should not be a problem since the sign extension is always
 * extending NOT contracting.
 *
 *-----------------------------------------------------------------------*/
void split_multiplier_a(nnode_t *node, int a0, int a1, int b)
{
	nnode_t *a0b, *a1b, *addsmall;
	int i;

	/* Check for a legitimate split */
	oassert(node->input_port_sizes[0] == (a0 + a1));
	oassert(node->input_port_sizes[1] == b);
	
	/* New node for a0b multiply */
	a0b = allocate_nnode();
	a0b->name = (char *)malloc(strlen(node->name) + 3);
	strcpy(a0b->name, node->name);
	strcat(a0b->name, "-0");
	init_split_multiplier(node, a0b, 0, a0, 0, b);
	mult_list = insert_in_vptr_list(mult_list, a0b);

	/* New node for a1b multiply */
	a1b = allocate_nnode();
	a1b->name = (char *)malloc(strlen(node->name) + 3);
	strcpy(a1b->name, node->name);
	strcat(a1b->name, "-1");
	init_split_multiplier(node, a1b, a0, a1, 0, b);
	mult_list = insert_in_vptr_list(mult_list, a1b);

	/* New node for the add */
	addsmall = allocate_nnode();
	addsmall->name = (char *)malloc(strlen(node->name) + 6);
	strcpy(addsmall->name, node->name);
	strcat(addsmall->name, "-add0");
	init_cascade_adder(addsmall, a1b, a1 + b);
	
	/* Connect pins for addsmall */
	for (i = a0; i < a0b->output_port_sizes[0]; i++)
		connect_nodes(a0b, i, addsmall, i-a0);
	for (i = a0b->output_port_sizes[0] - a0; i < a1+b; i++) /* Sign extend */
		connect_nodes(a0b, a0b->output_port_sizes[0]-1, addsmall, i);
	for (i = b+a1; i < (2 * (a1 + b)); i++)
		connect_nodes(a1b, i-(b+a1), addsmall, i);

	/* Move original output pins for multiply to new outputs */
	for (i = 0; i < a0; i++)
		remap_pin_to_new_node(node->output_pins[i], a0b, i);

	for (i = a0; i < node->num_output_pins; i++)
		remap_pin_to_new_node(node->output_pins[i], addsmall, i-a0);

	/* Probably more to do here in freeing the old node! */
	free(node->name);
	free(node->input_port_sizes);
	free(node->output_port_sizes);

	/* Free arrays NOT the pins since relocated! */
	free(node->input_pins); 
	free(node->output_pins); 
	free(node);
	return;
}

/*-------------------------------------------------------------------------
 * (function: split_multiplier_b)
 *
 * This function works to split the "b" input of a multiplier into 
 *  several smaller multipliers to better "fit" with the available 
 *  resources in a targeted FPGA architecture.
 *
 * This function is at the lowest level since it simply receives
 *  a multiplier and is told how to split it. The end result is:
 *
 *  a * b1b0 => a * b1 + a * b0 => c
 *  
 * Note that for the addition we need to perform sign extension,
 * but this should not be a problem since the sign extension is always
 * extending NOT contracting.
 *
 *-----------------------------------------------------------------------*/
void split_multiplier_b(nnode_t *node, int a, int b1, int b0)
{
	nnode_t *ab0, *ab1, *addsmall;
	int i;

	/* Check for a legitimate split */
	oassert(node->input_port_sizes[0] == a);
	oassert(node->input_port_sizes[1] == (b0 + b1));
	
	/* New node for ab0 multiply */
	ab0 = allocate_nnode();
	ab0->name = (char *)malloc(strlen(node->name) + 3);
	strcpy(ab0->name, node->name);
	strcat(ab0->name, "-0");
	init_split_multiplier(node, ab0, 0, a, 0, b0);
	mult_list = insert_in_vptr_list(mult_list, ab0);

	/* New node for ab1 multiply */
	ab1 = allocate_nnode();
	ab1->name = (char *)malloc(strlen(node->name) + 3);
	strcpy(ab1->name, node->name);
	strcat(ab1->name, "-1");
	init_split_multiplier(node, ab1, 0, a, b0, b1);
	mult_list = insert_in_vptr_list(mult_list, ab1);

	/* New node for the add */
	addsmall = allocate_nnode();
	addsmall->name = (char *)malloc(strlen(node->name) + 6);
	strcpy(addsmall->name, node->name);
	strcat(addsmall->name, "-add0");
	init_cascade_adder(addsmall, ab1, a + b1);
	
	/* Connect pins for addsmall */
	for (i = b0; i < ab0->output_port_sizes[0]; i++)
		connect_nodes(ab0, i, addsmall, i-b0);
	for (i = ab0->output_port_sizes[0] - b0; i < a+b1; i++) /* Sign extend */
		connect_nodes(ab0, ab0->output_port_sizes[0]-1, addsmall, i);
	for (i = b1+a; i < (2 * (a + b1)); i++)
		connect_nodes(ab1, i-(b1+a), addsmall, i);

	/* Move original output pins for multiply to new outputs */
	for (i = 0; i < b0; i++)
		remap_pin_to_new_node(node->output_pins[i], ab0, i);

	for (i = b0; i < node->num_output_pins; i++)
		remap_pin_to_new_node(node->output_pins[i], addsmall, i-b0);

	/* Probably more to do here in freeing the old node! */
	free(node->name);
	free(node->input_port_sizes);
	free(node->output_port_sizes);

	/* Free arrays NOT the pins since relocated! */
	free(node->input_pins); 
	free(node->output_pins); 
	free(node);
	return;
}

/*-------------------------------------------------------------------------
 * (function: pad_multiplier)
 *
 * Fill out a multiplier to a fixed size. Size is retrieved from global
 *	hard_multipliers data.
 *
 * NOTE: The inputs are extended based on multiplier padding setting.
 *-----------------------------------------------------------------------*/
void pad_multiplier(nnode_t *node, netlist_t *netlist)
{
	int diffa, diffb, diffout, i;
	int sizea, sizeb, sizeout;
	int ina, inb;

	int testa, testb;

	static int pad_pin_number = 0;

	oassert(node->type == MULTIPLY);
	oassert(hard_multipliers != NULL);

	sizea = node->input_port_sizes[0];
	sizeb = node->input_port_sizes[1];
	sizeout = node->output_port_sizes[0];
	record_mult_distribution(node);

	/* Calculate the BEST fit hard multiplier to use */
	ina = hard_multipliers->inputs->size;
	inb = hard_multipliers->inputs->next->size;
	if (ina < inb)
	{
		ina = hard_multipliers->inputs->next->size;
		inb = hard_multipliers->inputs->size;
	}
	diffa = ina - sizea;
	diffb = inb - sizeb;
	diffout = hard_multipliers->outputs->size - sizeout;

	if (configuration.split_hard_multiplier == 1)
	{
		struct s_linked_vptr *plist = hard_multipliers->pb_types;
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
				if (configuration.mult_padding == 0)
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
				if (configuration.mult_padding == 0)
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
			new_pin->name = append_string("", "unconnected_multiplier_output~%d", pad_pin_number++);
			add_output_pin_to_node(node, new_pin, i + sizeout);
		}
		node->output_port_sizes[0] = sizeout + diffout;
	}

	return;
}

/*-------------------------------------------------------------------------
 * (function: iterate_multipliers)
 *
 * This function will iterate over all of the multiply operations that
 *	exist in the netlist and perform a splitting so that they can
 *	fit into a basic hard multiplier block that exists on the FPGA.
 *	If the proper option is set, then it will be expanded as well
 *	to just use a fixed size hard multiplier.
 *-----------------------------------------------------------------------*/
void iterate_multipliers(netlist_t *netlist)
{
	int sizea, sizeb, swap;
	int mula, mulb;
	int a0, a1, b0, b1;
	nnode_t *node;

	/* Can only perform the optimisation if hard multipliers exist! */
	if (hard_multipliers == NULL)
		return;

	sizea = hard_multipliers->inputs->size;
	sizeb = hard_multipliers->inputs->next->size;
	if (sizea < sizeb)
	{
		swap = sizea;
		sizea = sizeb;
		sizeb = swap;
	}

	while (mult_list != NULL)
	{
		node = (nnode_t *)mult_list->data_vptr;
		mult_list = delete_in_vptr_list(mult_list);

		oassert(node != NULL);

		if (node->type == HARD_IP)
			node->type = MULTIPLY;

		oassert(node->type == MULTIPLY);

		mula = node->input_port_sizes[0];
		mulb = node->input_port_sizes[1];
		if (mula < mulb)
		{
			swap = sizea;
			sizea = sizeb;
			sizeb = swap;
		}

		/* Do I need to split the multiplier on both inputs? */
		if ((mula > sizea) && (mulb > sizeb))
		{
			a0 = sizea;
			a1 = mula - sizea;
			b0 = sizeb;
			b1 = mulb - sizeb;
			split_multiplier(node, a0, b0, a1, b1);
		}
		else if (mula > sizea) /* split multiplier on a input? */
		{
			a0 = sizea;
			a1 = mula - sizea;
			split_multiplier_a(node, a0, a1, mulb);
		}
		else if (mulb > sizeb) /* split multiplier on b input? */
		{
			b1 = sizeb;
			b0 = mulb - sizeb;
			split_multiplier_b(node, mula, b0, b1);
		}
		else if ((sizea >= min_mult) && (sizeb >= min_mult))
		{
			/* Check to ensure IF mult needs to be exact size */
			if(configuration.fixed_hard_multiplier != 0)
				pad_multiplier(node, netlist); 
		}
	}
	return;
}

/*-------------------------------------------------------------------------
 * (function: clean_multipliers)
 *
 * Clean up the memory by deleting the list structure of multipliers
 *	during optimization
 *-----------------------------------------------------------------------*/
void clean_multipliers()
{
	while (mult_list != NULL)
		mult_list = delete_in_vptr_list(mult_list);
	return;
}

#endif
