/**
 * @file
 * @brief Testable numerical primitives for electrostatic analytical placement.
 */

#include "electrostatic_density_utils.h"

#include <algorithm>
#include <cmath>
#include <complex>

#include <unsupported/Eigen/FFT>

#include "vtr_assert.h"

namespace {

constexpr double kPi = 3.141592653589793238462643383279502884;
// Keep the production solver's historical zero-mode threshold exactly. Device
// grids are far from this cutoff, but preserving it avoids an unintended model
// change while extracting the implementation into a testable utility.
constexpr double kPoissonEigenvalueEpsilon = 1e-9;

/**
 * @brief Reusable storage for the separable DCT Poisson solve.
 */
struct PoissonDctWorkspace {
    Eigen::FFT<double> fft;
    std::vector<double> extension;
    std::vector<std::complex<double>> fft_spectrum;
    std::vector<double> row_transformed;
    std::vector<double> row;
    std::vector<double> transformed_row;
    std::vector<double> poisson_spectrum;
    std::vector<double> column;
    std::vector<double> transformed_column;
    std::vector<double> column_inverse;
};

/**
 * @brief Apply an unnormalized DCT-II through an even FFT extension.
 */
void dct_ii(const std::vector<double>& input,
            std::vector<double>& output,
            PoissonDctWorkspace& workspace) {
    VTR_ASSERT(!input.empty());

    size_t size = input.size();
    workspace.extension.resize(2 * size);
    for (size_t idx = 0; idx < size; idx++) {
        workspace.extension[idx] = input[idx];
        workspace.extension[2 * size - 1 - idx] = input[idx];
    }

    workspace.fft.fwd(workspace.fft_spectrum, workspace.extension);

    output.resize(size);
    for (size_t frequency = 0; frequency < size; frequency++) {
        double angle = kPi * frequency / (2. * size);
        std::complex<double> phase(std::cos(angle), -std::sin(angle));
        output[frequency] = 0.5 * std::real(phase * workspace.fft_spectrum[frequency]);
    }
}

/**
 * @brief Apply the inverse of the unnormalized DCT-II through an inverse FFT.
 */
void idct_iii(const std::vector<double>& input,
              std::vector<double>& output,
              PoissonDctWorkspace& workspace) {
    VTR_ASSERT(!input.empty());

    size_t size = input.size();
    workspace.fft_spectrum.assign(2 * size, 0.);
    for (size_t frequency = 0; frequency < size; frequency++) {
        double angle = kPi * frequency / (2. * size);
        std::complex<double> phase(std::cos(angle), std::sin(angle));
        workspace.fft_spectrum[frequency] = 2. * phase * input[frequency];
        if (frequency != 0)
            workspace.fft_spectrum[2 * size - frequency] = std::conj(workspace.fft_spectrum[frequency]);
    }

    workspace.fft.inv(workspace.extension, workspace.fft_spectrum);
    output.assign(workspace.extension.begin(), workspace.extension.begin() + size);
}

/**
 * @brief Flatten a layer and two-dimensional coordinate.
 */
size_t density_site_index(size_t layer,
                          size_t x,
                          size_t y,
                          size_t width,
                          size_t height) {
    return (layer * height + y) * width + x;
}

} // namespace

BilinearDensityStencil make_bilinear_density_stencil(double x,
                                                      double y,
                                                      size_t width,
                                                      size_t height) {
    VTR_ASSERT(width > 0);
    VTR_ASSERT(height > 0);

    double max_x = std::max(0., static_cast<double>(width) - kDensityDeviceBoundaryEpsilon);
    double max_y = std::max(0., static_cast<double>(height) - kDensityDeviceBoundaryEpsilon);
    bool x_outside = x < 0. || x > max_x;
    bool y_outside = y < 0. || y > max_y;
    x = std::clamp(x, 0., max_x);
    y = std::clamp(y, 0., max_y);

    BilinearDensityStencil stencil;
    stencil.xs[0] = static_cast<size_t>(std::floor(x));
    stencil.ys[0] = static_cast<size_t>(std::floor(y));
    stencil.xs[1] = std::min(stencil.xs[0] + 1, width - 1);
    stencil.ys[1] = std::min(stencil.ys[0] + 1, height - 1);

    double fx = x - stencil.xs[0];
    double fy = y - stencil.ys[0];
    stencil.wx[0] = 1. - fx;
    stencil.wx[1] = fx;
    stencil.wy[0] = 1. - fy;
    stencil.wy[1] = fy;

    if (stencil.xs[0] != stencil.xs[1] && !x_outside) {
        stencil.dwx[0] = -1.;
        stencil.dwx[1] = 1.;
    } else {
        stencil.wx[0] = 1.;
        stencil.wx[1] = 0.;
    }
    if (stencil.ys[0] != stencil.ys[1] && !y_outside) {
        stencil.dwy[0] = -1.;
        stencil.dwy[1] = 1.;
    } else {
        stencil.wy[0] = 1.;
        stencil.wy[1] = 0.;
    }
    return stencil;
}

void deposit_bilinear_density(std::vector<double>& grid,
                              size_t layer,
                              size_t width,
                              size_t height,
                              const BilinearDensityStencil& stencil,
                              double mass) {
    VTR_ASSERT(width > 0);
    VTR_ASSERT(height > 0);
    VTR_ASSERT(grid.size() >= (layer + 1) * width * height);
    for (size_t xi = 0; xi < 2; xi++) {
        for (size_t yi = 0; yi < 2; yi++) {
            double weight = stencil.wx[xi] * stencil.wy[yi];
            if (weight != 0.) {
                size_t idx = density_site_index(layer, stencil.xs[xi], stencil.ys[yi], width, height);
                grid[idx] += mass * weight;
            }
        }
    }
}

double interpolate_bilinear_density(const std::vector<double>& grid,
                                    size_t layer,
                                    size_t width,
                                    size_t height,
                                    const BilinearDensityStencil& stencil) {
    VTR_ASSERT(width > 0);
    VTR_ASSERT(height > 0);
    VTR_ASSERT(grid.size() >= (layer + 1) * width * height);
    double value = 0.;
    for (size_t xi = 0; xi < 2; xi++) {
        for (size_t yi = 0; yi < 2; yi++) {
            size_t idx = density_site_index(layer, stencil.xs[xi], stencil.ys[yi], width, height);
            value += stencil.wx[xi] * stencil.wy[yi] * grid[idx];
        }
    }
    return value;
}

std::pair<double, double> gradient_bilinear_density(const std::vector<double>& grid,
                                                    size_t layer,
                                                    size_t width,
                                                    size_t height,
                                                    const BilinearDensityStencil& stencil) {
    VTR_ASSERT(width > 0);
    VTR_ASSERT(height > 0);
    VTR_ASSERT(grid.size() >= (layer + 1) * width * height);
    double phi00 = grid[density_site_index(layer, stencil.xs[0], stencil.ys[0], width, height)];
    double phi10 = grid[density_site_index(layer, stencil.xs[1], stencil.ys[0], width, height)];
    double phi01 = grid[density_site_index(layer, stencil.xs[0], stencil.ys[1], width, height)];
    double phi11 = grid[density_site_index(layer, stencil.xs[1], stencil.ys[1], width, height)];

    // Preserve the production placer's original arithmetic order on ordinary
    // in-bounds cells. The derivative is zero only where projection makes the
    // interpolant constant (outside the domain or in a degenerate dimension).
    double dx = stencil.dwx[1] == 0.
                    ? 0.
                    : stencil.wy[0] * (phi10 - phi00) + stencil.wy[1] * (phi11 - phi01);
    double dy = stencil.dwy[1] == 0.
                    ? 0.
                    : stencil.wx[0] * (phi01 - phi00) + stencil.wx[1] * (phi11 - phi10);
    return {dx, dy};
}

size_t rebalance_density_charge_on_capacity_sites(std::vector<double>& charge,
                                                  const std::vector<double>& target_capacity,
                                                  double capacity_epsilon) {
    VTR_ASSERT(charge.size() == target_capacity.size());
    double charge_sum = 0.;
    size_t capacity_site_count = 0;
    for (size_t idx = 0; idx < charge.size(); idx++) {
        charge_sum += charge[idx];
        if (target_capacity[idx] > capacity_epsilon)
            capacity_site_count++;
    }
    if (capacity_site_count == 0)
        return 0;

    double mean_charge = charge_sum / capacity_site_count;
    for (size_t idx = 0; idx < charge.size(); idx++) {
        if (target_capacity[idx] > capacity_epsilon)
            charge[idx] -= mean_charge;
    }
    return capacity_site_count;
}

void solve_neumann_poisson_dct(const std::vector<double>& charge,
                               size_t width,
                               size_t height,
                               std::vector<double>& potential) {
    VTR_ASSERT(width > 0);
    VTR_ASSERT(height > 0);
    VTR_ASSERT(charge.size() == width * height);

    thread_local PoissonDctWorkspace workspace;
    workspace.row_transformed.resize(width * height);
    workspace.row.resize(width);
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++)
            workspace.row[x] = charge[y * width + x];
        dct_ii(workspace.row, workspace.transformed_row, workspace);
        for (size_t x = 0; x < width; x++)
            workspace.row_transformed[y * width + x] = workspace.transformed_row[x];
    }

    workspace.poisson_spectrum.resize(width * height);
    workspace.column.resize(height);
    for (size_t x = 0; x < width; x++) {
        for (size_t y = 0; y < height; y++)
            workspace.column[y] = workspace.row_transformed[y * width + x];
        dct_ii(workspace.column, workspace.transformed_column, workspace);
        for (size_t y = 0; y < height; y++)
            workspace.poisson_spectrum[y * width + x] = workspace.transformed_column[y];
    }

    for (size_t y_frequency = 0; y_frequency < height; y_frequency++) {
        double y_eigenvalue = 2. * (1. - std::cos(kPi * y_frequency / height));
        for (size_t x_frequency = 0; x_frequency < width; x_frequency++) {
            size_t idx = y_frequency * width + x_frequency;
            double x_eigenvalue = 2. * (1. - std::cos(kPi * x_frequency / width));
            double eigenvalue = x_eigenvalue + y_eigenvalue;
            workspace.poisson_spectrum[idx] = eigenvalue > kPoissonEigenvalueEpsilon
                                                  ? workspace.poisson_spectrum[idx] / eigenvalue
                                                  : 0.;
        }
    }

    workspace.column_inverse.resize(width * height);
    for (size_t x = 0; x < width; x++) {
        for (size_t y = 0; y < height; y++)
            workspace.column[y] = workspace.poisson_spectrum[y * width + x];
        idct_iii(workspace.column, workspace.transformed_column, workspace);
        for (size_t y = 0; y < height; y++)
            workspace.column_inverse[y * width + x] = workspace.transformed_column[y];
    }

    potential.resize(width * height);
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++)
            workspace.row[x] = workspace.column_inverse[y * width + x];
        idct_iii(workspace.row, workspace.transformed_row, workspace);
        for (size_t x = 0; x < width; x++)
            potential[y * width + x] = workspace.transformed_row[x];
    }
}
