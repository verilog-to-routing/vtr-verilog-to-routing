#include "move_utils.h"

#include "place_util.h"
#include "globals.h"

#include "vtr_random.h"

#include "draw_debug.h"
#include "draw.h"

#include "place_constraints.h"
#include "placer_globals.h"

//f_placer_breakpoint_reached is used to stop the placer when a breakpoint is reached. When this flag is true, it stops the placer after the current perturbation. Thus, when a breakpoint is reached, this flag is set to true.
//Note: The flag is only effective if compiled with VTR_ENABLE_DEBUG_LOGGING
bool f_placer_breakpoint_reached = false;

//Records counts of reasons for aborted moves
static std::map<std::string, size_t> f_move_abort_reasons;

void log_move_abort(const std::string& reason) {
    ++f_move_abort_reasons[reason];
}

void report_aborted_moves() {
    VTR_LOG("\n");
    VTR_LOG("Aborted Move Reasons:\n");
    if (f_move_abort_reasons.empty()) {
        VTR_LOG("  No moves aborted\n");
    }
    for (auto kv : f_move_abort_reasons) {
        VTR_LOG("  %s: %zu\n", kv.first.c_str(), kv.second);
    }
}

e_create_move create_move(t_pl_blocks_to_be_moved& blocks_affected, ClusterBlockId b_from, t_pl_loc to) {
    e_block_move_result outcome = find_affected_blocks(blocks_affected, b_from, to);

    if (outcome == e_block_move_result::INVERT) {
        //Try inverting the swap direction

        auto& place_ctx = g_vpr_ctx.placement();
        ClusterBlockId b_to = place_ctx.grid_blocks.block_at_location(to);

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
        ClusterBlockId b_to = place_ctx.grid_blocks.block_at_location(to);
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

    if (place_ctx.block_locs[b_from].is_fixed) {
        return e_block_move_result::ABORT;
    }

    VTR_ASSERT_SAFE(to.sub_tile < int(place_ctx.grid_blocks.num_blocks_at_location({to.x, to.y, to.layer})));

    ClusterBlockId b_to = place_ctx.grid_blocks.block_at_location(to);

    t_pl_loc curr_from = place_ctx.block_locs[b_from].loc;

    e_block_move_result outcome = e_block_move_result::VALID;

    // Check whether the to_location is empty
    if (b_to == EMPTY_BLOCK_ID) {
        // Sets up the blocks moved
        outcome = record_block_move(blocks_affected, b_from, to);

    } else if (b_to != INVALID_BLOCK_ID) {
        // Check whether block to is compatible with from location
        if (b_to != EMPTY_BLOCK_ID && b_to != INVALID_BLOCK_ID) {
            if (!(is_legal_swap_to_location(b_to, curr_from)) || place_ctx.block_locs[b_to].is_fixed) {
                return e_block_move_result::ABORT;
            }
        }

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
            ClusterBlockId b_to = place_ctx.grid_blocks.block_at_location(curr_to);
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
    //below the other (not a big limitation since swapping in the opposite direction
    //allows these blocks to swap)
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
        t_pl_loc curr_from = place_ctx.block_locs[b_from].loc;

        ClusterBlockId b_to = place_ctx.pl_macros[imacro_to].members[imember_to].blk_index;
        VTR_ASSERT_SAFE(curr_to == place_ctx.block_locs[b_to].loc);

        // Check whether block to is compatible with from location
        if (b_to != EMPTY_BLOCK_ID && b_to != INVALID_BLOCK_ID) {
            if (!(is_legal_swap_to_location(b_to, curr_from))) {
                return e_block_move_result::ABORT;
            }
        }

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

        ClusterBlockId blk_to = place_ctx.grid_blocks.block_at_location(to);

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

        ClusterBlockId blk_to = place_ctx.grid_blocks.block_at_location(to);

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
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    if (to.x < 0 || to.x >= int(device_ctx.grid.width())
        || to.y < 0 || to.y >= int(device_ctx.grid.height())) {
        return false;
    }

    auto physical_tile = device_ctx.grid.get_physical_type(t_physical_tile_loc(to.x, to.y, to.layer));
    auto logical_block = cluster_ctx.clb_nlist.block_type(blk);

    if (to.sub_tile < 0 || to.sub_tile >= physical_tile->capacity
        || !is_sub_tile_compatible(physical_tile, logical_block, to.sub_tile)) {
        return false;
    }
    // If the destination block is user constrained, abort this swap
    auto b_to = place_ctx.grid_blocks.block_at_location(to);
    if (b_to != INVALID_BLOCK_ID && b_to != EMPTY_BLOCK_ID) {
        if (place_ctx.block_locs[b_to].is_fixed) {
            return false;
        }
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

ClusterBlockId propose_block_to_move(int& logical_blk_type_index, bool highly_crit_block, ClusterNetId* net_from, int* pin_from) {
    ClusterBlockId b_from = ClusterBlockId::INVALID();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    if (logical_blk_type_index == -1) { //If the block type is unspecified, choose any random block to be swapped with another random block
        if (highly_crit_block) {
            b_from = pick_from_highly_critical_block(*net_from, *pin_from);
        } else {
            b_from = pick_from_block();
        }

        //if a movable block found, set the block type
        if (b_from) {
            logical_blk_type_index = cluster_ctx.clb_nlist.block_type(b_from)->index;
        }
    } else { //If the block type is specified, choose a random block with blk_type to be swapped with another random block
        if (highly_crit_block) {
            b_from = pick_from_highly_critical_block(*net_from, *pin_from, logical_blk_type_index);
        } else {
            b_from = pick_from_block(logical_blk_type_index);
        }
    }

    return b_from;
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

    //Keep selecting random blocks as long as there are any untried blocks
    //Can get slow if there are many blocks but only a few (or none) can move
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

//Pick a random block with a specific blk_type to be swapped with another random block.
//If none is found return ClusterBlockId::INVALID()
ClusterBlockId pick_from_block(const int logical_blk_type_index) {
    /* Some blocks may be fixed, and should never be moved from their *
     * initial positions. If we randomly selected such a block try    *
     * another random block.                                          *
     *                                                                *
     * We need to track the blocks we have tried to avoid an infinite *
     * loop if all blocks are fixed.                                  */
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    t_logical_block_type blk_type_temp;
    blk_type_temp.index = logical_blk_type_index;
    const auto& blocks_per_type = cluster_ctx.clb_nlist.blocks_per_type(blk_type_temp);

    //no blocks with this type is available
    if (blocks_per_type.empty()) {
        return ClusterBlockId::INVALID();
    }

    std::unordered_set<ClusterBlockId> tried_from_blocks;

    //Keep selecting random blocks as long as there are any untried blocks with type "blk_type"
    //Can get slow if there are many blocks but only a few (or none) can move
    while (tried_from_blocks.size() < blocks_per_type.size()) {
        //Pick a block at random
        ClusterBlockId b_from = ClusterBlockId(blocks_per_type[vtr::irand((int)blocks_per_type.size() - 1)]);
        //Record it as tried
        tried_from_blocks.insert(b_from);

        if (place_ctx.block_locs[b_from].is_fixed) {
            continue; //Fixed location, try again
        }
        //Found a movable block
        return b_from;
    }

    //No movable blocks found
    //Unreachable statement
    return ClusterBlockId::INVALID();
}

//Pick a random highly critical block to be swapped with another random block.
//If none is found return ClusterBlockId::INVALID()
ClusterBlockId pick_from_highly_critical_block(ClusterNetId& net_from, int& pin_from) {
    auto& place_move_ctx = g_placer_ctx.move();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    //Initialize critical net and pin to be invalid
    net_from = ClusterNetId::INVALID();
    pin_from = -1;

    //check if any critical block is available
    if (place_move_ctx.highly_crit_pins.empty()) {
        return ClusterBlockId::INVALID();
    }

    //pick a random highly critical pin and find the nets driver block
    std::pair<ClusterNetId, int> crit_pin = place_move_ctx.highly_crit_pins[vtr::irand(place_move_ctx.highly_crit_pins.size() - 1)];
    ClusterBlockId b_from = cluster_ctx.clb_nlist.net_driver_block(crit_pin.first);

    if (place_ctx.block_locs[b_from].is_fixed) {
        return ClusterBlockId::INVALID(); //Block is fixed, cannot move
    }

    net_from = crit_pin.first;
    pin_from = crit_pin.second;
    return b_from;

    //Unreachable statement
    return ClusterBlockId::INVALID();
}

//Pick a random highly critical block with a specified block type to be swapped with another random block.
//If none is found return ClusterBlockId::INVALID()
ClusterBlockId pick_from_highly_critical_block(ClusterNetId& net_from, int& pin_from, const int logical_blk_type_index) {
    auto& place_move_ctx = g_placer_ctx.move();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    //Initialize critical net and pin to be invalid
    net_from = ClusterNetId::INVALID();
    pin_from = -1;

    //check if any critical block is available
    if (place_move_ctx.highly_crit_pins.empty()) {
        return ClusterBlockId::INVALID();
    }

    //pick a random highly critical pin and find the nets driver block
    std::pair<ClusterNetId, int> crit_pin = place_move_ctx.highly_crit_pins[vtr::irand(place_move_ctx.highly_crit_pins.size() - 1)];
    ClusterBlockId b_from = cluster_ctx.clb_nlist.net_driver_block(crit_pin.first);

    //Check if picked block type matches with the blk_type specified, and it is not fixed
    //blk_type from propose move doesn't account for the EMPTY type
    auto b_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    if (b_from_type->index == logical_blk_type_index) {
        if (place_ctx.block_locs[b_from].is_fixed) {
            return ClusterBlockId::INVALID(); //Block is fixed, cannot move
        }

        net_from = crit_pin.first;
        pin_from = crit_pin.second;
        return b_from;
    }

    //No critical block with 'blk_type' found
    //Unreachable statement
    return ClusterBlockId::INVALID();
}

bool find_to_loc_uniform(t_logical_block_type_ptr type,
                         float rlim,
                         const t_pl_loc from,
                         t_pl_loc& to,
                         ClusterBlockId b_from) {
    //Finds a legal swap to location for the given type, starting from 'from.x' and 'from.y'
    //
    //Note that the range limit (rlim) is applied in a logical sense (i.e. 'compressed' grid space consisting
    //of the same block types, and not the physical grid space). This means, for example, that columns of 'rare'
    //blocks (e.g. DSPs/RAMs) which are physically far appart but logically adjacent will be swappable even
    //at an rlim fo 1.
    //
    //This ensures that such blocks don't get locked down too early during placement (as would be the
    //case with a physical distance rlim)

    //Retrieve the compressed block grid for this block type
    const auto& compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[type->index];
    const int num_layers = g_vpr_ctx.device().grid.get_num_layers();
    const int from_layer_num = from.layer;

    //Determine the coordinates in the compressed grid space of the current block
    std::vector<t_physical_tile_loc> compressed_locs = get_compressed_loc(compressed_block_grid,
                                                                          from,
                                                                          num_layers);

    //Determine the valid compressed grid location ranges
    std::vector<t_bb> search_range = get_compressed_grid_target_search_range(compressed_block_grid,
                                                                             compressed_locs,
                                                                             rlim,
                                                                             num_layers);
    int delta_cx = search_range[from_layer_num].xmax - search_range[from_layer_num].xmin;

    t_physical_tile_loc to_compressed_loc;
    bool legal = false;

    //TODO: constraints should be adapted to 3D architecture
    if (is_cluster_constrained(b_from)) {
        bool intersect = intersect_range_limit_with_floorplan_constraints(type,
                                                                          b_from,
                                                                          search_range[from_layer_num],
                                                                          delta_cx,
                                                                          from_layer_num);
        if (!intersect) {
            return false;
        }
    }
    //TODO: For now, we only move the blocks on the same tile
    legal = find_compatible_compressed_loc_in_range(type,
                                                    delta_cx,
                                                    compressed_locs[from_layer_num],
                                                    search_range[from_layer_num],
                                                    to_compressed_loc,
                                                    false,
                                                    from_layer_num);

    if (!legal) {
        //No valid position found
        return false;
    }

    VTR_ASSERT(to_compressed_loc);

    //Convert to true (uncompressed) grid locations
    compressed_grid_to_loc(type, to_compressed_loc, to);

    auto& grid = g_vpr_ctx.device().grid;
    const auto& to_type = grid.get_physical_type(t_physical_tile_loc(to.x, to.y, to.layer));

    VTR_ASSERT_MSG(is_tile_compatible(to_type, type), "Type must be compatible");
    VTR_ASSERT_MSG(grid.get_width_offset({to.x, to.y, to.layer}) == 0, "Should be at block base location");
    VTR_ASSERT_MSG(grid.get_height_offset({to.x, to.y, to.layer}) == 0, "Should be at block base location");

    return true;
}

//Accessor for f_placer_breakpoint_reached
bool placer_breakpoint_reached() {
    return f_placer_breakpoint_reached;
}

void set_placer_breakpoint_reached(bool flag) {
    f_placer_breakpoint_reached = flag;
}

bool find_to_loc_median(t_logical_block_type_ptr blk_type,
                        const t_pl_loc& from_loc,
                        const t_bb* limit_coords,
                        t_pl_loc& to_loc,
                        ClusterBlockId b_from) {
    int num_layers = g_vpr_ctx.device().grid.get_num_layers();
    int from_layer_num = from_loc.layer;
    const auto& compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[blk_type->index];

    //Determine the coordinates in the compressed grid space of the current block
    std::vector<t_physical_tile_loc> from_compressed_locs = get_compressed_loc(compressed_block_grid,
                                                                               from_loc,
                                                                               g_vpr_ctx.device().grid.get_num_layers());

    VTR_ASSERT(limit_coords->xmin <= limit_coords->xmax);
    VTR_ASSERT(limit_coords->ymin <= limit_coords->ymax);

    //Determine the valid compressed grid location ranges
    std::vector<t_physical_tile_loc> min_compressed_loc = get_compressed_loc_approx(compressed_block_grid,
                                                                                    {limit_coords->xmin, limit_coords->ymin, 0, from_layer_num},
                                                                                    num_layers);
    std::vector<t_physical_tile_loc> max_compressed_loc = get_compressed_loc_approx(compressed_block_grid,
                                                                                    {limit_coords->xmax, limit_coords->ymax, 0, from_layer_num},
                                                                                    num_layers);

    VTR_ASSERT(min_compressed_loc[from_layer_num].x >= 0);
    VTR_ASSERT(static_cast<int>(compressed_block_grid.get_num_columns(from_layer_num)) - 1 - max_compressed_loc[from_layer_num].x >= 0);
    VTR_ASSERT(max_compressed_loc[from_layer_num].x >= min_compressed_loc[from_layer_num].x);
    int delta_cx = max_compressed_loc[from_layer_num].x - min_compressed_loc[from_layer_num].x;

    VTR_ASSERT(min_compressed_loc[from_layer_num].y >= 0);
    VTR_ASSERT(static_cast<int>(compressed_block_grid.get_num_rows(from_layer_num)) - 1 - max_compressed_loc[from_layer_num].y >= 0);
    VTR_ASSERT(max_compressed_loc[from_layer_num].y >= min_compressed_loc[from_layer_num].y);

    t_bb search_range(min_compressed_loc[from_layer_num].x,
                      max_compressed_loc[from_layer_num].x,
                      min_compressed_loc[from_layer_num].y,
                      max_compressed_loc[from_layer_num].y);

    t_physical_tile_loc to_compressed_loc;
    bool legal = false;

    if (is_cluster_constrained(b_from)) {
        bool intersect = intersect_range_limit_with_floorplan_constraints(blk_type,
                                                                          b_from,
                                                                          search_range,
                                                                          delta_cx,
                                                                          from_layer_num);
        if (!intersect) {
            return false;
        }
    }

    legal = find_compatible_compressed_loc_in_range(blk_type,
                                                    delta_cx,
                                                    from_compressed_locs[from_layer_num],
                                                    search_range,
                                                    to_compressed_loc,
                                                    true,
                                                    from_layer_num);

    if (!legal) {
        //No valid position found
        return false;
    }

    VTR_ASSERT(to_compressed_loc);

    //Convert to true (uncompressed) grid locations
    compressed_grid_to_loc(blk_type, to_compressed_loc, to_loc);

    auto& grid = g_vpr_ctx.device().grid;
    const auto& to_type = grid.get_physical_type(t_physical_tile_loc(to_loc.x, to_loc.y, to_loc.layer));

    VTR_ASSERT_MSG(is_tile_compatible(to_type, blk_type), "Type must be compatible");
    VTR_ASSERT_MSG(grid.get_width_offset({to_loc.x, to_loc.y, to_loc.layer}) == 0, "Should be at block base location");
    VTR_ASSERT_MSG(grid.get_height_offset({to_loc.x, to_loc.y, to_loc.layer}) == 0, "Should be at block base location");

    return true;
}

bool find_to_loc_centroid(t_logical_block_type_ptr blk_type,
                          const t_pl_loc& from_loc,
                          const t_pl_loc& centroid,
                          const t_range_limiters& range_limiters,
                          t_pl_loc& to_loc,
                          ClusterBlockId b_from) {
    //Retrieve the compressed block grid for this block type
    const auto& compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[blk_type->index];
    const int from_layer_num = from_loc.layer;
    const int num_layers = g_vpr_ctx.device().grid.get_num_layers();

    std::vector<t_physical_tile_loc> from_compressed_loc = get_compressed_loc(compressed_block_grid,
                                                                              from_loc,
                                                                              num_layers);

    //Determine the coordinates in the compressed grid space of the current block
    std::vector<t_physical_tile_loc> centroid_compressed_loc = get_compressed_loc_approx(compressed_block_grid,
                                                                                         centroid,
                                                                                         num_layers);

    //Determine the valid compressed grid location ranges
    int delta_cx;
    std::vector<t_bb> search_range;

    // If we are early in the anneal and the range limit still big enough --> search around the center location that the move proposed
    // If not --> search around the current location of the block but in the direction of the center location that the move proposed
    if (range_limiters.original_rlim > 0.15 * range_limiters.first_rlim) {
        search_range = get_compressed_grid_target_search_range(compressed_block_grid,
                                                               centroid_compressed_loc,
                                                               std::min<float>(range_limiters.original_rlim, range_limiters.dm_rlim),
                                                               num_layers);
    } else {
        search_range = get_compressed_grid_bounded_search_range(compressed_block_grid,
                                                                from_compressed_loc,
                                                                centroid_compressed_loc,
                                                                std::min<float>(range_limiters.original_rlim, range_limiters.dm_rlim),
                                                                num_layers);
    }
    delta_cx = search_range[from_layer_num].xmax - search_range[from_layer_num].xmin;

    t_physical_tile_loc to_compressed_loc;
    bool legal = false;

    if (is_cluster_constrained(b_from)) {
        bool intersect = intersect_range_limit_with_floorplan_constraints(blk_type,
                                                                          b_from,
                                                                          search_range[from_layer_num],
                                                                          delta_cx,
                                                                          from_layer_num);
        if (!intersect) {
            return false;
        }
    }

    //TODO: For now, we only move the blocks on the same tile
    legal = find_compatible_compressed_loc_in_range(blk_type,
                                                    delta_cx,
                                                    from_compressed_loc[from_layer_num],
                                                    search_range[from_layer_num],
                                                    to_compressed_loc,
                                                    false,
                                                    from_layer_num);

    if (!legal) {
        //No valid position found
        return false;
    }

    VTR_ASSERT(to_compressed_loc);

    //Convert to true (uncompressed) grid locations
    compressed_grid_to_loc(blk_type, to_compressed_loc, to_loc);

    auto& grid = g_vpr_ctx.device().grid;
    const auto& to_type = grid.get_physical_type(t_physical_tile_loc(to_loc.x, to_loc.y, to_loc.layer));

    VTR_ASSERT_MSG(is_tile_compatible(to_type, blk_type), "Type must be compatible");
    VTR_ASSERT_MSG(grid.get_width_offset({to_loc.x, to_loc.y, to_loc.layer}) == 0, "Should be at block base location");
    VTR_ASSERT_MSG(grid.get_height_offset({to_loc.x, to_loc.y, to_loc.layer}) == 0, "Should be at block base location");

    return true;
}

//Array of move type strings
static const std::array<std::string, NUM_PL_MOVE_TYPES + 1> move_type_strings = {
    "Uniform",
    "Median",
    "W. Centroid",
    "Centroid",
    "W. Median",
    "Crit. Uniform",
    "Feasible Region",
    "Manual Move"};

//To convert enum move type to string
const std::string& move_type_to_string(e_move_type move) {
    return move_type_strings[int(move)];
}

//Convert to true (uncompressed) grid locations
void compressed_grid_to_loc(t_logical_block_type_ptr blk_type,
                            t_physical_tile_loc compressed_loc,
                            t_pl_loc& to_loc) {
    const auto& compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[blk_type->index];
    auto grid_loc = compressed_block_grid.compressed_loc_to_grid_loc(compressed_loc);

    auto& grid = g_vpr_ctx.device().grid;
    auto to_type = grid.get_physical_type({grid_loc.x, grid_loc.y, grid_loc.layer_num});

    //Each x/y location contains only a single type, so we can pick a random z (capcity) location
    auto& compatible_sub_tiles = compressed_block_grid.compatible_sub_tile_num(to_type->index);
    int sub_tile = compatible_sub_tiles[vtr::irand((int)compatible_sub_tiles.size() - 1)];

    to_loc = t_pl_loc(grid_loc.x, grid_loc.y, sub_tile, grid_loc.layer_num);
}

bool find_compatible_compressed_loc_in_range(t_logical_block_type_ptr type,
                                             const int delta_cx,
                                             const t_physical_tile_loc& from_loc,
                                             t_bb search_range,
                                             t_physical_tile_loc& to_loc,
                                             bool is_median,
                                             int to_layer_num) {
    //TODO For the time being, the blocks only moved in the same layer. This assertion should be removed after VPR is updated to move blocks between layers
    VTR_ASSERT(to_layer_num == from_loc.layer_num);
    const auto& compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[type->index];
    to_loc.layer_num = to_layer_num;
    std::unordered_set<int> tried_cx_to;
    bool legal = false;
    int possibilities;
    if (is_median)
        possibilities = delta_cx + 1;
    else
        possibilities = delta_cx;

    while (!legal && (int)tried_cx_to.size() < possibilities) { //Until legal or all possibilities exhaused
        //Pick a random x-location within [min_cx, max_cx],
        //until we find a legal swap, or have exhuasted all possiblites
        to_loc.x = search_range.xmin + vtr::irand(delta_cx);

        VTR_ASSERT(to_loc.x >= search_range.xmin);
        VTR_ASSERT(to_loc.x <= search_range.xmax);

        //Record this x location as tried
        auto res = tried_cx_to.insert(to_loc.x);
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
        const auto& block_rows = compressed_block_grid.get_column_block_map(to_loc.x, to_layer_num);
        auto y_lower_iter = block_rows.lower_bound(search_range.ymin);
        if (y_lower_iter == block_rows.end()) {
            continue;
        }

        auto y_upper_iter = block_rows.upper_bound(search_range.ymax);

        if (y_lower_iter->first > search_range.ymin) {
            //No valid blocks at this x location which are within rlim_y
            //
            if (type->index != 1)
                continue;
            else {
                //Fall back to allow the whole y range
                y_lower_iter = block_rows.begin();
                y_upper_iter = block_rows.end();

                search_range.ymin = y_lower_iter->first;
                search_range.ymax = (y_upper_iter - 1)->first;
            }
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
            to_loc.y = (y_lower_iter + dy)->first;

            VTR_ASSERT(to_loc.y >= search_range.ymin);
            VTR_ASSERT(to_loc.y <= search_range.ymax);

            if (from_loc.x == to_loc.x && from_loc.y == to_loc.y && from_loc.layer_num == to_layer_num) {
                continue; //Same from/to location -- try again for new y-position
            } else {
                legal = true;
            }
        }
    }
    return legal;
}

std::vector<t_physical_tile_loc> get_compressed_loc(const t_compressed_block_grid& compressed_block_grid,
                                                    t_pl_loc grid_loc,
                                                    int num_layers) {
    //TODO: This function currently only determine the compressed location for the same layer as grid_loc - it should be updated to cover all layers
    std::vector<t_physical_tile_loc> compressed_locs(num_layers);

    for (int layer_num = 0; layer_num < num_layers; ++layer_num) {
        if (layer_num != grid_loc.layer) {
            continue;
        }
        compressed_locs[layer_num] = compressed_block_grid.grid_loc_to_compressed_loc({grid_loc.x, grid_loc.y, layer_num});
    }

    return compressed_locs;
}

std::vector<t_physical_tile_loc> get_compressed_loc_approx(const t_compressed_block_grid& compressed_block_grid,
                                                           t_pl_loc grid_loc,
                                                           int num_layers) {
    //TODO: This function currently only determine the compressed location for the same layer as grid_loc - it should be updated to cover all layers
    std::vector<t_physical_tile_loc> compressed_locs(num_layers);

    for (int layer_num = 0; layer_num < num_layers; ++layer_num) {
        if (layer_num != grid_loc.layer) {
            continue;
        }
        compressed_locs[layer_num] = compressed_block_grid.grid_loc_to_compressed_loc_approx({grid_loc.x, grid_loc.y, layer_num});
    }

    return compressed_locs;
}

std::vector<t_bb> get_compressed_grid_target_search_range(const t_compressed_block_grid& compressed_block_grid,
                                                          const std::vector<t_physical_tile_loc>& compressed_locs,
                                                          float rlim,
                                                          int num_layers) {
    std::vector<t_bb> search_ranges(num_layers, t_bb());
    for (int layer_num = 0; layer_num < num_layers; ++layer_num) {
        const auto& layer_loc = compressed_locs[layer_num];
        //TODO: This if condition is added because blocks are only moved in the same layer. After the update, this condition should be replaced with an assertion
        if (layer_loc.x == OPEN || layer_loc.y == OPEN || layer_loc.layer_num == OPEN) {
            //No valid compressed location for this layer
            continue;
        }
        int rlim_x_max_range = std::min<int>((int)compressed_block_grid.get_num_columns(layer_num), rlim);
        int rlim_y_max_range = std::min<int>((int)compressed_block_grid.get_num_rows(layer_num), rlim); /* for aspect_ratio != 1 case. */

        search_ranges[layer_num].xmin = std::max(0, layer_loc.x - rlim_x_max_range);
        search_ranges[layer_num].xmax = std::min<int>(compressed_block_grid.get_num_columns(layer_num) - 1, layer_loc.x + rlim_x_max_range);

        search_ranges[layer_num].ymin = std::max(0, layer_loc.y - rlim_y_max_range);
        search_ranges[layer_num].ymax = std::min<int>(compressed_block_grid.get_num_rows(layer_num) - 1, layer_loc.y + rlim_y_max_range);
    }

    return search_ranges;
}

std::vector<t_bb> get_compressed_grid_bounded_search_range(const t_compressed_block_grid& compressed_block_grid,
                                                           const std::vector<t_physical_tile_loc>& from_compressed_loc,
                                                           const std::vector<t_physical_tile_loc>& target_compressed_loc,
                                                           float rlim,
                                                           int num_layers) {
    std::vector<t_bb> search_range(num_layers, t_bb());

    int min_cx, max_cx, min_cy, max_cy;

    for (int layer_num = 0; layer_num < num_layers; layer_num++) {
        //TODO: This if condition is added because blocks are only moved in the same layer. After the update, this condition should be replaced with an assertion
        if (from_compressed_loc[layer_num].x == OPEN || from_compressed_loc[layer_num].y == OPEN || from_compressed_loc[layer_num].layer_num == OPEN) {
            continue;
        }
        VTR_ASSERT(from_compressed_loc[layer_num].layer_num == layer_num);
        VTR_ASSERT(target_compressed_loc[layer_num].layer_num == layer_num);

        int rlim_x_max_range = std::min<int>(compressed_block_grid.get_num_columns(layer_num), rlim);
        int rlim_y_max_range = std::min<int>(compressed_block_grid.get_num_rows(layer_num), rlim); /* for aspect_ratio != 1 case. */

        int cx_from = from_compressed_loc[layer_num].x;
        int cy_from = from_compressed_loc[layer_num].y;
        if (cx_from == OPEN || cy_from == OPEN) {
            continue;
        }

        int cx_centroid = target_compressed_loc[layer_num].x;
        int cy_centroid = target_compressed_loc[layer_num].y;

        if (cx_centroid < cx_from) {
            min_cx = std::max(0, cx_from - rlim_x_max_range);
            max_cx = cx_from;
        } else {
            min_cx = cx_from;
            max_cx = std::min<int>(compressed_block_grid.get_num_columns(layer_num) - 1, cx_from + rlim_x_max_range);
        }
        if (cy_centroid < cy_from) {
            min_cy = std::max(0, cy_from - rlim_y_max_range);
            max_cy = cy_from;
        } else {
            min_cy = cy_from;
            max_cy = std::min<int>(compressed_block_grid.get_num_rows(layer_num) - 1, cy_from + rlim_y_max_range);
        }

        search_range[layer_num] = t_bb(min_cx, max_cx, min_cy, max_cy);
    }

    return search_range;
}

bool intersect_range_limit_with_floorplan_constraints(t_logical_block_type_ptr type,
                                                      ClusterBlockId b_from,
                                                      t_bb& search_range,
                                                      int& delta_cx,
                                                      int layer_num) {
    //Retrieve the compressed block grid for this block type
    const auto& compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[type->index];

    auto min_grid_loc = compressed_block_grid.compressed_loc_to_grid_loc({search_range.xmin,
                                                                          search_range.ymin,
                                                                          layer_num});

    auto max_grid_loc = compressed_block_grid.compressed_loc_to_grid_loc({search_range.xmax,
                                                                          search_range.ymax,
                                                                          layer_num});

    Region range_reg;
    range_reg.set_region_rect({min_grid_loc.x,
                               min_grid_loc.y,
                               max_grid_loc.x,
                               max_grid_loc.y,
                               layer_num});

    auto& floorplanning_ctx = g_vpr_ctx.floorplanning();

    PartitionRegion pr = floorplanning_ctx.cluster_constraints[b_from];
    std::vector<Region> regions;
    if (!pr.empty()) {
        regions = pr.get_partition_region();
    }
    Region intersect_reg;
    /*
     * If region size is greater than 1, the block is constrained to more than one rectangular region.
     * In this case, we return true (i.e. the range limit intersects with
     * the floorplan constraints) to simplify the problem. This simplification can be done because
     * this routine is done for cpu time optimization, so we do not have to necessarily check each
     * complicated case to get correct functionality during place moves.
     */
    if (regions.size() == 1) {
        intersect_reg = intersection(regions[0], range_reg);

        if (intersect_reg.empty()) {
            return false;
        } else {
            const auto intersect_coord = intersect_reg.get_region_rect();
            VTR_ASSERT(intersect_coord.layer_num == layer_num);
            auto min_compressed_loc = compressed_block_grid.grid_loc_to_compressed_loc_approx({intersect_coord.xmin,
                                                                                               intersect_coord.ymin,
                                                                                               layer_num});

            auto max_compressed_loc = compressed_block_grid.grid_loc_to_compressed_loc_approx({intersect_coord.xmax,
                                                                                               intersect_coord.ymax,
                                                                                               layer_num});
            delta_cx = max_compressed_loc.x - min_compressed_loc.x;
        }
    }

    return true;
}

std::string e_move_result_to_string(e_move_result move_outcome) {
    std::string move_result_to_string[] = {"Rejected", "Accepted", "Aborted"};
    return move_result_to_string[move_outcome];
}
