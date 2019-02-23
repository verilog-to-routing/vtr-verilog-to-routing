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
#include "odin_types.h"
#include "odin_globals.h"

#include "netlist_utils.h"
#include "odin_util.h"
#include "ast_util.h"
#include "string_cache.h"
#include "netlist_check.h"
#include "netlist_visualizer.h"
#include "vtr_memory.h"

void levelize_backwards(netlist_t *netlist);
void levelize_backwards_clean_checking_for_liveness(short ast_based, netlist_t *netlist);
void levelize_forwards(netlist_t *netlist);
void levelize_forwards_clean_checking_for_combo_loop_and_liveness(short ast_based, netlist_t *netlist);
nnode_t *find_node_at_top_of_combo_loop(nnode_t *start_node);
void depth_first_traversal_check_if_forward_leveled(short marker_value, netlist_t *netlist);
void depth_first_traverse_check_if_forward_leveled(nnode_t *node, int traverse_mark_number);

void sequential_levelized_dfs(short marker_value, netlist_t *netlist);
void depth_first_traverse_until_next_ff_or_output(nnode_t *node, nnode_t *calling_node, int traverse_mark_number, int seq_level, netlist_t *netlist);

/*---------------------------------------------------------------------------------------------
 * (function: check_netlist)
 * Note: netlist passed in needs to be initialized by allocate_netlist() to make sure correctly initialized.
 *-------------------------------------------------------------------------------------------*/
void check_netlist(netlist_t *netlist)
{
	/* create a graph output of this netlist */
	if (configuration.output_netlist_graphs )
	{
		/* Path is where we are */
		graphVizOutputNetlist(configuration.debug_output_path, "net", 1, netlist);
	}
}

void depth_traverse_check_combinational_loop(nnode_t *node, short start, STRING_CACHE *in_path);
/*---------------------------------------------------------------------------------------------
 * (function: check_for_combinational_loop_and_liveness)
 *-------------------------------------------------------------------------------------------*/
void levelize_and_check_for_combinational_loop_and_liveness(short ast_based, netlist_t *netlist)
{
	/* go from the POs backwards and mark level */
	levelize_backwards(netlist);
	/* Since the net_data (void pointer) is used to record information, we need to clean this up so that it can be used again
	   also, during this cleaning we check if nodes are live based on all the fanout pins have been visited */
	levelize_backwards_clean_checking_for_liveness(ast_based, netlist);

	/* do a forward analysis */
	levelize_forwards(netlist);

	/* checks if there are any non-forward marked nodes */
	depth_first_traversal_check_if_forward_leveled(COMBO_LOOP, netlist);
	/* finds combo loops, but usually killed by previous.  Also cleans out net_data (void pointer) for next algorithm. */
	levelize_forwards_clean_checking_for_combo_loop_and_liveness(ast_based, netlist);

	/* assign each node which sequential level it is in, and keep the primary inputs to that level */
	sequential_levelized_dfs(SEQUENTIAL_LEVELIZE, netlist);
}

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traversal_check_if_forward_leveled()
 *-------------------------------------------------------------------------------------------*/
void sequential_levelized_dfs(short marker_value, netlist_t *netlist)
{
	int i;

	int sequential_level = 0;
	netlist->num_sequential_levels = 1;
	netlist->num_at_sequential_level = (int*)vtr::realloc(netlist->num_at_sequential_level, sizeof(int)*netlist->num_sequential_levels);
	netlist->sequential_level_nodes = (nnode_t***)vtr::realloc(netlist->sequential_level_nodes, sizeof(nnode_t**)*(netlist->num_sequential_levels));
	netlist->sequential_level_nodes[netlist->num_sequential_levels-1] = NULL;
	netlist->num_at_sequential_level[netlist->num_sequential_levels-1] = 0;

	/* allocate the first list.  Includes vcc and gnd */
	netlist->sequential_level_nodes[sequential_level] = (nnode_t**)vtr::realloc(netlist->sequential_level_nodes[sequential_level], sizeof(nnode_t*)*(netlist->num_top_input_nodes+2));

	/* add all the primary nodes to the first level */
	for (i = 0; i < netlist->num_top_input_nodes; i++)
	{
		if (netlist->top_input_nodes[i] != NULL)
		{
			netlist->sequential_level_nodes[sequential_level][i] = netlist->top_input_nodes[i];
			netlist->num_at_sequential_level[sequential_level]++;
			/* record the level */
			netlist->top_input_nodes[i]->sequential_level = sequential_level;
		}
	}

	/* now traverse the ground and vcc pins */
	if (netlist->gnd_node != NULL)
	{
		netlist->sequential_level_nodes[sequential_level][i] = netlist->gnd_node;
		netlist->num_at_sequential_level[sequential_level]++;
		/* record the level */
		netlist->gnd_node->sequential_level = sequential_level;
	}
	if (netlist->vcc_node != NULL)
	{
		netlist->sequential_level_nodes[sequential_level][i+1] = netlist->vcc_node;
		netlist->num_at_sequential_level[sequential_level]++;
		/* record the level */
		netlist->vcc_node->sequential_level = sequential_level;
	}

	while (netlist->num_at_sequential_level[sequential_level] > 0)
	{
		/* WHILE there are PIs at this level */

		/* Allocate the next level of storage since this part is a forward thing of the next flip-flops at the level */
		/* add anothersequential level.  Note, needs to be done before we depth first the current combinational level. */
		netlist->num_sequential_levels ++;
		netlist->sequential_level_nodes = (nnode_t***)vtr::realloc(netlist->sequential_level_nodes, sizeof(nnode_t**)*(netlist->num_sequential_levels));
		netlist->num_at_sequential_level = (int*)vtr::realloc(netlist->num_at_sequential_level, sizeof(int)*netlist->num_sequential_levels);
		netlist->sequential_level_nodes[netlist->num_sequential_levels-1] = NULL;
		netlist->num_at_sequential_level[netlist->num_sequential_levels-1] = 0;

		/* deals with recording the combinational nodes that terminate this level */
		netlist->num_sequential_level_combinational_termination_nodes ++;
		netlist->sequential_level_combinational_termination_node = (nnode_t***)vtr::realloc(netlist->sequential_level_combinational_termination_node, sizeof(nnode_t**)*(netlist->num_sequential_level_combinational_termination_nodes));
		netlist->num_at_sequential_level_combinational_termination_node = (int*)vtr::realloc(netlist->num_at_sequential_level_combinational_termination_node, sizeof(int)*netlist->num_sequential_level_combinational_termination_nodes);
		netlist->sequential_level_combinational_termination_node[netlist->num_sequential_level_combinational_termination_nodes-1] = NULL;
		netlist->num_at_sequential_level_combinational_termination_node[netlist->num_sequential_level_combinational_termination_nodes-1] = 0;

		/* go through the entire list, mark with sequential level, and build the next list */
		for (i = 0; i < netlist->num_at_sequential_level[sequential_level]; i++)
		{
			depth_first_traverse_until_next_ff_or_output(netlist->sequential_level_nodes[sequential_level][i], NULL, marker_value, sequential_level, netlist);
		}

		/* now potentially do next sequential level */
		sequential_level ++;
	}

}

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traverse_until_next_ff_or_output)
 *-------------------------------------------------------------------------------------------*/
void depth_first_traverse_until_next_ff_or_output(nnode_t *node, nnode_t *calling_node, int traverse_mark_number, int seq_level, netlist_t *netlist)
{
	int i, j;
	nnode_t *next_node;
	nnet_t *next_net;

	/* first, check if the clalling node should be recorderd */
	if ((calling_node != NULL) && ((node->type == FF_NODE) || (node->type == OUTPUT_NODE)))
	{
		/* IF - the this node is the end of a sequential level then the node before needs to be stored */
		if (calling_node->sequential_terminator == FALSE)
		{
			/* IF - it hasn't been stored before */
			netlist->num_at_sequential_level_combinational_termination_node[netlist->num_sequential_level_combinational_termination_nodes-1] ++;
			netlist->sequential_level_combinational_termination_node[netlist->num_sequential_level_combinational_termination_nodes-1] = (nnode_t**)vtr::realloc(netlist->sequential_level_combinational_termination_node[netlist->num_sequential_level_combinational_termination_nodes-1],sizeof(nnode_t*)*netlist->num_at_sequential_level_combinational_termination_node[netlist->num_sequential_level_combinational_termination_nodes-1]);
			netlist->sequential_level_combinational_termination_node[netlist->num_sequential_level_combinational_termination_nodes-1][netlist->num_at_sequential_level_combinational_termination_node[netlist->num_sequential_level_combinational_termination_nodes-1]-1] = calling_node;
			/* mark the node locally */
			calling_node->sequential_terminator = TRUE;
		}
	}

	if (node->traverse_visited == traverse_mark_number)
	{
		/* if already visited then nothing to do */
		return;
	}
	else if (node->type == CLOCK_NODE)
	{
		/* since this is a node that touches all flip flops, don't analyze for sequential level */
		return;
	}
	else if (node->type == FF_NODE)
	{
		/* ELSE IF - this is a ff_node, so add it to the list for the next sequential level */
		/* mark as traversed */
		node->traverse_visited = traverse_mark_number;
		node->sequential_level = seq_level+1;

		/* add to the next sequntial list */
		netlist->num_at_sequential_level[seq_level+1]++;
		netlist->sequential_level_nodes[seq_level+1] = (nnode_t**)vtr::realloc(netlist->sequential_level_nodes[seq_level+1], sizeof(nnode_t*)*netlist->num_at_sequential_level[seq_level+1]);
		netlist->sequential_level_nodes[seq_level+1][netlist->num_at_sequential_level[seq_level+1]-1] = node;

		return;
	}
	else
	{
		/* ELSE - this is a node so depth visit it */

		/* mark that we have visitied this node now */
		node->traverse_visited = traverse_mark_number;
		node->sequential_level = seq_level;

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
				depth_first_traverse_until_next_ff_or_output(next_node, node, traverse_mark_number, seq_level, netlist);
			}
		}
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traversal_check_if_forward_leveled()
 *-------------------------------------------------------------------------------------------*/
void depth_first_traversal_check_if_forward_leveled(short marker_value, netlist_t *netlist)
{
	int i;

	/* start with the primary input list */
	for (i = 0; i < netlist->num_top_input_nodes; i++)
	{
		if (netlist->top_input_nodes[i] != NULL)
		{
			depth_first_traverse_check_if_forward_leveled(netlist->top_input_nodes[i], marker_value);
		}
	}
	/* now traverse the ground and vcc pins */
	if (netlist->gnd_node != NULL)
		depth_first_traverse_check_if_forward_leveled(netlist->gnd_node, marker_value);
	if (netlist->vcc_node != NULL)
		depth_first_traverse_check_if_forward_leveled(netlist->vcc_node, marker_value);
}

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traverse)
 *-------------------------------------------------------------------------------------------*/
void depth_first_traverse_check_if_forward_leveled(nnode_t *node, int traverse_mark_number)
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

				if ((next_node->forward_level == -1) && (next_node->type != FF_NODE))
				{
					graphVizOutputCombinationalNet(configuration.debug_output_path, "combo_loop", COMBO_LOOP_ERROR, /*next_node);*/ find_node_at_top_of_combo_loop(next_node));
					oassert(FALSE);
				}

				/* recursive call point */
				depth_first_traverse_check_if_forward_leveled(next_node, traverse_mark_number);
			}
		}
	}
}
/*---------------------------------------------------------------------------------------------
 * (function: levelize_forwards)
 	Note that this levlizing is combinational delay levels where the assumption is that
	each node has a unit delay.
 *-------------------------------------------------------------------------------------------*/
void levelize_forwards(netlist_t *netlist)
{
	int i, j, k;
	int cur_for_level;
	short more_levels = TRUE;
	short all_visited = TRUE;

	/* add all the POs and FFs POs as forward level 0 */
	cur_for_level = 0;
	netlist->num_forward_levels = 1;
	netlist->num_at_forward_level = (int*)vtr::realloc(netlist->num_at_forward_level, sizeof(int)*netlist->num_forward_levels);
	netlist->forward_levels = (nnode_t***)vtr::realloc(netlist->forward_levels, sizeof(nnode_t**)*(netlist->num_forward_levels));
	netlist->forward_levels[netlist->num_forward_levels-1] = NULL;
	netlist->num_at_forward_level[netlist->num_forward_levels-1] = 0;
	for (i = 0; i < netlist->num_top_input_nodes+3; i++)
	{
		if ((i == netlist->num_top_input_nodes) && (netlist->vcc_node != NULL))
		{
			/* vcc */
			netlist->forward_levels[cur_for_level] = (nnode_t**)vtr::realloc(netlist->forward_levels[cur_for_level], sizeof(nnode_t*)*(netlist->num_at_forward_level[cur_for_level]+1));
			netlist->forward_levels[cur_for_level][netlist->num_at_forward_level[cur_for_level]] = netlist->vcc_node;
			netlist->num_at_forward_level[cur_for_level]++;
			netlist->vcc_node->forward_level = 0;
		}
		else if ((i == netlist->num_top_input_nodes+1) && (netlist->gnd_node != NULL))
		{
			/* gnd */
			netlist->forward_levels[cur_for_level] = (nnode_t**)vtr::realloc(netlist->forward_levels[cur_for_level], sizeof(nnode_t*)*(netlist->num_at_forward_level[cur_for_level]+1));
			netlist->forward_levels[cur_for_level][netlist->num_at_forward_level[cur_for_level]] = netlist->gnd_node;
			netlist->num_at_forward_level[cur_for_level]++;
			netlist->gnd_node->forward_level = 0;
		}
		else if ((i == netlist->num_top_input_nodes+2) && (netlist->pad_node != NULL))
		{
			/* pad */
			netlist->forward_levels[cur_for_level] = (nnode_t**)vtr::realloc(netlist->forward_levels[cur_for_level], sizeof(nnode_t*)*(netlist->num_at_forward_level[cur_for_level]+1));
			netlist->forward_levels[cur_for_level][netlist->num_at_forward_level[cur_for_level]] = netlist->pad_node;
			netlist->num_at_forward_level[cur_for_level]++;
			netlist->pad_node->forward_level = 0;
		}
		else if (i >= netlist->num_top_input_nodes)
		{
			continue;
		}
		else if (netlist->top_input_nodes[i] != NULL)
		{
			netlist->forward_levels[cur_for_level] = (nnode_t**)vtr::realloc(netlist->forward_levels[cur_for_level], sizeof(nnode_t*)*(netlist->num_at_forward_level[cur_for_level]+1));
			netlist->forward_levels[cur_for_level][netlist->num_at_forward_level[cur_for_level]] = netlist->top_input_nodes[i];
			netlist->num_at_forward_level[cur_for_level]++;
			netlist->top_input_nodes[i]->forward_level = 0;
		}
	}
	for (i = 0; i < netlist->num_ff_nodes; i++)
	{
		if (netlist->ff_nodes[i] != NULL)
		{
			netlist->forward_levels[cur_for_level] = (nnode_t**)vtr::realloc(netlist->forward_levels[cur_for_level], sizeof(nnode_t*)*(netlist->num_at_forward_level[cur_for_level]+1));
			netlist->forward_levels[cur_for_level][netlist->num_at_forward_level[cur_for_level]] = netlist->ff_nodes[i];
			netlist->num_at_forward_level[cur_for_level]++;
			netlist->ff_nodes[i]->forward_level = 0;
		}
	}

	while(more_levels)
	{
		/* another level so add space */
		netlist->num_forward_levels ++;
		netlist->num_at_forward_level = (int*)vtr::realloc(netlist->num_at_forward_level, sizeof(int)*netlist->num_forward_levels);
		netlist->forward_levels = (nnode_t***)vtr::realloc(netlist->forward_levels, sizeof(nnode_t**)*(netlist->num_forward_levels));
		netlist->forward_levels[netlist->num_forward_levels-1] = NULL;
		netlist->num_at_forward_level[netlist->num_forward_levels-1] = 0;

		/* go through each element at this level */
		for (i = 0; i < netlist->num_at_forward_level[cur_for_level]; i++)
		{
			nnode_t *current_node = netlist->forward_levels[cur_for_level][i];
			if (current_node == NULL)
				continue;

			/* at each node visit all the inputs */
			for (j = 0; j < current_node->num_output_pins; j++)
			{
				int *fanouts_visited;
				if (current_node->output_pins[j] == NULL)
					continue;

				for (k = 0; k < current_node->output_pins[j]->net->num_fanout_pins; k++)
				{
					int idx;
					/* visit the fanout point */
					if ((current_node->output_pins[j] == NULL) || (current_node->output_pins[j]->net == NULL) || ( current_node->output_pins[j]->net->fanout_pins[k] == NULL))
						continue;

					nnode_t *output_node = current_node->output_pins[j]->net->fanout_pins[k]->node;

					if (output_node == NULL)
						continue;

					if (output_node->node_data == NULL)
					{
						/* if this fanout hasn't been visited yet this will be null */
						fanouts_visited = (int*)vtr::malloc(sizeof(int)*(output_node->num_input_pins));

						for (idx = 0; idx < output_node->num_input_pins; idx++)
						{
							fanouts_visited[idx] = -1;
						}

						output_node->node_data = (void*)fanouts_visited;
						output_node->unique_node_data_id = LEVELIZE;
					}
					else
					{
						/* ELSE - get the list */
						oassert(output_node->unique_node_data_id == LEVELIZE);
						fanouts_visited = (int*)output_node->node_data;
					}

					/* mark this entry as visited */
					fanouts_visited[current_node->output_pins[j]->net->fanout_pins[k]->pin_node_idx] = cur_for_level;

					/* check if they've all been marked */
					all_visited = TRUE;
					for (idx = 0; idx < output_node->num_input_pins; idx++)
					{
						if (fanouts_visited[idx] == -1)
						{
							all_visited = FALSE;
							break;
						}
					}

					if ((all_visited == TRUE) && (output_node->type != FF_NODE))
					{
						/* This one has been visited by everyone */
						netlist->forward_levels[cur_for_level+1] = (nnode_t**)vtr::realloc(netlist->forward_levels[cur_for_level+1], sizeof(nnode_t*)*(netlist->num_at_forward_level[cur_for_level+1]+1));
						netlist->forward_levels[cur_for_level+1][netlist->num_at_forward_level[cur_for_level+1]] = output_node;
						netlist->num_at_forward_level[cur_for_level+1]++;

						output_node->forward_level = cur_for_level+1;
					}
				}
			}
		}

		/* check if tere are more elements to procees at the next level */
		if (netlist->num_at_forward_level[cur_for_level+1] > 0)
		{
			/* there are elements in the next set then process */
			cur_for_level++;
		}
		else
		{
			/* ELSE - we've levelized forwards */
			more_levels = FALSE;
		}
	}
}
/*---------------------------------------------------------------------------------------------
 * (function: levelize_forwards_clean_checking_for_combo_loop_and_liveness)
 *-------------------------------------------------------------------------------------------*/
void levelize_forwards_clean_checking_for_combo_loop_and_liveness(short ast_based, netlist_t *netlist)
{
	int i, j, k;
	int cur_for_level;
	short more_levels = TRUE;
	short all_visited = TRUE;

	cur_for_level = 0;

	while(more_levels)
	{
		/* go through each element at this level */
		for (i = 0; i < netlist->num_at_forward_level[cur_for_level]; i++)
		{
			nnode_t *current_node = netlist->forward_levels[cur_for_level][i];
			if (current_node == NULL)
				continue;

			/* at each node visit all the inputs */
			for (j = 0; j < current_node->num_output_pins; j++)
			{
				int *fanouts_visited;
				if (current_node->output_pins[j] == NULL)
					continue;

				for (k = 0; k < current_node->output_pins[j]->net->num_fanout_pins; k++)
				{
					if ((current_node->output_pins[j] == NULL) || (current_node->output_pins[j]->net == NULL) || ( current_node->output_pins[j]->net->fanout_pins[k] == NULL))
						continue;

					/* visit the fanout point */
					nnode_t *output_node = current_node->output_pins[j]->net->fanout_pins[k]->node;

					if (output_node == NULL)
						continue;

					if (output_node->node_data == NULL)
					{
						oassert(output_node->unique_node_data_id == RESET);
					}
					else
					{
						int idx;
						/* ELSE - get the list */
						oassert(output_node->unique_node_data_id == LEVELIZE);
						fanouts_visited = (int*)output_node->node_data;
						output_node->node_data = NULL;

						/* check if they've all been marked */
						all_visited = TRUE;
						for (idx = 0; idx < output_node->num_input_pins; idx++)
						{
							if (fanouts_visited[idx] == -1)
							{
								all_visited = FALSE;
								break;
							}
						}

						if (all_visited == FALSE)
						{
							/* Combo node since one of the outputs hasn'y been visisted. */
							if (ast_based)
								error_message(NETLIST_ERROR, output_node->related_ast_node->line_number, output_node->related_ast_node->file_number, "!!!Combinational loop on forward pass.  Node %s is missing a driven pin idx %ld.  Isn't neccessarily the culprit of the combinational loop.  Odin only detects combinational loops, but currently doesn't pinpoint.\n", output_node->name, idx);
							else
								error_message(NETLIST_ERROR, -1, -1, "!!!Combinational loop on forward pass.  Node %s is missing a driven pin idx %ld.  Isn't neccessarily the culprit of the combinational loop.  Odin only detects combinational loops, but currently doesn't pinpoint.\n", output_node->name, idx);
						}
						/* free the data and reset to be used elsewhere */
						vtr::free(fanouts_visited);
						output_node->unique_node_data_id = RESET;
					}

					if ((output_node->backward_level == -1) && (output_node->type != FF_NODE))
					{
						if (ast_based)
							warning_message(NETLIST_ERROR, output_node->related_ast_node->line_number, output_node->related_ast_node->file_number, "Node does not connect to a primary output or FF...DEAD NODE!!!.  Node %s is not connected to a primary output.\n", output_node->name);
						else
							warning_message(NETLIST_ERROR, -1, -1, "Node does not connect to a primary output or FF...DEAD NODE!!!.  Node %s is not connected to a primary output.\n", output_node->name);
					}
				}
			}
		}

		/* check if tere are more elements to procees at the next level */
		if (netlist->num_at_forward_level[cur_for_level+1] > 0)
		{
			/* there are elements in the next set then process */
			cur_for_level++;
		}
		else
		{
			/* ELSE - we've levelized forwards */
			more_levels = FALSE;
		}
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: levelize_backwards)
 	Note this levelizing is a reverse combinational delay count
 *-------------------------------------------------------------------------------------------*/
void levelize_backwards(netlist_t *netlist)
{
	int i, j, k;
	int cur_back_level;
	short more_levels = TRUE;
	short all_visited = TRUE;

	/* add all the POs and FFs POs as backward level 0 */
	cur_back_level = 0;
	netlist->num_backward_levels = 1;
	netlist->num_at_backward_level = (int*)vtr::realloc(netlist->num_at_backward_level, sizeof(int)*netlist->num_backward_levels);
	netlist->backward_levels = (nnode_t***)vtr::realloc(netlist->backward_levels, sizeof(nnode_t**)*(netlist->num_backward_levels));
	netlist->backward_levels[netlist->num_backward_levels-1] = NULL;
	netlist->num_at_backward_level[netlist->num_backward_levels-1] = 0;
	for (i = 0; i < netlist->num_top_output_nodes; i++)
	{
		if (netlist->top_output_nodes[i] != NULL)
		{
			netlist->backward_levels[cur_back_level] = (nnode_t**)vtr::realloc(netlist->backward_levels[cur_back_level], sizeof(nnode_t*)*(netlist->num_at_backward_level[cur_back_level]+1));
			netlist->backward_levels[cur_back_level][netlist->num_at_backward_level[cur_back_level]] = netlist->top_output_nodes[i];
			netlist->num_at_backward_level[cur_back_level]++;
			netlist->top_output_nodes[i]->backward_level = 0;
		}
	}
	for (i = 0; i < netlist->num_ff_nodes; i++)
	{
		if (netlist->ff_nodes[i] != NULL)
		{
			netlist->backward_levels[cur_back_level] = (nnode_t**)vtr::realloc(netlist->backward_levels[cur_back_level], sizeof(nnode_t*)*(netlist->num_at_backward_level[cur_back_level]+1));
			netlist->backward_levels[cur_back_level][netlist->num_at_backward_level[cur_back_level]] = netlist->ff_nodes[i];
			netlist->num_at_backward_level[cur_back_level]++;
			netlist->ff_nodes[i]->backward_level = 0;
		}
	}

	while(more_levels)
	{
		/* another level so add space */
		netlist->num_backward_levels ++;
		netlist->num_at_backward_level = (int*)vtr::realloc(netlist->num_at_backward_level, sizeof(int)*netlist->num_backward_levels);
		netlist->backward_levels = (nnode_t***)vtr::realloc(netlist->backward_levels, sizeof(nnode_t**)*(netlist->num_backward_levels));
		netlist->backward_levels[netlist->num_backward_levels-1] = NULL;
		netlist->num_at_backward_level[netlist->num_backward_levels-1] = 0;

		/* go through each element at this level */
		for (i = 0; i < netlist->num_at_backward_level[cur_back_level]; i++)
		{
			nnode_t *current_node = netlist->backward_levels[cur_back_level][i];
			if (current_node == NULL)
				continue;

			/* at each node visit all the inputs */
			for (j = 0; j < current_node->num_input_pins; j++)
			{
				int *fanouts_visited;
				if (current_node->input_pins[j] == NULL)
					continue;

				/* visit the fanout point */
				nnet_t *fanout_net = current_node->input_pins[j]->net;

				if (fanout_net->net_data == NULL)
				{
					int idx;
					/* if this fanout hasn't been visited yet this will be null */
					fanouts_visited = (int*)vtr::malloc(sizeof(int)*(fanout_net->num_fanout_pins));

					for (idx = 0; idx < fanout_net->num_fanout_pins; idx++)
					{
						fanouts_visited[idx] = -1;
					}

					fanout_net->net_data = (void*)fanouts_visited;
					fanout_net->unique_net_data_id = LEVELIZE;
				}
				else
				{
					/* ELSE - get the list */
					fanouts_visited = (int*)fanout_net->net_data;
					oassert(fanout_net->unique_net_data_id == LEVELIZE);
				}

				/* mark this entry as visited */
				if(fanout_net->driver_pin != NULL)
				{
					fanouts_visited[current_node->input_pins[j]->pin_net_idx] = cur_back_level;
				}

				/* check if they've all been marked */
				all_visited = TRUE;
				for (k = 0; k < fanout_net->num_fanout_pins; k++)
				{
					if ((fanout_net->fanout_pins[k] != NULL) && (fanout_net->fanout_pins[k]->node != NULL) && (fanouts_visited[k] == -1))
					{
						all_visited = FALSE;
						break;
					}
				}

				if ((all_visited == TRUE) && (fanout_net->driver_pin->node->type != FF_NODE))
				{
					/* This one has been visited by everyone */
					if (fanout_net->driver_pin->node->backward_level == -1)
					{
						/* already added to a list...this means that we won't have the correct ordering */
						netlist->backward_levels[cur_back_level+1] = (nnode_t**)vtr::realloc(netlist->backward_levels[cur_back_level+1], sizeof(nnode_t*)*(netlist->num_at_backward_level[cur_back_level+1]+1));
						netlist->backward_levels[cur_back_level+1][netlist->num_at_backward_level[cur_back_level+1]] = fanout_net->driver_pin->node;
						netlist->num_at_backward_level[cur_back_level+1]++;
					}

					fanout_net->driver_pin->node->backward_level = cur_back_level+1;
				}
			}
		}

		/* check if tere are more elements to procees at the next level */
		if (netlist->num_at_backward_level[cur_back_level+1] > 0)
		{
			/* there are elements in the next set then process */
			cur_back_level++;
		}
		else
		{
			/* ELSE - we've levelized backwards */
			more_levels = FALSE;
		}
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: levelize_backwards_clean_checking_for_liveness)
 *-------------------------------------------------------------------------------------------*/
void levelize_backwards_clean_checking_for_liveness(short ast_based, netlist_t *netlist)
{
	int i, j, k;
	int cur_back_level;
	short more_levels = TRUE;
	short all_visited = TRUE;

	cur_back_level = 0;

	while(more_levels)
	{
		/* go through each element at this level */
		for (i = 0; i < netlist->num_at_backward_level[cur_back_level]; i++)
		{
			nnode_t *current_node = netlist->backward_levels[cur_back_level][i];
			if (current_node == NULL)
				continue;

			/* at each node visit all the inputs */
			for (j = 0; j < current_node->num_input_pins; j++)
			{
				int *fanouts_visited;
				if (current_node->input_pins[j] == NULL)
					continue;

				/* visit the fanout point */
				nnet_t *fanout_net = current_node->input_pins[j]->net;

				if (fanout_net->net_data == NULL)
				{
					/* IF - already cleaned */
					oassert(fanout_net->unique_net_data_id == -1);
				}
				else
				{
					/* ELSE - get the list */
					oassert(fanout_net->unique_net_data_id == LEVELIZE);
					fanouts_visited = (int*)fanout_net->net_data;
					fanout_net->net_data = NULL;

					/* check if they've all been marked */
					all_visited = TRUE;
					for (k = 0; k < fanout_net->num_fanout_pins; k++)
					{
						if ((fanout_net->fanout_pins[k] != NULL) && (fanout_net->fanout_pins[k]->node != NULL) && (fanouts_visited[k] == -1))
						{
							all_visited = FALSE;
							break;
						}
					}

					if (all_visited == FALSE)
					{
						/* one of these nodes was not visited on the backward analysis */
						if (ast_based)
							warning_message(NETLIST_ERROR, current_node->related_ast_node->line_number, current_node->related_ast_node->file_number, "Liveness check on backward pass.  Node %s is missing a driving pin idx %ld\n", current_node->name, k);
						else
							warning_message(NETLIST_ERROR, -1, -1, "Liveness check on backward pass.  Node %s is missing a driving pin idx %ld\n", current_node->name, k);
					}

					/* free the data and reset to be used elsewhere */
					vtr::free(fanouts_visited);
					fanout_net->unique_net_data_id = -1;
				}
			}
		}

		/* check if tere are more elements to procees at the next level */
		if (netlist->num_at_backward_level[cur_back_level+1] > 0)
		{
			/* there are elements in the next set then process */
			cur_back_level++;
		}
		else
		{
			/* ELSE - we've levelized backwards */
			more_levels = FALSE;
		}
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: find_node_at_top_of_combo_loop)
 *-------------------------------------------------------------------------------------------*/
nnode_t *find_node_at_top_of_combo_loop(nnode_t *start_node)
{
	int i;
	int idx_missed;
	nnode_t *next_node = start_node;
	int *fanouts_visited;
	short all_visited;

	while(TRUE)
	{
		oassert(next_node->unique_node_data_id == LEVELIZE);
		fanouts_visited = (int*)next_node->node_data;
		next_node->node_data = NULL;

		/* check if they've all been marked */
		all_visited = TRUE;
		for (i = 0; i < next_node->num_input_pins; i++)
		{
			if (fanouts_visited[i] == -1)
			{
				all_visited = FALSE;
				idx_missed = i;
				break;
			}
		}

		if (all_visited == FALSE)
		{
			if (next_node->input_pins[idx_missed]->net->driver_pin->node->backward_level < next_node->backward_level)
				/* IF - the next node has a lower backward level than this node suggests that it is
				   closer to primary outputs and not in the combo loop */
				return next_node;

			next_node = next_node->input_pins[idx_missed]->net->driver_pin->node;
		}
		else
		{
			return next_node;
		}
	}
}
