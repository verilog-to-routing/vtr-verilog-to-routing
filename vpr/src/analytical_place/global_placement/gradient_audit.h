/**
 * @file
 * @brief Generic finite-difference gradient audit utilities.
 *
 * Extracted from the placer's runtime audit so tests can validate the
 * finite-difference math without constructing a full placer. The placer's
 * own audit (if needed at runtime) calls these same free functions.
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace vtr {
namespace ap {

/**
 * @brief Result of a single finite-difference probe.
 */
struct FdProbeResult {
    double step = 0.;
    double analytic = 0.;
    double finite_diff = 0.;
    double rel_err = 0.;
};

/**
 * @brief Central finite-difference check of an analytic gradient.
 *
 * @param f         Objective function f(x) -> double.
 * @param grad_f    Analytic gradient grad_f(x) -> (df/dx, df/dy).
 * @param x0        Probe position (x, y).
 * @param steps     Finite-difference step sizes to sweep.
 * @return Per-step results (relative error should fall as h^2 then rise as 1/h).
 */
inline std::vector<FdProbeResult> finite_difference_check(
    std::function<double(double, double)> f,
    std::function<std::pair<double, double>(double, double)> grad_f,
    std::pair<double, double> x0,
    const std::vector<double>& steps) {
    auto [x, y] = x0;
    auto [gx, gy] = grad_f(x, y);
    std::vector<FdProbeResult> results;
    for (double h : steps) {
        double f_plus = f(x + h, y);
        double f_minus = f(x - h, y);
        double fd = (f_plus - f_minus) / (2. * h);
        double scale = std::max({std::abs(fd), std::abs(gx), 1e-12});
        double rel = std::abs(fd - gx) / scale;
        results.push_back({h, gx, fd, rel});
    }
    return results;
}

/**
 * @brief Verify a gradient is correct: error falls as h^2 then rises as 1/h.
 *
 * A correct derivative's error decreases quadratically with step size
 * (truncation), then increases as 1/step (round-off). A wrong derivative
 * plateaus. Returns true if either:
 * - The smallest-error step is in the middle (truncation then round-off),
 * - Or the error is negligible at all steps (exact derivative, e.g. quadratic).
 */
inline bool gradient_is_correct(const std::vector<FdProbeResult>& results) {
    if (results.size() < 2)
        return false;
    // Find the step with minimum error.
    size_t best_idx = 0;
    for (size_t i = 1; i < results.size(); i++) {
        if (results[i].rel_err < results[best_idx].rel_err)
            best_idx = i;
    }
    // Case 1: best step is in the middle (truncation-dominated then round-off).
    if (best_idx > 0 && best_idx < results.size() - 1)
        return true;
    // Case 2: best step is the largest, but error is negligible everywhere
    // (exact derivative like a quadratic — central difference has zero truncation).
    if (best_idx == 0 && results[0].rel_err < 1e-10)
        return true;
    return false;
}

} // namespace ap
} // namespace vtr
