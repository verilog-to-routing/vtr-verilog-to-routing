#include "uniform_inter_layer_move_generator.h"
#include "globals.h"
#include "place_constraints.h"
#include "move_utils.h"

e_create_move UniformInterLayerMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected, t_propose_action& proposed_action, float /*rlim*/, const t_placer_opts& /*placer_opts*/, const PlacerCriticalities* /*criticalities*/) {
    // If this moved is called, we know that there are at least two layers.
    VTR_ASSERT(g_vpr_ctx.device().grid.get_num_layers() > 1);
    //Find a movable block based on blk_type
    ClusterBlockId b_from = propose_block_to_move(proposed_action.logical_blk_type_index, false, nullptr, nullptr);

    if (!b_from) { //No movable block found
        return e_create_move::ABORT;
    }

    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    t_pl_loc from = place_ctx.block_locs[b_from].loc;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = g_vpr_ctx.device().grid.get_physical_type({from.x, from.y, from.layer});
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));

    const auto& block_compressed_grid = g_vpr_ctx.placement().compressed_block_grids[cluster_from_type->index];

    const auto& compatible_layers = block_compressed_grid.get_layer_nums();

    if (compatible_layers.size() < 2) {
        return e_create_move::ABORT;
    }

    int to_layer = compatible_layers[vtr::irand((int)compatible_layers.size() - 1)];

    t_pl_loc to = from;
    to.layer = to_layer;

    e_create_move create_move = ::create_move(blocks_affected, b_from, to);

    //Check that all the blocks affected by the move would still be in a legal floorplan region after the swap
    if (!floorplan_legal(blocks_affected)) {
        return e_create_move::ABORT;
    }

    return create_move;
}