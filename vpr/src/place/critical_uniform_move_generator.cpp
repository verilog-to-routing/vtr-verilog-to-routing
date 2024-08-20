#include "critical_uniform_move_generator.h"
#include "globals.h"
#include "place_constraints.h"
#include "placer_state.h"
#include "move_utils.h"

CriticalUniformMoveGenerator::CriticalUniformMoveGenerator(PlacerState& placer_state)
    : MoveGenerator(placer_state) {}

e_create_move CriticalUniformMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected,
                                                         t_propose_action& proposed_action,
                                                         float rlim,
                                                         const t_placer_opts& placer_opts,
                                                         const PlacerCriticalities* /*criticalities*/) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& placer_state = placer_state_.get();
    const auto& block_locs = placer_state.block_locs();
    const auto& blk_loc_registry = placer_state.blk_loc_registry();

    ClusterNetId net_from;
    int pin_from;
    //Find a movable block based on blk_type
    ClusterBlockId b_from = propose_block_to_move(placer_opts,
                                                  proposed_action.logical_blk_type_index,
                                                  /*highly_crit_block=*/true,
                                                  &net_from,
                                                  &pin_from,
                                                  placer_state);

    VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "Critical Uniform Move Choose Block %d - rlim %f\n", size_t(b_from), rlim);

    if (!b_from) { //No movable block found
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\tNo movable block found\n");
        return e_create_move::ABORT;
    }

    t_pl_loc from = block_locs[b_from].loc;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = g_vpr_ctx.device().grid.get_physical_type({from.x, from.y, from.layer});
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));

    t_pl_loc to;
    to.layer = from.layer;
    if (!find_to_loc_uniform(cluster_from_type, rlim, from, to, b_from, blk_loc_registry)) {
        return e_create_move::ABORT;
    }

    e_create_move create_move = ::create_move(blocks_affected, b_from, to, blk_loc_registry);

    //Check that all the blocks affected by the move would still be in a legal floorplan region after the swap
    if (!floorplan_legal(blocks_affected)) {
        return e_create_move::ABORT;
    }

    return create_move;
}

