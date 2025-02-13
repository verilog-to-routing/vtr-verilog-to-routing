/*
 * @file 	manual_move_generator.cpp
 * @author	Paula Perdomo
 * @date 	2021-07-19
 * @brief 	Contains the ManualMoveGenerator class member definitions.
 * The ManualMoveGenerator class inherits from the MoveGenerator class.
 * The class contains a propose_move function that checks if the block requested
 * to move by the user exists and determines whether the manual move is VALID/ABORTED
 * by the placer. If the manual move is determined VALID, the move is created.
 * A manual move is ABORTED if the block requested is not found or movable and if there aren't any compatible subtiles.
 */

#include "manual_move_generator.h"
#include "manual_moves.h"
#include "physical_types_util.h"
#include "place_macro.h"
#include "placer_state.h"

#ifndef NO_GRAPHICS
#    include "draw.h"
#endif //NO_GRAPHICS

ManualMoveGenerator::ManualMoveGenerator(PlacerState& placer_state, vtr::RngContainer& rng)
    : MoveGenerator(placer_state, e_reward_function::UNDEFINED_REWARD, rng) {}

//Manual Move Generator function
e_create_move ManualMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected,
                                                t_propose_action& /*proposed_action*/,
                                                float /*rlim*/,
                                                const PlaceMacros& place_macros,
                                                const t_placer_opts& /*placer_opts*/,
                                                const PlacerCriticalities* /*criticalities*/) {
    const auto& place_ctx = g_vpr_ctx.placement();
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& blk_loc_registry = placer_state_.get().blk_loc_registry();
    const auto& block_locs = blk_loc_registry.block_locs();

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

    //Gets the current location of the block to move.
    t_pl_loc from = block_locs[b_from].loc;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = device_ctx.grid.get_physical_type({from.x, from.y, from.layer});
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));

    //Retrieving the compressed block grid for this block type
    const auto& compressed_block_grid = place_ctx.compressed_block_grids[cluster_from_type->index];
    //Checking if the block has a compatible subtile.
    auto to_type = device_ctx.grid.get_physical_type({to.x, to.y, to.layer});
    auto& compatible_subtiles = compressed_block_grid.compatible_sub_tile_num(to_type->index);

    //No compatible subtile is found.
    if (std::find(compatible_subtiles.begin(), compatible_subtiles.end(), to.sub_tile) == compatible_subtiles.end()) {
        return e_create_move::ABORT;
    }

    e_create_move create_move = ::create_move(blocks_affected, b_from, to, blk_loc_registry, place_macros);
    return create_move;
}

