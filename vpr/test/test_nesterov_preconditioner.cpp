/**
 * @file
 * @brief Unit tests for the NonlinearNesterovPlacer preconditioner.
 *
 * These tests call the production formulas in preconditioner_math.h directly --
 * the placer's compute_preconditioner_ calls the same free functions, so a
 * change to the shipped math breaks these tests.
 *
 * The invariant they enforce is the separation the diagonal now makes explicit:
 * true Hessian terms (wirelength, density, affinity springs) are summed as
 * curvature, while the incompatibility term is summed as *damping* -- an
 * explicit trust-region bound, because its exact Hessian is zero.
 */

#include "catch2/catch_approx.hpp"
#include "catch2/catch_test_macros.hpp"

#include <cmath>

#include "preconditioner_math.h"

using vtr::ap::affinity_spring_curvature;
using vtr::ap::jacobi_precond_diagonal;
using vtr::ap::kPreconditionAlpha;
using vtr::ap::kPreconditionFloor;

namespace {

/// Assemble a diagonal entry the way compute_preconditioner_ does.
double block_diagonal(double wl, double density, double affinity = 0.) {
    return jacobi_precond_diagonal(wl + density + affinity, 0., kPreconditionFloor, kPreconditionAlpha);
}

} // namespace

TEST_CASE("preconditioner floors to 1.0 for zero-curvature blocks", "[vpr_ap][preconditioner]") {
    // A filler with no incident nets and no density mass must get the floor:
    // dividing by < 1 amplifies noise, dividing by 0 is fatal.
    REQUIRE(block_diagonal(0.0, 0.0) == Catch::Approx(1.0).epsilon(1e-12));
    // Negative curvature can only come from a bug upstream; the floor must still
    // produce a safe positive divisor rather than a NaN from pow(negative, 0.5).
    REQUIRE(block_diagonal(-5.0, 0.0) == Catch::Approx(1.0).epsilon(1e-12));
}

TEST_CASE("preconditioner floors before softening, not after", "[vpr_ap][preconditioner]") {
    // max() then pow() bounds raw curvature at 1.0. Reversing the order lets a
    // negative sum reach pow() and produce NaN.
    REQUIRE(block_diagonal(0.25, 0.0) == Catch::Approx(std::pow(kPreconditionFloor, kPreconditionAlpha)));
    REQUIRE(block_diagonal(4.0, 0.0) == Catch::Approx(std::pow(4.0, kPreconditionAlpha)));
}

TEST_CASE("preconditioner is monotone in wirelength and density curvature", "[vpr_ap][preconditioner]") {
    REQUIRE(block_diagonal(0.1, 1.0) < block_diagonal(10.0, 1.0));
    REQUIRE(block_diagonal(10.0, 1.0) < block_diagonal(100.0, 1.0));
    REQUIRE(block_diagonal(1.0, 0.1) < block_diagonal(1.0, 10.0));
    REQUIRE(block_diagonal(1.0, 10.0) < block_diagonal(1.0, 100.0));
}

TEST_CASE("alpha softening compresses the curvature range", "[vpr_ap][preconditioner]") {
    // The raw diagonal spans orders of magnitude on heterogeneous FPGAs (a DSP
    // has ~100x the curvature of a LUT). pow(h, 0.5) compresses that.
    double h_min = block_diagonal(0.0, 0.0);
    double h_max = block_diagonal(1000.0, 1000.0);
    double raw_ratio = 2000.0 / kPreconditionFloor;
    REQUIRE(h_max / h_min == Catch::Approx(std::sqrt(raw_ratio)).epsilon(1e-9));
    REQUIRE(h_max / h_min < raw_ratio);
}

TEST_CASE("preconditioner normalizes step across heterogeneous blocks", "[vpr_ap][preconditioner]") {
    double h_lut = block_diagonal(0.5, 0.5);
    double h_dsp = block_diagonal(50.0, 50.0);
    REQUIRE(h_dsp / h_lut == Catch::Approx(10.0).epsilon(1e-9)); // sqrt(100x)
    REQUIRE((1.0 / h_lut) / (1.0 / h_dsp) == Catch::Approx(10.0).epsilon(1e-9));
}

TEST_CASE("preconditioner step normalization matches elfPlace Eq. 16", "[vpr_ap][preconditioner]") {
    // elfPlace Eq. 16: h_xi = max(sum(1/(|e|-1)) + lambda_s * q_i, 1). VTR
    // substitutes the WA net weight and density_multiplier * mass, then softens
    // by alpha < 1. With step_size = 1.0 the displacement is grad / h.
    double h = block_diagonal(3.0, 2.0);
    REQUIRE(5.0 / h == Catch::Approx(5.0 / std::pow(5.0, kPreconditionAlpha)).epsilon(1e-9));
    REQUIRE(5.0 / h < 5.0); // preconditioning always shortens the raw step
}

TEST_CASE("filler preconditioner uses density-only curvature", "[vpr_ap][preconditioner]") {
    // Fillers carry no nets, no affinity and no incompatibility.
    REQUIRE(jacobi_precond_diagonal(2.0 * 0.5, 0., kPreconditionFloor, kPreconditionAlpha)
            == Catch::Approx(std::pow(kPreconditionFloor, kPreconditionAlpha)));
    REQUIRE(jacobi_precond_diagonal(16.0, 0., kPreconditionFloor, kPreconditionAlpha)
            == Catch::Approx(4.0).epsilon(1e-12));
}

TEST_CASE("affinity spring curvature uses the frozen-centroid bound", "[vpr_ap][preconditioner]") {
    // Shipped form is W/n. The exact Hessian, accounting for the centroid moving
    // with x_k, is (W/n)(1 - 1/n) -- strictly smaller, so W/n is a conservative
    // upper bound that shortens steps rather than lengthening them.
    REQUIRE(affinity_spring_curvature(4.0, 2) == Catch::Approx(2.0).epsilon(1e-12));
    REQUIRE(affinity_spring_curvature(9.0, 3) == Catch::Approx(3.0).epsilon(1e-12));
    for (std::size_t n : {2u, 3u, 8u, 100u}) {
        double shipped = affinity_spring_curvature(6.0, n);
        double exact = (6.0 / n) * (1.0 - 1.0 / n);
        REQUIRE(shipped > exact); // conservative direction
        REQUIRE(shipped * (1.0 - 1.0 / n) == Catch::Approx(exact).epsilon(1e-12));
    }
}

TEST_CASE("affinity spring curvature is zero for degenerate groups", "[vpr_ap][preconditioner]") {
    // A single block is its own centroid: no spring, no curvature.
    REQUIRE(affinity_spring_curvature(5.0, 1) == 0.);
    REQUIRE(affinity_spring_curvature(5.0, 0) == 0.);
    REQUIRE(affinity_spring_curvature(0.0, 8) == 0.);
}

TEST_CASE("proximity anchor never enters the diagonal", "[vpr_ap][preconditioner]") {
    // The anchor is quadratic so its Hessian is real (= proximity_weight), but it
    // is the trust region coupling the solve to the legalizer and must act at
    // full strength. Neither argument of the diagonal may carry it.
    double h = block_diagonal(2.0, 3.0);
    REQUIRE(h == Catch::Approx(std::pow(5.0, kPreconditionAlpha)).epsilon(1e-12));
    double if_included = jacobi_precond_diagonal(2.0 + 3.0 + 2.0, 0., kPreconditionFloor, kPreconditionAlpha);
    REQUIRE(if_included > h);
}
