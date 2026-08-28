#pragma once
/**
 * @file swap_evaluator.h
 * @brief Declares SwapEvaluator, which evaluates, commits, or reverts a proposed
 * block swap against a specific placement state.
 */

#include "interposer_cost_handler.h"
#include "net_cost_handler.h"
#include "placer_state.h"

#include <optional>

class PlaceDelayModel;
class PlacerCriticalities;
struct t_pl_blocks_to_be_moved;
struct t_placer_opts;

/// @brief Cost deltas produced by evaluating a proposed swap.
struct t_swap_cost_deltas {
    /// Total change in the weighted placement cost.
    double delta_c = 0.;
    /// Change in the timing cost.
    double timing_delta_c = 0.;
    /// Per-net cost term deltas.
    t_net_cost_terms cost_terms_delta;
    /// True when interposer cost terms were evaluated and must be committed.
    bool update_interposer_costs = false;
};

/**
 * @class SwapEvaluator
 * @brief Applies a proposed move to a placement state, computes the resulting
 * cost deltas, and either commits or reverts the move.
 *
 * One instance operates on one placement state. The sequential annealer
 * uses a single instance.
 */
class SwapEvaluator {
  public:
    SwapEvaluator() = delete;
    SwapEvaluator(const SwapEvaluator&) = delete;
    SwapEvaluator& operator=(const SwapEvaluator&) = delete;

    /**
     * @param placer_opts Placement algorithm options (timing tradeoff, cost factors).
     * @param costs Current placement cost terms. Only the normalization factors are
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
     * commit() or revert() must be called before the next evaluation on this
     * state.
     */
    t_swap_cost_deltas apply_and_evaluate(t_pl_blocks_to_be_moved& blocks_affected,
                                          const t_place_algorithm& place_algorithm);

    /**
     * @brief Makes an applied move permanent: writes the committed net bounding
     * boxes and costs, grid_blocks, interposer costs, and connection delays and
     * timing costs (when `commit_td`).
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

  private:
    /// Placement algorithm options (timing tradeoff, cost factors).
    const t_placer_opts& placer_opts_;
    /// Placement cost terms; read for normalization factors only.
    const t_placer_costs& costs_;
    /// The placement state this evaluator operates on.
    PlacerState& placer_state_;
    /// Net cost handler bound to placer_state_.
    NetCostHandler& net_cost_handler_;
    /// Interposer cost handler bound to net_cost_handler_ (nullopt without interposer cuts).
    std::optional<InterposerCostHandler>& interposer_cost_handler_;
    /// Placement delay model (nullptr for non-timing-driven placement).
    const PlaceDelayModel* delay_model_;
    /// Connection criticalities (nullptr for non-timing-driven placement).
    const PlacerCriticalities* criticalities_;
};
