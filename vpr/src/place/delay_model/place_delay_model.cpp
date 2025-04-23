/**
 * @file place_delay_model.cpp
 * @brief This file implements all the class methods and individual
 *        routines related to the placer delay model.
 */

#include "place_delay_model.h"

#include "globals.h"
#include "physical_types_util.h"
#include "placer_state.h"
#include "vpr_error.h"

/**
 * @brief Returns the delay of one point to point connection.
 *
 * Only estimate delay for signals routed through the inter-block routing network.
 * TODO: Do how should we compute the delay for globals. "Global signals are assumed to have zero delay."
 */
float comp_td_single_connection_delay(const PlaceDelayModel* delay_model,
                                      const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs,
                                      ClusterNetId net_id,
                                      int ipin) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();

    float delay_source_to_sink = 0.;

    if (!cluster_ctx.clb_nlist.net_is_ignored(net_id)) {
        ClusterPinId source_pin = cluster_ctx.clb_nlist.net_driver(net_id);
        ClusterPinId sink_pin = cluster_ctx.clb_nlist.net_pin(net_id, ipin);

        ClusterBlockId source_block = cluster_ctx.clb_nlist.pin_block(source_pin);
        ClusterBlockId sink_block = cluster_ctx.clb_nlist.pin_block(sink_pin);

        int source_block_ipin = cluster_ctx.clb_nlist.pin_logical_index(source_pin);
        int sink_block_ipin = cluster_ctx.clb_nlist.pin_logical_index(sink_pin);

        t_pl_loc source_block_loc = block_locs[source_block].loc;
        t_pl_loc sink_block_loc = block_locs[sink_block].loc;

        /**
         * This heuristic only considers delta_x and delta_y, a much better
         * heuristic would be to to create a more comprehensive lookup table.
         *
         * In particular this approach does not accurately capture the effect
         * of fast carry-chain connections.
         */
        delay_source_to_sink = delay_model->delay({source_block_loc.x, source_block_loc.y, source_block_loc.layer}, source_block_ipin,
                                                  {sink_block_loc.x, sink_block_loc.y, sink_block_loc.layer}, sink_block_ipin);
        if (delay_source_to_sink < 0) {
            VPR_ERROR(VPR_ERROR_PLACE,
                      "in comp_td_single_connection_delay: Bad delay_source_to_sink value %g from %s (at %d,%d,%d) to %s (at %d,%d,%d)\n"
                      "in comp_td_single_connection_delay: Delay is less than 0\n",
                      block_type_pin_index_to_name(physical_tile_type(source_block_loc), source_block_ipin, false).c_str(),
                      source_block_loc.x, source_block_loc.y, source_block_loc.layer,
                      block_type_pin_index_to_name(physical_tile_type(sink_block_loc), sink_block_ipin, false).c_str(),
                      sink_block_loc.x, sink_block_loc.y, sink_block_loc.layer,
                      delay_source_to_sink);
        }
    }

    return (delay_source_to_sink);
}

///@brief Recompute all point to point delays, updating `connection_delay` matrix.
void comp_td_connection_delays(const PlaceDelayModel* delay_model,
                               PlacerState& placer_state) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& p_timing_ctx = placer_state.mutable_timing();
    auto& block_locs = placer_state.block_locs();
    auto& connection_delay = p_timing_ctx.connection_delay;

    for (ClusterNetId net_id : cluster_ctx.clb_nlist.nets()) {
        for (size_t ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ++ipin) {
            connection_delay[net_id][ipin] = comp_td_single_connection_delay(delay_model, block_locs, net_id, ipin);
        }
    }
}
