#pragma once
/**
 * @file swap_evaluator.h
 * @brief Declares SwapEvaluator, which evaluates, commits, or reverts a proposed
 * block swap against a specific placement state.
 *
 * The evaluate/commit/revert steps of a swap used to live inline in
 * PlacementAnnealer::try_swap_(). They are factored out here so the same code can
 * run against either the master placement state (sequential annealing) or a
 * worker-private replica of it (speculative parallel swap evaluation, see
 * parallel_anneal_engine.h).
 *
 * A key property this class maintains: apply_and_evaluate() only mutates
 * *evaluation* state (block_locs and the scratch/proposed data inside
 * NetCostHandler, InterposerCostHandler, and PlacerTimingContext). All
 * commit-only state (grid_blocks, committed per-net bounding boxes and costs,
 * committed connection delays/timing costs) is written exclusively by commit().
 * This separation is what makes speculative evaluation on replicas safe.
 */

#include "interposer_cost_handler.h"
#include "net_cost_handler.h"
#include "placer_state.h"

#include <functional>
#include <optional>

class PlaceDelayModel;
class PlacerCriticalities;
struct t_pl_blocks_to_be_moved;
struct t_placer_opts;

/**
 * @brief Cost deltas produced by evaluating a proposed swap.
 */
struct t_evaluated_move {
    /// Total change in the weighted placement cost. Not filled for
    /// SLACK_TIMING_PLACE (the caller computes it via setup slack analysis).
    double delta_c = 0.;
    /// Change in the timing cost (delay * criticality).
    double timing_delta_c = 0.;
    /// Per-net cost term deltas (bb, congestion, interposer terms).
    t_net_cost_terms cost_terms_delta;
    /// True when interposer cost terms were evaluated and must be committed.
    bool update_interposer_costs = false;
    /// True when the evaluation was abandoned partway because the caller's
    /// cancellation callback fired. The cost deltas are then meaningless and the
    /// move must be reverted, never committed.
    bool cancelled = false;
};

/**
 * @brief Everything an accepted move writes into committed placement state, in
 * replayable form: the affected nets' new BB/cost data and the affected
 * connections' new delays/timing costs.
 *
 * Captured once, on the state that evaluated the move, with
 * SwapEvaluator::extract_commit_record(); committed everywhere else with
 * SwapEvaluator::apply_commit_record(). This is how the speculative parallel
 * swap evaluation engine commits a winning move to the master state and every
 * replica with O(affected nets + pins) copies — each move is evaluated exactly
 * once, no matter how many state copies exist.
 */
struct t_swap_commit_record {
    /// Sink pins whose connection delay/timing cost changed (also needed for
    /// timing invalidation on the master state).
    std::vector<ClusterPinId> affected_pins;
    /// Per affected net: committed BB, sink counts, and net cost.
    std::vector<t_net_commit_entry> net_entries;
    /// Per affected connection: committed delay and timing cost.
    std::vector<t_connection_commit_entry> connection_entries;
};

/**
 * @class SwapEvaluator
 * @brief Applies a proposed move to a placement state, computes the resulting
 * cost deltas, and either commits or reverts the move.
 *
 * One instance operates on one placement state (a PlacerState plus its
 * NetCostHandler and optional InterposerCostHandler). The sequential annealer
 * uses a single instance over the master state; the parallel engine additionally
 * creates one instance per worker replica.
 */
class SwapEvaluator {
  public:
    SwapEvaluator() = delete;
    SwapEvaluator(const SwapEvaluator&) = delete;
    SwapEvaluator& operator=(const SwapEvaluator&) = delete;

    /**
     * @param placer_opts Placement algorithm options (timing tradeoff, cost factors).
     * @param costs Current placement cost terms; only the normalization factors are
     * read during evaluation. Never written by this class.
     * @param placer_state The placement state this evaluator operates on.
     * @param net_cost_handler Net cost handler bound to `placer_state`.
     * @param interposer_cost_handler Interposer cost handler bound to `net_cost_handler`
     * (std::nullopt when the device has no interposer cuts).
     * @param delay_model Placement delay model (nullptr for non-timing-driven placement).
     * @param criticalities Connection criticalities (nullptr for non-timing-driven placement).
     */
    SwapEvaluator(const t_placer_opts& placer_opts,
                  const t_placer_costs& costs,
                  PlacerState& placer_state,
                  NetCostHandler& net_cost_handler,
                  std::optional<InterposerCostHandler>& interposer_cost_handler,
                  const PlaceDelayModel* delay_model,
                  const PlacerCriticalities* criticalities);

    /**
     * @brief Applies the move in `blocks_affected` to block_locs and computes the
     * resulting cost deltas.
     *
     * For CRITICALITY_TIMING_PLACE and BOUNDING_BOX_PLACE the total weighted cost
     * change is returned in t_evaluated_move::delta_c. For SLACK_TIMING_PLACE only
     * the component deltas are computed; the caller performs the setup slack
     * analysis to obtain delta_c.
     *
     * After this call, either commit() or revert() must be called before the next
     * evaluation on this state — including when the evaluation was cancelled
     * partway via `should_cancel` (t_evaluated_move::cancelled set; only revert()
     * is valid then).
     */
    t_evaluated_move apply_and_evaluate(t_pl_blocks_to_be_moved& blocks_affected,
                                        const t_place_algorithm& place_algorithm,
                                        const std::function<bool()>& should_cancel = {});

    /**
     * @brief Commits an applied move to the commit-only placement state.
     *
     * Updates committed per-net bounding boxes/costs, interposer costs (when
     * `update_interposer_costs` is set), grid_blocks, and (when `commit_td` is set)
     * the committed connection delays and timing costs.
     */
    void commit(t_pl_blocks_to_be_moved& blocks_affected,
                bool update_interposer_costs,
                bool commit_td);

    /**
     * @brief Reverts an applied move, restoring block_locs and resetting the
     * scratch/proposed state. When `revert_td` is set, the proposed connection
     * delay/timing cost entries are also invalidated.
     */
    void revert(t_pl_blocks_to_be_moved& blocks_affected, bool revert_td);

    /**
     * @brief Captures the committed values the currently evaluated move would
     * write, so the move can later be committed on any identical state copy with
     * apply_commit_record() instead of being re-evaluated.
     *
     * Must be called between apply_and_evaluate() and commit()/revert() on this
     * state. Not supported when interposer cost terms are active (the parallel
     * engine falls back to the sequential annealer in that case).
     *
     * @param blocks_affected The evaluated move's record (source of affected pins).
     * @param record Filled with the replayable committed values.
     */
    void extract_commit_record(const t_pl_blocks_to_be_moved& blocks_affected,
                               t_swap_commit_record& record) const;

    /**
     * @brief Commits a move on this state from its recorded values, without
     * re-evaluating it: applies the block moves, writes the recorded committed
     * net/timing/interposer values, and commits grid_blocks.
     *
     * @param blocks_affected Scratch pre-loaded (via set_moved_blocks()) with the
     * winning move; cleared before returning.
     * @param record The values captured by extract_commit_record() on the state
     * copy that evaluated the move.
     */
    void apply_commit_record(t_pl_blocks_to_be_moved& blocks_affected,
                             const t_swap_commit_record& record);

  private:
    const t_placer_opts& placer_opts_;
    /// Placement cost terms; read for normalization factors only.
    const t_placer_costs& costs_;
    /// The placement state this evaluator operates on.
    PlacerState& placer_state_;
    /// Net cost handler bound to placer_state_.
    NetCostHandler& net_cost_handler_;
    /// Interposer cost handler bound to net_cost_handler_ (nullopt without interposer cuts).
    std::optional<InterposerCostHandler>& interposer_cost_handler_;
    const PlaceDelayModel* delay_model_;
    const PlacerCriticalities* criticalities_;
};
