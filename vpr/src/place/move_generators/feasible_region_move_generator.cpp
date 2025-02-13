#include "feasible_region_move_generator.h"

#include "globals.h"
#include "physical_types_util.h"
#include "place_constraints.h"
#include "place_macro.h"
#include "placer_state.h"
#include "move_utils.h"

#include <algorithm>
#include <cmath>

FeasibleRegionMoveGenerator::FeasibleRegionMoveGenerator(PlacerState& placer_state,
                                                         e_reward_function reward_function,
                                                         vtr::RngContainer& rng)
    : MoveGenerator(placer_state, reward_function, rng) {}

e_create_move FeasibleRegionMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected,
                                                        t_propose_action& proposed_action,
                                                        float rlim,
                                                        const PlaceMacros& place_macros,
                                                        const t_placer_opts& placer_opts,
                                                        const PlacerCriticalities* criticalities) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& placer_state = placer_state_.get();
    auto& place_move_ctx = placer_state.mutable_move();
    const auto& block_locs = placer_state.block_locs();
    const auto& blk_loc_registry = placer_state.blk_loc_registry();

    ClusterNetId net_from;
    int pin_from;
    //Find a movable block based on blk_type
    ClusterBlockId b_from = propose_block_to_move(placer_opts,
                                                  proposed_action.logical_blk_type_index,
                                                  /*highly_crit_block=*/true,
                                                  criticalities,
                                                  &net_from,
                                                  &pin_from,
                                                  placer_state,
                                                  rng_);

    VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "Feasible Region Move Choose Block %di - rlim %f\n", size_t(b_from), rlim);

    if (!b_from) { //No movable block found
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\tNo movable block found\n");
        return e_create_move::ABORT;
    }

    //from block data
    t_pl_loc from = block_locs[b_from].loc;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = g_vpr_ctx.device().grid.get_physical_type({from.x, from.y, from.layer});
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));

    /* Calculate the feasible region */
    t_pl_loc to;
    // Currently, we don't change the layer for this move
    to.layer = from.layer;
    int max_x, min_x, max_y, min_y;

    place_move_ctx.X_coord.clear();
    place_move_ctx.Y_coord.clear();
    //For critical input nodes, calculate the x & y min-max values
    for (ClusterPinId pin_id : cluster_ctx.clb_nlist.block_input_pins(b_from)) {
        ClusterNetId net_id = cluster_ctx.clb_nlist.pin_net(pin_id);
        if (cluster_ctx.clb_nlist.net_is_ignored(net_id))
            continue;

        int ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);
        if (criticalities->criticality(net_id, ipin) > placer_opts.place_crit_limit) {
            ClusterBlockId bnum = cluster_ctx.clb_nlist.net_driver_block(net_id);
            place_move_ctx.X_coord.push_back(block_locs[bnum].loc.x);
            place_move_ctx.Y_coord.push_back(block_locs[bnum].loc.y);
        }
    }
    if (!place_move_ctx.X_coord.empty()) {
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
    ClusterBlockId b_output = cluster_ctx.clb_nlist.net_pin_block(net_from, pin_from);
    t_pl_loc output_loc = block_locs[b_output].loc;
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

    FR_coords.layer_min = from.layer;
    FR_coords.layer_max = from.layer;
    VTR_ASSERT(FR_coords.ymin <= FR_coords.ymax);

    t_range_limiters range_limiters{rlim,
                                    place_move_ctx.first_rlim,
                                    placer_opts.place_dm_rlim};

    // Try to find a legal location inside the feasible region
    if (!find_to_loc_median(cluster_from_type, from, &FR_coords, to, b_from, blk_loc_registry, rng_)) {
        /** If there is no legal location in the feasible region, calculate the center of the FR and try to find a legal location 
         *  in a range around this center.
         */
        t_pl_loc center;
        center.x = (FR_coords.xmin + FR_coords.xmax) / 2;
        center.y = (FR_coords.ymin + FR_coords.ymax) / 2;
        // TODO: Currently, we don't move blocks between different types of layers
        center.layer = from.layer;
        if (!find_to_loc_centroid(cluster_from_type, from, center, range_limiters, to, b_from, blk_loc_registry, rng_))
            return e_create_move::ABORT;
    }

    e_create_move create_move = ::create_move(blocks_affected, b_from, to, blk_loc_registry, place_macros);

    //Check that all the blocks affected by the move would still be in a legal floorplan region after the swap
    if (!floorplan_legal(blocks_affected)) {
        return e_create_move::ABORT;
    }

    return create_move;
}
