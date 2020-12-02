#include "weighted_centroid_move_generator.h"
#include "globals.h"
#include <algorithm>
#include "directed_moves_util.h"

e_create_move WeightedCentroidMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected, e_move_type& /*move_type*/, MoveHelperData& move_helper, float rlim, const t_placer_opts& placer_opts, const PlacerCriticalities* criticalities) {
    /* Pick a random block to be swapped with another random block.   */
    ClusterBlockId b_from = pick_from_block();
    if (!b_from) {
        return e_create_move::ABORT; //No movable block found
    }
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    t_pl_loc from = place_ctx.block_locs[b_from].loc;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = device_ctx.grid[from.x][from.y].type;
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));

    t_range_limiters range_limiters;
    range_limiters.original_rlim = rlim;
    range_limiters.first_rlim = move_helper.first_rlim;
    range_limiters.dm_rlim = placer_opts.place_dm_rlim;

    t_pl_loc to, centroid;

    /* Calculate the weighted centroid */
    calculate_centroid_loc(b_from, true, centroid, criticalities);

    /* Find a  */
    if (!find_to_loc_centroid(cluster_from_type, from, centroid, range_limiters, to)) {
        return e_create_move::ABORT;
    }

    return ::create_move(blocks_affected, b_from, to);
}
