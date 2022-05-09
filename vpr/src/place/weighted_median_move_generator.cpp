#include "weighted_median_move_generator.h"
#include "globals.h"
#include <algorithm>
#include "math.h"
#include "place_constraints.h"

#define CRIT_MULT_FOR_W_MEDIAN 10

static void get_bb_cost_for_net_excluding_block(ClusterNetId net_id, ClusterBlockId block_id, ClusterPinId moving_pin_id, const PlacerCriticalities* criticalities, t_bb_cost* coords, bool& skip_net);

e_create_move WeightedMedianMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected, e_move_type& /*move_type*/, float rlim, const t_placer_opts& placer_opts, const PlacerCriticalities* criticalities) {
    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    auto& place_move_ctx = g_placer_ctx.mutable_move();

    /* Pick a random block to be swapped with another random block.   */
    ClusterBlockId b_from = pick_from_block();

    if (!b_from) {
        return e_create_move::ABORT; //No movable block found
    }

    t_pl_loc from = place_ctx.block_locs[b_from].loc;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = g_vpr_ctx.device().grid[from.x][from.y].type;
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));

    /* Calculate the Edge weighted median region */
    t_pl_loc to;

    t_bb_cost coords;
    t_bb limit_coords;

    //clear the vectors that saves X & Y coords
    //reused to save allocation time
    place_move_ctx.X_coord.clear();
    place_move_ctx.Y_coord.clear();

    //true if the net is a feedback from the block to itself (all the net terminals are connected to the same block)
    bool skip_net;

    //iterate over block pins
    for (ClusterPinId pin_id : cluster_ctx.clb_nlist.block_pins(b_from)) {
        ClusterNetId net_id = cluster_ctx.clb_nlist.pin_net(pin_id);
        if (cluster_ctx.clb_nlist.net_is_ignored(net_id))
            continue;
        if (int(cluster_ctx.clb_nlist.net_pins(net_id).size()) > placer_opts.place_high_fanout_net)
            continue;
        /**
         * Calculate the bounding box edges and the cost of each edge.
         *
         * Note: skip_net returns true if this net should be skipped. Currently, the only case to skip a net is the feedback nets
         *       (a net that all its terminals connected to the same block). Logically, this net should be neglected as it is only connected 
         *       to the moving block. Experimentally, we found that including these nets into calculations badly affect the averall placement
         *       solution especillay for some large designs.
         */
        get_bb_cost_for_net_excluding_block(net_id, b_from, pin_id, criticalities, &coords, skip_net);
        if (skip_net)
            continue;

        // We need to insert the calculated edges in the X,Y vectors multiple times based on the criticality of the pin that caused each of them.
        // As all the criticalities are [0,1], we map it to [0,CRIT_MULT_FOR_W_MEDIAN] inserts in the vectors for each edge
        // by multiplying each edge's criticality by CRIT_MULT_FOR_W_MEDIAN
        place_move_ctx.X_coord.insert(place_move_ctx.X_coord.end(), ceil(coords.xmin.criticality * CRIT_MULT_FOR_W_MEDIAN), coords.xmin.edge);
        place_move_ctx.X_coord.insert(place_move_ctx.X_coord.end(), ceil(coords.xmax.criticality * CRIT_MULT_FOR_W_MEDIAN), coords.xmax.edge);
        place_move_ctx.Y_coord.insert(place_move_ctx.Y_coord.end(), ceil(coords.ymin.criticality * CRIT_MULT_FOR_W_MEDIAN), coords.ymin.edge);
        place_move_ctx.Y_coord.insert(place_move_ctx.Y_coord.end(), ceil(coords.ymax.criticality * CRIT_MULT_FOR_W_MEDIAN), coords.ymax.edge);
    }

    if ((place_move_ctx.X_coord.size() == 0) || (place_move_ctx.Y_coord.size() == 0))
        return e_create_move::ABORT;

    //calculate the weighted median region
    std::sort(place_move_ctx.X_coord.begin(), place_move_ctx.X_coord.end());
    std::sort(place_move_ctx.Y_coord.begin(), place_move_ctx.Y_coord.end());

    if (place_move_ctx.X_coord.size() == 1) {
        limit_coords.xmin = place_move_ctx.X_coord[0];
        limit_coords.xmax = limit_coords.xmin;
    } else {
        limit_coords.xmin = place_move_ctx.X_coord[floor((place_move_ctx.X_coord.size() - 1) / 2)];
        limit_coords.xmax = place_move_ctx.X_coord[floor((place_move_ctx.X_coord.size() - 1) / 2) + 1];
    }

    if (place_move_ctx.Y_coord.size() == 1) {
        limit_coords.ymin = place_move_ctx.Y_coord[0];
        limit_coords.ymax = limit_coords.ymin;
    } else {
        limit_coords.ymin = place_move_ctx.Y_coord[floor((place_move_ctx.Y_coord.size() - 1) / 2)];
        limit_coords.ymax = place_move_ctx.Y_coord[floor((place_move_ctx.Y_coord.size() - 1) / 2) + 1];
    }

    t_range_limiters range_limiters;
    range_limiters.original_rlim = rlim;
    range_limiters.dm_rlim = placer_opts.place_dm_rlim;
    range_limiters.first_rlim = place_move_ctx.first_rlim;

    t_pl_loc w_median_point;
    w_median_point.x = (limit_coords.xmin + limit_coords.xmax) / 2;
    w_median_point.y = (limit_coords.ymin + limit_coords.ymax) / 2;
    if (!find_to_loc_centroid(cluster_from_type, from, w_median_point, range_limiters, to, b_from)) {
        return e_create_move::ABORT;
    }

    e_create_move create_move = ::create_move(blocks_affected, b_from, to);

    //Check that all of the blocks affected by the move would still be in a legal floorplan region after the swap
    if (!floorplan_legal(blocks_affected)) {
        return e_create_move::ABORT;
    }

    return create_move;
}

/**
 * This routine finds the bounding box and the cost of each side of the bounding box,
 * which is defined as the criticality of the connection that led to the bounding box extending 
 * that far. If more than one terminal leads to a bounding box edge, w pick the cost using the criticality of the first one. 
 * This is helpful in computing weighted median moves. 
 *
 * Outputs:
 *      - coords: the bounding box and the edge costs
 *      - skip_net: returns whether this net should be skipped in calculation or not
 *
 * Inputs:
 *      - net_id: The net we are considering
 *      - moving_pin_id: pin (which should be on this net) on a block that is being moved.
 *      - criticalities: the timing criticalities of all connections
 */
static void get_bb_cost_for_net_excluding_block(ClusterNetId net_id, ClusterBlockId, ClusterPinId moving_pin_id, const PlacerCriticalities* criticalities, t_bb_cost* coords, bool& skip_net) {
    int pnum, x, y, xmin, xmax, ymin, ymax;
    float xmin_cost, xmax_cost, ymin_cost, ymax_cost, cost;

    skip_net = true;

    xmin = 0;
    xmax = 0;
    ymin = 0;
    ymax = 0;
    cost = 0.0;
    xmin_cost = 0.0;
    xmax_cost = 0.0;
    ymin_cost = 0.0;
    ymax_cost = 0.0;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

    ClusterBlockId bnum;
    bool is_first_block = true;
    int ipin;
    for (auto pin_id : cluster_ctx.clb_nlist.net_pins(net_id)) {
        bnum = cluster_ctx.clb_nlist.pin_block(pin_id);

        if (pin_id != moving_pin_id) {
            skip_net = false;
            pnum = tile_pin_index(pin_id);
            /**
             * Calculates the pin index of the correct pin to calculate the required connection
             *
             * if the current pin is the driver, we only care about one sink (the moving pin)
             * else if the current pin is a sink, calculate the criticality of itself
             */
            if (cluster_ctx.clb_nlist.pin_type(pin_id) == PinType::DRIVER) {
                ipin = cluster_ctx.clb_nlist.pin_net_index(moving_pin_id);
            } else {
                ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
            }
            cost = criticalities->criticality(net_id, ipin);

            VTR_ASSERT(pnum >= 0);
            x = place_ctx.block_locs[bnum].loc.x + physical_tile_type(bnum)->pin_width_offset[pnum];
            y = place_ctx.block_locs[bnum].loc.y + physical_tile_type(bnum)->pin_height_offset[pnum];

            x = std::max(std::min(x, (int)grid.width() - 2), 1);  //-2 for no perim channels
            y = std::max(std::min(y, (int)grid.height() - 2), 1); //-2 for no perim channels

            if (is_first_block) {
                xmin = x;
                xmin_cost = cost;
                ymin = y;
                ymin_cost = cost;
                xmax = x;
                xmax_cost = cost;
                ymax = y;
                ymax_cost = cost;
                is_first_block = false;
            } else {
                if (x < xmin) {
                    xmin = x;
                    xmin_cost = cost;
                } else if (x > xmax) {
                    xmax = x;
                    xmax_cost = cost;
                }

                if (y < ymin) {
                    ymin = y;
                    ymin_cost = cost;
                } else if (y > ymax) {
                    ymax = y;
                    ymax_cost = cost;
                }
            }
        }
    }

    // Copy the bounding box edges and corresponding criticalities into the proper structure
    coords->xmin = {xmin, xmin_cost};
    coords->xmax = {xmax, xmax_cost};
    coords->ymin = {ymin, ymin_cost};
    coords->ymax = {ymax, ymax_cost};
}
