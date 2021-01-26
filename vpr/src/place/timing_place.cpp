/**
 * @file timing_place.cpp
 * @brief Stores the method definitions of classes defined in timing_place.h.
 */

#include <cstdio>
#include <cmath>

#include "vtr_util.h"
#include "vtr_memory.h"
#include "vtr_log.h"

#include "vpr_types.h"
#include "vpr_utils.h"
#include "globals.h"
#include "placer_globals.h"
#include "net_delay.h"
#include "timing_place_lookup.h"
#include "timing_place.h"

#include "timing_info.h"

///@brief Allocates space for the timing_place_crit_ data structure.
PlacerCriticalities::PlacerCriticalities(const ClusteredNetlist& clb_nlist, const ClusteredPinAtomPinsLookup& netlist_pin_lookup)
    : clb_nlist_(clb_nlist)
    , pin_lookup_(netlist_pin_lookup)
    , timing_place_crit_(make_net_pins_matrix(clb_nlist_, std::numeric_limits<float>::quiet_NaN())) {
}

/**
 * @brief Updated the criticalities in the timing_place_crit_ data structure.
 *
 * If the criticalities are not updated immediately after each time we call
 * timing_info->update(), then timing_info->pins_with_modified_setup_criticality()
 * cannot accurately account for all the pins that need to be updated. In this case,
 * `recompute_required` would be true, and we update all criticalities from scratch.
 *
 * If the criticality exponent has changed, we also need to update from scratch.
 */
void PlacerCriticalities::update_criticalities(const SetupTimingInfo* timing_info, const PlaceCritParams& crit_params) {
    /* If update is not enabled, exit the routine. */
    if (!update_enabled) {
        /* re-computation is required on the next iteration */
        recompute_required = true;
        return;
    }

    /* Determine what pins need updating */
    if (!recompute_required && crit_params.crit_exponent == last_crit_exponent_) {
        incr_update_criticalities(timing_info);
    } else {
        recompute_criticalities();

        /* Record new criticality exponent */
        last_crit_exponent_ = crit_params.crit_exponent;
    }

    ClusterBlockId crit_block;
    auto& place_move_ctx = g_placer_ctx.mutable_move();

    /* Performs a 1-to-1 mapping from criticality to timing_place_crit_.
     * For every pin on every net (or, equivalently, for every tedge ending
     * in that pin), timing_place_crit_ = criticality^(criticality exponent) */

    /* Update the affected pins */
    for (ClusterPinId clb_pin : cluster_pins_with_modified_criticality_) {
        ClusterNetId clb_net = clb_nlist_.pin_net(clb_pin);
        int pin_index_in_net = clb_nlist_.pin_net_index(clb_pin);

        float clb_pin_crit = calculate_clb_net_pin_criticality(*timing_info, pin_lookup_, clb_pin);

        float new_crit = pow(clb_pin_crit, crit_params.crit_exponent);
        /*
         * Update the highly critical pins container
         *
         * If the old criticality < limit and the new criticality > limit --> add this pin to the highly critical pins
         * If the old criticality > limit and the new criticality < limit --> remove this pin from the highly critical pins
         */
        if (!first_time_update_criticality) {
            if (new_crit > crit_params.crit_limit && timing_place_crit_[clb_net][pin_index_in_net] < crit_params.crit_limit) {
                place_move_ctx.highly_crit_pins.push_back(std::make_pair(clb_net, pin_index_in_net));
            } else if (new_crit < crit_params.crit_limit && timing_place_crit_[clb_net][pin_index_in_net] > crit_params.crit_limit) {
                place_move_ctx.highly_crit_pins.erase(std::remove(place_move_ctx.highly_crit_pins.begin(), place_move_ctx.highly_crit_pins.end(), std::make_pair(clb_net, pin_index_in_net)), place_move_ctx.highly_crit_pins.end());
            }
        } else {
            if (new_crit > crit_params.crit_limit)
                place_move_ctx.highly_crit_pins.push_back(std::make_pair(clb_net, pin_index_in_net));
        }

        /* The placer likes a great deal of contrast between criticalities.
         * Since path criticality varies much more than timing, we "sharpen" timing
         * criticality by taking it to some power, crit_exponent (between 1 and 8 by default). */
        timing_place_crit_[clb_net][pin_index_in_net] = new_crit;
    }

    /* Criticalities updated. In sync with timing info.   */
    /* Can be incrementally updated on the next iteration */
    recompute_required = false;

    first_time_update_criticality = false;
}

void PlacerCriticalities::set_recompute_required() {
    recompute_required = true;
}

/**
 * @brief Collect the cluster pins which need to be updated based on the latest timing
 *        analysis so that incremental updates to criticalities can be performed.
 *
 * Note we use the set of pins reported by the *timing_info* as having modified
 * criticality, rather than those marked as modified by the timing analyzer.
 *
 * Since timing_info uses shifted/relaxed criticality (which depends on max required
 * time and worst case slacks), additional nodes may be modified when updating the
 * atom pin criticalities.
 */

void PlacerCriticalities::incr_update_criticalities(const SetupTimingInfo* timing_info) {
    cluster_pins_with_modified_criticality_.clear();

    for (AtomPinId atom_pin : timing_info->pins_with_modified_setup_criticality()) {
        ClusterPinId clb_pin = pin_lookup_.connected_clb_pin(atom_pin);

        //Some atom pins correspond to connections which are completely
        //contained within a cluster, and hence have no corresponding
        //clustered pin.
        if (!clb_pin) continue;

        cluster_pins_with_modified_criticality_.insert(clb_pin);
    }
}

/**
 * @brief Collect all the sink pins in the netlist and prepare them update.
 *
 * For the incremental version, see PlacerCriticalities::incr_update_criticalities().
 */
void PlacerCriticalities::recompute_criticalities() {
    cluster_pins_with_modified_criticality_.clear();

    /* Non-incremental: all sink pins need updating */
    for (ClusterNetId net_id : clb_nlist_.nets()) {
        for (ClusterPinId pin_id : clb_nlist_.net_sinks(net_id)) {
            cluster_pins_with_modified_criticality_.insert(pin_id);
        }
    }
}

///@brief Override the criticality of a particular connection.
void PlacerCriticalities::set_criticality(ClusterNetId net_id, int ipin, float crit_val) {
    VTR_ASSERT_SAFE_MSG(ipin > 0, "The pin should not be a driver pin (ipin != 0)");
    VTR_ASSERT_SAFE_MSG(ipin < int(clb_nlist_.net_pins(net_id).size()), "The pin index in net should be smaller than fanout");

    timing_place_crit_[net_id][ipin] = crit_val;
}

/**
 * @brief Returns the range of clustered netlist pins (i.e. ClusterPinIds) which
 *        were modified by the last call to PlacerCriticalities::update_criticalities().
 */
PlacerCriticalities::pin_range PlacerCriticalities::pins_with_modified_criticality() const {
    return vtr::make_range(cluster_pins_with_modified_criticality_);
}

/**************************************/

///@brief Allocates space for the timing_place_setup_slacks_ data structure.
PlacerSetupSlacks::PlacerSetupSlacks(const ClusteredNetlist& clb_nlist, const ClusteredPinAtomPinsLookup& netlist_pin_lookup)
    : clb_nlist_(clb_nlist)
    , pin_lookup_(netlist_pin_lookup)
    , timing_place_setup_slacks_(make_net_pins_matrix(clb_nlist_, std::numeric_limits<float>::quiet_NaN())) {
}

/**
 * @brief Updated the setup slacks in the timing_place_setup_slacks_ data structure.
 *
 * If the setup slacks are not updated immediately after each time we call
 * timing_info->update(), then timing_info->pins_with_modified_setup_slack()
 * cannot accurately account for all the pins that need to be updated.
 *
 * In this case, `recompute_required` would be true, and we update all setup slacks
 * from scratch.
 */
void PlacerSetupSlacks::update_setup_slacks(const SetupTimingInfo* timing_info) {
    /* If update is not enabled, exit the routine. */
    if (!update_enabled) {
        /* re-computation is required on the next iteration */
        recompute_required = true;
        return;
    }

    /* Determine what pins need updating */
    if (!recompute_required) {
        incr_update_setup_slacks(timing_info);
    } else {
        recompute_setup_slacks();
    }

    /* Update the affected pins */
    for (ClusterPinId clb_pin : cluster_pins_with_modified_setup_slack_) {
        ClusterNetId clb_net = clb_nlist_.pin_net(clb_pin);
        int pin_index_in_net = clb_nlist_.pin_net_index(clb_pin);

        float clb_pin_setup_slack = calculate_clb_net_pin_setup_slack(*timing_info, pin_lookup_, clb_pin);

        timing_place_setup_slacks_[clb_net][pin_index_in_net] = clb_pin_setup_slack;
    }

    /* Setup slacks updated. In sync with timing info.     */
    /* Can be incrementally updated on the next iteration. */
    recompute_required = false;
}

/**
 * @brief Collect the cluster pins which need to be updated based on the latest timing
 *        analysis so that incremental updates to setup slacks can be performed.
 *
 * Note we use the set of pins reported by the *timing_info* as having modified
 * setup slacks, rather than those marked as modified by the timing analyzer.
 */
void PlacerSetupSlacks::incr_update_setup_slacks(const SetupTimingInfo* timing_info) {
    cluster_pins_with_modified_setup_slack_.clear();

    for (AtomPinId atom_pin : timing_info->pins_with_modified_setup_slack()) {
        ClusterPinId clb_pin = pin_lookup_.connected_clb_pin(atom_pin);

        //Some atom pins correspond to connections which are completely
        //contained within a cluster, and hence have no corresponding
        //clustered pin.
        if (!clb_pin) continue;

        cluster_pins_with_modified_setup_slack_.insert(clb_pin);
    }
}

/**
 * @brief Collect all the sink pins in the netlist and prepare them update.
 *
 * For the incremental version, see PlacerSetupSlacks::incr_update_setup_slacks().
 */
void PlacerSetupSlacks::recompute_setup_slacks() {
    cluster_pins_with_modified_setup_slack_.clear();

    /* Non-incremental: all sink pins need updating */
    for (ClusterNetId net_id : clb_nlist_.nets()) {
        for (ClusterPinId pin_id : clb_nlist_.net_sinks(net_id)) {
            cluster_pins_with_modified_setup_slack_.insert(pin_id);
        }
    }
}

///@brief Override the setup slack of a particular connection.
void PlacerSetupSlacks::set_setup_slack(ClusterNetId net_id, int ipin, float slack_val) {
    VTR_ASSERT_SAFE_MSG(ipin > 0, "The pin should not be a driver pin (ipin != 0)");
    VTR_ASSERT_SAFE_MSG(ipin < int(clb_nlist_.net_pins(net_id).size()), "The pin index in net should be smaller than fanout");

    timing_place_setup_slacks_[net_id][ipin] = slack_val;
}

/**
 * @brief Returns the range of clustered netlist pins (i.e. ClusterPinIds)
 *        which were modified by the last call to PlacerSetupSlacks::update_setup_slacks().
 */
PlacerSetupSlacks::pin_range PlacerSetupSlacks::pins_with_modified_setup_slack() const {
    return vtr::make_range(cluster_pins_with_modified_setup_slack_);
}
