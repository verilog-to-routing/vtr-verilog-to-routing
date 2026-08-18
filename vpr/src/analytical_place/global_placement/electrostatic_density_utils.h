#pragma once

#include <cstddef>
#include <utility>
#include <vector>

/**
 * @brief Offset from the upper device extent used by smooth density coordinates.
 */
constexpr double kDensityDeviceBoundaryEpsilon = 1e-6;

/**
 * @brief Bilinear grid support and weights for one continuous density coordinate.
 *
 * The derivative weights describe the one-sided derivative selected by the cell
 * containing the projected coordinate. They are zero on a degenerate dimension
 * and outside the projected device interval.
 */
struct BilinearDensityStencil {
    size_t xs[2] = {0, 0};    ///< Supporting x indices.
    size_t ys[2] = {0, 0};    ///< Supporting y indices.
    double wx[2] = {1., 0.};  ///< Bilinear x weights.
    double wy[2] = {1., 0.};  ///< Bilinear y weights.
    double dwx[2] = {0., 0.}; ///< Derivatives of x weights.
    double dwy[2] = {0., 0.}; ///< Derivatives of y weights.
};

/**
 * @brief Construct the bilinear stencil for a continuous device coordinate.
 *
 * Coordinates are projected into the valid smooth-density domain. Width and
 * height must both be non-zero.
 */
BilinearDensityStencil make_bilinear_density_stencil(double x,
                                                     double y,
                                                     size_t width,
                                                     size_t height);

/**
 * @brief Deposit mass through a bilinear stencil into one flattened layer grid.
 */
void deposit_bilinear_density(std::vector<double>& grid,
                              size_t layer,
                              size_t width,
                              size_t height,
                              const BilinearDensityStencil& stencil,
                              double mass);

/**
 * @brief Interpolate a flattened grid through a bilinear stencil.
 */
double interpolate_bilinear_density(const std::vector<double>& grid,
                                    size_t layer,
                                    size_t width,
                                    size_t height,
                                    const BilinearDensityStencil& stencil);

/**
 * @brief Differentiate the bilinear interpolant selected by a density stencil.
 */
std::pair<double, double> gradient_bilinear_density(const std::vector<double>& grid,
                                                    size_t layer,
                                                    size_t width,
                                                    size_t height,
                                                    const BilinearDensityStencil& stencil);

/**
 * @brief Rebalance total charge over the fixed positive-capacity-site mask.
 *
 * This makes the total charge zero without placement-dependent mask membership.
 * Because the subtraction is non-uniform when the capacity mask has holes, this
 * is an objective-shaping operation, not merely removal of Poisson's uniform DC
 * mode. The Poisson solver projects each layer's uniform DC mode independently.
 *
 * @return Number of capacity sites used for DC removal.
 */
size_t rebalance_density_charge_on_capacity_sites(std::vector<double>& charge,
                                                  const std::vector<double>& target_capacity,
                                                  double capacity_epsilon);

/**
 * @brief Solve the two-dimensional Neumann Poisson equation using separable DCTs.
 *
 * The input need not be neutral: the independent DC mode is projected out.
 */
void solve_neumann_poisson_dct(const std::vector<double>& charge,
                               size_t width,
                               size_t height,
                               std::vector<double>& potential);
