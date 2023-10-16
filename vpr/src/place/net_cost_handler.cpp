#include "net_cost_handler.h"
#include "globals.h"
#include "move_utils.h"

/* Flags for the states of the bounding box.                              *
 * Stored as char for memory efficiency.                                  */
#define NOT_UPDATED_YET 'N'
#define UPDATED_ONCE 'U'
#define GOT_FROM_SCRATCH 'S'

/* Cost of a net, and a temporary cost of a net used during move assessment. */
static vtr::vector<ClusterNetId, double> net_cost, proposed_net_cost;

/* [0...cluster_ctx.clb_nlist.nets().size()-1]                                               *
 * A flag array to indicate whether the specific bounding box has been updated   *
 * in this particular swap or not. If it has been updated before, the code       *
 * must use the updated data, instead of the out-of-date data passed into the    *
 * subroutine, particularly used in try_swap(). The value NOT_UPDATED_YET        *
 * indicates that the net has not been updated before, UPDATED_ONCE indicated    *
 * that the net has been updated once, if it is going to be updated again, the   *
 * values from the previous update must be used. GOT_FROM_SCRATCH is only        *
 * applicable for nets larger than SMALL_NETS and it indicates that the          *
 * particular bounding box cannot be updated incrementally before, hence the     *
 * bounding box is got from scratch, so the bounding box would definitely be     *
 * right, DO NOT update again.                                                   */
static vtr::vector<ClusterNetId, char> bb_updated_before;

/* The following arrays are used by the try_swap function for speed.   */
/* [0...cluster_ctx.clb_nlist.nets().size()-1] */
static vtr::vector<ClusterNetId, t_bb> ts_bb_coord_new, ts_bb_edge_new;
static std::vector<ClusterNetId> ts_nets_to_update;


static bool driven_by_moved_block(const AtomNetId net,
                                  const std::vector<t_pl_moved_atom_block>& moved_blocks);

static bool driven_by_moved_block(const ClusterNetId net,
                                  const std::vector<t_pl_moved_block>& moved_blocks);

static void update_net_bb(const ClusterNetId& net,
                          const ClusterBlockId& blk,
                          const ClusterPinId& blk_pin,
                          const t_pl_moved_block& pl_moved_block);

static void update_td_delta_costs(const PlaceDelayModel* delay_model,
                                  const PlacerCriticalities& criticalities,
                                  const ClusterNetId net,
                                  const ClusterPinId pin,
                                  std::vector<ClusterPinId>& affected_pins,
                                  double& delta_timing_cost,
                                  bool is_src_moving);

static void record_affected_net(const ClusterNetId net, int& num_affected_nets);

static void update_net_info_on_pin_move(const t_place_algorithm& place_algorithm,
                                        const PlaceDelayModel* delay_model,
                                        const PlacerCriticalities* criticalities,
                                        const ClusterBlockId& blk_id,
                                        const ClusterPinId& pin_id,
                                        const t_pl_moved_block& moving_blk_inf,
                                        std::vector<ClusterPinId>& affected_pins,
                                        double& timing_delta_c,
                                        int& num_affected_nets,
                                        bool is_src_moving);



//Returns true if 'net' is driven by one of the blocks in 'blocks_affected'
static bool driven_by_moved_block(const AtomNetId net,
                                  const std::vector<t_pl_moved_atom_block>& moved_blocks) {
    const auto& atom_nlist = g_vpr_ctx.atom().nlist;
    bool is_driven_by_move_blk;
    AtomBlockId net_driver_block = atom_nlist.net_driver_block(
        net);

    is_driven_by_move_blk = std::any_of(moved_blocks.begin(), moved_blocks.end(), [&net_driver_block](const auto& move_blk) {
        return net_driver_block == move_blk.block_num;
    });

    return is_driven_by_move_blk;
}

//Returns true if 'net' is driven by one of the blocks in 'blocks_affected'
static bool driven_by_moved_block(const ClusterNetId net,
                                  const std::vector<t_pl_moved_block>& moved_blocks) {
    auto& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    bool is_driven_by_move_blk;
    ClusterBlockId net_driver_block = clb_nlist.net_driver_block(
        net);

    is_driven_by_move_blk = std::any_of(moved_blocks.begin(), moved_blocks.end(), [&net_driver_block](const auto& move_blk) {
        return net_driver_block == move_blk.block_num;
    });

    return is_driven_by_move_blk;
}

/**
 * @brief Update the net bounding boxes.
 *
 * Do not update the net cost here since it should only
 * be updated once per net, not once per pin.
 */
static void update_net_bb(const ClusterNetId& net,
                          const ClusterBlockId& blk,
                          const ClusterPinId& blk_pin,
                          const t_pl_moved_block& pl_moved_block) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    if (cluster_ctx.clb_nlist.net_sinks(net).size() < SMALL_NET) {
        //For small nets brute-force bounding box update is faster

        if (bb_updated_before[net] == NOT_UPDATED_YET) { //Only once per-net
            get_non_updateable_bb(net, &ts_bb_coord_new[net]);
        }
    } else {
        //For large nets, update bounding box incrementally
        int iblk_pin = tile_pin_index(blk_pin);

        t_physical_tile_type_ptr blk_type = physical_tile_type(blk);
        int pin_width_offset = blk_type->pin_width_offset[iblk_pin];
        int pin_height_offset = blk_type->pin_height_offset[iblk_pin];

        //Incremental bounding box update
        update_bb(net, &ts_bb_coord_new[net], &ts_bb_edge_new[net],
                  pl_moved_block.old_loc.x + pin_width_offset,
                  pl_moved_block.old_loc.y
                      + pin_height_offset,
                  pl_moved_block.new_loc.x + pin_width_offset,
                  pl_moved_block.new_loc.y
                      + pin_height_offset);
    }
}

/**
 * @brief Calculate the new connection delay and timing cost of all the
 *        sink pins affected by moving a specific pin to a new location.
 *        Also calculates the total change in the timing cost.
 *
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
static void update_td_delta_costs(const PlaceDelayModel* delay_model,
                                  const PlacerCriticalities& criticalities,
                                  const ClusterNetId net,
                                  const ClusterPinId pin,
                                  std::vector<ClusterPinId>& affected_pins,
                                  double& delta_timing_cost,
                                  bool is_src_moving) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    const auto& connection_delay = g_placer_ctx.timing().connection_delay;
    auto& connection_timing_cost = g_placer_ctx.mutable_timing().connection_timing_cost;
    auto& proposed_connection_delay = g_placer_ctx.mutable_timing().proposed_connection_delay;
    auto& proposed_connection_timing_cost = g_placer_ctx.mutable_timing().proposed_connection_timing_cost;

    if (cluster_ctx.clb_nlist.pin_type(pin) == PinType::DRIVER) {
        /* This pin is a net driver on a moved block. */
        /* Recompute all point to point connection delays for the net sinks. */
        for (size_t ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net).size();
             ipin++) {
            float temp_delay = comp_td_single_connection_delay(delay_model, net,
                                                               ipin);
            /* If the delay hasn't changed, do not mark this pin as affected */
            if (temp_delay == connection_delay[net][ipin]) {
                continue;
            }

            /* Calculate proposed delay and cost values */
            proposed_connection_delay[net][ipin] = temp_delay;

            proposed_connection_timing_cost[net][ipin] = criticalities.criticality(net, ipin) * temp_delay;
            delta_timing_cost += proposed_connection_timing_cost[net][ipin]
                                 - connection_timing_cost[net][ipin];

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

            float temp_delay = comp_td_single_connection_delay(delay_model, net,
                                                               ipin);
            /* If the delay hasn't changed, do not mark this pin as affected */
            if (temp_delay == connection_delay[net][ipin]) {
                return;
            }

            /* Calculate proposed delay and cost values */
            proposed_connection_delay[net][ipin] = temp_delay;

            proposed_connection_timing_cost[net][ipin] = criticalities.criticality(net, ipin) * temp_delay;
            delta_timing_cost += proposed_connection_timing_cost[net][ipin]
                                 - connection_timing_cost[net][ipin];

            /* Record this connection in blocks_affected.affected_pins */
            affected_pins.push_back(pin);
        }
    }
}

///@brief Record effected nets.
static void record_affected_net(const ClusterNetId net,
                                int& num_affected_nets) {
    /* Record effected nets. */
    if (proposed_net_cost[net] < 0.) {
        /* Net not marked yet. */
        ts_nets_to_update[num_affected_nets] = net;
        num_affected_nets++;

        /* Flag to say we've marked this net. */
        proposed_net_cost[net] = 1.;
    }
}

static void update_net_info_on_pin_move(const t_place_algorithm& place_algorithm,
                                        const PlaceDelayModel* delay_model,
                                        const PlacerCriticalities* criticalities,
                                        const ClusterBlockId& blk_id,
                                        const ClusterPinId& pin_id,
                                        const t_pl_moved_block& moving_blk_inf,
                                        std::vector<ClusterPinId>& affected_pins,
                                        double& timing_delta_c,
                                        int& num_affected_nets,
                                        bool is_src_moving) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const ClusterNetId net_id = cluster_ctx.clb_nlist.pin_net(pin_id);
    VTR_ASSERT_SAFE_MSG(net_id,
                        "Only valid nets should be found in compressed netlist block pins");

    if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) {
        //TODO: Do we require anyting special here for global nets?
        //"Global nets are assumed to span the whole chip, and do not effect costs."
        return;
    }

    /* Record effected nets */
    record_affected_net(net_id, num_affected_nets);

    /* Update the net bounding boxes. */
    update_net_bb(net_id, blk_id, pin_id, moving_blk_inf);

    if (place_algorithm.is_timing_driven()) {
        /* Determine the change in connection delay and timing cost. */
        update_td_delta_costs(delay_model,
                              *criticalities,
                              net_id,
                              pin_id,
                              affected_pins,
                              timing_delta_c,
                              is_src_moving);
    }
}

int find_affected_nets_and_update_costs(
    const t_place_algorithm& place_algorithm,
    const PlaceDelayModel* delay_model,
    const PlacerCriticalities* criticalities,
    t_pl_atom_blocks_to_be_moved& blocks_affected,
    double& bb_delta_c,
    double& timing_delta_c) {

    const auto& atom_look_up = g_vpr_ctx.atom().lookup;
    const auto& atom_nlist = g_vpr_ctx.atom().nlist;
    const auto& clb_nlsit = g_vpr_ctx.clustering().clb_nlist;

    VTR_ASSERT_SAFE(bb_delta_c == 0.);
    VTR_ASSERT_SAFE(timing_delta_c == 0.);

    int num_affected_nets = 0;

    std::vector<ClusterPinId> affected_pins;

    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; iblk++) {
        AtomBlockId atom_blk_id = blocks_affected.moved_blocks[iblk].block_num;
        ClusterBlockId cluster_blk_id = atom_look_up.atom_clb(atom_blk_id);
        const auto& atom_old_loc = blocks_affected.moved_blocks[iblk].old_loc;
        const auto& atom_new_loc = blocks_affected.moved_blocks[iblk].new_loc;

        for (const AtomPinId& atom_pin: atom_nlist.block_pins(atom_blk_id)) {
            auto cluster_pins = cluster_pins_connected_to_atom_pin(atom_pin);
            for (const auto& cluster_pin : cluster_pins) {
                bool is_src_moving = false;
                if (atom_nlist.pin_type(atom_pin) == PinType::SINK) {
                    AtomNetId net_id = atom_nlist.pin_net(atom_pin);
                    is_src_moving = driven_by_moved_block(net_id, blocks_affected.moved_blocks);
                }
                t_pl_moved_block move_cluster_inf;
                move_cluster_inf.block_num = cluster_blk_id;
                move_cluster_inf.old_loc = t_pl_loc(atom_old_loc.x, atom_old_loc.y, atom_old_loc.sub_tile, atom_old_loc.layer);
                move_cluster_inf.new_loc = t_pl_loc(atom_new_loc.x, atom_new_loc.y, atom_new_loc.sub_tile, atom_new_loc.layer);
                update_net_info_on_pin_move(place_algorithm,
                                            delay_model,
                                            criticalities,
                                            cluster_blk_id,
                                            cluster_pin,
                                            move_cluster_inf,
                                            affected_pins,
                                            timing_delta_c,
                                            num_affected_nets,
                                            is_src_moving);


            }
        }
    }

    /* Now update the bounding box costs (since the net bounding     *
     * boxes are up-to-date). The cost is only updated once per net. */
    for (int inet_affected = 0; inet_affected < num_affected_nets;
         inet_affected++) {
        ClusterNetId net_id = ts_nets_to_update[inet_affected];

        proposed_net_cost[net_id] = get_net_cost(net_id,
                                                 &ts_bb_coord_new[net_id]);
        bb_delta_c += proposed_net_cost[net_id] - net_cost[net_id];
    }

    return num_affected_nets;
}

/**
 * @brief Find all the nets and pins affected by this swap and update costs.
 *
 * Find all the nets affected by this swap and update the bounding box (wiring)
 * costs. This cost function doesn't depend on the timing info.
 *
 * Find all the connections affected by this swap and update the timing cost.
 * For a connection to be affected, it not only needs to be on or driven by
 * a block, but it also needs to have its delay changed. Otherwise, it will
 * not be added to the affected_pins structure.
 *
 * For more, see update_td_delta_costs().
 *
 * The timing costs are calculated by getting the new connection delays,
 * multiplied by the connection criticalities returned by the timing
 * analyzer. These timing costs are stored in the proposed_* data structures.
 *
 * The change in the bounding box cost is stored in `bb_delta_c`.
 * The change in the timing cost is stored in `timing_delta_c`.
 *
 * @return The number of affected nets.
 */
int find_affected_nets_and_update_costs(
    const t_place_algorithm& place_algorithm,
    const PlaceDelayModel* delay_model,
    const PlacerCriticalities* criticalities,
    t_pl_blocks_to_be_moved& blocks_affected,
    double& bb_delta_c,
    double& timing_delta_c) {
    VTR_ASSERT_SAFE(bb_delta_c == 0.);
    VTR_ASSERT_SAFE(timing_delta_c == 0.);
    auto& clb_nlsit = g_vpr_ctx.clustering().clb_nlist;

    int num_affected_nets = 0;

    /* Go through all the blocks moved. */
    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; iblk++) {
        const auto& moving_block_inf = blocks_affected.moved_blocks[iblk];
        auto& affected_pins = blocks_affected.affected_pins;
        ClusterBlockId blk = blocks_affected.moved_blocks[iblk].block_num;

        /* Go through all the pins in the moved block. */
        for (ClusterPinId blk_pin : clb_nlsit.block_pins(blk)) {
            bool is_src_moving = false;
            if (clb_nlsit.pin_type(blk_pin) == PinType::SINK) {
                ClusterNetId net_id = clb_nlsit.pin_net(blk_pin);
                is_src_moving = driven_by_moved_block(net_id, blocks_affected.moved_blocks);
            }
            update_net_info_on_pin_move(place_algorithm,
                                        delay_model,
                                        criticalities,
                                        blk,
                                        blk_pin,
                                        moving_block_inf,
                                        affected_pins,
                                        timing_delta_c,
                                        num_affected_nets,
                                        is_src_moving);
        }
    }

    /* Now update the bounding box costs (since the net bounding     *
     * boxes are up-to-date). The cost is only updated once per net. */
    for (int inet_affected = 0; inet_affected < num_affected_nets;
         inet_affected++) {
        ClusterNetId net_id = ts_nets_to_update[inet_affected];

        proposed_net_cost[net_id] = get_net_cost(net_id,
                                                 &ts_bb_coord_new[net_id]);
        bb_delta_c += proposed_net_cost[net_id] - net_cost[net_id];
    }

    return num_affected_nets;
}
