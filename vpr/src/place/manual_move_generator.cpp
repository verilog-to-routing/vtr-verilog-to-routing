/*
 * @file 	manual_move_generator.cpp
 * @author	Paula Perdomo
 * @date 	2021-07-19
 * @brief 	Contains the ManualMoveGenerator class memeber definitions. The ManualMoveGenerator class inherits from the MoveGenerator class. The class contains a propose_move function that checks if the block requested to move by the user exists and determines whether the manual move is VALID/ABORTED by the placer. If the manual move is determined VALID, the move is created. A manual move is ABORTED if the block requested is not found or movable and if there aren't any compatible subtiles. 
 */

#include "manual_move_generator.h"
#include "manual_moves.h"

#ifndef NO_GRAPHICS
#    include "draw.h"
#endif //NO_GRAPHICS

//Manual Move Generator function
e_create_move ManualMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected, e_move_type& /*move_type*/, float /*rlim*/, const t_placer_opts& /*placer_opts*/, const PlacerCriticalities* /*criticalities*/) {
    int block_id = -1;
    t_pl_loc to;

#ifndef NO_GRAPHICS
    t_draw_state* draw_state = get_draw_state_vars();
    block_id = draw_state->manual_moves_state.manual_move_info.blockID;
    to = draw_state->manual_moves_state.manual_move_info.to_location;
#endif //NO_GRAPHICS

    ClusterBlockId b_from = ClusterBlockId(block_id);

    //Checking if the block was found
    if (!b_from) {
        return e_create_move::ABORT; //No movable block was found
    }

    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();

    //Gets the current location of the block to move.
    t_pl_loc from = place_ctx.block_locs[b_from].loc;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = device_ctx.grid[from.x][from.y].type;
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));

    //Retrieving the compressed block grid for this block type
    const auto& compressed_block_grid = place_ctx.compressed_block_grids[cluster_from_type->index];
    //Checking if the block has a compatible subtile.
    auto to_type = device_ctx.grid[to.x][to.y].type;
    auto& compatible_subtiles = compressed_block_grid.compatible_sub_tiles_for_tile.at(to_type->index);

    //No compatible subtile is found.
    if (std::find(compatible_subtiles.begin(), compatible_subtiles.end(), to.sub_tile) == compatible_subtiles.end()) {
        return e_create_move::ABORT;
    }

    e_create_move create_move = ::create_move(blocks_affected, b_from, to);
    return create_move;
}
