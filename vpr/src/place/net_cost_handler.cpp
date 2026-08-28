/**
 * @file net_cost_handler.cpp
 * @brief This file contains the implementation of functions used to update placement cost when a new move is proposed/committed.
 *
 * VPR placement cost consists of multiple terms which represent wirelength, timing, congestion, and NoC cost.
 *
 * To get an estimation of the wirelength of each net, the Half Perimeter Wire Length (HPWL) metric is used. In this approach,
 * half of the perimeter of the bounding box which contains all terminals of the net is multiplied by a correction factor,
 * and the resulting number is considered as an estimation of the wirelength needed to route this net.
 *
 * A 3D (cube) bounding box is used for every net. For 2D architectures the bounding box always has a z extent of 1.
 * For 3D architectures, when a net is stretched across multiple layers, the edges of the bounding box are determined
 * by all of the blocks on all layers.
 *
 * For timing estimation, the placement delay model is used. For 2D architectures, you can think of the placement delay model as a 2D array indexed by dx and dy.
 * To get a delay estimation of a connection (from a source to a sink), first, dx and dy between these two points should be calculated,
 * and these two numbers are the indices to access this 2D array. By default, the placement delay model is created by iterating over the router lookahead
 * to get the minimum cost for each dx and dy.
 *
 * For congestion modeling, we periodically estimate routing channel usage by distributing the estimated
 * wirelength (WL) of each net across all routing channels within its bounding box. The wirelength is divided
 * between CHANX and CHANY in proportion to the bounding box's width and height, respectively. However, all
 * routing channels of the same type (CHANX or CHANY) within the box receive an equal share of that net's WL.
 *
 * We compute a congestion cost for each net by averaging the estimated utilization over all CHANX and CHANY
 * channels in its bounding box. These average utilizations are then compared to a user-specified threshold.
 * If a net’s average utilization exceeds the threshold, the excess is penalized by adding a cost proportional
 * to the amount of the exceedance.
 */
#include "net_cost_handler.h"

#include "clustered_netlist.h"
#include "clustered_netlist_fwd.h"
#include "device_grid.h"
#include "globals.h"
#include "physical_types.h"
#include "placer_state.h"
#include "move_utils.h"
#include "place_timing_update.h"
#include "vpr_context.h"
#include "vtr_math.h"
#include "vtr_ndmatrix.h"
#include "PlacerCriticalities.h"
#include "vtr_prefix_sum.h"
#include "stats.h"

#include <algorithm>
#include <array>
#include <vector>

static constexpr int MAX_FANOUT_CROSSING_COUNT = 50;

/**
 * @brief Crossing counts for nets with different #'s of pins.  From
 * ICCAD 94 pp. 690 - 695 (with linear interpolation applied by me).
 * Multiplied to bounding box of a net to better estimate wire length
 * for higher fanout nets. Each entry is the correction factor for the
 * fanout index-1
 */
constexpr std::array<float, MAX_FANOUT_CROSSING_COUNT> cross_count = {1.0000, 1.0000, 1.0000, 1.0828, 1.1536, 1.2206, 1.2823, 1.3385,
                                                                      1.3991, 1.4493, 1.4974, 1.5455, 1.5937, 1.6418, 1.6899, 1.7304,
                                                                      1.7709, 1.8114, 1.8519, 1.8924, 1.9288, 1.9652, 2.0015, 2.0379,
                                                                      2.0743, 2.1061, 2.1379, 2.1698, 2.2016, 2.2334, 2.2646, 2.2958,
                                                                      2.3271, 2.3583, 2.3895, 2.4187, 2.4479, 2.4772, 2.5064, 2.5356,
                                                                      2.5610, 2.5864, 2.6117, 2.6371, 2.6625, 2.6887, 2.7148, 2.7410,
                                                                      2.7671, 2.7933};

NetCostHandler::NetCostHandler(PlacerState& placer_state,
                               t_place_algorithm place_algorithm,
                               double congestion_chan_util_threshold)
    : congestion_modeling_started_(false)
    , placer_state_(placer_state)
    , place_algorithm_(place_algorithm)
    , congestion_chan_util_threshold_(congestion_chan_util_threshold) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const DeviceGrid& grid = device_ctx.grid;

    const size_t num_layers = grid.get_num_layers();
    const size_t num_nets = g_vpr_ctx.clustering().clb_nlist.nets().size();

    is_multi_layer_ = num_layers > 1;

    ts_bb_edge_new_.resize(num_nets, t_bb());
    ts_bb_coord_new_.resize(num_nets, t_bb());

    // Committed bounding box state. Costs start negative, meaning not computed yet.
    net_bb_.resize(num_nets);

    ts_nets_to_update_.resize(num_nets, ClusterNetId::INVALID());

    // Used to store costs for moves not yet made and to indicate when a net's
    // cost has been recomputed. proposed_net_cost_[inet] < 0 means net's cost hasn't
    // been recomputed.
    proposed_net_cost_.resize(num_nets, -1.);

    // Tracks how far each net's bounding box has been updated for the proposed move.
    bb_update_status_.resize(num_nets, NetUpdateState::NOT_UPDATED_YET);

    alloc_and_load_chan_w_factors_for_place_cost_();

    // Congestion-related data members are not allocated until congestion modeling is enabled
    // by calling estimate_routing_chan_util().
    VTR_ASSERT(!congestion_modeling_started_);
    VTR_ASSERT(chan_util_.x.empty() && chan_util_.y.empty());
    VTR_ASSERT(acc_chan_util_.x.empty() && acc_chan_util_.y.empty());
    VTR_ASSERT(ts_avg_chan_util_new_.empty());
    VTR_ASSERT(net_cong_.empty() && proposed_net_cong_cost_.empty());
}

void NetCostHandler::alloc_and_load_chan_w_factors_for_place_cost_() {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const DeviceGrid& grid = device_ctx.grid;

    const size_t grid_height = grid.height();
    const size_t grid_width = grid.width();

    // These arrays contain accumulative channel width between channel zero and
    // the channel specified by the given index. The accumulated channel width
    // is inclusive, meaning that it includes both channel zero and channel `idx`.
    // To compute the total channel width between channels 'low' and 'high', use the
    // following formula:
    //      acc_chan?_width_[high] - acc_chan?_width_[low - 1]
    // This returns the total number of tracks between channels 'low' and 'high',
    // including tracks in these channels.
    acc_chan_width_.x = vtr::PrefixSum1D<int>(grid_height, [&](size_t y) noexcept {
        VTR_ASSERT_SAFE_MSG(y < device_ctx.rr_chan_width.x.size(), "Prefix sum sample point should be within bounds.");
        int chan_x_width = device_ctx.rr_chan_width.x[y];

        // If the number of tracks in a channel is zero, two consecutive elements take the same
        // value. This can lead to a division by zero in get_chanxy_cost_fac_(). To avoid this
        // potential issue, we assume that the channel width is at least 1.
        if (chan_x_width == 0) {
            return 1;
        }

        return chan_x_width;
    });

    acc_chan_width_.y = vtr::PrefixSum1D<int>(grid_width, [&](size_t x) noexcept {
        VTR_ASSERT_SAFE_MSG(x < device_ctx.rr_chan_width.y.size(), "Prefix sum sample point should be within bounds.");
        int chan_y_width = device_ctx.rr_chan_width.y[x];

        // to avoid a division by zero
        if (chan_y_width == 0) {
            return 1;
        }

        return chan_y_width;
    });

    if (is_multi_layer_) {
        // Calculate prefix sum of the inter-die connectivity up to and including the channel at (x, y).
        acc_tile_num_inter_die_conn_ = vtr::PrefixSum2D<int>(grid_width,
                                                             grid_height,
                                                             [&](size_t x, size_t y) {
                                                                 return device_ctx.rr_chan_segment_width.z[0][x][y];
                                                             });
    }
}

std::pair<t_net_cost_terms, double> NetCostHandler::comp_bb_cong_cost(e_cost_methods method) {
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();

    t_net_cost_terms cost_terms;
    double expected_wirelength = 0.;

    for (ClusterNetId net_id : cluster_ctx.clb_nlist.non_ignored_nets()) {
        // Small nets don't use incremental updating on their bounding boxes,
        // so they can use a fast bounding box calculator.
        if (cluster_ctx.clb_nlist.net_sinks(net_id).size() >= SMALL_NET && method == e_cost_methods::NORMAL) {
            get_bb_from_scratch_(net_id, /*use_ts=*/false);
        } else {
            get_non_updatable_bb_(net_id, /*use_ts=*/false);
        }

        t_net_bb_info& net_bb = net_bb_[net_id];
        net_bb.cost = get_net_bb_cost_(net_id, net_bb.coords);
        cost_terms.bb_cost += net_bb.cost;
        if (method == e_cost_methods::CHECK) {
            expected_wirelength += get_net_wirelength_estimate_(net_id);
        }
    }

    // Compute congestion cost using recomputed bounding boxes and channel utilization map
    if (congestion_modeling_started_) {
        for (ClusterNetId net_id : cluster_ctx.clb_nlist.non_ignored_nets()) {
            t_net_cong_info& net_cong = net_cong_[net_id];
            net_cong.cost = get_net_cong_cost_(net_cong.avg_chan_util);
            cost_terms.cong_cost += net_cong.cost;
        }
    }

    return {cost_terms, expected_wirelength};
}

void NetCostHandler::update_net_bb_(const ClusterNetId net,
                                    const ClusterBlockId blk,
                                    const ClusterPinId blk_pin,
                                    const t_pl_moved_block& pl_moved_block) {
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
    const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs = placer_state_.block_locs();

    if (cluster_ctx.clb_nlist.net_sinks(net).size() < SMALL_NET) {
        //For small nets brute-force bounding box update is faster

        if (bb_update_status_[net] == NetUpdateState::NOT_UPDATED_YET) { //Only once per-net
            get_non_updatable_bb_(net, /*use_ts=*/true);
        }
    } else {
        //For large nets, update bounding box incrementally
        int iblk_pin = placer_state_.blk_loc_registry().tile_pin_index(blk_pin);

        t_pl_loc block_loc = block_locs[blk].loc;
        t_physical_tile_type_ptr blk_type = physical_tile_type(block_loc);
        int pin_width_offset = blk_type->pin_width_offset[iblk_pin];
        int pin_height_offset = blk_type->pin_height_offset[iblk_pin];

        //Incremental bounding box update
        update_bb_(net,
                   {pl_moved_block.old_loc.x + pin_width_offset,
                    pl_moved_block.old_loc.y + pin_height_offset,
                    pl_moved_block.old_loc.layer},
                   {pl_moved_block.new_loc.x + pin_width_offset,
                    pl_moved_block.new_loc.y + pin_height_offset,
                    pl_moved_block.new_loc.layer});
    }
}

void NetCostHandler::update_td_delta_costs_(const PlaceDelayModel* delay_model,
                                            const PlacerCriticalities& criticalities,
                                            const ClusterNetId net,
                                            const ClusterPinId pin,
                                            std::vector<ClusterPinId>& affected_pins,
                                            double& delta_timing_cost,
                                            bool is_src_moving) {
    /**
     * Assumes that the blocks have been moved to the proposed new locations.
     * Otherwise, the routine comp_td_single_connection_delay() will not be
     * able to calculate the most up to date connection delay estimation value.
     *
     * If the moved pin is a driver pin, then all the sink connections that are
     * driven by this driver pin are considered.
     *
     * If the moved pin is a sink pin, then it is the only pin considered. But
     * in some cases, the sink is already accounted for if it is also driven
     * by a driver pin located on a moved block. Computing it again would double
     * count its affect on the total timing cost change (delta_timing_cost).
     *
     * It is possible for some connections to have unchanged delays. For instance,
     * if we are using a dx/dy delay model, this could occur if a sink pin moved
     * to a new position with the same dx/dy from its net's driver pin.
     *
     * We skip these connections with unchanged delay values as their delay need
     * not be updated. Their timing costs also do not require any update, since
     * the criticalities values are always kept stale/unchanged during an block
     * swap attempt. (Unchanged Delay * Unchanged Criticality = Unchanged Cost)
     *
     * This is also done to minimize the number of timing node/edge invalidations
     * for incremental static timing analysis (incremental STA).
     */
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
    const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs = placer_state_.block_locs();

    const NetPinsMatrix<float>& connection_delay = placer_state_.timing().connection_delay;
    PlacerTimingCosts& connection_timing_cost = placer_state_.mutable_timing().connection_timing_cost;
    ClbNetPinsMatrix<float>& proposed_connection_delay = placer_state_.mutable_timing().proposed_connection_delay;
    ClbNetPinsMatrix<double>& proposed_connection_timing_cost = placer_state_.mutable_timing().proposed_connection_timing_cost;

    if (cluster_ctx.clb_nlist.pin_type(pin) == PinType::DRIVER) {
        /* This pin is a net driver on a moved block. */
        /* Recompute all point to point connection delays for the net sinks. */
        for (size_t ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net).size(); ipin++) {
            float temp_delay = comp_td_single_connection_delay(delay_model, block_locs, net, ipin);
            /* If the delay hasn't changed, do not mark this pin as affected */
            if (temp_delay == connection_delay[net][ipin]) {
                continue;
            }

            /* Calculate proposed delay and cost values */
            proposed_connection_delay[net][ipin] = temp_delay;

            proposed_connection_timing_cost[net][ipin] = criticalities.criticality(net, ipin) * temp_delay;
            delta_timing_cost += proposed_connection_timing_cost[net][ipin] - connection_timing_cost[net][ipin];

            /* Record this connection in blocks_affected.affected_pins */
            ClusterPinId sink_pin = cluster_ctx.clb_nlist.net_pin(net, ipin);
            affected_pins.push_back(sink_pin);
        }
    } else {
        /* This pin is a net sink on a moved block */
        VTR_ASSERT_SAFE(cluster_ctx.clb_nlist.pin_type(pin) == PinType::SINK);

        /* Check if this sink's net is driven by a moved block */
        if (!is_src_moving) {
            /* Get the sink pin index in the net */
            int ipin = cluster_ctx.clb_nlist.pin_net_index(pin);

            float temp_delay = comp_td_single_connection_delay(delay_model, block_locs, net, ipin);
            /* If the delay hasn't changed, do not mark this pin as affected */
            if (temp_delay == connection_delay[net][ipin]) {
                return;
            }

            /* Calculate proposed delay and cost values */
            proposed_connection_delay[net][ipin] = temp_delay;

            proposed_connection_timing_cost[net][ipin] = criticalities.criticality(net, ipin) * temp_delay;
            delta_timing_cost += proposed_connection_timing_cost[net][ipin] - connection_timing_cost[net][ipin];

            /* Record this connection in blocks_affected.affected_pins */
            affected_pins.push_back(pin);
        }
    }
}

void NetCostHandler::record_affected_net_(const ClusterNetId net) {
    /* Record effected nets. */
    if (proposed_net_cost_[net] < 0.) {
        /* Net not marked yet. */
        VTR_ASSERT_SAFE(ts_nets_to_update_.size() < ts_nets_to_update_.capacity());
        ts_nets_to_update_.push_back(net);

        /* Flag to say we've marked this net. */
        proposed_net_cost_[net] = 1.;
    }
}

void NetCostHandler::update_net_info_on_pin_move_(const PlaceDelayModel* delay_model,
                                                  const PlacerCriticalities* criticalities,
                                                  const ClusterPinId pin_id,
                                                  const t_pl_moved_block& moving_blk_inf,
                                                  std::vector<ClusterPinId>& affected_pins,
                                                  double& timing_delta_c,
                                                  bool is_src_moving) {
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();

    const ClusterNetId net_id = cluster_ctx.clb_nlist.pin_net(pin_id);
    VTR_ASSERT_SAFE_MSG(net_id,
                        "Only valid nets should be found in compressed netlist block pins");

    if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) {
        //TODO: Do we require anything special here for global nets?
        //"Global nets are assumed to span the whole chip, and do not effect costs."
        return;
    }

    // Record effected nets
    record_affected_net_(net_id);

    ClusterBlockId blk_id = moving_blk_inf.block_num;
    // Update the net bounding boxes.
    update_net_bb_(net_id, blk_id, pin_id, moving_blk_inf);

    if (place_algorithm_.is_timing_driven()) {
        // Determine the change in connection delay and timing cost.
        update_td_delta_costs_(delay_model,
                               *criticalities,
                               net_id,
                               pin_id,
                               affected_pins,
                               timing_delta_c,
                               is_src_moving);
    }
}

void NetCostHandler::get_non_updatable_bb_(ClusterNetId net_id, bool use_ts) {
    //TODO: account for multiple physical pin instances per logical pin
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
    const BlkLocRegistry& blk_loc_registry = placer_state_.blk_loc_registry();

    // the bounding box coordinates that is going to be updated by this function
    t_bb& bb_coord_new = use_ts ? ts_bb_coord_new_[net_id] : net_bb_[net_id].coords;

    // get the source pin's location
    ClusterPinId source_pin_id = cluster_ctx.clb_nlist.net_pin(net_id, 0);
    t_physical_tile_loc source_pin_loc = blk_loc_registry.get_coordinate_of_pin(source_pin_id);

    // initialize the bounding box coordinates with the source pin's coordinates
    bb_coord_new.xmin = source_pin_loc.x;
    bb_coord_new.ymin = source_pin_loc.y;
    bb_coord_new.layer_min = source_pin_loc.layer_num;
    bb_coord_new.xmax = source_pin_loc.x;
    bb_coord_new.ymax = source_pin_loc.y;
    bb_coord_new.layer_max = source_pin_loc.layer_num;

    for (ClusterPinId pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
        t_physical_tile_loc pin_loc = blk_loc_registry.get_coordinate_of_pin(pin_id);

        if (pin_loc.x < bb_coord_new.xmin) {
            bb_coord_new.xmin = pin_loc.x;
        } else if (pin_loc.x > bb_coord_new.xmax) {
            bb_coord_new.xmax = pin_loc.x;
        }

        if (pin_loc.y < bb_coord_new.ymin) {
            bb_coord_new.ymin = pin_loc.y;
        } else if (pin_loc.y > bb_coord_new.ymax) {
            bb_coord_new.ymax = pin_loc.y;
        }

        if (pin_loc.layer_num < bb_coord_new.layer_min) {
            bb_coord_new.layer_min = pin_loc.layer_num;
        } else if (pin_loc.layer_num > bb_coord_new.layer_max) {
            bb_coord_new.layer_max = pin_loc.layer_num;
        }
    }

    // Update average CHANX and CHANY usage for this net within its bounding box if congestion modeling is enabled
    if (congestion_modeling_started_) {
        auto& [x_chan_util, y_chan_util] = use_ts ? ts_avg_chan_util_new_[net_id] : net_cong_[net_id].avg_chan_util;
        const int total_channels = (bb_coord_new.xmax - bb_coord_new.xmin + 1) * (bb_coord_new.ymax - bb_coord_new.ymin + 1);
        x_chan_util = acc_chan_util_.x.get_sum(bb_coord_new.xmin, bb_coord_new.ymin, bb_coord_new.xmax, bb_coord_new.ymax) / total_channels;
        y_chan_util = acc_chan_util_.y.get_sum(bb_coord_new.xmin, bb_coord_new.ymin, bb_coord_new.xmax, bb_coord_new.ymax) / total_channels;
    }
}

void NetCostHandler::update_bb_(ClusterNetId net_id,
                                t_physical_tile_loc pin_old_loc,
                                t_physical_tile_loc pin_new_loc) {
    //TODO: account for multiple physical pin instances per logical pin

    // Number of blocks on the edges of the bounding box
    t_bb& bb_edge_new = ts_bb_edge_new_[net_id];
    // Coordinates of the bounding box
    t_bb& bb_coord_new = ts_bb_coord_new_[net_id];

    /* Check if the net had been updated before. */
    if (bb_update_status_[net_id] == NetUpdateState::GOT_FROM_SCRATCH) {
        /* The net had been updated from scratch, DO NOT update again! */
        return;
    }

    const t_bb *curr_bb_edge, *curr_bb_coord;
    if (bb_update_status_[net_id] == NetUpdateState::NOT_UPDATED_YET) {
        /* The net had NOT been updated before, could use the old values */
        const t_net_bb_info& net_bb = net_bb_[net_id];
        curr_bb_edge = &net_bb.num_on_edges;
        curr_bb_coord = &net_bb.coords;
        bb_update_status_[net_id] = NetUpdateState::UPDATED_ONCE;
    } else {
        /* The net had been updated before, must use the new values */
        curr_bb_coord = &bb_coord_new;
        curr_bb_edge = &bb_edge_new;
    }

    /* Check if I can update the bounding box incrementally. */

    if (pin_new_loc.x < pin_old_loc.x) { /* Move to left. */

        /* Update the xmax fields for coordinates and number of edges first. */

        if (pin_old_loc.x == curr_bb_coord->xmax) { /* Old position at xmax. */
            if (curr_bb_edge->xmax == 1) {
                get_bb_from_scratch_(net_id, /*use_ts=*/true);
                bb_update_status_[net_id] = NetUpdateState::GOT_FROM_SCRATCH;
                return;
            } else {
                bb_edge_new.xmax = curr_bb_edge->xmax - 1;
                bb_coord_new.xmax = curr_bb_coord->xmax;
            }
        } else { /* Move to left, old position was not at xmax. */
            bb_coord_new.xmax = curr_bb_coord->xmax;
            bb_edge_new.xmax = curr_bb_edge->xmax;
        }

        /* Now do the xmin fields for coordinates and number of edges. */

        if (pin_new_loc.x < curr_bb_coord->xmin) { /* Moved past xmin */
            bb_coord_new.xmin = pin_new_loc.x;
            bb_edge_new.xmin = 1;
        } else if (pin_new_loc.x == curr_bb_coord->xmin) { /* Moved to xmin */
            bb_coord_new.xmin = pin_new_loc.x;
            bb_edge_new.xmin = curr_bb_edge->xmin + 1;
        } else { /* Xmin unchanged. */
            bb_coord_new.xmin = curr_bb_coord->xmin;
            bb_edge_new.xmin = curr_bb_edge->xmin;
        }
        /* End of move to left case. */

    } else if (pin_new_loc.x > pin_old_loc.x) { /* Move to right. */

        /* Update the xmin fields for coordinates and number of edges first. */

        if (pin_old_loc.x == curr_bb_coord->xmin) { /* Old position at xmin. */
            if (curr_bb_edge->xmin == 1) {
                get_bb_from_scratch_(net_id, /*use_ts=*/true);
                bb_update_status_[net_id] = NetUpdateState::GOT_FROM_SCRATCH;
                return;
            } else {
                bb_edge_new.xmin = curr_bb_edge->xmin - 1;
                bb_coord_new.xmin = curr_bb_coord->xmin;
            }
        } else { /* Move to right, old position was not at xmin. */
            bb_coord_new.xmin = curr_bb_coord->xmin;
            bb_edge_new.xmin = curr_bb_edge->xmin;
        }

        /* Now do the xmax fields for coordinates and number of edges. */

        if (pin_new_loc.x > curr_bb_coord->xmax) { /* Moved past xmax. */
            bb_coord_new.xmax = pin_new_loc.x;
            bb_edge_new.xmax = 1;
        } else if (pin_new_loc.x == curr_bb_coord->xmax) { /* Moved to xmax */
            bb_coord_new.xmax = pin_new_loc.x;
            bb_edge_new.xmax = curr_bb_edge->xmax + 1;
        } else { /* Xmax unchanged. */
            bb_coord_new.xmax = curr_bb_coord->xmax;
            bb_edge_new.xmax = curr_bb_edge->xmax;
        }
        /* End of move to right case. */

    } else { /* pin_new_loc.x == pin_old_loc.x -- no x motion. */
        bb_coord_new.xmin = curr_bb_coord->xmin;
        bb_coord_new.xmax = curr_bb_coord->xmax;
        bb_edge_new.xmin = curr_bb_edge->xmin;
        bb_edge_new.xmax = curr_bb_edge->xmax;
    }

    /* Now account for the y-direction motion. */

    if (pin_new_loc.y < pin_old_loc.y) { /* Move down. */

        /* Update the ymax fields for coordinates and number of edges first. */

        if (pin_old_loc.y == curr_bb_coord->ymax) { /* Old position at ymax. */
            if (curr_bb_edge->ymax == 1) {
                get_bb_from_scratch_(net_id, /*use_ts=*/true);
                bb_update_status_[net_id] = NetUpdateState::GOT_FROM_SCRATCH;
                return;
            } else {
                bb_edge_new.ymax = curr_bb_edge->ymax - 1;
                bb_coord_new.ymax = curr_bb_coord->ymax;
            }
        } else { /* Move down, old position was not at ymax. */
            bb_coord_new.ymax = curr_bb_coord->ymax;
            bb_edge_new.ymax = curr_bb_edge->ymax;
        }

        /* Now do the ymin fields for coordinates and number of edges. */

        if (pin_new_loc.y < curr_bb_coord->ymin) { /* Moved past ymin */
            bb_coord_new.ymin = pin_new_loc.y;
            bb_edge_new.ymin = 1;
        } else if (pin_new_loc.y == curr_bb_coord->ymin) { /* Moved to ymin */
            bb_coord_new.ymin = pin_new_loc.y;
            bb_edge_new.ymin = curr_bb_edge->ymin + 1;
        } else { /* ymin unchanged. */
            bb_coord_new.ymin = curr_bb_coord->ymin;
            bb_edge_new.ymin = curr_bb_edge->ymin;
        }
        /* End of move down case. */

    } else if (pin_new_loc.y > pin_old_loc.y) { /* Moved up. */

        /* Update the ymin fields for coordinates and number of edges first. */

        if (pin_old_loc.y == curr_bb_coord->ymin) { /* Old position at ymin. */
            if (curr_bb_edge->ymin == 1) {
                get_bb_from_scratch_(net_id, /*use_ts=*/true);
                bb_update_status_[net_id] = NetUpdateState::GOT_FROM_SCRATCH;
                return;
            } else {
                bb_edge_new.ymin = curr_bb_edge->ymin - 1;
                bb_coord_new.ymin = curr_bb_coord->ymin;
            }
        } else { /* Moved up, old position was not at ymin. */
            bb_coord_new.ymin = curr_bb_coord->ymin;
            bb_edge_new.ymin = curr_bb_edge->ymin;
        }

        /* Now do the ymax fields for coordinates and number of edges. */

        if (pin_new_loc.y > curr_bb_coord->ymax) { /* Moved past ymax. */
            bb_coord_new.ymax = pin_new_loc.y;
            bb_edge_new.ymax = 1;
        } else if (pin_new_loc.y == curr_bb_coord->ymax) { /* Moved to ymax */
            bb_coord_new.ymax = pin_new_loc.y;
            bb_edge_new.ymax = curr_bb_edge->ymax + 1;
        } else { /* ymax unchanged. */
            bb_coord_new.ymax = curr_bb_coord->ymax;
            bb_edge_new.ymax = curr_bb_edge->ymax;
        }
        /* End of move up case. */

    } else { /* pin_new_loc.y == yold -- no y motion. */
        bb_coord_new.ymin = curr_bb_coord->ymin;
        bb_coord_new.ymax = curr_bb_coord->ymax;
        bb_edge_new.ymin = curr_bb_edge->ymin;
        bb_edge_new.ymax = curr_bb_edge->ymax;
    }

    /* Now account for the layer motion. */
    if (is_multi_layer_) {
        if (pin_new_loc.layer_num < pin_old_loc.layer_num) {
            if (pin_old_loc.layer_num == curr_bb_coord->layer_max) {
                if (curr_bb_edge->layer_max == 1) {
                    get_bb_from_scratch_(net_id, /*use_ts=*/true);
                    bb_update_status_[net_id] = NetUpdateState::GOT_FROM_SCRATCH;
                    return;
                } else {
                    bb_edge_new.layer_max = curr_bb_edge->layer_max - 1;
                    bb_coord_new.layer_max = curr_bb_coord->layer_max;
                }
            } else {
                bb_coord_new.layer_max = curr_bb_coord->layer_max;
                bb_edge_new.layer_max = curr_bb_edge->layer_max;
            }

            if (pin_new_loc.layer_num < curr_bb_coord->layer_min) {
                bb_coord_new.layer_min = pin_new_loc.layer_num;
                bb_edge_new.layer_min = 1;
            } else if (pin_new_loc.layer_num == curr_bb_coord->layer_min) {
                bb_coord_new.layer_min = pin_new_loc.layer_num;
                bb_edge_new.layer_min = curr_bb_edge->layer_min + 1;
            } else {
                bb_coord_new.layer_min = curr_bb_coord->layer_min;
                bb_edge_new.layer_min = curr_bb_edge->layer_min;
            }

        } else if (pin_new_loc.layer_num > pin_old_loc.layer_num) {
            if (pin_old_loc.layer_num == curr_bb_coord->layer_min) {
                if (curr_bb_edge->layer_min == 1) {
                    get_bb_from_scratch_(net_id, /*use_ts=*/true);
                    bb_update_status_[net_id] = NetUpdateState::GOT_FROM_SCRATCH;
                    return;
                } else {
                    bb_edge_new.layer_min = curr_bb_edge->layer_min - 1;
                    bb_coord_new.layer_min = curr_bb_coord->layer_min;
                }
            } else {
                bb_coord_new.layer_min = curr_bb_coord->layer_min;
                bb_edge_new.layer_min = curr_bb_edge->layer_min;
            }

            if (pin_new_loc.layer_num > curr_bb_coord->layer_max) {
                bb_coord_new.layer_max = pin_new_loc.layer_num;
                bb_edge_new.layer_max = 1;
            } else if (pin_new_loc.layer_num == curr_bb_coord->layer_max) {
                bb_coord_new.layer_max = pin_new_loc.layer_num;
                bb_edge_new.layer_max = curr_bb_edge->layer_max + 1;
            } else {
                bb_coord_new.layer_max = curr_bb_coord->layer_max;
                bb_edge_new.layer_max = curr_bb_edge->layer_max;
            }

        } else { //pin_new_loc.layer_num == pin_old_loc.layer_num
            bb_coord_new.layer_min = curr_bb_coord->layer_min;
            bb_coord_new.layer_max = curr_bb_coord->layer_max;
            bb_edge_new.layer_min = curr_bb_edge->layer_min;
            bb_edge_new.layer_max = curr_bb_edge->layer_max;
        }

    } else { // num_layers == 1
        bb_coord_new.layer_min = curr_bb_coord->layer_min;
        bb_coord_new.layer_max = curr_bb_coord->layer_max;
        bb_edge_new.layer_min = curr_bb_edge->layer_min;
        bb_edge_new.layer_max = curr_bb_edge->layer_max;
    }

    if (bb_update_status_[net_id] == NetUpdateState::NOT_UPDATED_YET) {
        bb_update_status_[net_id] = NetUpdateState::UPDATED_ONCE;
    }

    // Update average CHANX and CHANY usage for this net within its bounding box if congestion modeling is enabled
    if (congestion_modeling_started_) {
        auto& [x_chan_util, y_chan_util] = ts_avg_chan_util_new_[net_id];
        const int total_channels = (bb_coord_new.xmax - bb_coord_new.xmin + 1) * (bb_coord_new.ymax - bb_coord_new.ymin + 1);
        x_chan_util = acc_chan_util_.x.get_sum(bb_coord_new.xmin, bb_coord_new.ymin, bb_coord_new.xmax, bb_coord_new.ymax) / total_channels;
        y_chan_util = acc_chan_util_.y.get_sum(bb_coord_new.xmin, bb_coord_new.ymin, bb_coord_new.xmax, bb_coord_new.ymax) / total_channels;
    }
}

void NetCostHandler::get_bb_from_scratch_(ClusterNetId net_id, bool use_ts) {
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
    const BlkLocRegistry& blk_loc_registry = placer_state_.blk_loc_registry();

    t_bb& coords = use_ts ? ts_bb_coord_new_[net_id] : net_bb_[net_id].coords;
    t_bb& num_on_edges = use_ts ? ts_bb_edge_new_[net_id] : net_bb_[net_id].num_on_edges;

    // get the source pin's location
    ClusterPinId source_pin_id = cluster_ctx.clb_nlist.net_pin(net_id, 0);
    t_physical_tile_loc source_pin_loc = blk_loc_registry.get_coordinate_of_pin(source_pin_id);

    int xmin = source_pin_loc.x;
    int ymin = source_pin_loc.y;
    int layer_min = source_pin_loc.layer_num;
    int xmax = source_pin_loc.x;
    int ymax = source_pin_loc.y;
    int layer_max = source_pin_loc.layer_num;

    int xmin_edge = 1;
    int ymin_edge = 1;
    int layer_min_edge = 1;
    int xmax_edge = 1;
    int ymax_edge = 1;
    int layer_max_edge = 1;

    for (ClusterPinId pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
        t_physical_tile_loc pin_loc = blk_loc_registry.get_coordinate_of_pin(pin_id);

        if (pin_loc.x == xmin) {
            xmin_edge++;
        }
        if (pin_loc.x == xmax) { /* Recall that xmin could equal xmax -- don't use else */
            xmax_edge++;
        } else if (pin_loc.x < xmin) {
            xmin = pin_loc.x;
            xmin_edge = 1;
        } else if (pin_loc.x > xmax) {
            xmax = pin_loc.x;
            xmax_edge = 1;
        }

        if (pin_loc.y == ymin) {
            ymin_edge++;
        }
        if (pin_loc.y == ymax) {
            ymax_edge++;
        } else if (pin_loc.y < ymin) {
            ymin = pin_loc.y;
            ymin_edge = 1;
        } else if (pin_loc.y > ymax) {
            ymax = pin_loc.y;
            ymax_edge = 1;
        }

        if (pin_loc.layer_num == layer_min) {
            layer_min_edge++;
        }
        if (pin_loc.layer_num == layer_max) {
            layer_max_edge++;
        } else if (pin_loc.layer_num < layer_min) {
            layer_min = pin_loc.layer_num;
            layer_min_edge = 1;
        } else if (pin_loc.layer_num > layer_max) {
            layer_max = pin_loc.layer_num;
            layer_max_edge = 1;
        }
    }

    // Copy the coordinates and number on edges information into the proper structures.
    coords.xmin = xmin;
    coords.xmax = xmax;
    coords.ymin = ymin;
    coords.ymax = ymax;
    coords.layer_min = layer_min;
    coords.layer_max = layer_max;
    VTR_ASSERT_DEBUG(layer_min >= 0 && layer_min < (int)g_vpr_ctx.device().grid.get_num_layers());
    VTR_ASSERT_DEBUG(layer_max >= 0 && layer_max < (int)g_vpr_ctx.device().grid.get_num_layers());

    num_on_edges.xmin = xmin_edge;
    num_on_edges.xmax = xmax_edge;
    num_on_edges.ymin = ymin_edge;
    num_on_edges.ymax = ymax_edge;
    num_on_edges.layer_min = layer_min_edge;
    num_on_edges.layer_max = layer_max_edge;

    // Update average CHANX and CHANY usage for this net within its bounding box if congestion modeling is enabled
    if (congestion_modeling_started_) {
        auto& [x_chan_util, y_chan_util] = use_ts ? ts_avg_chan_util_new_[net_id] : net_cong_[net_id].avg_chan_util;
        const int total_channels = (coords.xmax - coords.xmin + 1) * (coords.ymax - coords.ymin + 1);
        x_chan_util = acc_chan_util_.x.get_sum(coords.xmin, coords.ymin, coords.xmax, coords.ymax) / total_channels;
        y_chan_util = acc_chan_util_.y.get_sum(coords.xmin, coords.ymin, coords.xmax, coords.ymax) / total_channels;
    }
}

double NetCostHandler::get_net_bb_cost_(ClusterNetId net_id, const t_bb& bb) const {
    // Finds the cost due to one net by looking at its coordinate bounding box.
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();

    const double crossing = wirelength_crossing_count(cluster_ctx.clb_nlist.net_pins(net_id).size());

    /* Could insert a check for xmin == xmax.  In that case, assume  *
     * connection will be made with no bends and hence no x-cost.    *
     * Same thing for y-cost.                                        */

    /* Cost = wire length along channel * cross_count / average      *
     * channel capacity.   Do this for x, then y direction and add.  */

    /* For average channel width factor, I'll always include the channel immediately
     * below and the channel immediately to the left of the bounding box, so both bb.ymin
     * and bb.xmin are subtracted by 1 before being used as indices of chan?_place_cost_fac_.
     * chan?_place_cost_fac_ objects can handle -1 indices internally.
     */

    double ncost;
    const auto [chanx_cost_fac, chany_cost_fac] = get_chanxy_cost_fac_(bb);
    ncost = (bb.xmax - bb.xmin + 1) * chanx_cost_fac;
    ncost += (bb.ymax - bb.ymin + 1) * chany_cost_fac;
    if (is_multi_layer_) {
        ncost += (bb.layer_max - bb.layer_min) * get_chanz_cost_factor_(bb);
    }

    ncost *= crossing;

    return ncost;
}

double NetCostHandler::get_net_cong_cost_(const std::pair<float, float>& avg_chan_util) const {
    VTR_ASSERT_SAFE(congestion_modeling_started_);
    const auto [x_chan_util, y_chan_util] = avg_chan_util;

    const float threshold = congestion_chan_util_threshold_;

    float x_chan_cong = (x_chan_util < threshold) ? 0.0f : x_chan_util - threshold;
    float y_chan_cong = (y_chan_util < threshold) ? 0.0f : y_chan_util - threshold;

    return x_chan_cong + y_chan_cong;
}

double NetCostHandler::get_net_wirelength_estimate_(ClusterNetId net_id) const {
    const t_bb& bb = net_bb_[net_id].coords;
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();

    double crossing = wirelength_crossing_count(cluster_ctx.clb_nlist.net_pins(net_id).size());

    /* Could insert a check for xmin == xmax.  In that case, assume  *
     * connection will be made with no bends and hence no x-cost.    *
     * Same thing for y-cost.                                        */

    /* Cost = wire length along channel * cross_count / average      *
     * channel capacity.   Do this for x, then y direction and add.  */

    double ncost;
    ncost = (bb.xmax - bb.xmin + 1) * crossing;
    ncost += (bb.ymax - bb.ymin + 1) * crossing;

    return ncost;
}

std::pair<double, double> NetCostHandler::get_chanxy_cost_fac_(const t_bb& bb) const {
    const int total_chanx_width = acc_chan_width_.x.get_sum(bb.ymin, bb.ymax);
    const double inverse_average_chanx_width = (bb.ymax - bb.ymin + 1.0) / total_chanx_width;

    const int total_chany_width = acc_chan_width_.y.get_sum(bb.xmin, bb.xmax);
    const double inverse_average_chany_width = (bb.xmax - bb.xmin + 1.0) / total_chany_width;

    return {inverse_average_chanx_width, inverse_average_chany_width};
}

float NetCostHandler::get_chanz_cost_factor_(const t_bb& bb) const {
    int num_inter_dir_conn = acc_tile_num_inter_die_conn_.get_sum(bb.xmin,
                                                                  bb.ymin,
                                                                  bb.xmax,
                                                                  bb.ymax);

    float z_cost_factor;
    if (num_inter_dir_conn == 0) {
        return 1.0f;
    } else {
        int bb_num_tiles = (bb.xmax - bb.xmin + 1) * (bb.ymax - bb.ymin + 1);
        z_cost_factor = bb_num_tiles / static_cast<float>(num_inter_dir_conn);
    }

    return z_cost_factor;
}

t_net_cost_terms NetCostHandler::recompute_bb_cong_cost_() {
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();

    t_net_cost_terms cost_terms;

    for (ClusterNetId net_id : cluster_ctx.clb_nlist.non_ignored_nets()) {
        // Bounding boxes don't have to be recomputed; they're correct.
        cost_terms.bb_cost += net_bb_[net_id].cost;

        if (congestion_modeling_started_) {
            cost_terms.cong_cost += net_cong_[net_id].cost;
        }
    }

    return cost_terms;
}

double wirelength_crossing_count(size_t fanout) {
    // Get the expected "crossing count" of a net, based on its number of pins.
    // Extrapolate for very large nets.
    if (fanout > MAX_FANOUT_CROSSING_COUNT) {
        return 2.7933 + 0.02616 * (fanout - MAX_FANOUT_CROSSING_COUNT);
    } else {
        return cross_count[fanout - 1];
    }
}

void NetCostHandler::set_bb_delta_cost_(t_net_cost_terms& cost_terms_delta) {
    for (const ClusterNetId ts_net : ts_nets_to_update_) {
        ClusterNetId net_id = ts_net;

        proposed_net_cost_[net_id] = get_net_bb_cost_(net_id, ts_bb_coord_new_[net_id]);
        cost_terms_delta.bb_cost += proposed_net_cost_[net_id] - net_bb_[net_id].cost;

        if (congestion_modeling_started_) {
            proposed_net_cong_cost_[net_id] = get_net_cong_cost_(ts_avg_chan_util_new_[net_id]);
            cost_terms_delta.cong_cost += proposed_net_cong_cost_[net_id] - net_cong_[net_id].cost;
        }
    }
}

void NetCostHandler::find_affected_nets_and_update_costs(const PlaceDelayModel* delay_model,
                                                         const PlacerCriticalities* criticalities,
                                                         t_pl_blocks_to_be_moved& blocks_affected,
                                                         t_net_cost_terms& cost_terms_delta,
                                                         double& timing_delta_c) {
    VTR_ASSERT_DEBUG(cost_terms_delta.bb_cost == 0.);
    VTR_ASSERT_DEBUG(cost_terms_delta.cong_cost == 0.);
    VTR_ASSERT_DEBUG(cost_terms_delta.interposer_cost == 0.);
    VTR_ASSERT_DEBUG(timing_delta_c == 0.);
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    ts_nets_to_update_.resize(0);

    // Go through all the blocks moved.
    for (const t_pl_moved_block& moving_block : blocks_affected.moved_blocks) {
        std::vector<ClusterPinId>& affected_pins = blocks_affected.affected_pins;
        ClusterBlockId blk_id = moving_block.block_num;

        // Go through all the pins in the moved block.
        for (ClusterPinId blk_pin : clb_nlist.block_pins(blk_id)) {
            bool is_src_moving = false;
            if (clb_nlist.pin_type(blk_pin) == PinType::SINK) {
                ClusterNetId net_id = clb_nlist.pin_net(blk_pin);
                is_src_moving = blocks_affected.driven_by_moved_block(net_id);
            }
            update_net_info_on_pin_move_(delay_model,
                                         criticalities,
                                         blk_pin,
                                         moving_block,
                                         affected_pins,
                                         timing_delta_c,
                                         is_src_moving);
        }
    }

    // Now update the bounding box costs (since the net bounding
    // boxes are up-to-date). The cost is only updated once per net.
    set_bb_delta_cost_(cost_terms_delta);
}

void NetCostHandler::update_move_nets() {
    // update net cost functions and reset flags.
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();

    for (const ClusterNetId ts_net : ts_nets_to_update_) {
        ClusterNetId net_id = ts_net;

        set_ts_bb_coord_(net_id);

        if (cluster_ctx.clb_nlist.net_sinks(net_id).size() >= SMALL_NET) {
            set_ts_edge_(net_id);
        }

        net_bb_[net_id].cost = proposed_net_cost_[net_id];
        // negative proposed_net_cost value is acting as a flag to mean not computed yet.
        proposed_net_cost_[net_id] = -1;

        if (congestion_modeling_started_) {
            net_cong_[net_id].cost = proposed_net_cong_cost_[net_id];
            proposed_net_cong_cost_[net_id] = -1;
        }

        bb_update_status_[net_id] = NetUpdateState::NOT_UPDATED_YET;
    }
}

void NetCostHandler::reset_move_nets() {
    // Reset the net cost function flags first.
    for (const ClusterNetId net_id : ts_nets_to_update_) {
        proposed_net_cost_[net_id] = -1;

        if (congestion_modeling_started_) {
            proposed_net_cong_cost_[net_id] = -1;
        }

        bb_update_status_[net_id] = NetUpdateState::NOT_UPDATED_YET;
    }
}

void NetCostHandler::recompute_costs_from_scratch(const PlaceDelayModel* delay_model,
                                                  const PlacerCriticalities* criticalities,
                                                  t_placer_costs& costs) {
    auto check_and_print_cost = [](double new_cost,
                                   double old_cost,
                                   const std::string& cost_name) -> void {
        if (!vtr::isclose(new_cost, old_cost, PL_INCREMENTAL_COST_TOLERANCE, 0.)) {
            std::string msg = vtr::string_fmt(
                "in recompute_costs_from_scratch: new_%s = %g, old %s = %g, ERROR_TOL = %g\n",
                cost_name.c_str(), new_cost, cost_name.c_str(), old_cost, PL_INCREMENTAL_COST_TOLERANCE);
            VPR_ERROR(VPR_ERROR_PLACE, msg.c_str());
        }
    };

    t_net_cost_terms new_cost_terms = recompute_bb_cong_cost_();
    check_and_print_cost(new_cost_terms.bb_cost, costs.bb_cost, "bb_cost");
    costs.bb_cost = new_cost_terms.bb_cost;

    // Ignore tiny congestion costs due to floating-point round-off.
    constexpr double MIN_EXPECTED_CONG_COST = 1.e-6;
    if (congestion_modeling_started_ && new_cost_terms.cong_cost > MIN_EXPECTED_CONG_COST) {
        check_and_print_cost(new_cost_terms.cong_cost, costs.congestion_cost, "cong_cost");
        costs.congestion_cost = new_cost_terms.cong_cost;
    } else {
        costs.congestion_cost = 0.;
    }

    if (place_algorithm_.is_timing_driven()) {
        double new_timing_cost = 0.;
        comp_td_costs(delay_model, *criticalities, placer_state_, &new_timing_cost);
        check_and_print_cost(new_timing_cost, costs.timing_cost, "timing_cost");
        costs.timing_cost = new_timing_cost;
    } else {
        VTR_ASSERT(place_algorithm_ == e_place_algorithm::BOUNDING_BOX_PLACE);
        costs.cost = new_cost_terms.bb_cost * costs.bb_cost_norm;
    }
}

double NetCostHandler::get_total_wirelength_estimate() const {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    double estimated_wirelength = 0.0;
    for (ClusterNetId net_id : clb_nlist.non_ignored_nets()) {
        estimated_wirelength += get_net_wirelength_estimate_(net_id);
    }

    return estimated_wirelength;
}

int NetCostHandler::get_num_nets_spanning_multiple_layers() const {
    if (!is_multi_layer_) {
        return 0;
    }

    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    return static_cast<int>(std::ranges::count_if(clb_nlist.non_ignored_nets(), [this](ClusterNetId net_id) {
        const t_bb& bb = net_bb_[net_id].coords;
        return bb.layer_min != bb.layer_max;
    }));
}

const std::vector<ClusterNetId>& NetCostHandler::affected_nets() const {
    return ts_nets_to_update_;
}

double NetCostHandler::estimate_routing_chan_util(bool compute_congestion_cost /*=true*/) {
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
    const DeviceContext& device_ctx = g_vpr_ctx.device();

    const size_t grid_width = device_ctx.grid.width();
    const size_t grid_height = device_ctx.grid.height();
    const size_t num_layers = device_ctx.grid.get_num_layers();
    const size_t num_nets = g_vpr_ctx.clustering().clb_nlist.nets().size();

    // Congestion-related data members are allocated the first time this method is called
    // to enable congestion modeling. This lazy allocation helps save memory when congestion
    // modeling is not used.
    if (!congestion_modeling_started_) {
        congestion_modeling_started_ = true;

        chan_util_.x = vtr::NdMatrix<double, 3>({{num_layers, grid_width, grid_height}}, 0);
        chan_util_.y = vtr::NdMatrix<double, 3>({{num_layers, grid_width, grid_height}}, 0);

        acc_chan_util_.x = vtr::PrefixSum2D<double>(grid_width,
                                                    grid_height,
                                                    [&](size_t x, size_t y) {
                                                        return chan_util_.x[0][x][y];
                                                    });

        acc_chan_util_.y = vtr::PrefixSum2D<double>(grid_width,
                                                    grid_height,
                                                    [&](size_t x, size_t y) {
                                                        return chan_util_.y[0][x][y];
                                                    });

        ts_avg_chan_util_new_.resize(num_nets, {0., 0.});
        net_cong_.resize(num_nets);
        proposed_net_cong_cost_.resize(num_nets, -1.);
    }

    chan_util_.x.fill(0.);
    chan_util_.y.fill(0.);

    // For each net, this function estimates routing channel utilization by distributing
    // the net's expected wirelength across its bounding box. The expected wirelength
    // for each dimension (x, y) is computed proportionally based on the bounding box size
    // in each direction. The wirelength in each dimension is then **evenly spread** across
    // all grid locations within the bounding box, and the demand is accumulated in
    // the channel utilization matrices.

    for (ClusterNetId net_id : cluster_ctx.clb_nlist.non_ignored_nets()) {
        const t_bb& bb = net_bb_[net_id].coords;
        double expected_wirelength = get_net_wirelength_estimate_(net_id);

        int distance_x = bb.xmax - bb.xmin + 1;
        int distance_y = bb.ymax - bb.ymin + 1;
        int distance_z = bb.layer_max - bb.layer_min + 1;

        double expected_x_wl = (double)distance_x / (distance_x + distance_y) * expected_wirelength;
        double expected_y_wl = expected_wirelength - expected_x_wl;

        int total_channel_segments = distance_x * distance_y * distance_z;
        double expected_per_x_segment_wl = expected_x_wl / total_channel_segments;
        double expected_per_y_segment_wl = expected_y_wl / total_channel_segments;

        for (int layer = bb.layer_min; layer <= bb.layer_max; layer++) {
            for (int x = bb.xmin; x <= bb.xmax; x++) {
                for (int y = bb.ymin; y <= bb.ymax; y++) {
                    chan_util_.x[layer][x][y] += expected_per_x_segment_wl;
                    chan_util_.y[layer][x][y] += expected_per_y_segment_wl;
                }
            }
        }
    }

    const ChannelMetric<vtr::NdMatrix<int, 3>>& chan_width = device_ctx.rr_chan_segment_width;

    VTR_ASSERT(chan_util_.x.size() == chan_util_.y.size());
    VTR_ASSERT(chan_util_.x.size() == chan_width.x.size());
    VTR_ASSERT(chan_util_.y.size() == chan_width.y.size());

    // Normalize channel utilizations by dividing by the corresponding channel widths.
    // If a channel does not exist (i.e., its width is zero), we set its utilization to 1
    // to avoid division by zero.
    for (size_t layer = 0; layer < num_layers; ++layer) {
        for (size_t x = 0; x < grid_width; ++x) {
            for (size_t y = 0; y < grid_height; ++y) {
                if (chan_width.x[layer][x][y] > 0) {
                    chan_util_.x[layer][x][y] /= chan_width.x[layer][x][y];
                } else {
                    VTR_ASSERT_SAFE(chan_width.x[layer][x][y] == 0);
                    chan_util_.x[layer][x][y] = 1.;
                }

                if (chan_width.y[layer][x][y] > 0) {
                    chan_util_.y[layer][x][y] /= chan_width.y[layer][x][y];
                } else {
                    VTR_ASSERT_SAFE(chan_width.y[layer][x][y] == 0);
                    chan_util_.y[layer][x][y] = 1.;
                }
            }
        }
    }

    // For now, congestion modeling in the placement stage is limited to a single die
    // TODO: extend it to multiple dice
    acc_chan_util_.x = vtr::PrefixSum2D<double>(grid_width,
                                                grid_height,
                                                [&](size_t x, size_t y) {
                                                    return chan_util_.x[0][x][y];
                                                });

    acc_chan_util_.y = vtr::PrefixSum2D<double>(grid_width,
                                                grid_height,
                                                [&](size_t x, size_t y) {
                                                    return chan_util_.y[0][x][y];
                                                });

    double cong_cost = 0.;
    // Compute congestion cost using computed bounding boxes and channel utilization map
    if (compute_congestion_cost) {
        for (ClusterNetId net_id : cluster_ctx.clb_nlist.non_ignored_nets()) {
            t_net_cong_info& net_cong = net_cong_[net_id];
            net_cong.cost = get_net_cong_cost_(net_cong.avg_chan_util);
            cong_cost += net_cong.cost;
        }
    }

    return cong_cost;
}

const ChannelMetric<vtr::NdMatrix<double, 3>>& NetCostHandler::get_chan_util() const {
    return chan_util_;
}

void NetCostHandler::set_ts_bb_coord_(const ClusterNetId net_id) {
    net_bb_[net_id].coords = ts_bb_coord_new_[net_id];
    if (congestion_modeling_started_) {
        net_cong_[net_id].avg_chan_util = ts_avg_chan_util_new_[net_id];
    }
}

void NetCostHandler::set_ts_edge_(const ClusterNetId net_id) {
    net_bb_[net_id].num_on_edges = ts_bb_edge_new_[net_id];
}
