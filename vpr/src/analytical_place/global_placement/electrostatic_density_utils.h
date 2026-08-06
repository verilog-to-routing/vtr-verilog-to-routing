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
 * @brief Per-resource electrostatic field grid (elfPlace's per-resource bin grid B^s).
 *
 * The device tile grid is the natural domain for an abundant resource, but not
 * for a scarce one: a resource that only exists every eighth column has seven
 * capacity-free tile columns between each pair of legal sites, and those columns
 * still take part in a full-domain Poisson solve. The resulting field carries
 * structure finer than the resource can physically realize, and the residual
 * charge at a capacity-free site is an artifact of the grid rather than of the
 * placement.
 *
 * This grid instead resolves one resource at the pitch of that resource's own
 * capacity, so (nearly) every bin of the field domain can actually hold the
 * resource. Continuous tile coordinates map onto it affinely,
 * `u = x * scale_x`, which is exactly the identity when the selected stride is
 * one -- i.e. abundant resources keep the tile grid and the previous field
 * unchanged.
 *
 * Charge, potential, and energy stay in bin units on this grid, exactly as they
 * were in tile units on the tile grid. Only the domain changes; @ref spacing_x /
 * @ref spacing_y record the resulting bin pitch for reporting, and the tile ->
 * bin scales enter a placement gradient through the chain rule.
 */
struct ResourceFieldGrid {
    size_t width = 1;                    ///< Number of field bins along x.
    size_t height = 1;                   ///< Number of field bins along y.
    size_t num_layers = 1;               ///< Device layers (never coarsened).
    double scale_x = 1.;                 ///< Tile x coordinate -> field bin x coordinate.
    double scale_y = 1.;                 ///< Tile y coordinate -> field bin y coordinate.
    double spacing_x = 1.;               ///< Tile pitch of one field bin along x (1 / scale_x); reporting only.
    double spacing_y = 1.;               ///< Tile pitch of one field bin along y (1 / scale_y); reporting only.
    std::vector<double> target_capacity; ///< [layer][y][x] target capacity aggregated onto the field bins.
    double charge_scale = 1.;            ///< Mean capacity of a capacity-bearing field bin (residual-charge unit).
    double target_norm_floor = 0.;       ///< Floor applied when dividing by a field bin's capacity.
};

/**
 * @brief Choose an axis's bin stride from the resource's own capacity pitch.
 *
 * The stride is the mean spacing between capacity-bearing tiles along the axis,
 * so a resource that exists nearly everywhere along it keeps the tile grid
 * (stride one) and a resource living in every eighth column is resolved at that
 * column pitch. This is self-calibrating: no occupancy threshold has to be
 * chosen, and an abundant resource with a few holes (an empty perimeter, an
 * interleaved hard-block column) is not coarsened for them.
 *
 * @param axis_has_capacity Per tile index along one axis: does any site there hold the resource.
 * @param min_bins          Lower bound on the resulting bin count, capping how coarse the axis can get.
 */
size_t select_field_grid_stride(const std::vector<bool>& axis_has_capacity,
                                size_t min_bins);

/**
 * @brief Build the per-resource field grid for one resource's tile-grid capacity.
 *
 * @param fine_target_capacity  [layer][y][x] per-tile target capacity for this resource.
 * @param capacity_epsilon      Capacity above which a site counts as usable.
 * @param target_floor_fraction Fraction of the mean bin capacity used as a division floor.
 */
ResourceFieldGrid build_resource_field_grid(const std::vector<double>& fine_target_capacity,
                                            size_t width,
                                            size_t height,
                                            size_t num_layers,
                                            double capacity_epsilon,
                                            double target_floor_fraction,
                                            size_t min_bins);

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
