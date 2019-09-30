#include <cstdio>
#include <cmath>

#include "vtr_util.h"
#include "vtr_memory.h"
#include "vtr_log.h"

#include "vpr_types.h"
#include "vpr_utils.h"
#include "globals.h"
#include "net_delay.h"
#include "timing_place_lookup.h"
#include "timing_place.h"

#include "timing_info.h"

vtr::vector<ClusterNetId, float*> f_timing_place_crit; /* [0..cluster_ctx.clb_nlist.nets().size()-1][1..num_pins-1] */

static vtr::t_chunk f_timing_place_crit_ch;

/******** prototypes ******************/
static void alloc_crit(vtr::t_chunk* chunk_list_ptr);

static void free_crit(vtr::t_chunk* chunk_list_ptr);

/**************************************/

/* Allocates space for the f_timing_place_crit data structure *
 * I chunk the data to save space on large problems.           */
static void alloc_crit(vtr::t_chunk* chunk_list_ptr) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    float* tmp_ptr;

    f_timing_place_crit.resize(cluster_ctx.clb_nlist.nets().size());

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        tmp_ptr = (float*)vtr::chunk_malloc((cluster_ctx.clb_nlist.net_pins(net_id).size()) * sizeof(float), chunk_list_ptr);
        f_timing_place_crit[net_id] = tmp_ptr;
    }
}

/**************************************/
static void free_crit(vtr::t_chunk* chunk_list_ptr) {
    vtr::free_chunk_memory(chunk_list_ptr);
}

/**************************************/
void load_criticalities(SetupTimingInfo& timing_info, float crit_exponent, const ClusteredPinAtomPinsLookup& pin_lookup) {
    /* Performs a 1-to-1 mapping from criticality to f_timing_place_crit.
     * For every pin on every net (or, equivalently, for every tedge ending
     * in that pin), f_timing_place_crit = criticality^(criticality exponent) */

    auto& cluster_ctx = g_vpr_ctx.clustering();
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        if (cluster_ctx.clb_nlist.net_is_ignored(net_id))
            continue;

        for (auto clb_pin : cluster_ctx.clb_nlist.net_sinks(net_id)) {
            int ipin = cluster_ctx.clb_nlist.pin_net_index(clb_pin);

            float clb_pin_crit = calculate_clb_net_pin_criticality(timing_info, pin_lookup, clb_pin);

            /* The placer likes a great deal of contrast between criticalities.
             * Since path criticality varies much more than timing, we "sharpen" timing
             * criticality by taking it to some power, crit_exponent (between 1 and 8 by default). */
            f_timing_place_crit[net_id][ipin] = pow(clb_pin_crit, crit_exponent);
        }
    }
}

float get_timing_place_crit(ClusterNetId net_id, int ipin) {
    return f_timing_place_crit[net_id][ipin];
}

void set_timing_place_crit(ClusterNetId net_id, int ipin, float val) {
    f_timing_place_crit[net_id][ipin] = val;
}

/**************************************/
std::unique_ptr<PlaceDelayModel> alloc_lookups_and_criticalities(t_chan_width_dist chan_width_dist,
                                                                 const t_placer_opts& placer_opts,
                                                                 const t_router_opts& router_opts,
                                                                 t_det_routing_arch* det_routing_arch,
                                                                 std::vector<t_segment_inf>& segment_inf,
                                                                 const t_direct_inf* directs,
                                                                 const int num_directs) {
    alloc_crit(&f_timing_place_crit_ch);

    return compute_place_delay_model(placer_opts, router_opts, det_routing_arch, segment_inf,
                                     chan_width_dist, directs, num_directs);
}

/**************************************/
void free_lookups_and_criticalities() {
    //TODO: May need to free f_timing_place_crit ?
    free_crit(&f_timing_place_crit_ch);
}

/**************************************/
