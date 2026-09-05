#pragma once
/**
 * @file
 * @author  William Zhang
 * @date    August 2026
 * @brief   The objective-term interface of the nonlinear Nesterov placer.
 *
 * The placer's differentiable objective is a weighted sum of terms (wirelength,
 * density, affinity springs, timing springs). The unit of
 * genericity is the term: each term owns its value, its gradient, and its
 * Hessian-diagonal (preconditioner) contribution together, because those three
 * must never drift apart — an inconsistent pair produces steps the line search
 * cannot explain. A term's *generic* face is only what the optimizer needs;
 * term-specific configuration (anchors, schedules, weight refreshes) stays on
 * the concrete class and is driven by the flow harness between epochs.
 */

#include <optional>
#include <functional>
#include "ap_netlist.h"
#include "vtr_vector.h"

struct PartialPlacement;

/**
 * @brief Per-block placement gradient accumulated across objective terms.
 */
struct PlacementGradient {
    vtr::vector<APBlockId, double> dx; ///< Objective derivative with respect to x.
    vtr::vector<APBlockId, double> dy; ///< Objective derivative with respect to y.

    /**
     * @brief Construct a zero gradient sized for the AP netlist.
     */
    explicit PlacementGradient(const APNetlist& ap_netlist)
        : dx(ap_netlist.blocks().size(), 0.)
        , dy(ap_netlist.blocks().size(), 0.) {}

    /**
     * @brief Reset all gradient entries to zero.
     */
    void clear() {
        std::fill(dx.begin(), dx.end(), 0.);
        std::fill(dy.begin(), dy.end(), 0.);
    }
};

/**
 * @brief One differentiable term of the placement objective.
 */
class ObjectiveTerm {
  public:
    virtual ~ObjectiveTerm() = default;

    /// @brief Stable short name for logs and diagnostics.
    virtual const char* name() const = 0;

    /**
     * @brief Weighted term value at @p p_placement; accumulates the weighted
     *        gradient into @p grad when given.
     */
    virtual double evaluate(const PartialPlacement& p_placement,
                            std::optional<std::reference_wrapper<PlacementGradient>> grad) const = 0;

    /**
     * @brief Accumulate this term's Hessian-diagonal estimate into the
     *        preconditioner @p diagonal (same weighting as the gradient).
     */
    virtual void add_curvature(vtr::vector<APBlockId, double>& diagonal) const = 0;
};
