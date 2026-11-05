
#include "placer_state.h"

#include "clustered_netlist.h"
#include "globals.h"
#include "vpr_context.h"

PlacerTimingContext::PlacerTimingContext(bool placement_is_timing_driven, bool placement_uses_setup_slacks) {
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();

    const size_t num_nets = cluster_ctx.clb_nlist.nets().size();

    if (placement_is_timing_driven) {
        connection_delay = make_net_pins_matrix<float>((const Netlist<>&)cluster_ctx.clb_nlist, 0.f);

        if (placement_uses_setup_slacks) {
            connection_setup_slack = make_net_pins_matrix<float>(cluster_ctx.clb_nlist, std::numeric_limits<float>::infinity());
        }

        connection_timing_cost = PlacerTimingCosts(cluster_ctx.clb_nlist);
        net_timing_cost.resize(num_nets, 0.);

        for (ClusterNetId net_id : cluster_ctx.clb_nlist.nets()) {
            for (size_t ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ipin++) {
                connection_delay[net_id][ipin] = 0;

                if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) {
                    continue;
                }

                connection_timing_cost[net_id][ipin] = INVALID_DELAY;
            }
        }
    }
}

void PlacerTimingContext::commit_td_cost() {
    // Commit the timing delay and cost values of every affected connection
    for (const t_connection_commit_entry& entry : proposed_connection_timing) {
        connection_delay[entry.net_id][entry.ipin] = entry.connection_delay;
        connection_timing_cost[entry.net_id][entry.ipin] = entry.connection_timing_cost;
    }

    proposed_connection_timing.clear();
}

void PlacerTimingContext::extract_connection_commit_record(std::vector<t_connection_commit_entry>& record) const {
    record = proposed_connection_timing;
}

void PlacerTimingContext::apply_connection_commit_record(const std::vector<t_connection_commit_entry>& record) {
    VTR_ASSERT_SAFE_MSG(proposed_connection_timing.empty(),
                        "A commit record can only be applied while no move is in flight.");

    // Mirrors the writing side of commit_td_cost().
    for (const t_connection_commit_entry& entry : record) {
        connection_delay[entry.net_id][entry.ipin] = entry.connection_delay;
        connection_timing_cost[entry.net_id][entry.ipin] = entry.connection_timing_cost;
    }
}

void PlacerTimingContext::revert_td_cost() {
    proposed_connection_timing.clear();
}

PlacerState::PlacerState(bool placement_is_timing_driven, bool placement_uses_setup_slacks)
    : timing_(placement_is_timing_driven, placement_uses_setup_slacks) {}
