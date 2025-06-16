
#include "PlacerCriticalities.h"

#include "timing_info.h"
#include "timing_util.h"

PlacerCriticalities::PlacerCriticalities(const ClusteredNetlist& clb_nlist,
                                         const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                         std::shared_ptr<const SetupTimingInfo> timing_info)
    : clb_nlist_(clb_nlist)
    , pin_lookup_(netlist_pin_lookup)
    , timing_info_(std::move(timing_info))
    , timing_place_crit_(make_net_pins_matrix(clb_nlist_, std::numeric_limits<float>::quiet_NaN())) {
}

void PlacerCriticalities::update_criticalities(const PlaceCritParams& crit_params) {
    // If update is not enabled, exit the routine.
    if (!update_enabled) {
        // re-computation is required on the next iteration
        recompute_required = true;
        return;
    }

    // Determine what pins need updating
    if (!recompute_required && crit_params.crit_exponent == last_crit_exponent_) {
        incr_update_criticalities();
    } else {
        recompute_criticalities();

        // Record new criticality exponent
        last_crit_exponent_ = crit_params.crit_exponent;
    }

    /* Performs a 1-to-1 mapping from criticality to timing_place_crit_.
     * For every pin on every net (or, equivalently, for every tedge ending
     * in that pin), timing_place_crit_ = criticality^(criticality exponent) */

    // Update the affected pins
    for (ClusterPinId clb_pin : cluster_pins_with_modified_criticality_) {
        ClusterNetId clb_net = clb_nlist_.pin_net(clb_pin);
        int pin_index_in_net = clb_nlist_.pin_net_index(clb_pin);

        float clb_pin_crit = calculate_clb_net_pin_criticality(*timing_info_, pin_lookup_, ParentPinId(size_t(clb_pin)), /*is_flat=*/false);
        float new_crit = pow(clb_pin_crit, crit_params.crit_exponent);

        /* Update the highly critical pins container
         *
         * If the old criticality < limit and the new criticality > limit --> add this pin to the highly critical pins
         * If the old criticality > limit and the new criticality < limit --> remove this pin from the highly critical pins
         */
        if (!first_time_update_criticality) {
            if (new_crit > crit_params.crit_limit && timing_place_crit_[clb_net][pin_index_in_net] < crit_params.crit_limit) {
                highly_crit_pins.emplace_back(clb_net, pin_index_in_net);
            } else if (new_crit < crit_params.crit_limit && timing_place_crit_[clb_net][pin_index_in_net] > crit_params.crit_limit) {
                highly_crit_pins.erase(std::remove(highly_crit_pins.begin(), highly_crit_pins.end(), std::make_pair(clb_net, pin_index_in_net)),
                                       highly_crit_pins.end());
            }
        } else {
            if (new_crit > crit_params.crit_limit) {
                highly_crit_pins.emplace_back(clb_net, pin_index_in_net);
            }
        }

        /* The placer likes a great deal of contrast between criticalities.
         * Since path criticality varies much more than timing, we "sharpen" timing
         * criticality by taking it to some power, crit_exponent (between 1 and 8 by default). */
        timing_place_crit_[clb_net][pin_index_in_net] = new_crit;
    }

    /* Criticalities updated. In sync with timing info.
     * Can be incrementally updated on the next iteration */
    recompute_required = false;

    first_time_update_criticality = false;
}

void PlacerCriticalities::set_recompute_required() {
    recompute_required = true;
}

void PlacerCriticalities::incr_update_criticalities() {
    cluster_pins_with_modified_criticality_.clear();

    for (AtomPinId atom_pin : timing_info_->pins_with_modified_setup_criticality()) {
        ClusterPinId clb_pin = pin_lookup_.connected_clb_pin(atom_pin);

        /* Some atom pins correspond to connections which are completely
         * contained within a cluster, and hence have no corresponding
         * clustered pin. */
        if (!clb_pin) continue;

        cluster_pins_with_modified_criticality_.insert(clb_pin);
    }
}

void PlacerCriticalities::recompute_criticalities() {
    cluster_pins_with_modified_criticality_.clear();

    // Non-incremental: all sink pins need updating
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

PlacerCriticalities::pin_range PlacerCriticalities::pins_with_modified_criticality() const {
    return vtr::make_range(cluster_pins_with_modified_criticality_);
}
