#include "move_utils.h"

#include "globals.h"
#include "place_util.h"

//Records that block 'blk' should be moved to the specified 'to' location
e_block_move_result record_block_move(t_pl_blocks_to_be_moved& blocks_affected, ClusterBlockId blk, t_pl_loc to) {
    auto res = blocks_affected.moved_to.emplace(to);
    if (!res.second) {
        log_move_abort("duplicate block move to location");
        return e_block_move_result::ABORT;
    }

    auto& place_ctx = g_vpr_ctx.mutable_placement();

    t_pl_loc from = place_ctx.block_locs[blk].loc;

    auto res2 = blocks_affected.moved_from.emplace(from);
    if (!res2.second) {
        log_move_abort("duplicate block move from location");
        return e_block_move_result::ABORT;
    }

    VTR_ASSERT_SAFE(to.sub_tile < int(place_ctx.grid_blocks.num_blocks_at_location({to.x, to.y, to.layer})));

    // Sets up the blocks moved
    int imoved_blk = blocks_affected.num_moved_blocks;
    blocks_affected.moved_blocks[imoved_blk].block_num = blk;
    blocks_affected.moved_blocks[imoved_blk].old_loc = from;
    blocks_affected.moved_blocks[imoved_blk].new_loc = to;
    blocks_affected.num_moved_blocks++;

    return e_block_move_result::VALID;
}

//Moves the blocks in blocks_affected to their new locations
void apply_move_blocks(const t_pl_blocks_to_be_moved& blocks_affected) {
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    auto& device_ctx = g_vpr_ctx.device();

    //Swap the blocks, but don't swap the nets or update place_ctx.grid_blocks
    //yet since we don't know whether the swap will be accepted
    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; ++iblk) {
        ClusterBlockId blk = blocks_affected.moved_blocks[iblk].block_num;

        const t_pl_loc& old_loc = blocks_affected.moved_blocks[iblk].old_loc;
        const t_pl_loc& new_loc = blocks_affected.moved_blocks[iblk].new_loc;

        // move the block to its new location
        place_ctx.block_locs[blk].loc = new_loc;

        // get physical tile type of the old location
        t_physical_tile_type_ptr old_type = device_ctx.grid.get_physical_type({old_loc.x,old_loc.y,old_loc.layer});
        // get physical tile type of the new location
        t_physical_tile_type_ptr new_type = device_ctx.grid.get_physical_type({new_loc.x,new_loc.y, new_loc.layer});

        //if physical tile type of old location does not equal physical tile type of new location, sync the new physical pins
        if (old_type != new_type) {
            place_sync_external_block_connections(blk);
        }
    }
}

//Commits the blocks in blocks_affected to their new locations (updates inverse
//lookups via place_ctx.grid_blocks)
void commit_move_blocks(const t_pl_blocks_to_be_moved& blocks_affected) {
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    /* Swap physical location */
    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; ++iblk) {
        ClusterBlockId blk = blocks_affected.moved_blocks[iblk].block_num;

        const t_pl_loc& to = blocks_affected.moved_blocks[iblk].new_loc;
        const t_pl_loc& from = blocks_affected.moved_blocks[iblk].old_loc;

        //Remove from old location only if it hasn't already been updated by a previous block update
        if (place_ctx.grid_blocks.block_at_location(from) == blk) {
            place_ctx.grid_blocks.set_block_at_location(from, EMPTY_BLOCK_ID);
            place_ctx.grid_blocks.set_usage({from.x, from.y, from.layer},
                                            place_ctx.grid_blocks.get_usage({from.x, from.y, from.layer}) - 1);
        }

        //Add to new location
        if (place_ctx.grid_blocks.block_at_location(to) == EMPTY_BLOCK_ID) {
            //Only need to increase usage if previously unused
            place_ctx.grid_blocks.set_usage({to.x, to.y, to.layer},
                                            place_ctx.grid_blocks.get_usage({to.x, to.y, to.layer}) + 1);
        }
        place_ctx.grid_blocks.set_block_at_location(to, blk);

    } // Finish updating clb for all blocks
}

//Moves the blocks in blocks_affected to their old locations
void revert_move_blocks(const t_pl_blocks_to_be_moved& blocks_affected) {
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    auto& device_ctx = g_vpr_ctx.device();

    // Swap the blocks back, nets not yet swapped they don't need to be changed
    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; ++iblk) {
        ClusterBlockId blk = blocks_affected.moved_blocks[iblk].block_num;

        const t_pl_loc& old_loc = blocks_affected.moved_blocks[iblk].old_loc;
        const t_pl_loc& new_loc = blocks_affected.moved_blocks[iblk].new_loc;

        // return the block to where it was before the swap
        place_ctx.block_locs[blk].loc = old_loc;

        // get physical tile type of the old location
        t_physical_tile_type_ptr old_type = device_ctx.grid.get_physical_type({old_loc.x,old_loc.y,old_loc.layer});
        // get physical tile type of the new location
        t_physical_tile_type_ptr new_type = device_ctx.grid.get_physical_type({new_loc.x,new_loc.y, new_loc.layer});

        //if physical tile type of old location does not equal physical tile type of new location, sync the new physical pins
        if (old_type != new_type) {
            place_sync_external_block_connections(blk);
        }

        VTR_ASSERT_SAFE_MSG(place_ctx.grid_blocks.block_at_location(old_loc) == blk, "Grid blocks should only have been updated if swap committed (not reverted)");
    }
}

//Clears the current move so a new move can be proposed
void clear_move_blocks(t_pl_blocks_to_be_moved& blocks_affected) {
    //Reset moved flags
    blocks_affected.moved_to.clear();
    blocks_affected.moved_from.clear();

    //For run-time, we just reset num_moved_blocks to zero, but do not free the blocks_affected
    //array to avoid memory allocation

    blocks_affected.num_moved_blocks = 0;

    blocks_affected.affected_pins.clear();
}
