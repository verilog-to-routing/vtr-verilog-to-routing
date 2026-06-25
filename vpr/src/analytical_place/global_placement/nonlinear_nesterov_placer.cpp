/**
 * @file
 * @author  William Zhang
 * @date    June 2026
 * @brief   Implementation of a nonlinear Nesterov analytical global placer.
 */

#include "nonlinear_nesterov_placer.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <complex>
#include <functional>
#include <limits>
#include <optional>
#include <vector>
#include <unsupported/Eigen/FFT>
#include "PreClusterTimingManager.h"
#include "ap_netlist.h"
#include "atom_netlist.h"
#include "device_grid.h"
#include "flat_placement_bins.h"
#include "logic_types.h"
#include "partial_placement.h"
#include "physical_types.h"
#include "place_delay_model.h"
#include "prepack.h"
#include "primitive_vector.h"
#include "router_lookahead_constants.h"
#include "timing_info.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace {

/**
 * @brief Maximum number of accelerated first-order iterations.
 */
constexpr size_t kMaxNesterovIterations = 200;

/**
 * @brief Number of optimization/legalization epochs in the nonlinear Nesterov placer.
 */
constexpr size_t kNesterovEpochs = 4;

/**
 * @brief Number of first-order iterations performed before each legalization.
 */
constexpr size_t kNesterovIterationsPerEpoch = kMaxNesterovIterations / kNesterovEpochs;

/**
 * @brief Minimum inner iterations before displacement-based convergence may stop an epoch.
 */
constexpr size_t kMinNesterovIterationsPerEpoch = 5;

/**
 * @brief Proximity weight added per tile of mean partial-legalization displacement.
 */
constexpr double kProximityWeightPerLegalizationTile = 0.05;

/**
 * @brief Maximum legalizer-feedback proximity weight.
 */
constexpr double kMaxLegalizerFeedbackProximityWeight = 2.0;

/**
 * @brief Fraction of the prior legalizer-feedback penalty retained for the next epoch.
 */
constexpr double kLegalizerFeedbackRetention = 0.5;

/**
 * @brief Minimum line-search step size before accepting a non-improving move.
 */
constexpr double kMinStepSize = 1e-6;

/**
 * @brief Convergence threshold as a fraction of the larger device dimension.
 */
constexpr double kConvergenceDisplacementFraction = 1e-4;

/**
 * @brief Absolute lower bound on the displacement convergence threshold.
 */
constexpr double kMinConvergenceDisplacement = 1e-3;

/**
 * @brief Maximum fraction of the device span a block should move in one step.
 */
constexpr double kInitialStepSpanFraction = 0.02;

/**
 * @brief Smooth wirelength gamma as a fraction of the larger device dimension.
 */
constexpr double kWirelengthGammaFraction = 0.02;

/**
 * @brief Initial target ratio of density pressure to wirelength pressure.
 */
constexpr double kInitialDensityToWirelengthRatio = 0.05;

/**
 * @brief Density weight multiplier applied after each accepted iteration.
 */
constexpr double kDensityWeightGrowth = 1.03;

/**
 * @brief Largest density weight multiplier relative to its initialized value.
 */
constexpr double kMaxDensityWeightGrowth = 50.0;

/**
 * @brief Read a double tunable from an environment variable, falling back to a default.
 *
 * Used during the elfPlace/RePlAce porting study so density tunables can be swept
 * without recompiling. Parsed once at first use.
 */
double env_or(const char* name, double fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr)
        return fallback;
    char* end = nullptr;
    double parsed = std::strtod(value, &end);
    return end == value ? fallback : parsed;
}

/**
 * @brief Scale on the legalizer-feedback proximity weight.
 *
 * The legalizer-feedback proximity anchor helps large designs (it closes a large
 * legalization gap) but suppresses timing- and wirelength-driven motion on small
 * designs. A sweep across MCNC, VTR, and Titan showed that scaling the anchor to
 * 0.25 on small designs improved both suites (WL -5.3%, CPD -0.3%) while any
 * reduction regressed Titan wirelength. The scale is therefore applied only below
 * @ref kProximitySizeThreshold movable blocks; larger designs keep the full anchor.
 */
const double kProximityScale = env_or("VTR_NESTEROV_PROXIMITY", 0.25);

/**
 * @brief Movable-block count at or above which the full proximity anchor is kept.
 */
const size_t kProximitySizeThreshold = static_cast<size_t>(env_or("VTR_NESTEROV_PROXIMITY_SIZE", 30000.0));

/**
 * @brief Maximum filler-particle displacement per iteration as a fraction of the device span.
 */
const double kFillerMoveSpanFraction = env_or("VTR_NESTEROV_FILLER_MOVE", 0.05);

/**
 * @brief Absolute cap (in tiles) on the per-iteration filler move.
 *
 * The span-relative move rate is ~0.05*span, which is small on MCNC/VTR but large
 * enough on Titan-scale grids that the filler field never settles within the
 * iteration budget (the observed Titan over-spread). This caps the move so filler
 * dynamics converge on large devices while leaving small devices unchanged.
 */
const double kFillerMoveAbsoluteCap = env_or("VTR_NESTEROV_FILLER_MOVE_CAP", 2.0);

/**
 * @brief Fraction of available whitespace mass converted into mobile fillers.
 *
 * Full fillers (1.0) close the whole legalization gap but remove the spreading
 * pressure that keeps timing-critical blocks apart. A sweep found fillers help
 * VTR wirelength but cost MCNC and regress large-device (Titan) wirelength, so
 * they are disabled by default and left as env-tunable infrastructure.
 */
const double kFillerMassFraction = env_or("VTR_NESTEROV_FILLER_MASS", 0.0);

/**
 * @brief Largest number of filler particles created for any single primitive dimension.
 *
 * Large devices need many light fillers for a smooth field; too low a cap makes
 * each filler heavy and spiky, which over-spreads physical blocks (observed as a
 * Titan wirelength regression). Env-tunable for the device-scale study.
 */
const size_t kMaxFillersPerDim = static_cast<size_t>(env_or("VTR_NESTEROV_FILLER_CAP", 400000.0));

/**
 * @brief Mass inflation per unit of above-average block pin density.
 *
 * Disabled by default: a sweep showed pin-density inflation lands on a chaotic,
 * non-robust parameter landscape (good points are isolated spikes surrounded by
 * regressions), so it is left as dormant, env-tunable infrastructure.
 */
const double kAreaInflationAlpha = env_or("VTR_NESTEROV_AREA_ALPHA", 0.0);

/**
 * @brief Largest mass inflation factor applied to any single block.
 */
const double kMaxAreaInflation = env_or("VTR_NESTEROV_AREA_MAX", 1.5);

/**
 * @brief Enable the elfPlace-style diagonal (Jacobi) preconditioner.
 *
 * FPGA netlists are extreme mixed-size: block area (mass) and connectivity span
 * orders of magnitude (LUT vs BRAM vs DSP). A single global step length is
 * throttled by the stiffest block, so light/sparsely-connected blocks barely
 * move. The preconditioner divides each block's gradient by an estimate of its
 * objective curvature -- the sum of incident net weights (wirelength Hessian
 * diagonal) plus the density penalty times block mass (density Hessian diagonal)
 * -- giving every block a near-Newton step regardless of size. This is the
 * elfPlace remedy for mixed-size placement and the suspected fix for the
 * large-device (Titan) over-spread. Env-gated while it is being characterized.
 */
const bool kPreconditionEnable = env_or("VTR_NESTEROV_PRECOND", 0.0) != 0.0;

/**
 * @brief Floor on the per-block preconditioner to avoid dividing by ~0 curvature.
 */
const double kPreconditionFloor = env_or("VTR_NESTEROV_PRECOND_FLOOR", 1.0);

/**
 * @brief Strength exponent on the diagonal preconditioner.
 *
 * The effective preconditioner is curvature^alpha. alpha=1 is the full Jacobi
 * (Newton-diagonal) preconditioner; alpha=0 disables it (uniform); alpha~0.5 is
 * a gentler partial preconditioner that reduces over-correction (large relative
 * steps for low-curvature blocks) while still equalizing across block sizes.
 */
const double kPreconditionStrength = env_or("VTR_NESTEROV_PRECOND_ALPHA", 1.0);

/**
 * @brief Optional cap on the stiffest/softest preconditioner ratio (0 disables).
 *
 * Bounds how much more the most-preconditioned block is damped relative to the
 * least, limiting conditioning span so a few very stiff blocks cannot freeze
 * while everything else races. Applied to the post-exponent value.
 */
const double kPreconditionMaxRatio = env_or("VTR_NESTEROV_PRECOND_RATIO", 0.0);

/**
 * @brief Small value used to avoid division by zero.
 */
constexpr double kEpsilon = 1e-9;

/**
 * @brief Device-bound epsilon to keep floor-based bin lookup inside the grid.
 */
constexpr double kDeviceBoundaryEpsilon = 1e-6;

/**
 * @brief Apply an unnormalized DCT-II through an even FFT extension.
 */
void dct_ii(const std::vector<double>& input, std::vector<double>& output) {
    VTR_ASSERT(!input.empty());

    size_t size = input.size();
    std::vector<double> extension(2 * size);
    for (size_t idx = 0; idx < size; idx++) {
        extension[idx] = input[idx];
        extension[2 * size - 1 - idx] = input[idx];
    }

    Eigen::FFT<double> fft;
    std::vector<std::complex<double>> spectrum;
    fft.fwd(spectrum, extension);

    output.resize(size);
    for (size_t frequency = 0; frequency < size; frequency++) {
        double angle = M_PI * frequency / (2. * size);
        std::complex<double> phase(std::cos(angle), -std::sin(angle));
        output[frequency] = 0.5 * std::real(phase * spectrum[frequency]);
    }
}

/**
 * @brief Apply the inverse of the unnormalized DCT-II through an inverse FFT.
 */
void idct_iii(const std::vector<double>& input, std::vector<double>& output) {
    VTR_ASSERT(!input.empty());

    size_t size = input.size();
    std::vector<std::complex<double>> spectrum(2 * size, 0.);
    for (size_t frequency = 0; frequency < size; frequency++) {
        double angle = M_PI * frequency / (2. * size);
        std::complex<double> phase(std::cos(angle), std::sin(angle));
        spectrum[frequency] = 2. * phase * input[frequency];
        if (frequency != 0)
            spectrum[2 * size - frequency] = std::conj(spectrum[frequency]);
    }

    Eigen::FFT<double> fft;
    std::vector<double> extension;
    fft.inv(extension, spectrum);
    output.assign(extension.begin(), extension.begin() + size);
}

/**
 * @brief Solve the two-dimensional Neumann Poisson equation through separable DCTs.
 *
 * The discrete cosine transform diagonalizes the Laplacian under Neumann (zero
 * normal-derivative) boundaries: transform the charge, divide each spectral
 * coefficient by its eigenvalue, and invert. The zero-frequency (DC) mode has a
 * zero eigenvalue and is left at zero, which fixes the otherwise-undetermined
 * constant offset of the potential.
 */
void solve_neumann_poisson_dct(const std::vector<double>& charge,
                               size_t width,
                               size_t height,
                               std::vector<double>& potential) {
    VTR_ASSERT(charge.size() == width * height);

    std::vector<double> row_transformed(width * height);
    std::vector<double> row(width);
    std::vector<double> transformed_row;
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++)
            row[x] = charge[y * width + x];
        dct_ii(row, transformed_row);
        for (size_t x = 0; x < width; x++)
            row_transformed[y * width + x] = transformed_row[x];
    }

    std::vector<double> spectrum(width * height);
    std::vector<double> column(height);
    std::vector<double> transformed_column;
    for (size_t x = 0; x < width; x++) {
        for (size_t y = 0; y < height; y++)
            column[y] = row_transformed[y * width + x];
        dct_ii(column, transformed_column);
        for (size_t y = 0; y < height; y++)
            spectrum[y * width + x] = transformed_column[y];
    }

    for (size_t y_frequency = 0; y_frequency < height; y_frequency++) {
        double y_eigenvalue = 2. * (1. - std::cos(M_PI * y_frequency / height));
        for (size_t x_frequency = 0; x_frequency < width; x_frequency++) {
            size_t idx = y_frequency * width + x_frequency;
            double x_eigenvalue = 2. * (1. - std::cos(M_PI * x_frequency / width));
            double eigenvalue = x_eigenvalue + y_eigenvalue;
            spectrum[idx] = eigenvalue > kEpsilon ? spectrum[idx] / eigenvalue : 0.;
        }
    }

    std::vector<double> column_inverse(width * height);
    for (size_t x = 0; x < width; x++) {
        for (size_t y = 0; y < height; y++)
            column[y] = spectrum[y * width + x];
        idct_iii(column, transformed_column);
        for (size_t y = 0; y < height; y++)
            column_inverse[y * width + x] = transformed_column[y];
    }

    potential.resize(width * height);
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++)
            row[x] = column_inverse[y * width + x];
        idct_iii(row, transformed_row);
        for (size_t x = 0; x < width; x++)
            potential[y * width + x] = transformed_row[x];
    }
}

/**
 * @brief Evaluate the weighted-average approximation of a coordinate extremum.
 *
 * With @p negate false, this approximates the maximum coordinate. With @p
 * negate true, this approximates the minimum coordinate. The returned weights
 * are reused by the caller to form the analytical derivative.
 *
 * @param values Coordinate values for one net dimension. Must be non-empty.
 * @param gamma Positive smoothing factor: smaller values more closely
 *              approximate the extremum, while larger values smooth it more.
 * @param negate When true, negate each value before computing log-sum-exp to
 *               approximate the negated minimum instead of the maximum.
 * @param weights Optional normalized exponential weights.
 * @return The weighted-average coordinate.
 */
double weighted_average_coordinate(const std::vector<double>& values,
                                   double gamma,
                                   bool negate,
                                   std::vector<double>* weights) {
    VTR_ASSERT(!values.empty());
    VTR_ASSERT(gamma > 0.);

    double max_scaled = negate ? -values.front() / gamma : values.front() / gamma;
    for (double value : values) {
        double scaled_value = negate ? -value / gamma : value / gamma;
        max_scaled = std::max(max_scaled, scaled_value);
    }

    double exp_sum = 0.;
    double weighted_sum = 0.;
    for (double value : values) {
        double scaled_value = negate ? -value / gamma : value / gamma;
        double exponential = std::exp(scaled_value - max_scaled);
        exp_sum += exponential;
        weighted_sum += value * exponential;
    }

    VTR_ASSERT_SAFE(exp_sum > 0.);
    if (weights) {
        weights->resize(values.size());
        for (size_t idx = 0; idx < values.size(); idx++) {
            double scaled_value = negate ? -values[idx] / gamma : values[idx] / gamma;
            (*weights)[idx] = std::exp(scaled_value - max_scaled) / exp_sum;
        }
    }
    return weighted_sum / exp_sum;
}

} // namespace

NonlinearNesterovPlacer::PlacementGradient::PlacementGradient(const APNetlist& ap_netlist)
    : dx(ap_netlist.blocks().size(), 0.)
    , dy(ap_netlist.blocks().size(), 0.) {}

void NonlinearNesterovPlacer::PlacementGradient::clear() {
    std::fill(dx.begin(), dx.end(), 0.);
    std::fill(dy.begin(), dy.end(), 0.);
}

NonlinearNesterovPlacer::NonlinearNesterovPlacer(const APNetlist& ap_netlist,
                                                 const Prepacker& prepacker,
                                                 const AtomNetlist& atom_netlist,
                                                 const DeviceGrid& device_grid,
                                                 const std::vector<t_logical_block_type>& logical_block_types,
                                                 const std::vector<t_physical_tile_type>& physical_tile_types,
                                                 const LogicalModels& models,
                                                 PreClusterTimingManager& pre_cluster_timing_manager,
                                                 std::shared_ptr<PlaceDelayModel> place_delay_model,
                                                 float ap_timing_tradeoff,
                                                 bool generate_mass_report,
                                                 const std::vector<std::string>& target_density_arg_strs,
                                                 e_ap_partial_legalizer partial_legalizer_type,
                                                 int log_verbosity)
    : GlobalPlacer(ap_netlist, log_verbosity)
    , atom_netlist_(atom_netlist)
    , pre_cluster_timing_manager_(pre_cluster_timing_manager)
    , place_delay_model_(place_delay_model)
    , net_weights_(ap_netlist.nets().size(), 1.0)
    , device_grid_width_(device_grid.width())
    , device_grid_height_(device_grid.height())
    , device_grid_num_layers_(device_grid.get_num_layers())
    , ap_timing_tradeoff_(ap_timing_tradeoff) {
    vtr::ScopedStartFinishTimer nonlinear_nesterov_placer_building_timer("Constructing Nonlinear Nesterov Global Placer");

    density_manager_ = std::make_shared<FlatPlacementDensityManager>(ap_netlist_,
                                                                     prepacker,
                                                                     atom_netlist,
                                                                     device_grid,
                                                                     logical_block_types,
                                                                     physical_tile_types,
                                                                     models,
                                                                     target_density_arg_strs,
                                                                     log_verbosity_);
    if (generate_mass_report)
        density_manager_->generate_mass_report();

    partial_legalizer_ = make_partial_legalizer(partial_legalizer_type,
                                                ap_netlist_,
                                                density_manager_,
                                                prepacker,
                                                models,
                                                log_verbosity_);

    optimizable_blocks_.reserve(ap_netlist_.blocks().size());
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        if (block_is_optimizable_(blk_id))
            optimizable_blocks_.push_back(blk_id);
    }
}

bool NonlinearNesterovPlacer::block_is_optimizable_(APBlockId blk_id) const {
    return ap_netlist_.block_mobility(blk_id) == APBlockMobility::MOVEABLE;
}

PartialPlacement NonlinearNesterovPlacer::initialize_placement_() const {
    PartialPlacement p_placement(ap_netlist_);

    if (optimizable_blocks_.empty()) {
        project_placement_(p_placement);
        return p_placement;
    }

    // This is deterministic least-dense spreading, rather than the B2B
    // initializer. B2B spreads only solver-connected blocks in row-ID order
    // and centers disconnected blocks; this optimizer needs a location for
    // every optimizable AP block before its first objective evaluation.
    size_t num_layers = std::max<size_t>(1, device_grid_num_layers_);
    size_t num_blocks_per_layer = std::max<size_t>(1, std::ceil(optimizable_blocks_.size() / static_cast<double>(num_layers)));
    double area_per_block = static_cast<double>(device_grid_width_ * device_grid_height_) / static_cast<double>(num_blocks_per_layer);
    double gap = std::max(1.0, std::sqrt(area_per_block));
    size_t cols = std::max<size_t>(1, std::ceil(device_grid_width_ / gap));

    for (size_t idx = 0; idx < optimizable_blocks_.size(); idx++) {
        APBlockId blk_id = optimizable_blocks_[idx];
        size_t layer = (idx / num_blocks_per_layer) % num_layers;
        size_t layer_idx = idx % num_blocks_per_layer;
        size_t row = layer_idx / cols;
        size_t col = layer_idx % cols;

        p_placement.block_x_locs[blk_id] = std::min<double>((col + 0.5) * gap, device_grid_width_ - kDeviceBoundaryEpsilon);
        p_placement.block_y_locs[blk_id] = std::min<double>((row + 0.5) * gap, device_grid_height_ - kDeviceBoundaryEpsilon);
        p_placement.block_layer_nums[blk_id] = layer;
        p_placement.block_sub_tiles[blk_id] = 0;
    }

    project_placement_(p_placement);
    return p_placement;
}

PartialPlacement NonlinearNesterovPlacer::place() {
    vtr::ScopedStartFinishTimer global_placer_time("AP Nonlinear Nesterov Global Placer");

    std::vector<PrimitiveVectorDim> density_dimensions = density_manager_->get_used_dims_mask().get_non_zero_dims();
    compute_area_inflation_();

    double device_span = std::max<double>(device_grid_width_, device_grid_height_);
    double convergence_displacement = std::max(kMinConvergenceDisplacement,
                                               device_span * kConvergenceDisplacementFraction);

    precond_active_ = kPreconditionEnable;
    precond_alpha_active_ = kPreconditionStrength;
    return run_global_optimization_(density_dimensions, device_span, convergence_displacement);
}

PartialPlacement NonlinearNesterovPlacer::run_global_optimization_(const std::vector<PrimitiveVectorDim>& density_dimensions,
                                                                   double device_span,
                                                                   double convergence_displacement) {
    // Smooth global placement by accelerated gradient descent on an augmented
    // Lagrangian objective: minimize weighted-average wirelength subject to a
    // per-resource electrostatic density penalty. Each epoch runs an inner
    // Nesterov solve at fixed penalty weights, partially legalizes the result to
    // form a proximity anchor, then raises the density multipliers (subgradient
    // ascent) so the following epoch enforces density more strongly.
    PartialPlacement current = initialize_placement_();
    std::vector<double> density_multipliers(density_dimensions.size(), 1.);
    std::vector<double> density_penalties(density_dimensions.size(), 0.);
    // Seed the density multiplier so the density term starts at a small fixed
    // fraction of the initial wirelength, and scale each resource's quadratic
    // penalty to a common energy reference so heterogeneous resource types
    // (LUT, BRAM, DSP, ...) begin balanced rather than dominated by one type.
    ObjectiveValue initial_components = evaluate_objective_(current,
                                                            density_multipliers,
                                                            density_penalties,
                                                            std::nullopt,
                                                            0.,
                                                            std::nullopt);
    double initial_density_weight = 1e-3;
    if (initial_components.density > kEpsilon) {
        initial_density_weight = kInitialDensityToWirelengthRatio * std::max(initial_components.wirelength, 1.0) / initial_components.density;
    }
    initial_density_weight = std::clamp(initial_density_weight, 1e-5, 1e3);
    for (size_t dim_idx = 0; dim_idx < density_dimensions.size(); dim_idx++) {
        density_multipliers[dim_idx] = initial_density_weight;
        density_penalties[dim_idx] = 2e3 / std::max(initial_components.density_energies[dim_idx], kEpsilon);
    }

    // Initialize mobile fillers after the physical-only scaling above, then
    // activate them so every subsequent density evaluation deposits filler mass.
    size_t num_fillers = initialize_fillers_(density_dimensions);
    fillers_active_ = true;

    if (log_verbosity_ >= 1) {
        VTR_LOG("Nonlinear Nesterov mobile fillers: %zu particles across %zu primitive dimensions.\n",
                num_fillers, density_dimensions.size());
    }

    if (log_verbosity_ >= 1) {
        VTR_LOG("Epoch  Pre HPWL  Post HPWL  Pre Oflow  Post Oflow  Pre Max  Post Max  Mean Move  Max Move  Density Wt  Prox Wt\n");
        VTR_LOG("-----  --------  ---------  ---------  ----------  -------  --------  ---------  --------  ----------  -------\n");
    }

    double legalizer_feedback_proximity_weight = 0.;
    // Augmented-Lagrangian outer loop. Penalty weights are held fixed within an
    // epoch and raised between epochs; timing net weights and the preconditioner
    // are refreshed against the current placement at the start of each one.
    for (size_t epoch = 0; epoch < kNesterovEpochs; epoch++) {
        if (ap_timing_tradeoff_ != 0.f && place_delay_model_) {
            update_timing_info_with_partial_placement(pre_cluster_timing_manager_,
                                                      *place_delay_model_,
                                                      current,
                                                      ap_netlist_);
        }
        update_timing_net_weights_();
        compute_preconditioner_(density_dimensions, density_multipliers);

        PartialPlacement legal_anchor = current;
        PartialPlacement y_placement = current;
        PartialPlacement next = current;
        PlacementGradient grad(ap_netlist_);
        double proximity_scale = optimizable_blocks_.size() < kProximitySizeThreshold ? kProximityScale : 1.0;
        double proximity_weight = legalizer_feedback_proximity_weight * proximity_scale;
        // A preconditioned gradient already carries position units (a near-Newton
        // step), so its natural step length is ~1; the raw gradient instead needs
        // a span-scaled step. Backtracking adapts either from this starting point.
        double step_size = precond_active_
                               ? 1.0
                               : std::max(0.1, device_span * kInitialStepSpanFraction);
        double nesterov_t = 1.0;
        ObjectiveValue current_obj = evaluate_objective_(current,
                                                         density_multipliers,
                                                         density_penalties,
                                                         std::cref(legal_anchor),
                                                         proximity_weight,
                                                         std::nullopt);

        // Nesterov accelerated-gradient inner solve. The gradient is taken at the
        // extrapolated look-ahead point y_placement; a backtracking line search
        // halves the step until the objective decreases.
        for (size_t iter = 0; iter < kNesterovIterationsPerEpoch; iter++) {
            ObjectiveValue y_obj = evaluate_objective_(y_placement,
                                                       density_multipliers,
                                                       density_penalties,
                                                       std::cref(legal_anchor),
                                                       proximity_weight,
                                                       std::ref(grad));
            double grad_norm_sq = gradient_norm_squared_(grad);
            if (grad_norm_sq < kEpsilon)
                break;

            double accepted_step = step_size;
            ObjectiveValue next_obj;
            bool accepted = false;
            while (accepted_step >= kMinStepSize) {
                gradient_step_(y_placement, grad, accepted_step, next);
                next_obj = evaluate_objective_(next,
                                               density_multipliers,
                                               density_penalties,
                                               std::cref(legal_anchor),
                                               proximity_weight,
                                               std::nullopt);
                if (next_obj.total <= y_obj.total || accepted_step == kMinStepSize) {
                    accepted = true;
                    break;
                }
                accepted_step *= 0.5;
            }

            if (!accepted)
                break;

            double max_step_displacement = max_block_displacement_(y_placement, next);

            // FISTA momentum: the t-sequence sets the extrapolation weight beta.
            double next_t = 0.5 * (1.0 + std::sqrt(1.0 + 4.0 * nesterov_t * nesterov_t));
            double beta = (nesterov_t - 1.0) / next_t;

            if (next_obj.total > current_obj.total) {
                // Adaptive restart: the step worsened the objective, so drop the
                // momentum and continue from the new point without extrapolation.
                nesterov_t = 1.0;
                copy_placement_(next, y_placement);
            } else {
                // Extrapolate the look-ahead point ahead of the accepted step.
                extrapolate_(current, next, beta, y_placement);
                nesterov_t = next_t;
            }

            copy_placement_(next, current);
            current_obj = next_obj;

            step_size = std::min(device_span, accepted_step * 1.05);

            // Advance fillers down the density force buffered while evaluating
            // y_placement; the decoupled flow fills whitespace faster than the
            // wirelength-limited physical step would allow. The absolute cap keeps
            // the flow convergent on large grids.
            move_fillers_(std::min(kFillerMoveSpanFraction * device_span, kFillerMoveAbsoluteCap));

            if (iter + 1 >= kMinNesterovIterationsPerEpoch
                && max_step_displacement <= convergence_displacement) {
                break;
            }
        }

        ObjectiveValue pre_legalization = evaluate_objective_(current,
                                                              density_multipliers,
                                                              density_penalties,
                                                              std::cref(legal_anchor),
                                                              proximity_weight,
                                                              std::nullopt);
        // Partially legalize the smooth result. This both cleans up overlap and
        // produces the anchor that the next epoch's proximity term pulls toward;
        // the per-epoch displacement it causes also drives the proximity weight.
        PartialPlacement before_legalization = current;
        partial_legalizer_->legalize(current);
        ObjectiveValue post_legalization = evaluate_objective_(current,
                                                               density_multipliers,
                                                               density_penalties,
                                                               std::cref(legal_anchor),
                                                               proximity_weight,
                                                               std::nullopt);

        // Subgradient ascent on the per-resource density multipliers: each
        // resource's weight grows with its remaining (post-legalization) density
        // energy, so resources that are still over-packed are penalized harder
        // next epoch. The step is normalized across resources and grows with the
        // epoch so the density constraint tightens steadily over the schedule.
        double normalized_subgradient_norm_squared = 0.;
        std::vector<double> normalized_subgradients(density_dimensions.size(), 0.);
        for (size_t dim_idx = 0; dim_idx < density_dimensions.size(); dim_idx++) {
            double normalized_energy = post_legalization.density_energies[dim_idx]
                                       / std::max(initial_components.density_energies[dim_idx], kEpsilon);
            normalized_subgradients[dim_idx] = normalized_energy
                                               + 0.5 * density_penalties[dim_idx]
                                                     * initial_components.density_energies[dim_idx]
                                                     * normalized_energy * normalized_energy;
            normalized_subgradient_norm_squared += normalized_subgradients[dim_idx] * normalized_subgradients[dim_idx];
        }
        if (normalized_subgradient_norm_squared > kEpsilon) {
            double multiplier_step = initial_density_weight * std::pow(1.05, epoch);
            double inverse_norm = 1. / std::sqrt(normalized_subgradient_norm_squared);
            for (size_t dim_idx = 0; dim_idx < density_dimensions.size(); dim_idx++)
                density_multipliers[dim_idx] += multiplier_step * normalized_subgradients[dim_idx] * inverse_norm;
        }

        double total_displacement = 0.;
        double max_displacement = 0.;
        for (APBlockId blk_id : optimizable_blocks_) {
            double dx = current.block_x_locs[blk_id] - before_legalization.block_x_locs[blk_id];
            double dy = current.block_y_locs[blk_id] - before_legalization.block_y_locs[blk_id];
            double displacement = std::hypot(dx, dy);
            total_displacement += displacement;
            max_displacement = std::max(max_displacement, displacement);
        }
        double mean_displacement = optimizable_blocks_.empty() ? 0. : total_displacement / optimizable_blocks_.size();
        legalizer_feedback_proximity_weight = std::min(kMaxLegalizerFeedbackProximityWeight,
                                                       std::max(kLegalizerFeedbackRetention * legalizer_feedback_proximity_weight,
                                                                kProximityWeightPerLegalizationTile * mean_displacement));

        VTR_LOG("%5zu  %8.2f  %9.2f  %9.4f  %10.4f  %7.4f  %8.4f  %9.4f  %8.4f  %10.4g  %7.4g\n",
                epoch,
                before_legalization.get_hpwl(ap_netlist_),
                current.get_hpwl(ap_netlist_),
                pre_legalization.total_overflow,
                post_legalization.total_overflow,
                pre_legalization.max_overflow,
                post_legalization.max_overflow,
                mean_displacement,
                max_displacement,
                density_multipliers.empty() ? 0. : density_multipliers.front(),
                proximity_weight);
    }

    VTR_LOG("Nonlinear Nesterov Global Placer Statistics:\n");
    VTR_LOG("\tPlacement HPWL after final partial legalization: %g\n", current.get_hpwl(ap_netlist_));
    VTR_LOG("\tFinal first density multiplier: %g\n", density_multipliers.empty() ? 0. : density_multipliers.front());
    partial_legalizer_->print_statistics();

    return current;
}

NonlinearNesterovPlacer::ObjectiveValue NonlinearNesterovPlacer::evaluate_objective_(const PartialPlacement& p_placement,
                                                                                     const std::vector<double>& density_multipliers,
                                                                                     const std::vector<double>& density_penalties,
                                                                                     std::optional<std::reference_wrapper<const PartialPlacement>> legal_anchor,
                                                                                     double proximity_weight,
                                                                                     std::optional<std::reference_wrapper<PlacementGradient>> grad) const {
    if (grad)
        grad->get().clear();

    ObjectiveValue value;
    value.wirelength = add_wirelength_gradient_(p_placement, grad);
    value.density = add_density_gradient_(p_placement,
                                          density_multipliers,
                                          density_penalties,
                                          value.density_energies,
                                          value.total_overflow,
                                          value.max_overflow,
                                          grad);
    if (legal_anchor)
        value.proximity = add_proximity_gradient_(p_placement,
                                                  legal_anchor->get(),
                                                  proximity_weight,
                                                  grad);
    value.total = value.wirelength + proximity_weight * value.proximity;
    for (size_t dim_idx = 0; dim_idx < value.density_energies.size(); dim_idx++) {
        double energy = value.density_energies[dim_idx];
        value.total += density_multipliers[dim_idx] * (energy + 0.5 * density_penalties[dim_idx] * energy * energy);
    }
    return value;
}

double NonlinearNesterovPlacer::add_wirelength_gradient_(const PartialPlacement& p_placement,
                                                         std::optional<std::reference_wrapper<PlacementGradient>> grad) const {
    // Weighted-average (WA) wirelength: a smooth, differentiable surrogate for
    // per-net half-perimeter wirelength. Each net's span in x and y is the
    // softmax-weighted max minus the softmax-weighted min of its pin coordinates;
    // gamma controls the smoothing (smaller gamma -> closer to true HPWL, sharper
    // gradient). gamma scales with the device so smoothing is grid-relative.
    double gamma = std::max(1.0, std::max<double>(device_grid_width_, device_grid_height_) * kWirelengthGammaFraction);
    double smooth_wirelength = 0.;

    for (APNetId net_id : ap_netlist_.nets()) {
        if (ap_netlist_.net_is_ignored(net_id))
            continue;

        size_t num_pins = ap_netlist_.net_pins(net_id).size();
        if (num_pins < 2)
            continue;

        double net_weight = net_weights_[net_id];
        std::vector<double> x_locs;
        std::vector<double> y_locs;
        x_locs.reserve(num_pins);
        y_locs.reserve(num_pins);

        for (APPinId pin_id : ap_netlist_.net_pins(net_id)) {
            APBlockId blk_id = ap_netlist_.pin_block(pin_id);
            x_locs.push_back(p_placement.block_x_locs[blk_id]);
            y_locs.push_back(p_placement.block_y_locs[blk_id]);
        }

        std::vector<double> positive_x_weights;
        std::vector<double> negative_x_weights;
        std::vector<double> positive_y_weights;
        std::vector<double> negative_y_weights;
        std::vector<double>* positive_x_weights_ptr = grad ? &positive_x_weights : nullptr;
        std::vector<double>* negative_x_weights_ptr = grad ? &negative_x_weights : nullptr;
        std::vector<double>* positive_y_weights_ptr = grad ? &positive_y_weights : nullptr;
        std::vector<double>* negative_y_weights_ptr = grad ? &negative_y_weights : nullptr;
        double positive_x = weighted_average_coordinate(x_locs, gamma, false, positive_x_weights_ptr);
        double negative_x = weighted_average_coordinate(x_locs, gamma, true, negative_x_weights_ptr);
        double positive_y = weighted_average_coordinate(y_locs, gamma, false, positive_y_weights_ptr);
        double negative_y = weighted_average_coordinate(y_locs, gamma, true, negative_y_weights_ptr);

        smooth_wirelength += net_weight * (positive_x - negative_x + positive_y - negative_y);

        if (!grad)
            continue;

        size_t pin_idx = 0;
        for (APPinId pin_id : ap_netlist_.net_pins(net_id)) {
            APBlockId blk_id = ap_netlist_.pin_block(pin_id);
            if (block_is_optimizable_(blk_id)) {
                double positive_x_gradient = positive_x_weights[pin_idx] * (1. + (x_locs[pin_idx] - positive_x) / gamma);
                double negative_x_gradient = negative_x_weights[pin_idx] * (1. - (x_locs[pin_idx] - negative_x) / gamma);
                double positive_y_gradient = positive_y_weights[pin_idx] * (1. + (y_locs[pin_idx] - positive_y) / gamma);
                double negative_y_gradient = negative_y_weights[pin_idx] * (1. - (y_locs[pin_idx] - negative_y) / gamma);
                grad->get().dx[blk_id] += net_weight * (positive_x_gradient - negative_x_gradient);
                grad->get().dy[blk_id] += net_weight * (positive_y_gradient - negative_y_gradient);
            }
            pin_idx++;
        }
    }

    return smooth_wirelength;
}

void NonlinearNesterovPlacer::update_timing_net_weights_() {
    std::fill(net_weights_.begin(), net_weights_.end(), 1.0);

    if (ap_timing_tradeoff_ == 0.f)
        return;

    if (!pre_cluster_timing_manager_.is_valid()) {
        VTR_LOG_WARN("Nonlinear Nesterov analytical placement requested timing tradeoff %g, but pre-cluster timing is unavailable; using unit net weights.\n",
                     ap_timing_tradeoff_);
        return;
    }

    double total_weight = 0.;
    double min_weight = std::numeric_limits<double>::infinity();
    double max_weight = 0.;
    size_t weighted_nets = 0;

    for (APNetId net_id : ap_netlist_.nets()) {
        if (ap_netlist_.net_is_ignored(net_id))
            continue;

        AtomNetId atom_net_id = ap_netlist_.net_atom_net(net_id);
        VTR_ASSERT_SAFE(atom_net_id.is_valid());

        double crit = pre_cluster_timing_manager_.calc_net_setup_criticality(atom_net_id, atom_netlist_);
        double weight = ap_timing_tradeoff_ * crit + (1.0 - ap_timing_tradeoff_);
        net_weights_[net_id] = weight;

        total_weight += weight;
        min_weight = std::min(min_weight, weight);
        max_weight = std::max(max_weight, weight);
        weighted_nets++;
    }

    if (log_verbosity_ >= 1 && weighted_nets > 0) {
        VTR_LOG("Nonlinear Nesterov timing net weights: tradeoff=%g min=%g avg=%g max=%g nets=%zu\n",
                ap_timing_tradeoff_,
                min_weight,
                total_weight / weighted_nets,
                max_weight,
                weighted_nets);
    }
}

double NonlinearNesterovPlacer::add_density_gradient_(const PartialPlacement& p_placement,
                                                      const std::vector<double>& density_multipliers,
                                                      const std::vector<double>& density_penalties,
                                                      std::vector<double>& density_energies,
                                                      double& total_overflow,
                                                      double& max_overflow,
                                                      std::optional<std::reference_wrapper<PlacementGradient>> grad) const {
    total_overflow = 0.;
    max_overflow = 0.;

    const FlatPlacementBins& bins = density_manager_->flat_placement_bins();
    std::vector<PrimitiveVectorDim> dimensions = density_manager_->get_used_dims_mask().get_non_zero_dims();
    if (dimensions.empty())
        return 0.;
    VTR_ASSERT(density_multipliers.size() == dimensions.size());
    VTR_ASSERT(density_penalties.size() == dimensions.size());
    density_energies.assign(dimensions.size(), 0.);

    size_t width = device_grid_width_;
    size_t height = device_grid_height_;
    size_t num_layers = device_grid_num_layers_;
    size_t num_sites = width * height * num_layers;
    auto site_index = [width, height](size_t layer, size_t x, size_t y) {
        return (layer * height + y) * width + x;
    };

    // Arrays with grid information of the density objective via electrostatic formulation
    // All are recreated & zero-filled at every density-objective evaluation
    // Utilization: deposited block mass at each site
    // Target Capacity: available target mass/capacity at each site
    // Potential: solved electrostatic potential
    // field_x, field_y: components of the potential gradient
    std::vector<std::vector<double>> utilization(dimensions.size(), std::vector<double>(num_sites, 0.));
    std::vector<std::vector<double>> target_capacity(dimensions.size(), std::vector<double>(num_sites, 0.));
    std::vector<std::vector<double>> potential(dimensions.size(), std::vector<double>(num_sites, 0.));
    std::vector<std::vector<double>> field_x(dimensions.size(), std::vector<double>(num_sites, 0.));
    std::vector<std::vector<double>> field_y(dimensions.size(), std::vector<double>(num_sites, 0.));

    // Spread each bin's capacity over its footprint, so hard blocks do not
    // create an artificially large electrostatic charge at their root tile.
    for (size_t layer = 0; layer < num_layers; layer++) {
        for (size_t x = 0; x < width; x++) {
            for (size_t y = 0; y < height; y++) {
                FlatPlacementBinId bin_id = density_manager_->get_bin(x, y, layer);
                const vtr::Rect<double>& region = bins.bin_region(bin_id);
                double bin_area = std::max(1.0, region.width() * region.height());
                double target_density = density_manager_->get_bin_target_density(bin_id);
                size_t idx = site_index(layer, x, y);
                for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
                    PrimitiveVectorDim dim = dimensions[dim_idx];
                    target_capacity[dim_idx][idx] = density_manager_->get_bin_capacity(bin_id).get_dim_val(dim)
                                                    * target_density / bin_area;
                }
            }
        }
    }

    // Deposit each primitive-vector mass bilinearly onto the tile grid.
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        PrimitiveVector block_mass = density_manager_->mass_calculator().get_block_mass(blk_id);
        if (block_mass.is_zero())
            continue;

        double x = std::clamp(p_placement.block_x_locs[blk_id], 0., device_grid_width_ - kDeviceBoundaryEpsilon);
        double y = std::clamp(p_placement.block_y_locs[blk_id], 0., device_grid_height_ - kDeviceBoundaryEpsilon);
        size_t layer = static_cast<size_t>(std::clamp(std::round(p_placement.block_layer_nums[blk_id]),
                                                      0.,
                                                      static_cast<double>(device_grid_num_layers_ - 1)));
        size_t x0 = static_cast<size_t>(std::floor(x));
        size_t y0 = static_cast<size_t>(std::floor(y));
        size_t x1 = std::min(x0 + 1, width - 1);
        size_t y1 = std::min(y0 + 1, height - 1);
        double fx = x - x0;
        double fy = y - y0;
        double wx[2] = {1. - fx, fx};
        double wy[2] = {1. - fy, fy};
        size_t xs[2] = {x0, x1};
        size_t ys[2] = {y0, y1};

        if (x0 == x1) {
            wx[0] = 1.;
            wx[1] = 0.;
        }
        if (y0 == y1) {
            wy[0] = 1.;
            wy[1] = 0.;
        }

        // Traverses dimensions, i.e. resource types of the mass abstraction
        double block_scale = block_mass_scale_.empty() ? 1.0 : block_mass_scale_[blk_id];
        for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
            double mass = block_mass.get_dim_val(dimensions[dim_idx]) * block_scale;
            if (mass == 0.)
                continue;
            for (size_t xi = 0; xi < 2; xi++) {
                for (size_t yi = 0; yi < 2; yi++) {
                    double weight = wx[xi] * wy[yi];
                    if (weight != 0.)
                        utilization[dim_idx][site_index(layer, xs[xi], ys[yi])] += mass * weight;
                }
            }
        }
    }

    // Deposit mobile filler mass so the field can reach a compact target-density
    // equilibrium without forcing physical blocks to occupy distant whitespace.
    if (fillers_active_) {
        for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
            double mass = fillers_.unit_mass[dim_idx];
            if (mass <= 0.)
                continue;
            const std::vector<double>& filler_x = fillers_.x[dim_idx];
            const std::vector<double>& filler_y = fillers_.y[dim_idx];
            const std::vector<size_t>& filler_layer = fillers_.layer[dim_idx];
            for (size_t i = 0; i < filler_x.size(); i++) {
                double x = std::clamp(filler_x[i], 0., device_grid_width_ - kDeviceBoundaryEpsilon);
                double y = std::clamp(filler_y[i], 0., device_grid_height_ - kDeviceBoundaryEpsilon);
                size_t layer = std::min(filler_layer[i], num_layers - 1);
                size_t x0 = static_cast<size_t>(std::floor(x));
                size_t y0 = static_cast<size_t>(std::floor(y));
                size_t x1 = std::min(x0 + 1, width - 1);
                size_t y1 = std::min(y0 + 1, height - 1);
                double fx = x - x0;
                double fy = y - y0;
                double wx[2] = {1. - fx, fx};
                double wy[2] = {1. - fy, fy};
                size_t xs[2] = {x0, x1};
                size_t ys[2] = {y0, y1};
                if (x0 == x1) {
                    wx[0] = 1.;
                    wx[1] = 0.;
                }
                if (y0 == y1) {
                    wy[0] = 1.;
                    wy[1] = 0.;
                }
                for (size_t xi = 0; xi < 2; xi++) {
                    for (size_t yi = 0; yi < 2; yi++) {
                        double weight = wx[xi] * wy[yi];
                        if (weight != 0.)
                            utilization[dim_idx][site_index(layer, xs[xi], ys[yi])] += mass * weight;
                    }
                }
            }
        }
    }

    // Electrostatic density model (ePlace/elfPlace), solved independently per
    // resource dimension. Treat the excess block density at each tile as electric
    // charge (charge = utilization/target - 1, so overfilled tiles are positive
    // and empty tiles negative); solving Poisson's equation gives a potential
    // whose negative gradient is a force that pushes blocks from dense to sparse
    // regions. The total potential energy is the density penalty in the objective.
    double density_energy = 0.;
    for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
        std::vector<double> charge(num_sites, 0.);
        std::vector<bool> active(num_sites, false);
        double charge_sum = 0.;
        size_t active_sites = 0;

        for (size_t idx = 0; idx < num_sites; idx++) {
            double target = target_capacity[dim_idx][idx];
            double utilization_at_site = utilization[dim_idx][idx];
            if (target <= kEpsilon) {
                // A block in a tile without capacity for its primitive type is
                // an overfill source, not an empty destination.
                if (utilization_at_site <= kEpsilon)
                    continue;
                charge[idx] = utilization_at_site;
                total_overflow += utilization_at_site;
                max_overflow = std::max(max_overflow, utilization_at_site);
            } else {
                double normalized_utilization = utilization_at_site / std::max(target, 1.0);
                charge[idx] = normalized_utilization - 1.0;

                double normalized_overflow = std::max(0., normalized_utilization - 1.0);
                total_overflow += normalized_overflow;
                max_overflow = std::max(max_overflow, normalized_overflow);
            }
            active[idx] = true;
            charge_sum += charge[idx];
            active_sites++;
        }

        if (active_sites == 0)
            continue;

        // DC removal: subtract the mean so total charge is zero. The Neumann
        // (zero-flux) boundary makes the Poisson problem solvable only for a
        // charge-neutral system, and it keeps blocks spreading evenly rather than
        // all drifting toward one boundary.
        double mean_charge = charge_sum / active_sites;
        for (size_t idx = 0; idx < num_sites; idx++) {
            if (active[idx])
                charge[idx] -= mean_charge;
        }

        // Solve each layer on the full rectangular field domain. Capacity-free
        // sites carry zero charge but remain part of the Neumann PDE domain.
        std::vector<double> layer_charge(width * height);
        std::vector<double> layer_potential;
        for (size_t layer = 0; layer < num_layers; layer++) {
            for (size_t x = 0; x < width; x++) {
                for (size_t y = 0; y < height; y++)
                    layer_charge[y * width + x] = charge[site_index(layer, x, y)];
            }
            solve_neumann_poisson_dct(layer_charge, width, height, layer_potential);
            for (size_t x = 0; x < width; x++) {
                for (size_t y = 0; y < height; y++)
                    potential[dim_idx][site_index(layer, x, y)] = layer_potential[y * width + x];
            }
        }

        for (size_t idx = 0; idx < num_sites; idx++)
            density_energies[dim_idx] += 0.5 * charge[idx] * potential[dim_idx][idx];
        density_energy += density_energies[dim_idx];

        // Computes field, i.e. potential gradient by central differences
        // site_index computes (layer * height + y) * width + x;
        for (size_t layer = 0; layer < num_layers; layer++) {
            for (size_t x = 0; x < width; x++) {
                for (size_t y = 0; y < height; y++) {
                    size_t idx = site_index(layer, x, y);
                    if (!active[idx])
                        continue;
                    size_t left = site_index(layer, x == 0 ? x : x - 1, y);
                    size_t right = site_index(layer, x + 1 == width ? x : x + 1, y);
                    size_t down = site_index(layer, x, y == 0 ? y : y - 1);
                    size_t up = site_index(layer, x, y + 1 == height ? y : y + 1);
                    field_x[dim_idx][idx] = 0.5 * (potential[dim_idx][right] - potential[dim_idx][left]);
                    field_y[dim_idx][idx] = 0.5 * (potential[dim_idx][up] - potential[dim_idx][down]);
                }
            }
        }

        // Buffer the density force on each filler so the caller can advance them.
        // Fillers feel the same field as physical blocks; the move step later
        // normalizes per dimension, so the scalar coefficient only sets direction.
        if (grad && fillers_active_ && fillers_.unit_mass[dim_idx] > 0.) {
            double mass = fillers_.unit_mass[dim_idx];
            double coefficient = density_multipliers[dim_idx] * (1. + density_penalties[dim_idx] * density_energies[dim_idx]);
            const std::vector<double>& filler_x = fillers_.x[dim_idx];
            const std::vector<double>& filler_y = fillers_.y[dim_idx];
            const std::vector<size_t>& filler_layer = fillers_.layer[dim_idx];
            for (size_t i = 0; i < filler_x.size(); i++) {
                double x = std::clamp(filler_x[i], 0., device_grid_width_ - kDeviceBoundaryEpsilon);
                double y = std::clamp(filler_y[i], 0., device_grid_height_ - kDeviceBoundaryEpsilon);
                size_t layer = std::min(filler_layer[i], num_layers - 1);
                size_t x0 = static_cast<size_t>(std::floor(x));
                size_t y0 = static_cast<size_t>(std::floor(y));
                size_t x1 = std::min(x0 + 1, width - 1);
                size_t y1 = std::min(y0 + 1, height - 1);
                double fx = x - x0;
                double fy = y - y0;
                double wx[2] = {1. - fx, fx};
                double wy[2] = {1. - fy, fy};
                size_t xs[2] = {x0, x1};
                size_t ys[2] = {y0, y1};
                if (x0 == x1) {
                    wx[0] = 1.;
                    wx[1] = 0.;
                }
                if (y0 == y1) {
                    wy[0] = 1.;
                    wy[1] = 0.;
                }
                double local_field_x = 0.;
                double local_field_y = 0.;
                double local_target = 0.;
                for (size_t xi = 0; xi < 2; xi++) {
                    for (size_t yi = 0; yi < 2; yi++) {
                        double weight = wx[xi] * wy[yi];
                        size_t idx = site_index(layer, xs[xi], ys[yi]);
                        local_field_x += weight * field_x[dim_idx][idx];
                        local_field_y += weight * field_y[dim_idx][idx];
                        local_target += weight * target_capacity[dim_idx][idx];
                    }
                }
                double normalized_mass = mass / std::max(local_target, 1.0);
                filler_grad_x_[dim_idx][i] = coefficient * normalized_mass * local_field_x;
                filler_grad_y_[dim_idx][i] = coefficient * normalized_mass * local_field_y;
            }
        }
    }

    if (!grad)
        return density_energy;

    // Turn the grid field into a block gradient
    for (APBlockId blk_id : optimizable_blocks_) {
        PrimitiveVector block_mass = density_manager_->mass_calculator().get_block_mass(blk_id);
        if (block_mass.is_zero())
            continue;

        double x = std::clamp(p_placement.block_x_locs[blk_id], 0., device_grid_width_ - kDeviceBoundaryEpsilon);
        double y = std::clamp(p_placement.block_y_locs[blk_id], 0., device_grid_height_ - kDeviceBoundaryEpsilon);
        size_t layer = static_cast<size_t>(std::clamp(std::round(p_placement.block_layer_nums[blk_id]),
                                                      0.,
                                                      static_cast<double>(device_grid_num_layers_ - 1)));
        size_t x0 = static_cast<size_t>(std::floor(x));
        size_t y0 = static_cast<size_t>(std::floor(y));
        size_t x1 = std::min(x0 + 1, width - 1);
        size_t y1 = std::min(y0 + 1, height - 1);
        double fx = x - x0;
        double fy = y - y0;
        double wx[2] = {1. - fx, fx};
        double wy[2] = {1. - fy, fy};
        size_t xs[2] = {x0, x1};
        size_t ys[2] = {y0, y1};

        if (x0 == x1) {
            wx[0] = 1.;
            wx[1] = 0.;
        }
        if (y0 == y1) {
            wy[0] = 1.;
            wy[1] = 0.;
        }

        double block_scale = block_mass_scale_.empty() ? 1.0 : block_mass_scale_[blk_id];
        for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
            double mass = block_mass.get_dim_val(dimensions[dim_idx]) * block_scale;
            if (mass == 0.)
                continue;

            double local_field_x = 0.;
            double local_field_y = 0.;
            double local_target = 0.;
            for (size_t xi = 0; xi < 2; xi++) {
                for (size_t yi = 0; yi < 2; yi++) {
                    double weight = wx[xi] * wy[yi];
                    size_t idx = site_index(layer, xs[xi], ys[yi]);
                    local_field_x += weight * field_x[dim_idx][idx];
                    local_field_y += weight * field_y[dim_idx][idx];
                    local_target += weight * target_capacity[dim_idx][idx];
                }
            }

            double normalized_mass = mass / std::max(local_target, 1.0);
            double coefficient = density_multipliers[dim_idx] * (1. + density_penalties[dim_idx] * density_energies[dim_idx]);
            grad->get().dx[blk_id] += coefficient * normalized_mass * local_field_x;
            grad->get().dy[blk_id] += coefficient * normalized_mass * local_field_y;
        }
    }

    return density_energy;
}

void NonlinearNesterovPlacer::compute_area_inflation_() {
    block_mass_scale_.resize(ap_netlist_.blocks().size(), 1.0);

    double total_pins = 0.;
    size_t num_blocks = 0;
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        total_pins += ap_netlist_.block_pins(blk_id).size();
        num_blocks++;
    }
    if (num_blocks == 0 || total_pins <= 0.)
        return;

    double mean_pins = total_pins / num_blocks;
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        double ratio = ap_netlist_.block_pins(blk_id).size() / mean_pins;
        double scale = 1.0 + kAreaInflationAlpha * std::max(0., ratio - 1.0);
        block_mass_scale_[blk_id] = std::min(scale, kMaxAreaInflation);
    }
}

void NonlinearNesterovPlacer::compute_preconditioner_(const std::vector<PrimitiveVectorDim>& dimensions,
                                                      const std::vector<double>& density_multipliers) {
    block_precond_.resize(ap_netlist_.blocks().size(), 1.0);
    std::fill(block_precond_.begin(), block_precond_.end(), 0.0);

    // Wirelength Hessian diagonal: the WA gradient's self-coefficient for a block
    // is proportional to the sum of the weights of the nets incident to it.
    for (APNetId net_id : ap_netlist_.nets()) {
        if (ap_netlist_.net_is_ignored(net_id))
            continue;
        if (ap_netlist_.net_pins(net_id).size() < 2)
            continue;
        double net_weight = net_weights_[net_id];
        for (APPinId pin_id : ap_netlist_.net_pins(net_id)) {
            APBlockId blk_id = ap_netlist_.pin_block(pin_id);
            block_precond_[blk_id] += net_weight;
        }
    }

    // Density Hessian diagonal: the per-dimension penalty multiplier weights the
    // block mass it deposits into that resource's field. Heavier blocks under a
    // stronger density push have larger curvature and so take proportionally
    // smaller steps.
    const auto& mass_calculator = density_manager_->mass_calculator();
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        const PrimitiveVector& block_mass = mass_calculator.get_block_mass(blk_id);
        double block_scale = block_mass_scale_.empty() ? 1.0 : block_mass_scale_[blk_id];
        double density_curvature = 0.;
        for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
            double mass = block_mass.get_dim_val(dimensions[dim_idx]) * block_scale;
            density_curvature += density_multipliers[dim_idx] * mass;
        }
        block_precond_[blk_id] += density_curvature;
        if (block_precond_[blk_id] < kPreconditionFloor)
            block_precond_[blk_id] = kPreconditionFloor;
    }

    // Soften the preconditioner toward uniform with a strength exponent < 1 to
    // reduce over-correction (full Jacobi can give low-curvature blocks too large
    // a relative step).
    if (precond_alpha_active_ != 1.0) {
        for (APBlockId blk_id : ap_netlist_.blocks())
            block_precond_[blk_id] = std::pow(block_precond_[blk_id], precond_alpha_active_);
    }

    // Optionally bound the conditioning span so a few very stiff blocks cannot
    // freeze while the rest race: clamp each block to [max/ratio, max].
    if (kPreconditionMaxRatio > 1.0 && !block_precond_.empty()) {
        double max_precond = 0.;
        for (APBlockId blk_id : ap_netlist_.blocks())
            max_precond = std::max(max_precond, block_precond_[blk_id]);
        double min_allowed = max_precond / kPreconditionMaxRatio;
        for (APBlockId blk_id : ap_netlist_.blocks())
            block_precond_[blk_id] = std::max(block_precond_[blk_id], min_allowed);
    }
}

size_t NonlinearNesterovPlacer::initialize_fillers_(const std::vector<PrimitiveVectorDim>& dimensions) {
    fillers_.x.assign(dimensions.size(), {});
    fillers_.y.assign(dimensions.size(), {});
    fillers_.layer.assign(dimensions.size(), {});
    fillers_.unit_mass.assign(dimensions.size(), 0.);
    filler_grad_x_.assign(dimensions.size(), {});
    filler_grad_y_.assign(dimensions.size(), {});

    size_t width = device_grid_width_;
    size_t height = device_grid_height_;
    size_t num_layers = std::max<size_t>(1, device_grid_num_layers_);

    // Total target capacity per dimension, target-density-scaled and summed over
    // every site, matching the deposition normalization in add_density_gradient_.
    const FlatPlacementBins& bins = density_manager_->flat_placement_bins();
    std::vector<double> total_target(dimensions.size(), 0.);
    for (size_t layer = 0; layer < device_grid_num_layers_; layer++) {
        for (size_t x = 0; x < width; x++) {
            for (size_t y = 0; y < height; y++) {
                FlatPlacementBinId bin_id = density_manager_->get_bin(x, y, layer);
                const vtr::Rect<double>& region = bins.bin_region(bin_id);
                double bin_area = std::max(1.0, region.width() * region.height());
                double target_density = density_manager_->get_bin_target_density(bin_id);
                for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
                    total_target[dim_idx] += density_manager_->get_bin_capacity(bin_id).get_dim_val(dimensions[dim_idx])
                                             * target_density / bin_area;
                }
            }
        }
    }

    // Total deposited physical mass and the mean nonzero block mass per dimension.
    std::vector<double> total_phys(dimensions.size(), 0.);
    std::vector<size_t> block_count(dimensions.size(), 0);
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        PrimitiveVector block_mass = density_manager_->mass_calculator().get_block_mass(blk_id);
        double scale = block_mass_scale_.empty() ? 1.0 : block_mass_scale_[blk_id];
        for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
            double mass = block_mass.get_dim_val(dimensions[dim_idx]) * scale;
            if (mass <= 0.)
                continue;
            total_phys[dim_idx] += mass;
            block_count[dim_idx]++;
        }
    }

    size_t total_fillers = 0;
    for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
        double filler_mass = kFillerMassFraction * (total_target[dim_idx] - total_phys[dim_idx]);
        if (filler_mass <= kEpsilon || block_count[dim_idx] == 0)
            continue;
        double mean_block_mass = total_phys[dim_idx] / block_count[dim_idx];
        size_t num_fillers = static_cast<size_t>(std::floor(filler_mass / std::max(mean_block_mass, kEpsilon)));
        num_fillers = std::min(num_fillers, kMaxFillersPerDim);
        if (num_fillers == 0)
            continue;

        // Match the deposited filler mass to the available whitespace exactly.
        fillers_.unit_mass[dim_idx] = filler_mass / num_fillers;

        // Scatter fillers on a uniform grid spanning all layers of the device.
        size_t per_layer = std::max<size_t>(1, (num_fillers + num_layers - 1) / num_layers);
        size_t cols = std::max<size_t>(1, static_cast<size_t>(std::ceil(std::sqrt(static_cast<double>(per_layer)
                                                                                  * static_cast<double>(width)
                                                                                  / static_cast<double>(std::max<size_t>(1, height))))));
        size_t rows = std::max<size_t>(1, (per_layer + cols - 1) / cols);
        fillers_.x[dim_idx].reserve(num_fillers);
        fillers_.y[dim_idx].reserve(num_fillers);
        fillers_.layer[dim_idx].reserve(num_fillers);
        for (size_t i = 0; i < num_fillers; i++) {
            size_t layer = (i / per_layer) % num_layers;
            size_t idx_in_layer = i % per_layer;
            size_t col = idx_in_layer % cols;
            size_t row = idx_in_layer / cols;
            double fx = (static_cast<double>(col) + 0.5) / static_cast<double>(cols) * static_cast<double>(width);
            double fy = (static_cast<double>(row) + 0.5) / static_cast<double>(rows) * static_cast<double>(height);
            fillers_.x[dim_idx].push_back(std::min<double>(fx, width - kDeviceBoundaryEpsilon));
            fillers_.y[dim_idx].push_back(std::min<double>(fy, height - kDeviceBoundaryEpsilon));
            fillers_.layer[dim_idx].push_back(layer);
        }
        filler_grad_x_[dim_idx].assign(num_fillers, 0.);
        filler_grad_y_[dim_idx].assign(num_fillers, 0.);
        total_fillers += num_fillers;
    }
    return total_fillers;
}

void NonlinearNesterovPlacer::move_fillers_(double move_cap) {
    double max_x = std::max(0.0, static_cast<double>(device_grid_width_) - kDeviceBoundaryEpsilon);
    double max_y = std::max(0.0, static_cast<double>(device_grid_height_) - kDeviceBoundaryEpsilon);
    double component_cap = 4.0 * move_cap;

    for (size_t dim_idx = 0; dim_idx < fillers_.x.size(); dim_idx++) {
        size_t count = fillers_.x[dim_idx].size();
        if (count == 0)
            continue;

        // RMS-normalize so a typically forced filler flows ~move_cap tiles per
        // iteration. The shared physical step is throttled by the wirelength
        // gradient and leaves fillers nearly stationary, so they flow on a
        // decoupled schedule that depends only on the density field.
        double sum_sq = 0.;
        for (size_t i = 0; i < count; i++)
            sum_sq += filler_grad_x_[dim_idx][i] * filler_grad_x_[dim_idx][i]
                      + filler_grad_y_[dim_idx][i] * filler_grad_y_[dim_idx][i];
        double rms = std::sqrt(sum_sq / count);
        if (rms <= kEpsilon)
            continue;

        double lr = move_cap / rms;
        for (size_t i = 0; i < count; i++) {
            double step_x = std::clamp(-lr * filler_grad_x_[dim_idx][i], -component_cap, component_cap);
            double step_y = std::clamp(-lr * filler_grad_y_[dim_idx][i], -component_cap, component_cap);
            fillers_.x[dim_idx][i] = std::clamp(fillers_.x[dim_idx][i] + step_x, 0.0, max_x);
            fillers_.y[dim_idx][i] = std::clamp(fillers_.y[dim_idx][i] + step_y, 0.0, max_y);
        }
    }
}

double NonlinearNesterovPlacer::add_proximity_gradient_(const PartialPlacement& p_placement,
                                                        const PartialPlacement& legal_anchor,
                                                        double proximity_weight,
                                                        std::optional<std::reference_wrapper<PlacementGradient>> grad) const {
    if (proximity_weight == 0.)
        return 0.;

    double proximity_penalty = 0.;
    for (APBlockId blk_id : optimizable_blocks_) {
        double dx = p_placement.block_x_locs[blk_id] - legal_anchor.block_x_locs[blk_id];
        double dy = p_placement.block_y_locs[blk_id] - legal_anchor.block_y_locs[blk_id];
        proximity_penalty += 0.5 * (dx * dx + dy * dy);
        if (grad) {
            grad->get().dx[blk_id] += proximity_weight * dx;
            grad->get().dy[blk_id] += proximity_weight * dy;
        }
    }
    return proximity_penalty;
}

void NonlinearNesterovPlacer::project_placement_(PartialPlacement& p_placement) const {
    double max_x = std::max(0.0, static_cast<double>(device_grid_width_) - kDeviceBoundaryEpsilon);
    double max_y = std::max(0.0, static_cast<double>(device_grid_height_) - kDeviceBoundaryEpsilon);
    double max_layer = std::max(0.0, static_cast<double>(device_grid_num_layers_ - 1));

    for (APBlockId blk_id : ap_netlist_.blocks()) {
        p_placement.block_x_locs[blk_id] = std::clamp(p_placement.block_x_locs[blk_id], 0.0, max_x);
        p_placement.block_y_locs[blk_id] = std::clamp(p_placement.block_y_locs[blk_id], 0.0, max_y);
        p_placement.block_layer_nums[blk_id] = std::clamp(p_placement.block_layer_nums[blk_id], 0.0, max_layer);
        p_placement.block_sub_tiles[blk_id] = std::max(0, p_placement.block_sub_tiles[blk_id]);

        if (ap_netlist_.block_mobility(blk_id) == APBlockMobility::FIXED) {
            const APFixedBlockLoc& loc = ap_netlist_.block_loc(blk_id);
            if (loc.x != APFixedBlockLoc::UNFIXED_DIM)
                p_placement.block_x_locs[blk_id] = loc.x;
            if (loc.y != APFixedBlockLoc::UNFIXED_DIM)
                p_placement.block_y_locs[blk_id] = loc.y;
            if (loc.layer_num != APFixedBlockLoc::UNFIXED_DIM)
                p_placement.block_layer_nums[blk_id] = loc.layer_num;
            if (loc.sub_tile != APFixedBlockLoc::UNFIXED_DIM)
                p_placement.block_sub_tiles[blk_id] = loc.sub_tile;
        }
    }
}

void NonlinearNesterovPlacer::copy_placement_(const PartialPlacement& src, PartialPlacement& dst) const {
    dst.block_x_locs = src.block_x_locs;
    dst.block_y_locs = src.block_y_locs;
    dst.block_layer_nums = src.block_layer_nums;
    dst.block_sub_tiles = src.block_sub_tiles;
}

void NonlinearNesterovPlacer::gradient_step_(const PartialPlacement& y_placement,
                                             const PlacementGradient& grad,
                                             double step_size,
                                             PartialPlacement& next_placement) const {
    copy_placement_(y_placement, next_placement);
    if (precond_active_ && !block_precond_.empty()) {
        // Preconditioned (near-Newton) step: divide each block's gradient by its
        // objective-curvature estimate so step length is size-independent.
        for (APBlockId blk_id : optimizable_blocks_) {
            double inv_precond = 1.0 / block_precond_[blk_id];
            next_placement.block_x_locs[blk_id] = y_placement.block_x_locs[blk_id] - step_size * grad.dx[blk_id] * inv_precond;
            next_placement.block_y_locs[blk_id] = y_placement.block_y_locs[blk_id] - step_size * grad.dy[blk_id] * inv_precond;
        }
    } else {
        for (APBlockId blk_id : optimizable_blocks_) {
            next_placement.block_x_locs[blk_id] = y_placement.block_x_locs[blk_id] - step_size * grad.dx[blk_id];
            next_placement.block_y_locs[blk_id] = y_placement.block_y_locs[blk_id] - step_size * grad.dy[blk_id];
        }
    }
    project_placement_(next_placement);
}

void NonlinearNesterovPlacer::extrapolate_(const PartialPlacement& current,
                                           const PartialPlacement& next,
                                           double beta,
                                           PartialPlacement& y_placement) const {
    copy_placement_(next, y_placement);
    for (APBlockId blk_id : optimizable_blocks_) {
        y_placement.block_x_locs[blk_id] = next.block_x_locs[blk_id] + beta * (next.block_x_locs[blk_id] - current.block_x_locs[blk_id]);
        y_placement.block_y_locs[blk_id] = next.block_y_locs[blk_id] + beta * (next.block_y_locs[blk_id] - current.block_y_locs[blk_id]);
    }
    project_placement_(y_placement);
}

double NonlinearNesterovPlacer::gradient_norm_squared_(const PlacementGradient& grad) const {
    double norm_squared = 0.;
    for (APBlockId blk_id : optimizable_blocks_) {
        norm_squared += grad.dx[blk_id] * grad.dx[blk_id];
        norm_squared += grad.dy[blk_id] * grad.dy[blk_id];
    }
    return norm_squared;
}

double NonlinearNesterovPlacer::max_block_displacement_(const PartialPlacement& from,
                                                        const PartialPlacement& to) const {
    double max_displacement = 0.;
    for (APBlockId blk_id : optimizable_blocks_) {
        double dx = to.block_x_locs[blk_id] - from.block_x_locs[blk_id];
        double dy = to.block_y_locs[blk_id] - from.block_y_locs[blk_id];
        max_displacement = std::max(max_displacement, std::hypot(dx, dy));
    }
    return max_displacement;
}
