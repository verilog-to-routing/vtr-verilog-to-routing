#include "centroid_move_generator.h"
#include "vpr_types.h"
#include "globals.h"
#include "directed_moves_util.h"
#include "place_constraints.h"

e_create_move CentroidMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected, e_move_type& /*move_type*/, float rlim, const t_placer_opts& placer_opts, const PlacerCriticalities* /*criticalities*/) {
    /* Pick a random block to be swapped with another random block.   */
    ClusterBlockId b_from = pick_from_block();
    if (!b_from) {
        return e_create_move::ABORT; //No movable block found
    }
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    auto& place_move_ctx = g_placer_ctx.mutable_move();

    t_pl_loc from = place_ctx.block_locs[b_from].loc;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = device_ctx.grid[from.x][from.y].type;
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));

    t_range_limiters range_limiters;
    range_limiters.original_rlim = rlim;
    range_limiters.dm_rlim = placer_opts.place_dm_rlim;
    range_limiters.first_rlim = place_move_ctx.first_rlim;

    t_pl_loc to, centroid;

    /* Calculate the centroid location*/
    calculate_centroid_loc(b_from, false, centroid, NULL);

    /* Find a location near the weighted centroid_loc */
    if (!find_to_loc_centroid(cluster_from_type, from, centroid, range_limiters, to, b_from)) {
        return e_create_move::ABORT;
    }

    e_create_move create_move = ::create_move(blocks_affected, b_from, to);

    //Check that all of the blocks affected by the move would still be in a legal floorplan region after the swap
    if (!floorplan_legal(blocks_affected)) {
        return e_create_move::ABORT;
    }

    return create_move;
}
