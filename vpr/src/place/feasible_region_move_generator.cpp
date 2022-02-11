#include "feasible_region_move_generator.h"
#include "globals.h"
#include <algorithm>
#include "math.h"
#include "place_constraints.h"

e_create_move FeasibleRegionMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected, e_move_type& /*move_type*/, float rlim, const t_placer_opts& placer_opts, const PlacerCriticalities* criticalities) {
    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_move_ctx = g_placer_ctx.mutable_move();

    /* Pick a random block to be swapped with another random block.   */
    // pick it from the highly critical blocks
    if (place_move_ctx.highly_crit_pins.size() == 0) {
        return e_create_move::ABORT; //No critical block
    }
    std::pair<ClusterNetId, int> crit_pin = place_move_ctx.highly_crit_pins[vtr::irand(place_move_ctx.highly_crit_pins.size() - 1)];
    ClusterBlockId b_from = cluster_ctx.clb_nlist.net_driver_block(crit_pin.first);

    if (!b_from) {
        return e_create_move::ABORT; //No movable block found
    }

    if (place_ctx.block_locs[b_from].is_fixed) {
        return e_create_move::ABORT; //Block is fixed, cannot move
    }

    //from block data
    t_pl_loc from = place_ctx.block_locs[b_from].loc;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = g_vpr_ctx.device().grid[from.x][from.y].type;
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));

    /* Calculate the feasible region */
    t_pl_loc to;
    int ipin;
    ClusterBlockId bnum;
    int max_x, min_x, max_y, min_y;

    place_move_ctx.X_coord.clear();
    place_move_ctx.Y_coord.clear();
    //For critical input nodes, calculate the x & y min-max values
    for (ClusterPinId pin_id : cluster_ctx.clb_nlist.block_input_pins(b_from)) {
        ClusterNetId net_id = cluster_ctx.clb_nlist.pin_net(pin_id);
        if (cluster_ctx.clb_nlist.net_is_ignored(net_id))
            continue;

        ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
        if (criticalities->criticality(net_id, ipin) > placer_opts.place_crit_limit) {
            bnum = cluster_ctx.clb_nlist.net_driver_block(net_id);
            place_move_ctx.X_coord.push_back(place_ctx.block_locs[bnum].loc.x);
            place_move_ctx.Y_coord.push_back(place_ctx.block_locs[bnum].loc.y);
        }
    }
    if (place_move_ctx.X_coord.size() != 0) {
        max_x = *(std::max_element(place_move_ctx.X_coord.begin(), place_move_ctx.X_coord.end()));
        min_x = *(std::min_element(place_move_ctx.X_coord.begin(), place_move_ctx.X_coord.end()));
        max_y = *(std::max_element(place_move_ctx.Y_coord.begin(), place_move_ctx.Y_coord.end()));
        min_y = *(std::min_element(place_move_ctx.Y_coord.begin(), place_move_ctx.Y_coord.end()));
    } else {
        max_x = from.x;
        min_x = from.x;
        max_y = from.y;
        min_y = from.y;
    }

    //Get the most critical output of the node
    int xt, yt;
    xt = 0;
    yt = 0;

    ClusterBlockId b_output = cluster_ctx.clb_nlist.net_pin_block(crit_pin.first, crit_pin.second);
    t_pl_loc output_loc = place_ctx.block_locs[b_output].loc;
    xt = output_loc.x;
    yt = output_loc.y;

    /**
     * @brief determine the feasible region
     *
     * The algorithm is described in Chen et al. "Simultaneous Timing-Driven Placement and Duplication"
     * it determines the coords of the feasible region based on the relative location of the location of 
     * the most critical output block, the current location of the block and the locations of the highly critical inputs
     */
    t_bb FR_coords;
    if (xt < min_x) {
        FR_coords.xmin = std::min(from.x, xt);
        FR_coords.xmax = std::max(from.x, min_x);
    } else if (xt < max_x) {
        FR_coords.xmin = std::min(from.x, xt);
        FR_coords.xmax = std::max(from.x, xt);
    } else {
        FR_coords.xmin = std::min(from.x, max_x);
        FR_coords.xmax = std::max(from.x, xt);
    }
    VTR_ASSERT(FR_coords.xmin <= FR_coords.xmax);

    if (yt < min_y) {
        FR_coords.ymin = std::min(from.y, yt);
        FR_coords.ymax = std::max(from.y, min_y);
    } else if (yt < max_y) {
        FR_coords.ymin = std::min(from.y, yt);
        FR_coords.ymax = std::max(from.y, yt);
    } else {
        FR_coords.ymin = std::min(from.y, max_y);
        FR_coords.ymax = std::max(from.y, yt);
    }
    VTR_ASSERT(FR_coords.ymin <= FR_coords.ymax);

    t_range_limiters range_limiters;
    range_limiters.original_rlim = rlim;
    range_limiters.dm_rlim = placer_opts.place_dm_rlim;
    range_limiters.first_rlim = place_move_ctx.first_rlim;

    // Try to find a legal location inside the feasible region
    if (!find_to_loc_median(cluster_from_type, from, &FR_coords, to, b_from)) {
        /** If there is no legal location in the feasible region, calculate the center of the FR and try to find a legal location 
         *  in a range around this center.
         */
        t_pl_loc center;
        center.x = (FR_coords.xmin + FR_coords.xmax) / 2;
        center.y = (FR_coords.ymin + FR_coords.ymax) / 2;
        if (!find_to_loc_centroid(cluster_from_type, from, center, range_limiters, to, b_from))
            return e_create_move::ABORT;
    }

    e_create_move create_move = ::create_move(blocks_affected, b_from, to);

    //Check that all of the blocks affected by the move would still be in a legal floorplan region after the swap
    if (!floorplan_legal(blocks_affected)) {
        return e_create_move::ABORT;
    }

    return create_move;
}
