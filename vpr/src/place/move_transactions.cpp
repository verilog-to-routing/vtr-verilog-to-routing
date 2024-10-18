#include "move_transactions.h"
#include "move_utils.h"

#include "globals.h"
#include "grid_block.h"
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
        move_abortion_logger.log_move_abort("duplicate block move to location");
        return e_block_move_result::ABORT;
    }

    t_pl_loc from = blk_loc_registry.block_locs()[blk].loc;

    auto [_, from_success] = moved_from.emplace(from);
    if (!from_success) {
        moved_to.erase(to_it);
        move_abortion_logger.log_move_abort("duplicate block move from location");
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

bool t_pl_blocks_to_be_moved::driven_by_moved_block(const ClusterNetId net) const {
    auto& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    bool is_driven_by_move_blk = false;
    ClusterBlockId net_driver_block = clb_nlist.net_driver_block(net);

    for (const t_pl_moved_block& block : moved_blocks) {
        if (net_driver_block == block.block_num) {
            is_driven_by_move_blk = true;
            break;
        }
    }

    return is_driven_by_move_blk;
}

void MoveAbortionLogger::log_move_abort(std::string_view reason) {
    auto it = move_abort_reasons_.find(reason);
    if (it != move_abort_reasons_.end()) {
        it->second++;
    } else {
        move_abort_reasons_.emplace(reason, 1);
    }
}

void MoveAbortionLogger::report_aborted_moves() const {
    VTR_LOG("\n");
    VTR_LOG("Aborted Move Reasons:\n");
    if (move_abort_reasons_.empty()) {
        VTR_LOG("  No moves aborted\n");
    }
    for (const auto& kv : move_abort_reasons_) {
        VTR_LOG("  %s: %zu\n", kv.first.c_str(), kv.second);
    }
}
