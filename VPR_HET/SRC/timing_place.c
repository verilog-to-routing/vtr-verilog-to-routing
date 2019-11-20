#include <stdio.h>
#include <math.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "path_delay.h"
#include "path_delay2.h"
#include "net_delay.h"
#include "timing_place_lookup.h"
#include "timing_place.h"


float **timing_place_crit;	/*available externally */

static struct s_linked_vptr *timing_place_crit_chunk_list_head;
static struct s_linked_vptr *net_delay_chunk_list_head;


/******** prototypes ******************/
static float **alloc_crit(struct s_linked_vptr **chunk_list_head_ptr);

static void free_crit(struct s_linked_vptr **chunk_list_head_ptr);

/**************************************/

static float **
alloc_crit(struct s_linked_vptr **chunk_list_head_ptr)
{

    /* Allocates space for the timing_place_crit data structure *
     * [0..num_nets-1][1..num_pins-1].  I chunk the data to save space on large    *
     * problems.                                                                   */

    float **local_crit;		/* [0..num_nets-1][1..num_pins-1] */
    float *tmp_ptr;
    int inet;
    int chunk_bytes_avail;
    char *chunk_next_avail_mem;

    *chunk_list_head_ptr = NULL;
    chunk_bytes_avail = 0;
    chunk_next_avail_mem = NULL;

    local_crit = (float **)my_malloc(num_nets * sizeof(float *));

    for(inet = 0; inet < num_nets; inet++)
	{
	    tmp_ptr = (float *)my_chunk_malloc((net[inet].num_sinks) *
					       sizeof(float),
					       chunk_list_head_ptr,
					       &chunk_bytes_avail,
					       &chunk_next_avail_mem);
	    local_crit[inet] = tmp_ptr - 1;	/* [1..num_sinks] */
	}

    return (local_crit);
}

/**************************************/
static void
free_crit(struct s_linked_vptr **chunk_list_head_ptr)
{

    free_chunk_memory(*chunk_list_head_ptr);
    *chunk_list_head_ptr = NULL;
}

/**************************************/
void
print_sink_delays(char *fname)
{


    int num_at_level, num_edges, inode, ilevel, i;
    FILE *fp;

    fp = my_fopen(fname, "w");


    for(ilevel = num_tnode_levels - 1; ilevel >= 0; ilevel--)
	{
	    num_at_level = tnodes_at_level[ilevel].nelem;

	    for(i = 0; i < num_at_level; i++)
		{
		    inode = tnodes_at_level[ilevel].list[i];
		    num_edges = tnode[inode].num_edges;

		    if(num_edges == 0)
			{	/* sink */
			    fprintf(fp, "%g\n", tnode[inode].T_arr);
			}
		}
	}
    fclose(fp);
}

/**************************************/
void
load_criticalities(struct s_placer_opts placer_opts,
		   float **net_slack,
		   float d_max,
		   float crit_exponent)
{

    /*set criticality values, returns the maximum criticality found */
    /*assumes that net_slack contains correct values, ie. assumes  *
     *that load_net_slack has been called*/

    int inet, ipin;
    float pin_crit;



    for(inet = 0; inet < num_nets; inet++)
	{

	    if(inet == OPEN)
		continue;
	    if(net[inet].is_global)
		continue;

	    for(ipin = 1; ipin <= net[inet].num_sinks; ipin++)
		{
		    /*clip the criticality to never go negative (could happen */
		    /*for a constant generator since it's slack is huge) */
		    pin_crit = max(1 - net_slack[inet][ipin] / d_max, 0.);
		    timing_place_crit[inet][ipin] =
			pow(pin_crit, crit_exponent);

		}
	}
}

/**************************************/

void
alloc_lookups_and_criticalities(t_chan_width_dist chan_width_dist,
				struct s_router_opts router_opts,
				struct s_det_routing_arch det_routing_arch,
				t_segment_inf * segment_inf,
				t_timing_inf timing_inf,
				t_subblock_data subblock_data,
				float ***net_delay,
				float ***net_slack)
{

    (*net_slack) = alloc_and_load_timing_graph(timing_inf, subblock_data);

    (*net_delay) = alloc_net_delay(&net_delay_chunk_list_head);

    compute_delay_lookup_tables(router_opts, det_routing_arch, segment_inf,
				timing_inf, chan_width_dist, subblock_data);

    timing_place_crit = alloc_crit(&timing_place_crit_chunk_list_head);

}

/**************************************/
void
free_lookups_and_criticalities(float ***net_delay,
			       float ***net_slack)
{

    free(timing_place_crit);
    free_crit(&timing_place_crit_chunk_list_head);

    free_timing_graph(*net_slack);
    free_net_delay(*net_delay, &net_delay_chunk_list_head);

}

/**************************************/
