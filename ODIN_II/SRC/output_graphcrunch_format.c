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

void depth_first_traverse_graphcrunch(FILE *out, short marker_value, netlist_t *netlist);
void depth_first_traversal_graphcrunch_display(nnode_t *node, FILE *fp, int traverse_mark_number);

/*---------------------------------------------------------------------------------------------
 * (function: graphVizOutputAst)
 *-------------------------------------------------------------------------------------------*/
void graphcrunch_output(char* path, char* name, short marker_value, netlist_t *netlist)
{
	char path_and_file[4096];
	FILE *fp;

	/* open the file */
	sprintf(path_and_file, "%s/%s", path, name);
	fp = fopen(path_and_file, "w");

	depth_first_traverse_graphcrunch(fp, marker_value, netlist);

        /* close graph */
	fclose(fp);
}

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traverse_graphcrunch()
 *-------------------------------------------------------------------------------------------*/
void depth_first_traverse_graphcrunch(FILE *out, short marker_value, netlist_t *netlist)
{
	int i;

	/* start with the primary input list */
	for (i = 0; i < netlist->num_top_input_nodes; i++)
	{
		if (netlist->top_input_nodes[i] != NULL)
		{
			depth_first_traversal_graphcrunch_display(netlist->top_input_nodes[i], out, marker_value);
		}
	}
	/* now traverse the ground and vcc pins */
	if (netlist->gnd_node != NULL)
		depth_first_traversal_graphcrunch_display(netlist->gnd_node, out, marker_value);
	if (netlist->vcc_node != NULL)
		depth_first_traversal_graphcrunch_display(netlist->vcc_node, out, marker_value);
}

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traverse)
 *-------------------------------------------------------------------------------------------*/
void depth_first_traversal_graphcrunch_display(nnode_t *node, FILE *fp, int traverse_mark_number)
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
				depth_first_traversal_graphcrunch_display(next_node, fp, traverse_mark_number);
			}
		}
	}
}
