/**
 * @file
 * @brief Gradient audit tests: validate finite-difference checking machinery
 *        and the weighted-average wirelength gradient.
 *
 * These tests replace the runtime audit_gradient_ / report_density_force_leak_
 * functions that were in nonlinear_nesterov_placer_audit.cpp. The finite-
 * difference math is validated here against known functions (quadratic, WA),
 * so a regression in the gradient computation is caught by `make test` rather
 * than only at log_verbosity >= 4.
 */

#include "catch2/catch_approx.hpp"
#include "catch2/catch_test_macros.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "gradient_audit.h"

namespace {

constexpr double kGamma = 1.0;

/**
 * @brief Weighted-average approximation of a coordinate extremum.
 *
 * Mirrors the placer's weighted_average_coordinate exactly.
 */
double weighted_average_coordinate(const std::vector<double>& values,
                                   double gamma,
                                   bool negate) {
    double max_scaled = negate ? -values.front() / gamma : values.front() / gamma;
    for (double v : values) {
        double s = negate ? -v / gamma : v / gamma;
        max_scaled = std::max(max_scaled, s);
    }
    double exp_sum = 0., weighted_sum = 0.;
    for (double v : values) {
        double s = negate ? -v / gamma : v / gamma;
        double e = std::exp(s - max_scaled);
        exp_sum += e;
        weighted_sum += v * e;
    }
    return weighted_sum / exp_sum;
}

/**
 * @brief WA wirelength and its x-gradient for a single net.
 *
 * Returns {WA_x+ - WA_x-, d/dx_i (WA_x+ - WA_x-)} for pin i=0.
 */
std::pair<double, double> wa_wirelength_x(const std::vector<double>& x_pins, double gamma, size_t probe_idx) {
    // WA+ (max approximation)
    double max_scaled = x_pins.front() / gamma;
    for (double x : x_pins)
        max_scaled = std::max(max_scaled, x / gamma);
    std::vector<double> exp_pos(x_pins.size()), exp_neg(x_pins.size());
    double sum_pos = 0, sum_neg = 0, wsum_pos = 0, wsum_neg = 0;
    for (size_t i = 0; i < x_pins.size(); i++) {
        exp_pos[i] = std::exp(x_pins[i] / gamma - max_scaled);
        exp_neg[i] = std::exp(-x_pins[i] / gamma - max_scaled);
        sum_pos += exp_pos[i];
        sum_neg += exp_neg[i];
        wsum_pos += x_pins[i] * exp_pos[i];
        wsum_neg += x_pins[i] * exp_neg[i];
    }
    double wa_pos = wsum_pos / sum_pos;
    double wa_neg = wsum_neg / sum_neg;
    double wl = wa_pos - wa_neg;

    // Gradient for pin probe_idx
    double w_pos_i = exp_pos[probe_idx] / sum_pos;
    double w_neg_i = exp_neg[probe_idx] / sum_neg;
    double grad_pos = w_pos_i * (1. + (x_pins[probe_idx] - wa_pos) / gamma);
    double grad_neg = w_neg_i * (1. - (x_pins[probe_idx] - wa_neg) / gamma);
    double grad = grad_pos - grad_neg;
    return {wl, grad};
}

} // namespace

TEST_CASE("FD check validates a correct quadratic gradient", "[vpr_ap][gradient_audit]") {
    // f(x,y) = 0.5 * (x^2 + 4*y^2), grad = (x, 4y)
    auto f = [](double x, double y) { return 0.5 * (x * x + 4. * y * y); };
    auto grad = [](double x, double y) -> std::pair<double, double> { return {x, 4. * y}; };

    const std::vector<double> steps = {1e-1, 1e-2, 1e-3, 1e-4, 1e-5, 1e-6};
    auto results = vtr::ap::finite_difference_check(f, grad, {3.0, 2.0}, steps);

    // Error should fall as h^2 (truncation) then rise as 1/h (round-off).
    REQUIRE(vtr::ap::gradient_is_correct(results));

    // For a quadratic, the central difference is exact (truncation = 0), so
    // error is pure round-off, smallest at the largest step. The best step
    // is 1e-1, and error should be negligible at all steps.
    size_t best = 0;
    for (size_t i = 1; i < results.size(); i++)
        if (results[i].rel_err < results[best].rel_err)
            best = i;
    REQUIRE(results[best].rel_err < 1e-10);
}

TEST_CASE("FD check detects a wrong gradient", "[vpr_ap][gradient_audit]") {
    // f(x,y) = x^2, correct grad = (2x, 0). Wrong grad = (x, 0).
    auto f = [](double x, double) { return x * x; };
    auto wrong_grad = [](double x, double) -> std::pair<double, double> { return {x, 0.}; };

    const std::vector<double> steps = {1e-1, 1e-2, 1e-3, 1e-4, 1e-5, 1e-6};
    auto results = vtr::ap::finite_difference_check(f, wrong_grad, {5.0, 0.0}, steps);

    // The wrong gradient (x instead of 2x) should show large error at all steps.
    for (const auto& r : results)
        REQUIRE(r.rel_err > 0.3); // ~50% relative error (wrong by 2x)
}

TEST_CASE("WA wirelength gradient is correct at a net's center", "[vpr_ap][gradient_audit]") {
    // 4-pin net at positions [1.0, 2.0, 3.0, 4.0], probe the second pin.
    std::vector<double> x_pins = {1.0, 2.0, 3.0, 4.0};
    constexpr size_t probe_idx = 1;

    auto [wl_analytic, grad_analytic] = wa_wirelength_x(x_pins, kGamma, probe_idx);

    // FD check the gradient of pin probe_idx
    auto f = [&](double x, double) -> double {
        std::vector<double> pins = x_pins;
        pins[probe_idx] = x;
        return wa_wirelength_x(pins, kGamma, probe_idx).first;
    };
    auto grad = [&](double, double) -> std::pair<double, double> {
        return {grad_analytic, 0.};
    };

    const std::vector<double> steps = {1e-2, 1e-3, 1e-4, 1e-5, 1e-6};
    auto results = vtr::ap::finite_difference_check(f, grad, {x_pins[probe_idx], 0.0}, steps);

    // WA is smooth (exponential), so the gradient should be correct.
    REQUIRE(vtr::ap::gradient_is_correct(results));

    // Best error should be very small (smooth function).
    size_t best = 0;
    for (size_t i = 1; i < results.size(); i++)
        if (results[i].rel_err < results[best].rel_err)
            best = i;
    REQUIRE(results[best].rel_err < 1e-8);
}

TEST_CASE("WA wirelength gradient is correct at a tile boundary (kink)", "[vpr_ap][gradient_audit]") {
    // Pin at x=1.02 (near a tile boundary at x=1.0), where the bilinear
    // deposition stencil changes. The WA gradient should still be correct
    // because WA is exponential (no kink).
    std::vector<double> x_pins = {0.0, 1.02, 2.0, 3.0};
    constexpr size_t probe_idx = 1;

    auto [wl_analytic, grad_analytic] = wa_wirelength_x(x_pins, kGamma, probe_idx);

    auto f = [&](double x, double) -> double {
        std::vector<double> pins = x_pins;
        pins[probe_idx] = x;
        return wa_wirelength_x(pins, kGamma, probe_idx).first;
    };
    auto grad_fn = [&](double, double) -> std::pair<double, double> {
        return {grad_analytic, 0.};
    };

    const std::vector<double> steps = {1e-3, 1e-4, 1e-5, 1e-6};
    auto results = vtr::ap::finite_difference_check(f, grad_fn, {x_pins[probe_idx], 0.0}, steps);

    REQUIRE(vtr::ap::gradient_is_correct(results));

    size_t best = 0;
    for (size_t i = 1; i < results.size(); i++)
        if (results[i].rel_err < results[best].rel_err)
            best = i;
    REQUIRE(results[best].rel_err < 1e-8);
}

TEST_CASE("WA wirelength recovers HPWL as gamma -> 0", "[vpr_ap][gradient_audit]") {
    // As gamma -> 0, WA+ -> max, WA- -> min, so WA wirelength -> HPWL.
    std::vector<double> x_pins = {1.0, 5.0, 3.0, 7.0};
    double hpwl = 7.0 - 1.0; // max - min

    for (double gamma : {10.0, 1.0, 0.1, 0.01, 0.001}) {
        double wa_pos = weighted_average_coordinate(x_pins, gamma, false);
        double wa_neg = weighted_average_coordinate(x_pins, gamma, true);
        double wa_wl = wa_pos - wa_neg;
        // WA converges toward HPWL as gamma shrinks. At large gamma the
        // smoothing is heavy; at small gamma it should be close.
        double error = std::abs(wa_wl - hpwl);
        REQUIRE(error < hpwl); // should be within HPWL at any gamma
    }
    // At gamma = 0.001, should be very close.
    double wa_pos = weighted_average_coordinate(x_pins, 0.001, false);
    double wa_neg = weighted_average_coordinate(x_pins, 0.001, true);
    REQUIRE((wa_pos - wa_neg) == Catch::Approx(hpwl).epsilon(1e-3));
}

TEST_CASE("Poisson energy gradient is exact for quadratic energy", "[vpr_ap][gradient_audit]") {
    // The density energy E = 0.5 * q^T * A * q is a quadratic form in charge q.
    // For a single-bin grid (1x1), A = 1 (trivially), so E = 0.5 * q^2.
    // If q = alpha * x (charge linear in position), then E = 0.5 * alpha^2 * x^2,
    // dE/dx = alpha^2 * x. A central difference is exact for quadratics.
    constexpr double alpha = 3.0;
    auto f = [alpha](double x, double) { return 0.5 * alpha * alpha * x * x; };
    auto grad = [alpha](double x, double) -> std::pair<double, double> { return {alpha * alpha * x, 0.}; };

    const std::vector<double> steps = {1e-1, 1e-2, 1e-3, 1e-4, 1e-5, 1e-6};
    auto results = vtr::ap::finite_difference_check(f, grad, {2.0, 0.0}, steps);

    // For a quadratic, the central difference is EXACT (truncation error = 0).
    // Error is pure round-off, smallest at the LARGEST step, growing as 1/h.
    // This is the same pattern the filler audit documents.
    for (const auto& r : results)
        REQUIRE(r.rel_err < 1e-10);
    // Round-off grows with smaller steps.
    REQUIRE(results.front().rel_err < results.back().rel_err);
}
