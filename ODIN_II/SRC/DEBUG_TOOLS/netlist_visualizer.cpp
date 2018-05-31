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
#include "vtr_memory.h"

void print_vizualizer_node_type_to_file(int index_in_stack, nnode_t *current_node, FILE *fp);
void print_vizualizer_nodes_to_file(int i, int j, short input, nnode_t *current_node, nnode_t *next_node, FILE *fp);

void depth_first_traverse_visualize(nnode_t *node, FILE *fp, int traverse_mark_number);
void depth_first_traversal_graph_display(FILE *out, short marker_value, netlist_t *netllist);

void forward_traversal_net_graph_display(FILE *out, short marker_value, nnode_t *node);
void backward_traversal_net_graph_display(FILE *out, short marker_value, nnode_t *node);

void graphVizOutputNetlist(std::string path, const char* name, short marker_value, netlist_t *netlist);
void graphVizOutputCombinationalNet(std::string path, const char* name, short marker_value, nnode_t *current_node);

/*---------------------------------------------------------------------------------------------
 * (function: graphVizOutputNetlist)
 *-------------------------------------------------------------------------------------------*/
void graphVizOutputNetlist(std::string path, const char* name, short marker_value, netlist_t *netlist)
{
	char path_and_file[4096];
	FILE *fp;

	/* open the file */
	sprintf(path_and_file, "%s/%s.dot", path.c_str(), name);
	fp = fopen(path_and_file, "w");

        /* open graph */
        fprintf(fp, "digraph G {\n\tranksep=.25;\n");

	depth_first_traversal_graph_display(fp, marker_value, netlist);

        /* close graph */
        fprintf(fp, "}\n");
	fclose(fp);
}

void print_vizualizer_node_type_to_file(int index_in_stack, nnode_t *current_node, FILE *fp)
{
	std::string temp_string = make_simple_name(current_node->name, "^-+.", '_');
	if (index_in_stack == 0)
	{
		fprintf(fp, "\t%s [shape=box,color=red];\n", temp_string.c_str());
	}
	else
	{
		switch(current_node->type)
		{
			case FF_NODE :	//fallthrough
			case BUF_NODE:		fprintf(fp, "\t%s [shape=box];\n", temp_string.c_str());	return;
			case INPUT_NODE:	fprintf(fp, "\t%s [shape=triangle];\n", temp_string.c_str());	return;
			case CLOCK_NODE:	fprintf(fp, "\t%s [shape=triangle];\n", temp_string.c_str());	return;
			case OUTPUT_NODE:	fprintf(fp, "\t%s_O [shape=triangle];\n", temp_string.c_str());	return;
			default:					fprintf(fp, "\t%s [label=\"%d:%d\"];\n", temp_string.c_str(), current_node->forward_level, current_node->backward_level); return;

		}
	}
}

void print_vizualizer_nodes_to_file(int i, int j, short input, nnode_t *current_node, nnode_t *next_node, FILE *fp)
{
	if(!current_node || !next_node)
		return;

	/* renaming for output nodes */
	std::string cur_str = make_simple_name(current_node->name, "^-+.", '_');
	cur_str += (!input && current_node->type == OUTPUT_NODE)?"_O":"";

	std::string next_str = make_simple_name(next_node->name, "^-+.", '_');
	next_str +=	(!input && next_node->type == OUTPUT_NODE)?"_O":"";

	if(input)
	{
		fprintf(fp, "\t\"%s\" -> \"%s\"", next_str.c_str(), cur_str.c_str());
		if (current_node->input_pins[j]->name)
			fprintf(fp, "[label=\"%s\"]", current_node->input_pins[j]->name);
	}
	else
	{
		fprintf(fp, "\t\"%s\" -> \"%s\"", cur_str.c_str(), next_str.c_str());
		if (current_node->output_pins[i]->net->fanout_pins[j]->name)
			fprintf(fp, "[label=\"%s\"]", current_node->output_pins[i]->net->fanout_pins[j]->name);
	}
	fprintf(fp, ";\n");
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
void depth_first_traverse_visualize(nnode_t *current_node, FILE *fp, int traverse_mark_number)
{
	int i, j;
	nnode_t *next_node;

	if (current_node->traverse_visited == traverse_mark_number)
	{
		return;
	}
	else
	{
		/* mark that we have visitied this node now */
		current_node->traverse_visited = traverse_mark_number;
		print_vizualizer_node_type_to_file(1,current_node,fp);

		for (i = 0; i < current_node->num_output_pins; i++)
		{
			if (!current_node->output_pins[i] \
			|| !current_node->output_pins[i]->net)
				continue;

			for (j = 0 ;j < current_node->output_pins[i]->net->num_fanout_pins ;j++)
			{
				if (!current_node->output_pins[i]->net->fanout_pins[j] \
				|| !current_node->output_pins[i]->net->fanout_pins[j]->node)
					continue;

				next_node = current_node->output_pins[i]->net->fanout_pins[j]->node;

				print_vizualizer_nodes_to_file(i,j, 0, current_node, next_node, fp);
				/* recursive call point */
				depth_first_traverse_visualize(next_node, fp, traverse_mark_number);
			}
		}
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: graphVizOutputCobinationalNet)
 *-------------------------------------------------------------------------------------------*/
void graphVizOutputCombinationalNet(std::string path, const char* name, short marker_value, nnode_t *current_node)
{
	char path_and_file[4096];
	FILE *fp;

	/* open the file */
	sprintf(path_and_file, "%s/%s.dot", path.c_str(), name);
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
	std::vector<nnode_t*> stack_of_nodes;
	stack_of_nodes.push_back(node);
	for(int i=0; i < stack_of_nodes.size(); i++)
	{
		stack_of_nodes[i]->traverse_visited = marker_value;
		print_vizualizer_node_type_to_file(i,node,fp);

		/* at each node visit all the outputs */
		for (int j = 0; j < stack_of_nodes[i]->num_output_pins; j++)
		{
			if (stack_of_nodes[i]->output_pins[j] == NULL\
			|| stack_of_nodes[i]->output_pins[j]->net == NULL)
				continue;

			for (int k = 0; k < stack_of_nodes[i]->output_pins[j]->net->num_fanout_pins; k++)
			{
				if(!stack_of_nodes[i]->output_pins[j]->net->fanout_pins[k]\
				|| !stack_of_nodes[i]->output_pins[j]->net->fanout_pins[k]->node)
					continue;

				/* visit the fanout point */
				nnode_t *next_node = stack_of_nodes[i]->output_pins[j]->net->fanout_pins[k]->node;
				if ((next_node->traverse_visited != marker_value) && (next_node->type != FF_NODE))
					stack_of_nodes.push_back(next_node);

				print_vizualizer_nodes_to_file(j,k,0, stack_of_nodes[i], next_node, fp);
			}
		}
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: backward_traversal_net_graph_display()
 *-------------------------------------------------------------------------------------------*/
void backward_traversal_net_graph_display(FILE *fp, short marker_value, nnode_t *node)
{
	std::vector<nnode_t*> stack_of_nodes;
	stack_of_nodes.push_back(node);
	for (int i=0; i != stack_of_nodes.size(); i++)
	{
		/* mark it */
		stack_of_nodes[i]->traverse_visited = marker_value;
		print_vizualizer_node_type_to_file(i,node,fp);

		/* at each node visit all the outputs */
		for (int j = 0; j < stack_of_nodes[i]->num_input_pins; j++)
		{
			if(!stack_of_nodes[i]->input_pins[j]\
			|| !stack_of_nodes[i]->input_pins[j]->net \
			|| !stack_of_nodes[i]->input_pins[j]->net->driver_pin \
			|| !stack_of_nodes[i]->input_pins[j]->net->driver_pin->node)
				continue;

			/* visit the fanout point */
			nnode_t *next_node = stack_of_nodes[i]->input_pins[j]->net->driver_pin->node;
			if ((next_node->traverse_visited != marker_value) && (next_node->type != FF_NODE))
				stack_of_nodes.push_back(next_node);

			print_vizualizer_nodes_to_file(i,j,1, stack_of_nodes[i], next_node, fp);
		}
	}
}
