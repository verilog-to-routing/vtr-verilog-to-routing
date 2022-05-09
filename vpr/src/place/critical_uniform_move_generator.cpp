#include "critical_uniform_move_generator.h"
#include "globals.h"
#include "place_constraints.h"

e_create_move CriticalUniformMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected, e_move_type& /*move_type*/, float rlim, const t_placer_opts& /*placer_opts*/, const PlacerCriticalities* /*criticalities*/) {
    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_move_ctx = g_placer_ctx.move();

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

    t_pl_loc from = place_ctx.block_locs[b_from].loc;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = g_vpr_ctx.device().grid[from.x][from.y].type;
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));

    t_pl_loc to;

    if (!find_to_loc_uniform(cluster_from_type, rlim, from, to, b_from)) {
        return e_create_move::ABORT;
    }

    e_create_move create_move = ::create_move(blocks_affected, b_from, to);

    //Check that all of the blocks affected by the move would still be in a legal floorplan region after the swap
    if (!floorplan_legal(blocks_affected)) {
        return e_create_move::ABORT;
    }

    return create_move;
}
