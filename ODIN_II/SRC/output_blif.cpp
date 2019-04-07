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
#include "odin_types.h"
#include "odin_globals.h"

#include "netlist_utils.h"
#include "odin_util.h"
#include "output_blif.h"

#include "node_creation_library.h"

#include "multipliers.h"
#include "hard_blocks.h"
#include "adders.h"
#include "subtractions.h"
#include "vtr_util.h"
#include "vtr_memory.h"

bool haveOutputLatchBlackbox = false;

void depth_first_traversal_to_output(short marker_value, FILE *fp, netlist_t *netlist);
void depth_traverse_output_blif(nnode_t *node, int traverse_mark_number, FILE *fp);
void output_node(nnode_t *node, short traverse_number, FILE *fp);
void define_logical_function(nnode_t *node, FILE *out);
void define_set_input_logical_function(nnode_t *node, const char *bit_output, FILE *out);
void define_ff(nnode_t *node, FILE *out);
void define_decoded_mux(nnode_t *node, FILE *out);
void output_blif_pin_connect(nnode_t *node, FILE *out);

/*---------------------------------------------------------------------------
 * (function: output_blif)
 * 	The function that prints out the details for a blif formatted file
 *-------------------------------------------------------------------------*/
void output_blif(char *file_name, netlist_t *netlist)
{
	int i;
	int count = 0;
	short first_time_inputs = FALSE;
	short first_time_outputs = FALSE;
	FILE *out;
	char *out_file;

	/* open the file for output */
	if (global_args.high_level_block != NULL)
	{
		out_file = (char*)vtr::malloc(sizeof(char)*(1+strlen(file_name)+strlen(global_args.high_level_block)+6));
		odin_sprintf(out_file, "%s_%s.blif", file_name, global_args.high_level_block.value());
		out = fopen(out_file, "w");
	}
	else
	{
		out = fopen(file_name, "w");
	}

	if (out == NULL)
	{
		error_message(NETLIST_ERROR, -1, -1, "Could not open output file %s\n", file_name);
	}

	fprintf(out, ".model %s\n", top_module->children[0]->types.identifier);

	/* generate all te signals */
	for (i = 0; i < netlist->num_top_input_nodes; i++)
	{
		if (first_time_inputs == FALSE)
		{
			count = fprintf(out, ".inputs");
			first_time_inputs = TRUE;
		}

		if (global_args.high_level_block != NULL)
		{
			if (strlen(netlist->top_input_nodes[i]->name) + count < 79)
				count = count + fprintf(out, " %s^^%i-%i", netlist->top_input_nodes[i]->name, netlist->top_input_nodes[i]->related_ast_node->far_tag, netlist->top_input_nodes[i]->related_ast_node->high_number);
			else
			{
				/* wrapping line */
				count = fprintf(out, " \\\n %s^^%i-%i", netlist->top_input_nodes[i]->name,netlist->top_input_nodes[i]->related_ast_node->far_tag, netlist->top_input_nodes[i]->related_ast_node->high_number);
				count = count - 3;
			}
		}
		else
		{
			if (strlen(netlist->top_input_nodes[i]->name) + count < 79)
			{
				count = count + fprintf(out, " %s", netlist->top_input_nodes[i]->name);
			}
			else
			{
				/* wrapping line */
				count = fprintf(out, " \\\n %s", netlist->top_input_nodes[i]->name);
				count = count - 3;
			}
		}

	}
	fprintf(out, "\n");

	count = 0;
	for (i = 0; i < netlist->num_top_output_nodes; i++)
	{
		if (netlist->top_output_nodes[i]->input_pins[0]->net->driver_pin == NULL)
		{
			warning_message(NETLIST_ERROR, netlist->top_output_nodes[i]->related_ast_node->line_number, netlist->top_output_nodes[i]->related_ast_node->file_number, "This output is undriven (%s) and will be removed\n", netlist->top_output_nodes[i]->name);
		}
		else
		{
			if (first_time_outputs == FALSE)
			{
				count = fprintf(out, ".outputs");
				first_time_outputs = TRUE;
			}

			if (global_args.high_level_block != NULL)
			{
				if ((strlen(netlist->top_output_nodes[i]->name) + count) < 79)
					count = count + fprintf(out, " %s^^%i-%i", netlist->top_output_nodes[i]->name,netlist->top_output_nodes[i]->related_ast_node->far_tag, netlist->top_output_nodes[i]->related_ast_node->high_number);
				else
				{
					/* wrapping line */
					count = fprintf(out, "\\\n %s^^%i-%i", netlist->top_output_nodes[i]->name,netlist->top_output_nodes[i]->related_ast_node->far_tag, netlist->top_output_nodes[i]->related_ast_node->high_number);
					count = count - 3;
				}
			}
			else
			{
				if ((strlen(netlist->top_output_nodes[i]->name) + count) < 79)
					count = count + fprintf(out, " %s", netlist->top_output_nodes[i]->name);
				else
				{
					/* wrapping line */
					count = fprintf(out, "\\\n %s", netlist->top_output_nodes[i]->name);
					count = count - 3;
				}
			}
		}
	}
	fprintf(out, "\n");

	/* add gnd, unconn, and vcc */
	fprintf(out, "\n.names gnd\n.names unconn\n.names vcc\n1\n");
	fprintf(out, "\n");

	/* traverse the internals of the flat net-list */
	if (strcmp(configuration.output_type.c_str(), "blif") == 0)
	{
		depth_first_traversal_to_output(OUTPUT_TRAVERSE_VALUE, out, netlist);
	}
	else
	{
		error_message(NETLIST_ERROR, 0, -1, "%s", "Invalid output file type.");
	}

	/* connect all the outputs up to the last gate */
	for (i = 0; i < netlist->num_top_output_nodes; i++)
	{
		/* KEN -- DPRAM WORKING HERE FOR JASON */
		nnode_t *node = netlist->top_output_nodes[i];
		if (node->input_pins[0]->net->driver_pin != NULL)
		{
			if (global_args.high_level_block != NULL)
			{
				fprintf(out, ".names %s^^%i-%i %s^^%i-%i\n1 1\n",
						node->input_pins[0]->net->driver_pin->node->name,
						node->input_pins[0]->net->driver_pin->node->related_ast_node->far_tag,
						node->input_pins[0]->net->driver_pin->node->related_ast_node->high_number,
						node->name,
						node->related_ast_node->far_tag,
						node->related_ast_node->high_number
				);
			}
			else
			{
				/*
				 *  Use the name of the driver pin as the name of the driver
				 *  as long as that name is set, and is not equal to the name of the output pin.
				 *
				 * 	Otherwise, use the name of the driver node.
				 */
				char *driver = node->input_pins[0]->net->driver_pin->name;
				char *output = node->name;
				if (!driver || !strcmp(driver,output))
					driver = node->input_pins[0]->net->driver_pin->node->name;

				/* Skip if the driver and output have the same name (i.e. the output of a flip-flop) */
				if (strcmp(driver,output) != 0) fprintf(out, ".names %s %s\n1 1\n", driver, output);
			}

		}
		fprintf(out, "\n");
	}

	/* finish off the top level module */
	fprintf(out, ".end\n");
	fprintf(out, "\n");

	/* Print out any hard block modules */
	add_the_blackbox_for_mults(out);
	add_the_blackbox_for_adds(out);

	output_hard_blocks(out);
	fclose(out);
}

/*---------------------------------------------------------------------------
 * (function: depth_first_traversal_to_parital_map()
 *-------------------------------------------------------------------------------------------*/
void depth_first_traversal_to_output(short marker_value, FILE *fp, netlist_t *netlist)
{
	int i;

	netlist->gnd_node->name = vtr::strdup("gnd");
	netlist->vcc_node->name = vtr::strdup("vcc");
	netlist->pad_node->name = vtr::strdup("unconn");
	/* now traverse the ground, vcc, and unconn pins */
	depth_traverse_output_blif(netlist->gnd_node, marker_value, fp);
	depth_traverse_output_blif(netlist->vcc_node, marker_value, fp);
	depth_traverse_output_blif(netlist->pad_node, marker_value, fp);

	/* start with the primary input list */
	for (i = 0; i < netlist->num_top_input_nodes; i++)
	{
		if (netlist->top_input_nodes[i] != NULL)
		{
			depth_traverse_output_blif(netlist->top_input_nodes[i], marker_value, fp);
		}
	}
}

/*--------------------------------------------------------------------------
 * (function: depth_first_traverse)
 *------------------------------------------------------------------------*/
void depth_traverse_output_blif(nnode_t *node, int traverse_mark_number, FILE *fp)
{
	int i, j;
	nnode_t *next_node;
	nnet_t *next_net;

	if (node->traverse_visited == traverse_mark_number)
	{
		return;
	}
	else
	{
		/* ELSE - this is a new node so depth visit it */

		/* POST traverse  map the node since you might delete */
		output_node(node, traverse_mark_number, fp);

		/* mark that we have visitied this node now */
		node->traverse_visited = traverse_mark_number;

		for (i = 0; i < node->num_output_pins; i++)
		{
			if (node->output_pins[i]->net == NULL)
				continue;

			next_net = node->output_pins[i]->net;
			for (j = 0; j < next_net->num_fanout_pins; j++)
			{
				if (next_net->fanout_pins[j] == NULL)
					continue;

				next_node = next_net->fanout_pins[j]->node;
				if (next_node == NULL)
					continue;

				/* recursive call point */
				depth_traverse_output_blif(next_node, traverse_mark_number, fp);
			}
		}

	}
}
/*-------------------------------------------------------------------
 * (function: partial_map_node)
 * 	Depending on node type, figures out what to print for this node
 *------------------------------------------------------------------*/
void output_node(nnode_t *node, short /*traverse_number*/, FILE *fp)
{
	switch (node->type)
	{
		case GT:
			define_set_input_logical_function(node, "100 1\n", fp);
			oassert(node->num_input_pins == 3);
			oassert(node->input_pins[2] != NULL);
			break;
		case LT:
			define_set_input_logical_function(node, "010 1\n", fp); // last input decides if this
			oassert(node->num_input_pins == 3);
			oassert(node->input_pins[2] != NULL);
			break;
		case ADDER_FUNC:
			define_set_input_logical_function(node, "001 1\n010 1\n100 1\n111 1\n", fp);
			break;
		case CARRY_FUNC:
			define_set_input_logical_function(node, "011 1\n101 1\n110 1\n111 1\n", fp);
			break;
		case BITWISE_NOT:
			define_set_input_logical_function(node, "0 1\n", fp);
			break;

		case LOGICAL_AND:
		case LOGICAL_OR:
		case LOGICAL_XOR:
		case LOGICAL_XNOR:
		case LOGICAL_NAND:
		case LOGICAL_NOR:
		case LOGICAL_EQUAL:
		case NOT_EQUAL:
		case LOGICAL_NOT:
			define_logical_function(node, fp);
			break;

		case MUX_2:
			define_decoded_mux(node, fp);
			break;

		case FF_NODE:
			define_ff(node, fp);
			break;

		case MULTIPLY:
			if (hard_multipliers == NULL)
				oassert(FALSE); /* should be soft logic! */
			define_mult_function(node, fp);

			break;

		//case FULLADDER:
		case ADD:
			if (hard_adders == NULL)
				oassert(FALSE); /* should be soft logic! */
			define_add_function(node, fp);
			break;

		case MINUS:
			oassert(hard_adders); /* should be soft logic! */
			if(hard_adders)
				define_add_function(node, fp);
			break;

		case MEMORY:
		case HARD_IP:
			define_hard_block(node, fp);
			break;
		case INPUT_NODE:
		case OUTPUT_NODE:
		case PAD_NODE:
		case CLOCK_NODE:
		case GND_NODE:
		case VCC_NODE:
			/* some nodes already converted */
			break;

		case BITWISE_AND:
		case BITWISE_NAND:
		case BITWISE_NOR:
		case BITWISE_XNOR:
		case BITWISE_XOR:
		case BITWISE_OR:
		case BUF_NODE:
		case MULTI_PORT_MUX:
		case SL:
		case SR:
        case ASR:
		case CASE_EQUAL:
		case CASE_NOT_EQUAL:
		case DIVIDE:
		case MODULO:
		case GTE:
		case LTE:
		default:
			/* these nodes should have been converted to softer versions */
			error_message(NETLIST_ERROR, 0,-1, "%s", "Output blif: node should have been converted to softer version.");
			break;
	}
}

/*-------------------------------------------------------------------------
 * (function: define_logical_function)
 *-----------------------------------------------------------------------*/
void define_logical_function(nnode_t *node, FILE *out)
{
	int i, j;
	char *temp_string;
	int flag = 0;

	fprintf(out, ".names");


	if (global_args.high_level_block != NULL)
	{
		/* printout all the port hookups */
		for (i = 0; i < node->num_input_pins; i++)
		{
			/* now hookup the input wires with their respective ports.  [1+i] to skip output spot. */
			if (node->input_pins[i]->net->driver_pin->node->related_ast_node != NULL)
				fprintf(out, " %s^^%i-%i", node->input_pins[i]->net->driver_pin->node->name, 	node->input_pins[i]->net->driver_pin->node->related_ast_node->far_tag, node->input_pins[i]->net->driver_pin->node->related_ast_node->high_number);
			else
				fprintf(out, " %s", node->input_pins[i]->net->driver_pin->node->name);
		}
		/* now print the output */
		if (node->related_ast_node != NULL)
			fprintf(out, " %s^^%i-%i", node->name, node->related_ast_node->far_tag, node->related_ast_node->high_number);
		else
			fprintf(out, " %s", node->name);
	}
	else
	{
		/* printout all the port hookups */
		for (i = 0; i < node->num_input_pins; i++)
		{
			/* now hookup the input wires with their respective ports.  [1+i] to skip output spot. */
			/* Just print the driver_pin->name NOT driver_pin->node->name -- KEN */
			nnet_t *net = node->input_pins[i]->net;
			if (net && net->driver_pin)
			{
				if (net->driver_pin->name != NULL)
				{
					if ((net->driver_pin->node->type == MULTIPLY) ||
					(net->driver_pin->node->type == HARD_IP) ||
					(net->driver_pin->node->type == MEMORY) ||
					(net->driver_pin->node->type == ADD) ||
					(net->driver_pin->node->type == MINUS) )
					{
						fprintf(out, " %s", net->driver_pin->name);
					}
					else
					{
						fprintf(out, " %s", net->driver_pin->node->name);
					}
				}
				else
				{
					fprintf(out, " %s", net->driver_pin->node->name);
				}
			}
			else
			{
				int line_number = node->related_ast_node?node->related_ast_node->line_number:0;
				warning_message(NETLIST_ERROR, line_number, -1, "Net %s driving node %s is itself undriven.", net->name, node->name);

				fprintf(out, " %s", "unconn");
			}
		}
		/* now print the output */
		fprintf(out, " %s", node->name);
	}
	fprintf(out, "\n");

	oassert(node->num_output_pins == 1);

	/* print out the blif definition of this gate */
	switch (node->type)
	{
		case LOGICAL_AND:
		{
			/* generates: 111111 1 */
			for (i = 0; i < node->num_input_pins; i++)
			{
				fprintf(out, "1");
			}
			fprintf(out, " 1\n");
			break;
		}
		case LOGICAL_OR:
		{
			/* generates: 1----- 1\n-1----- 1\n ... */
			for (i = 0; i < node->num_input_pins; i++)
			{
				for (j = 0; j < node->num_input_pins; j++)
				{
					if (i == j)
						fprintf(out, "1");
					else
						fprintf(out, "-");
				}
				fprintf(out, " 1\n");
			}
			break;
		}
		case LOGICAL_NAND:
		{
			/* generates: 0----- 1\n-0----- 1\n ... */
			for (i = 0; i < node->num_input_pins; i++)
			{
				for (j = 0; j < node->num_input_pins; j++)
				{
					if (i == j)
						fprintf(out, "0");
					else
						fprintf(out, "-");
				}
				fprintf(out, " 1\n");
			}
			break;
		}
		case LOGICAL_NOT:
		case LOGICAL_NOR:
		{
			/* generates: 0000000 1 */
			for (i = 0; i < node->num_input_pins; i++)
			{
				fprintf(out, "0");
			}
			fprintf(out, " 1\n");
			break;
		}
		case LOGICAL_EQUAL:
		case LOGICAL_XOR:
		{
			oassert(node->num_input_pins <= 3);
			/* generates: a 1 when odd number of 1s */
			for (i = 0; i < my_power(2, node->num_input_pins); i++)
			{
				if ((i % 8 == 1) || (i % 8 == 2) || (i % 8 == 4) || (i % 8 == 7))
				{
					temp_string = convert_long_to_bit_string(i, node->num_input_pins);
					fprintf(out, "%s", temp_string);
					vtr::free(temp_string);
					fprintf(out, " 1\n");
				}
			}
			break;
		}
		case NOT_EQUAL:
		case LOGICAL_XNOR:
		{
			oassert(node->num_input_pins <= 3);
			for (i = 0; i < my_power(2, node->num_input_pins); i++)
			{
				if ((i % 8 == 0) || (i % 8 == 3) || (i % 8 == 5) || (i % 8 == 6))
				{
					temp_string = convert_long_to_bit_string(i, node->num_input_pins);
					fprintf(out, "%s", temp_string);
					vtr::free(temp_string);
					fprintf(out, " 1\n");
				}
			}
			break;
		}
		default:
			oassert(FALSE);
			break;
	}

	fprintf(out, "\n");
	if (flag == 1)
		output_blif_pin_connect(node, out);
}

/*------------------------------------------------------------------------
 * (function: define_set_input_logical_function)
 *----------------------------------------------------------------------*/
void define_set_input_logical_function(nnode_t *node, const char *bit_output, FILE *out)
{
	int i;
	int flag = 0;

	fprintf(out, ".names");

	oassert(node->num_output_pins == 1);
	oassert(node->num_input_pins >= 1);


	if (global_args.high_level_block != NULL)
	{
		/* printout all the port hookups */
		for (i = 0; i < node->num_input_pins; i++)
		{
			/* now hookup the input wires with their respective ports.  [1+i] to skip output spot. */
			if (node->input_pins[i]->net->driver_pin->node->related_ast_node != NULL)
				fprintf(out, " %s^^%i-%i", node->input_pins[i]->net->driver_pin->node->name, node->input_pins[i]->net->driver_pin->node->related_ast_node->far_tag, node->input_pins[i]->net->driver_pin->node->related_ast_node->high_number);
			else
				fprintf(out, " %s", node->input_pins[i]->net->driver_pin->node->name);
		}

		/* now print the output */
		if (node->related_ast_node != NULL)
			fprintf(out, " %s^^%i-%i", node->name, node->related_ast_node->far_tag, node->related_ast_node->high_number);
		else
			fprintf(out, " %s", node->name);
		fprintf(out, "\n");

		/* print out the blif definition of this gate */
		if (bit_output != NULL)
		{
			fprintf(out, "%s", bit_output);
		}
		fprintf(out, "\n");
	}
	else
	{
		/* printout all the port hookups */
		for (i = 0; i < node->num_input_pins; i++)
		{
			if (node->input_pins[i]->net->driver_pin != NULL)
			{
				if (node->input_pins[i]->net->driver_pin->name != NULL)
				{
					/* now hookup the input wires with their respective ports.  [1+i] to skip output spot. */
					/* Just print the driver_pin->name NOT driver_pin->node->name -- KEN */
					if ((node->input_pins[i]->net->driver_pin->node->type == MULTIPLY) ||
			    	 	  (node->input_pins[i]->net->driver_pin->node->type == HARD_IP) ||
			    	   	(node->input_pins[i]->net->driver_pin->node->type == MEMORY) ||
			    	   	(node->input_pins[i]->net->driver_pin->node->type == ADD) ||
			    	   	(node->input_pins[i]->net->driver_pin->node->type == MINUS))
					{
						fprintf(out, " %s", node->input_pins[i]->net->driver_pin->name);
					}
					else
					{
						fprintf(out, " %s", node->input_pins[i]->net->driver_pin->node->name);
					}
				}
				else
				{
					fprintf(out, " %s", node->input_pins[i]->net->driver_pin->node->name);
				}
			}
		}

		/* now print the output */
		fprintf(out, " %s", node->name);
		fprintf(out, "\n");

		/* print out the blif definition of this gate */
		if (bit_output != NULL)
		{
			fprintf(out, "%s", bit_output);
		}
		fprintf(out, "\n");
	}

	if ((flag == 1) && (node->type == HARD_IP))
		output_blif_pin_connect(node, out);
}


/*-------------------------------------------------------------------------
 * (function: define_ff)
 *-----------------------------------------------------------------------*/
void define_ff(nnode_t *node, FILE *out)
{
	oassert(node->num_output_pins == 1);
	oassert(node->num_input_pins == 2);


	int initial_value = global_args.sim_initial_value;
	if(node->has_initial_value)
		initial_value = node->initial_value;
	
	/* By default, latches value are unknown, represented by 3 in a BLIF file
	and by -1 internally in ODIN */
	// TODO switch to default!! to avoid confusion
	if(initial_value == -1)
		initial_value = 3;

	// grab the edge sensitivity of the flip flop
	const char *edge_type_str = edge_type_blif_str(node); 

	std::string input;
	std::string output;
	std::string clock_driver;

	fprintf(out, ".latch ");

	/* input */
	if (global_args.high_level_block != NULL)
	{
		fprintf(out, "%s^^%i-%i ",
						node->input_pins[0]->net->driver_pin->node->name,
						node->input_pins[0]->net->driver_pin->node->related_ast_node->far_tag,
						node->input_pins[0]->net->driver_pin->node->related_ast_node->high_number);
	}
	else if (node->input_pins[0]->net->driver_pin->name == NULL)
		fprintf(out, "%s ", node->input_pins[0]->net->driver_pin->node->name);
	else
		fprintf(out, "%s ", node->input_pins[0]->net->driver_pin->name);


	/* output */
	if (global_args.high_level_block != NULL)
	{
		fprintf(out, "%s^^%i-%i ",
						node->name, 
						node->related_ast_node->far_tag,
						node->related_ast_node->high_number);
	}
	else
		fprintf(out, "%s ", node->name);


	/* clock */
	fprintf(out, "%s ", edge_type_str);
	if(global_args.high_level_block != NULL)
	{
		fprintf(out, "%s^^%i-%i ",
						node->input_pins[1]->net->driver_pin->node->name,
						node->input_pins[1]->net->driver_pin->node->related_ast_node->far_tag,
						node->input_pins[1]->net->driver_pin->node->related_ast_node->high_number);
	}
	else if (node->input_pins[1]->net->driver_pin->name == NULL)
		fprintf(out, "%s ", node->input_pins[1]->net->driver_pin->node->name);
	else
		fprintf(out, "%s ", node->input_pins[1]->net->driver_pin->name);

	fprintf(out, "%d\n\n", initial_value);
}

/*--------------------------------------------------------------------------
 * (function: define_decoded_mux)
 *------------------------------------------------------------------------*/
void define_decoded_mux(nnode_t *node, FILE *out)
{
	int i, j;

	oassert(node->input_port_sizes[0] == node->input_port_sizes[1]);

	fprintf(out, ".names");

	if (global_args.high_level_block != NULL)
	{
		/* printout all the port hookups */
		for (i = 0; i < node->num_input_pins; i++)
		{
			/* now hookup the input wires with their respective ports.  [1+i] to skip output spot. */
			if (node->input_pins[i]->net->driver_pin->node->related_ast_node != NULL)
			{
				fprintf(out, " %s^^%i-%i", node->input_pins[i]->net->driver_pin->node->name, node->input_pins[i]->net->driver_pin->node->related_ast_node->far_tag, node->input_pins[i]->net->driver_pin->node->related_ast_node->high_number);
			}
			else
			{
				fprintf(out, " %s", node->input_pins[i]->net->driver_pin->node->name);
			}
		}
		/* now print the output */
		if (node->related_ast_node != NULL)
			fprintf(out, " %s^^%i-%i", node->name, node->related_ast_node->far_tag, node->related_ast_node->high_number);
		else
			fprintf(out, " %s", node->name);
	}
	else
	{
		/* printout all the port hookups */
		for (i = 0; i < node->num_input_pins; i++)
		{
			/* now hookup the input wires with their respective ports.  [1+i] to skip output spot. */
			/* Just print the driver_pin->name NOT driver_pin->node->name -- KEN */
			nnet_t *net = node->input_pins[i]->net;

			if (!net->driver_pin)
			{
				// Add a warning for an undriven net.
				int line_number = node->related_ast_node?node->related_ast_node->line_number:0;
				warning_message(NETLIST_ERROR, line_number, -1, "Net %s driving node %s is itself undriven.", net->name, node->name);

				fprintf(out, " %s", "unconn");
			}
			else
			{
				if (net->driver_pin->name != NULL)
				{
					if ((net->driver_pin->node->type == MULTIPLY) ||
				    	(net->driver_pin->node->type == HARD_IP) ||
				    	(net->driver_pin->node->type == MEMORY) ||
				    	(net->driver_pin->node->type == ADD) ||
				    	(net->driver_pin->node->type == MINUS))
					{
						fprintf(out, " %s", net->driver_pin->name);
					}
					else
					{
						fprintf(out, " %s", net->driver_pin->node->name);
					}
				}
				else
				{
					fprintf(out, " %s", net->driver_pin->node->name);
				}
			}
		}

		// Now print the output
		fprintf(out, " %s", node->name);
	}
	fprintf(out, "\n");

	oassert(node->num_output_pins == 1);

	/* generates: 1----- 1\n-1----- 1\n ... */
	for (i = 0; i < node->input_port_sizes[0]; i++)
	{
		for (j = 0; j < node->num_input_pins; j++)
		{
			if (i == j)
				fprintf(out, "1");
			else if (i+node->input_port_sizes[0] == j)
				fprintf(out, "1");
			else if (i > node->input_port_sizes[0])
				fprintf(out, "0");
			else
				fprintf(out, "-");
		}
		fprintf(out, " 1\n");
	}

	fprintf(out, "\n");
}

/*--------------------------------------------------------------------------
 * (function: output_blif_pin_connect)
 *------------------------------------------------------------------------*/
void output_blif_pin_connect(nnode_t *node, FILE *out)
{
	int i;

	/* printout all the port hookups */
	for (i = 0; i < node->num_input_pins; i++)
	{
		/* Find pins that need to be connected -- KEN */
		if (node->input_pins[i]->net->driver_pin->name != NULL)
			fprintf(out, ".names %s %s\n1 1\n\n", node->input_pins[i]->net->driver_pin->node->name, node->input_pins[i]->net->driver_pin->name);
	}

	return;
}
