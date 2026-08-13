/**
 * @file
 * @brief Jacobi (diagonal) preconditioner math for the nonlinear Nesterov placer.
 *
 * Extracted from NonlinearNesterovPlacer::compute_preconditioner_ so tests can
 * validate the actual production formulas without constructing a full placer.
 * The placer calls these same free functions -- a test that passes here is a
 * test of the shipped math, not of a copy of it.
 *
 * The diagonal approximates the objective Hessian. Only terms with a genuine,
 * non-zero second derivative belong in it (see @ref jacobi_precond_diagonal).
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace vtr {
namespace ap {

/**
 * @brief Exponent applied to the Jacobi preconditioner diagonal.
 *
 * The Jacobi preconditioner diagonal h_xi is raised to this power before
 * dividing the gradient. elfPlace uses alpha=1.0 (raw diagonal), but VTR's
 * heterogeneous FPGA blocks have much wider curvature range than the ASIC
 * standard cells elfPlace targets: a dense DSP block has wirelength+density
 * curvature orders of magnitude above a single LUT. Alpha < 1 compresses
 * the range so low-curvature blocks aren't under-stepped and high-curvature
 * blocks aren't over-stepped. 0.5 (square root) is the validated value from
 * the original nesterov placer.
 */
constexpr double kPreconditionAlpha = 0.5;

/**
 * @brief Floor on the per-block preconditioner to avoid dividing by ~0 curvature.
 *
 * Following elfPlace Eq. 16, the Jacobi preconditioner diagonal is
 * `max(sum_wirelength_curvature + density_multiplier * block_mass, 1.0)`,
 * clamped to >= 1 to protect filler instances with no incident nets.
 */
constexpr double kPreconditionFloor = 1.0;

/**
 * @brief Hessian diagonal of one centroid affinity spring, per member block.
 *
 * The affinity penalty for a group of n blocks with weight W is
 *
 *     P = W * sum_i (1 / 2n) * (x_i - c)^2,   c = (1/n) * sum_j x_j
 *
 * Holding the centroid fixed gives d2P/dx_k2 = W/n, which is what this returns.
 * Letting the centroid move with x_k -- which it does -- gives the exact value
 *
 *     d2P/dx_k2 = (W/n) * (1 - 1/n)
 *
 * so the frozen-centroid form used here is an upper bound, by a factor of
 * n/(n-1) (2x for the common 2-block I/O-pair groups). Over-estimating a
 * preconditioner diagonal is the conservative direction -- it shortens steps
 * rather than lengthening them -- so this is a deliberate approximation, not an
 * oversight. Switching to the exact factor is a QoR change and must be measured
 * on its own before being adopted; it has not been.
 *
 * @param weight     Group kernel weight W (already includes any per-kind scaling).
 * @param group_size Number of blocks n in the group.
 * @return Per-block curvature contribution, or 0 for degenerate groups.
 */
inline double affinity_spring_curvature(double weight, std::size_t group_size) {
    if (weight == 0. || group_size < 2)
        return 0.;
    return weight / static_cast<double>(group_size);
}

/**
 * @brief Assemble a preconditioner diagonal entry.
 *
 * Applies the floor *before* the softening exponent, so the floor bounds raw
 * curvature rather than the softened value:
 *
 *     h = max(curvature_sum + damping_sum, floor) ^ alpha
 *
 * The two arguments are kept separate because they are different things, and
 * conflating them is what previously made this function impossible to reason
 * about:
 *
 * - @p curvature_sum holds terms with a genuinely non-zero second derivative:
 *   wirelength (sum of incident net weights), density (density multiplier times
 *   block mass), and affinity springs (@ref affinity_spring_curvature).
 * - @p damping_sum holds explicit step-control terms that are *not* curvature.
 *   No such term is currently in the objective, so callers pass 0; the argument
 *   stays because keeping true curvature separate from step-control damping is
 *   the invariant that made this diagonal possible to reason about, and
 *   conflating them silently is what previously broke it. The one historical
 *   occupant was a trust-region bound on the continuous incompatibility
 *   penalty, removed with that penalty.
 *
 * The proximity anchor belongs to neither and is excluded. Its Hessian is
 * genuinely non-zero (it is quadratic, diagonal = proximity_weight), but the
 * anchor is the trust region coupling the continuous solve to the legalizer and
 * is deliberately allowed to act at full strength.
 *
 * @param curvature_sum True Hessian-diagonal terms.
 * @param damping_sum   Explicit non-curvature step-control terms.
 * @param floor         Lower bound before softening (@ref kPreconditionFloor).
 * @param alpha         Softening exponent (@ref kPreconditionAlpha).
 * @return The preconditioner diagonal entry to divide the gradient by.
 */
inline double jacobi_precond_diagonal(double curvature_sum, double damping_sum, double floor, double alpha) {
    return std::pow(std::max(curvature_sum + damping_sum, floor), alpha);
}

} // namespace ap
} // namespace vtr
