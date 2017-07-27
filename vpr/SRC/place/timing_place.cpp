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

static float **f_timing_place_crit;

static vtr::t_chunk f_timing_place_crit_ch = {NULL, 0, NULL};

/******** prototypes ******************/
static float **alloc_crit(vtr::t_chunk *chunk_list_ptr);

static void free_crit(vtr::t_chunk *chunk_list_ptr);

/**************************************/

static float ** alloc_crit(vtr::t_chunk *chunk_list_ptr) {

	/* Allocates space for the f_timing_place_crit data structure *
	 * [0..cluster_ctx.clbs_nlist.net.size()-1][1..num_pins-1].  I chunk the data to save space on large    *
	 * problems.                                                                   */

    auto& cluster_ctx = g_vpr_ctx.clustering();
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
void print_sink_delays(const char* fname) {
    //TODO: re-implement with tatum timing analysis

    int num_at_level, num_edges, inode, ilevel, i;
    FILE *fp;

    auto& timing_ctx = g_vpr_ctx.timing();

    fp = vtr::fopen(fname, "w");

    for (ilevel = timing_ctx.num_tnode_levels - 1; ilevel >= 0; ilevel--) {
        num_at_level = timing_ctx.tnodes_at_level[ilevel].size();

        for (i = 0; i < num_at_level; i++) {
            inode = timing_ctx.tnodes_at_level[ilevel][i];
            num_edges = timing_ctx.tnodes[inode].num_edges;

            if (num_edges == 0) { /* sink */
                fprintf(fp, "%g\n", timing_ctx.tnodes[inode].T_arr);
            }
        }
    }
    fclose(fp);
}

/**************************************/
void load_criticalities(SetupTimingInfo& timing_info, float crit_exponent, const IntraLbPbPinLookup& pb_gpin_lookup) {
	/* Performs a 1-to-1 mapping from criticality to f_timing_place_crit.  
	  For every pin on every net (or, equivalently, for every tedge ending 
	  in that pin), f_timing_place_crit = criticality^(criticality exponent) */

    auto& cluster_ctx = g_vpr_ctx.clustering();
	for (size_t inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
		if (cluster_ctx.clb_nlist.net_global((NetId)inet))
			continue;
		for (size_t ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ipin++) {

            float clb_pin_crit = calculate_clb_net_pin_criticality(timing_info, pb_gpin_lookup, inet, ipin);

            /* The placer likes a great deal of contrast between criticalities. 
            Since path criticality varies much more than timing, we "sharpen" timing 
            criticality by taking it to some power, crit_exponent (between 1 and 8 by default). */
            f_timing_place_crit[inet][ipin] = pow(clb_pin_crit, crit_exponent);
		}
	}
}


float get_timing_place_crit(int inet, int ipin) {
    return f_timing_place_crit[inet][ipin];
}

void set_timing_place_crit(int inet, int ipin, float val) {
    f_timing_place_crit[inet][ipin] = val;
}

/**************************************/
void alloc_lookups_and_criticalities(t_chan_width_dist chan_width_dist,
		t_router_opts router_opts,
		t_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
		const t_direct_inf *directs, 
		const int num_directs) {

	compute_delay_lookup_tables(router_opts, det_routing_arch, segment_inf,
			chan_width_dist, directs, num_directs);
	
	f_timing_place_crit = alloc_crit(&f_timing_place_crit_ch);
}

/**************************************/
void free_lookups_and_criticalities() {

	free(f_timing_place_crit);
	free_crit(&f_timing_place_crit_ch);

	free_place_lookup_structs();
}

/**************************************/
