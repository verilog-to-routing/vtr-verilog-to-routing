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
#include "types.h"
#include "globals.h"
#include "netlist_utils.h"
#include "odin_util.h"

void depth_first_traverse_visualize(nnode_t *node, FILE *fp, int traverse_mark_number);
void depth_first_traversal_graph_display(FILE *out, short marker_value, netlist_t *netllist);

void forward_traversal_net_graph_display(FILE *out, short marker_value, nnode_t *node);
void backward_traversal_net_graph_display(FILE *out, short marker_value, nnode_t *node);

/*---------------------------------------------------------------------------------------------
 * (function: graphVizOutputNetlist)
 *-------------------------------------------------------------------------------------------*/
void graphVizOutputNetlist(char* path, char* name, short marker_value, netlist_t *netlist)
{
	char path_and_file[4096];
	FILE *fp;

	/* open the file */
	sprintf(path_and_file, "%s/%s.dot", path, name);
	fp = fopen(path_and_file, "w");

        /* open graph */
        fprintf(fp, "digraph G {\n\tranksep=.25;\n");
	
	depth_first_traversal_graph_display(fp, marker_value, netlist);

        /* close graph */
        fprintf(fp, "}\n");
	fclose(fp);
}

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traversal_start()
 *-------------------------------------------------------------------------------------------*/
void depth_first_traversal_graph_display(FILE *out, short marker_value, netlist_t *netlist)
{
	int i;

	/* start with the primary input list */
	for (i = 0; i < netlist->num_top_input_nodes; i++)
	{
		if (netlist->top_input_nodes[i] != NULL)
		{
			depth_first_traverse_visualize(netlist->top_input_nodes[i], out, marker_value);
		}
	}
	/* now traverse the ground and vcc pins */
	if (netlist->gnd_node != NULL)
		depth_first_traverse_visualize(netlist->gnd_node, out, marker_value);
	if (netlist->vcc_node != NULL)
		depth_first_traverse_visualize(netlist->vcc_node, out, marker_value);
}

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traverse)
 *-------------------------------------------------------------------------------------------*/
void depth_first_traverse_visualize(nnode_t *node, FILE *fp, int traverse_mark_number)
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
		
		temp_string = make_simple_name(node->name, "^-+.", '_');
		if ((node->type == FF_NODE) || (node->type == BUF_NODE))
		{
			fprintf(fp, "\t\"%s\" [shape=box];\n", temp_string);
		}
		else if (node->type == INPUT_NODE)
		{
			fprintf(fp, "\t\"%s\" [shape=triangle];\n", temp_string);
		}
		else if (node->type == CLOCK_NODE)
		{
			fprintf(fp, "\t\"%s\" [shape=triangle];\n", temp_string);
		}
		else if (node->type == OUTPUT_NODE)
		{
			fprintf(fp, "\t\"%s_O\" [shape=triangle];\n", temp_string);
		}
		else
		{
			fprintf(fp, "\t\"%s\"\n", temp_string);
		}
		free(temp_string);

		for (i = 0; i < node->num_output_pins; i++)
		{
			if (node->output_pins[i]->net == NULL)
				continue;


			next_net = node->output_pins[i]->net;
			for (j = 0; j < next_net->num_fanout_pins; j++)
			{
				next_node = next_net->fanout_pins[j]->node;
				if (next_node == NULL)
					continue;
// To see just combinational stuff...also comment above triangels and box
//				if ((next_node->type == FF_NODE) || (next_node->type == INPUT_NODE) || (next_node->type == OUTPUT_NODE))
//					continue;
//				if ((node->type == FF_NODE) || (node->type == INPUT_NODE) || (node->type == OUTPUT_NODE))
//					continue;

				temp_string = make_simple_name(node->name, "^-+.", '_');
				temp_string2 = make_simple_name(next_node->name, "^-+.", '_');
				/* renaming for output nodes */
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

				fprintf(fp, "\t\"%s\" -> \"%s\"", temp_string, temp_string2); 
				if (next_net->fanout_pins[j]->name)
					fprintf(fp, "[label=\"%s\"]", next_net->fanout_pins[j]->name);
				fprintf(fp, ";\n");

				free(temp_string);
				free(temp_string2);

				/* recursive call point */
				depth_first_traverse_visualize(next_node, fp, traverse_mark_number);
			}
		}
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: graphVizOutputCobinationalNet)
 *-------------------------------------------------------------------------------------------*/
void graphVizOutputCombinationalNet(char* path, char* name, short marker_value, nnode_t *current_node)
{
	char path_and_file[4096];
	FILE *fp;

	/* open the file */
	sprintf(path_and_file, "%s/%s.dot", path, name);
	fp = fopen(path_and_file, "w");

        /* open graph */
        fprintf(fp, "digraph G {\n\tranksep=.25;\n");
	
	forward_traversal_net_graph_display(fp, marker_value, current_node);
	backward_traversal_net_graph_display(fp, marker_value, current_node);

        /* close graph */
        fprintf(fp, "}\n");
	fclose(fp);
}

/*---------------------------------------------------------------------------------------------
 * (function: forward_traversal_net_graph_display()
 *-------------------------------------------------------------------------------------------*/
void forward_traversal_net_graph_display(FILE *fp, short marker_value, nnode_t *node)
{
	int j, k;
	nnode_t** stack_of_nodes;
	int index_in_stack = 0;
	int num_stack_of_nodes = 1;
	char *temp_string;
	char *temp_string2;

	stack_of_nodes = (nnode_t**)malloc(sizeof(nnode_t*)*1);
	stack_of_nodes[0] = node;

	while (index_in_stack != num_stack_of_nodes)
	{
		nnode_t *current_node = stack_of_nodes[index_in_stack];

		/* mark it */
		current_node->traverse_visited = marker_value;
		
		/* printout the details of it */
		temp_string = make_simple_name(current_node->name, "^-+.", '_');
		if (index_in_stack == 0)
		{
			fprintf(fp, "\t%s [shape=box,color=red];\n", temp_string);
		}
		else if ((current_node->type == FF_NODE) || (current_node->type == BUF_NODE))
		{
			fprintf(fp, "\t%s [shape=box];\n", temp_string);
		}
		else if (current_node->type == INPUT_NODE)
		{
			fprintf(fp, "\t%s [shape=triangle];\n", temp_string);
		}
		else if (current_node->type == CLOCK_NODE)
		{
			fprintf(fp, "\t%s [shape=triangle];\n", temp_string);
		}
		else if (current_node->type == OUTPUT_NODE)
		{
			fprintf(fp, "\t%s_O [shape=triangle];\n", temp_string);
		}
		else
		{
			fprintf(fp, "\t%s [label=\"%d:%d\"];\n", temp_string, current_node->forward_level, current_node->backward_level);
		}
		free(temp_string);

		/* at each node visit all the outputs */
		for (j = 0; j < current_node->num_output_pins; j++)
		{
			if (current_node->output_pins[j] == NULL)
				continue; 

			for (k = 0; k < current_node->output_pins[j]->net->num_fanout_pins; k++)
			{
				if ((current_node->output_pins[j] == NULL) || (current_node->output_pins[j]->net == NULL) || ( current_node->output_pins[j]->net->fanout_pins[k] == NULL))
					continue;

				/* visit the fanout point */
				nnode_t *next_node = current_node->output_pins[j]->net->fanout_pins[k]->node;

				if (next_node == NULL)
					continue;

				temp_string = make_simple_name(current_node->name, "^-+.", '_');
				temp_string2 = make_simple_name(next_node->name, "^-+.", '_');
				if (current_node->type == OUTPUT_NODE)
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

				fprintf(fp, "\t%s -> %s [label=\"%s\"];\n", temp_string, temp_string2, current_node->output_pins[j]->net->fanout_pins[k]->name);

				free(temp_string);
				free(temp_string2);

				if ((next_node->traverse_visited != marker_value) && (next_node->type != FF_NODE))
				{
					/* IF - not visited yet then add to list */
					stack_of_nodes = (nnode_t**)realloc(stack_of_nodes, sizeof(nnode_t*)*(num_stack_of_nodes+1));
					stack_of_nodes[num_stack_of_nodes] = next_node;
					num_stack_of_nodes ++;
				}
			}
		}

		/* process next element in net */
		index_in_stack ++;
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: backward_traversal_net_graph_display()
 *-------------------------------------------------------------------------------------------*/
void backward_traversal_net_graph_display(FILE *fp, short marker_value, nnode_t *node)
{
	int j;
	char *temp_string;
	char *temp_string2;
	nnode_t** stack_of_nodes;
	int index_in_stack = 0;
	int num_stack_of_nodes = 1;

	stack_of_nodes = (nnode_t**)malloc(sizeof(nnode_t*)*1);
	stack_of_nodes[0] = node;

	while (index_in_stack != num_stack_of_nodes)
	{
		nnode_t *current_node = stack_of_nodes[index_in_stack];

		/* mark it */
		current_node->traverse_visited = marker_value;
		
		/* printout the details of it */
		temp_string = make_simple_name(current_node->name, "^-+.", '_');
		if (index_in_stack != 0)
		{
			if ((current_node->type == FF_NODE) || (current_node->type == BUF_NODE))
			{
				fprintf(fp, "\t%s [shape=box];\n", temp_string);
			}
			else if (current_node->type == INPUT_NODE)
			{
				fprintf(fp, "\t%s [shape=triangle];\n", temp_string);
			}
			else if (current_node->type == CLOCK_NODE)
			{
				fprintf(fp, "\t%s [shape=triangle];\n", temp_string);
			}
			else if (current_node->type == OUTPUT_NODE)
			{
				fprintf(fp, "\t%s_O [shape=triangle];\n", temp_string);
			}
			else
			{
				fprintf(fp, "\t%s [label=\"%d:%d\"];\n", temp_string, current_node->forward_level, current_node->backward_level);
			}
		}
		free(temp_string);

		/* at each node visit all the outputs */
		for (j = 0; j < current_node->num_input_pins; j++)
		{
			if (current_node->input_pins[j] == NULL)
				continue; 

			if ((current_node->input_pins[j] == NULL) || (current_node->input_pins[j]->net == NULL) || (current_node->input_pins[j]->net->driver_pin == NULL))
				continue;

			/* visit the fanout point */
			nnode_t *next_node = current_node->input_pins[j]->net->driver_pin->node;

			if (next_node == NULL)
				continue;

			temp_string = make_simple_name(current_node->name, "^-+.", '_');
			temp_string2 = make_simple_name(next_node->name, "^-+.", '_');

			fprintf(fp, "\t%s -> %s [label=\"%s\"];\n", temp_string2, temp_string, current_node->input_pins[j]->name);

			free(temp_string);
			free(temp_string2);

			if ((next_node->traverse_visited != marker_value) && (next_node->type != FF_NODE))
			{
				/* IF - not visited yet then add to list */
				stack_of_nodes = (nnode_t**)realloc(stack_of_nodes, sizeof(nnode_t*)*(num_stack_of_nodes+1));
				stack_of_nodes[num_stack_of_nodes] = next_node;
				num_stack_of_nodes ++;
			}
		}

		/* process next element in net */
		index_in_stack ++;
	}
}
