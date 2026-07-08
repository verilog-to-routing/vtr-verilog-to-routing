/**
 * @file
 * @author  William Zhang
 * @date    June 2026
 * @brief   Implementation of a nonlinear Nesterov analytical global placer.
 */

#include "nonlinear_nesterov_placer.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <complex>
#include <functional>
#include <limits>
#include <optional>
#include <random>
#include <vector>
#include <unsupported/Eigen/FFT>
#include "PreClusterTimingManager.h"
#include "analytical_solver.h"
#include "ap_flow_enums.h"
#include "ap_netlist.h"
#include "atom_netlist.h"
#include "device_grid.h"
#include "flat_placement_bins.h"
#include "globals.h"
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
constexpr size_t kMaxNesterovIterations = 80;

/**
 * @brief Number of optimization/legalization epochs in the nonlinear Nesterov placer.
 */
constexpr size_t kNesterovEpochs = 2;

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
 * @brief Minimum target capacity used when normalizing electrostatic charge.
 *
 * FPGA resource capacities are often fractional after target-density and
 * footprint spreading. The density field should normalize by those fractional
 * capacities, not by one full block. This floor only prevents numerical spikes at
 * interpolation points that barely touch a legal site for a sparse resource.
 */
constexpr double kDensityTargetFloorFraction = 0.01;

/**
 * @brief Scale on the legalizer-feedback proximity weight for small designs.
 *
 * The legalizer-feedback proximity anchor helps large designs (it closes a large
 * legalization gap) but suppresses timing- and wirelength-driven motion on small
 * designs. Scaling the anchor to 0.25 below @ref kProximitySizeThreshold movable
 * blocks improved both small suites (WL -5.3%, CPD -0.3%) while any reduction
 * regressed large-device wirelength; larger designs keep the full anchor.
 */
constexpr double kProximityScale = 0.25;

/**
 * @brief Movable-block count at or above which the full proximity anchor is kept
 *        and the large-design preconditioner is enabled.
 */
constexpr size_t kProximitySizeThreshold = 30000;

/**
 * @brief Preconditioner strength exponent used for large designs.
 *
 * The elfPlace-style diagonal (Jacobi) preconditioner divides each block's
 * gradient by an estimate of its objective curvature -- the sum of incident net
 * weights (wirelength Hessian diagonal) plus the density penalty times block mass
 * (density Hessian diagonal) -- giving every block a near-Newton step regardless
 * of size. It is the validated remedy for the large-device (Titan) over-spread,
 * so it is enabled for designs at or above @ref kProximitySizeThreshold movable
 * blocks; small/homogeneous designs are too uniform to benefit. alpha=0.5 is the
 * timing-safe operating point: on Titan-neuron it cut global HPWL ~12% and routed
 * CPD ~7% at neutral routed WL, where full Jacobi (alpha=1) over-corrected timing.
 */
constexpr double kPreconditionLargeAlpha = 0.5;

/**
 * @brief Floor on the per-block preconditioner to avoid dividing by ~0 curvature.
 */
constexpr double kPreconditionFloor = 1.0;

/**
 * @brief Use residual-capacity electrostatic charge instead of relative charge.
 *
 * Residual charge, `(utilization - target) / average_target`, follows ePlace:
 * object area and target area are balanced in the same units with only a per
 * resource normalization, instead of the relative `utilization / target - 1`
 * charge that over-amplifies fractional-capacity sites on heterogeneous devices.
 */
constexpr bool kUseResidualDensityCharge = true;

/**
 * @brief Fraction of per-resource whitespace represented by dynamic fillers.
 *
 * elfPlace uses movable filler instances to let the density system balance real
 * cells against whitespace. Full whitespace was too aggressive in the VTR flow
 * because APPack/annealing already perform downstream spreading, so this default
 * keeps the filler mechanism active while limiting CPD-damaging over-spread.
 */
constexpr double kDynamicFillerWhitespaceFraction = 0.35;

/**
 * @brief Target dynamic filler mass in units of average per-site target capacity.
 */
constexpr double kDynamicFillerUnitFraction = 1.0;

/**
 * @brief Cap dynamic filler particles per resource dimension.
 */
constexpr size_t kMaxDynamicFillersPerDim = 60000;

/**
 * @brief Device-edge band used to identify resources confined to the boundary.
 *
 * Several architectures leave the true perimeter empty and place I/O-capable
 * tiles one tile in from the edge, so use a two-tile band rather than only x/y
 * equals 0 or max.
 */
constexpr size_t kBoundaryConfinedBandTiles = 2;

/**
 * @brief Fraction of a resource dimension's capacity that must lie in the edge
 *        band before it is treated as boundary-confined.
 */
constexpr double kBoundaryConfinedCapacityFraction = 0.95;

/**
 * @brief Extra wirelength weight for I/O-related AP nets.
 *
 * Values above 1.0 were tested as a nesterov-local LU_Network repair, but the
 * broad long-net variant regressed guard circuits and the boundary-only variant
 * did not improve routed CPD. Keep this neutral by default; the flagged nets are
 * still reported by the telemetry below.
 */
constexpr double kBoundaryNetCohesionWeight = 1.0;

/**
 * @brief Minimum warm-start HPWL, as a fraction of device span, for applying
 *        boundary-net cohesion.
 */
constexpr double kBoundaryNetCohesionMinSeedHpwlFraction = 0.25;

/**
 * @brief Extra smooth-WL weight for direct I/O-chain AP nets.
 *
 * LU_Network's failure mode is not generic boundary spreading; it is specific
 * pad/obuf/OCT/termination chains being split before APPack can form compact
 * I/O clusters. Prior broad boundary-net weighting regressed guard circuits, so
 * this stronger weight is applied only to two-pin nets whose endpoints are both
 * I/O-chain primitives on boundary-confined resources.
 */
constexpr double kIoChainNetCohesionWeight = 2.0;

/**
 * @brief Long-chain pack-pattern cohesion weight for direct I/O-chain designs.
 *
 * Gated to designs with long direct I/O-chain nets, where LU_Network needs extra
 * AP-to-APPack chain coherence. Designs without that pathology disable the term
 * after boundary/I/O-chain classification.
 */
constexpr double kPackPatternCohesionWeight = 0.02;

/**
 * @brief Minimum B2B solve+legalize cycles used to build the warm-start seed.
 *
 * The nonlinear Nesterov placer seeds itself from a wirelength-aware B2B/QP solve
 * (a short SimPL run) rather than a block-ID grid spread. The warm start runs a
 * *convergence-based* number of cycles (see @ref kWarmStartMaxIters /
 * @ref kWarmStartTol): it iterates until the seed HPWL stops improving, so large /
 * under-converged designs (Titan/Titanium) get enough cycles to produce a tight,
 * clusterable placement -- the post-APPack BB inflation that drove their wirelength
 * gap -- while small designs that converge quickly stop early. This is the floor.
 */
constexpr size_t kWarmStartIters = 4;

/**
 * @brief Maximum B2B warm-start cycles (cap on the convergence loop).
 */
constexpr size_t kWarmStartMaxIters = 24;

/**
 * @brief Relative HPWL-improvement threshold below which the warm start stops.
 *
 * Once a B2B cycle improves the seed HPWL by less than this fraction, further
 * cycles are not worth their runtime, so the warm start ends (at or above the
 * @ref kWarmStartIters floor). Larger designs keep improving longer and so run
 * more cycles automatically.
 */
constexpr double kWarmStartTol = 0.01;

/**
 * @brief Seed-overflow gate below which the warm start is deepened.
 *
 * If the warm-start seed's physical overflow ratio is below this, the design is
 * electrostatic-inert (the field has no overfill to spread), so B2B compaction
 * must carry packability; the warm start is extended to @ref kSparseWarmStartIters
 * cycles. Above the gate the field does real work and the short warm start stands.
 */
constexpr double kSparseGateOverflow = 0.0007;

/**
 * @brief Deep warm-start cycle count used when the sparse-overflow gate trips.
 */
constexpr size_t kSparseWarmStartIters = 24;

/**
 * @brief Epoch cap for the electrostatic phase on sparse seeds.
 *
 * On sparse-seed designs (overflow below @ref kSparseGateOverflow) the density
 * field has nothing to spread, and the full epoch schedule was measured to be
 * pure waste on the 2026-07-03 titanium head-to-head: all seven sparse-seed
 * circuits ran 2 epochs with 40k-318k fillers (up to 5098 s of GP, 11.5x lp-b2b)
 * and then selected the warm-start seed anyway. One cheap filler-free
 * wirelength-refinement epoch is kept because it can still beat the seed
 * (fft1d: epoch placement 4.2% lower HPWL than seed).
 */
constexpr size_t kSparseSeedMaxEpochs = 1;

/**
 * @brief Inner-iteration cap for the sparse-seed probe epoch.
 *
 * The probe refines wirelength near an already density-feasible seed, so it does
 * not need the full inner budget; each iteration still pays the per-resource
 * Poisson solves, which dominate sparse-seed epoch cost once fillers are gone.
 */
constexpr size_t kSparseSeedProbeIterations = 12;

/**
 * @brief Minimum AP block count for high-pin designs that need one more B2B seed
 *        cycle before electrostatic refinement.
 */
constexpr size_t kHighPinWarmStartBlockThreshold = 9000;

/**
 * @brief Pin-per-block threshold for high-pin seed compaction.
 */
constexpr double kHighPinWarmStartPinsPerBlock = 8.0;

/**
 * @brief AP block count at which convergence-based warm start is forced to keep
 *        at least the high-pin floor even if HPWL plateaus early.
 */
constexpr size_t kHugeWarmStartBlockThreshold = 200000;

/**
 * @brief Adaptive warm-start floor used by the high-pin and huge-design gates.
 */
constexpr size_t kAdaptiveWarmStartIters = 6;

/**
 * @brief Medium-large designs receive a stronger timing weight by default.
 *
 * This repaired CPD misses on cholesky/nyuzi and removed the sparc wirelength loss,
 * while the high-pin Koios cases regressed under the same timing pressure. The
 * block-count window captures that middle regime and leaves tiny, high-pin, and
 * huge sparse tails on the normal tradeoff.
 */
constexpr size_t kAdaptiveTimingMinBlocks = 50000;
constexpr size_t kAdaptiveTimingMaxBlocks = 150000;
constexpr double kAdaptiveTimingTradeoff = 0.75;

/**
 * @brief Coarse (epoch 0) and sharp (final epoch) gamma fractions for continuation.
 *
 * The placer anneals the weighted-average gamma from coarse (smooth, easy global
 * gradient) to sharp (close to true HPWL) across epochs, instead of holding it
 * fixed, so early epochs spread on an easy landscape and later epochs recover real
 * wirelength.
 */
constexpr double kGammaStartFraction = 0.04;
constexpr double kGammaEndFraction = 0.008;

/**
 * @brief Initial target ratio of density pressure to wirelength pressure.
 *
 * A small fixed linear density weight keeps the first Nesterov pass close to the
 * warm-start seed while still letting the electrostatic field relieve overlap.
 */
constexpr double kInitialDensityToWirelengthRatio = 0.05;

/**
 * @brief Final density-weight multiplier for the simple continuation schedule.
 */
constexpr double kFinalDensityWeightMultiplier = 4.0;

/**
 * @brief Target physical-overflow ratio for the WL-favoring penalty stop.
 *
 * Once the *physical* placement is already spread enough -- the mass exceeding
 * tile capacity falls below this fraction of total capacity -- further density
 * continuation only adds wirelength, so the outer loop stops.
 */
constexpr double kTargetOverflow = 0.1;

/**
 * @brief Minimum epochs before the overflow stop may trigger.
 *
 * The warm-start seed is already roughly legal, so allow at least a couple of
 * refinement epochs before the physical-overflow stop can end the loop.
 */
constexpr size_t kMinEpochsBeforeOverflowStop = 2;

/**
 * @brief Pi constant for portable math; M_PI is not guaranteed by <cmath>.
 */
constexpr double kPi = 3.141592653589793238462643383279502884;

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
        double angle = kPi * frequency / (2. * size);
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
        double angle = kPi * frequency / (2. * size);
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
        double y_eigenvalue = 2. * (1. - std::cos(kPi * y_frequency / height));
        for (size_t x_frequency = 0; x_frequency < width; x_frequency++) {
            size_t idx = y_frequency * width + x_frequency;
            double x_eigenvalue = 2. * (1. - std::cos(kPi * x_frequency / width));
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

using OptionalWeightVectorRef = std::optional<std::reference_wrapper<std::vector<double>>>;

/**
 * @brief Evaluate the weighted-average approximation of a coordinate extremum.
 *
 * With @p negate false, this approximates the maximum coordinate. With @p
 * negate true, this approximates the minimum coordinate. The optional returned
 * weights are reused by the caller to form the analytical derivative.
 *
 * @param values Coordinate values for one net dimension. Must be non-empty.
 * @param gamma Positive smoothing factor: smaller values more closely
 *              approximate the extremum, while larger values smooth it more.
 * @param negate When true, negate each value before forming the weighted
 *               average to approximate the negated minimum instead of the maximum.
 * @param weights Optional storage for normalized exponential weights.
 * @return The weighted-average coordinate.
 */
double weighted_average_coordinate(const std::vector<double>& values,
                                   double gamma,
                                   bool negate,
                                   OptionalWeightVectorRef weights) {
    VTR_ASSERT(!values.empty());
    VTR_ASSERT(gamma > 0.);

    double max_scaled = negate ? -values.front() / gamma : values.front() / gamma;
    for (double value : values) {
        double scaled_value = negate ? -value / gamma : value / gamma;
        max_scaled = std::max(max_scaled, scaled_value);
    }

    double exp_sum = 0.;
    double weighted_sum = 0.;
    if (weights)
        weights->get().assign(values.size(), 0.);

    for (size_t idx = 0; idx < values.size(); idx++) {
        double value = values[idx];
        double scaled_value = negate ? -value / gamma : value / gamma;
        double exponential = std::exp(scaled_value - max_scaled);
        exp_sum += exponential;
        weighted_sum += value * exponential;
        if (weights)
            weights->get()[idx] = exponential;
    }

    VTR_ASSERT_SAFE(exp_sum > 0.);
    if (weights) {
        for (double& weight : weights->get())
            weight /= exp_sum;
    }
    return weighted_sum / exp_sum;
}

std::string lower_copy(const std::string& value) {
    std::string lowered = value;
    for (char& c : lowered)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return lowered;
}

bool model_name_is_io_chain(const std::string& model_name) {
    if (model_name == LogicalModels::MODEL_INPUT || model_name == LogicalModels::MODEL_OUTPUT)
        return true;

    std::string lowered = lower_copy(model_name);
    return lowered.find("io") != std::string::npos
           || lowered.find("pad") != std::string::npos
           || lowered.find("obuf") != std::string::npos
           || lowered.find("oct") != std::string::npos
           || lowered.find("termination") != std::string::npos;
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
    , models_(models)
    , net_weights_(ap_netlist.nets().size(), 1.0)
    , boundary_cohesion_nets_(ap_netlist.nets().size(), false)
    , io_chain_cohesion_nets_(ap_netlist.nets().size(), false)
    , device_grid_width_(device_grid.width())
    , device_grid_height_(device_grid.height())
    , device_grid_num_layers_(device_grid.get_num_layers())
    , ap_timing_tradeoff_(ap_timing_tradeoff)
    , io_chain_net_cohesion_weight_(kIoChainNetCohesionWeight)
    , pack_pattern_cohesion_weight_(kPackPatternCohesionWeight) {
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

    initialize_pack_pattern_cohesion_groups_(prepacker);

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

    size_t optimizable_pins = 0;
    for (APBlockId blk_id : optimizable_blocks_)
        optimizable_pins += ap_netlist_.block_pins(blk_id).size();
    double pins_per_optimizable_block = optimizable_blocks_.empty()
                                            ? 0.
                                            : static_cast<double>(optimizable_pins) / optimizable_blocks_.size();

    effective_timing_tradeoff_ = ap_timing_tradeoff_;
    if (ap_timing_tradeoff_ > 0.f
        && optimizable_blocks_.size() >= kAdaptiveTimingMinBlocks
        && optimizable_blocks_.size() <= kAdaptiveTimingMaxBlocks) {
        effective_timing_tradeoff_ = std::max(ap_timing_tradeoff_, static_cast<float>(kAdaptiveTimingTradeoff));
    }

    bool high_pin_seed = optimizable_blocks_.size() >= kHighPinWarmStartBlockThreshold
                         && pins_per_optimizable_block >= kHighPinWarmStartPinsPerBlock;
    bool huge_seed = optimizable_blocks_.size() >= kHugeWarmStartBlockThreshold;
    if (high_pin_seed || huge_seed)
        warmstart_iters_ = std::max(kWarmStartIters, kAdaptiveWarmStartIters);
    else
        warmstart_iters_ = kWarmStartIters;
    warmstart_max_iters_ = std::max(kWarmStartMaxIters, warmstart_iters_);

    if (log_verbosity_ >= 1) {
        VTR_LOG("Nonlinear Nesterov adaptive policy: blocks=%zu pins/block=%.2f warm-start-floor=%zu timing=%g io_chain_cohesion=%g pack_pattern_cohesion=%g groups=%zu.\n",
                optimizable_blocks_.size(),
                pins_per_optimizable_block,
                warmstart_iters_,
                effective_timing_tradeoff_,
                io_chain_net_cohesion_weight_,
                pack_pattern_cohesion_weight_,
                pack_pattern_cohesion_groups_.size());
    }

    // Build the B2B warm-start solver. Constructed here because the DeviceGrid is
    // only in scope during construction. Single-threaded so concurrent VPR runs do
    // not oversubscribe via Eigen.
    warmstart_solver_ = make_analytical_solver(e_ap_analytical_solver::LP_B2B,
                                               ap_netlist_,
                                               device_grid,
                                               atom_netlist,
                                               pre_cluster_timing_manager,
                                               place_delay_model,
                                               effective_timing_tradeoff_,
                                               1 /*num_threads*/,
                                               log_verbosity_);
}

NonlinearNesterovPlacer::~NonlinearNesterovPlacer() = default;

bool NonlinearNesterovPlacer::block_is_optimizable_(APBlockId blk_id) const {
    return ap_netlist_.block_mobility(blk_id) == APBlockMobility::MOVEABLE;
}

PartialPlacement NonlinearNesterovPlacer::initialize_placement_() {
    PartialPlacement p_placement(ap_netlist_);

    // No movable blocks: every block is fixed, so there is nothing for the
    // optimizer to do. Snap fixed blocks into device bounds and return.
    if (optimizable_blocks_.empty()) {
        project_placement_(p_placement);
        return p_placement;
    }

    // Warm start from a B2B/QP analytical solve. Iterate solve+legalize until the
    // seed HPWL stops improving (convergence-based), so large/under-converged
    // designs run enough cycles to produce a tight, clusterable seed -- the post-
    // APPack clustering inflation that drove the Titan/Titanium wirelength gap --
    // while small designs that converge fast stop at the floor. The legalizer
    // places every block (including solver-disconnected ones), so all optimizable
    // blocks have a valid location afterward.
    double previous_hpwl = std::numeric_limits<double>::infinity();
    size_t solver_iteration = 0;
    size_t min_cycles = warmstart_iters_;
    size_t max_cycles = warmstart_max_iters_;
    bool sparse_checked = false;
    bool reached_sparse_deepening = false;
    bool stopped_by_convergence = false;
    double sparse_seed_overflow = 0.;

    while (solver_iteration < max_cycles) {
        warmstart_solver_->solve(solver_iteration, p_placement);
        partial_legalizer_->legalize(p_placement);
        size_t cycles_done = solver_iteration + 1;

        double hpwl = p_placement.get_hpwl(ap_netlist_);
        bool converged = hpwl > previous_hpwl * (1.0 - kWarmStartTol);
        previous_hpwl = hpwl;

        if (cycles_done < min_cycles) {
            solver_iteration++;
            continue;
        }

        bool reached_max_cycles = cycles_done >= max_cycles;
        if (!converged && !reached_max_cycles) {
            solver_iteration++;
            continue;
        }

        // Sparsity-gated deep warm start. When the seed is so sparse that physical
        // mass barely exceeds tile capacity (overflow below the gate), the
        // electrostatic field has nothing to spread, so the smooth stage no-ops and
        // the placement is left at this loose seed -- APPack then cannot pack distant
        // molecules into shared logic blocks, inflating routed wirelength (measured
        // on sparse Stratix-10 designs fft2d/sobel: +21% routed WL). The cure is to
        // keep compacting with more B2B solve+legalize cycles (what SimPL does
        // implicitly), which the HPWL-plateau convergence stops too early. Dense
        // designs, where the field does real work, keep the short warm start so the
        // electrostatic stage is not handed an over-compacted seed.
        if (!sparse_checked && kSparseGateOverflow > 0. && solver_iteration < kSparseWarmStartIters) {
            project_placement_(p_placement);
            std::vector<PrimitiveVectorDim> dims = density_manager_->get_used_dims_mask().get_non_zero_dims();
            sparse_seed_overflow = compute_physical_overflow_ratio_(p_placement, dims);
            sparse_checked = true;
            if (sparse_seed_overflow < kSparseGateOverflow) {
                sparse_seed_ = true;
                min_cycles = kSparseWarmStartIters;
                max_cycles = std::max(max_cycles, kSparseWarmStartIters);
                reached_sparse_deepening = true;
                continue;
            }
        }

        stopped_by_convergence = converged;
        break;
    }
    project_placement_(p_placement);

    if (log_verbosity_ >= 1) {
        if (reached_sparse_deepening) {
            VTR_LOG("Nonlinear Nesterov warm start: sparse seed (overflow %.4f < %.4f); deepened to %zu cycles, HPWL %g.\n",
                    sparse_seed_overflow,
                    kSparseGateOverflow,
                    kSparseWarmStartIters,
                    p_placement.get_hpwl(ap_netlist_));
        } else {
            VTR_LOG("Nonlinear Nesterov warm start: %zu B2B solve+legalize cycles (%s), seed HPWL %g.\n",
                    std::min(solver_iteration + 1, warmstart_max_iters_),
                    stopped_by_convergence ? "converged" : "max iterations",
                    p_placement.get_hpwl(ap_netlist_));
        }
    }
    return p_placement;
}

PartialPlacement NonlinearNesterovPlacer::place() {
    vtr::ScopedStartFinishTimer global_placer_time("AP Nonlinear Nesterov Global Placer");

    std::vector<PrimitiveVectorDim> density_dimensions = density_manager_->get_used_dims_mask().get_non_zero_dims();
    boundary_confined_dims_ = identify_boundary_confined_dims_(density_dimensions);

    double device_span = std::max<double>(device_grid_width_, device_grid_height_);
    double convergence_displacement = std::max(kMinConvergenceDisplacement,
                                               device_span * kConvergenceDisplacementFraction);

    // Size-gate the preconditioner: enable it only for large designs (>= the
    // proximity-anchor threshold) at the timing-safe alpha, where it fixes the
    // large-device over-spread. Small/homogeneous designs are left unpreconditioned
    // (it is a pure perturbation there).
    precond_active_ = optimizable_blocks_.size() >= kProximitySizeThreshold;
    precond_alpha_active_ = kPreconditionLargeAlpha;
    return run_global_optimization_(density_dimensions, device_span, convergence_displacement);
}

PartialPlacement NonlinearNesterovPlacer::run_global_optimization_(const std::vector<PrimitiveVectorDim>& density_dimensions,
                                                                   double device_span,
                                                                   double convergence_displacement) {
    vtr::Timer warmstart_timer;
    PartialPlacement seed = initialize_placement_();
    if (log_verbosity_ >= 1)
        VTR_LOG("Nonlinear Nesterov phase time: warm start took %.2f seconds.\n", warmstart_timer.elapsed_sec());
    update_boundary_net_flags_(density_dimensions, seed);
    if (pack_pattern_cohesion_weight_ > 0. && num_io_chain_cohesion_nets_ == 0) {
        if (log_verbosity_ >= 1) {
            VTR_LOG("Nonlinear Nesterov pack-pattern cohesion disabled: no long direct I/O-chain nets were found.\n");
        }
        pack_pattern_cohesion_weight_ = 0.;
        pack_pattern_cohesion_groups_.clear();
    }

    PartialPlacement result = optimize_from_seed_(seed, density_dimensions, device_span, convergence_displacement);

    // Leave the pre-cluster timing manager consistent with the returned placement,
    // deterministically -- matching the lp-b2b handoff.
    if (pre_cluster_timing_manager_.is_valid() && place_delay_model_) {
        update_timing_info_with_partial_placement(pre_cluster_timing_manager_,
                                                  *place_delay_model_,
                                                  result,
                                                  ap_netlist_);
    }

    // Print the same post-global-placement statistics block as the SimPL placer,
    // so the standard QoR parse regexes (post_gp_hpwl/cpd/sTNS/overfill) are
    // populated for this placer as well.
    VTR_LOG("Placement after Global Placement:\n");
    print_placement_stats(result,
                          ap_netlist_,
                          *density_manager_,
                          pre_cluster_timing_manager_);
    return result;
}

PartialPlacement NonlinearNesterovPlacer::optimize_from_seed_(const PartialPlacement& seed,
                                                              const std::vector<PrimitiveVectorDim>& density_dimensions,
                                                              double device_span,
                                                              double convergence_displacement) {
    // Smooth global placement by accelerated gradient descent on a simple
    // weighted objective: smooth wirelength plus a per-resource electrostatic
    // density penalty. Each epoch runs an inner Nesterov solve, partially
    // legalizes the result to form a proximity anchor, then optionally increases
    // the fixed density weight through a short continuation schedule.
    PartialPlacement current(ap_netlist_);
    current.block_x_locs = seed.block_x_locs;
    current.block_y_locs = seed.block_y_locs;
    current.block_layer_nums = seed.block_layer_nums;
    current.block_sub_tiles = seed.block_sub_tiles;
    vtr::Timer epoch_phase_timer;
    double legalizer_time_sec = 0.;
    double timing_update_time_sec = 0.;

    // Sparse-seed guard: the seed already satisfies the density stop target, so
    // the full filler/epoch schedule can only waste runtime (its result loses the
    // HPWL selection to the seed on these designs). Run a short filler-free
    // wirelength-refinement probe instead; the seed-vs-epoch selection below
    // still protects quality either way.
    size_t num_epochs = sparse_seed_ ? kSparseSeedMaxEpochs : kNesterovEpochs;
    size_t iterations_per_epoch = sparse_seed_ ? kSparseSeedProbeIterations : kNesterovIterationsPerEpoch;
    if (sparse_seed_ && log_verbosity_ >= 1) {
        VTR_LOG("Nonlinear Nesterov sparse-seed guard: capping electrostatic phase to %zu filler-free epoch(s) of %zu iterations.\n",
                num_epochs, iterations_per_epoch);
    }

    FillerState current_fillers;
    initialize_dynamic_fillers_(seed,
                                density_dimensions,
                                sparse_seed_ ? 0. : kDynamicFillerWhitespaceFraction,
                                current_fillers);

    // The initial density weight is derived from the seed's smooth wirelength, which
    // is net-weighted, so start from unit weights before the epoch loop refreshes
    // timing at epoch 0.
    std::fill(net_weights_.begin(), net_weights_.end(), 1.0);
    // Gamma continuation seeds the fixed fraction; the epoch loop overrides it
    // with the annealed coarse->sharp schedule below.
    current_gamma_fraction_ = kWirelengthGammaFraction;
    std::vector<double> density_multipliers(density_dimensions.size(), 1.);
    // Seed the density multiplier so the density term starts at a small fixed
    // fraction of the initial wirelength.
    double initial_density_weight = 1e-3;
    auto reset_density_weights = [&](const PartialPlacement& placement) {
        std::fill(density_multipliers.begin(), density_multipliers.end(), 1.);
        ObjectiveValue components = evaluate_objective_(placement,
                                                        density_multipliers,
                                                        std::nullopt,
                                                        0.,
                                                        std::nullopt,
                                                        current_fillers,
                                                        nullptr);
        initial_density_weight = 1e-3;
        if (components.density > kEpsilon) {
            initial_density_weight = kInitialDensityToWirelengthRatio * std::max(components.wirelength, 1.0) / components.density;
        }
        initial_density_weight = std::clamp(initial_density_weight, 1e-5, 1e3);
        for (size_t dim_idx = 0; dim_idx < density_dimensions.size(); dim_idx++)
            density_multipliers[dim_idx] = initial_density_weight;
    };
    reset_density_weights(current);

    if (log_verbosity_ >= 1) {
        VTR_LOG("Epoch  Pre HPWL  Post HPWL  Pre Oflow  Post Oflow  Pre Max  Post Max  Mean Move  Max Move  Density Wt  Prox Wt\n");
        VTR_LOG("-----  --------  ---------  ---------  ----------  -------  --------  ---------  --------  ----------  -------\n");
    }

    double legalizer_feedback_proximity_weight = 0.;
    PartialPlacement best_placement(ap_netlist_);
    best_placement.block_x_locs = current.block_x_locs;
    best_placement.block_y_locs = current.block_y_locs;
    best_placement.block_layer_nums = current.block_layer_nums;
    best_placement.block_sub_tiles = current.block_sub_tiles;
    double best_hpwl = current.get_hpwl(ap_netlist_);
    int best_source = -1; // -1 = warm-start seed, otherwise epoch index.
    PartialPlacement legal_anchor(ap_netlist_);
    PartialPlacement y_placement(ap_netlist_);
    PartialPlacement next(ap_netlist_);
    PartialPlacement before_legalization(ap_netlist_);
    FillerState y_fillers;
    FillerState next_fillers;

    // Short continuation loop. Density weights are fixed within an epoch and
    // follow a simple geometric ramp between epochs; timing net weights and the
    // preconditioner are refreshed against the current placement at the start of
    // each one.
    for (size_t epoch = 0; epoch < num_epochs; epoch++) {
        // Anneal the wirelength smoothing fraction geometrically from coarse
        // (smooth global gradient) to sharp (near true HPWL) across epochs, so
        // early epochs spread on an easy landscape and later epochs recover real
        // wirelength. The sparse-seed probe refines wirelength near an already
        // spread seed, so it goes straight to the sharp end of the schedule.
        if (sparse_seed_) {
            current_gamma_fraction_ = kGammaEndFraction;
        } else if (kNesterovEpochs > 1) {
            double schedule = static_cast<double>(epoch) / static_cast<double>(kNesterovEpochs - 1);
            current_gamma_fraction_ = kGammaStartFraction
                                      * std::pow(kGammaEndFraction / kGammaStartFraction, schedule);
        }
        if (effective_timing_tradeoff_ != 0.f && place_delay_model_) {
            vtr::Timer timing_update_timer;
            update_timing_info_with_partial_placement(pre_cluster_timing_manager_,
                                                      *place_delay_model_,
                                                      current,
                                                      ap_netlist_);
            timing_update_time_sec += timing_update_timer.elapsed_sec();
        }
        update_timing_net_weights_();
        double density_weight_scale = 1.0;
        if (!sparse_seed_ && num_epochs > 1) {
            double schedule = static_cast<double>(epoch) / static_cast<double>(num_epochs - 1);
            density_weight_scale = std::pow(kFinalDensityWeightMultiplier, schedule);
        }
        for (double& multiplier : density_multipliers)
            multiplier = initial_density_weight * density_weight_scale;
        compute_preconditioner_(density_dimensions, density_multipliers);

        legal_anchor.block_x_locs = current.block_x_locs;
        legal_anchor.block_y_locs = current.block_y_locs;
        legal_anchor.block_layer_nums = current.block_layer_nums;
        legal_anchor.block_sub_tiles = current.block_sub_tiles;
        y_placement.block_x_locs = current.block_x_locs;
        y_placement.block_y_locs = current.block_y_locs;
        y_placement.block_layer_nums = current.block_layer_nums;
        y_placement.block_sub_tiles = current.block_sub_tiles;
        next.block_x_locs = current.block_x_locs;
        next.block_y_locs = current.block_y_locs;
        next.block_layer_nums = current.block_layer_nums;
        next.block_sub_tiles = current.block_sub_tiles;
        y_fillers.x = current_fillers.x;
        y_fillers.y = current_fillers.y;
        y_fillers.layer = current_fillers.layer;
        next_fillers.x = current_fillers.x;
        next_fillers.y = current_fillers.y;
        next_fillers.layer = current_fillers.layer;
        PlacementGradient grad(ap_netlist_);
        FillerGradient filler_grad;
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
                                                         std::cref(legal_anchor),
                                                         proximity_weight,
                                                         std::nullopt,
                                                         current_fillers,
                                                         nullptr);

        // Nesterov accelerated-gradient inner solve. The gradient is taken at the
        // extrapolated look-ahead point y_placement; a backtracking line search
        // halves the step until the objective decreases.
        for (size_t iter = 0; iter < iterations_per_epoch; iter++) {
            ObjectiveValue y_obj = evaluate_objective_(y_placement,
                                                       density_multipliers,
                                                       std::cref(legal_anchor),
                                                       proximity_weight,
                                                       std::ref(grad),
                                                       y_fillers,
                                                       &filler_grad);
            double grad_norm_sq = gradient_norm_squared_(grad) + filler_gradient_norm_squared_(filler_grad);
            if (grad_norm_sq < kEpsilon)
                break;

            // Monotone backtracking line search: halve the step until the objective
            // does not increase (or the minimum step is reached).
            double accepted_step = step_size;
            ObjectiveValue next_obj;
            bool accepted = false;
            while (accepted_step >= kMinStepSize) {
                gradient_step_(y_placement, grad, y_fillers, filler_grad, accepted_step, next, next_fillers);
                next_obj = evaluate_objective_(next,
                                               density_multipliers,
                                               std::cref(legal_anchor),
                                               proximity_weight,
                                               std::nullopt,
                                               next_fillers,
                                               nullptr);
                if (next_obj.total <= y_obj.total || accepted_step == kMinStepSize) {
                    accepted = true;
                    break;
                }
                accepted_step *= 0.5;
            }

            if (!accepted)
                break;

            double max_step_displacement = std::max(max_block_displacement_(y_placement, next),
                                                    max_filler_displacement_(y_fillers, next_fillers));

            // FISTA momentum: the t-sequence sets the extrapolation weight beta.
            double next_t = 0.5 * (1.0 + std::sqrt(1.0 + 4.0 * nesterov_t * nesterov_t));
            double beta = (nesterov_t - 1.0) / next_t;

            if (next_obj.total > current_obj.total) {
                // Adaptive restart: the step worsened the objective, so drop the
                // momentum and continue from the new point without extrapolation.
                nesterov_t = 1.0;
                y_placement.block_x_locs = next.block_x_locs;
                y_placement.block_y_locs = next.block_y_locs;
                y_placement.block_layer_nums = next.block_layer_nums;
                y_placement.block_sub_tiles = next.block_sub_tiles;
                y_fillers.x = next_fillers.x;
                y_fillers.y = next_fillers.y;
                y_fillers.layer = next_fillers.layer;
            } else {
                // Extrapolate the look-ahead point ahead of the accepted step.
                extrapolate_(current, next, current_fillers, next_fillers, beta, y_placement, y_fillers);
                nesterov_t = next_t;
            }

            current.block_x_locs = next.block_x_locs;
            current.block_y_locs = next.block_y_locs;
            current.block_layer_nums = next.block_layer_nums;
            current.block_sub_tiles = next.block_sub_tiles;
            current_fillers.x = next_fillers.x;
            current_fillers.y = next_fillers.y;
            current_fillers.layer = next_fillers.layer;
            current_obj = next_obj;

            step_size = std::min(device_span, accepted_step * 1.05);

            if (iter + 1 >= kMinNesterovIterationsPerEpoch
                && max_step_displacement <= convergence_displacement) {
                break;
            }
        }

        ObjectiveValue pre_legalization = evaluate_objective_(current,
                                                              density_multipliers,
                                                              std::cref(legal_anchor),
                                                              proximity_weight,
                                                              std::nullopt,
                                                              current_fillers,
                                                              nullptr);
        // Partially legalize the smooth result. This both cleans up overlap and
        // produces the anchor that the next epoch's proximity term pulls toward;
        // the per-epoch displacement it causes also drives the proximity weight.
        before_legalization.block_x_locs = current.block_x_locs;
        before_legalization.block_y_locs = current.block_y_locs;
        before_legalization.block_layer_nums = current.block_layer_nums;
        before_legalization.block_sub_tiles = current.block_sub_tiles;
        double pre_leg_overflow = compute_physical_overflow_ratio_(before_legalization, density_dimensions);
        vtr::Timer legalizer_timer;
        partial_legalizer_->legalize(current);
        legalizer_time_sec += legalizer_timer.elapsed_sec();
        ObjectiveValue post_legalization = evaluate_objective_(current,
                                                               density_multipliers,
                                                               std::cref(legal_anchor),
                                                               proximity_weight,
                                                               std::nullopt,
                                                               current_fillers,
                                                               nullptr);

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

        double post_legalization_hpwl = current.get_hpwl(ap_netlist_);
        if (post_legalization_hpwl < best_hpwl) {
            best_hpwl = post_legalization_hpwl;
            best_placement.block_x_locs = current.block_x_locs;
            best_placement.block_y_locs = current.block_y_locs;
            best_placement.block_layer_nums = current.block_layer_nums;
            best_placement.block_sub_tiles = current.block_sub_tiles;
            best_source = static_cast<int>(epoch);
        }

        VTR_LOG("%5zu  %8.2f  %9.2f  %9.4f  %10.4f  %7.4f  %8.4f  %9.4f  %8.4f  %10.4g  %7.4g\n",
                epoch,
                before_legalization.get_hpwl(ap_netlist_),
                post_legalization_hpwl,
                pre_legalization.total_overflow,
                post_legalization.total_overflow,
                pre_legalization.max_overflow,
                post_legalization.max_overflow,
                mean_displacement,
                max_displacement,
                density_multipliers.empty() ? 0. : density_multipliers.front(),
                proximity_weight);

        // #2 overflow-target stop: once the smooth (pre-legalization) placement is
        // already spread enough that physical mass barely exceeds tile capacity,
        // further density tightening only costs wirelength, so end the loop.
        if (kTargetOverflow > 0.
            && epoch + 1 >= kMinEpochsBeforeOverflowStop
            && epoch + 1 < num_epochs) {
            if (pre_leg_overflow <= kTargetOverflow) {
                if (log_verbosity_ >= 1) {
                    VTR_LOG("Nonlinear Nesterov: physical overflow %.4f <= target %.4f after epoch %zu; stopping density tightening.\n",
                            pre_leg_overflow, kTargetOverflow, epoch);
                }
                break;
            }
        }
    }

    VTR_LOG("Nonlinear Nesterov Global Placer Statistics:\n");
    VTR_LOG("\tFinal-epoch placement HPWL after partial legalization: %g\n", current.get_hpwl(ap_netlist_));
    if (best_source < 0)
        VTR_LOG("\tSelected placement HPWL: %g (warm-start seed)\n", best_hpwl);
    else
        VTR_LOG("\tSelected placement HPWL: %g (epoch %d)\n", best_hpwl, best_source);
    VTR_LOG("\tFinal first density weight: %g\n", density_multipliers.empty() ? 0. : density_multipliers.front());
    if (log_verbosity_ >= 1) {
        VTR_LOG("Nonlinear Nesterov phase time: epoch loop took %.2f seconds (partial legalization %.2f, timing updates %.2f).\n",
                epoch_phase_timer.elapsed_sec(), legalizer_time_sec, timing_update_time_sec);
    }
    partial_legalizer_->print_statistics();

    return best_placement;
}

NonlinearNesterovPlacer::ObjectiveValue NonlinearNesterovPlacer::evaluate_objective_(const PartialPlacement& p_placement,
                                                                                     const std::vector<double>& density_multipliers,
                                                                                     std::optional<std::reference_wrapper<const PartialPlacement>> legal_anchor,
                                                                                     double proximity_weight,
                                                                                     std::optional<std::reference_wrapper<PlacementGradient>> grad,
                                                                                     const FillerState& fillers,
                                                                                     FillerGradient* filler_grad) const {
    if (grad)
        grad->get().clear();

    ObjectiveValue value;
    value.wirelength = add_wirelength_gradient_(p_placement, grad);
    value.density = add_density_gradient_(p_placement,
                                          density_multipliers,
                                          value.density_energies,
                                          value.total_overflow,
                                          value.max_overflow,
                                          value.overflow_ratio,
                                          grad,
                                          fillers,
                                          filler_grad);
    if (legal_anchor)
        value.proximity = add_proximity_gradient_(p_placement,
                                                  legal_anchor->get(),
                                                  proximity_weight,
                                                  grad);
    value.pack_pattern_cohesion = add_pack_pattern_cohesion_gradient_(p_placement, grad);
    value.total = value.wirelength
                  + pack_pattern_cohesion_weight_ * value.pack_pattern_cohesion
                  + proximity_weight * value.proximity;
    for (size_t dim_idx = 0; dim_idx < value.density_energies.size(); dim_idx++) {
        double energy = value.density_energies[dim_idx];
        value.total += density_multipliers[dim_idx] * energy;
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
    double gamma = std::max(1.0, std::max<double>(device_grid_width_, device_grid_height_) * current_gamma_fraction_);
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
        OptionalWeightVectorRef positive_x_weights_ref;
        OptionalWeightVectorRef negative_x_weights_ref;
        OptionalWeightVectorRef positive_y_weights_ref;
        OptionalWeightVectorRef negative_y_weights_ref;
        if (grad) {
            positive_x_weights_ref = std::ref(positive_x_weights);
            negative_x_weights_ref = std::ref(negative_x_weights);
            positive_y_weights_ref = std::ref(positive_y_weights);
            negative_y_weights_ref = std::ref(negative_y_weights);
        }

        double positive_x = weighted_average_coordinate(x_locs, gamma, false, positive_x_weights_ref);
        double negative_x = weighted_average_coordinate(x_locs, gamma, true, negative_x_weights_ref);
        double positive_y = weighted_average_coordinate(y_locs, gamma, false, positive_y_weights_ref);
        double negative_y = weighted_average_coordinate(y_locs, gamma, true, negative_y_weights_ref);

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

    bool use_timing_weights = effective_timing_tradeoff_ != 0.f && pre_cluster_timing_manager_.is_valid();
    if (effective_timing_tradeoff_ != 0.f && !pre_cluster_timing_manager_.is_valid()) {
        VTR_LOG_WARN("Nonlinear Nesterov analytical placement requested timing tradeoff %g, but pre-cluster timing is unavailable; using unit net weights.\n",
                     effective_timing_tradeoff_);
    }

    double total_weight = 0.;
    double min_weight = std::numeric_limits<double>::infinity();
    double max_weight = 0.;
    size_t weighted_nets = 0;

    for (APNetId net_id : ap_netlist_.nets()) {
        if (ap_netlist_.net_is_ignored(net_id))
            continue;

        double weight = 1.0;
        if (use_timing_weights) {
            AtomNetId atom_net_id = ap_netlist_.net_atom_net(net_id);
            VTR_ASSERT_SAFE(atom_net_id.is_valid());

            double crit = pre_cluster_timing_manager_.calc_net_setup_criticality(atom_net_id, atom_netlist_);
            weight = effective_timing_tradeoff_ * crit + (1.0 - effective_timing_tradeoff_);
        }

        // Nets flagged by update_boundary_net_flags_/block_is_io_chain_block_ get extra
        // wirelength weight so their pin blocks are pulled tightly together; this counteracts
        // the smooth objective's tendency to let long boundary-anchored and I/O-chain nets
        // spread out, which otherwise fragments those chains across the AP-to-APPack handoff.
        if (static_cast<size_t>(net_id) < boundary_cohesion_nets_.size() && boundary_cohesion_nets_[net_id])
            weight *= kBoundaryNetCohesionWeight;
        if (static_cast<size_t>(net_id) < io_chain_cohesion_nets_.size() && io_chain_cohesion_nets_[net_id])
            weight *= io_chain_net_cohesion_weight_;
        net_weights_[net_id] = weight;

        total_weight += weight;
        min_weight = std::min(min_weight, weight);
        max_weight = std::max(max_weight, weight);
        weighted_nets++;
    }

    if (log_verbosity_ >= 1 && weighted_nets > 0) {
        VTR_LOG("Nonlinear Nesterov timing/cohesion net weights: tradeoff=%g boundary_cohesion=%g io_chain_cohesion=%g min=%g avg=%g max=%g nets=%zu\n",
                effective_timing_tradeoff_,
                kBoundaryNetCohesionWeight,
                io_chain_net_cohesion_weight_,
                min_weight,
                total_weight / weighted_nets,
                max_weight,
                weighted_nets);
    }
}

void NonlinearNesterovPlacer::initialize_pack_pattern_cohesion_groups_(const Prepacker& prepacker) {
    pack_pattern_cohesion_groups_.clear();
    if (pack_pattern_cohesion_weight_ == 0.)
        return;

    std::vector<std::vector<APBlockId>> chain_groups(prepacker.get_num_molecule_chains());
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        std::vector<MoleculeChainId> block_chain_ids;
        for (PackMoleculeId mol_id : ap_netlist_.block_molecules(blk_id)) {
            const t_pack_molecule& molecule = prepacker.get_molecule(mol_id);
            if (!molecule.is_chain() || !molecule.chain_id.is_valid())
                continue;
            if (!prepacker.get_molecule_chain_info(molecule.chain_id).is_long_chain)
                continue;
            if (std::find(block_chain_ids.begin(), block_chain_ids.end(), molecule.chain_id) != block_chain_ids.end())
                continue;

            size_t chain_idx = static_cast<size_t>(molecule.chain_id);
            VTR_ASSERT_SAFE(chain_idx < chain_groups.size());
            chain_groups[chain_idx].push_back(blk_id);
            block_chain_ids.push_back(molecule.chain_id);
        }
    }

    for (std::vector<APBlockId>& group : chain_groups) {
        if (group.size() < 2)
            continue;
        pack_pattern_cohesion_groups_.push_back(std::move(group));
    }

    if (log_verbosity_ >= 1) {
        size_t grouped_blocks = 0;
        for (const std::vector<APBlockId>& group : pack_pattern_cohesion_groups_)
            grouped_blocks += group.size();
        VTR_LOG("Nonlinear Nesterov pack-pattern cohesion: %zu long-chain groups, %zu grouped AP blocks, weight=%g.\n",
                pack_pattern_cohesion_groups_.size(),
                grouped_blocks,
                pack_pattern_cohesion_weight_);
    }
}

double NonlinearNesterovPlacer::add_pack_pattern_cohesion_gradient_(const PartialPlacement& p_placement,
                                                                    std::optional<std::reference_wrapper<PlacementGradient>> grad) const {
    if (pack_pattern_cohesion_weight_ == 0. || pack_pattern_cohesion_groups_.empty())
        return 0.;

    double cohesion_penalty = 0.;
    for (const std::vector<APBlockId>& group : pack_pattern_cohesion_groups_) {
        VTR_ASSERT_SAFE(group.size() >= 2);
        double centroid_x = 0.;
        double centroid_y = 0.;
        for (APBlockId blk_id : group) {
            centroid_x += p_placement.block_x_locs[blk_id];
            centroid_y += p_placement.block_y_locs[blk_id];
        }

        double inv_group_size = 1. / static_cast<double>(group.size());
        centroid_x *= inv_group_size;
        centroid_y *= inv_group_size;

        for (APBlockId blk_id : group) {
            double dx = p_placement.block_x_locs[blk_id] - centroid_x;
            double dy = p_placement.block_y_locs[blk_id] - centroid_y;
            cohesion_penalty += 0.5 * inv_group_size * (dx * dx + dy * dy);
            if (grad && block_is_optimizable_(blk_id)) {
                grad->get().dx[blk_id] += pack_pattern_cohesion_weight_ * inv_group_size * dx;
                grad->get().dy[blk_id] += pack_pattern_cohesion_weight_ * inv_group_size * dy;
            }
        }
    }

    return cohesion_penalty;
}

double NonlinearNesterovPlacer::add_density_gradient_(const PartialPlacement& p_placement,
                                                      const std::vector<double>& density_multipliers,
                                                      std::vector<double>& density_energies,
                                                      double& total_overflow,
                                                      double& max_overflow,
                                                      double& overflow_ratio,
                                                      std::optional<std::reference_wrapper<PlacementGradient>> grad,
                                                      const FillerState& fillers,
                                                      FillerGradient* filler_grad) const {
    total_overflow = 0.;
    max_overflow = 0.;
    overflow_ratio = 0.;
    // ePlace legality signal: overflow mass (mass exceeding tile target) over total
    // deposited mass. Normalizing by movable mass (not total grid capacity) avoids
    // diluting local overfill on designs that occupy a small fraction of the grid.
    double overflow_area = 0.;
    double total_deposited_mass = 0.;

    const FlatPlacementBins& bins = density_manager_->flat_placement_bins();
    std::vector<PrimitiveVectorDim> dimensions = density_manager_->get_used_dims_mask().get_non_zero_dims();
    if (dimensions.empty())
        return 0.;
    VTR_ASSERT(density_multipliers.size() == dimensions.size());
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
    //
    std::vector<double> target_norm_floor(dimensions.size(), kEpsilon);
    std::vector<double> residual_charge_scale(dimensions.size(), 1.0);
    for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
        double target_sum = 0.;
        size_t target_sites = 0;
        for (double target : target_capacity[dim_idx]) {
            if (target <= kEpsilon)
                continue;
            target_sum += target;
            target_sites++;
        }
        if (target_sites != 0) {
            target_norm_floor[dim_idx] = std::max(kEpsilon,
                                                  kDensityTargetFloorFraction * target_sum / target_sites);
            residual_charge_scale[dim_idx] = std::max(kEpsilon, target_sum / target_sites);
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
        for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
            double mass = block_mass.get_dim_val(dimensions[dim_idx]);
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

    // Overflow and stopping use only real physical block mass. Fillers are
    // movable whitespace in the density objective, not legal instances.
    for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
        double dim_overflow_area = 0.;
        double dim_deposited_mass = 0.;
        for (size_t idx = 0; idx < num_sites; idx++) {
            double target = target_capacity[dim_idx][idx];
            double utilization_at_site = utilization[dim_idx][idx];
            if (target <= kEpsilon) {
                if (utilization_at_site <= kEpsilon)
                    continue;
                total_overflow += utilization_at_site;
                max_overflow = std::max(max_overflow, utilization_at_site);
                dim_overflow_area += utilization_at_site;
                dim_deposited_mass += utilization_at_site;
            } else {
                double normalized_utilization = utilization_at_site / std::max(target, target_norm_floor[dim_idx]);
                double normalized_overflow = std::max(0., normalized_utilization - 1.0);
                total_overflow += normalized_overflow;
                max_overflow = std::max(max_overflow, normalized_overflow);
                dim_overflow_area += std::max(0., utilization_at_site - target);
                dim_deposited_mass += utilization_at_site;
            }
        }
        overflow_area += dim_overflow_area;
        total_deposited_mass += dim_deposited_mass;
    }

    // Deposit dynamic fillers into the electrostatic field. They carry density
    // charge but are deliberately excluded from overflow accounting above.
    for (size_t dim_idx = 0; dim_idx < dimensions.size() && dim_idx < fillers.x.size(); dim_idx++) {
        double unit_mass = dim_idx < filler_unit_mass_.size() ? filler_unit_mass_[dim_idx] : 0.;
        if (unit_mass <= 0.)
            continue;
        size_t n = fillers.x[dim_idx].size();
        for (size_t filler_idx = 0; filler_idx < n; filler_idx++) {
            double x = std::clamp(fillers.x[dim_idx][filler_idx], 0., device_grid_width_ - kDeviceBoundaryEpsilon);
            double y = std::clamp(fillers.y[dim_idx][filler_idx], 0., device_grid_height_ - kDeviceBoundaryEpsilon);
            size_t layer = static_cast<size_t>(std::clamp(fillers.layer[dim_idx][filler_idx],
                                                          0,
                                                          static_cast<int>(device_grid_num_layers_ - 1)));
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
                        utilization[dim_idx][site_index(layer, xs[xi], ys[yi])] += unit_mass * weight;
                }
            }
        }
    }

    // Electrostatic density model (ePlace/elfPlace), solved independently per
    // resource dimension. Treat the excess block density at each tile as electric
    // charge; solving Poisson's equation gives a potential whose negative
    // gradient is a force that pushes blocks from dense to sparse regions. The
    // residual-charge mode uses (utilization - target) in area units, normalized
    // by the average target capacity for this resource dimension. This avoids
    // over-amplifying fractional-capacity sites on heterogeneous FPGA grids.
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
                charge[idx] = kUseResidualDensityCharge
                                  ? utilization_at_site / residual_charge_scale[dim_idx]
                                  : utilization_at_site;
            } else {
                double normalized_utilization = utilization_at_site / std::max(target, target_norm_floor[dim_idx]);
                charge[idx] = kUseResidualDensityCharge
                                  ? (utilization_at_site - target) / residual_charge_scale[dim_idx]
                                  : normalized_utilization - 1.0;
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
    }

    overflow_ratio = total_deposited_mass > kEpsilon ? overflow_area / total_deposited_mass : 0.;

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

        for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
            double mass = block_mass.get_dim_val(dimensions[dim_idx]);
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

            double normalized_mass = kUseResidualDensityCharge
                                         ? mass / residual_charge_scale[dim_idx]
                                         : mass / std::max(local_target, target_norm_floor[dim_idx]);
            double coefficient = density_multipliers[dim_idx];
            grad->get().dx[blk_id] += coefficient * normalized_mass * local_field_x;
            grad->get().dy[blk_id] += coefficient * normalized_mass * local_field_y;
        }
    }

    if (filler_grad) {
        filler_grad->dx.assign(dimensions.size(), {});
        filler_grad->dy.assign(dimensions.size(), {});
        for (size_t dim_idx = 0; dim_idx < dimensions.size() && dim_idx < fillers.x.size(); dim_idx++) {
            double unit_mass = dim_idx < filler_unit_mass_.size() ? filler_unit_mass_[dim_idx] : 0.;
            size_t n = fillers.x[dim_idx].size();
            filler_grad->dx[dim_idx].assign(n, 0.);
            filler_grad->dy[dim_idx].assign(n, 0.);
            if (unit_mass <= 0.)
                continue;
            double coefficient = density_multipliers[dim_idx];
            double normalized_mass = kUseResidualDensityCharge
                                         ? unit_mass / residual_charge_scale[dim_idx]
                                         : unit_mass;
            for (size_t filler_idx = 0; filler_idx < n; filler_idx++) {
                double x = std::clamp(fillers.x[dim_idx][filler_idx], 0., device_grid_width_ - kDeviceBoundaryEpsilon);
                double y = std::clamp(fillers.y[dim_idx][filler_idx], 0., device_grid_height_ - kDeviceBoundaryEpsilon);
                size_t layer = static_cast<size_t>(std::clamp(fillers.layer[dim_idx][filler_idx],
                                                              0,
                                                              static_cast<int>(device_grid_num_layers_ - 1)));
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
                for (size_t xi = 0; xi < 2; xi++) {
                    for (size_t yi = 0; yi < 2; yi++) {
                        double weight = wx[xi] * wy[yi];
                        size_t idx = site_index(layer, xs[xi], ys[yi]);
                        local_field_x += weight * field_x[dim_idx][idx];
                        local_field_y += weight * field_y[dim_idx][idx];
                    }
                }
                filler_grad->dx[dim_idx][filler_idx] = coefficient * normalized_mass * local_field_x;
                filler_grad->dy[dim_idx][filler_idx] = coefficient * normalized_mass * local_field_y;
            }
        }
    }

    return density_energy;
}

void NonlinearNesterovPlacer::initialize_dynamic_fillers_(const PartialPlacement& seed,
                                                          const std::vector<PrimitiveVectorDim>& dimensions,
                                                          double whitespace_fraction,
                                                          FillerState& fillers) {
    fillers.x.assign(dimensions.size(), {});
    fillers.y.assign(dimensions.size(), {});
    fillers.layer.assign(dimensions.size(), {});
    filler_unit_mass_.assign(dimensions.size(), 0.);
    filler_precond_.assign(dimensions.size(), kPreconditionFloor);
    if (dimensions.empty() || whitespace_fraction <= 0.) {
        if (log_verbosity_ >= 1)
            VTR_LOG("Nonlinear Nesterov dynamic fillers: disabled (whitespace fraction %g).\n", whitespace_fraction);
        return;
    }

    const FlatPlacementBins& bins = density_manager_->flat_placement_bins();
    size_t width = device_grid_width_;
    size_t height = device_grid_height_;
    size_t num_layers = device_grid_num_layers_;
    size_t num_sites = width * height * num_layers;
    auto site_index = [width, height](size_t layer, size_t x, size_t y) {
        return (layer * height + y) * width + x;
    };

    std::vector<std::vector<double>> target_capacity(dimensions.size(), std::vector<double>(num_sites, 0.));
    for (size_t layer = 0; layer < num_layers; layer++) {
        for (size_t x = 0; x < width; x++) {
            for (size_t y = 0; y < height; y++) {
                FlatPlacementBinId bin_id = density_manager_->get_bin(x, y, layer);
                const vtr::Rect<double>& region = bins.bin_region(bin_id);
                double bin_area = std::max(1.0, region.width() * region.height());
                double target_density = density_manager_->get_bin_target_density(bin_id);
                size_t idx = site_index(layer, x, y);
                for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
                    target_capacity[dim_idx][idx] = density_manager_->get_bin_capacity(bin_id).get_dim_val(dimensions[dim_idx])
                                                    * target_density / bin_area;
                }
            }
        }
    }

    std::vector<std::vector<double>> utilization(dimensions.size(), std::vector<double>(num_sites, 0.));
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        PrimitiveVector block_mass = density_manager_->mass_calculator().get_block_mass(blk_id);
        if (block_mass.is_zero())
            continue;
        double x = std::clamp(seed.block_x_locs[blk_id], 0., device_grid_width_ - kDeviceBoundaryEpsilon);
        double y = std::clamp(seed.block_y_locs[blk_id], 0., device_grid_height_ - kDeviceBoundaryEpsilon);
        size_t layer = static_cast<size_t>(std::clamp(std::round(seed.block_layer_nums[blk_id]),
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
        for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
            double mass = block_mass.get_dim_val(dimensions[dim_idx]);
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

    std::mt19937 rng(0x51f17e5u);
    size_t total_fillers = 0;
    for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
        double target_total = 0.;
        double movable_total = 0.;
        double target_sites = 0.;
        std::vector<double> whitespace(num_sites, 0.);
        for (size_t idx = 0; idx < num_sites; idx++) {
            double target = target_capacity[dim_idx][idx];
            target_total += target;
            movable_total += utilization[dim_idx][idx];
            if (target > kEpsilon)
                target_sites += 1.;
            whitespace[idx] = std::max(0., target - utilization[dim_idx][idx]);
        }

        double filler_total = whitespace_fraction * std::max(0., target_total - movable_total);
        if (filler_total <= kEpsilon || target_sites <= 0.)
            continue;

        double average_target = std::max(kEpsilon, target_total / target_sites);
        double unit_mass = average_target * kDynamicFillerUnitFraction;
        if (filler_total / std::max(unit_mass, kEpsilon) > static_cast<double>(kMaxDynamicFillersPerDim))
            unit_mass = filler_total / static_cast<double>(kMaxDynamicFillersPerDim);
        size_t num_fillers = std::max<size_t>(1, static_cast<size_t>(std::llround(filler_total / unit_mass)));
        filler_unit_mass_[dim_idx] = filler_total / static_cast<double>(num_fillers);

        double whitespace_total = 0.;
        for (double value : whitespace)
            whitespace_total += value;
        if (whitespace_total <= kEpsilon) {
            for (size_t idx = 0; idx < num_sites; idx++)
                whitespace[idx] = target_capacity[dim_idx][idx];
        }

        std::discrete_distribution<size_t> site_dist(whitespace.begin(), whitespace.end());
        std::uniform_real_distribution<double> jitter(0.05, 0.95);
        fillers.x[dim_idx].resize(num_fillers);
        fillers.y[dim_idx].resize(num_fillers);
        fillers.layer[dim_idx].resize(num_fillers);
        for (size_t filler_idx = 0; filler_idx < num_fillers; filler_idx++) {
            size_t idx = site_dist(rng);
            size_t layer = idx / (width * height);
            size_t rem = idx % (width * height);
            size_t x = rem % width;
            size_t y = rem / width;
            fillers.x[dim_idx][filler_idx] = static_cast<double>(x) + jitter(rng);
            fillers.y[dim_idx][filler_idx] = static_cast<double>(y) + jitter(rng);
            fillers.layer[dim_idx][filler_idx] = static_cast<int>(layer);
        }
        total_fillers += num_fillers;
    }
    project_fillers_(fillers);

    if (log_verbosity_ >= 1) {
        VTR_LOG("Nonlinear Nesterov dynamic fillers: %zu particles across %zu resource dims (fraction=%g).\n",
                total_fillers,
                dimensions.size(),
                whitespace_fraction);
    }
}

std::vector<bool> NonlinearNesterovPlacer::identify_boundary_confined_dims_(const std::vector<PrimitiveVectorDim>& dimensions) const {
    std::vector<bool> boundary_confined(dimensions.size(), false);
    if (dimensions.empty())
        return boundary_confined;

    const FlatPlacementBins& bins = density_manager_->flat_placement_bins();
    size_t width = device_grid_width_;
    size_t height = device_grid_height_;
    size_t num_layers = device_grid_num_layers_;

    for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
        double target_total = 0.;
        double boundary_target_total = 0.;
        for (size_t layer = 0; layer < num_layers; layer++) {
            for (size_t x = 0; x < width; x++) {
                for (size_t y = 0; y < height; y++) {
                    FlatPlacementBinId bin_id = density_manager_->get_bin(x, y, layer);
                    const vtr::Rect<double>& region = bins.bin_region(bin_id);
                    double bin_area = std::max(1.0, region.width() * region.height());
                    double target_density = density_manager_->get_bin_target_density(bin_id);
                    double target = density_manager_->get_bin_capacity(bin_id).get_dim_val(dimensions[dim_idx])
                                    * target_density / bin_area;
                    target_total += target;
                    bool in_boundary_band = x < kBoundaryConfinedBandTiles
                                            || y < kBoundaryConfinedBandTiles
                                            || x + kBoundaryConfinedBandTiles >= width
                                            || y + kBoundaryConfinedBandTiles >= height;
                    if (in_boundary_band)
                        boundary_target_total += target;
                }
            }
        }
        boundary_confined[dim_idx] = target_total > kEpsilon
                                     && boundary_target_total >= kBoundaryConfinedCapacityFraction * target_total;
    }

    if (log_verbosity_ >= 1) {
        size_t num_boundary_dims = 0;
        for (bool is_boundary : boundary_confined) {
            if (is_boundary)
                num_boundary_dims++;
        }
        VTR_LOG("Nonlinear Nesterov boundary-confined resource dims: %zu / %zu (edge band=%zu, threshold=%g).\n",
                num_boundary_dims,
                dimensions.size(),
                kBoundaryConfinedBandTiles,
                kBoundaryConfinedCapacityFraction);
    }

    return boundary_confined;
}

bool NonlinearNesterovPlacer::block_has_boundary_mass_(APBlockId blk_id,
                                                       const std::vector<PrimitiveVectorDim>& dimensions) const {
    if (boundary_confined_dims_.size() != dimensions.size())
        return false;

    PrimitiveVector block_mass = density_manager_->mass_calculator().get_block_mass(blk_id);
    for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
        double mass = block_mass.get_dim_val(dimensions[dim_idx]);
        if (mass != 0. && boundary_confined_dims_[dim_idx])
            return true;
    }
    return false;
}

void NonlinearNesterovPlacer::update_boundary_net_flags_(const std::vector<PrimitiveVectorDim>& dimensions,
                                                         const PartialPlacement& seed) {
    boundary_cohesion_nets_.resize(ap_netlist_.nets().size(), false);
    std::fill(boundary_cohesion_nets_.begin(), boundary_cohesion_nets_.end(), false);
    io_chain_cohesion_nets_.resize(ap_netlist_.nets().size(), false);
    std::fill(io_chain_cohesion_nets_.begin(), io_chain_cohesion_nets_.end(), false);
    num_io_chain_cohesion_nets_ = 0;

    size_t boundary_blocks = 0;
    size_t io_chain_blocks = 0;
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        bool has_boundary_mass = block_has_boundary_mass_(blk_id, dimensions);
        if (has_boundary_mass)
            boundary_blocks++;
        if (has_boundary_mass && block_is_io_chain_block_(blk_id))
            io_chain_blocks++;
    }

    size_t boundary_nets = 0;
    size_t io_chain_nets = 0;
    for (APNetId net_id : ap_netlist_.nets()) {
        if (ap_netlist_.net_is_ignored(net_id))
            continue;
        if (ap_netlist_.net_pins(net_id).size() != 2)
            continue;

        bool has_boundary_endpoint = false;
        APBlockId first_blk_id = APBlockId::INVALID();
        APBlockId second_blk_id = APBlockId::INVALID();
        for (APPinId pin_id : ap_netlist_.net_pins(net_id)) {
            APBlockId blk_id = ap_netlist_.pin_block(pin_id);
            if (!first_blk_id.is_valid())
                first_blk_id = blk_id;
            else
                second_blk_id = blk_id;
            has_boundary_endpoint = has_boundary_endpoint || block_has_boundary_mass_(blk_id, dimensions);
        }
        if (!has_boundary_endpoint)
            continue;

        double seed_hpwl = std::abs(seed.block_x_locs[first_blk_id] - seed.block_x_locs[second_blk_id])
                           + std::abs(seed.block_y_locs[first_blk_id] - seed.block_y_locs[second_blk_id]);
        double device_span = std::max<double>(device_grid_width_, device_grid_height_);
        if (seed_hpwl < kBoundaryNetCohesionMinSeedHpwlFraction * device_span)
            continue;

        boundary_cohesion_nets_[net_id] = true;
        boundary_nets++;

        if (block_has_boundary_mass_(first_blk_id, dimensions)
            && block_has_boundary_mass_(second_blk_id, dimensions)
            && block_is_io_chain_block_(first_blk_id)
            && block_is_io_chain_block_(second_blk_id)) {
            io_chain_cohesion_nets_[net_id] = true;
            io_chain_nets++;
        }
    }
    num_io_chain_cohesion_nets_ = io_chain_nets;

    if (log_verbosity_ >= 1) {
        VTR_LOG("Nonlinear Nesterov boundary-net cohesion: %zu boundary-mass blocks, %zu long two-pin boundary-related nets, weight=%g, min_seed_hpwl_frac=%g.\n",
                boundary_blocks,
                boundary_nets,
                kBoundaryNetCohesionWeight,
                kBoundaryNetCohesionMinSeedHpwlFraction);
        VTR_LOG("Nonlinear Nesterov I/O-chain cohesion: %zu boundary I/O-chain blocks, %zu long direct I/O-chain nets, weight=%g.\n",
                io_chain_blocks,
                io_chain_nets,
                io_chain_net_cohesion_weight_);
    }
}

bool NonlinearNesterovPlacer::block_is_io_chain_block_(APBlockId blk_id) const {
    bool saw_atom = false;

    for (APPinId pin_id : ap_netlist_.block_pins(blk_id)) {
        AtomPinId atom_pin_id = ap_netlist_.pin_atom_pin(pin_id);
        if (!atom_pin_id.is_valid())
            continue;

        AtomBlockId atom_blk_id = atom_netlist_.pin_block(atom_pin_id);
        if (!atom_blk_id.is_valid())
            continue;

        saw_atom = true;
        LogicalModelId model_id = atom_netlist_.block_model(atom_blk_id);
        if (!model_name_is_io_chain(models_.model_name(model_id)))
            return false;
    }

    return saw_atom;
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

    if (pack_pattern_cohesion_weight_ > 0.) {
        for (const std::vector<APBlockId>& group : pack_pattern_cohesion_groups_) {
            if (group.empty())
                continue;
            double curvature = pack_pattern_cohesion_weight_ / static_cast<double>(group.size());
            for (APBlockId blk_id : group)
                block_precond_[blk_id] += curvature;
        }
    }

    // Density Hessian diagonal: the per-dimension density weight scales the block
    // mass it deposits into that resource's field. Heavier blocks under a
    // stronger density push have larger curvature and so take proportionally
    // smaller steps.
    const auto& mass_calculator = density_manager_->mass_calculator();
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        const PrimitiveVector& block_mass = mass_calculator.get_block_mass(blk_id);
        double density_curvature = 0.;
        for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
            double mass = block_mass.get_dim_val(dimensions[dim_idx]);
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

    filler_precond_.assign(dimensions.size(), kPreconditionFloor);
    for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
        double unit_mass = dim_idx < filler_unit_mass_.size() ? filler_unit_mass_[dim_idx] : 0.;
        double curvature = std::max(kPreconditionFloor, density_multipliers[dim_idx] * unit_mass);
        filler_precond_[dim_idx] = precond_alpha_active_ != 1.0 ? std::pow(curvature, precond_alpha_active_) : curvature;
    }
}

double NonlinearNesterovPlacer::compute_physical_overflow_ratio_(const PartialPlacement& p_placement,
                                                                 const std::vector<PrimitiveVectorDim>& dimensions) const {
    if (dimensions.empty())
        return 0.;

    size_t width = device_grid_width_;
    size_t height = device_grid_height_;
    size_t num_layers = std::max<size_t>(1, device_grid_num_layers_);
    size_t num_sites = width * height * num_layers;
    auto site_index = [width, height](size_t layer, size_t x, size_t y) {
        return (layer * height + y) * width + x;
    };

    // Bin only physical block mass (no fillers) at the floor tile. Nearest-tile
    // deposition concentrates mass relative to the bilinear density field, so this
    // slightly over-reads overflow -- a conservative (stop-later) bias for the stop.
    std::vector<std::vector<double>> utilization(dimensions.size(), std::vector<double>(num_sites, 0.));
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        PrimitiveVector block_mass = density_manager_->mass_calculator().get_block_mass(blk_id);
        if (block_mass.is_zero())
            continue;
        double x = std::clamp(p_placement.block_x_locs[blk_id], 0., device_grid_width_ - kDeviceBoundaryEpsilon);
        double y = std::clamp(p_placement.block_y_locs[blk_id], 0., device_grid_height_ - kDeviceBoundaryEpsilon);
        size_t layer = static_cast<size_t>(std::clamp(std::round(p_placement.block_layer_nums[blk_id]),
                                                      0.,
                                                      static_cast<double>(device_grid_num_layers_ - 1)));
        size_t idx = site_index(layer, static_cast<size_t>(std::floor(x)), static_cast<size_t>(std::floor(y)));
        for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
            double mass = block_mass.get_dim_val(dimensions[dim_idx]);
            if (mass != 0.)
                utilization[dim_idx][idx] += mass;
        }
    }

    const FlatPlacementBins& bins = density_manager_->flat_placement_bins();
    double total_overflow_mass = 0.;
    double total_capacity = 0.;
    for (size_t layer = 0; layer < num_layers; layer++) {
        for (size_t x = 0; x < width; x++) {
            for (size_t y = 0; y < height; y++) {
                FlatPlacementBinId bin_id = density_manager_->get_bin(x, y, layer);
                const vtr::Rect<double>& region = bins.bin_region(bin_id);
                double bin_area = std::max(1.0, region.width() * region.height());
                double target_density = density_manager_->get_bin_target_density(bin_id);
                size_t idx = site_index(layer, x, y);
                for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
                    double capacity = density_manager_->get_bin_capacity(bin_id).get_dim_val(dimensions[dim_idx])
                                      * target_density / bin_area;
                    total_capacity += capacity;
                    double overflow = utilization[dim_idx][idx] - capacity;
                    if (overflow > 0.)
                        total_overflow_mass += overflow;
                }
            }
        }
    }

    return total_capacity > kEpsilon ? total_overflow_mass / total_capacity : 0.;
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

void NonlinearNesterovPlacer::project_fillers_(FillerState& fillers) const {
    double max_x = std::max(0.0, static_cast<double>(device_grid_width_) - kDeviceBoundaryEpsilon);
    double max_y = std::max(0.0, static_cast<double>(device_grid_height_) - kDeviceBoundaryEpsilon);
    int max_layer = std::max(0, static_cast<int>(device_grid_num_layers_) - 1);
    for (std::vector<double>& dim_x : fillers.x) {
        for (double& x : dim_x)
            x = std::clamp(x, 0.0, max_x);
    }
    for (std::vector<double>& dim_y : fillers.y) {
        for (double& y : dim_y)
            y = std::clamp(y, 0.0, max_y);
    }
    for (std::vector<int>& dim_layer : fillers.layer) {
        for (int& layer : dim_layer)
            layer = std::clamp(layer, 0, max_layer);
    }
}

void NonlinearNesterovPlacer::gradient_step_(const PartialPlacement& y_placement,
                                             const PlacementGradient& grad,
                                             const FillerState& y_fillers,
                                             const FillerGradient& filler_grad,
                                             double step_size,
                                             PartialPlacement& next_placement,
                                             FillerState& next_fillers) const {
    next_placement.block_x_locs = y_placement.block_x_locs;
    next_placement.block_y_locs = y_placement.block_y_locs;
    next_placement.block_layer_nums = y_placement.block_layer_nums;
    next_placement.block_sub_tiles = y_placement.block_sub_tiles;
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
    // Projected-gradient box constraint, not legalization. ePlace/elfPlace keep
    // electrostatic density forces well behaved at the placement-region boundary
    // with Neumann boundary conditions, but the full AP objective also includes
    // wirelength/proximity terms and FISTA/backtracking trial points. Clamp here
    // so every evaluated candidate remains inside the physical device domain.
    project_placement_(next_placement);

    next_fillers.x.resize(y_fillers.x.size());
    next_fillers.y.resize(y_fillers.y.size());
    next_fillers.layer = y_fillers.layer;
    for (size_t dim_idx = 0; dim_idx < y_fillers.x.size(); dim_idx++) {
        VTR_ASSERT_SAFE(dim_idx < y_fillers.y.size());
        next_fillers.x[dim_idx].resize(y_fillers.x[dim_idx].size());
        next_fillers.y[dim_idx].resize(y_fillers.y[dim_idx].size());
        double inv_precond = (dim_idx < filler_precond_.size() && filler_precond_[dim_idx] > 0.)
                                 ? 1.0 / filler_precond_[dim_idx]
                                 : 1.0;
        for (size_t filler_idx = 0; filler_idx < y_fillers.x[dim_idx].size(); filler_idx++) {
            VTR_ASSERT_SAFE(filler_idx < y_fillers.y[dim_idx].size());
            double gx = (dim_idx < filler_grad.dx.size() && filler_idx < filler_grad.dx[dim_idx].size())
                            ? filler_grad.dx[dim_idx][filler_idx]
                            : 0.;
            double gy = (dim_idx < filler_grad.dy.size() && filler_idx < filler_grad.dy[dim_idx].size())
                            ? filler_grad.dy[dim_idx][filler_idx]
                            : 0.;
            next_fillers.x[dim_idx][filler_idx] = y_fillers.x[dim_idx][filler_idx] - step_size * gx * inv_precond;
            next_fillers.y[dim_idx][filler_idx] = y_fillers.y[dim_idx][filler_idx] - step_size * gy * inv_precond;
        }
    }
    project_fillers_(next_fillers);
}

void NonlinearNesterovPlacer::extrapolate_(const PartialPlacement& current,
                                           const PartialPlacement& next,
                                           const FillerState& current_fillers,
                                           const FillerState& next_fillers,
                                           double beta,
                                           PartialPlacement& y_placement,
                                           FillerState& y_fillers) const {
    y_placement.block_x_locs = next.block_x_locs;
    y_placement.block_y_locs = next.block_y_locs;
    y_placement.block_layer_nums = next.block_layer_nums;
    y_placement.block_sub_tiles = next.block_sub_tiles;
    for (APBlockId blk_id : optimizable_blocks_) {
        y_placement.block_x_locs[blk_id] = next.block_x_locs[blk_id] + beta * (next.block_x_locs[blk_id] - current.block_x_locs[blk_id]);
        y_placement.block_y_locs[blk_id] = next.block_y_locs[blk_id] + beta * (next.block_y_locs[blk_id] - current.block_y_locs[blk_id]);
    }
    // FISTA extrapolation can overshoot the device rectangle; keep the look-ahead
    // point in the same box-constrained smooth domain before evaluating gradients.
    project_placement_(y_placement);

    y_fillers.x.resize(next_fillers.x.size());
    y_fillers.y.resize(next_fillers.y.size());
    y_fillers.layer = next_fillers.layer;
    for (size_t dim_idx = 0; dim_idx < next_fillers.x.size(); dim_idx++) {
        VTR_ASSERT_SAFE(dim_idx < next_fillers.y.size());
        VTR_ASSERT_SAFE(dim_idx < current_fillers.x.size());
        VTR_ASSERT_SAFE(dim_idx < current_fillers.y.size());
        y_fillers.x[dim_idx].resize(next_fillers.x[dim_idx].size());
        y_fillers.y[dim_idx].resize(next_fillers.y[dim_idx].size());
        for (size_t filler_idx = 0; filler_idx < next_fillers.x[dim_idx].size(); filler_idx++) {
            VTR_ASSERT_SAFE(filler_idx < next_fillers.y[dim_idx].size());
            VTR_ASSERT_SAFE(filler_idx < current_fillers.x[dim_idx].size());
            VTR_ASSERT_SAFE(filler_idx < current_fillers.y[dim_idx].size());
            y_fillers.x[dim_idx][filler_idx] = next_fillers.x[dim_idx][filler_idx]
                                               + beta * (next_fillers.x[dim_idx][filler_idx] - current_fillers.x[dim_idx][filler_idx]);
            y_fillers.y[dim_idx][filler_idx] = next_fillers.y[dim_idx][filler_idx]
                                               + beta * (next_fillers.y[dim_idx][filler_idx] - current_fillers.y[dim_idx][filler_idx]);
        }
    }
    project_fillers_(y_fillers);
}

double NonlinearNesterovPlacer::gradient_norm_squared_(const PlacementGradient& grad) const {
    double norm_squared = 0.;
    for (APBlockId blk_id : optimizable_blocks_) {
        norm_squared += grad.dx[blk_id] * grad.dx[blk_id];
        norm_squared += grad.dy[blk_id] * grad.dy[blk_id];
    }
    return norm_squared;
}

double NonlinearNesterovPlacer::filler_gradient_norm_squared_(const FillerGradient& grad) const {
    double norm_squared = 0.;
    for (const std::vector<double>& dim_dx : grad.dx) {
        for (double dx : dim_dx)
            norm_squared += dx * dx;
    }
    for (const std::vector<double>& dim_dy : grad.dy) {
        for (double dy : dim_dy)
            norm_squared += dy * dy;
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

double NonlinearNesterovPlacer::max_filler_displacement_(const FillerState& from,
                                                         const FillerState& to) const {
    double max_displacement = 0.;
    for (size_t dim_idx = 0; dim_idx < from.x.size() && dim_idx < to.x.size(); dim_idx++) {
        for (size_t filler_idx = 0; filler_idx < from.x[dim_idx].size() && filler_idx < to.x[dim_idx].size(); filler_idx++) {
            double dx = to.x[dim_idx][filler_idx] - from.x[dim_idx][filler_idx];
            double dy = to.y[dim_idx][filler_idx] - from.y[dim_idx][filler_idx];
            max_displacement = std::max(max_displacement, std::hypot(dx, dy));
        }
    }
    return max_displacement;
}
