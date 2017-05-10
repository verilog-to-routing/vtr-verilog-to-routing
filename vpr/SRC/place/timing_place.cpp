#include <cstdio>
#include <cmath>
using namespace std;

#include "vtr_util.h"
#include "vtr_memory.h"
#include "vtr_log.h"

#include "vpr_types.h"
#include "vpr_utils.h"
#include "globals.h"
#include "path_delay.h"
#include "path_delay2.h"
#include "net_delay.h"
#include "timing_place_lookup.h"
#include "timing_place.h"

#include "timing_info.h"

float **timing_place_crit; /*available externally */

static vtr::t_chunk timing_place_crit_ch = {NULL, 0, NULL};

/******** prototypes ******************/
static float **alloc_crit(vtr::t_chunk *chunk_list_ptr);

static void free_crit(vtr::t_chunk *chunk_list_ptr);

/**************************************/

static float ** alloc_crit(vtr::t_chunk *chunk_list_ptr) {

	/* Allocates space for the timing_place_crit data structure *
	 * [0..cluster_ctx.clbs_nlist.net.size()-1][1..num_pins-1].  I chunk the data to save space on large    *
	 * problems.                                                                   */

    auto& cluster_ctx = g_ctx.clustering();
	float **local_crit; /* [0..cluster_ctx.clbs_nlist.net.size()-1][1..num_pins-1] */
	float *tmp_ptr;
	unsigned int inet;

	local_crit = (float **) vtr::malloc(cluster_ctx.clbs_nlist.net.size() * sizeof(float *));

	for (inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
		tmp_ptr = (float *) vtr::chunk_malloc(
				(cluster_ctx.clbs_nlist.net[inet].num_sinks()) * sizeof(float), chunk_list_ptr);
		local_crit[inet] = tmp_ptr - 1; /* [1..num_sinks] */
	}

	return (local_crit);
}

/**************************************/
static void free_crit(vtr::t_chunk *chunk_list_ptr){
    vtr::free_chunk_memory(chunk_list_ptr);
}

/**************************************/
void print_sink_delays(const char *fname) {

	int num_at_level, num_edges, inode, ilevel, i;
	FILE *fp;

	fp = vtr::fopen(fname, "w");

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
void load_criticalities(SetupTimingInfo& timing_info, float crit_exponent, const IntraLbPbPinLookup& pb_gpin_lookup) {
	/* Performs a 1-to-1 mapping from criticality to timing_place_crit.  
	  For every pin on every net (or, equivalently, for every tedge ending 
	  in that pin), timing_place_crit = criticality^(criticality exponent) */

    auto& cluster_ctx = g_ctx.clustering();
	for (size_t inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
		if (cluster_ctx.clbs_nlist.net[inet].is_global)
			continue;
		for (size_t ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ipin++) {

            float clb_pin_crit = calculate_clb_net_pin_criticality(timing_info, pb_gpin_lookup, inet, ipin);

            /* The placer likes a great deal of contrast between criticalities. 
            Since path criticality varies much more than timing, we "sharpen" timing 
            criticality by taking it to some power, crit_exponent (between 1 and 8 by default). */
            timing_place_crit[inet][ipin] = pow(clb_pin_crit, crit_exponent);
		}
	}
}

/**************************************/
void alloc_lookups_and_criticalities(t_chan_width_dist chan_width_dist,
		struct s_router_opts router_opts,
		struct s_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
		const t_direct_inf *directs, 
		const int num_directs) {

	compute_delay_lookup_tables(router_opts, det_routing_arch, segment_inf,
			chan_width_dist, directs, num_directs);
	
	timing_place_crit = alloc_crit(&timing_place_crit_ch);
}

/**************************************/
void free_lookups_and_criticalities() {

	free(timing_place_crit);
	free_crit(&timing_place_crit_ch);

	free_place_lookup_structs();
}

/**************************************/
