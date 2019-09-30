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

    VTR_ASSERT_SAFE(to.z < int(place_ctx.grid_blocks[to.x][to.y].blocks.size()));

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

    //Swap the blocks, but don't swap the nets or update place_ctx.grid_blocks
    //yet since we don't know whether the swap will be accepted
    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; ++iblk) {
        ClusterBlockId blk = blocks_affected.moved_blocks[iblk].block_num;

        place_ctx.block_locs[blk].loc = blocks_affected.moved_blocks[iblk].new_loc;
    }
}

//Commits the blocks in blocks_affected to their new locations (updates inverse
//lookups via place_ctx.grid_blocks)
void commit_move_blocks(const t_pl_blocks_to_be_moved& blocks_affected) {
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    /* Swap physical location */
    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; ++iblk) {
        ClusterBlockId blk = blocks_affected.moved_blocks[iblk].block_num;

        t_pl_loc to = blocks_affected.moved_blocks[iblk].new_loc;

        t_pl_loc from = blocks_affected.moved_blocks[iblk].old_loc;

        //Remove from old location only if it hasn't already been updated by a previous block update
        if (place_ctx.grid_blocks[from.x][from.y].blocks[from.z] == blk) {
            ;
            place_ctx.grid_blocks[from.x][from.y].blocks[from.z] = EMPTY_BLOCK_ID;
            --place_ctx.grid_blocks[from.x][from.y].usage;
        }

        //Add to new location
        if (place_ctx.grid_blocks[to.x][to.y].blocks[to.z] == EMPTY_BLOCK_ID) {
            ;
            //Only need to increase usage if previously unused
            ++place_ctx.grid_blocks[to.x][to.y].usage;
        }
        place_ctx.grid_blocks[to.x][to.y].blocks[to.z] = blk;

    } // Finish updating clb for all blocks
}

//Moves the blocks in blocks_affected to their old locations
void revert_move_blocks(t_pl_blocks_to_be_moved& blocks_affected) {
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    // Swap the blocks back, nets not yet swapped they don't need to be changed
    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; ++iblk) {
        ClusterBlockId blk = blocks_affected.moved_blocks[iblk].block_num;

        t_pl_loc old = blocks_affected.moved_blocks[iblk].old_loc;

        place_ctx.block_locs[blk].loc = old;

        VTR_ASSERT_SAFE_MSG(place_ctx.grid_blocks[old.x][old.y].blocks[old.z] = blk, "Grid blocks should only have been updated if swap commited (not reverted)");
    }
}

//Clears the current move so a new move can be proposed
void clear_move_blocks(t_pl_blocks_to_be_moved& blocks_affected) {
    //Reset moved flags
    blocks_affected.moved_to.clear();
    blocks_affected.moved_from.clear();

    //For run-time we just reset num_moved_blocks to zero, but do not free the blocks_affected
    //array to avoid memory allocation
    blocks_affected.num_moved_blocks = 0;
}
