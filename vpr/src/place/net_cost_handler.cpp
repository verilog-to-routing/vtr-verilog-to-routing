#include "net_cost_handler.h"
#include "globals.h"
#include "placer_globals.h"
#include "move_utils.h"
#include "place_timing_update.h"
#include "noc_place_utils.h"

using std::max;
using std::min;

/* Flags for the states of the bounding box.                              *
 * Stored as char for memory efficiency.                                  */
#define NOT_UPDATED_YET 'N'
#define UPDATED_ONCE 'U'
#define GOT_FROM_SCRATCH 'S'

/* This defines the error tolerance for floating points variables used in *
 * cost computation. 0.01 means that there is a 1% error tolerance.       */
#define ERROR_TOL .01

/* Expected crossing counts for nets with different #'s of pins.  From *
 * ICCAD 94 pp. 690 - 695 (with linear interpolation applied by me).   *
 * Multiplied to bounding box of a net to better estimate wire length  *
 * for higher fanout nets. Each entry is the correction factor for the *
 * fanout index-1                                                      */
static const float cross_count[50] = {/* [0..49] */ 1.0, 1.0, 1.0, 1.0828,
                                      1.1536, 1.2206, 1.2823, 1.3385, 1.3991, 1.4493, 1.4974, 1.5455, 1.5937,
                                      1.6418, 1.6899, 1.7304, 1.7709, 1.8114, 1.8519, 1.8924, 1.9288, 1.9652,
                                      2.0015, 2.0379, 2.0743, 2.1061, 2.1379, 2.1698, 2.2016, 2.2334, 2.2646,
                                      2.2958, 2.3271, 2.3583, 2.3895, 2.4187, 2.4479, 2.4772, 2.5064, 2.5356,
                                      2.5610, 2.5864, 2.6117, 2.6371, 2.6625, 2.6887, 2.7148, 2.7410, 2.7671,
                                      2.7933};

/* The arrays below are used to precompute the inverse of the average   *
 * number of tracks per channel between [subhigh] and [sublow].  Access *
 * them as chan?_place_cost_fac[subhigh][sublow].  They are used to     *
 * speed up the computation of the cost function that takes the length  *
 * of the net bounding box in each dimension, divided by the average    *
 * number of tracks in that direction; for other cost functions they    *
 * will never be used.                                                  *
 */
static vtr::NdMatrix<float, 2> chanx_place_cost_fac({0, 0}); //[0...device_ctx.grid.width()-2]
static vtr::NdMatrix<float, 2> chany_place_cost_fac({0, 0}); //[0...device_ctx.grid.height()-2]

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

static void get_non_updateable_bb(ClusterNetId net_id, t_bb* bb_coord_new);

static void update_bb(ClusterNetId net_id, t_bb* bb_coord_new, t_bb* bb_edge_new, int xold, int yold, int xnew, int ynew);

static void get_bb_from_scratch(ClusterNetId net_id, t_bb* coords, t_bb* num_on_edges);

static double get_net_wirelength_estimate(ClusterNetId net_id, t_bb* bbptr);

static double recompute_bb_cost();

static double wirelength_crossing_count(size_t fanout);

static double get_net_bounding_box_cost(ClusterNetId net_id, t_bb* bbptr);



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

/* Finds the bounding box of a net and stores its coordinates in the  *
 * bb_coord_new data structure.  This routine should only be called   *
 * for small nets, since it does not determine enough information for *
 * the bounding box to be updated incrementally later.                *
 * Currently assumes channels on both sides of the CLBs forming the   *
 * edges of the bounding box can be used.  Essentially, I am assuming *
 * the pins always lie on the outside of the bounding box.            */
static void get_non_updateable_bb(ClusterNetId net_id, t_bb* bb_coord_new) {
    //TODO: account for multiple physical pin instances per logical pin

    int xmax, ymax, xmin, ymin, x, y;
    int pnum;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();

    ClusterBlockId bnum = cluster_ctx.clb_nlist.net_driver_block(net_id);
    pnum = net_pin_to_tile_pin_index(net_id, 0);

    x = place_ctx.block_locs[bnum].loc.x
        + physical_tile_type(bnum)->pin_width_offset[pnum];
    y = place_ctx.block_locs[bnum].loc.y
        + physical_tile_type(bnum)->pin_height_offset[pnum];

    xmin = x;
    ymin = y;
    xmax = x;
    ymax = y;

    for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
        bnum = cluster_ctx.clb_nlist.pin_block(pin_id);
        pnum = tile_pin_index(pin_id);
        x = place_ctx.block_locs[bnum].loc.x
            + physical_tile_type(bnum)->pin_width_offset[pnum];
        y = place_ctx.block_locs[bnum].loc.y
            + physical_tile_type(bnum)->pin_height_offset[pnum];

        if (x < xmin) {
            xmin = x;
        } else if (x > xmax) {
            xmax = x;
        }

        if (y < ymin) {
            ymin = y;
        } else if (y > ymax) {
            ymax = y;
        }
    }

    /* Now I've found the coordinates of the bounding box.  There are no *
     * channels beyond device_ctx.grid.width()-2 and                     *
     * device_ctx.grid.height() - 2, so I want to clip to that.  As well,*
     * since I'll always include the channel immediately below and the   *
     * channel immediately to the left of the bounding box, I want to    *
     * clip to 1 in both directions as well (since minimum channel index *
     * is 0).  See route_common.cpp for a channel diagram.               */

    bb_coord_new->xmin = max(min<int>(xmin, device_ctx.grid.width() - 2), 1);  //-2 for no perim channels
    bb_coord_new->ymin = max(min<int>(ymin, device_ctx.grid.height() - 2), 1); //-2 for no perim channels
    bb_coord_new->xmax = max(min<int>(xmax, device_ctx.grid.width() - 2), 1);  //-2 for no perim channels
    bb_coord_new->ymax = max(min<int>(ymax, device_ctx.grid.height() - 2), 1); //-2 for no perim channels
}

static void update_bb(ClusterNetId net_id, t_bb* bb_coord_new, t_bb* bb_edge_new, int xold, int yold, int xnew, int ynew) {
    /* Updates the bounding box of a net by storing its coordinates in    *
     * the bb_coord_new data structure and the number of blocks on each   *
     * edge in the bb_edge_new data structure.  This routine should only  *
     * be called for large nets, since it has some overhead relative to   *
     * just doing a brute force bounding box calculation.  The bounding   *
     * box coordinate and edge information for inet must be valid before  *
     * this routine is called.                                            *
     * Currently assumes channels on both sides of the CLBs forming the   *
     * edges of the bounding box can be used.  Essentially, I am assuming *
     * the pins always lie on the outside of the bounding box.            *
     * The x and y coordinates are the pin's x and y coordinates.         */
    /* IO blocks are considered to be one cell in for simplicity.         */
    //TODO: account for multiple physical pin instances per logical pin
    const t_bb *curr_bb_edge, *curr_bb_coord;

    auto& device_ctx = g_vpr_ctx.device();
    auto& place_move_ctx = g_placer_ctx.move();

    xnew = max(min<int>(xnew, device_ctx.grid.width() - 2), 1);  //-2 for no perim channels
    ynew = max(min<int>(ynew, device_ctx.grid.height() - 2), 1); //-2 for no perim channels
    xold = max(min<int>(xold, device_ctx.grid.width() - 2), 1);  //-2 for no perim channels
    yold = max(min<int>(yold, device_ctx.grid.height() - 2), 1); //-2 for no perim channels

    /* Check if the net had been updated before. */
    if (bb_updated_before[net_id] == GOT_FROM_SCRATCH) {
        /* The net had been updated from scratch, DO NOT update again! */
        return;
    } else if (bb_updated_before[net_id] == NOT_UPDATED_YET) {
        /* The net had NOT been updated before, could use the old values */
        curr_bb_coord = &place_move_ctx.bb_coords[net_id];
        curr_bb_edge = &place_move_ctx.bb_num_on_edges[net_id];
        bb_updated_before[net_id] = UPDATED_ONCE;
    } else {
        /* The net had been updated before, must use the new values */
        curr_bb_coord = bb_coord_new;
        curr_bb_edge = bb_edge_new;
    }

    /* Check if I can update the bounding box incrementally. */

    if (xnew < xold) { /* Move to left. */

        /* Update the xmax fields for coordinates and number of edges first. */

        if (xold == curr_bb_coord->xmax) { /* Old position at xmax. */
            if (curr_bb_edge->xmax == 1) {
                get_bb_from_scratch(net_id, bb_coord_new, bb_edge_new);
                bb_updated_before[net_id] = GOT_FROM_SCRATCH;
                return;
            } else {
                bb_edge_new->xmax = curr_bb_edge->xmax - 1;
                bb_coord_new->xmax = curr_bb_coord->xmax;
            }
        } else { /* Move to left, old postion was not at xmax. */
            bb_coord_new->xmax = curr_bb_coord->xmax;
            bb_edge_new->xmax = curr_bb_edge->xmax;
        }

        /* Now do the xmin fields for coordinates and number of edges. */

        if (xnew < curr_bb_coord->xmin) { /* Moved past xmin */
            bb_coord_new->xmin = xnew;
            bb_edge_new->xmin = 1;
        } else if (xnew == curr_bb_coord->xmin) { /* Moved to xmin */
            bb_coord_new->xmin = xnew;
            bb_edge_new->xmin = curr_bb_edge->xmin + 1;
        } else { /* Xmin unchanged. */
            bb_coord_new->xmin = curr_bb_coord->xmin;
            bb_edge_new->xmin = curr_bb_edge->xmin;
        }
        /* End of move to left case. */

    } else if (xnew > xold) { /* Move to right. */

        /* Update the xmin fields for coordinates and number of edges first. */

        if (xold == curr_bb_coord->xmin) { /* Old position at xmin. */
            if (curr_bb_edge->xmin == 1) {
                get_bb_from_scratch(net_id, bb_coord_new, bb_edge_new);
                bb_updated_before[net_id] = GOT_FROM_SCRATCH;
                return;
            } else {
                bb_edge_new->xmin = curr_bb_edge->xmin - 1;
                bb_coord_new->xmin = curr_bb_coord->xmin;
            }
        } else { /* Move to right, old position was not at xmin. */
            bb_coord_new->xmin = curr_bb_coord->xmin;
            bb_edge_new->xmin = curr_bb_edge->xmin;
        }

        /* Now do the xmax fields for coordinates and number of edges. */

        if (xnew > curr_bb_coord->xmax) { /* Moved past xmax. */
            bb_coord_new->xmax = xnew;
            bb_edge_new->xmax = 1;
        } else if (xnew == curr_bb_coord->xmax) { /* Moved to xmax */
            bb_coord_new->xmax = xnew;
            bb_edge_new->xmax = curr_bb_edge->xmax + 1;
        } else { /* Xmax unchanged. */
            bb_coord_new->xmax = curr_bb_coord->xmax;
            bb_edge_new->xmax = curr_bb_edge->xmax;
        }
        /* End of move to right case. */

    } else { /* xnew == xold -- no x motion. */
        bb_coord_new->xmin = curr_bb_coord->xmin;
        bb_coord_new->xmax = curr_bb_coord->xmax;
        bb_edge_new->xmin = curr_bb_edge->xmin;
        bb_edge_new->xmax = curr_bb_edge->xmax;
    }

    /* Now account for the y-direction motion. */

    if (ynew < yold) { /* Move down. */

        /* Update the ymax fields for coordinates and number of edges first. */

        if (yold == curr_bb_coord->ymax) { /* Old position at ymax. */
            if (curr_bb_edge->ymax == 1) {
                get_bb_from_scratch(net_id, bb_coord_new, bb_edge_new);
                bb_updated_before[net_id] = GOT_FROM_SCRATCH;
                return;
            } else {
                bb_edge_new->ymax = curr_bb_edge->ymax - 1;
                bb_coord_new->ymax = curr_bb_coord->ymax;
            }
        } else { /* Move down, old postion was not at ymax. */
            bb_coord_new->ymax = curr_bb_coord->ymax;
            bb_edge_new->ymax = curr_bb_edge->ymax;
        }

        /* Now do the ymin fields for coordinates and number of edges. */

        if (ynew < curr_bb_coord->ymin) { /* Moved past ymin */
            bb_coord_new->ymin = ynew;
            bb_edge_new->ymin = 1;
        } else if (ynew == curr_bb_coord->ymin) { /* Moved to ymin */
            bb_coord_new->ymin = ynew;
            bb_edge_new->ymin = curr_bb_edge->ymin + 1;
        } else { /* ymin unchanged. */
            bb_coord_new->ymin = curr_bb_coord->ymin;
            bb_edge_new->ymin = curr_bb_edge->ymin;
        }
        /* End of move down case. */

    } else if (ynew > yold) { /* Moved up. */

        /* Update the ymin fields for coordinates and number of edges first. */

        if (yold == curr_bb_coord->ymin) { /* Old position at ymin. */
            if (curr_bb_edge->ymin == 1) {
                get_bb_from_scratch(net_id, bb_coord_new, bb_edge_new);
                bb_updated_before[net_id] = GOT_FROM_SCRATCH;
                return;
            } else {
                bb_edge_new->ymin = curr_bb_edge->ymin - 1;
                bb_coord_new->ymin = curr_bb_coord->ymin;
            }
        } else { /* Moved up, old position was not at ymin. */
            bb_coord_new->ymin = curr_bb_coord->ymin;
            bb_edge_new->ymin = curr_bb_edge->ymin;
        }

        /* Now do the ymax fields for coordinates and number of edges. */

        if (ynew > curr_bb_coord->ymax) { /* Moved past ymax. */
            bb_coord_new->ymax = ynew;
            bb_edge_new->ymax = 1;
        } else if (ynew == curr_bb_coord->ymax) { /* Moved to ymax */
            bb_coord_new->ymax = ynew;
            bb_edge_new->ymax = curr_bb_edge->ymax + 1;
        } else { /* ymax unchanged. */
            bb_coord_new->ymax = curr_bb_coord->ymax;
            bb_edge_new->ymax = curr_bb_edge->ymax;
        }
        /* End of move up case. */

    } else { /* ynew == yold -- no y motion. */
        bb_coord_new->ymin = curr_bb_coord->ymin;
        bb_coord_new->ymax = curr_bb_coord->ymax;
        bb_edge_new->ymin = curr_bb_edge->ymin;
        bb_edge_new->ymax = curr_bb_edge->ymax;
    }

    if (bb_updated_before[net_id] == NOT_UPDATED_YET) {
        bb_updated_before[net_id] = UPDATED_ONCE;
    }
}

/* This routine finds the bounding box of each net from scratch (i.e.   *
 * from only the block location information).  It updates both the       *
 * coordinate and number of pins on each edge information.  It           *
 * should only be called when the bounding box information is not valid. */
static void get_bb_from_scratch(ClusterNetId net_id, t_bb* coords, t_bb* num_on_edges) {
    int pnum, x, y, xmin, xmax, ymin, ymax;
    int xmin_edge, xmax_edge, ymin_edge, ymax_edge;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

    ClusterBlockId bnum = cluster_ctx.clb_nlist.net_driver_block(net_id);
    pnum = net_pin_to_tile_pin_index(net_id, 0);
    VTR_ASSERT(pnum >= 0);
    x = place_ctx.block_locs[bnum].loc.x
        + physical_tile_type(bnum)->pin_width_offset[pnum];
    y = place_ctx.block_locs[bnum].loc.y
        + physical_tile_type(bnum)->pin_height_offset[pnum];

    x = max(min<int>(x, grid.width() - 2), 1);
    y = max(min<int>(y, grid.height() - 2), 1);

    xmin = x;
    ymin = y;
    xmax = x;
    ymax = y;
    xmin_edge = 1;
    ymin_edge = 1;
    xmax_edge = 1;
    ymax_edge = 1;

    for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
        bnum = cluster_ctx.clb_nlist.pin_block(pin_id);
        pnum = tile_pin_index(pin_id);
        x = place_ctx.block_locs[bnum].loc.x
            + physical_tile_type(bnum)->pin_width_offset[pnum];
        y = place_ctx.block_locs[bnum].loc.y
            + physical_tile_type(bnum)->pin_height_offset[pnum];

        /* Code below counts IO blocks as being within the 1..grid.width()-2, 1..grid.height()-2 clb array. *
         * This is because channels do not go out of the 0..grid.width()-2, 0..grid.height()-2 range, and   *
         * I always take all channels impinging on the bounding box to be within   *
         * that bounding box.  Hence, this "movement" of IO blocks does not affect *
         * the which channels are included within the bounding box, and it         *
         * simplifies the code a lot.                                              */

        x = max(min<int>(x, grid.width() - 2), 1);  //-2 for no perim channels
        y = max(min<int>(y, grid.height() - 2), 1); //-2 for no perim channels

        if (x == xmin) {
            xmin_edge++;
        }
        if (x == xmax) { /* Recall that xmin could equal xmax -- don't use else */
            xmax_edge++;
        } else if (x < xmin) {
            xmin = x;
            xmin_edge = 1;
        } else if (x > xmax) {
            xmax = x;
            xmax_edge = 1;
        }

        if (y == ymin) {
            ymin_edge++;
        }
        if (y == ymax) {
            ymax_edge++;
        } else if (y < ymin) {
            ymin = y;
            ymin_edge = 1;
        } else if (y > ymax) {
            ymax = y;
            ymax_edge = 1;
        }
    }

    /* Copy the coordinates and number on edges information into the proper   *
     * structures.                                                            */
    coords->xmin = xmin;
    coords->xmax = xmax;
    coords->ymin = ymin;
    coords->ymax = ymax;

    num_on_edges->xmin = xmin_edge;
    num_on_edges->xmax = xmax_edge;
    num_on_edges->ymin = ymin_edge;
    num_on_edges->ymax = ymax_edge;
}

static double get_net_wirelength_estimate(ClusterNetId net_id, t_bb* bbptr) {
    /* WMF: Finds the estimate of wirelength due to one net by looking at   *
     * its coordinate bounding box.                                         */

    double ncost, crossing;
    auto& cluster_ctx = g_vpr_ctx.clustering();

    crossing = wirelength_crossing_count(
        cluster_ctx.clb_nlist.net_pins(net_id).size());

    /* Could insert a check for xmin == xmax.  In that case, assume  *
     * connection will be made with no bends and hence no x-cost.    *
     * Same thing for y-cost.                                        */

    /* Cost = wire length along channel * cross_count / average      *
     * channel capacity.   Do this for x, then y direction and add.  */

    ncost = (bbptr->xmax - bbptr->xmin + 1) * crossing;

    ncost += (bbptr->ymax - bbptr->ymin + 1) * crossing;

    return (ncost);
}

static double recompute_bb_cost() {
    /* Recomputes the cost to eliminate roundoff that may have accrued.  *
     * This routine does as little work as possible to compute this new  *
     * cost.                                                             */

    double cost = 0;

    auto& cluster_ctx = g_vpr_ctx.clustering();

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {       /* for each net ... */
        if (!cluster_ctx.clb_nlist.net_is_ignored(net_id)) { /* Do only if not ignored. */
            /* Bounding boxes don't have to be recomputed; they're correct. */
            cost += net_cost[net_id];
        }
    }

    return (cost);
}

static double wirelength_crossing_count(size_t fanout) {
    /* Get the expected "crossing count" of a net, based on its number *
     * of pins.  Extrapolate for very large nets.                      */

    if (fanout > 50) {
        return 2.7933 + 0.02616 * (fanout - 50);
    } else {
        return cross_count[fanout - 1];
    }
}


static double get_net_bounding_box_cost(ClusterNetId net_id, t_bb* bbptr) {
    /* Finds the cost due to one net by looking at its coordinate bounding  *
     * box.                                                                 */

    double ncost, crossing;
    auto& cluster_ctx = g_vpr_ctx.clustering();

    crossing = wirelength_crossing_count(
        cluster_ctx.clb_nlist.net_pins(net_id).size());

    /* Could insert a check for xmin == xmax.  In that case, assume  *
     * connection will be made with no bends and hence no x-cost.    *
     * Same thing for y-cost.                                        */

    /* Cost = wire length along channel * cross_count / average      *
     * channel capacity.   Do this for x, then y direction and add.  */

    ncost = (bbptr->xmax - bbptr->xmin + 1) * crossing
            * chanx_place_cost_fac[bbptr->ymax][bbptr->ymin - 1];

    ncost += (bbptr->ymax - bbptr->ymin + 1) * crossing
             * chany_place_cost_fac[bbptr->xmax][bbptr->xmin - 1];

    return (ncost);
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

        proposed_net_cost[net_id] = get_net_bounding_box_cost(net_id,
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

        proposed_net_cost[net_id] = get_net_bounding_box_cost(net_id,
                                                 &ts_bb_coord_new[net_id]);
        bb_delta_c += proposed_net_cost[net_id] - net_cost[net_id];
    }

    return num_affected_nets;
}

/* Finds the cost from scratch.  Done only when the placement   *
 * has been radically changed (i.e. after initial placement).   *
 * Otherwise find the cost change incrementally.  If method     *
 * check is NORMAL, we find bounding boxes that are updateable  *
 * for the larger nets.  If method is CHECK, all bounding boxes *
 * are found via the non_updateable_bb routine, to provide a    *
 * cost which can be used to check the correctness of the       *
 * other routine.                                               */
double comp_bb_cost(e_cost_methods method) {
    double cost = 0;
    double expected_wirelength = 0.0;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_move_ctx = g_placer_ctx.mutable_move();

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {       /* for each net ... */
        if (!cluster_ctx.clb_nlist.net_is_ignored(net_id)) { /* Do only if not ignored. */
            /* Small nets don't use incremental updating on their bounding boxes, *
             * so they can use a fast bounding box calculator.                    */
            if (cluster_ctx.clb_nlist.net_sinks(net_id).size() >= SMALL_NET
                && method == NORMAL) {
                get_bb_from_scratch(net_id, &place_move_ctx.bb_coords[net_id],
                                    &place_move_ctx.bb_num_on_edges[net_id]);
            } else {
                get_non_updateable_bb(net_id,
                                      &place_move_ctx.bb_coords[net_id]);
            }

            net_cost[net_id] = get_net_bounding_box_cost(net_id,
                                                         &place_move_ctx.bb_coords[net_id]);
            cost += net_cost[net_id];
            if (method == CHECK)
                expected_wirelength += get_net_wirelength_estimate(net_id,
                                                                   &place_move_ctx.bb_coords[net_id]);
        }
    }

    if (method == CHECK) {
        VTR_LOG("\n");
        VTR_LOG("BB estimate of min-dist (placement) wire length: %.0f\n",
                expected_wirelength);
    }
    return cost;
}

void update_move_nets(int num_nets_affected) {
    /* update net cost functions and reset flags. */
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_move_ctx = g_placer_ctx.mutable_move();

    for (int inet_affected = 0; inet_affected < num_nets_affected;
         inet_affected++) {
        ClusterNetId net_id = ts_nets_to_update[inet_affected];

        place_move_ctx.bb_coords[net_id] = ts_bb_coord_new[net_id];
        if (cluster_ctx.clb_nlist.net_sinks(net_id).size() >= SMALL_NET)
            place_move_ctx.bb_num_on_edges[net_id] = ts_bb_edge_new[net_id];

        net_cost[net_id] = proposed_net_cost[net_id];

        /* negative proposed_net_cost value is acting as a flag. */
        proposed_net_cost[net_id] = -1;
        bb_updated_before[net_id] = NOT_UPDATED_YET;
    }
}

void reset_move_nets(int num_nets_affected) {
    /* Reset the net cost function flags first. */
    for (int inet_affected = 0; inet_affected < num_nets_affected;
         inet_affected++) {
        ClusterNetId net_id = ts_nets_to_update[inet_affected];
        proposed_net_cost[net_id] = -1;
        bb_updated_before[net_id] = NOT_UPDATED_YET;
    }
}

void recompute_costs_from_scratch(const t_placer_opts& placer_opts,
                                  const t_noc_opts& noc_opts,
                                  const PlaceDelayModel* delay_model,
                                  const PlacerCriticalities* criticalities,
                                  t_placer_costs* costs) {
    double new_bb_cost = recompute_bb_cost();
    if (fabs(new_bb_cost - costs->bb_cost) > costs->bb_cost * ERROR_TOL) {
        std::string msg = vtr::string_fmt(
            "in recompute_costs_from_scratch: new_bb_cost = %g, old bb_cost = %g\n",
            new_bb_cost, costs->bb_cost);
        VPR_ERROR(VPR_ERROR_PLACE, msg.c_str());
    }
    costs->bb_cost = new_bb_cost;

    if (placer_opts.place_algorithm.is_timing_driven()) {
        double new_timing_cost = 0.;
        comp_td_costs(delay_model, *criticalities, &new_timing_cost);
        if (fabs(
                new_timing_cost
                - costs->timing_cost)
            > costs->timing_cost * ERROR_TOL) {
            std::string msg = vtr::string_fmt(
                "in recompute_costs_from_scratch: new_timing_cost = %g, old timing_cost = %g, ERROR_TOL = %g\n",
                new_timing_cost, costs->timing_cost, ERROR_TOL);
            VPR_ERROR(VPR_ERROR_PLACE, msg.c_str());
        }
        costs->timing_cost = new_timing_cost;
    } else {
        VTR_ASSERT(placer_opts.place_algorithm == BOUNDING_BOX_PLACE);

        costs->cost = new_bb_cost * costs->bb_cost_norm;
    }

    if (noc_opts.noc) {
        double new_noc_aggregate_bandwidth_cost = 0.;
        double new_noc_latency_cost = 0.;
        recompute_noc_costs(new_noc_aggregate_bandwidth_cost, new_noc_latency_cost);

        if (fabs(
                new_noc_aggregate_bandwidth_cost
                - costs->noc_aggregate_bandwidth_cost)
            > costs->noc_aggregate_bandwidth_cost * ERROR_TOL) {
            std::string msg = vtr::string_fmt(
                "in recompute_costs_from_scratch: new_noc_aggregate_bandwidth_cost = %g, old noc_aggregate_bandwidth_cost = %g, ERROR_TOL = %g\n",
                new_noc_aggregate_bandwidth_cost, costs->noc_aggregate_bandwidth_cost, ERROR_TOL);
            VPR_ERROR(VPR_ERROR_PLACE, msg.c_str());
        }
        costs->noc_aggregate_bandwidth_cost = new_noc_aggregate_bandwidth_cost;

        // only check if the recomputed cost and the current noc latency cost are within the error tolerance if the cost is above 1 picosecond.
        // Otherwise, there is no need to check (we expect the latency cost to be above the threshold of 1 picosecond)
        if (new_noc_latency_cost > MIN_EXPECTED_NOC_LATENCY_COST) {
            if (fabs(
                    new_noc_latency_cost
                    - costs->noc_latency_cost)
                > costs->noc_latency_cost * ERROR_TOL) {
                std::string msg = vtr::string_fmt(
                    "in recompute_costs_from_scratch: new_noc_latency_cost = %g, old noc_latency_cost = %g, ERROR_TOL = %g\n",
                    new_noc_latency_cost, costs->noc_latency_cost, ERROR_TOL);
                VPR_ERROR(VPR_ERROR_PLACE, msg.c_str());
            }
        }
        costs->noc_latency_cost = new_noc_latency_cost;
    }
}

void alloc_and_load_for_fast_cost_update(float place_cost_exp) {
    /* Allocates and loads the chanx_place_cost_fac and chany_place_cost_fac *
     * arrays with the inverse of the average number of tracks per channel   *
     * between [subhigh] and [sublow].  This is only useful for the cost     *
     * function that takes the length of the net bounding box in each        *
     * dimension divided by the average number of tracks in that direction.  *
     * For other cost functions, you don't have to bother calling this       *
     * routine; when using the cost function described above, however, you   *
     * must always call this routine after you call init_chan and before     *
     * you do any placement cost determination.  The place_cost_exp factor   *
     * specifies to what power the width of the channel should be taken --   *
     * larger numbers make narrower channels more expensive.                 */

    auto& device_ctx = g_vpr_ctx.device();

    /* Access arrays below as chan?_place_cost_fac[subhigh][sublow].  Since   *
     * subhigh must be greater than or equal to sublow, we only need to       *
     * allocate storage for the lower half of a matrix.                       */

    //chanx_place_cost_fac = new float*[(device_ctx.grid.height())];
    //for (size_t i = 0; i < device_ctx.grid.height(); i++)
    //    chanx_place_cost_fac[i] = new float[(i + 1)];

    //chany_place_cost_fac = new float*[(device_ctx.grid.width() + 1)];
    //for (size_t i = 0; i < device_ctx.grid.width(); i++)
    //    chany_place_cost_fac[i] = new float[(i + 1)];

    chanx_place_cost_fac.resize({device_ctx.grid.height(), device_ctx.grid.height() + 1});
    chany_place_cost_fac.resize({device_ctx.grid.width(), device_ctx.grid.width() + 1});

    /* First compute the number of tracks between channel high and channel *
     * low, inclusive, in an efficient manner.                             */

    chanx_place_cost_fac[0][0] = device_ctx.chan_width.x_list[0];

    for (size_t high = 1; high < device_ctx.grid.height(); high++) {
        chanx_place_cost_fac[high][high] = device_ctx.chan_width.x_list[high];
        for (size_t low = 0; low < high; low++) {
            chanx_place_cost_fac[high][low] = chanx_place_cost_fac[high - 1][low]
                                              + device_ctx.chan_width.x_list[high];
        }
    }

    /* Now compute the inverse of the average number of tracks per channel *
     * between high and low.  The cost function divides by the average     *
     * number of tracks per channel, so by storing the inverse I convert   *
     * this to a faster multiplication.  Take this final number to the     *
     * place_cost_exp power -- numbers other than one mean this is no      *
     * longer a simple "average number of tracks"; it is some power of     *
     * that, allowing greater penalization of narrow channels.             */

    for (size_t high = 0; high < device_ctx.grid.height(); high++)
        for (size_t low = 0; low <= high; low++) {
            /* Since we will divide the wiring cost by the average channel *
             * capacity between high and low, having only 0 width channels *
             * will result in infinite wiring capacity normalization       *
             * factor, and extremely bad placer behaviour. Hence we change *
             * this to a small (1 track) channel capacity instead.         */
            if (chanx_place_cost_fac[high][low] == 0.0f) {
                VTR_LOG_WARN("CHANX place cost fac is 0 at %d %d\n", high, low);
                chanx_place_cost_fac[high][low] = 1.0f;
            }

            chanx_place_cost_fac[high][low] = (high - low + 1.)
                                              / chanx_place_cost_fac[high][low];
            chanx_place_cost_fac[high][low] = pow(
                (double)chanx_place_cost_fac[high][low],
                (double)place_cost_exp);
        }

    /* Now do the same thing for the y-directed channels.  First get the  *
     * number of tracks between channel high and channel low, inclusive.  */

    chany_place_cost_fac[0][0] = device_ctx.chan_width.y_list[0];

    for (size_t high = 1; high < device_ctx.grid.width(); high++) {
        chany_place_cost_fac[high][high] = device_ctx.chan_width.y_list[high];
        for (size_t low = 0; low < high; low++) {
            chany_place_cost_fac[high][low] = chany_place_cost_fac[high - 1][low]
                                              + device_ctx.chan_width.y_list[high];
        }
    }

    /* Now compute the inverse of the average number of tracks per channel *
     * between high and low.  Take to specified power.                     */

    for (size_t high = 0; high < device_ctx.grid.width(); high++)
        for (size_t low = 0; low <= high; low++) {
            /* Since we will divide the wiring cost by the average channel *
             * capacity between high and low, having only 0 width channels *
             * will result in infinite wiring capacity normalization       *
             * factor, and extremely bad placer behaviour. Hence we change *
             * this to a small (1 track) channel capacity instead.         */
            if (chany_place_cost_fac[high][low] == 0.0f) {
                VTR_LOG_WARN("CHANY place cost fac is 0 at %d %d\n", high, low);
                chany_place_cost_fac[high][low] = 1.0f;
            }

            chany_place_cost_fac[high][low] = (high - low + 1.)
                                              / chany_place_cost_fac[high][low];
            chany_place_cost_fac[high][low] = pow(
                (double)chany_place_cost_fac[high][low],
                (double)place_cost_exp);
        }
}

void free_fast_cost_update() {
    chanx_place_cost_fac.clear();
    chany_place_cost_fac.clear();
}

void init_net_cost_structs(size_t num_nets) {
    net_cost.resize(num_nets, -1.);
    proposed_net_cost.resize(num_nets, -1.);
    /* Used to store costs for moves not yet made and to indicate when a net's   *
     * cost has been recomputed. proposed_net_cost[inet] < 0 means net's cost hasn't *
     * been recomputed.                                                          */
    bb_updated_before.resize(num_nets, NOT_UPDATED_YET);

}

void free_net_cost_structs() {
    vtr::release_memory(net_cost);
    vtr::release_memory(proposed_net_cost);
    vtr::release_memory(bb_updated_before);
}

void init_try_swap_net_cost_structs(size_t num_nets) {
    ts_bb_coord_new.resize(num_nets, t_bb());
    ts_bb_edge_new.resize(num_nets, t_bb());
    ts_nets_to_update.resize(num_nets, ClusterNetId::INVALID());
}

void free_try_swap_net_cost_structs() {
    vtr::release_memory(ts_bb_coord_new);
    vtr::release_memory(ts_bb_edge_new);
    vtr::release_memory(ts_nets_to_update);
}
