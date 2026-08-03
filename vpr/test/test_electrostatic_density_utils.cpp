/**
 * @file
 * @brief Architectural edge-case tests for the electrostatic density gradient.
 */

#include "catch2/catch_approx.hpp"
#include "catch2/catch_test_macros.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "electrostatic_density_utils.h"

namespace {

constexpr double kCapacityEpsilon = 1e-9;

/**
 * @brief Flatten a multilayer grid coordinate.
 */
size_t site_index(size_t layer,
                  size_t x,
                  size_t y,
                  size_t width,
                  size_t height) {
    return (layer * height + y) * width + x;
}

/**
 * @brief Apply the positive semidefinite path-graph Neumann Laplacian.
 */
std::vector<double> apply_neumann_laplacian(const std::vector<double>& values,
                                            size_t width,
                                            size_t height) {
    REQUIRE(values.size() == width * height);
    std::vector<double> result(values.size(), 0.);
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            size_t idx = y * width + x;
            if (x > 0)
                result[idx] += values[idx] - values[idx - 1];
            if (x + 1 < width)
                result[idx] += values[idx] - values[idx + 1];
            if (y > 0)
                result[idx] += values[idx] - values[idx - width];
            if (y + 1 < height)
                result[idx] += values[idx] - values[idx + width];
        }
    }
    return result;
}

/**
 * @brief Return one layer's Poisson energy.
 */
double poisson_energy(const std::vector<double>& charge,
                      size_t width,
                      size_t height) {
    std::vector<double> potential;
    solve_neumann_poisson_dct(charge, width, height, potential);
    return 0.5 * std::inner_product(charge.begin(), charge.end(), potential.begin(), 0.);
}

struct SyntheticParticle {
    double x = 0.;    ///< Continuous x coordinate.
    double y = 0.;    ///< Continuous y coordinate.
    size_t layer = 0; ///< Discrete device layer.
    double mass = 1.; ///< Density mass, including any inflation.
};

struct SyntheticEvaluation {
    double energy = 0.;                               ///< Electrostatic energy.
    std::vector<std::pair<double, double>> gradients; ///< Per-particle x/y gradient.
    std::vector<double> charge;                       ///< Neutralized charge.
    std::vector<double> potential;                    ///< Per-layer potential.
};

/**
 * @brief Evaluate the same residual-charge energy and particle gradient as the placer.
 */
SyntheticEvaluation evaluate_synthetic_density(size_t width,
                                               size_t height,
                                               size_t num_layers,
                                               const std::vector<double>& target_capacity,
                                               const std::vector<SyntheticParticle>& particles) {
    size_t layer_size = width * height;
    size_t num_sites = layer_size * num_layers;
    REQUIRE(target_capacity.size() == num_sites);

    std::vector<double> utilization(num_sites, 0.);
    for (const SyntheticParticle& particle : particles) {
        REQUIRE(particle.layer < num_layers);
        BilinearDensityStencil stencil = make_bilinear_density_stencil(particle.x,
                                                                       particle.y,
                                                                       width,
                                                                       height);
        deposit_bilinear_density(utilization,
                                 particle.layer,
                                 width,
                                 height,
                                 stencil,
                                 particle.mass);
    }

    SyntheticEvaluation result;
    result.charge.resize(num_sites, 0.);
    for (size_t idx = 0; idx < num_sites; idx++) {
        result.charge[idx] = target_capacity[idx] > kCapacityEpsilon
                                 ? utilization[idx] - target_capacity[idx]
                                 : utilization[idx];
    }
    rebalance_density_charge_on_capacity_sites(result.charge, target_capacity, kCapacityEpsilon);

    result.potential.resize(num_sites, 0.);
    std::vector<double> layer_charge(layer_size);
    std::vector<double> layer_potential;
    for (size_t layer = 0; layer < num_layers; layer++) {
        std::copy_n(result.charge.begin() + layer * layer_size,
                    layer_size,
                    layer_charge.begin());
        solve_neumann_poisson_dct(layer_charge, width, height, layer_potential);
        std::copy(layer_potential.begin(),
                  layer_potential.end(),
                  result.potential.begin() + layer * layer_size);
    }
    result.energy = 0.5 * std::inner_product(result.charge.begin(), result.charge.end(), result.potential.begin(), 0.);

    result.gradients.reserve(particles.size());
    for (const SyntheticParticle& particle : particles) {
        BilinearDensityStencil stencil = make_bilinear_density_stencil(particle.x,
                                                                       particle.y,
                                                                       width,
                                                                       height);
        auto [dx, dy] = gradient_bilinear_density(result.potential,
                                                  particle.layer,
                                                  width,
                                                  height,
                                                  stencil);
        result.gradients.emplace_back(particle.mass * dx, particle.mass * dy);
    }
    return result;
}

/**
 * @brief Check particle gradients by central differences away from stencil kinks.
 */
void require_synthetic_gradients(size_t width,
                                 size_t height,
                                 size_t num_layers,
                                 const std::vector<double>& target_capacity,
                                 const std::vector<SyntheticParticle>& particles) {
    SyntheticEvaluation base = evaluate_synthetic_density(width,
                                                          height,
                                                          num_layers,
                                                          target_capacity,
                                                          particles);
    constexpr double h = 1e-4;
    for (size_t particle_idx = 0; particle_idx < particles.size(); particle_idx++) {
        for (int axis = 0; axis < 2; axis++) {
            size_t extent = axis == 0 ? width : height;
            double coordinate = axis == 0 ? particles[particle_idx].x : particles[particle_idx].y;
            if (extent == 1
                || coordinate - h < 0.
                || coordinate + h > static_cast<double>(extent) - kDensityDeviceBoundaryEpsilon
                || std::floor(coordinate - h) != std::floor(coordinate + h)) {
                continue;
            }

            std::vector<SyntheticParticle> plus = particles;
            std::vector<SyntheticParticle> minus = particles;
            double& plus_coordinate = axis == 0 ? plus[particle_idx].x : plus[particle_idx].y;
            double& minus_coordinate = axis == 0 ? minus[particle_idx].x : minus[particle_idx].y;
            plus_coordinate += h;
            minus_coordinate -= h;
            double plus_energy = evaluate_synthetic_density(width,
                                                            height,
                                                            num_layers,
                                                            target_capacity,
                                                            plus)
                                     .energy;
            double minus_energy = evaluate_synthetic_density(width,
                                                             height,
                                                             num_layers,
                                                             target_capacity,
                                                             minus)
                                      .energy;
            double finite_difference = (plus_energy - minus_energy) / (2. * h);
            double analytic = axis == 0 ? base.gradients[particle_idx].first
                                        : base.gradients[particle_idx].second;
            double scale = std::max({1., std::abs(finite_difference), std::abs(analytic)});
            INFO("particle=" << particle_idx << " axis=" << axis
                             << " grid=" << width << "x" << height
                             << " layers=" << num_layers
                             << " analytic=" << analytic
                             << " finite_difference=" << finite_difference);
            REQUIRE(std::abs(finite_difference - analytic) <= 2e-7 * scale);
        }
    }
}

TEST_CASE("electrostatic Poisson solve covers degenerate and rectangular grids", "[vpr_ap][density_gradient]") {
    const std::vector<std::pair<size_t, size_t>> shapes = {
        {1, 1},
        {1, 7},
        {8, 1},
        {2, 2},
        {5, 4},
        {6, 7},
    };
    for (const auto& [width, height] : shapes) {
        DYNAMIC_SECTION("grid " << width << "x" << height) {
            std::vector<double> charge(width * height);
            for (size_t idx = 0; idx < charge.size(); idx++)
                charge[idx] = std::sin(0.7 * (idx + 1)) + 0.13 * static_cast<double>(idx % 3);

            std::vector<double> potential;
            solve_neumann_poisson_dct(charge, width, height, potential);
            REQUIRE(potential.size() == charge.size());
            for (double value : potential)
                REQUIRE(std::isfinite(value));

            double potential_mean = std::accumulate(potential.begin(), potential.end(), 0.)
                                    / static_cast<double>(potential.size());
            REQUIRE(std::abs(potential_mean) <= 2e-11);

            std::vector<double> residual = apply_neumann_laplacian(potential, width, height);
            double charge_mean = std::accumulate(charge.begin(), charge.end(), 0.)
                                 / static_cast<double>(charge.size());
            for (size_t idx = 0; idx < charge.size(); idx++) {
                INFO("idx=" << idx << " grid=" << width << "x" << height);
                REQUIRE(residual[idx] == Catch::Approx(charge[idx] - charge_mean).margin(2e-10));
            }

            std::vector<double> other(charge.size());
            for (size_t idx = 0; idx < other.size(); idx++)
                other[idx] = std::cos(0.31 * (idx + 2)) - 0.07 * static_cast<double>(idx % 5);
            std::vector<double> other_potential;
            solve_neumann_poisson_dct(other, width, height, other_potential);
            double cross_ab = std::inner_product(charge.begin(), charge.end(), other_potential.begin(), 0.);
            double cross_ba = std::inner_product(other.begin(), other.end(), potential.begin(), 0.);
            REQUIRE(cross_ab == Catch::Approx(cross_ba).margin(2e-10));

            if (charge.size() > 1) {
                constexpr double h = 1e-5;
                std::vector<double> plus = charge;
                std::vector<double> minus = charge;
                size_t probe = charge.size() / 2;
                plus[probe] += h;
                minus[probe] -= h;
                double finite_difference = (poisson_energy(plus, width, height)
                                            - poisson_energy(minus, width, height))
                                           / (2. * h);
                REQUIRE(finite_difference == Catch::Approx(potential[probe]).margin(2e-8));
            }
        }
    }
}

TEST_CASE("bilinear density stencil conserves mass and differentiates its interpolant", "[vpr_ap][density_gradient]") {
    const std::vector<std::pair<size_t, size_t>> shapes = {
        {1, 1},
        {1, 6},
        {7, 1},
        {2, 2},
        {5, 4},
    };
    for (const auto& [width, height] : shapes) {
        DYNAMIC_SECTION("grid " << width << "x" << height) {
            const std::vector<std::pair<double, double>> coordinates = {
                {-0.5, -0.25},
                {0., 0.},
                {0.27, 0.61},
                {static_cast<double>(width) - 1.0001,
                 static_cast<double>(height) - 1.0001},
                {static_cast<double>(width) + 0.5,
                 static_cast<double>(height) + 0.25},
            };
            for (const auto& [x, y] : coordinates) {
                BilinearDensityStencil stencil = make_bilinear_density_stencil(x, y, width, height);
                std::vector<double> grid(width * height, 0.);
                deposit_bilinear_density(grid, 0, width, height, stencil, 3.25);
                REQUIRE(std::accumulate(grid.begin(), grid.end(), 0.)
                        == Catch::Approx(3.25).margin(1e-14));
            }

            std::vector<double> field(width * height);
            for (size_t idx = 0; idx < field.size(); idx++)
                field[idx] = std::sin(0.41 * (idx + 1)) + 0.2 * static_cast<double>(idx);

            double x = width > 1 ? 0.37 : 0.;
            double y = height > 1 ? 0.43 : 0.;
            BilinearDensityStencil stencil = make_bilinear_density_stencil(x, y, width, height);
            auto [analytic_x, analytic_y] = gradient_bilinear_density(field, 0, width, height, stencil);
            constexpr double h = 1e-6;
            if (width > 1) {
                double plus = interpolate_bilinear_density(field,
                                                           0,
                                                           width,
                                                           height,
                                                           make_bilinear_density_stencil(x + h, y, width, height));
                double minus = interpolate_bilinear_density(field,
                                                            0,
                                                            width,
                                                            height,
                                                            make_bilinear_density_stencil(x - h, y, width, height));
                REQUIRE((plus - minus) / (2. * h) == Catch::Approx(analytic_x).margin(2e-10));
            } else {
                REQUIRE(analytic_x == 0.);
            }
            if (height > 1) {
                double plus = interpolate_bilinear_density(field,
                                                           0,
                                                           width,
                                                           height,
                                                           make_bilinear_density_stencil(x, y + h, width, height));
                double minus = interpolate_bilinear_density(field,
                                                            0,
                                                            width,
                                                            height,
                                                            make_bilinear_density_stencil(x, y - h, width, height));
                REQUIRE((plus - minus) / (2. * h) == Catch::Approx(analytic_y).margin(2e-10));
            } else {
                REQUIRE(analytic_y == 0.);
            }
        }
    }
}

TEST_CASE("bilinear density stencil uses documented one-sided derivatives at kinks", "[vpr_ap][density_gradient]") {
    constexpr size_t width = 5;
    constexpr size_t height = 4;
    std::vector<double> field(width * height);
    for (size_t idx = 0; idx < field.size(); idx++)
        field[idx] = std::cos(0.23 * (idx + 1)) + 0.11 * static_cast<double>(idx);

    constexpr double h = 1e-7;
    double x = 2.;
    double y = 1.37;
    BilinearDensityStencil stencil = make_bilinear_density_stencil(x, y, width, height);
    double value = interpolate_bilinear_density(field, 0, width, height, stencil);
    double plus = interpolate_bilinear_density(field,
                                               0,
                                               width,
                                               height,
                                               make_bilinear_density_stencil(x + h, y, width, height));
    auto [analytic_x, analytic_y] = gradient_bilinear_density(field, 0, width, height, stencil);
    REQUIRE((plus - value) / h == Catch::Approx(analytic_x).margin(2e-8));
    REQUIRE(std::isfinite(analytic_y));

    BilinearDensityStencil upper_plateau = make_bilinear_density_stencil(width - 0.25,
                                                                         height - 0.4,
                                                                         width,
                                                                         height);
    auto [plateau_x, plateau_y] = gradient_bilinear_density(field,
                                                            0,
                                                            width,
                                                            height,
                                                            upper_plateau);
    REQUIRE(plateau_x == 0.);
    REQUIRE(plateau_y == 0.);
}

TEST_CASE("density charge rebalancing has fixed architectural membership", "[vpr_ap][density_gradient]") {
    std::vector<double> charge = {2., -1., 0.5, 4., -2., 1.25};
    std::vector<double> target = {1., 0., 0.25, 0., 2., 0.};
    std::vector<double> original = charge;
    REQUIRE(rebalance_density_charge_on_capacity_sites(charge, target, kCapacityEpsilon) == 3);
    REQUIRE(std::accumulate(charge.begin(), charge.end(), 0.) == Catch::Approx(0.).margin(1e-14));
    for (size_t idx : {size_t{1}, size_t{3}, size_t{5}})
        REQUIRE(charge[idx] == original[idx]);

    std::vector<double> no_capacity(charge.size(), 0.);
    charge = original;
    REQUIRE(rebalance_density_charge_on_capacity_sites(charge, no_capacity, kCapacityEpsilon) == 0);
    REQUIRE(charge == original);

    SECTION("capacity-mask rebalancing is distinguished from uniform Poisson DC projection") {
        constexpr size_t width = 3;
        constexpr size_t height = 2;
        std::vector<double> raw = {2., -1., 0.5, 4., -2., 1.25};
        std::vector<double> uniform_dc_removed = raw;
        double mean = std::accumulate(raw.begin(), raw.end(), 0.) / raw.size();
        for (double& value : uniform_dc_removed)
            value -= mean;

        std::vector<double> capacity_rebalanced = raw;
        rebalance_density_charge_on_capacity_sites(capacity_rebalanced,
                                                   target,
                                                   kCapacityEpsilon);

        std::vector<double> raw_potential;
        std::vector<double> uniform_potential;
        std::vector<double> rebalanced_potential;
        solve_neumann_poisson_dct(raw, width, height, raw_potential);
        solve_neumann_poisson_dct(uniform_dc_removed, width, height, uniform_potential);
        solve_neumann_poisson_dct(capacity_rebalanced, width, height, rebalanced_potential);

        // Uniform DC projection is already performed by the Poisson solve.
        for (size_t idx = 0; idx < raw_potential.size(); idx++)
            REQUIRE(raw_potential[idx] == Catch::Approx(uniform_potential[idx]).margin(2e-13));

        // Subtracting only on a sparse capacity mask changes spatial modes and
        // therefore deliberately changes the objective/force field.
        double max_field_change = 0.;
        for (size_t idx = 0; idx < raw_potential.size(); idx++)
            max_field_change = std::max(max_field_change,
                                        std::abs(raw_potential[idx] - rebalanced_potential[idx]));
        REQUIRE(max_field_change > 0.1);
    }
}

TEST_CASE("Poisson DC projection is independent for every device layer", "[vpr_ap][density_gradient]") {
    constexpr size_t width = 4;
    constexpr size_t height = 3;
    constexpr size_t layers = 3;
    constexpr size_t layer_size = width * height;
    std::vector<double> charge(layer_size * layers);
    for (size_t idx = 0; idx < charge.size(); idx++)
        charge[idx] = std::sin(0.37 * (idx + 1));

    for (size_t layer = 0; layer < layers; layer++) {
        std::vector<double> base(charge.begin() + layer * layer_size,
                                 charge.begin() + (layer + 1) * layer_size);
        std::vector<double> shifted = base;
        double layer_offset = 10. * static_cast<double>(layer + 1);
        for (double& value : shifted)
            value += layer_offset;

        std::vector<double> base_potential;
        std::vector<double> shifted_potential;
        solve_neumann_poisson_dct(base, width, height, base_potential);
        solve_neumann_poisson_dct(shifted, width, height, shifted_potential);
        for (size_t idx = 0; idx < layer_size; idx++) {
            INFO("layer=" << layer << " idx=" << idx);
            REQUIRE(base_potential[idx] == Catch::Approx(shifted_potential[idx]).margin(2e-12));
        }
    }
}

TEST_CASE("full electrostatic gradient covers architectural capacity masks and layers", "[vpr_ap][density_gradient]") {
    SECTION("uniform capacity and mixed masses") {
        size_t width = 5, height = 4, layers = 1;
        std::vector<double> target(width * height * layers, 1.);
        std::vector<SyntheticParticle> particles = {
            {0.31, 0.47, 0, 1.0},
            {2.62, 1.28, 0, 2.0},
            {3.41, 2.53, 0, 0.35},
        };
        require_synthetic_gradients(width, height, layers, target, particles);
    }

    SECTION("checkerboard holes and fractional capacities") {
        size_t width = 7, height = 5, layers = 1;
        std::vector<double> target(width * height * layers, 0.);
        for (size_t y = 0; y < height; y++) {
            for (size_t x = 0; x < width; x++) {
                if ((x + 2 * y) % 3 != 0)
                    target[site_index(0, x, y, width, height)] = 0.2 + 0.15 * ((x + y) % 4);
            }
        }
        std::vector<SyntheticParticle> particles = {
            {0.42, 0.36, 0, 0.8},
            {2.37, 1.64, 0, 1.7},
            {5.29, 3.18, 0, 2.4},
        };
        require_synthetic_gradients(width, height, layers, target, particles);
    }

    SECTION("scarce resource column and isolated sites") {
        size_t width = 9, height = 6, layers = 1;
        std::vector<double> target(width * height * layers, 0.);
        for (size_t y = 0; y < height; y++)
            target[site_index(0, 4, y, width, height)] = 1.;
        target[site_index(0, 1, 1, width, height)] = 0.5;
        target[site_index(0, 7, 4, width, height)] = 0.75;
        std::vector<SyntheticParticle> particles = {
            {0.33, 0.72, 0, 1.0},
            {3.46, 2.21, 0, 1.5},
            {6.58, 4.39, 0, 0.6},
        };
        require_synthetic_gradients(width, height, layers, target, particles);
    }

    SECTION("heterogeneous multilayer capacity") {
        size_t width = 6, height = 5, layers = 3;
        std::vector<double> target(width * height * layers, 0.);
        for (size_t y = 0; y < height; y++) {
            for (size_t x = 0; x < width; x++) {
                target[site_index(0, x, y, width, height)] = 1.;
                if (x == 2)
                    target[site_index(1, x, y, width, height)] = 0.5;
                if ((x == 1 && y == 1) || (x == 4 && y == 3))
                    target[site_index(2, x, y, width, height)] = 1.5;
            }
        }
        std::vector<SyntheticParticle> particles = {
            {0.27, 0.63, 0, 1.0},
            {3.52, 2.34, 0, 1.8},
            {1.43, 1.72, 1, 0.7},
            {4.36, 3.26, 1, 2.1},
            {2.22, 0.41, 2, 1.3},
            {4.51, 3.62, 2, 0.4},
        };
        require_synthetic_gradients(width, height, layers, target, particles);
    }

    SECTION("degenerate one-dimensional and one-site grids") {
        {
            size_t width = 1, height = 7, layers = 2;
            std::vector<double> target(width * height * layers, 0.);
            for (size_t y = 0; y < height; y++)
                target[site_index(y % 2, 0, y, width, height)] = 0.5;
            std::vector<SyntheticParticle> particles = {
                {0., 0.37, 0, 1.0},
                {0., 4.28, 1, 1.6},
            };
            require_synthetic_gradients(width, height, layers, target, particles);
            SyntheticEvaluation evaluation = evaluate_synthetic_density(width,
                                                                        height,
                                                                        layers,
                                                                        target,
                                                                        particles);
            for (const auto& gradient : evaluation.gradients)
                REQUIRE(gradient.first == 0.);
        }
        {
            size_t width = 8, height = 1, layers = 1;
            std::vector<double> target(width * height * layers, 0.);
            target[2] = 1.;
            target[6] = 0.5;
            std::vector<SyntheticParticle> particles = {
                {1.37, 0., 0, 1.0},
                {5.42, 0., 0, 0.8},
            };
            require_synthetic_gradients(width, height, layers, target, particles);
            SyntheticEvaluation evaluation = evaluate_synthetic_density(width,
                                                                        height,
                                                                        layers,
                                                                        target,
                                                                        particles);
            for (const auto& gradient : evaluation.gradients)
                REQUIRE(gradient.second == 0.);
        }
        {
            std::vector<double> target = {1.};
            std::vector<SyntheticParticle> particles = {{0., 0., 0, 2.}};
            SyntheticEvaluation evaluation = evaluate_synthetic_density(1, 1, 1, target, particles);
            REQUIRE(evaluation.energy == Catch::Approx(0.).margin(1e-14));
            REQUIRE(evaluation.gradients.front().first == 0.);
            REQUIRE(evaluation.gradients.front().second == 0.);
        }
    }

    SECTION("no capacity is numerically defined for invalid architecture diagnostics") {
        size_t width = 4, height = 3, layers = 2;
        std::vector<double> target(width * height * layers, 0.);
        std::vector<SyntheticParticle> particles = {
            {0.38, 0.51, 0, 1.0},
            {2.44, 1.37, 1, 0.9},
        };
        require_synthetic_gradients(width, height, layers, target, particles);
    }
}

} // namespace
