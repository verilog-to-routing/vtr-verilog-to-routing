#include "move_transactions.h"
#include "move_utils.h"

#include "globals.h"
#include "grid_block.h"
#include "vtr_assert.h"

#include <algorithm>

t_pl_blocks_to_be_moved::t_pl_blocks_to_be_moved(size_t max_blocks) {
    moved_blocks.reserve(max_blocks);
    moved_from.reserve(max_blocks);
    moved_to.reserve(max_blocks);
}

size_t t_pl_blocks_to_be_moved::get_size_and_increment() {
    VTR_ASSERT_SAFE(moved_blocks.size() < moved_blocks.capacity());
    moved_blocks.resize(moved_blocks.size() + 1);
    return moved_blocks.size() - 1;
}

e_block_move_result t_pl_blocks_to_be_moved::record_block_move(ClusterBlockId blk,
                                                               t_pl_loc to,
                                                               const BlkLocRegistry& blk_loc_registry) {
    if (std::ranges::find(moved_to, to) != moved_to.end()) {
        move_abortion_logger.log_move_abort("duplicate block move to location");
        return e_block_move_result::ABORT;
    }

    t_pl_loc from = blk_loc_registry.block_locs()[blk].loc;

    if (std::ranges::find(moved_from, from) != moved_from.end()) {
        move_abortion_logger.log_move_abort("duplicate block move from location");
        return e_block_move_result::ABORT;
    }

    moved_to.push_back(to);
    moved_from.push_back(from);

    VTR_ASSERT_SAFE(to.sub_tile < int(blk_loc_registry.grid_blocks().num_blocks_at_location({to.x, to.y, to.layer})));

    // Sets up the blocks moved
    size_t imoved_blk = get_size_and_increment();
    moved_blocks[imoved_blk].block_num = blk;
    moved_blocks[imoved_blk].old_loc = from;
    moved_blocks[imoved_blk].new_loc = to;

    return e_block_move_result::VALID;
}

std::set<t_pl_loc> t_pl_blocks_to_be_moved::determine_locations_emptied_by_move() const {
    std::set<t_pl_loc> moved_from_set;
    std::set<t_pl_loc> moved_to_set;

    for (const t_pl_moved_block& moved_block : moved_blocks) {
        // When a block is moved its old location becomes free
        moved_from_set.emplace(moved_block.old_loc);

        // But any block later moved to a position fills it
        moved_to_set.emplace(moved_block.new_loc);
    }

    std::set<t_pl_loc> empty_locs;
    std::ranges::set_difference(moved_from_set, moved_to_set,
                                std::inserter(empty_locs, empty_locs.begin()),
                                std::less{});

    return empty_locs;
}

void t_pl_blocks_to_be_moved::clear_move_blocks() {
    // Reset moved locations
    moved_to.clear();
    moved_from.clear();

    // For run-time, we just reset the size of moved_blocks to zero, but do not free
    // the array to avoid memory allocation

    moved_blocks.resize(0);

    affected_pins.clear();
}

bool t_pl_blocks_to_be_moved::driven_by_moved_block(const ClusterNetId net) const {
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    ClusterBlockId net_driver_block = clb_nlist.net_driver_block(net);

    auto it = std::ranges::find(moved_blocks, net_driver_block, &t_pl_moved_block::block_num);
    return it != moved_blocks.end();
}

void MoveAbortionLogger::log_move_abort(std::string_view reason) {
    auto it = move_abort_reasons_.find(reason);
    if (it != move_abort_reasons_.end()) {
        it->second++;
    } else {
        move_abort_reasons_.emplace(reason, 1);
    }
}

void MoveAbortionLogger::log_macro_move_proposal(bool user_defined, bool aborted) {
    if (user_defined) {
        user_macro_proposals_++;
        user_macro_aborts_ += aborted;
    } else {
        arch_macro_proposals_++;
        arch_macro_aborts_ += aborted;
    }
}

void MoveAbortionLogger::report_aborted_moves() const {
    VTR_LOG("\n");
    VTR_LOG("Aborted Move Reasons:\n");
    if (move_abort_reasons_.empty()) {
        VTR_LOG("  No moves aborted\n");
    }
    for (const auto& [reason, count] : move_abort_reasons_) {
        VTR_LOG("  %s: %zu\n", reason.c_str(), count);
    }

    // Only printed when user relative placement macros were proposed
    if (user_macro_proposals_ != 0) {
        VTR_LOG("\n");
        VTR_LOG("Macro Move Proposals:\n");
        if (arch_macro_proposals_ != 0) {
            VTR_LOG("  arch macros: %zu proposed, %zu aborted (%.2f%%)\n",
                    arch_macro_proposals_, arch_macro_aborts_,
                    100.0f * arch_macro_aborts_ / arch_macro_proposals_);
        }
        if (user_macro_proposals_ != 0) {
            VTR_LOG("  user relative placement macros: %zu proposed, %zu aborted (%.2f%%)\n",
                    user_macro_proposals_, user_macro_aborts_,
                    100.0f * user_macro_aborts_ / user_macro_proposals_);
        }
    }
}
