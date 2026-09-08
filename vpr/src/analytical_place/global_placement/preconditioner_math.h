#pragma once

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

#include <algorithm>
#include <cmath>

namespace vtr {
namespace ap {

/**
 * @brief Exponent applied to the Jacobi preconditioner diagonal.
 *
 * The Hessian-diagonal estimate is raised to this power before dividing the
 * gradient. A full Jacobi diagonal produced the best routed wirelength and
 * runtime in the full-board exponent sweep.
 */
constexpr double kPreconditionAlpha = 1.0;

/**
 * @brief Floor on the per-block preconditioner to avoid dividing by ~0 curvature.
 *
 * Following elfPlace Eq. 16, the Jacobi preconditioner diagonal is
 * `max(sum_wirelength_curvature + density_multiplier * block_mass, 1.0)`,
 * clamped to >= 1 to protect filler instances with no incident nets.
 */
constexpr double kPreconditionFloor = 1.0;

/**
 * @brief Assemble a preconditioner diagonal entry.
 *
 * Applies the floor *before* the softening exponent, so the floor bounds raw
 * curvature rather than the softened value:
 *
 *     h = max(curvature_sum, floor) ^ alpha
 *
 * @param curvature_sum True Hessian-diagonal terms.
 * @param floor         Lower bound before softening (@ref kPreconditionFloor).
 * @param alpha         Softening exponent (@ref kPreconditionAlpha).
 * @return The preconditioner diagonal entry to divide the gradient by.
 */
inline double jacobi_precond_diagonal(double curvature_sum, double floor, double alpha) {
    return std::pow(std::max(curvature_sum, floor), alpha);
}

} // namespace ap
} // namespace vtr
