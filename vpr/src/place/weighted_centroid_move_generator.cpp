#include "weighted_centroid_move_generator.h"

#include "globals.h"
#include "directed_moves_util.h"
#include "place_constraints.h"
#include "placer_state.h"
#include "move_utils.h"

WeightedCentroidMoveGenerator::WeightedCentroidMoveGenerator(PlacerState& placer_state,
                                                             e_reward_function reward_function)
    : MoveGenerator(placer_state, reward_function) {}

e_create_move WeightedCentroidMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected,
                                                          t_propose_action& proposed_action,
                                                          float rlim,
                                                          const t_placer_opts& placer_opts,
                                                          const PlacerCriticalities* criticalities) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& device_ctx = g_vpr_ctx.device();
    auto& placer_state = placer_state_.get();
    const auto& block_locs = placer_state.block_locs();
    auto& place_move_ctx = placer_state.mutable_move();
    const auto& blk_loc_registry = placer_state.blk_loc_registry();

    //Find a movable block based on blk_type
    ClusterBlockId b_from = propose_block_to_move(placer_opts,
                                                  proposed_action.logical_blk_type_index,
                                                  /*highly_crit_block=*/false,
                                                  /*net_from=*/nullptr,
                                                  /*pin_from=*/nullptr,
                                                  placer_state);

    VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "Weighted Centroid Move Choose Block %d - rlim %f\n", size_t(b_from), rlim);

    if (!b_from) { //No movable block found
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\tNo movable block found\n");
        return e_create_move::ABORT;
    }

    t_pl_loc from = block_locs[b_from].loc;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = device_ctx.grid.get_physical_type({from.x, from.y, from.layer});
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));

    t_range_limiters range_limiters{rlim,
                                    place_move_ctx.first_rlim,
                                    placer_opts.place_dm_rlim};

    t_pl_loc to, centroid;

    /* Calculate the weighted centroid */
    calculate_centroid_loc(b_from, true, centroid, criticalities, blk_loc_registry);

    // Centroid location is not necessarily a valid location, and the downstream location expect a valid
    // layer for "to" location. So if the layer is not valid, we set it to the same layer as from loc.
    centroid.layer = (centroid.layer < 0) ? from.layer : centroid.layer;
    if (!find_to_loc_centroid(cluster_from_type, from, centroid, range_limiters, to, b_from, blk_loc_registry)) {
        return e_create_move::ABORT;
    }

    e_create_move create_move = ::create_move(blocks_affected, b_from, to, blk_loc_registry);

    //Check that all the blocks affected by the move would still be in a legal floorplan region after the swap
    if (!floorplan_legal(blocks_affected)) {
        return e_create_move::ABORT;
    }

    return create_move;
}
