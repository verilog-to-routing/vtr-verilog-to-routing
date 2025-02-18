
#include "PlacerSetupSlacks.h"

#include "timing_util.h"
#include "timing_info.h"


PlacerSetupSlacks::PlacerSetupSlacks(const ClusteredNetlist& clb_nlist,
                                     const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                     std::shared_ptr<const SetupTimingInfo> timing_info)
    : clb_nlist_(clb_nlist)
    , pin_lookup_(netlist_pin_lookup)
    , timing_info_(std::move(timing_info))
    , timing_place_setup_slacks_(make_net_pins_matrix(clb_nlist_, std::numeric_limits<float>::quiet_NaN())) {
}

void PlacerSetupSlacks::update_setup_slacks() {
    // If update is not enabled, exit the routine.
    if (!update_enabled) {
        // re-computation is required on the next iteration
        recompute_required = true;
        return;
    }

    // Determine what pins need updating
    if (!recompute_required) {
        incr_update_setup_slacks();
    } else {
        recompute_setup_slacks();
    }

    // Update the affected pins
    for (ClusterPinId clb_pin : cluster_pins_with_modified_setup_slack_) {
        ClusterNetId clb_net = clb_nlist_.pin_net(clb_pin);
        int pin_index_in_net = clb_nlist_.pin_net_index(clb_pin);

        float clb_pin_setup_slack = calculate_clb_net_pin_setup_slack(*timing_info_, pin_lookup_, clb_pin);

        timing_place_setup_slacks_[clb_net][pin_index_in_net] = clb_pin_setup_slack;
    }

    /* Setup slacks updated. In sync with timing info.
     * Can be incrementally updated on the next iteration. */
    recompute_required = false;
}

void PlacerSetupSlacks::incr_update_setup_slacks() {
    cluster_pins_with_modified_setup_slack_.clear();

    for (AtomPinId atom_pin : timing_info_->pins_with_modified_setup_slack()) {
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

    // Non-incremental: all sink pins need updating
    for (ClusterNetId net_id : clb_nlist_.nets()) {
        for (ClusterPinId pin_id : clb_nlist_.net_sinks(net_id)) {
            cluster_pins_with_modified_setup_slack_.insert(pin_id);
        }
    }
}

void PlacerSetupSlacks::set_setup_slack(ClusterNetId net_id, int ipin, float slack_val) {
    VTR_ASSERT_SAFE_MSG(ipin > 0, "The pin should not be a driver pin (ipin != 0)");
    VTR_ASSERT_SAFE_MSG(ipin < int(clb_nlist_.net_pins(net_id).size()), "The pin index in net should be smaller than fanout");

    timing_place_setup_slacks_[net_id][ipin] = slack_val;
}

PlacerSetupSlacks::pin_range PlacerSetupSlacks::pins_with_modified_setup_slack() const {
    return vtr::make_range(cluster_pins_with_modified_setup_slack_);
}
