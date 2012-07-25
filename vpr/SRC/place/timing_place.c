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

float **timing_place_crit; /*available externally */

static t_chunk timing_place_crit_ch = {NULL, 0, NULL};
static t_chunk net_delay_ch = {NULL, 0, NULL};

/*static struct s_linked_vptr *timing_place_crit_chunk_list_head;
static struct s_linked_vptr *net_delay_chunk_list_head;*/

/******** prototypes ******************/
/*static float **alloc_crit(struct s_linked_vptr **chunk_list_head_ptr);

static void free_crit(struct s_linked_vptr **chunk_list_head_ptr);*/

static float **alloc_crit(t_chunk *chunk_list_ptr);

static void free_crit(t_chunk *chunk_list_ptr);

/**************************************/

static float ** alloc_crit(t_chunk *chunk_list_ptr) {

	/* Allocates space for the timing_place_crit data structure *
	 * [0..num_nets-1][1..num_pins-1].  I chunk the data to save space on large    *
	 * problems.                                                                   */

	float **local_crit; /* [0..num_nets-1][1..num_pins-1] */
	float *tmp_ptr;
	int inet;

	local_crit = (float **) my_malloc(num_nets * sizeof(float *));

	for (inet = 0; inet < num_nets; inet++) {
		tmp_ptr = (float *) my_chunk_malloc(
				(clb_net[inet].num_sinks) * sizeof(float), chunk_list_ptr);
		local_crit[inet] = tmp_ptr - 1; /* [1..num_sinks] */
	}

	return (local_crit);
}

/**************************************/
static void free_crit(t_chunk *chunk_list_ptr){
	free_chunk_memory(chunk_list_ptr);
}

/**************************************/
void print_sink_delays(const char *fname) {

	int num_at_level, num_edges, inode, ilevel, i;
	FILE *fp;

	fp = my_fopen(fname, "w", 0);

	for (ilevel = num_tnode_levels - 1; ilevel >= 0; ilevel--) {
		num_at_level = tnodes_at_level[ilevel].nelem;

		for (i = 0; i < num_at_level; i++) {
			inode = tnodes_at_level[ilevel].list[i];
			num_edges = tnode[inode].num_edges;

			if (num_edges == 0) { /* sink */
				fprintf(fp, "%g\n", tnode[inode].T_arr);
			}
		}
	}
	fclose(fp);
}

/**************************************/
void load_criticalities(float ** net_slack_ratio, float crit_exponent) {
	/* Performs a 1-to-1 mapping from net_slack_ratio to timing_place_crit.  
	  For every pin on every net (or, equivalently, for every tedge ending 
	  in that pin), criticality = (1-slack ratio)^(criticality exponent) */

	int inet, ipin;

	for (inet = 0; inet < num_nets; inet++) {
		if (inet == OPEN)
			continue;
		if (clb_net[inet].is_global)
			continue;
        for (ipin = 1; ipin <= clb_net[inet].num_sinks; ipin++) {
			if (net_slack_ratio[inet][ipin] > HUGE_POSITIVE_FLOAT - 1) {
				/* We didn't analyze this connection, so give it a dummy, very negative criticality. */
				timing_place_crit[inet][ipin] = HUGE_NEGATIVE_FLOAT;
			} else {
				timing_place_crit[inet][ipin] = pow(1 - net_slack_ratio[inet][ipin], crit_exponent);
			}
		}
	}
}

/**************************************/
t_slack * alloc_lookups_and_criticalities(t_chan_width_dist chan_width_dist,
		struct s_router_opts router_opts,
		struct s_det_routing_arch det_routing_arch, t_segment_inf * segment_inf,
		t_timing_inf timing_inf, float ***net_delay) {

	t_slack * slacks = alloc_and_load_timing_graph(timing_inf);

	(*net_delay) = alloc_net_delay(&net_delay_ch, clb_net,
			num_nets);

	compute_delay_lookup_tables(router_opts, det_routing_arch, segment_inf,
			timing_inf, chan_width_dist);
	
	timing_place_crit = alloc_crit(&timing_place_crit_ch);

	return slacks;
}

/**************************************/
void free_lookups_and_criticalities(float ***net_delay, t_slack * slacks) {

	free(timing_place_crit);
	free_crit(&timing_place_crit_ch);

	free_timing_graph(slacks);
	free_net_delay(*net_delay, &net_delay_ch);

	free_place_lookup_structs();
}

/**************************************/
