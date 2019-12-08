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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <algorithm>
#include "odin_types.h"
#include "odin_globals.h"

#include "netlist_utils.h"
#include "node_creation_library.h"
#include "odin_util.h"

#include "partial_map.h"
#include "multipliers.h"
#include "hard_blocks.h"
#include "math.h"
#include "memories.h"
#include "adders.h"
#include "subtractions.h"
#include "vtr_memory.h"
#include "vtr_util.h"

// MEHRSHAD //
adder_t **adders_list;
short *chromosome;
short *fittest;
int chromosome_fitness;

npin_t **cloud_pins_list;
nnet_t  **cloud_nets_list;
nnode_t **cloud_nodes_list;

int num_cloud_pins;
int num_cloud_nets;
int num_cloud_nodes;

long num_of_adders = 0;
// MEHRSHAD //

void depth_first_traversal_to_partial_map(short marker_value, netlist_t *netlist);
void depth_first_traverse_partial_map(nnode_t *node, int traverse_mark_number, netlist_t *netlist);

void partial_map_node(nnode_t *node, short traverse_number, netlist_t *netlist);

void instantiate_not_logic(nnode_t *node, short mark, netlist_t *netlist);
void instantiate_buffer(nnode_t *node, short mark, netlist_t *netlist);
void instantiate_bitwise_logic(nnode_t *node, operation_list op, short mark, netlist_t *netlist);
void instantiate_bitwise_reduction(nnode_t *node, operation_list op, short mark, netlist_t *netlist);
void instantiate_logical_logic(nnode_t *node, operation_list op, short mark, netlist_t *netlist);
void instantiate_EQUAL(nnode_t *node, operation_list type, short mark, netlist_t *netlist);
void instantiate_GE(nnode_t *node, operation_list type, short mark, netlist_t *netlist);
void instantiate_GT(nnode_t *node, operation_list type, short mark, netlist_t *netlist);
void instantiate_shift_left_or_right(nnode_t *node, operation_list type, short mark, netlist_t *netlist);
void instantiate_arithmetic_shift_right(nnode_t *node, short mark, netlist_t *netlist);
void instantiate_unary_sub(nnode_t *node, short mark, netlist_t *netlist);
void instantiate_sub_w_carry(nnode_t *node, short mark, netlist_t *netlist);

void instantiate_soft_logic_ram(nnode_t *node, short mark, netlist_t *netlist);

/*-------------------------------------------------------------------------
 * (function: partial_map_top)
 *-----------------------------------------------------------------------*/
void partial_map_top(netlist_t *netlist)
{
	/* depending on the output target choose how to do partial mapping */
	if (strcmp(configuration.output_type.c_str(), "blif") == 0)
	{
		/* do the partial map without any larger structures identified */
		depth_first_traversal_to_partial_map(PARTIAL_MAP_TRAVERSE_VALUE, netlist);
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traversal_to_parital_map()
 *-------------------------------------------------------------------------------------------*/
void depth_first_traversal_to_partial_map(short marker_value, netlist_t *netlist)
{
	int i;

	/* start with the primary input list */
	for (i = 0; i < netlist->num_top_input_nodes; i++)
	{
		if (netlist->top_input_nodes[i] != NULL)
		{
			depth_first_traverse_partial_map(netlist->top_input_nodes[i], marker_value, netlist);
		}
	}
	/* now traverse the ground and vcc pins  */
	depth_first_traverse_partial_map(netlist->gnd_node, marker_value, netlist);
	depth_first_traverse_partial_map(netlist->vcc_node, marker_value, netlist);
	depth_first_traverse_partial_map(netlist->pad_node, marker_value, netlist);
}

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traverse)
 *-------------------------------------------------------------------------------------------*/
void depth_first_traverse_partial_map(nnode_t *node, int traverse_mark_number, netlist_t *netlist)
{
	int i, j;

	if (node->traverse_visited != traverse_mark_number)
	{
		/*this is a new node so depth visit it */

		/* mark that we have visitied this node now */
		node->traverse_visited = traverse_mark_number;

		for (i = 0; i < node->num_output_pins; i++)
		{
			if (node->output_pins[i]->net)
			{
				nnet_t *next_net = node->output_pins[i]->net;
				if(next_net->fanout_pins)
				{
					for (j = 0; j < next_net->num_fanout_pins; j++)
					{
						if (next_net->fanout_pins[j])
						{
							if (next_net->fanout_pins[j]->node)
							{
							/* recursive call point */
								depth_first_traverse_partial_map(next_net->fanout_pins[j]->node, traverse_mark_number, netlist);
							}
						}
					}
				}
			}
		}

		/* POST traverse  map the node since you might delete */
		partial_map_node(node, traverse_mark_number, netlist);
	}
}

/*----------------------------------------------------------------------
 * (function: partial_map_node)
 *--------------------------------------------------------------------*/
void partial_map_node(nnode_t *node, short traverse_number, netlist_t *netlist)
{
	switch (node->type)
	{
		case BITWISE_NOT:
			instantiate_not_logic(node, traverse_number, netlist);
			break;
		case BUF_NODE:
			instantiate_buffer(node, traverse_number, netlist);
			break;

		case BITWISE_AND:
		case BITWISE_OR:
		case BITWISE_NAND:
		case BITWISE_NOR:
		case BITWISE_XNOR:
		case BITWISE_XOR:
			if (node->num_input_port_sizes >= 2)
			{
				instantiate_bitwise_logic(node, node->type, traverse_number, netlist);
			}
			else if (node->num_input_port_sizes == 1)
			{
				instantiate_bitwise_reduction(node, node->type, traverse_number, netlist);
			}
			else
				oassert(false);
			break;

		case LOGICAL_OR:
		case LOGICAL_AND:
		case LOGICAL_NOR:
		case LOGICAL_NAND:
		case LOGICAL_XOR:
		case LOGICAL_XNOR:
			if (node->num_input_port_sizes == 2)
			{
				instantiate_logical_logic(node, node->type, traverse_number, netlist);
			}
			break;

		case LOGICAL_NOT:
			/* don't need to do anything since this is a reduction */
			break;

		case ADD:
			if (hard_adders && node->bit_width >= min_threshold_adder){
				// Check if the size of this adder is greater than the hard vs soft logic threshold
					instantiate_hard_adder(node, traverse_number, netlist);
			}else{
				//MEHRSHAD//
				// instantiate_add_w_carry(node, traverse_number, netlist);
				adders_list = (adder_t **)vtr::realloc (adders_list, sizeof(adder_t*)*(num_of_adders+1));
				chromosome = (short *)vtr::realloc (chromosome, sizeof(short)*(num_of_adders+1));
				adders_list[num_of_adders] = (adder_t *)vtr::malloc(sizeof(adder_t));

				//set the variable of the the adder_list array
				adders_list[num_of_adders]->node = node;

				adders_list[num_of_adders]->input->pins = node->input_pins;
				adders_list[num_of_adders]->input->count = node->num_input_pins;
				adders_list[num_of_adders]->input->is_adder = 'y';
				
				adders_list[num_of_adders]->output->pins = node->output_pins;
				adders_list[num_of_adders]->output->count = node->num_output_pins;
				adders_list[num_of_adders]->output->is_adder = 'y';

				chromosome[num_of_adders] = adders_list[num_of_adders]->type;
				num_of_adders++;
				//MEHRSHAD//

			}
			break;
		case MINUS:
			if (hard_adders)
			{
				if(node->num_input_port_sizes == 3)
				{
					int max_num = (node->input_port_sizes[0] >= node->input_port_sizes[1])? node->input_port_sizes[0] : node->input_port_sizes[1];
					if (max_num >= min_add)
						instantiate_hard_adder_subtraction(node, traverse_number, netlist);
					else
						instantiate_add_w_carry(DEFAULT, node, traverse_number, netlist);
				}
				else if (node->num_input_port_sizes == 2)
				{
					instantiate_sub_w_carry(node, traverse_number, netlist);
				}
				else if (node->num_input_port_sizes == 1)
				{
					instantiate_unary_sub(node, traverse_number, netlist);
				}
				else
					oassert(false);
			}
			else{
				if (node->num_input_port_sizes == 2)
				{
					instantiate_sub_w_carry(node, traverse_number, netlist);
				}
				else if (node->num_input_port_sizes == 1)
				{
					instantiate_unary_sub(node, traverse_number, netlist);
				}
				else
					oassert(false);
			}

			break;
		case LOGICAL_EQUAL:
		case NOT_EQUAL:
			instantiate_EQUAL(node, node->type, traverse_number, netlist);
			break;
		case GTE:
		case LTE:
			instantiate_GE(node, node->type, traverse_number, netlist);
			break;
		case GT:
		case LT:
			instantiate_GT(node, node->type, traverse_number, netlist);
			break;
		case SL:
		case SR:
			instantiate_shift_left_or_right(node, node->type, traverse_number, netlist);
			break;
        case ASR:
            instantiate_arithmetic_shift_right(node, node->type, netlist);
            break;
        case MULTI_PORT_MUX:
			instantiate_multi_port_mux(node, traverse_number, netlist);
			break;
		case MULTIPLY:
        {
            int mult_size = std::max<int>(node->input_port_sizes[0], node->input_port_sizes[1]);
			if (hard_multipliers && mult_size >= min_mult) {
                instantiate_hard_multiplier(node, traverse_number, netlist);
            } else if (!hard_adders) {
				instantiate_simple_soft_multiplier(node, traverse_number, netlist);
			}
			break;
        }
		case MEMORY:
		{
			ast_node_t *ast_node = node->related_ast_node;
			char *identifier = ast_node->children[0]->types.identifier;
			if (find_hard_block(identifier))
			{
				long depth = is_sp_ram(node)? get_sp_ram_depth(node) : get_dp_ram_depth(node);
				long width = is_sp_ram(node)? get_sp_ram_width(node) : get_dp_ram_width(node);

				// If the memory satisfies the threshold for the use of a hard logic block, use one.
				if (depth > configuration.soft_logic_memory_depth_threshold || width > configuration.soft_logic_memory_width_threshold)
				{
					instantiate_hard_block(node, traverse_number, netlist);
				}
				else
				{
					printf("\tInferring soft logic ram: %zux%zu\n", width, depth);
					instantiate_soft_logic_ram(node, traverse_number, netlist);
				}
			}
			else
			{
				instantiate_soft_logic_ram(node, traverse_number, netlist);
			}
			break;
		}
		case HARD_IP:
			instantiate_hard_block(node, traverse_number, netlist);

			break;
		case ADDER_FUNC:
		case CARRY_FUNC:
		case MUX_2:
		case INPUT_NODE:
		case CLOCK_NODE:
		case OUTPUT_NODE:
		case GND_NODE:
		case VCC_NODE:
		case FF_NODE:
		case PAD_NODE:
			/* some nodes already in the form that is mapable */
			break;
		case CASE_EQUAL:
		case CASE_NOT_EQUAL:
		case DIVIDE:
		case MODULO:
		default:
			error_message(NETLIST_ERROR, 0, -1, "%s", "Partial map: node should have been converted to softer version.");
			break;
	}
}

void instantiate_soft_logic_ram(nnode_t *node, short mark, netlist_t *netlist)
{
	if (is_sp_ram(node))
		instantiate_soft_single_port_ram(node, mark, netlist);
	else if (is_dp_ram(node))
		instantiate_soft_dual_port_ram(node, mark, netlist);
	else
		oassert(false);
}


/*---------------------------------------------------------------------------------------------
 * (function: instantiate_multi_port_mux )
 * 	Makes the multiport into a series of 2-Mux-decoded
 *-------------------------------------------------------------------------------------------*/
void instantiate_multi_port_mux(nnode_t *node, short mark, netlist_t * /*netlist*/)
{
	int i, j;
	int width_of_one_hot_logic;
	int num_ports;
	int port_offset;
	nnode_t **muxes;

	/* setup the calculations for padding and indexing */
	width_of_one_hot_logic = node->input_port_sizes[0];
	num_ports = node->num_input_port_sizes;
	port_offset = node->input_port_sizes[1];

	muxes = (nnode_t**)vtr::malloc(sizeof(nnode_t*)*(num_ports-1));
	for(i = 0; i < num_ports-1; i++)
	{
		muxes[i] = make_2port_gate(MUX_2, width_of_one_hot_logic, width_of_one_hot_logic, 1, node, mark);
	}

	for(j = 0; j < num_ports - 1; j++)
	{
		for(i = 0; i < width_of_one_hot_logic; i++)
		{
			/* map the inputs to the muxt */
			remap_pin_to_new_node(node->input_pins[i+(j+1)*port_offset], muxes[j], width_of_one_hot_logic+i);

			/* map the one hot logic control */
			if (j == 0)
				remap_pin_to_new_node(node->input_pins[i], muxes[j], i);
			else
				add_input_pin_to_node(muxes[j], copy_input_npin(muxes[0]->input_pins[i]), i);
		}

		/* now hookup outputs */
		remap_pin_to_new_node(node->output_pins[j], muxes[j], 0);
	}
	vtr::free(muxes);
	free_nnode(node);
}

/*---------------------------------------------------------------------------------------------
 * (function: instantiate_not_logic )
 *-------------------------------------------------------------------------------------------*/
void instantiate_not_logic(nnode_t *node, short mark, netlist_t * /*netlist*/)
{
	int width = node->num_input_pins;
	nnode_t **new_not_cells;
	int i;

	new_not_cells = (nnode_t**)vtr::malloc(sizeof(nnode_t*)*width);

	for (i = 0; i < width; i++)
	{
		new_not_cells[i] = make_not_gate(node, mark);
	}

	/* connect inputs and outputs */
	for(i = 0; i < width; i++)
	{
		/* Joining the inputs to the new soft NOT GATES */
		remap_pin_to_new_node(node->input_pins[i], new_not_cells[i], 0);
		remap_pin_to_new_node(node->output_pins[i], new_not_cells[i], 0);
	}

	vtr::free(new_not_cells);
	free_nnode(node);
}

/*---------------------------------------------------------------------------------------------
 * (function: instantiate_buffer )
 * 	Buffers just pass through signals
 *-------------------------------------------------------------------------------------------*/
void instantiate_buffer(nnode_t *node, short /*mark*/, netlist_t * /*netlist*/)
{
	int width = node->num_input_pins;
	int i;

	/* for now we just pass the signals directly through */
	for (i = 0; i < width; i++)
	{
		int idx_2_buffer = node->input_pins[i]->pin_net_idx;

		/* join all fanouts of the output net with the input pins net */
		join_nets(node->input_pins[i]->net, node->output_pins[i]->net);

		/* erase the pointer to this buffer */
		node->input_pins[i]->net->fanout_pins[idx_2_buffer] = NULL;
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: instantiate_logical_logic )
 *-------------------------------------------------------------------------------------------*/
void instantiate_logical_logic(nnode_t *node, operation_list op, short mark, netlist_t *netlist)
{
	int i;
	int port_B_offset;
	int width_a;
	int width_b;
	nnode_t *new_logic_cell;
	nnode_t *reduction1;
	nnode_t *reduction2;

	oassert(node->num_input_pins > 0);
	oassert(node->num_input_port_sizes == 2);
	oassert(node->num_output_pins == 1);
	/* setup the calculations for padding and indexing */
	width_a = node->input_port_sizes[0];
	width_b = node->input_port_sizes[1];
	port_B_offset = width_a;

	/* instantiate the cells */
	new_logic_cell = make_1port_logic_gate(op, 2, node, mark);
	reduction1 = make_1port_logic_gate(BITWISE_OR, width_a, node, mark);
	reduction2 = make_1port_logic_gate(BITWISE_OR, width_b, node, mark);

	/* connect inputs.  In the case that a signal is smaller than the other then zero pad */
	for(i = 0; i < width_a; i++)
	{
		/* Joining the inputs to the input 1 of that gate */
		if (i < width_a)
		{
			remap_pin_to_new_node(node->input_pins[i], reduction1, i);
		}
		else
		{
			/* ELSE - the B input does not exist, so this answer goes right through */
			add_input_pin_to_node(reduction1, get_zero_pin(netlist), i);
		}
	}
	for(i = 0; i < width_b; i++)
	{
		/* Joining the inputs to the input 1 of that gate */
		if (i < width_b)
		{
			remap_pin_to_new_node(node->input_pins[i+port_B_offset], reduction2, i);
		}
		else
		{
			/* ELSE - the B input does not exist, so this answer goes right through */
			add_input_pin_to_node(reduction2, get_zero_pin(netlist), i);
		}
	}

	connect_nodes(reduction1, 0, new_logic_cell, 0);
	connect_nodes(reduction2, 0, new_logic_cell, 1);

	instantiate_bitwise_reduction(reduction1, BITWISE_OR, mark, netlist);
	instantiate_bitwise_reduction(reduction2, BITWISE_OR, mark, netlist);

	remap_pin_to_new_node(node->output_pins[0], new_logic_cell, 0);
	free_nnode(node);
}
/*---------------------------------------------------------------------------------------------
 * (function: instantiate_bitwise_reduction )
 * 	Makes 2 input gates to break into bitwise
 *-------------------------------------------------------------------------------------------*/
void instantiate_bitwise_reduction(nnode_t *node, operation_list op, short mark, netlist_t *netlist)
{
	int i;
	int width_a;
	nnode_t *new_logic_cell;
	operation_list cell_op;

	oassert(node->num_input_pins > 0);
	oassert(node->num_input_port_sizes == 1);
	oassert(node->output_port_sizes[0] == 1);
	/* setup the calculations for padding and indexing */
	width_a = node->input_port_sizes[0];

	switch (op)
	{
		case BITWISE_AND:
		case LOGICAL_AND:
			cell_op = LOGICAL_AND;
			break;
		case BITWISE_OR:
		case LOGICAL_OR:
			cell_op = LOGICAL_OR;
			break;
		case BITWISE_NAND:
		case LOGICAL_NAND:
			cell_op = LOGICAL_NAND;
			break;
		case BITWISE_NOR:
		case LOGICAL_NOR:
			cell_op = LOGICAL_NOR;
			break;
		case BITWISE_XNOR:
		case LOGICAL_XNOR:
			cell_op = LOGICAL_XNOR;
			break;
		case BITWISE_XOR:
		case LOGICAL_XOR:
			cell_op = LOGICAL_XOR;
			break;
		default:
			cell_op = NO_OP;
			oassert(false);
			break;
	}
	/* instantiate the cells */
	new_logic_cell = make_1port_logic_gate(cell_op, width_a, node, mark);

	/* connect inputs.  In the case that a signal is smaller than the other then zero pad */
	for(i = 0; i < width_a; i++)
	{
		/* Joining the inputs to the input 1 of that gate */
		if (i < width_a)
		{
			remap_pin_to_new_node(node->input_pins[i], new_logic_cell, i);
		}
		else
		{
			/* ELSE - the B input does not exist, so this answer goes right through */
			add_input_pin_to_node(new_logic_cell, get_zero_pin(netlist), i);
		}
	}

	remap_pin_to_new_node(node->output_pins[0], new_logic_cell, 0);
	free_nnode(node);
}

/*---------------------------------------------------------------------------------------------
 * (function: instantiate_bitwise_logic )
 * 	Makes 2 input gates to break into bitwise
 *-------------------------------------------------------------------------------------------*/
void instantiate_bitwise_logic(nnode_t *node, operation_list op, short mark, netlist_t *netlist)
{
	int i, j;

	operation_list cell_op;
	if (!node) return;
	oassert(node->num_input_pins > 0);
	oassert(node->num_input_port_sizes >= 2);

	switch (op)
	{
		case BITWISE_AND:
			cell_op = LOGICAL_AND;
			break;
		case BITWISE_OR:
			cell_op = LOGICAL_OR;
			break;
		case BITWISE_NAND:
			cell_op = LOGICAL_NAND;
			break;
		case BITWISE_NOR:
			cell_op = LOGICAL_NOR;
			break;
		case BITWISE_XNOR:
			cell_op = LOGICAL_XNOR;
			break;
		case BITWISE_XOR:
			cell_op = LOGICAL_XOR;
			break;
		default:
			cell_op = NO_OP;
			oassert(false);
			break;
	}

	/* connect inputs.  In the case that a signal is smaller than the other then zero pad */
	for(i = 0; i < node->output_port_sizes[0]; i++)
	{
		nnode_t *new_logic_cells = make_nport_gate(cell_op, node->num_input_port_sizes, 1, 1, node, mark);
		int current_port_offset = 0;
		/* Joining the inputs to the input 1 of that gate */
		for(j = 0; j < node->num_input_port_sizes; j++)
		{
			/* IF - this current input will also have a corresponding other input ports then join it to the gate */
			if(i < node->input_port_sizes[j])
				remap_pin_to_new_node(node->input_pins[i+current_port_offset], new_logic_cells, j);

			/* ELSE - the input does not exist, so this answer goes right through */
			else
				add_input_pin_to_node(new_logic_cells, get_zero_pin(netlist), j);

			current_port_offset += node->input_port_sizes[j];
        }

		remap_pin_to_new_node(node->output_pins[i], new_logic_cells, 0);
	}

	free_nnode(node);
}

/*--------------------------------------------------------------------------
 * (function: instantiate_add_w_carry )
 * 	This is for soft addition in output formats that don't handle
 *	multi-output logic functions (BLIF).  We use one function for the
 *	add, and one for the carry.
 *------------------------------------------------------------------------*/
void instantiate_add_w_carry(short type, nnode_t *node, short mark, netlist_t *netlist)
{
		// define locations in array when fetching pins
	const int out = 0, input_a = 1, input_b = 2, pinout_count = 3;

	oassert(node->num_input_pins > 0);

	int *width = (int*)vtr::malloc(pinout_count * sizeof(int));

	if(node->num_input_port_sizes == 2)
		width[out] = node->output_port_sizes[0];
	else
		width[out] = node->num_output_pins;

	width[input_a] = node->input_port_sizes[0];
	width[input_b] = node->input_port_sizes[1];

	instantiate_add_w_carry_block(type, width, node, mark, netlist, 0);

	vtr::free(width);
}

/*---------------------------------------------------------------------------------------------
 * (function: instantiate_sub_w_carry )
 * 	This subtraction is intended for sof subtraction with output formats that can't handle
 * 	multi output logic functions.  We split the add and the carry over two logic functions.
 *-------------------------------------------------------------------------------------------*/
void instantiate_sub_w_carry(nnode_t *node, short mark, netlist_t *netlist)
{
		// define locations in array when fetching pins
	const int out = 0, input_a = 1, input_b = 2, pinout_count = 3;

	oassert(node->num_input_pins > 0);

	int *width = (int*)vtr::malloc(pinout_count * sizeof(int));
	width[out] = node->output_port_sizes[0];

	if(node->num_input_port_sizes == 1)
	{
		width[input_a] = 0;
		width[input_b] = node->input_port_sizes[0];
	}
	else if(node->num_input_port_sizes == 2)
	{
		width[input_a] = node->input_port_sizes[0];
		width[input_b] = node->input_port_sizes[1];
	}

	instantiate_add_w_carry_block(DEFAULT, width, node, mark, netlist, 1);

	vtr::free(width);
}

/*---------------------------------------------------------------------------------------------
 * (function:  instantiate_unary_sub )
 *	Does 2's complement which is the equivalent of a unary subtraction as a HW implementation.
 *-------------------------------------------------------------------------------------------*/
void instantiate_unary_sub(nnode_t *node, short mark, netlist_t *netlist)
{
	instantiate_sub_w_carry(node, mark, netlist);
}


/*---------------------------------------------------------------------------------------------
 * (function: instantiate_EQUAL )
 *	Builds the hardware for an equal comparison by building EQ for parallel lines and then
 *	taking them all through an AND tree.
 *-------------------------------------------------------------------------------------------*/
void instantiate_EQUAL(nnode_t *node, operation_list type, short mark, netlist_t *netlist)
{
	int width_a;
	int width_b;
	int width_max;
	int i;
	int port_B_offset;
	nnode_t *compare;
	nnode_t *combine;

	oassert(node->num_output_pins == 1);
	oassert(node->num_input_pins > 0);
	oassert(node->num_input_port_sizes == 2);
	width_a = node->input_port_sizes[0];
	width_b = node->input_port_sizes[1];
	width_max = width_a > width_b ? width_a : width_b;
	port_B_offset = width_a;

	/* build an xnor bitwise XNOR */
	if (type == LOGICAL_EQUAL)
	{
		compare = make_2port_gate(LOGICAL_XNOR, width_a, width_b, width_max, node, mark);
		combine = make_1port_logic_gate(LOGICAL_AND, width_max, node, mark);
	}
	else
	{
		compare = make_2port_gate(LOGICAL_XOR, width_a, width_b, width_max, node, mark);
		combine = make_1port_logic_gate(LOGICAL_OR, width_max, node, mark);
	}
	/* build an and bitwise AND */


	/* connect inputs.  In the case that a signal is smaller than the other then zero pad */
	for(i = 0; i < width_max; i++)
	{
		/* Joining the inputs to the input 1 of that gate */
		if (i < width_a)
		{
			if (i < width_b)
			{
				/* IF - this current input will also have a corresponding b_port input then join it to the gate */
				remap_pin_to_new_node(node->input_pins[i], compare, i);
			}
			else
			{
				/* ELSE - the B input does not exist, so this answer goes right through */
				add_input_pin_to_node(compare, get_zero_pin(netlist), i);
			}
		}

		if (i < width_b)
		{
			if (i < width_a)
			{
				/* IF - this current input will also have a corresponding a_port input then join it to the gate */
				/* Joining the inputs to the input 2 of that gate */
				remap_pin_to_new_node(node->input_pins[i+port_B_offset], compare, i+port_B_offset);
			}
			else
			{
				/* ELSE - the A input does not exist, so this answer goes right through */
				add_input_pin_to_node(compare, get_zero_pin(netlist), i+port_B_offset);
			}
		}

		/* hook it up to the logcial AND */
		connect_nodes(compare, i, combine, i);
	}

	/* join that gate to the output */
	remap_pin_to_new_node(node->output_pins[0], combine, 0);

	if (type == LOGICAL_EQUAL)
		instantiate_bitwise_logic(compare, BITWISE_XNOR, mark, netlist);
	else
		instantiate_bitwise_logic(compare, BITWISE_XOR, mark, netlist);
	/* Don't need to instantiate a Logic and gate since it is a function itself */

	oassert(combine->num_output_pins == 1);

	free_nnode(node);
}

/*---------------------------------------------------------------------------------------------
 * (function: instantiate_GT )
 *	Defines the HW needed for greter than equal with EQ, GT, AND and OR gates to create
 *	the appropriate logic function.
 *-------------------------------------------------------------------------------------------*/
void instantiate_GT(nnode_t *node, operation_list type, short mark, netlist_t *netlist)
{
	int width_a;
	int width_b;
	int width_max;
	int i;
	int port_A_offset;
	int port_B_offset;
	int port_A_index;
	int port_B_index;
	int index = 0;
	nnode_t *xor_gate=NULL;
	nnode_t *logical_or_gate;
	nnode_t **or_cells;
	nnode_t **gt_cells;

	oassert(node->num_output_pins == 1);
	oassert(node->num_input_pins > 0);
	oassert(node->num_input_port_sizes == 2);
	oassert(node->input_port_sizes[0] == node->input_port_sizes[1]);
	width_a = node->input_port_sizes[0];
	width_b = node->input_port_sizes[1];
	width_max = width_a > width_b ? width_a : width_b;

	/* swaps ports A and B */
	if (type == GT)
	{
		port_A_offset = 0;
		port_B_offset = width_a;
		port_A_index = 0;
		port_B_index = width_a-1;
	}
	else if (type == LT)
	{
		port_A_offset = width_b;
		port_B_offset = 0;
		port_A_index = width_b-1;
		port_B_index = 0;
	}
	else
	{
		port_A_offset = 0;
		port_B_offset = 0;
		port_A_index = 0;
		port_B_index = 0;
		error_message(NETLIST_ERROR, node->related_ast_node->line_number, node->related_ast_node->file_number, "Invalid node type %s in instantiate_GT\n",
			node_name_based_on_op(node));
	}

	if (width_max>1)
	{
		/* xor gate identifies if any bits don't match */
		xor_gate = make_2port_gate(LOGICAL_XOR, width_a-1, width_b-1, width_max-1, node, mark);
	}

	/* collects all the GT signals and determines if gt */
	logical_or_gate = make_1port_logic_gate(LOGICAL_OR, width_max, node, mark);
	/* collects a chain if any 1 happens than the GT cells output 0 */
	or_cells = (nnode_t**)vtr::malloc(sizeof(nnode_t*)*width_max-1);
	/* each cell checks if A > B and sends out a 1 if history has no 1s (3rd input) */
	gt_cells = (nnode_t**)vtr::malloc(sizeof(nnode_t*)*width_max);

	for (i = 0; i < width_max; i++)
	{
		gt_cells[i] = make_3port_gate(GT, 1, 1, 1, 1, node, mark);
		if (i < width_max-1)
		{
			or_cells[i] = make_2port_gate(LOGICAL_OR, 1, 1, 1, node, mark);
		}
	}

	/* connect inputs.  In the case that a signal is smaller than the other then zero pad */
	for(i = 0; i < width_max; i++)
	{
		/* Joining the inputs to the input 1 of that gate */
		if (i < width_a)
		{
			/* IF - this current input will also have a corresponding b_port input then join it to the gate */
			remap_pin_to_new_node(node->input_pins[i+port_A_offset], gt_cells[i], 0);
			if (i > 0)
				add_input_pin_to_node(xor_gate, copy_input_npin(gt_cells[i]->input_pins[0]), index+port_A_index);
		}
		else
		{
			/* ELSE - the B input does not exist, so this answer goes right through */
			add_input_pin_to_node(gt_cells[i], get_zero_pin(netlist), 0);
			if (i > 0)
				add_input_pin_to_node(xor_gate, get_zero_pin(netlist), index+port_A_index);
		}

		if (i < width_b)
		{
			/* IF - this current input will also have a corresponding a_port input then join it to the gate */
			/* Joining the inputs to the input 2 of that gate */
			remap_pin_to_new_node(node->input_pins[i+port_B_offset], gt_cells[i], 1);
			if (i > 0)
				add_input_pin_to_node(xor_gate, copy_input_npin(gt_cells[i]->input_pins[1]), index+port_B_index);
		}
		else
		{
			/* ELSE - the A input does not exist, so this answer goes right through */
			add_input_pin_to_node(gt_cells[i], get_zero_pin(netlist), 1);
			if (i > 0)
				add_input_pin_to_node(xor_gate, get_zero_pin(netlist), index+port_B_index);
		}

		if (i < width_max-1)
		{
			/* number of OR gates */
			if (i < width_max-2)
			{
				/* connect the msb or to the next lower bit */
				connect_nodes(or_cells[i+1], 0, or_cells[i], 1);
			}
			else
			{
				/* deal with the first greater than test which autom gets a zero */
				add_input_pin_to_node(or_cells[i], get_zero_pin(netlist), 1);
			}
			if (width_max>1)
			{
				/* get all the equals with the or gates */
				connect_nodes(xor_gate, i, or_cells[i], 0);
			}

			connect_nodes(or_cells[i], 0, gt_cells[i], 2);

		}
		else
		{
			/* deal with the first greater than test which autom gets a zero */
			add_input_pin_to_node(gt_cells[i], get_zero_pin(netlist), 2);
		}

		/* hook it up to the logcial AND */
		connect_nodes(gt_cells[i], 0, logical_or_gate, i);

		if (i > 0)
		{
			index++;
		}
	}

	/* join that gate to the output */
	remap_pin_to_new_node(node->output_pins[0], logical_or_gate, 0);
	oassert(logical_or_gate->num_output_pins == 1);
	if (xor_gate!= NULL)
	{
		instantiate_bitwise_logic(xor_gate, BITWISE_XOR, mark, netlist);
	}
	
	vtr::free(gt_cells);
	vtr::free(or_cells);
	free_nnode(node);
}

/*---------------------------------------------------------------------------------------------
 * (function: instantiate_GE )
 *	Defines the HW needed for greter than equal with EQ, GT, AND and OR gates to create
 *	the appropriate logic function.
 *-------------------------------------------------------------------------------------------*/
void instantiate_GE(nnode_t *node, operation_list type, short mark, netlist_t *netlist)
{
	int width_a;
	int width_b;
	int width_max;
	int i;
	int port_B_offset;
	int port_A_offset;
	nnode_t *equal;
	nnode_t *compare;
	nnode_t *logical_or_final_gate;

	oassert(node->num_output_pins == 1);
	oassert(node->num_input_pins > 0);
	oassert(node->num_input_port_sizes == 2);
	oassert(node->input_port_sizes[0] == node->input_port_sizes[1]);
	width_a = node->input_port_sizes[0];
	width_b = node->input_port_sizes[1];
	oassert(width_a == width_b);
	width_max = width_a > width_b ? width_a : width_b;

	port_A_offset = 0;
	port_B_offset = width_a;

	/* build an xnor bitwise XNOR */
	equal = make_2port_gate(LOGICAL_EQUAL, width_a, width_b, 1, node, mark);

	if (type == GTE) compare = make_2port_gate(GT, width_a, width_b, 1, node, mark);
	else             compare = make_2port_gate(LT, width_a, width_b, 1, node, mark);

	logical_or_final_gate = make_1port_logic_gate(LOGICAL_OR, 2, node, mark);

	/* connect inputs.  In the case that a signal is smaller than the other then zero pad */
	for(i = 0; i < width_max; i++)
	{
		/* Joining the inputs to the input 1 of that gate */
		if (i < width_a)
		{
			/* IF - this current input will also have a corresponding b_port input then join it to the gate */
			remap_pin_to_new_node(node->input_pins[i+port_A_offset], equal, i+port_A_offset);
			add_input_pin_to_node(compare, copy_input_npin(equal->input_pins[i+port_A_offset]), i+port_A_offset);
		}
		else
		{
			/* ELSE - the B input does not exist, so this answer goes right through */
			add_input_pin_to_node(equal, get_zero_pin(netlist), i+port_A_offset);
			add_input_pin_to_node(compare, get_zero_pin(netlist), i+port_A_offset);
		}

		if (i < width_b)
		{
			/* IF - this current input will also have a corresponding a_port input then join it to the gate */
			/* Joining the inputs to the input 2 of that gate */
			remap_pin_to_new_node(node->input_pins[i+port_B_offset], equal, i+port_B_offset);
			add_input_pin_to_node(compare, copy_input_npin(equal->input_pins[i+port_B_offset]), i+port_B_offset);
		}
		else
		{
			/* ELSE - the A input does not exist, so this answer goes right through */
			add_input_pin_to_node(equal, get_zero_pin(netlist), i+port_B_offset);
			add_input_pin_to_node(compare, get_zero_pin(netlist), i+port_B_offset);
		}
	}
	connect_nodes(equal, 0, logical_or_final_gate, 0);
	connect_nodes(compare, 0, logical_or_final_gate, 1);

	/* join that gate to the output */
	remap_pin_to_new_node(node->output_pins[0], logical_or_final_gate, 0);
	oassert(logical_or_final_gate->num_output_pins == 1);

	/* make the two intermediate gates */
	instantiate_EQUAL(equal, LOGICAL_EQUAL, mark, netlist);

	if (type == GTE) instantiate_GT(compare, GT, mark, netlist);
	else             instantiate_GT(compare, LT, mark, netlist);

	free_nnode(node);
}

/*---------------------------------------------------------------------------------------------
 * (function: instantiate_shift_left_or_right )
 *	Creates the hardware for a shift left or right operation by a constant size.
 *-------------------------------------------------------------------------------------------*/
void instantiate_shift_left_or_right(nnode_t *node, operation_list type, short mark, netlist_t *netlist)
{
	/* these variables are used in an attempt so that I don't need if cases.  Probably a bad idea, but fun */
	int width;
	int i;
	int shift_size;
	nnode_t *buf_node;

	width = node->input_port_sizes[0];

	if (node->related_ast_node->children[1]->type == NUMBERS)
	{
		/* record the size of the shift */
		shift_size = node->related_ast_node->children[1]->types.vnumber->get_value();
	}
	else
	{
		shift_size = 0;
		error_message(NETLIST_ERROR, node->related_ast_node->line_number, node->related_ast_node->file_number, "%s\n", "Odin only supports constant shifts at present");
	}

	buf_node = make_1port_gate(BUF_NODE, width, width, node, mark);

	if (type == SL)
	{
		/* IF shift left */

		/* connect inputs to outputs */
	 	for(i = 0; i < width - shift_size; i++)
		{
			// connect higher output pin to lower input pin
			remap_pin_to_new_node(node->input_pins[i], buf_node, i+shift_size);
		}

		/* connect ZERO to outputs that don't have inputs connected */
	 	for(i = 0; i < shift_size; i++)
		{
			// connect 0 to lower outputs
			add_input_pin_to_node(buf_node, get_zero_pin(netlist), i);
		}

	 	for(i = width-1; i >= width-shift_size; i--)
		{
			/* demap the node from the net */
			int idx_2_buffer = node->input_pins[i]->pin_net_idx;
			node->input_pins[i]->net->fanout_pins[idx_2_buffer] = NULL;
		}
	}
	else
	{
		/* ELSE shift right */

		/* connect inputs to outputs */
	 	for(i = width - 1; i >= shift_size; i--)
		{
			// connect higher input pin to lower output pin
			remap_pin_to_new_node(node->input_pins[i], buf_node, i-shift_size);
		}

		/* connect ZERO to outputs that don't have inputs connected */
	 	for(i = width - 1; i >= width - shift_size; i--)
		{
			// connect 0 to lower outputs
			add_input_pin_to_node(buf_node, get_zero_pin(netlist), i);
		}
	 	for(i = 0; i < shift_size; i++)
		{
			/* demap the node from the net */
			int idx_2_buffer = node->input_pins[i]->pin_net_idx;
			node->input_pins[i]->net->fanout_pins[idx_2_buffer] = NULL;
		}
	}

	for(i = 0; i < width; i++)
	{
		remap_pin_to_new_node(node->output_pins[i], buf_node, i);
	}
	/* instantiate the buffer */
	instantiate_buffer(buf_node, mark, netlist);
  
  	/* clean up */
	for (i = 0; i < buf_node->num_input_pins; i++) {
		buf_node->output_pins[i]->net = free_nnet(buf_node->output_pins[i]->net);
		buf_node->input_pins[i] = free_npin(buf_node->input_pins[i]);
	}
	free_nnode(buf_node);
	free_nnode(node);
}

/*---------------------------------------------------------------------------------------------
 * (function: instantiate_arithmatic_shift_right )
 *	Creates the hardware for an arithmatic shift right operation by a constant size.
 *-------------------------------------------------------------------------------------------*/
void instantiate_arithmetic_shift_right(nnode_t *node, short mark, netlist_t *netlist)
{
 	/* these variables are used in an attempt so that I don't need if cases.  Probably a bad idea, but fun */
	int width;
	int i;
	int shift_size;
	nnode_t *buf_node;
	width = node->input_port_sizes[0];
	if (node->related_ast_node->children[1]->type == NUMBERS)
    {
		/* record the size of the shift */
		shift_size = node->related_ast_node->children[1]->types.vnumber->get_value();
	}
	else
	{
		shift_size = 0;
		error_message(NETLIST_ERROR, node->related_ast_node->line_number, node->related_ast_node->file_number, "%s\n", "Odin only supports constant shifts at present");
	}
	buf_node = make_1port_gate(BUF_NODE, width, width, node, mark);
	/* connect inputs to outputs */
	for(i = width - 1; i >= shift_size; i--)
	{
		// connect higher input pin to lower output pin
		remap_pin_to_new_node(node->input_pins[i], buf_node, i-shift_size);
	}
	
	int pad_bit = width - 1 - shift_size;

	/* connect pad_bit pin to outputs that don't have inputs connected */
	for(i = width - 1; i >= width - shift_size; i--)
	{
		add_input_pin_to_node(buf_node, copy_input_npin(buf_node->input_pins[pad_bit]), i);
	}

	for(i = 0; i < shift_size; i++)
	{
		/* demap the node from the net */
		int idx_2_buffer = node->input_pins[i]->pin_net_idx;
		node->input_pins[i]->net->fanout_pins[idx_2_buffer] = NULL;
	}	
	for(i = 0; i < width; i++)
	{
		remap_pin_to_new_node(node->output_pins[i], buf_node, i);
	}
	/* instantiate the buffer */
	instantiate_buffer(buf_node, mark, netlist);


	/* clean up */
	for (i = 0; i < buf_node->num_input_pins; i++) {
		buf_node->output_pins[i]->net = free_nnet(buf_node->output_pins[i]->net);
		buf_node->input_pins[i] = free_npin(buf_node->input_pins[i]);
	}
	free_nnode(buf_node);
	free_nnode(node);
}


//MEHRSHAD//
/*----------------------------------------------------------------------
 * (function: partial_map_adders_GA_top) 
 *--------------------------------------------------------------------*/
void partial_map_adders_GA_top(netlist_t *netlist)
{
	long generation_counter = 0;
	generation_t *previous_generation = NULL;
	generation_t *current_generation = NULL;

	while (/*find the fittest configuration*/1)
	{
		current_generation = create_generation(previous_generation, generation_counter);
		generation_counter++;

		for (int i=0; i<CHILDREN_NUM+1; i++)
		{
			chromosome = current_generation->chromosomes[i];
			partial_map_adders(netlist);
			// calculate fitness and find the fittest of the generation and change the flag of while if neccesary
			destroy_adders();			
		}

		previous_generation = current_generation;

		if (generation_counter == 10) break;
	}

	// create logic based on the fittest configuration
	chromosome = fittest;
	partial_map_adders(netlist);

}

/*----------------------------------------------------------------------
 * (function: partial_map_adders) This function employ the new type of 
 * adders into the adder list and after that, it instantiates adders and 
 * shrinks them into logics
 *--------------------------------------------------------------------*/
void partial_map_adders(/*short traverse_number, */netlist_t *netlist)
{
	for (int i=0; i<num_of_adders; i++)
	{
		adders_list[i]->type = chromosome[i];
		instantiate_add_w_carry(adders_list[i]->type, adders_list[i]->node, /*traverse_number*/PARTIAL_MAP_TRAVERSE_VALUE, netlist);
	}
}

/*---------------------------------------------------------------- 
* This function destroys all adders in the netlist and create them
* again as new one.
*---------------------------------------------------------------*/
void destroy_adders()
{	
	for (int i=0; i<num_of_adders; i++)
	{
		/*---------------------------------------------- 
		 * create new empty adder with the same pins 
		 * as the previous main adder (not pins related
		 *  to nodes in the adder could)
		 *--------------------------------------------*/
		adder_t *new_adder = create_empty_adder(adders_list[i]);

		// Free all created small-detailed logic nodes
		destroy_adder_cloud(adders_list[i]);
		 
		// Replace the new empty adder with previous one in adder list
		adders_list[i] = new_adder;
	}
}

/*------------------------------------------------------------------- 
* This function creates an empty adder with the previous inpu/output
* pins. It is useful when we want to delete the detailed cloud of an
* adder which was created in partial map section.
*------------------------------------------------------------------*/
adder_t *create_empty_adder (adder_t *previous_adder)
{
	adder_t *new_adder = (adder_t *) vtr::malloc (sizeof(adder_t));

	new_adder->node = (nnode_t *) vtr::malloc(sizeof(nnode_t));
	// Make copy of input pins
	new_adder->input->pins = make_copy_of_pins (previous_adder->input->pins, previous_adder->input->count);
	new_adder->input->count = previous_adder->input->count;
	new_adder->input->is_adder = previous_adder->input->is_adder;
	// Make copy of output pins
	new_adder->output->pins = make_copy_of_pins (previous_adder->output->pins, previous_adder->output->count);
	new_adder->output->count = previous_adder->output->count;
	new_adder->output->is_adder = previous_adder->output->is_adder;

	return new_adder;
}

/*------------------------------------------- 
* This function creates a copy of a given pin
*------------------------------------------*/
npin_t** make_copy_of_pins (npin_t **copy, long copy_size)
{
	npin_t **paste = (npin_t **) vtr::malloc(sizeof(npin_t *)*copy_size);

	for (int i=0; i<copy_size; i++)

		paste[i] = copy_npin(copy[i]);

	return paste;
}

/*------------------------------------------------------------------- 
* At first, this function calls the recursive function to get all the
* pointers. After that, it frees all pointers realted to the cloud
*------------------------------------------------------------------*/
void destroy_adder_cloud (adder_t *adder)
{
	num_cloud_pins = 0;
	num_cloud_nets = 0;
	num_cloud_nodes = 0;

	for (int i=0; i<adder->output->count; i++)
		recursive_save_pointers (adder, adder->output->pins[i]->node);

	// Free all pointers related to the cloud
	while (num_cloud_pins != 0)
	{
		free_npin(cloud_pins_list[num_cloud_pins]);
		num_cloud_pins--;
	}
	while (num_cloud_nets != 0)
	{
		free_nnet(cloud_nets_list[num_cloud_nets]);
		num_cloud_nets--;
	}
	while (num_cloud_nodes != 0)
	{
		free_nnode(cloud_nodes_list[num_cloud_nodes]);
		num_cloud_nodes--;
	}

	vtr::free (cloud_pins_list);
	vtr::free (cloud_nets_list);
	vtr::free (cloud_nodes_list);

}

/*----------------------------------------------------------- 
* This function, recursively, adds any pointers of pins, nets 
* and nodes related to the cloud which was created in partial
* map for add function
*----------------------------------------------------------*/
void recursive_save_pointers (adder_t *adder, nnode_t * node)
{
	cloud_nodes_list = (nnode_t**) vtr::realloc (cloud_nodes_list, sizeof(nnode_t *)*(num_cloud_nodes+1));
	cloud_nodes_list[num_cloud_nodes] = node;
	num_cloud_nodes++;

	for (int i=0; i<node->num_input_pins; i++)
	{
		cloud_pins_list = (npin_t **) vtr::realloc (cloud_pins_list, sizeof(npin_t *)*(num_cloud_pins+1));
		cloud_pins_list[num_cloud_pins] = node->input_pins[i];
		num_cloud_pins++;

		cloud_nets_list = (nnet_t **) vtr::realloc(cloud_nets_list, sizeof(nnet_t *)*(num_cloud_nets+1));
		cloud_nets_list[num_cloud_nets] = node->input_pins[i]->net;
		num_cloud_nets++;

		cloud_pins_list = (npin_t **) vtr::realloc (cloud_pins_list, sizeof(npin_t *)*(num_cloud_pins+1));
		for ( int j=0; j<adder->input->count; j++ )
			if ( node->input_pins[i]->net->driver_pin == adder->input->pins[j]->net->driver_pin )
				return;
		cloud_pins_list[num_cloud_pins] = node->input_pins[i]->net->driver_pin;
		num_cloud_pins++;

		recursive_save_pointers (adder, node->input_pins[i]->net->driver_pin->node);
	}
	
}

/*--------------------------------------------------------------------- 
* This function creates a new generation. It changes the values of the 
* genes by doing mutation. In each generation, except of the first one,
* the parent of the next generation will be the fittest chromosome in 
* the previous generation.
*--------------------------------------------------------------------*/
generation_t* create_generation (generation_t *previous_generation, int generation_counter)
{
	// Initialization the structure of new generation
	generation_t *new_generation = (generation_t *) vtr::malloc (sizeof(generation_t));
	// Initilization of chromosomes in new generation
	new_generation->fittest = (short *) vtr::malloc(sizeof(short)*num_of_adders);
	new_generation->chromosomes = (short **) vtr::malloc(sizeof(short*)*CHILDREN_NUM+1);
	for (int i=0; i<CHILDREN_NUM+1; i++)
		new_generation->chromosomes[i] = (short *) vtr::malloc (sizeof(short)*num_of_adders);
	
	// Here the value setting of the new generation chromosomes will occur
	// The first chromosome of each generation is the parent
	if (generation_counter == 0)
	{
		// Considering the parent of the first generation as the chromosome which is produced by the first traverse
		new_generation->chromosomes[0] = chromosome;
		new_generation->fitnesses[0] = chromosome_fitness;
	}
	else
	{
		new_generation->chromosomes[0] = previous_generation->fittest;
		new_generation->fitnesses[0] = -1;
	}

	for (int i=1; i<CHILDREN_NUM+1; i++)
	{
		// The children of each generation will be mutated with their parent chromosome
		new_generation->chromosomes[i] = mutate (new_generation->chromosomes[0]);
		new_generation->fitnesses[i] = -1;
	}

	return new_generation;
}

/*--------------------------------------------------------------------- 
* This function changes the value of the genes in the given chromosome.
* Based on the specified mutation rate, it finds the position of genes 
* which should be mutated randomly, and after that, it changes their 
* values through rand function.
*--------------------------------------------------------------------*/
short *mutate (short *parent)
{
	short *new_chromosome = (short*) vtr::malloc (sizeof(short)*num_of_adders);
	// Find a point and flip the chromosome from there
	// int cross_point = rand()%num_of_adders;
	// Find points based on the mutation rate and change their values randomly
	int mutation_rate = 0.05;
	int num_of_genes_will_be_changed = mutation_rate*num_of_adders;

	for (int i=0; i<num_of_adders; i++)
		new_chromosome[i] = parent[i];

	while (num_of_genes_will_be_changed != 0)
	{
		int point = rand()%num_of_adders;
		new_chromosome[point] = rand()%3;

		num_of_genes_will_be_changed--;
	}
	
	return new_chromosome;
}
//MEHRSHAD//