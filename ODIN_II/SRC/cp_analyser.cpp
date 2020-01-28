#include "cp_analyser.hpp"

#include "odin_types.h"


/*---------------------------------------------------------------------------------------------
 * function: dfs_to_cp() it starts from head and end of the netlist to calculate critical path
 *-------------------------------------------------------------------------------------------*/
void dfs_to_cp(netlist_t *netlist)
{
	int i;

	/* start with the primary input list */
	for (i = 0; i < netlist->num_top_input_nodes; i++)
	{
		if (netlist->top_input_nodes[i] != NULL)
		{
			dfs_s2e_cp(netlist->top_input_nodes[i], TRAVERSE_VALUE_CP_S2E, netlist);
		}
	}
	/* now traverse the ground and vcc pins  */
	dfs_s2e_cp(netlist->vcc_node, TRAVERSE_VALUE_CP_S2E, netlist);
	dfs_s2e_cp(netlist->pad_node, TRAVERSE_VALUE_CP_S2E, netlist);
	dfs_s2e_cp(netlist->gnd_node, TRAVERSE_VALUE_CP_S2E, netlist);


	/* start with the primary output list */
	for (i = 0; i < netlist->num_top_output_nodes; i++)
	{
		if (netlist->top_output_nodes[i] != NULL)
		{
			dfs_e2s_cp(netlist->top_output_nodes[i], TRAVERSE_VALUE_CP_E2S, netlist);
		}
	}
	/* now traverse the ground and vcc pins  */
	dfs_e2s_cp(netlist->vcc_node, TRAVERSE_VALUE_CP_E2S, netlist);
	dfs_e2s_cp(netlist->pad_node, TRAVERSE_VALUE_CP_E2S, netlist);
	dfs_e2s_cp(netlist->gnd_node, TRAVERSE_VALUE_CP_E2S, netlist);
}

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traverse)
 *-------------------------------------------------------------------------------------------*/
void dfs_s2e_cp(nnode_t *node, int traverse_mark_number, netlist_t *netlist)
{
	int i, j;

	if (node->traverse_visited != traverse_mark_number)
	{
		/* this is a new node so depth visit it */

		/* mark that we have visitied this node now */
		node->traverse_visited = traverse_mark_number;

		for (i = 0; i < node->num_output_pins; i++)
		{
			if (node->output_pins[i]->net)
			{
				nnet_t *next_net = node->output_pins[i]->net;
				next_net->cp_up++;

				if(next_net->fanout_pins)
				{
					for (j = 0; j < next_net->num_fanout_pins; j++)
					{
						if (next_net->fanout_pins[j])
						{
							if (next_net->fanout_pins[j]->node)
							{
							/* recursive call point */
								dfs_s2e_cp(next_net->fanout_pins[j]->node, traverse_mark_number, netlist);
							}
						}
					}
				}
			}
		}

		node->cp_from_start = node->output_pins[0]->net->cp_up;

		for (i = 1; i < node->num_output_pins; i++)

			if ( node->cp_from_start < node->output_pins[i]->net->cp_up )

				node->cp_from_start = node->output_pins[i]->net->cp_up;
	}
}

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traverse)
 *-------------------------------------------------------------------------------------------*/
void dfs_e2s_cp(nnode_t *node, int traverse_mark_number, netlist_t *netlist)
{
	int i;

	if (node->traverse_visited != traverse_mark_number)
	{
		/* this is a new node so depth visit it */

		/* mark that we have visitied this node now */
		node->traverse_visited = traverse_mark_number;

		for (i = 0; i < node->num_input_pins; i++)
		{
			if (node->input_pins[i]->net)
			{
				nnet_t *next_net = node->input_pins[i]->net;
				next_net->cp_down++;

				if(next_net->driver_pin)
				{
					if (next_net->driver_pin->node)
					{
					/* recursive call point */
						dfs_s2e_cp(next_net->driver_pin->node, traverse_mark_number, netlist);
					}
				}
			}
		}

		node->cp_from_start = node->input_pins[0]->net->cp_down;

		for (i = 1; i < node->num_input_pins; i++)

			if ( node->cp_from_start < node->input_pins[i]->net->cp_down )

				node->cp_from_start = node->input_pins[i]->net->cp_down;
	}
}