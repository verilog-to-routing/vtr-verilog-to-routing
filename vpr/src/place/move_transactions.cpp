#include "move_transactions.h"
#include "move_utils.h"

#include "globals.h"
#include "place_util.h"
#include "vtr_assert.h"

t_pl_blocks_to_be_moved::t_pl_blocks_to_be_moved(size_t max_blocks){
        moved_blocks.reserve(max_blocks);
}

size_t t_pl_blocks_to_be_moved::get_size_and_increment() {
    VTR_ASSERT_SAFE(moved_blocks.size() < moved_blocks.capacity());
    moved_blocks.resize(moved_blocks.size() + 1);
    return moved_blocks.size() - 1;
}

//Records that block 'blk' should be moved to the specified 'to' location
e_block_move_result t_pl_blocks_to_be_moved::record_block_move(ClusterBlockId blk,
                                                               t_pl_loc to,
                                                               const BlkLocRegistry& blk_loc_registry) {
    auto [to_it, to_success] = moved_to.emplace(to);
    if (!to_success) {
        log_move_abort("duplicate block move to location");
        return e_block_move_result::ABORT;
    }

    t_pl_loc from = blk_loc_registry.block_locs()[blk].loc;

    auto [_, from_success] = moved_from.emplace(from);
    if (!from_success) {
        moved_to.erase(to_it);
        log_move_abort("duplicate block move from location");
        return e_block_move_result::ABORT;
    }

    VTR_ASSERT_SAFE(to.sub_tile < int(blk_loc_registry.grid_blocks().num_blocks_at_location({to.x, to.y, to.layer})));

    // Sets up the blocks moved
    size_t imoved_blk = get_size_and_increment();
    moved_blocks[imoved_blk].block_num = blk;
    moved_blocks[imoved_blk].old_loc = from;
    moved_blocks[imoved_blk].new_loc = to;

    return e_block_move_result::VALID;
}

//Examines the currently proposed move and determine any empty locations
std::set<t_pl_loc> t_pl_blocks_to_be_moved::determine_locations_emptied_by_move() {
    std::set<t_pl_loc> moved_from_set;
    std::set<t_pl_loc> moved_to_set;

    for (const t_pl_moved_block& moved_block : moved_blocks) {
        //When a block is moved its old location becomes free
        moved_from_set.emplace(moved_block.old_loc);

        //But any block later moved to a position fills it
        moved_to_set.emplace(moved_block.new_loc);
    }

    std::set<t_pl_loc> empty_locs;
    std::set_difference(moved_from_set.begin(), moved_from_set.end(),
                        moved_to_set.begin(), moved_to_set.end(),
                        std::inserter(empty_locs, empty_locs.begin()));

    return empty_locs;
}

//Moves the blocks in blocks_affected to their new locations
void apply_move_blocks(const t_pl_blocks_to_be_moved& blocks_affected,
                       BlkLocRegistry& blk_loc_registry) {
    auto& device_ctx = g_vpr_ctx.device();

    //Swap the blocks, but don't swap the nets or update place_ctx.grid_blocks
    //yet since we don't know whether the swap will be accepted
    for (const t_pl_moved_block& moved_block : blocks_affected.moved_blocks) {
        ClusterBlockId blk = moved_block.block_num;

        const t_pl_loc& old_loc = moved_block.old_loc;
        const t_pl_loc& new_loc = moved_block.new_loc;

        // move the block to its new location
        blk_loc_registry.mutable_block_locs()[blk].loc = new_loc;

        // get physical tile type of the old location
        t_physical_tile_type_ptr old_type = device_ctx.grid.get_physical_type({old_loc.x,old_loc.y,old_loc.layer});
        // get physical tile type of the new location
        t_physical_tile_type_ptr new_type = device_ctx.grid.get_physical_type({new_loc.x,new_loc.y, new_loc.layer});

        //if physical tile type of old location does not equal physical tile type of new location, sync the new physical pins
        if (old_type != new_type) {
            place_sync_external_block_connections(blk, blk_loc_registry);
        }
    }
}

//Commits the blocks in blocks_affected to their new locations (updates inverse
//lookups via place_ctx.grid_blocks)
void commit_move_blocks(const t_pl_blocks_to_be_moved& blocks_affected,
                        GridBlock& grid_blocks) {

    /* Swap physical location */
    for (const t_pl_moved_block& moved_block : blocks_affected.moved_blocks) {
        ClusterBlockId blk = moved_block.block_num;

        const t_pl_loc& to = moved_block.new_loc;
        const t_pl_loc& from = moved_block.old_loc;

        //Remove from old location only if it hasn't already been updated by a previous block update
        if (grid_blocks.block_at_location(from) == blk) {
            grid_blocks.set_block_at_location(from, EMPTY_BLOCK_ID);
            grid_blocks.set_usage({from.x, from.y, from.layer},
                                  grid_blocks.get_usage({from.x, from.y, from.layer}) - 1);
        }

        //Add to new location
        if (grid_blocks.block_at_location(to) == EMPTY_BLOCK_ID) {
            //Only need to increase usage if previously unused
            grid_blocks.set_usage({to.x, to.y, to.layer},
                                  grid_blocks.get_usage({to.x, to.y, to.layer}) + 1);
        }
        grid_blocks.set_block_at_location(to, blk);

    } // Finish updating clb for all blocks
}

//Moves the blocks in blocks_affected to their old locations
void revert_move_blocks(const t_pl_blocks_to_be_moved& blocks_affected,
                        BlkLocRegistry& blk_loc_registry) {
    auto& device_ctx = g_vpr_ctx.device();

    // Swap the blocks back, nets not yet swapped they don't need to be changed
    for (const t_pl_moved_block& moved_block : blocks_affected.moved_blocks) {
        ClusterBlockId blk = moved_block.block_num;

        const t_pl_loc& old_loc = moved_block.old_loc;
        const t_pl_loc& new_loc = moved_block.new_loc;

        // return the block to where it was before the swap
        blk_loc_registry.mutable_block_locs()[blk].loc = old_loc;

        // get physical tile type of the old location
        t_physical_tile_type_ptr old_type = device_ctx.grid.get_physical_type({old_loc.x,old_loc.y,old_loc.layer});
        // get physical tile type of the new location
        t_physical_tile_type_ptr new_type = device_ctx.grid.get_physical_type({new_loc.x,new_loc.y, new_loc.layer});

        //if physical tile type of old location does not equal physical tile type of new location, sync the new physical pins
        if (old_type != new_type) {
            place_sync_external_block_connections(blk, blk_loc_registry);
        }

        VTR_ASSERT_SAFE_MSG(blk_loc_registry.grid_blocks().block_at_location(old_loc) == blk,
                            "Grid blocks should only have been updated if swap committed (not reverted)");
    }
}

//Clears the current move so a new move can be proposed
void t_pl_blocks_to_be_moved::clear_move_blocks() {
    //Reset moved flags
    moved_to.clear();
    moved_from.clear();

    //For run-time, we just reset size of blocks_affected.moved_blocks to zero, but do not free the blocks_affected
    //array to avoid memory allocation

    moved_blocks.resize(0);

    affected_pins.clear();
}
