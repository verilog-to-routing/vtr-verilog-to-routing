
#include "placer_state.h"

#include "globals.h"
#include "move_transactions.h"

PlacerMoveContext::PlacerMoveContext(bool cube_bb) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& cluster_ctx = g_vpr_ctx.clustering();

    const size_t num_nets = cluster_ctx.clb_nlist.nets().size();

    const int num_layers = device_ctx.grid.get_num_layers();

    if (cube_bb) {
        bb_coords.resize(num_nets, t_bb());
        bb_num_on_edges.resize(num_nets, t_bb());
    } else {
        layer_bb_num_on_edges.resize(num_nets, std::vector<t_2D_bb>(num_layers, t_2D_bb()));
        layer_bb_coords.resize(num_nets, std::vector<t_2D_bb>(num_layers, t_2D_bb()));
    }

    num_sink_pin_layer.resize({num_nets, size_t(num_layers)});
    for (size_t flat_idx = 0; flat_idx < num_sink_pin_layer.size(); flat_idx++) {
        int& elem = num_sink_pin_layer.get(flat_idx);
        elem = OPEN;
    }
}

PlacerTimingContext::PlacerTimingContext(bool placement_is_timing_driven) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();

    const size_t num_nets = cluster_ctx.clb_nlist.nets().size();

    if (placement_is_timing_driven) {
        connection_delay = make_net_pins_matrix<float>((const Netlist<>&)cluster_ctx.clb_nlist, 0.f);
        proposed_connection_delay = make_net_pins_matrix<float>(cluster_ctx.clb_nlist, 0.f);

        connection_setup_slack = make_net_pins_matrix<float>(cluster_ctx.clb_nlist, std::numeric_limits<float>::infinity());

        connection_timing_cost = PlacerTimingCosts(cluster_ctx.clb_nlist);
        proposed_connection_timing_cost = make_net_pins_matrix<double>(cluster_ctx.clb_nlist, 0.);
        net_timing_cost.resize(num_nets, 0.);

        for (ClusterNetId net_id : cluster_ctx.clb_nlist.nets()) {
            for (size_t ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ipin++) {
                connection_delay[net_id][ipin] = 0;
                proposed_connection_delay[net_id][ipin] = INVALID_DELAY;

                proposed_connection_timing_cost[net_id][ipin] = INVALID_DELAY;

                if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) {
                    continue;
                }

                connection_timing_cost[net_id][ipin] = INVALID_DELAY;
            }
        }
    }
}

void PlacerTimingContext::commit_td_cost(const t_pl_blocks_to_be_moved& blocks_affected) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& clb_nlist = cluster_ctx.clb_nlist;

    // Go through all the sink pins affected
    for (ClusterPinId pin_id : blocks_affected.affected_pins) {
        ClusterNetId net_id = clb_nlist.pin_net(pin_id);
        int ipin = clb_nlist.pin_net_index(pin_id);

        // Commit the timing delay and cost values
        connection_delay[net_id][ipin] = proposed_connection_delay[net_id][ipin];
        proposed_connection_delay[net_id][ipin] = INVALID_DELAY;
        connection_timing_cost[net_id][ipin] = proposed_connection_timing_cost[net_id][ipin];
        proposed_connection_timing_cost[net_id][ipin] = INVALID_DELAY;
    }
}

void PlacerTimingContext::revert_td_cost(const t_pl_blocks_to_be_moved& blocks_affected) {
#ifndef VTR_ASSERT_SAFE_ENABLED
    (void)blocks_affected;
#else
    //Invalidate temp delay & timing cost values to match sanity checks in
    //comp_td_connection_cost()
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& clb_nlist = cluster_ctx.clb_nlist;


    for (ClusterPinId pin : blocks_affected.affected_pins) {
        ClusterNetId net = clb_nlist.pin_net(pin);
        int ipin = clb_nlist.pin_net_index(pin);
        proposed_connection_delay[net][ipin] = INVALID_DELAY;
        proposed_connection_timing_cost[net][ipin] = INVALID_DELAY;
    }
#endif
}

PlacerState::PlacerState(bool placement_is_timing_driven, bool cube_bb)
    : timing_(placement_is_timing_driven)
    , move_(cube_bb) {}

