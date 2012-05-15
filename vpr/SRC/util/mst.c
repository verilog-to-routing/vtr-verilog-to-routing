#include <limits.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "mst.h"

#define ABS_DIFF(X, Y) (((X) > (Y))? ((X) - (Y)):((Y) - (X)))

t_mst_edge * get_mst_of_net(int inet)
{
    int i, ipin, node_dist, num_pins_on_net, num_edges, blk1, blk2;
    int nearest_node;
    t_mst_edge *min_spantree;
    boolean *in_mst;
	int *edge_weight_with_curr_mst, *nearest_node_with_curr_mst;

/* given the net number, find and return its minimum spanning tree */

    num_pins_on_net = (clb_net[inet].num_sinks + 1);
	
    if(num_pins_on_net > USHRT_MAX)
	{
	    printf("Error: num_pins_on_net (%d) > USHRT_MAX(%u)\n",
		   num_pins_on_net, USHRT_MAX);
	    exit(1);
	}

    /* minimum spanning tree represented by a set of edges */
    min_spantree =
	(t_mst_edge *) my_malloc(sizeof(t_mst_edge) * (num_pins_on_net - 1));
    in_mst = (boolean *) my_malloc(sizeof(boolean) * (num_pins_on_net));
	edge_weight_with_curr_mst = (int *) my_malloc(sizeof(int) * (num_pins_on_net));
	nearest_node_with_curr_mst = (int *) my_malloc(sizeof(int) * (num_pins_on_net));

    /* identifier for minimum spanning tree by nodes */
    in_mst[0] = TRUE;
    for(ipin = 1; ipin < num_pins_on_net; ipin++) {
		in_mst[ipin] = FALSE;
		blk1 = clb_net[inet].node_block[0];
		blk2 = clb_net[inet].node_block[ipin];
		edge_weight_with_curr_mst[ipin] = ABS_DIFF(block[blk1].x, block[blk2].x) + 
									ABS_DIFF(block[blk1].y, block[blk2].y);
		nearest_node_with_curr_mst[ipin] = 0;
	}

    num_edges = 0;

    /* need to add num_pins - 1 number of edges */
    for(i = 1; i < num_pins_on_net; i++)
	{
	    /* look at remaining num_pins - 1 pins, and add them to the mst one at a time */

	    nearest_node = -1;

	    for(ipin = 1; ipin < num_pins_on_net; ipin++)
		{
		    /* look at each pin not in the mst, find the nearest to the current partial mst */
		    if(!in_mst[ipin])
			{
				if(nearest_node == -1 || edge_weight_with_curr_mst[ipin] <= edge_weight_with_curr_mst[nearest_node]) {
				    nearest_node = ipin;
				}
			}
		}

	    /* found the nearest to the current partial mst, so add an edge to mst */
	    min_spantree[num_edges].from_node = nearest_node_with_curr_mst[nearest_node];

	    min_spantree[num_edges].to_node = nearest_node;

	    num_edges++;

	    in_mst[nearest_node] = TRUE;

		for(ipin = 1; ipin < num_pins_on_net; ipin++)
		{
		    /* Update closest node to partial mst */
		    if(!in_mst[ipin])
			{
				blk1 = clb_net[inet].node_block[nearest_node];
				blk2 = clb_net[inet].node_block[ipin];
				node_dist = ABS_DIFF(block[blk1].x, block[blk2].x) + 
									ABS_DIFF(block[blk1].y, block[blk2].y);
				if(node_dist < edge_weight_with_curr_mst[ipin]) {
				    edge_weight_with_curr_mst[ipin] = node_dist;
					nearest_node_with_curr_mst[ipin] = nearest_node;
				}
			}
		}
	}	

    free(in_mst);
	free(edge_weight_with_curr_mst);
	free(nearest_node_with_curr_mst);
    return min_spantree;
}
