#include "swap_evaluator.h"

#include "globals.h"
#include "interposer_cost_handler.h"
#include "move_transactions.h"
#include "placer_state.h"
#include "vpr_types.h"

SwapEvaluator::SwapEvaluator(const t_placer_opts& placer_opts,
                             const t_placer_costs& costs,
                             PlacerState& placer_state,
                             NetCostHandler& net_cost_handler,
                             std::optional<InterposerCostHandler>& interposer_cost_handler,
                             const PlaceDelayModel* delay_model,
                             const PlacerCriticalities* criticalities)
    : placer_opts_(placer_opts)
    , costs_(costs)
    , placer_state_(placer_state)
    , net_cost_handler_(net_cost_handler)
    , interposer_cost_handler_(interposer_cost_handler)
    , delay_model_(delay_model)
    , criticalities_(criticalities) {}

t_swap_cost_deltas SwapEvaluator::apply_and_evaluate(t_pl_blocks_to_be_moved& blocks_affected,
                                                     const t_place_algorithm& place_algorithm,
                                                     const std::function<bool()>& should_cancel) {
    t_swap_cost_deltas deltas;

    // To make evaluating the move simpler (e.g. calculating changed bounding box),
    // we first move the blocks to their new locations (apply the move to
    // blk_loc_registry.block_locs) and then compute the change in cost. If the move
    // is accepted, the inverse look-up in blk_loc_registry.grid_blocks is updated
    // (committing the move). If the move is rejected, the blocks are returned to
    // their original positions (reverting blk_loc_registry.block_locs to its original state).
    //
    // Note that the inverse look-up blk_loc_registry.grid_blocks is only updated after
    // move acceptance is determined, so it should not be used when evaluating a move.

    // Update the block positions
    placer_state_.mutable_blk_loc_registry().apply_move_blocks(blocks_affected);

    // Find all the nets affected by this swap and update their wiring costs.
    // This cost value doesn't depend on the timing info.
    //
    // Also find all the pins affected by the swap, and calculates new connection
    // delays and timing costs.
    bool completed = net_cost_handler_.find_affected_nets_and_update_costs(delay_model_, criticalities_, blocks_affected,
                                                                           deltas.cost_terms_delta,
                                                                           deltas.timing_delta_c,
                                                                           should_cancel);
    if (!completed) {
        // The caller no longer needs this evaluation. The partially staged
        // scratch is consistent, so revert() restores the state; the deltas
        // computed so far are meaningless.
        deltas.cancelled = true;
        return deltas;
    }

    deltas.update_interposer_costs = interposer_cost_handler_.has_value() && interposer_cost_handler_->has_active_cost_terms();
    if (deltas.update_interposer_costs) {
        const auto [interposer_cost_delta, interposer_cong_cost_delta] = interposer_cost_handler_->total_proposed_cost(net_cost_handler_.affected_nets());
        deltas.cost_terms_delta.interposer_cost = interposer_cost_delta;
        deltas.cost_terms_delta.interposer_cong_cost = interposer_cong_cost_delta;
    }

    if (place_algorithm == e_place_algorithm::CRITICALITY_TIMING_PLACE) {
        // Take delta_c as a combination of timing and wiring cost. In
        // addition to `timing_tradeoff`, we normalize the cost values.
        // CRITICALITY_TIMING_PLACE algorithm works with somewhat stale
        // timing information to save CPU time.
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug,
                       "\t\tMove bb_delta_c %e, bb_cost_norm %e, timing_tradeoff %f, "
                       "timing_delta_c %e, timing_cost_norm %e\n",
                       deltas.cost_terms_delta.bb_cost,
                       costs_.bb_cost_norm,
                       placer_opts_.timing_tradeoff,
                       deltas.timing_delta_c,
                       costs_.timing_cost_norm);
        deltas.delta_c = (1 - placer_opts_.timing_tradeoff) * deltas.cost_terms_delta.bb_cost * costs_.bb_cost_norm
                         + placer_opts_.timing_tradeoff * deltas.timing_delta_c * costs_.timing_cost_norm
                         + placer_opts_.congestion_factor * deltas.cost_terms_delta.cong_cost * costs_.congestion_cost_norm;
        if (deltas.update_interposer_costs) {
            deltas.delta_c += placer_opts_.interposer_cost_params.net_cost_factor * deltas.cost_terms_delta.interposer_cost * costs_.interposer_cost_norm
                              + placer_opts_.interposer_cost_params.cong_cost_factor * deltas.cost_terms_delta.interposer_cong_cost * costs_.interposer_cong_cost_norm;
        }
    } else if (place_algorithm == e_place_algorithm::SLACK_TIMING_PLACE) {
        // delta_c is derived from a setup slack analysis performed by the caller
        // (see PlacementAnnealer::try_swap_()); only the component deltas are
        // computed here.
    } else {
        VTR_ASSERT_SAFE(place_algorithm == e_place_algorithm::BOUNDING_BOX_PLACE);
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug,
                       "\t\tMove bb_delta_c %e, bb_cost_norm %e\n",
                       deltas.cost_terms_delta.bb_cost,
                       costs_.bb_cost_norm);
        deltas.delta_c = deltas.cost_terms_delta.bb_cost * costs_.bb_cost_norm;
        if (deltas.update_interposer_costs) {
            deltas.delta_c += placer_opts_.interposer_cost_params.net_cost_factor * deltas.cost_terms_delta.interposer_cost * costs_.interposer_cost_norm
                              + placer_opts_.interposer_cost_params.cong_cost_factor * deltas.cost_terms_delta.interposer_cong_cost * costs_.interposer_cong_cost_norm;
        }
    }

    return deltas;
}

void SwapEvaluator::commit(t_pl_blocks_to_be_moved& blocks_affected,
                           bool update_interposer_costs,
                           bool commit_td) {
    if (commit_td) {
        // Update the connection_timing_cost and connection_delay
        // values from the temporary values.
        placer_state_.mutable_timing().commit_td_cost(blocks_affected);
    }

    // Update net cost functions and reset flags.
    net_cost_handler_.update_move_nets();
    if (update_interposer_costs) {
        interposer_cost_handler_->commit_costs(net_cost_handler_.affected_nets());
    }

    // Update clb data structures since we kept the move.
    placer_state_.mutable_blk_loc_registry().commit_move_blocks(blocks_affected);
}

void SwapEvaluator::extract_commit_record(const t_pl_blocks_to_be_moved& blocks_affected,
                                          t_swap_commit_record& record) const {
    net_cost_handler_.extract_commit_record(record.net_entries);

    if (placer_opts_.place_algorithm.is_timing_driven()) {
        record.affected_pins = blocks_affected.affected_pins;
        placer_state_.timing().extract_connection_commit_record(blocks_affected.affected_pins,
                                                                record.connection_entries);
    } else {
        record.affected_pins.clear();
        record.connection_entries.clear();
    }
}

void SwapEvaluator::apply_commit_record(t_pl_blocks_to_be_moved& blocks_affected,
                                        const t_swap_commit_record& record) {
    VTR_ASSERT_SAFE(!blocks_affected.moved_blocks.empty());

    BlkLocRegistry& blk_loc_registry = placer_state_.mutable_blk_loc_registry();
    blk_loc_registry.apply_move_blocks(blocks_affected);

    net_cost_handler_.apply_commit_record(record.net_entries);

    if (!record.connection_entries.empty()) {
        placer_state_.mutable_timing().apply_connection_commit_record(record.connection_entries);
    }

    blk_loc_registry.commit_move_blocks(blocks_affected);
    blocks_affected.clear_move_blocks();
}

void SwapEvaluator::revert(t_pl_blocks_to_be_moved& blocks_affected, bool revert_td) {
    // Reset the net cost function flags first.
    net_cost_handler_.reset_move_nets();

    // Restore the blk_loc_registry.block_locs data structures to their state before the move.
    placer_state_.mutable_blk_loc_registry().revert_move_blocks(blocks_affected);

    if (revert_td) {
        // Un-stage the values stored in proposed_* data structures
        placer_state_.mutable_timing().revert_td_cost(blocks_affected);
    }
}
