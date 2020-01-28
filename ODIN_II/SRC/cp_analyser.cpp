#include "cp_analyser.hpp"

#include "odin_types.h"
#include "netlist_utils.h"


/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traversal_to_parital_map()
 *-------------------------------------------------------------------------------------------*/
void depth_first_traversal_to_cp(short marker_value, netlist_t *netlist)
{
	int i;

	/* start with the primary input list */
	for (i = 0; i < netlist->num_top_input_nodes; i++)
	{
		if (netlist->top_input_nodes[i] != NULL)
		{
			depth_first_traverse_cp(netlist->top_input_nodes[i], marker_value, netlist);
		}
	}
	/* now traverse the ground and vcc pins  */
	depth_first_traverse_cp(netlist->gnd_node, marker_value, netlist);
	depth_first_traverse_cp(netlist->vcc_node, marker_value, netlist);
	depth_first_traverse_cp(netlist->pad_node, marker_value, netlist);
}

/*---------------------------------------------------------------------------------------------
 * (function: depth_first_traverse)
 *-------------------------------------------------------------------------------------------*/
void depth_first_traverse_cp(nnode_t *node, int traverse_mark_number, netlist_t *netlist)
{
	int i, j, k;

	if (node->traverse_visited != traverse_mark_number)
	{
		/* this is a new node so depth visit it */

		/* mark that we have visitied this node now */
		node->traverse_visited = traverse_mark_number;

		for (k = 0; k < node->num_input_pins; k++)
			node->input_pins[k]->net->cp

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
								depth_first_traverse_cp(next_net->fanout_pins[j]->node, traverse_mark_number, netlist);
							}
						}
					}
				}
			}
		}

		/* POST traverse  map the node since you might delete */
		// partial_map_node(node, traverse_mark_number, netlist);
	}
}