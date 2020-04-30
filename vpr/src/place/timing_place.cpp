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

std::vector<ClusterPinId> f_cluster_pins_to_update_criticality;

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
void load_criticalities(const SetupTimingInfo& timing_info, float crit_exponent, const ClusteredPinAtomPinsLookup& pin_lookup) {
    /* Performs a 1-to-1 mapping from criticality to f_timing_place_crit.
     * For every pin on every net (or, equivalently, for every tedge ending
     * in that pin), f_timing_place_crit = criticality^(criticality exponent) */


    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& clb_nlist = cluster_ctx.clb_nlist;

    //Collect the cluster pins which need to be updated based on the latest timing
    //analysis
    //
    //Note we use the set of pins reported by the timing_info as having modified
    //criticality, rather than those marked as modified by the timing analyzer.
    //Since we use shifted criticality (which depends on max required time and
    //worst case slacks), additional nodes may be modified when calculating atom
    //pin criticality.
    f_cluster_pins_to_update_criticality.clear();
    for (AtomPinId atom_pin : timing_info.pins_with_modified_setup_criticality()) {
        ClusterPinId clb_pin = pin_lookup.connected_clb_pin(atom_pin);

        //Some atom pins correspond to connections which are completely
        //contained within a cluster, and hece have no corresponding
        //clustered pin.
        if (!clb_pin) continue;

        f_cluster_pins_to_update_criticality.push_back(clb_pin);
    }

    //Uniquify to avoid re-calculating the same CLB pin multiple times
    //(since each CLB pin may correspond to multiple atom pins within the
    //cluster
    vtr::uniquify(f_cluster_pins_to_update_criticality);

    //Update the effected pins
    for (ClusterPinId clb_pin : f_cluster_pins_to_update_criticality) {
        ClusterNetId clb_net = clb_nlist.pin_net(clb_pin);
        int pin_index_in_net = clb_nlist.pin_net_index(clb_pin);

        float clb_pin_crit = calculate_clb_net_pin_criticality(timing_info, pin_lookup, clb_pin);

        /* The placer likes a great deal of contrast between criticalities.
         * Since path criticality varies much more than timing, we "sharpen" timing
         * criticality by taking it to some power, crit_exponent (between 1 and 8 by default). */
        f_timing_place_crit[clb_net][pin_index_in_net] = pow(clb_pin_crit, crit_exponent);
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
