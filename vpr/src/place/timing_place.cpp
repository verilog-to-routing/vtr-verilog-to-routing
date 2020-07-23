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

//Use an incremental approach to updating criticalities and setup slacks?
constexpr bool INCR_UPDATE_CRITICALITIES = true;
constexpr bool INCR_UPDATE_SETUP_SLACKS = true;

/**************************************/

/* Allocates space for the timing_place_crit_ data structure *
 * I chunk the data to save space on large problems.           */
PlacerCriticalities::PlacerCriticalities(const ClusteredNetlist& clb_nlist, const ClusteredPinAtomPinsLookup& netlist_pin_lookup)
    : clb_nlist_(clb_nlist)
    , pin_lookup_(netlist_pin_lookup)
    , timing_place_crit_(make_net_pins_matrix(clb_nlist_, std::numeric_limits<float>::quiet_NaN())) {
}

void PlacerCriticalities::update_criticalities(const SetupTimingInfo* timing_info, float crit_exponent, bool recompute) {
    //If the criticalities are not updated immediately after each time we call
    //timing_info->update(), then timing_info->pins_with_modified_setup_criticality()
    //cannot accurately account for all the pins that need to be updated.
    //In this case, we pass in recompute=true to update all criticalities from scratch.
    //
    //If the criticality exponent has changed, we also need to update from scratch.

    //Determine what pins need updating
    if (!recompute && crit_exponent == last_crit_exponent_ && INCR_UPDATE_CRITICALITIES) {
        incr_update_criticalities(timing_info);
    } else {
        recompute_criticalities();

        //Record new criticality exponent
        last_crit_exponent_ = crit_exponent;
    }

    /* Performs a 1-to-1 mapping from criticality to timing_place_crit_.
     * For every pin on every net (or, equivalently, for every tedge ending
     * in that pin), timing_place_crit_ = criticality^(criticality exponent) */

    // Update the effected pins
    for (ClusterPinId clb_pin : cluster_pins_with_modified_criticality_) {
        ClusterNetId clb_net = clb_nlist_.pin_net(clb_pin);
        int pin_index_in_net = clb_nlist_.pin_net_index(clb_pin);

        float clb_pin_crit = calculate_clb_net_pin_criticality(*timing_info, pin_lookup_, clb_pin);

        /* The placer likes a great deal of contrast between criticalities.
         * Since path criticality varies much more than timing, we "sharpen" timing
         * criticality by taking it to some power, crit_exponent (between 1 and 8 by default). */
        timing_place_crit_[clb_net][pin_index_in_net] = pow(clb_pin_crit, crit_exponent);
    }
}

void PlacerCriticalities::incr_update_criticalities(const SetupTimingInfo* timing_info) {
    cluster_pins_with_modified_criticality_.clear();

    //Collect the cluster pins which need to be updated based on the latest timing
    //analysis
    //
    //Note we use the set of pins reported by the *timing_info* as having modified
    //criticality, rather than those marked as modified by the timing analyzer.
    //Since timing_info uses shifted/relaxed criticality (which depends on max
    //required time and worst case slacks), additional nodes may be modified
    //when updating the atom pin criticalities.

    for (AtomPinId atom_pin : timing_info->pins_with_modified_setup_criticality()) {
        ClusterPinId clb_pin = pin_lookup_.connected_clb_pin(atom_pin);

        //Some atom pins correspond to connections which are completely
        //contained within a cluster, and hence have no corresponding
        //clustered pin.
        if (!clb_pin) continue;

        cluster_pins_with_modified_criticality_.insert(clb_pin);
    }
}

void PlacerCriticalities::recompute_criticalities() {
    cluster_pins_with_modified_criticality_.clear();

    //Non-incremental: all sink pins need updating
    for (ClusterNetId net_id : clb_nlist_.nets()) {
        for (ClusterPinId pin_id : clb_nlist_.net_sinks(net_id)) {
            cluster_pins_with_modified_criticality_.insert(pin_id);
        }
    }
}

void PlacerCriticalities::set_criticality(ClusterNetId net_id, int ipin, float val) {
    timing_place_crit_[net_id][ipin] = val;
}

PlacerCriticalities::pin_range PlacerCriticalities::pins_with_modified_criticality() const {
    return vtr::make_range(cluster_pins_with_modified_criticality_);
}

/**************************************/

/* Allocates space for the timing_place_setup_slacks_ data structure */
PlacerSetupSlacks::PlacerSetupSlacks(const ClusteredNetlist& clb_nlist, const ClusteredPinAtomPinsLookup& netlist_pin_lookup)
    : clb_nlist_(clb_nlist)
    , pin_lookup_(netlist_pin_lookup)
    , timing_place_setup_slacks_(make_net_pins_matrix(clb_nlist_, std::numeric_limits<float>::quiet_NaN())) {
}

void PlacerSetupSlacks::update_setup_slacks(const SetupTimingInfo* timing_info, bool recompute) {
    //If the setup slacks are not updated immediately after each time we call
    //timing_info->update(), then timing_info->pins_with_modified_setup_slack()
    //cannot accurately account for all the pins that need to be updated.
    //In this case, we pass in recompute=true to update all setup slacks from scratch.
    if (!recompute && INCR_UPDATE_SETUP_SLACKS) {
        incr_update_setup_slacks(timing_info);
    } else {
        recompute_setup_slacks();
    }

    //Update the effected pins
    for (ClusterPinId clb_pin : cluster_pins_with_modified_setup_slack_) {
        ClusterNetId clb_net = clb_nlist_.pin_net(clb_pin);
        int pin_index_in_net = clb_nlist_.pin_net_index(clb_pin);

        float clb_pin_setup_slack = calculate_clb_net_pin_setup_slack(*timing_info, pin_lookup_, clb_pin);

        timing_place_setup_slacks_[clb_net][pin_index_in_net] = clb_pin_setup_slack;
    }
}

void PlacerSetupSlacks::incr_update_setup_slacks(const SetupTimingInfo* timing_info) {
    cluster_pins_with_modified_setup_slack_.clear();

    //Collect the cluster pins which need to be updated based on the latest timing analysis
    //
    //Note we use the set of pins reported by the *timing_info* as having modified
    //setup slacks, rather than those marked as modified by the timing analyzer.
    for (AtomPinId atom_pin : timing_info->pins_with_modified_setup_slack()) {
        ClusterPinId clb_pin = pin_lookup_.connected_clb_pin(atom_pin);

        //Some atom pins correspond to connections which are completely
        //contained within a cluster, and hence have no corresponding
        //clustered pin.
        if (!clb_pin) continue;

        cluster_pins_with_modified_setup_slack_.insert(clb_pin);
    }
}

void PlacerSetupSlacks::recompute_setup_slacks() {
    cluster_pins_with_modified_setup_slack_.clear();

    //Non-incremental: all sink pins need updating
    for (ClusterNetId net_id : clb_nlist_.nets()) {
        for (ClusterPinId pin_id : clb_nlist_.net_sinks(net_id)) {
            cluster_pins_with_modified_setup_slack_.insert(pin_id);
        }
    }
}

void PlacerSetupSlacks::set_setup_slack(ClusterNetId net_id, int ipin, float val) {
    timing_place_setup_slacks_[net_id][ipin] = val;
}

PlacerSetupSlacks::pin_range PlacerSetupSlacks::pins_with_modified_setup_slack() const {
    return vtr::make_range(cluster_pins_with_modified_setup_slack_);
}

/**************************************/

std::unique_ptr<PlaceDelayModel> alloc_lookups_and_criticalities(t_chan_width_dist chan_width_dist,
                                                                 const t_placer_opts& placer_opts,
                                                                 const t_router_opts& router_opts,
                                                                 t_det_routing_arch* det_routing_arch,
                                                                 std::vector<t_segment_inf>& segment_inf,
                                                                 const t_direct_inf* directs,
                                                                 const int num_directs) {
    return compute_place_delay_model(placer_opts, router_opts, det_routing_arch, segment_inf,
                                     chan_width_dist, directs, num_directs);
}
