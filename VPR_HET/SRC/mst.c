#include <limits.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "mst.h"

#define ABS_DIFF(X, Y) (((X) > (Y))? ((X) - (Y)):((Y) - (X)))

static int min_dist_from_mst(int node_outside,
			     int inet,
			     boolean * in_mst,
			     int *node_inside);

t_mst_edge *
get_mst_of_net(int inet)
{
    int i, ipin, node_dist, node_in_mst, num_pins_on_net, num_edges;
    int nearest_node, nearest_node_dist, nearest_node_in_mst;
    t_mst_edge *min_spantree;
    boolean *in_mst;

/* given the net number, find and return its minimum spanning tree */

    num_pins_on_net = (net[inet].num_sinks + 1);
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

    /* identifier for minimum spanning tree by nodes */
    in_mst[0] = TRUE;
    for(ipin = 1; ipin < num_pins_on_net; ipin++)
	in_mst[ipin] = FALSE;

    num_edges = 0;

    /* need to add num_pins - 1 number of edges */
    for(i = 1; i < num_pins_on_net; i++)
	{
	    /* look at remaining num_pins - 1 pins, and add them to the mst one at a time */

	    nearest_node = -1;
	    nearest_node_dist = -1;
	    nearest_node_in_mst = -1;

	    for(ipin = 1; ipin < num_pins_on_net; ipin++)
		{
		    /* look at each pin not in the mst, find the nearest to the current partial mst */
		    if(!in_mst[ipin])
			{
			    node_dist =
				min_dist_from_mst(ipin, inet, in_mst,
						  &node_in_mst);

			    if(nearest_node == -1)
				{
				    nearest_node = ipin;
				    nearest_node_dist = node_dist;
				    nearest_node_in_mst = node_in_mst;
				}
			    else
				{
				    /* Does 0 make sense?  Currently allow.  0 means the net loops back to the same block */
				    if(nearest_node_dist >= node_dist)
					{
					    nearest_node = ipin;
					    nearest_node_dist = node_dist;
					    nearest_node_in_mst = node_in_mst;
					}

				    if(nearest_node_dist < 0)
					{
					    printf
						("Error: distance is negative\n");
					    exit(1);
					}
				}
			}
		}

	    /* found the nearest to the current partial mst, so add an edge to mst */
	    min_spantree[num_edges].from_node = nearest_node_in_mst;

	    min_spantree[num_edges].to_node = nearest_node;

	    num_edges++;

	    in_mst[nearest_node] = TRUE;
	}

    free(in_mst);
    return min_spantree;
}

static int
min_dist_from_mst(int node_outside,
		  int inet,
		  boolean * in_mst,
		  int *node_inside)
{
    int ipin, blk1, blk2, dist, closest_node_in_mst, shortest_dist;

/* given a node outside the mst this routine finds its shortest distance to the current partial 
 * mst, as well as the corresponding node inside mst */

    closest_node_in_mst = -1;
    shortest_dist = -1;
    /* search over all blocks inside mst */
    for(ipin = 0; ipin < (net[inet].num_sinks + 1); ipin++)
	{
	    if(in_mst[ipin])
		{
		    blk1 = net[inet].node_block[node_outside];
		    blk2 = net[inet].node_block[ipin];

		    dist =
			ABS_DIFF(block[blk1].x,
				 block[blk2].x) + ABS_DIFF(block[blk1].y,
							   block[blk2].y);

		    if(closest_node_in_mst == -1)
			{
			    closest_node_in_mst = ipin;
			    shortest_dist = dist;
			}
		    else
			{
			    if(shortest_dist > dist)
				{
				    closest_node_in_mst = ipin;
				    shortest_dist = dist;
				}
			}
		}
	}

    *node_inside = closest_node_in_mst;

    return shortest_dist;
}
