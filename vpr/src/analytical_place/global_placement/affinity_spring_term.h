#pragma once
/**
 * @file
 * @author  William Zhang
 * @date    August 2026
 * @brief   Quadratic centroid affinity-spring objective term.
 *
 * Groups of AP blocks (direct output-driver↔outpad pairs, long prepacker
 * chains) are pulled toward their group centroid by a quadratic spring. Group
 * *detection* stays with its producers (the placer's io-pair/pack-pattern
 * initialization); this term owns the groups' energy, gradient, and
 * preconditioner contribution.
 */

#include <vector>
#include "objective_term.h"

/**
 * @brief Affinity-spring detector kind (logging / per-kind weights).
 */
enum class e_affinity_kind {
    IO_PAIR,     ///< Direct 2-pin output-driver↔outpad pairs.
    PACK_PATTERN ///< Long prepacker chain groups spanning multiple AP blocks.
};

/**
 * @brief A set of AP blocks pulled together by a quadratic centroid spring.
 */
struct AffinityGroup {
    e_affinity_kind kind = e_affinity_kind::IO_PAIR; ///< Detector that created this group.
    std::vector<APBlockId> blocks;                   ///< AP blocks in the group (size >= 2).
};

class AffinitySpringTerm final : public ObjectiveTerm {
  public:
    /**
     * @brief Construct with per-kind kernel inputs; precomputes block mobility.
     *
     * @param io_pair_attraction_weight Legacy per-block I/O pair spring strength
     *                                  (the kernel applies 2x for n=2 pack math).
     * @param pack_pattern_weight       Pack-pattern kernel weight; may later be
     *                                  zeroed via @ref set_pack_pattern_weight.
     */
    AffinitySpringTerm(const APNetlist& ap_netlist,
                       double io_pair_attraction_weight,
                       double pack_pattern_weight);

    void clear_groups() { groups_.clear(); }
    void add_group(AffinityGroup group) { groups_.push_back(std::move(group)); }
    const std::vector<AffinityGroup>& groups() const { return groups_; }

    /// @brief Runtime gate: pack-pattern springs are disabled on designs
    ///        without long direct I/O-chain nets.
    void set_pack_pattern_weight(double weight) { pack_pattern_weight_ = weight; }

    const char* name() const final { return "affinity-springs"; }

    /**
     * @brief Weighted spring penalty; gradient only on moveable blocks.
     *
     * Penalty (unweighted): sum_b 0.5 / n * ||x_b - c||^2 per group.
     */
    double evaluate(const PartialPlacement& p_placement,
                    std::optional<std::reference_wrapper<PlacementGradient>> grad) const final;

    /**
     * @brief Frozen-centroid curvature per group member (see
     *        affinity_spring_curvature() for why the exact (1 - 1/n) factor is
     *        not used).
     */
    void add_curvature(vtr::vector<APBlockId, double>& diagonal) const final;

  private:
    double kernel_weight_(e_affinity_kind kind) const;

    std::vector<AffinityGroup> groups_;
    double io_pair_attraction_weight_ = 0.;
    double pack_pattern_weight_ = 0.;
    vtr::vector<APBlockId, bool> moveable_; ///< Block mobility, precomputed (static per netlist).
};
