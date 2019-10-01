#include "move_utils.h"

#include "place_util.h"
#include "globals.h"

#include "vtr_random.h"

//Records counts of reasons for aborted moves
static std::map<std::string, size_t> f_move_abort_reasons;

void log_move_abort(std::string reason) {
    ++f_move_abort_reasons[reason];
}

void report_aborted_moves() {
    VTR_LOG("\n");
    VTR_LOG("Aborted Move Reasons:\n");
    for (auto kv : f_move_abort_reasons) {
        VTR_LOG("  %s: %zu\n", kv.first.c_str(), kv.second);
    }
}

e_create_move create_move(t_pl_blocks_to_be_moved& blocks_affected, ClusterBlockId b_from, t_pl_loc to) {
    e_block_move_result outcome = find_affected_blocks(blocks_affected, b_from, to);

    if (outcome == e_block_move_result::INVERT) {
        //Try inverting the swap direction

        auto& place_ctx = g_vpr_ctx.placement();
        ClusterBlockId b_to = place_ctx.grid_blocks[to.x][to.y].blocks[to.z];

        if (!b_to) {
            log_move_abort("inverted move no to block");
            outcome = e_block_move_result::ABORT;
        } else {
            t_pl_loc from = place_ctx.block_locs[b_from].loc;

            outcome = find_affected_blocks(blocks_affected, b_to, from);

            if (outcome == e_block_move_result::INVERT) {
                log_move_abort("inverted move recurrsion");
                outcome = e_block_move_result::ABORT;
            }
        }
    }

    if (outcome == e_block_move_result::VALID
        || outcome == e_block_move_result::INVERT_VALID) {
        return e_create_move::VALID;
    } else {
        VTR_ASSERT_SAFE(outcome == e_block_move_result::ABORT);
        return e_create_move::ABORT;
    }
}

e_block_move_result find_affected_blocks(t_pl_blocks_to_be_moved& blocks_affected, ClusterBlockId b_from, t_pl_loc to) {
    /* Finds and set ups the affected_blocks array.
     * Returns abort_swap. */
    VTR_ASSERT_SAFE(b_from);

    int imacro_from;
    ClusterBlockId curr_b_from;
    e_block_move_result outcome = e_block_move_result::VALID;

    auto& place_ctx = g_vpr_ctx.placement();

    t_pl_loc from = place_ctx.block_locs[b_from].loc;

    auto& pl_macros = place_ctx.pl_macros;

    get_imacro_from_iblk(&imacro_from, b_from, pl_macros);
    if (imacro_from != -1) {
        // b_from is part of a macro, I need to swap the whole macro

        // Record down the relative position of the swap
        t_pl_offset swap_offset = to - from;

        int imember_from = 0;
        outcome = record_macro_swaps(blocks_affected, imacro_from, imember_from, swap_offset);

        VTR_ASSERT_SAFE(outcome != e_block_move_result::VALID || imember_from == int(pl_macros[imacro_from].members.size()));

    } else {
        ClusterBlockId b_to = place_ctx.grid_blocks[to.x][to.y].blocks[to.z];
        int imacro_to = -1;
        get_imacro_from_iblk(&imacro_to, b_to, pl_macros);

        if (imacro_to != -1) {
            //To block is a macro but from is a single block.
            //
            //Since we support swapping a macro as 'from' to a single 'to' block,
            //just invert the swap direction (which is equivalent)
            outcome = e_block_move_result::INVERT;
        } else {
            // This is not a macro - I could use the from and to info from before
            outcome = record_single_block_swap(blocks_affected, b_from, to);
        }

    } // Finish handling cases for blocks in macro and otherwise

    return outcome;
}

e_block_move_result record_single_block_swap(t_pl_blocks_to_be_moved& blocks_affected, ClusterBlockId b_from, t_pl_loc to) {
    /* Find all the blocks affected when b_from is swapped with b_to.
     * Returns abort_swap.                  */

    VTR_ASSERT_SAFE(b_from);

    auto& place_ctx = g_vpr_ctx.mutable_placement();

    VTR_ASSERT_SAFE(to.z < int(place_ctx.grid_blocks[to.x][to.y].blocks.size()));

    ClusterBlockId b_to = place_ctx.grid_blocks[to.x][to.y].blocks[to.z];

    e_block_move_result outcome = e_block_move_result::VALID;

    // Check whether the to_location is empty
    if (b_to == EMPTY_BLOCK_ID) {
        // Sets up the blocks moved
        outcome = record_block_move(blocks_affected, b_from, to);

    } else if (b_to != INVALID_BLOCK_ID) {
        // Sets up the blocks moved
        outcome = record_block_move(blocks_affected, b_from, to);

        if (outcome != e_block_move_result::VALID) {
            return outcome;
        }

        t_pl_loc from = place_ctx.block_locs[b_from].loc;
        outcome = record_block_move(blocks_affected, b_to, from);

    } // Finish swapping the blocks and setting up blocks_affected

    return outcome;
}

//Records all the block movements required to move the macro imacro_from starting at member imember_from
//to a new position offset from its current position by swap_offset. The new location may be a
//single (non-macro) block, or another macro.
e_block_move_result record_macro_swaps(t_pl_blocks_to_be_moved& blocks_affected, const int imacro_from, int& imember_from, t_pl_offset swap_offset) {
    auto& place_ctx = g_vpr_ctx.placement();
    auto& pl_macros = place_ctx.pl_macros;

    e_block_move_result outcome = e_block_move_result::VALID;

    for (; imember_from < int(pl_macros[imacro_from].members.size()) && outcome == e_block_move_result::VALID; imember_from++) {
        // Gets the new from and to info for every block in the macro
        // cannot use the old from and to info
        ClusterBlockId curr_b_from = pl_macros[imacro_from].members[imember_from].blk_index;

        t_pl_loc curr_from = place_ctx.block_locs[curr_b_from].loc;

        t_pl_loc curr_to = curr_from + swap_offset;

        //Make sure that the swap_to location is valid
        //It must be:
        // * on chip, and
        // * match the correct block type
        //
        //Note that we need to explicitly check that the types match, since the device floorplan is not
        //(neccessarily) translationally invariant for an arbitrary macro
        if (!is_legal_swap_to_location(curr_b_from, curr_to)) {
            log_move_abort("macro_from swap to location illegal");
            outcome = e_block_move_result::ABORT;
        } else {
            ClusterBlockId b_to = place_ctx.grid_blocks[curr_to.x][curr_to.y].blocks[curr_to.z];
            int imacro_to = -1;
            get_imacro_from_iblk(&imacro_to, b_to, pl_macros);

            if (imacro_to != -1) {
                //To block is a macro

                if (imacro_from == imacro_to) {
                    outcome = record_macro_self_swaps(blocks_affected, imacro_from, swap_offset);
                    imember_from = pl_macros[imacro_from].members.size();
                    break; //record_macro_self_swaps() handles this case completely, so we don't need to continue the loop
                } else {
                    outcome = record_macro_macro_swaps(blocks_affected, imacro_from, imember_from, imacro_to, b_to, swap_offset);
                    if (outcome == e_block_move_result::INVERT_VALID) {
                        break; //The move was inverted and successfully proposed, don't need to continue the loop
                    }
                    imember_from -= 1; //record_macro_macro_swaps() will have already advanced the original imember_from
                }
            } else {
                //To block is not a macro
                outcome = record_single_block_swap(blocks_affected, curr_b_from, curr_to);
            }
        }
    } // Finish going through all the blocks in the macro
    return outcome;
}

//Records all the block movements required to move the macro imacro_from starting at member imember_from
//to a new position offset from its current position by swap_offset. The new location must be where
//blk_to is located and blk_to must be part of imacro_to.
e_block_move_result record_macro_macro_swaps(t_pl_blocks_to_be_moved& blocks_affected, const int imacro_from, int& imember_from, const int imacro_to, ClusterBlockId blk_to, t_pl_offset swap_offset) {
    //Adds the macro imacro_to to the set of affected block caused by swapping 'blk_to' to it's
    //new position.
    //
    //This function is only called when both the main swap's from/to blocks are placement macros.
    //The position in the from macro ('imacro_from') is specified by 'imember_from', and the relevant
    //macro fro the to block is 'imacro_to'.

    auto& place_ctx = g_vpr_ctx.placement();

    //At the moment, we only support blk_to being the first element of the 'to' macro.
    //
    //For instance, this means that we can swap two carry chains so long as one starts
    //below the other (not a big limitation since swapping in the oppostie direction would
    //allow these blocks to swap)
    if (place_ctx.pl_macros[imacro_to].members[0].blk_index != blk_to) {
        int imember_to = 0;
        auto outcome = record_macro_swaps(blocks_affected, imacro_to, imember_to, -swap_offset);
        if (outcome == e_block_move_result::INVERT) {
            log_move_abort("invert recursion2");
            outcome = e_block_move_result::ABORT;
        } else if (outcome == e_block_move_result::VALID) {
            outcome = e_block_move_result::INVERT_VALID;
        }
        return outcome;
    }

    //From/To blocks should be exactly the swap offset appart
    ClusterBlockId blk_from = place_ctx.pl_macros[imacro_from].members[imember_from].blk_index;
    VTR_ASSERT_SAFE(place_ctx.block_locs[blk_from].loc + swap_offset == place_ctx.block_locs[blk_to].loc);

    //Continue walking along the overlapping parts of the from and to macros, recording
    //each block swap.
    //
    //At the momemnt we only support swapping the two macros if they have the same shape.
    //This will be the case with the common cases we care about (i.e. carry-chains), so
    //we just abort in any other cases (if these types of macros become more common in
    //the future this could be updated).
    //
    //Unless the two macros have thier root blocks aligned (i.e. the mutual overlap starts
    //at imember_from == 0), then theree will be a fixed offset between the macros' relative
    //position. We record this as from_to_macro_*_offset which is used to verify the shape
    //of the macros is consistent.
    //
    //NOTE: We mutate imember_from so the outer from macro walking loop moves in lock-step
    int imember_to = 0;
    t_pl_offset from_to_macro_offset = place_ctx.pl_macros[imacro_from].members[imember_from].offset;
    for (; imember_from < int(place_ctx.pl_macros[imacro_from].members.size()) && imember_to < int(place_ctx.pl_macros[imacro_to].members.size());
         ++imember_from, ++imember_to) {
        //Check that both macros have the same shape while they overlap
        if (place_ctx.pl_macros[imacro_from].members[imember_from].offset != place_ctx.pl_macros[imacro_to].members[imember_to].offset + from_to_macro_offset) {
            log_move_abort("macro shapes disagree");
            return e_block_move_result::ABORT;
        }

        ClusterBlockId b_from = place_ctx.pl_macros[imacro_from].members[imember_from].blk_index;

        t_pl_loc curr_to = place_ctx.block_locs[b_from].loc + swap_offset;

        ClusterBlockId b_to = place_ctx.pl_macros[imacro_to].members[imember_to].blk_index;
        VTR_ASSERT_SAFE(curr_to == place_ctx.block_locs[b_to].loc);

        if (!is_legal_swap_to_location(b_from, curr_to)) {
            log_move_abort("macro_from swap to location illegal");
            return e_block_move_result::ABORT;
        }

        auto outcome = record_single_block_swap(blocks_affected, b_from, curr_to);
        if (outcome != e_block_move_result::VALID) {
            return outcome;
        }
    }

    if (imember_to < int(place_ctx.pl_macros[imacro_to].members.size())) {
        //The to macro extends beyond the from macro.
        //
        //Swap the remainder of the 'to' macro to locations after the 'from' macro.
        //Note that we are swapping in the opposite direction so the swap offsets are inverted.
        return record_macro_swaps(blocks_affected, imacro_to, imember_to, -swap_offset);
    }

    return e_block_move_result::VALID;
}

//Moves the macro imacro by the specified offset
//
//Records the block movements in block_moves, the other blocks displaced in displaced_blocks,
//and any generated empty locations in empty_locations.
//
//This function moves a single macro and does not check for overlap with other macros!
e_block_move_result record_macro_move(t_pl_blocks_to_be_moved& blocks_affected,
                                      std::vector<ClusterBlockId>& displaced_blocks,
                                      const int imacro,
                                      t_pl_offset swap_offset) {
    auto& place_ctx = g_vpr_ctx.placement();

    for (const t_pl_macro_member& member : place_ctx.pl_macros[imacro].members) {
        t_pl_loc from = place_ctx.block_locs[member.blk_index].loc;

        t_pl_loc to = from + swap_offset;

        if (!is_legal_swap_to_location(member.blk_index, to)) {
            log_move_abort("macro move to location illegal");
            return e_block_move_result::ABORT;
        }

        ClusterBlockId blk_to = place_ctx.grid_blocks[to.x][to.y].blocks[to.z];

        record_block_move(blocks_affected, member.blk_index, to);

        int imacro_to = -1;
        get_imacro_from_iblk(&imacro_to, blk_to, place_ctx.pl_macros);
        if (blk_to && imacro_to != imacro) { //Block displaced only if exists and not part of current macro
            displaced_blocks.push_back(blk_to);
        }
    }
    return e_block_move_result::VALID;
}

//Returns the set of macros affected by moving imacro by the specified offset
//
//The resulting 'macros' may contain duplicates
e_block_move_result identify_macro_self_swap_affected_macros(std::vector<int>& macros, const int imacro, t_pl_offset swap_offset) {
    e_block_move_result outcome = e_block_move_result::VALID;
    auto& place_ctx = g_vpr_ctx.placement();

    for (size_t imember = 0; imember < place_ctx.pl_macros[imacro].members.size() && outcome == e_block_move_result::VALID; ++imember) {
        ClusterBlockId blk = place_ctx.pl_macros[imacro].members[imember].blk_index;

        t_pl_loc from = place_ctx.block_locs[blk].loc;
        t_pl_loc to = from + swap_offset;

        if (!is_legal_swap_to_location(blk, to)) {
            log_move_abort("macro move to location illegal");
            return e_block_move_result::ABORT;
        }

        ClusterBlockId blk_to = place_ctx.grid_blocks[to.x][to.y].blocks[to.z];

        int imacro_to = -1;
        get_imacro_from_iblk(&imacro_to, blk_to, place_ctx.pl_macros);

        if (imacro_to != -1) {
            auto itr = std::find(macros.begin(), macros.end(), imacro_to);
            if (itr == macros.end()) {
                macros.push_back(imacro_to);
                outcome = identify_macro_self_swap_affected_macros(macros, imacro_to, swap_offset);
            }
        }
    }
    return e_block_move_result::VALID;
}

e_block_move_result record_macro_self_swaps(t_pl_blocks_to_be_moved& blocks_affected,
                                            const int imacro,
                                            t_pl_offset swap_offset) {
    auto& place_ctx = g_vpr_ctx.placement();

    //Reset any partial move
    clear_move_blocks(blocks_affected);

    //Collect the macros affected
    std::vector<int> affected_macros;
    auto outcome = identify_macro_self_swap_affected_macros(affected_macros, imacro,
                                                            swap_offset);

    if (outcome != e_block_move_result::VALID) {
        return outcome;
    }

    //Remove any duplicate macros
    affected_macros.resize(std::distance(affected_macros.begin(), std::unique(affected_macros.begin(), affected_macros.end())));

    std::vector<ClusterBlockId> displaced_blocks;

    //Move all the affected macros by the offset
    for (int imacro_affected : affected_macros) {
        outcome = record_macro_move(blocks_affected, displaced_blocks, imacro_affected, swap_offset);

        if (outcome != e_block_move_result::VALID) {
            return outcome;
        }
    }

    auto is_non_macro_block = [&](ClusterBlockId blk) {
        int imacro_blk = -1;
        get_imacro_from_iblk(&imacro_blk, blk, place_ctx.pl_macros);

        if (std::find(affected_macros.begin(), affected_macros.end(), imacro_blk) != affected_macros.end()) {
            return false;
        }
        return true;
    };

    std::vector<ClusterBlockId> non_macro_displaced_blocks;
    std::copy_if(displaced_blocks.begin(), displaced_blocks.end(), std::back_inserter(non_macro_displaced_blocks), is_non_macro_block);

    //Based on the currently queued block moves, find the empty 'holes' left behind
    auto empty_locs = determine_locations_emptied_by_move(blocks_affected);

    VTR_ASSERT_SAFE(empty_locs.size() >= non_macro_displaced_blocks.size());

    //Fit the displaced blocks into the empty locations
    auto loc_itr = empty_locs.begin();
    for (auto blk : non_macro_displaced_blocks) {
        outcome = record_block_move(blocks_affected, blk, *loc_itr);
        ++loc_itr;
    }

    return outcome;
}

bool is_legal_swap_to_location(ClusterBlockId blk, t_pl_loc to) {
    //Make sure that the swap_to location is valid
    //It must be:
    // * on chip, and
    // * match the correct block type
    //
    //Note that we need to explicitly check that the types match, since the device floorplan is not
    //(neccessarily) translationally invariant for an arbitrary macro

    auto& device_ctx = g_vpr_ctx.device();

    if (to.x < 0 || to.x >= int(device_ctx.grid.width())
        || to.y < 0 || to.y >= int(device_ctx.grid.height())
        || to.z < 0 || to.z >= device_ctx.grid[to.x][to.y].type->capacity
        || (device_ctx.grid[to.x][to.y].type != physical_tile_type(blk))) {
        return false;
    }
    return true;
}

//Examines the currently proposed move and determine any empty locations
std::set<t_pl_loc> determine_locations_emptied_by_move(t_pl_blocks_to_be_moved& blocks_affected) {
    std::set<t_pl_loc> moved_from;
    std::set<t_pl_loc> moved_to;

    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; ++iblk) {
        //When a block is moved it's old location becomes free
        moved_from.emplace(blocks_affected.moved_blocks[iblk].old_loc);

        //But any block later moved to a position fills it
        moved_to.emplace(blocks_affected.moved_blocks[iblk].new_loc);
    }

    std::set<t_pl_loc> empty_locs;
    std::set_difference(moved_from.begin(), moved_from.end(),
                        moved_to.begin(), moved_to.end(),
                        std::inserter(empty_locs, empty_locs.begin()));

    return empty_locs;
}

//Pick a random block to be swapped with another random block.
//If none is found return ClusterBlockId::INVALID()
ClusterBlockId pick_from_block() {
    /* Some blocks may be fixed, and should never be moved from their *
     * initial positions. If we randomly selected such a block try    *
     * another random block.                                          *
     *                                                                *
     * We need to track the blocks we have tried to avoid an infinite *
     * loop if all blocks are fixed.                                  */
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    std::unordered_set<ClusterBlockId> tried_from_blocks;

    //So long as untried blocks remain
    while (tried_from_blocks.size() < cluster_ctx.clb_nlist.blocks().size()) {
        //Pick a block at random
        ClusterBlockId b_from = ClusterBlockId(vtr::irand((int)cluster_ctx.clb_nlist.blocks().size() - 1));

        //Record it as tried
        tried_from_blocks.insert(b_from);

        if (place_ctx.block_locs[b_from].is_fixed) {
            continue; //Fixed location, try again
        }

        //Found a movable block
        return b_from;
    }

    //No movable blocks found
    return ClusterBlockId::INVALID();
}

bool find_to_loc_uniform(t_physical_tile_type_ptr type,
                         float rlim,
                         const t_pl_loc from,
                         t_pl_loc& to) {
    //Finds a legal swap to location for the given type, starting from 'from.x' and 'from.y'
    //
    //Note that the range limit (rlim) is applied in a logical sense (i.e. 'compressed' grid space consisting
    //of the same block types, and not the physical grid space). This means, for example, that columns of 'rare'
    //blocks (e.g. DSPs/RAMs) which are physically far appart but logically adjacent will be swappable even
    //at an rlim fo 1.
    //
    //This ensures that such blocks don't get locked down too early during placement (as would be the
    //case with a physical distance rlim)
    auto& grid = g_vpr_ctx.device().grid;

    auto from_type = grid[from.x][from.y].type;

    //Retrieve the compressed block grid for this block type
    const auto& to_compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[type->index];
    const auto& from_compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[from_type->index];

    //Determine the rlim in each dimension
    int rlim_x = std::min<int>(to_compressed_block_grid.compressed_to_grid_x.size(), rlim);
    int rlim_y = std::min<int>(to_compressed_block_grid.compressed_to_grid_y.size(), rlim); /* for aspect_ratio != 1 case. */

    //Determine the coordinates in the compressed grid space of the current block
    int cx_from = grid_to_compressed(from_compressed_block_grid.compressed_to_grid_x, from.x);
    int cy_from = grid_to_compressed(from_compressed_block_grid.compressed_to_grid_y, from.y);

    //Determine the valid compressed grid location ranges
    int min_cx = std::max(0, cx_from - rlim_x);
    int max_cx = std::min<int>(to_compressed_block_grid.compressed_to_grid_x.size() - 1, cx_from + rlim_x);
    int delta_cx = max_cx - min_cx;

    int min_cy = std::max(0, cy_from - rlim_y);
    int max_cy = std::min<int>(to_compressed_block_grid.compressed_to_grid_y.size() - 1, cy_from + rlim_y);

    int cx_to = OPEN;
    int cy_to = OPEN;
    std::unordered_set<int> tried_cx_to;
    bool legal = false;
    while (!legal && (int)tried_cx_to.size() < delta_cx) { //Until legal or all possibilities exhaused
        //Pick a random x-location within [min_cx, max_cx],
        //until we find a legal swap, or have exhuasted all possiblites
        cx_to = min_cx + vtr::irand(delta_cx);

        VTR_ASSERT(cx_to >= min_cx);
        VTR_ASSERT(cx_to <= max_cx);

        //Record this x location as tried
        auto res = tried_cx_to.insert(cx_to);
        if (!res.second) {
            continue; //Already tried this position
        }

        //Pick a random y location
        //
        //We are careful here to consider that there may be a sparse
        //set of candidate blocks in the y-axis at this x location.
        //
        //The candidates are stored in a flat_map so we can efficiently find the set of valid
        //candidates with upper/lower bound.
        auto y_lower_iter = to_compressed_block_grid.grid[cx_to].lower_bound(min_cy);
        if (y_lower_iter == to_compressed_block_grid.grid[cx_to].end()) {
            continue;
        }

        auto y_upper_iter = to_compressed_block_grid.grid[cx_to].upper_bound(max_cy);

        if (y_lower_iter->first > min_cy) {
            //No valid blocks at this x location which are within rlim_y
            //
            //Fall back to allow the whole y range
            y_lower_iter = to_compressed_block_grid.grid[cx_to].begin();
            y_upper_iter = to_compressed_block_grid.grid[cx_to].end();

            min_cy = y_lower_iter->first;
            max_cy = (y_upper_iter - 1)->first;
        }

        int y_range = std::distance(y_lower_iter, y_upper_iter);
        VTR_ASSERT(y_range >= 0);

        //At this point we know y_lower_iter and y_upper_iter
        //bound the range of valid blocks at this x-location, which
        //are within rlim_y
        std::unordered_set<int> tried_dy;
        while (!legal && (int)tried_dy.size() < y_range) { //Until legal or all possibilities exhausted
            //Randomly pick a y location
            int dy = vtr::irand(y_range - 1);

            //Record this y location as tried
            auto res2 = tried_dy.insert(dy);
            if (!res2.second) {
                continue; //Already tried this position
            }

            //Key in the y-dimension is the compressed index location
            cy_to = (y_lower_iter + dy)->first;

            VTR_ASSERT(cy_to >= min_cy);
            VTR_ASSERT(cy_to <= max_cy);

            if (cx_from == cx_to && cy_from == cy_to) {
                continue; //Same from/to location -- try again for new y-position
            } else {
                legal = true;
            }
        }
    }

    if (!legal) {
        //No valid position found
        return false;
    }

    VTR_ASSERT(cx_to != OPEN);
    VTR_ASSERT(cy_to != OPEN);

    //Convert to true (uncompressed) grid locations
    to.x = to_compressed_block_grid.compressed_to_grid_x[cx_to];
    to.y = to_compressed_block_grid.compressed_to_grid_y[cy_to];

    //Each x/y location contains only a single type, so we can pick a random
    //z (capcity) location
    to.z = vtr::irand(type->capacity - 1);

    VTR_ASSERT_MSG(grid[to.x][to.y].type == type, "Type must match");
    VTR_ASSERT_MSG(grid[to.x][to.y].width_offset == 0, "Should be at block base location");
    VTR_ASSERT_MSG(grid[to.x][to.y].height_offset == 0, "Should be at block base location");

    return true;
}
