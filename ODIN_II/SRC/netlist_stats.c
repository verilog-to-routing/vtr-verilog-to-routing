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
#include "types.h"
#include "globals.h"
#include "errors.h"
#include "netlist_utils.h"
#include "odin_util.h"
#include "netlist_stats.h"
#include "multipliers.h"


/*------------------------------------------------------------------------
 * (function: netlist_stats)
 *----------------------------------------------------------------------*/
void netlist_stats(netlist_t *netlist, char *path, char *name)
{
	FILE *fp;
	char path_and_file[4096];

	/* open the file */
	sprintf(path_and_file, "%s/%s", path, name);
	fp = fopen(path_and_file, "w");

	/* allocate the stat structure */
	netlist->stats = (netlist_stats_t*)malloc(sizeof(netlist_stats_t));
	netlist->stats->fanin_distribution = NULL;
	netlist->stats->num_fanin_distribution = 0;
	netlist->stats->fanout_distribution = NULL;
	netlist->stats->num_fanout_distribution = 0;

	/* collect high-lelve stats */
	netlist->stats->num_inputs = netlist->num_top_input_nodes;
	netlist->stats->num_outputs = netlist->num_top_output_nodes;
	netlist->stats->num_ff_nodes = netlist->num_ff_nodes;
	netlist->stats->num_logic_nodes = netlist->num_internal_nodes;
	netlist->stats->num_nodes = netlist->stats->num_inputs + netlist->stats->num_outputs + netlist->stats->num_ff_nodes + netlist->stats->num_logic_nodes; 

	/* calculate the average fanout */
	calculate_avg_fanout(netlist);
	/* calculate the average fanin */
	calculate_avg_fanin(netlist);
	/* calculate combinational shape of each sequential level */
	calculate_combinational_shapes(netlist);

	/* traverse the netlist looking for the stats */
	depth_first_traverse_stats(fp, STATS, netlist);

	display_per_node_stats(fp, netlist);

	fclose(fp);
}

/*---------------------------------------------------------------------------------------------
 * (function: display_per_node_stats()
 *-------------------------------------------------------------------------------------------*/
void display_per_node_stats(FILE *fp, netlist_t *netlist)
{
	int i;

	/* GND nnode_t *gnd_node;*/
	fprintf(fp, "GND NODE");
	fprintf(fp, "gnd-----------------------------------------------------");
	display_node_stats(fp, netlist->gnd_node);

	/* VCC nnode_t *vcc_node;*/
	fprintf(fp, "VCC NODE");
	fprintf(fp, "vcc-----------------------------------------------------");
	display_node_stats(fp, netlist->vcc_node);

	/* PIs nnode_t** top_input_nodes;
	int num_top_input_nodes; */
	fprintf(fp, "PI NODEs");
	fprintf(fp, "pi-----------------------------------------------------");
	for (i = 0; i < netlist->num_top_input_nodes; i++)
	{
		display_node_stats(fp, netlist->top_input_nodes[i]);
	}

	/* POs nnode_t** top_output_nodes;
	int num_top_output_nodes; */
	fprintf(fp, "PO NODEs");
	fprintf(fp, "po-----------------------------------------------------");
	for (i = 0; i < netlist->num_top_output_nodes; i++)
	{
		display_node_stats(fp, netlist->top_output_nodes[i]);
	}

	/* FFs nnode_t** ff_nodes;
	int num_ff_nodes; */
	fprintf(fp, "FF NODEs");
	fprintf(fp, "ff-----------------------------------------------------");
	for (i = 0; i < netlist->num_ff_nodes; i++)
	{
		display_node_stats(fp, netlist->ff_nodes[i]);
	}

	/* COBINATIONAL nnode_t** internal_nodes;
	int num_internal_nodes; */
	fprintf(fp, "COMBO NODEs");
	fprintf(fp, "combo-----------------------------------------------------");
	for (i = 0; i < netlist->num_internal_nodes; i++)
	{
		display_node_stats(fp, netlist->internal_nodes[i]);
	}

	/* CLOCKS nnode_t** clocks;
	int num_clocks; */
	fprintf(fp, "CLOCK NODEs");
	fprintf(fp, "clock-----------------------------------------------------");
	for (i = 0; i < netlist->num_clocks; i++)
	{
		display_node_stats(fp, netlist->clocks[i]);
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: display_node_stats()
 *-------------------------------------------------------------------------------------------*/
void display_node_stats(FILE *fp, nnode_t* node)
{
	fprintf(fp, "Node name: %s -->", node->name);
	fprintf(fp, "inputs: %d; outputs: %d; delay: %d; backward_delay: %d; sequential_level: %d; is_seq_term: %d\n", node->num_input_pins, node->num_output_pins, node->forward_level, node->backward_level, node->sequential_level, node->sequential_terminator);
}

/*---------------------------------------------------------------------------------------------
 * (function: calculate_avg_fanout()
 *-------------------------------------------------------------------------------------------*/
void calculate_avg_fanout(netlist_t *netlist)
{
	int i, j;
	long fanout_total = 0;
	long node_fanout;
	long output_pins_total = 0;

	/* GND nnode_t *gnd_node;*/

	/* VCC nnode_t *vcc_node;*/

	/* PIs nnode_t** top_input_nodes;
	int num_top_input_nodes; */
	for (i = 0; i < netlist->num_top_input_nodes; i++)
	{
		if (netlist->top_input_nodes[i]->type == CLOCK_NODE)
			continue;

		for (j = 0; j < netlist->top_input_nodes[i]->num_output_pins; j++)
		{
			node_fanout = num_fanouts_on_output_pin(netlist->top_input_nodes[i], j);
			if (node_fanout > 0)
			{
				output_pins_total ++;
				add_to_distribution(&netlist->stats->fanout_distribution, &netlist->stats->num_fanout_distribution, node_fanout);
				fanout_total += node_fanout;
			}
		}
	}

	/* POs nnode_t** top_output_nodes;
	int num_top_output_nodes; */

	/* FFs nnode_t** ff_nodes;
	int num_ff_nodes; */
	for (i = 0; i < netlist->num_ff_nodes; i++)
	{
		for (j = 0; j < netlist->ff_nodes[i]->num_output_pins; j++)
		{
			node_fanout = num_fanouts_on_output_pin(netlist->ff_nodes[i], j);
			if (node_fanout > 0)
			{
				output_pins_total ++;
				add_to_distribution(&netlist->stats->fanout_distribution, &netlist->stats->num_fanout_distribution, node_fanout);
				fanout_total += node_fanout;
			}
		}
	}

	/* COBINATIONAL nnode_t** internal_nodes;
	int num_internal_nodes; */
	for (i = 0; i < netlist->num_internal_nodes; i++)
	{
		for (j = 0; j < netlist->internal_nodes[i]->num_output_pins; j++)
		{
			node_fanout = num_fanouts_on_output_pin(netlist->internal_nodes[i], j);
			if (node_fanout > 0)
			{
				output_pins_total ++;
				add_to_distribution(&netlist->stats->fanout_distribution, &netlist->stats->num_fanout_distribution, node_fanout);
				fanout_total += node_fanout;
			}
		}
	}

	/* CLOCKS nnode_t** clocks;
	int num_clocks; */

	/* calculate the arithmetic average */
	netlist->stats->average_fanout = (float)(fanout_total)/(float)(output_pins_total);
	netlist->stats->num_edges_fo = fanout_total;
	netlist->stats->num_output_pins = output_pins_total;
	netlist->stats->average_output_pins_per_node = (float)output_pins_total/(float)(netlist->num_internal_nodes+netlist->num_ff_nodes+netlist->num_top_input_nodes);
}

/*---------------------------------------------------------------------------------------------
 * (function: calculate_avg_fanin()
 *-------------------------------------------------------------------------------------------*/
void calculate_avg_fanin(netlist_t *netlist)
{
	int i;
	long fanin_total = 0;

	/* GND nnode_t *gnd_node;*/

	/* VCC nnode_t *vcc_node;*/

	/* PIs nnode_t** top_input_nodes;
	int num_top_input_nodes; */

	/* POs nnode_t** top_output_nodes;
	int num_top_output_nodes; */
	fanin_total += netlist->num_top_output_nodes;
	for (i = 0; i <  netlist->num_top_output_nodes; i++)
	{
		add_to_distribution(&netlist->stats->fanin_distribution, &netlist->stats->num_fanin_distribution, 1);
	}

	/* FFs nnode_t** ff_nodes;
	int num_ff_nodes; */
	fanin_total += netlist->num_ff_nodes;
	for (i = 0; i <  netlist->num_ff_nodes; i++)
	{
		add_to_distribution(&netlist->stats->fanin_distribution, &netlist->stats->num_fanin_distribution, 1);
	}

	/* COBINATIONAL nnode_t** internal_nodes;
	int num_internal_nodes; */
	for (i = 0; i < netlist->num_internal_nodes; i++)
	{
		fanin_total += netlist->internal_nodes[i]->num_input_pins;
		/* fanin is number of input pins */
		add_to_distribution(&netlist->stats->fanin_distribution, &netlist->stats->num_fanin_distribution, netlist->internal_nodes[i]->num_input_pins);
	}

	/* CLOCKS nnode_t** clocks;
	int num_clocks; */

	/* calculate the arithmetic average */
	netlist->stats->average_fanin = (float)(fanin_total)/(float)(netlist->num_top_output_nodes+netlist->num_ff_nodes+ netlist->num_internal_nodes);
	netlist->stats->num_edges_fi = fanin_total;
}

/*---------------------------------------------------------------------------------------------
 * (function: calculate_combinational_shapes()
 * 	Shape is defined by M. Hutton as the 0..delay(N) where shape[d] is the number of nodes
 *	with delay d in that part of the sequential level.  Note: our forward level is delay
 *	and shapes are essentially distributions.
 *-------------------------------------------------------------------------------------------*/
void calculate_combinational_shapes(netlist_t *netlist)
{
	int i;

	netlist->stats->combinational_shape = (int **)malloc(sizeof(int*)*netlist->num_sequential_levels);
	netlist->stats->num_combinational_shape_for_sequential_level = (int *)malloc(sizeof(int)*netlist->num_sequential_levels);

	/* initializ the shape records */
	for (i = 0; i < netlist->num_sequential_levels; i++)
	{
		netlist->stats->num_combinational_shape_for_sequential_level = 0;
		netlist->stats->combinational_shape = NULL;
	}

	/* COBINATIONAL nnode_t** internal_nodes;
	int num_internal_nodes; */
	for (i = 0; i < netlist->num_internal_nodes; i++)
	{
		/* add the delay to the sequential level shape */
		add_to_distribution(&netlist->stats->combinational_shape[netlist->internal_nodes[i]->sequential_level], &netlist->stats->num_combinational_shape_for_sequential_level[netlist->internal_nodes[i]->sequential_level], netlist->internal_nodes[i]->forward_level);
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traverse_stats()
 *-------------------------------------------------------------------------------------------*/
void depth_first_traverse_stats(FILE *out, short marker_value, netlist_t *netlist)
{
	int i;

	/* start with the primary input list */
	for (i = 0; i < netlist->num_top_input_nodes; i++)
	{
		if (netlist->top_input_nodes[i] != NULL)
		{
			depth_first_traversal_graphcrunch_stats(netlist->top_input_nodes[i], out, marker_value);
		}
	}
	/* now traverse the ground and vcc pins */
	if (netlist->gnd_node != NULL)
		depth_first_traversal_graphcrunch_stats(netlist->gnd_node, out, marker_value);
	if (netlist->vcc_node != NULL)
		depth_first_traversal_graphcrunch_stats(netlist->vcc_node, out, marker_value);
}

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traverse)
 *-------------------------------------------------------------------------------------------*/
void depth_first_traversal_graphcrunch_stats(nnode_t *node, FILE *fp, int traverse_mark_number)
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
		char *temp_string;
		char *temp_string2;

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

				//temp_string = make_string_based_on_id(node);
				//temp_string2 = make_string_based_on_id(next_node);
				temp_string = make_simple_name(node->name, "^-+.", '_');
				temp_string2 = make_simple_name(next_node->name, "^-+.", '_');

				if (node->type == OUTPUT_NODE)
				{
					/* renaming for output nodes */
					temp_string = (char*)realloc(temp_string, sizeof(char)*strlen(temp_string)+1+2);
					sprintf(temp_string, "%s_O", temp_string);
				}
				if (next_node->type == OUTPUT_NODE)
				{
					/* renaming for output nodes */
					temp_string2 = (char*)realloc(temp_string2, sizeof(char)*strlen(temp_string2)+1+2);
					sprintf(temp_string2, "%s_O", temp_string2);
				}

				fprintf(fp, "%s\t%s\n", temp_string, temp_string2);

				free(temp_string);
				free(temp_string2);

				/* recursive call point */
				depth_first_traversal_graphcrunch_stats(next_node, fp, traverse_mark_number);
			}
		}
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: num_fanouts_on_output_pin )
 *-------------------------------------------------------------------------------------------*/
int num_fanouts_on_output_pin(nnode_t *node, int output_pin_idx)
{
	nnet_t *next_net;
	int return_value = 0;
	int j;

	if (node->output_pins[output_pin_idx]->net == NULL)
		return 0;

	next_net = node->output_pins[output_pin_idx]->net;
	for (j = 0; j < next_net->num_fanout_pins; j++)
	{
		if (next_net->fanout_pins[j] == NULL)
			continue;

		if (next_net->fanout_pins[j]->node == NULL)
			continue;

		return_value++;
	}

	return return_value;
}

/*---------------------------------------------------------------------------------------------
 * (function: add_to_distribution()
 *-------------------------------------------------------------------------------------------*/
void add_to_distribution(int **distrib_ptr, int *distrib_size, int new_element)
{
	if (new_element < (*distrib_size))
	{
		/* IF - the distribution is already big enough */
		(*distrib_ptr)[new_element]++;
	}
	else
	{
		(*distrib_size)++;
		(*distrib_ptr) = (int*)realloc((*distrib_ptr), sizeof(int)*(new_element+1));
		(*distrib_ptr)[new_element]++;
	}
}


