/**
 * @file
 * @author  William Zhang
 * @date    June 2026
 * @brief   Implementation of a nonlinear Nesterov analytical global placer.
 */

#include "nonlinear_nesterov_placer.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <limits>
#include <optional>
#include <random>
#include <string>
#include <vector>
#include "PreClusterTimingManager.h"
#include "analytical_solver.h"
#include "ap_flow_enums.h"
#include "ap_netlist.h"
#include "atom_netlist.h"
#include "device_grid.h"
#include "electrostatic_density_utils.h"
#include "flat_placement_bins.h"
#include "globals.h"
#include "logic_types.h"
#include "partial_placement.h"
#include "physical_types.h"
#include "place_delay_model.h"
#include "prepack.h"
#include "primitive_dim_manager.h"
#include "primitive_vector.h"
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
 *
 * Each epoch ends with one partial legalization and one timing-criticality net
 * weight refresh (@ref update_timing_net_weights_), so this also sets how often
 * the smooth optimizer's objective sees fresh legality/timing feedback. The
 * baseline SimPL/B2B placer refreshes both every one of its (up to 100) outer
 * iterations; 2 epochs left this placer refreshing only once across an entire
 * run, with a single large jump in density weight and wirelength smoothing
 * between the two. Raised to close that gap while keeping the total inner
 * iteration budget (@ref kMaxNesterovIterations) fixed, so each epoch just gets
 * a shorter, more frequent slice of it.
 */
constexpr size_t kNesterovEpochs = 5;

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
 * designs. Larger designs keep the full anchor.
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
 * of size. It is the validated remedy for large-device over-spread, so it is
 * enabled for designs at or above @ref kProximitySizeThreshold movable blocks.
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
 * Kept neutral by default; direct I/O chains use the narrower weights below.
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
 * Some designs' failure mode is not generic boundary spreading; it is specific
 * pad/obuf/OCT/termination chains being split before APPack can form compact
 * I/O clusters. Prior broad boundary-net weighting regressed guard circuits, so
 * this stronger weight is applied only to two-pin nets whose endpoints are both
 * I/O-chain primitives on boundary-confined resources.
 */
constexpr double kIoChainNetCohesionWeight = 2.0;

/**
 * @brief Long-chain pack-pattern affinity-spring weight for I/O-chain designs.
 *
 * Probing showed that ungated pack springs worsen general QoR, while gating the
 * 0.02 weight to designs with long direct I/O-chain nets is required to recover
 * the win on the designs that motivated it.
 */
constexpr double kPackPatternCohesionWeight = 0.02;

/**
 * @brief Smooth-WL multiplier for direct output-driver↔outpad pair nets (always on).
 *
 * Generic I/O-chain cohesion only flags nets that are already long in the
 * warm-start seed, so most pad-drive pairs never get that boost. This stronger
 * multiplier applies to every detected 2-pin output-driver↔outpad AP net so GP
 * keeps those endpoints local, eliminating cross-edge pack/place splits.
 */
constexpr double kIoPairNetWeight = 8.0;

/**
 * @brief Quadratic attraction weight for direct output-driver↔outpad pairs (always on).
 *
 * Soft spring only -- no post-epoch snap.
 */
constexpr double kIoPairAttractionWeight = 8.0;

/**
 * @brief Maximum AP-HPWL regression admitted by the CPD tie-break in checkpoint
 *        selection (see the checkpoint-selection block in @ref optimize_from_seed_).
 *
 * Kept narrow so estimated CPD only breaks near-ties in HPWL.
 */
constexpr double kCheckpointHpwlGuard = 0.01;

/**
 * @brief Minimum B2B solve+legalize cycles used to build the warm-start seed.
 *
 * The nonlinear Nesterov placer seeds itself from a wirelength-aware B2B/QP solve
 * (a short SimPL run) rather than a block-ID grid spread. The warm start runs a
 * *convergence-based* number of cycles (see @ref kWarmStartMaxIters /
 * @ref kWarmStartTol): it iterates until the seed HPWL stops improving, so large /
 * under-converged designs get enough cycles to produce a tight, clusterable
 * placement -- the post-APPack BB inflation that drove the wirelength gap on
 * those designs -- while small designs that converge quickly stop early. This is
 * the floor.
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
 * Sparse seeds need only a cheap filler-free wirelength-refinement epoch because
 * their density field has little remaining work.
 */
constexpr size_t kSparseSeedMaxEpochs = 1;

/**
 * @brief Inner-iteration cap for the sparse-seed probe epoch.
 *
 * The probe refines wirelength near an already density-feasible seed, so it does
 * not need the full inner budget; each iteration still pays the per-resource
 * Poisson solves, which dominate sparse-seed epoch cost once fillers are gone.
 *
 * The probe is also a checkpoint-selection candidate.
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
 * This repaired CPD misses on medium-large designs and removed a wirelength loss
 * on others, while high-pin-count small/medium designs regressed under the same
 * timing pressure. The block-count window captures that middle regime and leaves
 * tiny, high-pin, and huge sparse tails on the normal tradeoff.
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
 *
 */
constexpr double kGammaStartFraction = 0.04;
constexpr double kGammaEndFraction = 0.008;

/**
 * @brief Floor on the weighted-average smoothing width, in tiles.
 *
 * This keeps the exponentials numerically useful and retains smoothing on
 * short nets and narrow devices.
 */
constexpr double kMinWirelengthGamma = 1.0;

/**
 * @brief Initial target ratio of density pressure to wirelength pressure.
 *
 * A small fixed linear density weight keeps the first Nesterov pass close to the
 * warm-start seed while still letting the electrostatic field relieve overlap.
 */
constexpr double kInitialDensityToWirelengthRatio = 0.05;

/**
 * @brief Final density-weight multiplier for the simple continuation schedule.
 *
 * The partial and full legalizers handle the remaining discrete overlap, so the
 * smooth penalty is kept moderate to avoid unnecessary wirelength growth.
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
 * @brief Max per-dimension adaptive density boost relative to the schedule weight.
 *
 * Used when adaptive density is enabled: scarce overflowing dimensions may be
 * strengthened up to this factor without globally inflating all density weights.
 */
constexpr double kMaxAdaptiveDensityBoost = 4.0;

/**
 * @brief True if a density dimension should receive adaptive overflow boosts.
 *
 * Abundant logic dimensions stay on the uniform schedule; scarce hard-block
 * and I/O dimensions are the ones telemetry showed staying overfilled.
 */
bool dim_allows_adaptive_density_boost(const std::string& dim_name) {
    return dim_name != ".names" && dim_name != ".latch";
}

/**
 * @brief Pin count, as a multiple of the design's average pins-per-block,
 *        above which a block's density-term mass starts being inflated.
 *
 * Standard ePlace/RePlAce-style cell inflation for routability: a block with
 * more pins than its share of the design's average needs more room around it
 * for the extra wires, so its mass in the smooth density term (not its real
 * legalized footprint) is scaled up, pushing the spreader to leave it more
 * whitespace. Blocks at or below the reference keep their true mass.
 */
constexpr double kPinDensityInflationPinsPerBlockRatio = 1.0;

/**
 * @brief Maximum per-block density-term mass inflation factor from pin count.
 */
constexpr double kMaxPinDensityInflation = 2.0;

/**
 * @brief Small value used to avoid division by zero.
 */
constexpr double kEpsilon = 1e-9;

/**
 * @brief Device-bound epsilon to keep floor-based bin lookup inside the grid.
 */
constexpr double kDeviceBoundaryEpsilon = kDensityDeviceBoundaryEpsilon;

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
    , pack_pattern_cohesion_weight_(kPackPatternCohesionWeight)
    , io_pair_net_weight_(kIoPairNetWeight)
    , io_pair_attraction_weight_(kIoPairAttractionWeight) {
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

    affinity_groups_.clear();
    num_io_pair_affinity_groups_ = 0;
    num_pack_pattern_affinity_groups_ = 0;
    initialize_pack_pattern_affinity_groups_(prepacker);
    initialize_io_pair_affinity_groups_();

    partial_legalizer_ = make_partial_legalizer(partial_legalizer_type,
                                                ap_netlist_,
                                                density_manager_,
                                                prepacker,
                                                models,
                                                log_verbosity_);

    moveable_blocks_.reserve(ap_netlist_.blocks().size());
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        if (block_is_moveable_(blk_id))
            moveable_blocks_.push_back(blk_id);
    }

    size_t moveable_pins = 0;
    for (APBlockId blk_id : moveable_blocks_)
        moveable_pins += ap_netlist_.block_pins(blk_id).size();
    double pins_per_moveable_block = moveable_blocks_.empty()
                                         ? 0.
                                         : static_cast<double>(moveable_pins) / moveable_blocks_.size();

    pin_density_inflation_.resize(ap_netlist_.blocks().size(), 1.0f);
    double pin_density_inflation_reference = pins_per_moveable_block * kPinDensityInflationPinsPerBlockRatio;
    double max_pin_density_inflation = 1.0;
    if (pin_density_inflation_reference > 0.) {
        for (APBlockId blk_id : moveable_blocks_) {
            double pin_ratio = static_cast<double>(ap_netlist_.block_pins(blk_id).size()) / pin_density_inflation_reference;
            float inflation = static_cast<float>(std::clamp(pin_ratio, 1.0, kMaxPinDensityInflation));
            pin_density_inflation_[blk_id] = inflation;
            max_pin_density_inflation = std::max(max_pin_density_inflation, static_cast<double>(inflation));
        }
    }

    effective_timing_tradeoff_ = ap_timing_tradeoff_;
    if (ap_timing_tradeoff_ > 0.f
        && moveable_blocks_.size() >= kAdaptiveTimingMinBlocks
        && moveable_blocks_.size() <= kAdaptiveTimingMaxBlocks) {
        effective_timing_tradeoff_ = std::max(ap_timing_tradeoff_, static_cast<float>(kAdaptiveTimingTradeoff));
    }

    bool high_pin_seed = moveable_blocks_.size() >= kHighPinWarmStartBlockThreshold
                         && pins_per_moveable_block >= kHighPinWarmStartPinsPerBlock;
    bool huge_seed = moveable_blocks_.size() >= kHugeWarmStartBlockThreshold;
    if (high_pin_seed || huge_seed)
        warmstart_iters_ = std::max(kWarmStartIters, kAdaptiveWarmStartIters);
    else
        warmstart_iters_ = kWarmStartIters;
    warmstart_max_iters_ = std::max(kWarmStartMaxIters, warmstart_iters_);

    if (log_verbosity_ >= 1) {
        size_t affinity_blocks = 0;
        for (const AffinityGroup& group : affinity_groups_)
            affinity_blocks += group.blocks.size();
        VTR_LOG("Nonlinear Nesterov adaptive policy: blocks=%zu pins/block=%.2f warm-start-floor=%zu timing=%g io_chain_cohesion=%g.\n",
                moveable_blocks_.size(),
                pins_per_moveable_block,
                warmstart_iters_,
                effective_timing_tradeoff_,
                io_chain_net_cohesion_weight_);
        VTR_LOG("Nonlinear Nesterov affinity springs: io_pairs=%zu weight=%g; pack_groups=%zu weight=%g; blocks=%zu.\n",
                num_io_pair_affinity_groups_,
                io_pair_attraction_weight_,
                num_pack_pattern_affinity_groups_,
                pack_pattern_cohesion_weight_,
                affinity_blocks);
        VTR_LOG("Nonlinear Nesterov pin-density inflation: reference=%.2f pins/block max_inflation=%.3g.\n",
                pin_density_inflation_reference,
                max_pin_density_inflation);
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

bool NonlinearNesterovPlacer::block_is_moveable_(APBlockId blk_id) const {
    return ap_netlist_.block_mobility(blk_id) == APBlockMobility::MOVEABLE;
}

PartialPlacement NonlinearNesterovPlacer::initialize_placement_() {
    PartialPlacement p_placement(ap_netlist_);

    // No movable blocks: every block is fixed, so there is nothing for the
    // optimizer to do. Snap fixed blocks into device bounds and return.
    if (moveable_blocks_.empty()) {
        project_placement_(p_placement);
        return p_placement;
    }

    // Warm start from a B2B/QP analytical solve. Iterate solve+legalize until the
    // seed HPWL stops improving (convergence-based), so large/under-converged
    // designs run enough cycles to produce a tight, clusterable seed -- the post-
    // APPack clustering inflation that drove the wirelength gap on large designs --
    // while small designs that converge fast stop at the floor. The legalizer
    // places every block (including solver-disconnected ones), so all moveable
    // blocks have a valid location afterward.
    double previous_hpwl = std::numeric_limits<double>::infinity();
    size_t solver_iteration = 0;
    size_t min_cycles = warmstart_iters_;
    size_t max_cycles = warmstart_max_iters_;
    bool sparse_checked = false;
    bool reached_sparse_deepening = false;
    bool stopped_by_convergence = false;
    double sparse_seed_overflow = 0.;
    warmstart_seed_overflow_ = 0.;

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
        // electrostatic field has nothing to spread, so the Nesterov epoch loop no-ops and
        // the placement is left at this loose seed -- APPack then cannot pack distant
        // molecules into shared logic blocks, inflating routed wirelength (measured up
        // to +21% routed WL on sparse designs). The cure is to keep compacting with
        // more B2B solve+legalize cycles (what SimPL does
        // implicitly), which the HPWL-plateau convergence stops too early. Dense
        // designs, where the field does real work, keep the short warm start so the
        // electrostatic stage is not handed an over-compacted seed.
        if (!sparse_checked && kSparseGateOverflow > 0. && solver_iteration < kSparseWarmStartIters) {
            project_placement_(p_placement);
            std::vector<PrimitiveVectorDim> dims = density_manager_->get_used_dims_mask().get_non_zero_dims();
            sparse_seed_overflow = compute_physical_overflow_ratio_(p_placement, dims);
            warmstart_seed_overflow_ = sparse_seed_overflow;
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
    precond_active_ = moveable_blocks_.size() >= kProximitySizeThreshold;
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
    if (pack_pattern_cohesion_weight_ > 0.
        && num_io_chain_cohesion_nets_ == 0) {
        if (log_verbosity_ >= 1) {
            VTR_LOG("Nonlinear Nesterov pack-pattern affinity disabled: no long direct I/O-chain nets were found.\n");
        }
        pack_pattern_cohesion_weight_ = 0.;
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
    size_t iterations_per_epoch = sparse_seed_
                                      ? kSparseSeedProbeIterations
                                      : (kMaxNesterovIterations + num_epochs - 1) / num_epochs;
    const size_t min_epochs_before_overflow_stop = num_epochs == kNesterovEpochs
                                                       ? kMinEpochsBeforeOverflowStop
                                                       : num_epochs;
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
                                                        std::nullopt);
        initial_density_weight = 1e-3;
        if (components.density > kEpsilon) {
            initial_density_weight = kInitialDensityToWirelengthRatio * std::max(components.wirelength, 1.0) / components.density;
        }
        initial_density_weight = std::clamp(initial_density_weight, 1e-5, 1e3);
        for (size_t dim_idx = 0; dim_idx < density_dimensions.size(); dim_idx++)
            density_multipliers[dim_idx] = initial_density_weight;
    };
    reset_density_weights(current);

    // Count of FISTA adaptive restarts, reported at the end of the run.
    size_t num_objective_restarts = 0;

    // Adaptive per-resource density multipliers: scarce dimensions still overfilled
    // relative to the seed's own physical overflow get a boosted density weight, on
    // top of the schedule's uniform ramp, so they don't lag the abundant dimensions
    // in reaching legality.
    const PrimitiveDimManager& dim_manager = density_manager_->mass_calculator().get_dim_manager();
    std::vector<double> adaptive_density_boosts(density_dimensions.size(), 1.);
    std::vector<double> seed_phys_oflows = compute_physical_overflow_ratios_per_dim_(current, density_dimensions);
    for (size_t dim_idx = 0; dim_idx < density_dimensions.size(); dim_idx++) {
        const std::string& dim_name = dim_manager.get_dim_name(density_dimensions[dim_idx]);
        if (!dim_allows_adaptive_density_boost(dim_name))
            continue;
        if (seed_phys_oflows[dim_idx] > kTargetOverflow) {
            double ratio = seed_phys_oflows[dim_idx] / kTargetOverflow;
            adaptive_density_boosts[dim_idx] = std::clamp(ratio, 1.0, kMaxAdaptiveDensityBoost);
        }
    }

    if (log_verbosity_ >= 1) {
        VTR_LOG("Epoch  Pre HPWL  Post HPWL  Pre Oflow  Post Oflow  Pre Max  Post Max  Mean Move  Max Move  Density Wt  Prox Wt\n");
        VTR_LOG("-----  --------  ---------  ---------  ----------  -------  --------  ---------  --------  ----------  -------\n");
    }

    double legalizer_feedback_proximity_weight = 0.;
    const double checkpoint_hpwl_guard = kCheckpointHpwlGuard;
    std::vector<PartialPlacement> checkpoints;
    std::vector<double> checkpoint_hpwls;
    std::vector<double> checkpoint_cpds_ns;
    std::vector<int> checkpoint_sources;
    checkpoints.push_back(seed);
    checkpoint_hpwls.push_back(seed.get_hpwl(ap_netlist_));
    {
        vtr::Timer timing_update_timer;
        checkpoint_cpds_ns.push_back(evaluate_checkpoint_cpd_(seed));
        timing_update_time_sec += timing_update_timer.elapsed_sec();
    }
    checkpoint_sources.push_back(-1); // -1 = warm-start seed, otherwise epoch index.
    PartialPlacement legal_anchor(ap_netlist_);
    PartialPlacement y_placement(ap_netlist_);
    PartialPlacement next(ap_netlist_);
    PartialPlacement before_legalization(ap_netlist_);
    FillerState y_fillers;
    FillerState next_fillers;
    auto apply_continuation_schedule = [&](double schedule) {
        current_gamma_fraction_ = kGammaStartFraction
                                  * std::pow(kGammaEndFraction / kGammaStartFraction, schedule);
        double density_weight_scale = std::pow(kFinalDensityWeightMultiplier, schedule);
        for (double& multiplier : density_multipliers)
            multiplier = initial_density_weight * density_weight_scale;
    };

    // Short continuation loop. Density weights are fixed within an epoch and
    // follow a simple geometric ramp between epochs; timing net weights and the
    // preconditioner are refreshed against the current placement at the start of
    // each one.
    //
    for (size_t epoch = 0; epoch < num_epochs; epoch++) {
        // Anneal the wirelength smoothing fraction geometrically from coarse
        // (smooth global gradient) to sharp (near true HPWL) across epochs, so
        // early epochs spread on an easy landscape and later epochs recover real
        // wirelength. The sparse-seed probe refines wirelength near an already
        // spread seed, so it goes straight to the sharp end of the schedule.
        if (sparse_seed_)
            current_gamma_fraction_ = kGammaEndFraction;
        // Checkpoint timing is evaluated for the seed and after every partial
        // legalization, so the timing manager already describes `current` here.
        update_timing_net_weights_();
        if (sparse_seed_) {
            for (double& multiplier : density_multipliers)
                multiplier = initial_density_weight;
        } else {
            double schedule = num_epochs > 1
                                  ? static_cast<double>(epoch) / static_cast<double>(num_epochs - 1)
                                  : 0.;
            apply_continuation_schedule(schedule);
        }
        for (size_t dim_idx = 0; dim_idx < density_multipliers.size(); dim_idx++)
            density_multipliers[dim_idx] *= adaptive_density_boosts[dim_idx];
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
        double proximity_scale = moveable_blocks_.size() < kProximitySizeThreshold ? kProximityScale : 1.0;
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
                                                         std::nullopt);

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
                                                       std::ref(filler_grad));
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
                                               std::nullopt);
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

            // Adaptive restart drops momentum after an uphill accelerated step.
            // The accepted point is retained so the short epoch keeps progressing.
            bool objective_restart = next_obj.total > current_obj.total;
            if (objective_restart)
                num_objective_restarts++;

            if (objective_restart) {
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
                                                              std::nullopt);
        // Partially legalize the smooth result. This both cleans up overlap and
        // produces the anchor that the next epoch's proximity term pulls toward;
        // the per-epoch displacement it causes also drives the proximity weight.
        before_legalization.block_x_locs = current.block_x_locs;
        before_legalization.block_y_locs = current.block_y_locs;
        before_legalization.block_layer_nums = current.block_layer_nums;
        before_legalization.block_sub_tiles = current.block_sub_tiles;
        // Diagnostic only, and it must run here: it reads the potential the last
        // objective evaluation left in the workspace, which still describes the
        // smooth (pre-legalization) result at this point and will be overwritten
        // by the post-legalization evaluation below.
        if (log_verbosity_ >= 3)
            report_density_force_leak_(before_legalization, density_dimensions, density_multipliers, epoch);

        // Gradient audit on the first epoch only: it costs two full objective
        // evaluations per probe, and the point is to check the derivative, which
        // does not change from epoch to epoch.
        if (log_verbosity_ >= 4 && epoch == 0) {
            audit_gradient_(before_legalization, density_dimensions, density_multipliers,
                            std::cref(legal_anchor), proximity_weight, current_fillers);
        }

        double pre_leg_overflow = compute_physical_overflow_ratio_(before_legalization, density_dimensions);
        density_manager_->import_placement_into_bins(before_legalization);
        size_t pre_leg_overfilled_bins = density_manager_->get_overfilled_bins().size();
        vtr::Timer legalizer_timer;
        partial_legalizer_->legalize(current);
        legalizer_time_sec += legalizer_timer.elapsed_sec();
        ObjectiveValue post_legalization = evaluate_objective_(current,
                                                               density_multipliers,
                                                               std::cref(legal_anchor),
                                                               proximity_weight,
                                                               std::nullopt,
                                                               current_fillers,
                                                               std::nullopt);

        double total_displacement = 0.;
        double max_displacement = 0.;
        for (APBlockId blk_id : moveable_blocks_) {
            double dx = current.block_x_locs[blk_id] - before_legalization.block_x_locs[blk_id];
            double dy = current.block_y_locs[blk_id] - before_legalization.block_y_locs[blk_id];
            double displacement = std::hypot(dx, dy);
            total_displacement += displacement;
            max_displacement = std::max(max_displacement, displacement);
        }
        double mean_displacement = moveable_blocks_.empty() ? 0. : total_displacement / moveable_blocks_.size();
        legalizer_feedback_proximity_weight = std::min(kMaxLegalizerFeedbackProximityWeight,
                                                       std::max(kLegalizerFeedbackRetention * legalizer_feedback_proximity_weight,
                                                                kProximityWeightPerLegalizationTile * mean_displacement));

        double post_legalization_hpwl = current.get_hpwl(ap_netlist_);
        vtr::Timer timing_update_timer;
        double post_legalization_cpd_ns = evaluate_checkpoint_cpd_(current);
        timing_update_time_sec += timing_update_timer.elapsed_sec();
        checkpoints.push_back(current);
        checkpoint_hpwls.push_back(post_legalization_hpwl);
        checkpoint_cpds_ns.push_back(post_legalization_cpd_ns);
        checkpoint_sources.push_back(static_cast<int>(epoch));
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

        std::vector<double> phys_oflows = compute_physical_overflow_ratios_per_dim_(before_legalization, density_dimensions);
        VTR_LOG("  Nesterov density dims (epoch %zu): pre_overfilled_bins=%zu mean_pl_disp=%.4f pre_leg_overflow=%.4f\n",
                epoch, pre_leg_overfilled_bins, mean_displacement, pre_leg_overflow);
        for (size_t dim_idx = 0; dim_idx < density_dimensions.size(); dim_idx++) {
            const std::string& dim_name = dim_manager.get_dim_name(density_dimensions[dim_idx]);
            double dens_energy = dim_idx < pre_legalization.density_energies.size()
                                     ? pre_legalization.density_energies[dim_idx]
                                     : 0.;
            double oflow_ratio = dim_idx < pre_legalization.dim_overflow_ratios.size()
                                     ? pre_legalization.dim_overflow_ratios[dim_idx]
                                     : 0.;
            double oflow_mass = dim_idx < pre_legalization.dim_overflow_mass.size()
                                    ? pre_legalization.dim_overflow_mass[dim_idx]
                                    : 0.;
            double max_oflow = dim_idx < pre_legalization.dim_max_overflow.size()
                                   ? pre_legalization.dim_max_overflow[dim_idx]
                                   : 0.;
            double phys_oflow = dim_idx < phys_oflows.size() ? phys_oflows[dim_idx] : 0.;
            double multiplier = density_multipliers[dim_idx];
            VTR_LOG("    dim=%-20s mult=%.4g densE=%.4g oflow=%.4f mass=%.4g max=%.4f phys=%.4f boost=%.3g\n",
                    dim_name.c_str(),
                    multiplier,
                    dens_energy,
                    oflow_ratio,
                    oflow_mass,
                    max_oflow,
                    phys_oflow,
                    adaptive_density_boosts[dim_idx]);
        }
        for (size_t dim_idx = 0; dim_idx < density_dimensions.size(); dim_idx++) {
            const std::string& dim_name = dim_manager.get_dim_name(density_dimensions[dim_idx]);
            if (!dim_allows_adaptive_density_boost(dim_name))
                continue;
            double phys_oflow = dim_idx < phys_oflows.size() ? phys_oflows[dim_idx] : 0.;
            if (phys_oflow > kTargetOverflow) {
                double ratio = phys_oflow / kTargetOverflow;
                double boost = std::clamp(ratio, 1.0, kMaxAdaptiveDensityBoost);
                adaptive_density_boosts[dim_idx] = std::max(adaptive_density_boosts[dim_idx], boost);
            }
        }

        // #2 overflow-target stop: once the smooth (pre-legalization) placement is
        // already spread enough that physical mass barely exceeds tile capacity,
        // further density tightening only costs wirelength, so skip the remaining
        // intermediate epochs. Jump straight to the final epoch (schedule=1: the
        // sharp gamma / full density weight the continuation schedule was built to
        // reach) instead of just breaking, so an early stop still lands the smooth
        // optimizer at the continuation endpoint rather than stranding it mid-ramp.
        //
        if (kTargetOverflow > 0.
            && epoch + 1 >= min_epochs_before_overflow_stop
            && epoch + 1 < num_epochs) {
            if (pre_leg_overflow <= kTargetOverflow) {
                if (log_verbosity_ >= 1) {
                    VTR_LOG("Nonlinear Nesterov: physical overflow %.4f <= target %.4f after epoch %zu; skipping to final continuation step.\n",
                            pre_leg_overflow, kTargetOverflow, epoch);
                }
                epoch = num_epochs - 2; // Loop increment advances this to num_epochs - 1, the final epoch.
                continue;
            }
        }
    }

    // Checkpoint selection is min-HPWL, with a tight-window CPD tie-break: among
    // checkpoints within kCheckpointHpwlGuard of the best HPWL, prefer the one
    // with the lower estimated CPD. The tie-break only applies when timing is
    // enabled; a zero timing tradeoff keeps pure minimum-HPWL selection.
    size_t best_checkpoint_idx = std::distance(checkpoint_hpwls.begin(),
                                               std::min_element(checkpoint_hpwls.begin(), checkpoint_hpwls.end()));
    if (effective_timing_tradeoff_ != 0.f) {
        double minimum_checkpoint_hpwl = checkpoint_hpwls[best_checkpoint_idx];
        double hpwl_limit = minimum_checkpoint_hpwl * (1. + checkpoint_hpwl_guard);
        double best_cpd_ns = std::numeric_limits<double>::infinity();
        for (size_t checkpoint_idx = 0; checkpoint_idx < checkpoints.size(); checkpoint_idx++) {
            if (checkpoint_hpwls[checkpoint_idx] <= hpwl_limit
                && std::isfinite(checkpoint_cpds_ns[checkpoint_idx])
                && checkpoint_cpds_ns[checkpoint_idx] < best_cpd_ns) {
                best_checkpoint_idx = checkpoint_idx;
                best_cpd_ns = checkpoint_cpds_ns[checkpoint_idx];
            }
        }
        VTR_LOG("Nonlinear Nesterov: selecting checkpoint within %.1f%% of min-HPWL with lowest estimated CPD.\n",
                100. * checkpoint_hpwl_guard);
    } else {
        VTR_LOG("Nonlinear Nesterov: selecting min-HPWL checkpoint (timing tradeoff is 0).\n");
    }

    for (size_t checkpoint_idx = 0; checkpoint_idx < checkpoints.size(); checkpoint_idx++) {
        const char* source_name = checkpoint_sources[checkpoint_idx] < 0 ? "seed" : "epoch";
        VTR_LOG("Nonlinear Nesterov checkpoint: source=%s index=%d hpwl=%g estimated_cpd_ns=%g selected=%s.\n",
                source_name,
                checkpoint_sources[checkpoint_idx],
                checkpoint_hpwls[checkpoint_idx],
                checkpoint_cpds_ns[checkpoint_idx],
                checkpoint_idx == best_checkpoint_idx ? "yes" : "no");
    }

    const double best_hpwl = checkpoint_hpwls[best_checkpoint_idx];
    const int best_source = checkpoint_sources[best_checkpoint_idx];

    VTR_LOG("Nonlinear Nesterov Global Placer Statistics:\n");
    VTR_LOG("\tFinal-epoch placement HPWL after partial legalization: %g\n", current.get_hpwl(ap_netlist_));
    if (best_source < 0)
        VTR_LOG("\tSelected placement HPWL: %g (warm-start seed)\n", best_hpwl);
    else
        VTR_LOG("\tSelected placement HPWL: %g (epoch %d)\n", best_hpwl, best_source);
    VTR_LOG("\tFinal first density weight: %g\n", density_multipliers.empty() ? 0. : density_multipliers.front());
    VTR_LOG("\tAdaptive restarts: %zu\n", num_objective_restarts);
    if (log_verbosity_ >= 1) {
        VTR_LOG("Nonlinear Nesterov phase time: epoch loop took %.2f seconds (partial legalization %.2f, timing updates %.2f).\n",
                epoch_phase_timer.elapsed_sec(), legalizer_time_sec, timing_update_time_sec);
    }
    partial_legalizer_->print_statistics();

    return checkpoints[best_checkpoint_idx];
}

NonlinearNesterovPlacer::ObjectiveValue NonlinearNesterovPlacer::evaluate_objective_(const PartialPlacement& p_placement,
                                                                                     const std::vector<double>& density_multipliers,
                                                                                     std::optional<std::reference_wrapper<const PartialPlacement>> legal_anchor,
                                                                                     double proximity_weight,
                                                                                     std::optional<std::reference_wrapper<PlacementGradient>> grad,
                                                                                     const FillerState& fillers,
                                                                                     std::optional<std::reference_wrapper<FillerGradient>> filler_grad) const {
    if (grad)
        grad->get().clear();

    ObjectiveValue value;
    value.wirelength = add_wirelength_gradient_(p_placement, grad);
    value.density = add_density_gradient_(p_placement,
                                          density_multipliers,
                                          value.density_energies,
                                          value.dim_overflow_ratios,
                                          value.dim_overflow_mass,
                                          value.dim_max_overflow,
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
    value.affinity_spring = add_affinity_spring_gradient_(p_placement, grad);
    value.total = value.wirelength
                  + value.affinity_spring
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
    double gamma = std::max(kMinWirelengthGamma,
                            std::max<double>(device_grid_width_, device_grid_height_) * current_gamma_fraction_);
    double smooth_wirelength = 0.; // "smooth" == differentiable here: the WA surrogate above.

    // Reuse pin-coordinate and WA-weight buffers across nets. The net traversal
    // and arithmetic order are unchanged; only per-net heap churn is removed.
    std::vector<double> x_locs;
    std::vector<double> y_locs;
    std::vector<double> positive_x_weights;
    std::vector<double> negative_x_weights;
    std::vector<double> positive_y_weights;
    std::vector<double> negative_y_weights;

    for (APNetId net_id : ap_netlist_.nets()) {
        if (ap_netlist_.net_is_ignored(net_id))
            continue;

        size_t num_pins = ap_netlist_.net_pins(net_id).size();
        if (num_pins < 2)
            continue;

        double net_weight = net_weights_[net_id];
        x_locs.clear();
        y_locs.clear();
        if (x_locs.capacity() < num_pins) {
            x_locs.reserve(num_pins);
            y_locs.reserve(num_pins);
        }

        for (APPinId pin_id : ap_netlist_.net_pins(net_id)) {
            APBlockId blk_id = ap_netlist_.pin_block(pin_id);
            x_locs.push_back(p_placement.block_x_locs[blk_id]);
            y_locs.push_back(p_placement.block_y_locs[blk_id]);
        }

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
            if (block_is_moveable_(blk_id)) {
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

    // --ap_timing_tradeoff defaults to a nonzero value (0.5), so falling back to unit
    // net weights on architectures without timing data is the expected common case,
    // not something worth warning about.
    bool use_timing_weights = effective_timing_tradeoff_ != 0.f && pre_cluster_timing_manager_.is_valid();

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

            // Interpolate between unit weight and net criticality. Connection-
            // specific timing pressure is handled separately so one critical
            // sink does not increase the hypernet weight of every sink.
            double crit = pre_cluster_timing_manager_.calc_net_setup_criticality(atom_net_id, atom_netlist_);
            weight = effective_timing_tradeoff_ * crit + (1.0 - effective_timing_tradeoff_);
        }

        // Nets flagged by update_boundary_net_flags_/block_is_io_chain_block_ get extra
        // wirelength weight so their pin blocks are pulled tightly together; this counteracts
        // the differentiable wirelength term's tendency to let long boundary-anchored and I/O-chain nets
        // spread out, which otherwise fragments those chains across the AP-to-APPack handoff.
        if (static_cast<size_t>(net_id) < boundary_cohesion_nets_.size() && boundary_cohesion_nets_[net_id])
            weight *= kBoundaryNetCohesionWeight;
        if (static_cast<size_t>(net_id) < io_chain_cohesion_nets_.size() && io_chain_cohesion_nets_[net_id])
            weight *= io_chain_net_cohesion_weight_;
        // Boost all direct output-driver↔outpad nets (not just seed-long I/O-chain ones).
        if (static_cast<size_t>(net_id) < io_pair_locality_nets_.size() && io_pair_locality_nets_[net_id]) {
            weight *= io_pair_net_weight_;
        }
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

void NonlinearNesterovPlacer::initialize_pack_pattern_affinity_groups_(const Prepacker& prepacker) {
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
        AffinityGroup affinity;
        affinity.kind = e_affinity_kind::PACK_PATTERN;
        affinity.blocks = std::move(group);
        affinity_groups_.push_back(std::move(affinity));
        num_pack_pattern_affinity_groups_++;
    }
}

void NonlinearNesterovPlacer::initialize_io_pair_affinity_groups_() {
    io_pair_locality_nets_.resize(ap_netlist_.nets().size(), false);
    std::fill(io_pair_locality_nets_.begin(), io_pair_locality_nets_.end(), false);
    if (io_pair_attraction_weight_ == 0. && io_pair_net_weight_ == 1.)
        return;

    for (APNetId net_id : ap_netlist_.nets()) {
        if (ap_netlist_.net_is_ignored(net_id))
            continue;
        if (ap_netlist_.net_pins(net_id).size() != 2)
            continue;

        AtomNetId atom_net_id = ap_netlist_.net_atom_net(net_id);
        if (!atom_net_id.is_valid())
            continue;
        if (atom_netlist_.net_sinks(atom_net_id).size() != 1)
            continue;

        AtomBlockId driver_blk = atom_netlist_.net_driver_block(atom_net_id);
        AtomPinId sink_pin = *atom_netlist_.net_sinks(atom_net_id).begin();
        AtomBlockId sink_blk = atom_netlist_.pin_block(sink_pin);
        if (!driver_blk.is_valid() || !sink_blk.is_valid())
            continue;
        if (atom_netlist_.block_type(driver_blk) != AtomBlockType::BLOCK)
            continue;
        // Treat only combinational blocks immediately upstream of the primary
        // output as output drivers. This excludes registered I/O primitives
        // without relying on architecture-specific logical-model names.
        if (!atom_netlist_.block_clock_pins(driver_blk).empty())
            continue;
        if (atom_netlist_.block_type(sink_blk) != AtomBlockType::OUTPAD)
            continue;

        APBlockId driver_ap_blk = APBlockId::INVALID();
        APBlockId outpad_ap_blk = APBlockId::INVALID();
        for (APPinId pin_id : ap_netlist_.net_pins(net_id)) {
            AtomPinId atom_pin_id = ap_netlist_.pin_atom_pin(pin_id);
            if (!atom_pin_id.is_valid())
                continue;
            AtomBlockId atom_blk = atom_netlist_.pin_block(atom_pin_id);
            APBlockId ap_blk = ap_netlist_.pin_block(pin_id);
            if (atom_blk == driver_blk)
                driver_ap_blk = ap_blk;
            if (atom_blk == sink_blk)
                outpad_ap_blk = ap_blk;
        }
        if (!driver_ap_blk.is_valid() || !outpad_ap_blk.is_valid())
            continue;
        // Already co-located in one AP block — nothing for GP to reunite.
        if (driver_ap_blk == outpad_ap_blk)
            continue;

        io_pair_locality_nets_[net_id] = true;
        if (io_pair_attraction_weight_ == 0.)
            continue;

        AffinityGroup affinity;
        affinity.kind = e_affinity_kind::IO_PAIR;
        affinity.blocks = {driver_ap_blk, outpad_ap_blk};
        affinity_groups_.push_back(std::move(affinity));
        num_io_pair_affinity_groups_++;
    }
}

double NonlinearNesterovPlacer::evaluate_checkpoint_cpd_(const PartialPlacement& placement) {
    if (!pre_cluster_timing_manager_.is_valid() || !place_delay_model_)
        return std::numeric_limits<double>::infinity();

    update_timing_info_with_partial_placement(pre_cluster_timing_manager_,
                                              *place_delay_model_,
                                              placement,
                                              ap_netlist_);
    return pre_cluster_timing_manager_.get_timing_info().least_slack_critical_path().delay() * 1e9;
}

double NonlinearNesterovPlacer::affinity_kernel_weight_(e_affinity_kind kind) const {
    switch (kind) {
        case e_affinity_kind::IO_PAIR:
            // Legacy I/O pair spring used grad += W * dx (no 1/n). Pack-math kernel
            // uses W_kernel / n; for n=2 set W_kernel = 2W to preserve strength.
            return 2. * io_pair_attraction_weight_;
        case e_affinity_kind::PACK_PATTERN:
            return pack_pattern_cohesion_weight_;
        default:
            VTR_ASSERT_MSG(false, "Unhandled affinity kind");
            return 0.;
    }
}

double NonlinearNesterovPlacer::add_affinity_spring_gradient_(const PartialPlacement& p_placement,
                                                              std::optional<std::reference_wrapper<PlacementGradient>> grad) const {
    if (affinity_groups_.empty())
        return 0.;

    double weighted_penalty = 0.;
    for (const AffinityGroup& group : affinity_groups_) {
        VTR_ASSERT_SAFE(group.blocks.size() >= 2);
        double weight = affinity_kernel_weight_(group.kind);
        if (weight == 0.)
            continue;

        double centroid_x = 0.;
        double centroid_y = 0.;
        for (APBlockId blk_id : group.blocks) {
            centroid_x += p_placement.block_x_locs[blk_id];
            centroid_y += p_placement.block_y_locs[blk_id];
        }

        double inv_group_size = 1. / static_cast<double>(group.blocks.size());
        centroid_x *= inv_group_size;
        centroid_y *= inv_group_size;

        double unweighted = 0.;
        for (APBlockId blk_id : group.blocks) {
            double dx = p_placement.block_x_locs[blk_id] - centroid_x;
            double dy = p_placement.block_y_locs[blk_id] - centroid_y;
            unweighted += 0.5 * inv_group_size * (dx * dx + dy * dy);
            if (grad && block_is_moveable_(blk_id)) {
                grad->get().dx[blk_id] += weight * inv_group_size * dx;
                grad->get().dy[blk_id] += weight * inv_group_size * dy;
            }
        }
        weighted_penalty += weight * unweighted;
    }
    return weighted_penalty;
}

void NonlinearNesterovPlacer::report_density_force_leak_(const PartialPlacement& p_placement,
                                                         const std::vector<PrimitiveVectorDim>& dimensions,
                                                         const std::vector<double>& density_multipliers,
                                                         size_t epoch) const {
    if (dimensions.empty() || density_potential_workspace_.size() < dimensions.size())
        return;

    const size_t width = device_grid_width_;
    const size_t height = device_grid_height_;
    const size_t num_layers = std::max<size_t>(1, device_grid_num_layers_);
    const size_t num_sites = width * height * num_layers;
    initialize_density_target_cache_(dimensions);
    if (cached_target_capacity_.size() < dimensions.size())
        return;
    auto site_index = [width, height](size_t layer, size_t x, size_t y) {
        return (layer * height + y) * width + x;
    };

    const std::vector<std::vector<double>>& target_capacity = cached_target_capacity_;
    const std::vector<double>& target_norm_floor = cached_target_norm_floor_;
    const std::vector<double>& residual_charge_scale = cached_residual_charge_scale_;
    const std::vector<std::vector<double>>& potential = density_potential_workspace_;

    // Share of the device that can hold each resource at all. A force with no
    // directional information would land on an unusable tile at exactly this
    // rate, so it is the baseline the measured leak has to be read against.
    std::vector<double> usable_site_fraction(dimensions.size(), 0.);
    for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
        size_t usable = 0;
        for (size_t idx = 0; idx < num_sites; idx++) {
            if (target_capacity[dim_idx][idx] > kEpsilon)
                usable++;
        }
        usable_site_fraction[dim_idx] = num_sites > 0 ? static_cast<double>(usable) / num_sites : 0.;
    }

    std::vector<double> force_total(dimensions.size(), 0.);
    std::vector<double> force_into_void(dimensions.size(), 0.);
    std::vector<size_t> blocks_seen(dimensions.size(), 0);
    std::vector<size_t> blocks_starting_in_void(dimensions.size(), 0);

    for (APBlockId blk_id : moveable_blocks_) {
        PrimitiveVector block_mass = density_manager_->mass_calculator().get_block_mass(blk_id);
        if (block_mass.is_zero())
            continue;
        block_mass *= pin_density_inflation_[blk_id];

        double x = std::clamp(p_placement.block_x_locs[blk_id], 0., device_grid_width_ - kDeviceBoundaryEpsilon);
        double y = std::clamp(p_placement.block_y_locs[blk_id], 0., device_grid_height_ - kDeviceBoundaryEpsilon);
        size_t layer = static_cast<size_t>(std::clamp(std::round(p_placement.block_layer_nums[blk_id]),
                                                      0.,
                                                      static_cast<double>(device_grid_num_layers_ - 1)));
        BilinearDensityStencil stencil = make_bilinear_density_stencil(x, y, width, height);

        // Per dimension, rebuild exactly the force add_density_gradient_ applies
        // (the gradient is descended, so motion is along its negation) and probe
        // one tile ahead of it.
        for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
            double mass = block_mass.get_dim_val(dimensions[dim_idx]);
            if (mass == 0.)
                continue;

            double local_target = interpolate_bilinear_density(target_capacity[dim_idx],
                                                               layer,
                                                               width,
                                                               height,
                                                               stencil);
            auto [local_field_x, local_field_y] = gradient_bilinear_density(potential[dim_idx],
                                                                            layer,
                                                                            width,
                                                                            height,
                                                                            stencil);

            double normalized_mass = kUseResidualDensityCharge
                                         ? mass / residual_charge_scale[dim_idx]
                                         : mass / std::max(local_target, target_norm_floor[dim_idx]);
            double coefficient = density_multipliers[dim_idx];
            double gradient_x = coefficient * normalized_mass * local_field_x;
            double gradient_y = coefficient * normalized_mass * local_field_y;

            double magnitude = std::hypot(gradient_x, gradient_y);
            blocks_seen[dim_idx]++;
            if (target_capacity[dim_idx][site_index(layer, stencil.xs[0], stencil.ys[0])] <= kEpsilon)
                blocks_starting_in_void[dim_idx]++;
            if (magnitude <= kEpsilon)
                continue;

            // Descent moves against the gradient.
            double step_x = -gradient_x / magnitude;
            double step_y = -gradient_y / magnitude;
            double dest_x = std::clamp(x + step_x, 0., device_grid_width_ - kDeviceBoundaryEpsilon);
            double dest_y = std::clamp(y + step_y, 0., device_grid_height_ - kDeviceBoundaryEpsilon);
            size_t dest_idx = site_index(layer,
                                         static_cast<size_t>(std::floor(dest_x)),
                                         static_cast<size_t>(std::floor(dest_y)));

            force_total[dim_idx] += magnitude;
            if (target_capacity[dim_idx][dest_idx] <= kEpsilon)
                force_into_void[dim_idx] += magnitude;
        }
    }

    const PrimitiveDimManager& dim_manager = density_manager_->mass_calculator().get_dim_manager();
    VTR_LOG("  Nesterov density-force leak (epoch %zu): fraction of density force aiming at tiles with no capacity for the resource.\n",
            epoch);
    VTR_LOG("    %-44s %9s %9s %9s %8s\n", "dim", "leak", "baseline", "excess", "blocks");
    for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
        if (blocks_seen[dim_idx] == 0)
            continue;
        double leak = force_total[dim_idx] > kEpsilon
                          ? force_into_void[dim_idx] / force_total[dim_idx]
                          : 0.;
        double baseline = 1. - usable_site_fraction[dim_idx];
        std::string dim_name = dim_manager.get_dim_name(dimensions[dim_idx]);
        if (dim_name.size() > 44)
            dim_name = dim_name.substr(0, 44);
        VTR_LOG("    %-44s %9.4f %9.4f %+9.4f %8zu  (in_void=%zu)\n",
                dim_name.c_str(),
                leak,
                baseline,
                leak - baseline,
                blocks_seen[dim_idx],
                blocks_starting_in_void[dim_idx]);
    }
}

void NonlinearNesterovPlacer::audit_gradient_(const PartialPlacement& p_placement,
                                              const std::vector<PrimitiveVectorDim>& dimensions,
                                              const std::vector<double>& density_multipliers,
                                              std::optional<std::reference_wrapper<const PartialPlacement>> legal_anchor,
                                              double proximity_weight,
                                              const FillerState& fillers) const {
    if (moveable_blocks_.empty())
        return;
    initialize_density_target_cache_(dimensions);

    const size_t width = device_grid_width_;
    const size_t height = device_grid_height_;
    auto site_index = [width, height](size_t layer, size_t x, size_t y) {
        return (layer * height + y) * width + x;
    };

    // --- Probe selection -----------------------------------------------------
    // Group 1: within kBoundaryBand of a tile boundary (deposition support changes).
    // Group 2: on a site with no capacity for one of the block's own resources
    //          (the charge-neutrality active-set boundary).
    // Group 3: uniform control.
    constexpr double kBoundaryBand = 0.02;
    constexpr size_t kPerGroup = 8;
    std::vector<APBlockId> at_tile_edge, in_void, control;
    for (APBlockId blk_id : moveable_blocks_) {
        double x = p_placement.block_x_locs[blk_id];
        double y = p_placement.block_y_locs[blk_id];
        double fx = x - std::floor(x);
        double fy = y - std::floor(y);
        if ((fx < kBoundaryBand || fx > 1. - kBoundaryBand || fy < kBoundaryBand || fy > 1. - kBoundaryBand)
            && at_tile_edge.size() < kPerGroup) {
            at_tile_edge.push_back(blk_id);
            continue;
        }
        if (in_void.size() < kPerGroup && !cached_target_capacity_.empty()) {
            size_t layer = static_cast<size_t>(std::clamp(std::round(p_placement.block_layer_nums[blk_id]),
                                                          0., static_cast<double>(device_grid_num_layers_ - 1)));
            size_t sx = static_cast<size_t>(std::clamp(std::floor(x), 0., device_grid_width_ - 1.));
            size_t sy = static_cast<size_t>(std::clamp(std::floor(y), 0., device_grid_height_ - 1.));
            size_t idx = site_index(layer, sx, sy);
            const PrimitiveVector& mass = density_manager_->mass_calculator().get_block_mass(blk_id);
            for (size_t d = 0; d < dimensions.size(); d++) {
                if (mass.get_dim_val(dimensions[d]) != 0.
                    && idx < cached_target_capacity_[d].size()
                    && cached_target_capacity_[d][idx] <= kEpsilon) {
                    in_void.push_back(blk_id);
                    break;
                }
            }
            if (!in_void.empty() && in_void.back() == blk_id)
                continue;
        }
        if (control.size() < kPerGroup)
            control.push_back(blk_id);
    }

    struct Group {
        const char* name;
        const std::vector<APBlockId>* blks;
    };
    const Group groups[] = {{"at tile boundary", &at_tile_edge},
                            {"on zero-capacity site", &in_void},
                            {"control (interior)", &control}};

    // Two weight settings: full objective, and density switched off so the
    // wirelength/affinity/proximity part can be audited on its own.
    std::vector<double> zero_multipliers(density_multipliers.size(), 0.);
    struct Config {
        const char* name;
        const std::vector<double>* mult;
    };
    const Config configs[] = {{"full objective", &density_multipliers},
                              {"density weights = 0 (WL+affinity+proximity)", &zero_multipliers}};

    const double steps[] = {1e-1, 1e-2, 1e-3, 1e-4};

    VTR_LOG("\n=== Nonlinear Nesterov gradient audit (directional finite differences) ===\n");
    VTR_LOG("probes: %zu at tile boundary, %zu on zero-capacity sites, %zu control\n",
            at_tile_edge.size(), in_void.size(), control.size());
    VTR_LOG("A correct derivative's error falls with the step, then rises as 1/step from\n"
            "round-off. A wrong derivative plateaus instead.\n");

    PartialPlacement probe(ap_netlist_);
    for (const Config& cfg : configs) {
        PlacementGradient grad(ap_netlist_);
        FillerGradient filler_grad;
        evaluate_objective_(p_placement, *cfg.mult, legal_anchor, proximity_weight,
                            std::ref(grad), fillers, std::ref(filler_grad));
        VTR_LOG("\n  [%s]\n", cfg.name);
        VTR_LOG("    %-24s %10s %12s %12s %12s\n", "probe group", "step", "max rel err", "mean rel err", "samples");
        for (const Group& g : groups) {
            if (g.blks->empty())
                continue;
            for (double h : steps) {
                double worst = 0., total = 0.;
                size_t count = 0;
                APBlockId worst_blk;
                int worst_axis = -1;
                double worst_fd = 0., worst_an = 0.;
                for (APBlockId blk_id : *g.blks) {
                    for (int axis = 0; axis < 2; axis++) {
                        double analytic = axis == 0 ? grad.dx[blk_id] : grad.dy[blk_id];
                        probe.block_x_locs = p_placement.block_x_locs;
                        probe.block_y_locs = p_placement.block_y_locs;
                        probe.block_layer_nums = p_placement.block_layer_nums;
                        probe.block_sub_tiles = p_placement.block_sub_tiles;
                        auto& coord = axis == 0 ? probe.block_x_locs : probe.block_y_locs;
                        double base = coord[blk_id];
                        coord[blk_id] = base + h;
                        double f_plus = evaluate_objective_(probe, *cfg.mult, legal_anchor, proximity_weight,
                                                            std::nullopt, fillers, std::nullopt)
                                            .total;
                        coord[blk_id] = base - h;
                        double f_minus = evaluate_objective_(probe, *cfg.mult, legal_anchor, proximity_weight,
                                                             std::nullopt, fillers, std::nullopt)
                                             .total;
                        double fd = (f_plus - f_minus) / (2. * h);
                        double scale = std::max({std::abs(fd), std::abs(analytic), 1e-12});
                        double rel = std::abs(fd - analytic) / scale;
                        if (rel > worst) {
                            worst = rel;
                            worst_blk = blk_id;
                            worst_axis = axis;
                            worst_fd = fd;
                            worst_an = analytic;
                        }
                        total += rel;
                        count++;
                    }
                }
                VTR_LOG("    %-24s %10.0e %12.3e %12.3e %12zu\n",
                        g.name, h, worst, count ? total / count : 0., count);
                // Name the offending probe when the disagreement is gross, so the
                // reader can tell a real derivative error from a kink in the
                // piecewise-bilinear kernel at a cell boundary.
                if (worst > 0.5 && worst_axis >= 0) {
                    double wx = p_placement.block_x_locs[worst_blk];
                    double wy = p_placement.block_y_locs[worst_blk];
                    VTR_LOG("        worst probe: blk=%zu axis=%s pos=(%.6f, %.6f) "
                            "floor=(%.0f, %.0f) grid=%zux%zu analytic=%.6g fd=%.6g\n",
                            static_cast<size_t>(worst_blk), worst_axis == 0 ? "x" : "y",
                            wx, wy, std::floor(wx), std::floor(wy),
                            width, height, worst_an, worst_fd);
                }
            }
        }
    }
    // --- Fillers -------------------------------------------------------------
    // Fillers reach the objective through their own deposition loop, their own
    // force-extraction loop and their own gradient container, so nothing above
    // touches that path. They are not a small corner of the model either: large
    // designs carry tens of thousands of filler particles.
    bool any_fillers = false;
    for (const std::vector<double>& per_dim : fillers.x) {
        if (!per_dim.empty()) {
            any_fillers = true;
            break;
        }
    }
    if (any_fillers) {
        PlacementGradient grad(ap_netlist_);
        FillerGradient filler_grad;
        evaluate_objective_(p_placement, density_multipliers, legal_anchor, proximity_weight,
                            std::ref(grad), fillers, std::ref(filler_grad));

        // Sample a few fillers per dimension, evenly spaced across each dimension's
        // list including both ends. Spacing them by (s * n) / kPerDim instead put two
        // probes on index 0 whenever a dimension held exactly two fillers, so such a
        // dimension was audited twice at one particle and never at the other.
        std::vector<std::pair<size_t, size_t>> probes; // (dim, filler index)
        constexpr size_t kPerDim = 3;
        for (size_t d = 0; d < fillers.x.size(); d++) {
            size_t n = fillers.x[d].size();
            if (n == 0 || d >= filler_grad.dx.size())
                continue;
            size_t k = std::min(kPerDim, n);
            for (size_t s = 0; s < k; s++)
                probes.emplace_back(d, k == 1 ? 0 : (s * (n - 1)) / (k - 1));
        }

        // A probe only tests the derivative fairly when the whole [c-h, c+h] bracket
        // stays inside the grid cell the filler currently occupies and clear of the
        // clamp at the device edge. Outside that, the piecewise-bilinear deposition
        // kernel has a kink between the two evaluation points (or, past the clamp, a
        // plateau), so a central difference straddles a corner of the function and
        // disagrees with the true one-sided derivative however correct that
        // derivative is. Those probes are reported as their own group rather than
        // dropped, so an inflated boundary number cannot mask a real interior error
        // and an interior error cannot be excused as a boundary artifact.
        struct WorstProbe {
            double rel = 0.;
            size_t dim = 0, idx = 0;
            int axis = -1;
            double x = 0., y = 0., analytic = 0., fd = 0.;
        };

        VTR_LOG("\n  [fillers: density-only, separate gradient path]\n");
        // The h^2-then-1/h guidance printed at the top of this audit does NOT apply to
        // the interior filler rows, and a reader who expects it will misdiagnose a
        // correct result. Deposition weights are linear in the filler coordinate, so
        // charge is linear in it; the Poisson solve is linear; and energy is a
        // quadratic form in charge. Within one cell the density energy is therefore
        // *exactly quadratic* in the filler position, and a central difference is exact
        // for quadratics. Truncation error vanishes identically, leaving only
        // round-off: the interior error is smallest at the LARGEST step and grows as
        // 1/h. That pattern is the correct one here.
        VTR_LOG("    interior rows: energy is exactly quadratic in a filler's position within a cell,\n"
                "    so the central difference is exact and error is pure round-off (grows as 1/step).\n"
                "    Smallest error at the largest step is correct here, not a defect.\n");
        VTR_LOG("    %-28s %10s %12s %12s %12s\n", "probe group", "step", "max rel err", "mean rel err", "samples");
        FillerState probe_fillers;
        for (double h : steps) {
            double worst[2] = {0., 0.}, total[2] = {0., 0.};
            size_t count[2] = {0, 0};
            WorstProbe worst_probe[2];
            for (const auto& [d, i] : probes) {
                for (int axis = 0; axis < 2; axis++) {
                    double analytic = axis == 0 ? filler_grad.dx[d][i] : filler_grad.dy[d][i];
                    probe_fillers.x = fillers.x;
                    probe_fillers.y = fillers.y;
                    probe_fillers.layer = fillers.layer;
                    auto& coord = axis == 0 ? probe_fillers.x : probe_fillers.y;
                    double base = coord[d][i];

                    // Classify the bracket before perturbing anything.
                    double extent = axis == 0 ? static_cast<double>(width) : static_cast<double>(height);
                    double upper = extent - kDeviceBoundaryEpsilon;
                    double centre = std::clamp(base, 0., upper);
                    bool interior = base - h >= 0. && base + h <= upper
                                    && std::floor(base - h) == std::floor(centre)
                                    && std::floor(base + h) == std::floor(centre)
                                    && std::floor(centre) < extent - 1.;
                    int g = interior ? 0 : 1;

                    coord[d][i] = base + h;
                    ObjectiveValue plus = evaluate_objective_(p_placement, density_multipliers, legal_anchor,
                                                              proximity_weight, std::nullopt, probe_fillers,
                                                              std::nullopt);
                    coord[d][i] = base - h;
                    ObjectiveValue minus = evaluate_objective_(p_placement, density_multipliers, legal_anchor,
                                                               proximity_weight, std::nullopt, probe_fillers,
                                                               std::nullopt);
                    // Difference the weighted density energy of this filler's own
                    // dimension rather than the full objective. Wirelength, affinity
                    // and proximity do not depend on filler positions at all, so in
                    // exact arithmetic they cancel -- but they are orders of magnitude
                    // larger than the density term being measured, and subtracting two
                    // nearly equal large numbers puts a cancellation-noise floor under
                    // every error this loop can report. Differencing exactly the term
                    // the gradient describes removes that floor. Moving one filler in
                    // dimension d perturbs no other dimension's energy, so restricting
                    // to index d loses nothing.
                    double mult = d < density_multipliers.size() ? density_multipliers[d] : 0.;
                    double f_plus = mult * (d < plus.density_energies.size() ? plus.density_energies[d] : 0.);
                    double f_minus = mult * (d < minus.density_energies.size() ? minus.density_energies[d] : 0.);
                    double fd = (f_plus - f_minus) / (2. * h);
                    double scale = std::max({std::abs(fd), std::abs(analytic), 1e-12});
                    double rel = std::abs(fd - analytic) / scale;
                    if (rel > worst[g]) {
                        worst[g] = rel;
                        worst_probe[g] = WorstProbe{rel, d, i, axis,
                                                    fillers.x[d][i], fillers.y[d][i], analytic, fd};
                    }
                    total[g] += rel;
                    count[g]++;
                }
            }
            for (int g = 0; g < 2; g++) {
                if (count[g] == 0)
                    continue;
                VTR_LOG("    %-28s %10.0e %12.3e %12.3e %12zu\n",
                        g == 0 ? "filler (cell interior)" : "filler (cell/edge boundary)",
                        h, worst[g], total[g] / count[g], count[g]);
                const WorstProbe& w = worst_probe[g];
                if (w.rel > 0.5 && w.axis >= 0) {
                    VTR_LOG("        worst probe: dim=%zu filler=%zu axis=%s pos=(%.6f, %.6f) "
                            "floor=(%.0f, %.0f) grid=%zux%zu analytic=%.6g fd=%.6g\n",
                            w.dim, w.idx, w.axis == 0 ? "x" : "y", w.x, w.y,
                            std::floor(w.x), std::floor(w.y), width, height, w.analytic, w.fd);
                }
            }
        }
    } else {
        VTR_LOG("\n  [fillers: none active on this design -- filler gradient NOT audited]\n");
    }

    VTR_LOG("=== end gradient audit ===\n\n");
}

void NonlinearNesterovPlacer::initialize_density_target_cache_(const std::vector<PrimitiveVectorDim>& dimensions) const {
    size_t width = device_grid_width_;
    size_t height = device_grid_height_;
    size_t num_layers = device_grid_num_layers_;
    size_t num_sites = width * height * num_layers;
    if (cached_target_capacity_.size() == dimensions.size()
        && (dimensions.empty() || cached_target_capacity_.front().size() == num_sites))
        return;

    cached_target_capacity_.assign(dimensions.size(), std::vector<double>(num_sites, 0.));
    cached_target_norm_floor_.assign(dimensions.size(), kEpsilon);
    cached_residual_charge_scale_.assign(dimensions.size(), 1.0);

    const FlatPlacementBins& bins = density_manager_->flat_placement_bins();
    auto site_index = [width, height](size_t layer, size_t x, size_t y) {
        return (layer * height + y) * width + x;
    };
    for (size_t layer = 0; layer < num_layers; layer++) {
        for (size_t x = 0; x < width; x++) {
            for (size_t y = 0; y < height; y++) {
                FlatPlacementBinId bin_id = density_manager_->get_bin(x, y, layer);
                const vtr::Rect<double>& region = bins.bin_region(bin_id);
                double bin_area = std::max(1.0, region.width() * region.height());
                double target_density = density_manager_->get_bin_target_density(bin_id);
                size_t idx = site_index(layer, x, y);
                for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
                    cached_target_capacity_[dim_idx][idx] = density_manager_->get_bin_capacity(bin_id).get_dim_val(dimensions[dim_idx])
                                                            * target_density / bin_area;
                }
            }
        }
    }

    for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
        double target_sum = 0.;
        size_t target_sites = 0;
        for (double target : cached_target_capacity_[dim_idx]) {
            if (target <= kEpsilon)
                continue;
            target_sum += target;
            target_sites++;
        }
        if (target_sites != 0) {
            cached_target_norm_floor_[dim_idx] = std::max(kEpsilon,
                                                          kDensityTargetFloorFraction * target_sum / target_sites);
            cached_residual_charge_scale_[dim_idx] = std::max(kEpsilon, target_sum / target_sites);
        }
    }
}

double NonlinearNesterovPlacer::add_density_gradient_(const PartialPlacement& p_placement,
                                                      const std::vector<double>& density_multipliers,
                                                      std::vector<double>& density_energies,
                                                      std::vector<double>& dim_overflow_ratios,
                                                      std::vector<double>& dim_overflow_mass,
                                                      std::vector<double>& dim_max_overflow,
                                                      double& total_overflow,
                                                      double& max_overflow,
                                                      double& overflow_ratio,
                                                      std::optional<std::reference_wrapper<PlacementGradient>> grad,
                                                      const FillerState& fillers,
                                                      std::optional<std::reference_wrapper<FillerGradient>> filler_grad) const {
    total_overflow = 0.;
    max_overflow = 0.;
    overflow_ratio = 0.;
    // ePlace legality signal: overflow mass (mass exceeding tile target) over total
    // deposited mass. Normalizing by movable mass (not total grid capacity) avoids
    // diluting local overfill on designs that occupy a small fraction of the grid.
    double overflow_area = 0.;
    double total_deposited_mass = 0.;

    std::vector<PrimitiveVectorDim> dimensions = density_manager_->get_used_dims_mask().get_non_zero_dims();
    if (dimensions.empty())
        return 0.;
    VTR_ASSERT(density_multipliers.size() == dimensions.size());
    density_energies.assign(dimensions.size(), 0.);
    dim_overflow_ratios.assign(dimensions.size(), 0.);
    dim_overflow_mass.assign(dimensions.size(), 0.);
    dim_max_overflow.assign(dimensions.size(), 0.);

    size_t width = device_grid_width_;
    size_t height = device_grid_height_;
    size_t num_layers = device_grid_num_layers_;
    size_t num_sites = width * height * num_layers;
    initialize_density_target_cache_(dimensions);
    auto site_index = [width, height](size_t layer, size_t x, size_t y) {
        return (layer * height + y) * width + x;
    };

    // Arrays with grid information of the density objective via electrostatic formulation.
    // Reuse their storage across the many objective/line-search evaluations.
    // Utilization: deposited block mass at each site
    // Target Capacity: available target mass/capacity at each site
    // Potential: solved electrostatic potential
    // The potential's spatial derivative is not materialized on the grid: the
    // block/filler gradients take it analytically from the four surrounding
    // potential samples (the derivative of the bilinear interpolant the mass
    // deposition implies), so no field grid is needed.
    auto reset_grid_workspace = [num_sites, &dimensions](std::vector<std::vector<double>>& workspace) {
        if (workspace.size() != dimensions.size()
            || (!workspace.empty() && workspace.front().size() != num_sites)) {
            workspace.assign(dimensions.size(), std::vector<double>(num_sites, 0.));
            return;
        }
        for (std::vector<double>& dimension_values : workspace)
            std::fill(dimension_values.begin(), dimension_values.end(), 0.);
    };
    // Potential only needs a size check, not a zero-fill: every dimension in
    // `dimensions` comes from get_used_dims_mask().get_non_zero_dims(), so
    // active_sites > 0 below and the Poisson write-back unconditionally
    // overwrites every site for every dimension before potential is read.
    auto resize_grid_workspace = [num_sites, &dimensions](std::vector<std::vector<double>>& workspace) {
        if (workspace.size() != dimensions.size()
            || (!workspace.empty() && workspace.front().size() != num_sites)) {
            workspace.assign(dimensions.size(), std::vector<double>(num_sites, 0.));
        }
    };
    reset_grid_workspace(density_utilization_workspace_);
    resize_grid_workspace(density_potential_workspace_);
    std::vector<std::vector<double>>& utilization = density_utilization_workspace_;
    std::vector<std::vector<double>>& potential = density_potential_workspace_;

    // The target capacity spread over each bin's footprint, and the per-dimension
    // normalization constants derived from it, depend only on the (fixed) device
    // grid, bin capacity, and target density -- not on the placement. Build them
    // once and reuse across every objective evaluation.
    //  - target_norm_floor: a floor added wherever utilization is divided by target
    //    capacity. Bin-footprint spreading and target_density can leave a site with
    //    a vanishingly small (but nonzero) fractional capacity for this dimension;
    //    without this floor, dividing by it would spike the normalized
    //    utilization/overflow numerically even though barely any mass sits there.
    //  - residual_charge_scale: used in the residual-charge mode below to express
    //    (utilization - target) in units of "typical site capacity for this
    //    dimension" rather than raw mass. Resource dimensions can have very
    //    different natural capacity magnitudes on a heterogeneous grid (e.g. an
    //    abundant LUT dimension vs. a sparse DSP/BRAM one); this keeps their charge
    //    contributions comparably scaled before they feed the shared Poisson solve.
    const std::vector<std::vector<double>>& target_capacity = cached_target_capacity_;
    const std::vector<double>& target_norm_floor = cached_target_norm_floor_;
    const std::vector<double>& residual_charge_scale = cached_residual_charge_scale_;

    // Deposit each primitive-vector mass bilinearly onto the tile grid.
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        PrimitiveVector block_mass = density_manager_->mass_calculator().get_block_mass(blk_id);
        if (block_mass.is_zero())
            continue;
        // Pin-density cell inflation (routability): scale up the SMOOTH density
        // term's mass for high-pin blocks, not the real legalized footprint the
        // partial legalizer and full legalizer see, so the electrostatic field
        // leaves them more spreading room.
        block_mass *= pin_density_inflation_[blk_id];

        double x = std::clamp(p_placement.block_x_locs[blk_id], 0., device_grid_width_ - kDeviceBoundaryEpsilon);
        double y = std::clamp(p_placement.block_y_locs[blk_id], 0., device_grid_height_ - kDeviceBoundaryEpsilon);
        size_t layer = static_cast<size_t>(std::clamp(std::round(p_placement.block_layer_nums[blk_id]),
                                                      0.,
                                                      static_cast<double>(device_grid_num_layers_ - 1)));
        BilinearDensityStencil stencil = make_bilinear_density_stencil(x, y, width, height);

        // Traverses dimensions, i.e. resource types of the mass abstraction
        for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
            double mass = block_mass.get_dim_val(dimensions[dim_idx]);
            if (mass == 0.)
                continue;
            deposit_bilinear_density(utilization[dim_idx], layer, width, height, stencil, mass);
        }
    }

    // Overflow and stopping use only real physical block mass. Fillers are
    // movable whitespace in the density objective, not legal instances.
    for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
        double dim_overflow_area = 0.;
        double dim_deposited_mass = 0.;
        double dim_peak_overflow = 0.;
        for (size_t idx = 0; idx < num_sites; idx++) {
            double target = target_capacity[dim_idx][idx];
            double utilization_at_site = utilization[dim_idx][idx];
            if (target <= kEpsilon) {
                if (utilization_at_site <= kEpsilon)
                    continue;
                total_overflow += utilization_at_site;
                max_overflow = std::max(max_overflow, utilization_at_site);
                dim_peak_overflow = std::max(dim_peak_overflow, utilization_at_site);
                dim_overflow_area += utilization_at_site;
                dim_deposited_mass += utilization_at_site;
            } else {
                double normalized_utilization = utilization_at_site / std::max(target, target_norm_floor[dim_idx]);
                double normalized_overflow = std::max(0., normalized_utilization - 1.0);
                total_overflow += normalized_overflow;
                max_overflow = std::max(max_overflow, normalized_overflow);
                dim_peak_overflow = std::max(dim_peak_overflow, normalized_overflow);
                dim_overflow_area += std::max(0., utilization_at_site - target);
                dim_deposited_mass += utilization_at_site;
            }
        }
        dim_overflow_mass[dim_idx] = dim_overflow_area;
        dim_max_overflow[dim_idx] = dim_peak_overflow;
        dim_overflow_ratios[dim_idx] = dim_deposited_mass > kEpsilon ? dim_overflow_area / dim_deposited_mass : 0.;
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
            BilinearDensityStencil stencil = make_bilinear_density_stencil(x, y, width, height);
            deposit_bilinear_density(utilization[dim_idx], layer, width, height, stencil, unit_mass);
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
        if (density_charge_workspace_.size() != num_sites)
            density_charge_workspace_.resize(num_sites);
        std::fill(density_charge_workspace_.begin(), density_charge_workspace_.end(), 0.);
        std::vector<double>& charge = density_charge_workspace_;
        size_t active_sites = 0;

        for (size_t idx = 0; idx < num_sites; idx++) {
            double target = target_capacity[dim_idx][idx];
            double utilization_at_site = utilization[dim_idx][idx];
            bool has_capacity = target > kEpsilon;
            if (!has_capacity) {
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
            active_sites++;
        }

        if (active_sites == 0)
            continue;

        // Rebalance charge over the fixed capacity-site mask. Bilinear deposition
        // conserves total mass, so both the mask and the subtracted amount are
        // placement-invariant and contribute no missing derivative. On sparse
        // resources this also acts as an architecture-attraction field; it is not
        // merely the uniform DC projection already performed by the Poisson solve.
        rebalance_density_charge_on_capacity_sites(charge, target_capacity[dim_idx], kEpsilon);

        // Solve each layer on the full rectangular field domain. Capacity-free
        // sites carry zero charge but remain part of the Neumann PDE domain.
        density_layer_charge_workspace_.resize(width * height);
        std::vector<double>& layer_charge = density_layer_charge_workspace_;
        std::vector<double>& layer_potential = density_layer_potential_workspace_;
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
    }

    overflow_ratio = total_deposited_mass > kEpsilon ? overflow_area / total_deposited_mass : 0.;

    if (!grad)
        return density_energy;

    // Turn the grid field into a block gradient
    for (APBlockId blk_id : moveable_blocks_) {
        PrimitiveVector block_mass = density_manager_->mass_calculator().get_block_mass(blk_id);
        if (block_mass.is_zero())
            continue;
        // Same pin-density inflation as the deposition pass above, so the force
        // extracted here matches the (inflated) mass that shaped the field.
        block_mass *= pin_density_inflation_[blk_id];

        double x = std::clamp(p_placement.block_x_locs[blk_id], 0., device_grid_width_ - kDeviceBoundaryEpsilon);
        double y = std::clamp(p_placement.block_y_locs[blk_id], 0., device_grid_height_ - kDeviceBoundaryEpsilon);
        size_t layer = static_cast<size_t>(std::clamp(std::round(p_placement.block_layer_nums[blk_id]),
                                                      0.,
                                                      static_cast<double>(device_grid_num_layers_ - 1)));
        BilinearDensityStencil stencil = make_bilinear_density_stencil(x, y, width, height);

        // Accumulate density-only force so along-rim damping does not touch WL/proximity.
        double density_dx = 0.;
        double density_dy = 0.;
        for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
            double mass = block_mass.get_dim_val(dimensions[dim_idx]);
            if (mass == 0.)
                continue;

            double local_target = interpolate_bilinear_density(target_capacity[dim_idx],
                                                               layer,
                                                               width,
                                                               height,
                                                               stencil);
            double local_field_x = 0.;
            double local_field_y = 0.;
            {
                // For E = 0.5*q^T*A*q with symmetric Poisson inverse A, the
                // deposition-consistent derivative is m*sum_i(dw_i/dx)*Phi_i.
                // Differentiating the bilinear potential directly is essential;
                // interpolating grid-point finite differences is a different
                // discretization. Degenerate cells correctly produce zero.
                std::pair<double, double> local_field = gradient_bilinear_density(potential[dim_idx],
                                                                                  layer,
                                                                                  width,
                                                                                  height,
                                                                                  stencil);
                local_field_x = local_field.first;
                local_field_y = local_field.second;
            }

            double normalized_mass = kUseResidualDensityCharge
                                         ? mass / residual_charge_scale[dim_idx]
                                         : mass / std::max(local_target, target_norm_floor[dim_idx]);
            double coefficient = density_multipliers[dim_idx];
            density_dx += coefficient * normalized_mass * local_field_x;
            density_dy += coefficient * normalized_mass * local_field_y;
        }

        grad->get().dx[blk_id] += density_dx;
        grad->get().dy[blk_id] += density_dy;
    }

    if (filler_grad) {
        filler_grad->get().dx.assign(dimensions.size(), {});
        filler_grad->get().dy.assign(dimensions.size(), {});
        for (size_t dim_idx = 0; dim_idx < dimensions.size() && dim_idx < fillers.x.size(); dim_idx++) {
            double unit_mass = dim_idx < filler_unit_mass_.size() ? filler_unit_mass_[dim_idx] : 0.;
            size_t n = fillers.x[dim_idx].size();
            filler_grad->get().dx[dim_idx].assign(n, 0.);
            filler_grad->get().dy[dim_idx].assign(n, 0.);
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
                BilinearDensityStencil stencil = make_bilinear_density_stencil(x, y, width, height);
                // Same deposition-consistent derivative as the block gradient above.
                auto [local_field_x, local_field_y] = gradient_bilinear_density(potential[dim_idx],
                                                                                layer,
                                                                                width,
                                                                                height,
                                                                                stencil);
                filler_grad->get().dx[dim_idx][filler_idx] = coefficient * normalized_mass * local_field_x;
                filler_grad->get().dy[dim_idx][filler_idx] = coefficient * normalized_mass * local_field_y;
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
    if (!dimensions.empty())
        initialize_density_target_cache_(dimensions);
    if (dimensions.empty() || whitespace_fraction <= 0.) {
        if (log_verbosity_ >= 1)
            VTR_LOG("Nonlinear Nesterov dynamic fillers: disabled (whitespace fraction %g).\n", whitespace_fraction);
        return;
    }

    size_t width = device_grid_width_;
    size_t height = device_grid_height_;
    size_t num_layers = device_grid_num_layers_;
    size_t num_sites = width * height * num_layers;

    const std::vector<std::vector<double>>& target_capacity = cached_target_capacity_;

    density_utilization_workspace_.resize(dimensions.size());
    for (std::vector<double>& dim_utilization : density_utilization_workspace_) {
        dim_utilization.resize(num_sites);
        std::fill(dim_utilization.begin(), dim_utilization.end(), 0.);
    }
    std::vector<std::vector<double>>& utilization = density_utilization_workspace_;
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        PrimitiveVector block_mass = density_manager_->mass_calculator().get_block_mass(blk_id);
        if (block_mass.is_zero())
            continue;
        double x = std::clamp(seed.block_x_locs[blk_id], 0., device_grid_width_ - kDeviceBoundaryEpsilon);
        double y = std::clamp(seed.block_y_locs[blk_id], 0., device_grid_height_ - kDeviceBoundaryEpsilon);
        size_t layer = static_cast<size_t>(std::clamp(std::round(seed.block_layer_nums[blk_id]),
                                                      0.,
                                                      static_cast<double>(device_grid_num_layers_ - 1)));
        BilinearDensityStencil stencil = make_bilinear_density_stencil(x, y, width, height);
        for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
            double mass = block_mass.get_dim_val(dimensions[dim_idx]);
            if (mass == 0.)
                continue;
            deposit_bilinear_density(utilization[dim_idx], layer, width, height, stencil, mass);
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

    for (const AffinityGroup& group : affinity_groups_) {
        double weight = affinity_kernel_weight_(group.kind);
        if (weight == 0. || group.blocks.empty())
            continue;
        double curvature = weight / static_cast<double>(group.blocks.size());
        for (APBlockId blk_id : group.blocks)
            block_precond_[blk_id] += curvature;
    }

    // Density Hessian diagonal: the per-dimension density weight scales the block
    // mass it deposits into that resource's field. Heavier blocks under a
    // stronger density push have larger curvature and so take proportionally
    // smaller steps.
    const auto& mass_calculator = density_manager_->mass_calculator();
    for (APBlockId blk_id : ap_netlist_.blocks()) {
        const PrimitiveVector& block_mass = mass_calculator.get_block_mass(blk_id);
        // Match the pin-density-inflated mass add_density_gradient_ actually uses,
        // so the curvature estimate agrees with the objective it is preconditioning.
        double inflation = pin_density_inflation_[blk_id];
        double density_curvature = 0.;
        for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
            double mass = block_mass.get_dim_val(dimensions[dim_idx]) * inflation;
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
    initialize_density_target_cache_(dimensions);
    auto site_index = [width, height](size_t layer, size_t x, size_t y) {
        return (layer * height + y) * width + x;
    };

    // Bin only physical block mass (no fillers) at the floor tile. Nearest-tile
    // deposition concentrates mass relative to the bilinear density field, so this
    // slightly over-reads overflow -- a conservative (stop-later) bias for the stop.
    density_utilization_workspace_.resize(dimensions.size());
    for (std::vector<double>& dim_utilization : density_utilization_workspace_) {
        dim_utilization.resize(num_sites);
        std::fill(dim_utilization.begin(), dim_utilization.end(), 0.);
    }
    std::vector<std::vector<double>>& utilization = density_utilization_workspace_;
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

    double total_overflow_mass = 0.;
    double total_capacity = 0.;
    for (size_t layer = 0; layer < num_layers; layer++) {
        for (size_t x = 0; x < width; x++) {
            for (size_t y = 0; y < height; y++) {
                size_t idx = site_index(layer, x, y);
                for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
                    double capacity = cached_target_capacity_[dim_idx][idx];
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

std::vector<double> NonlinearNesterovPlacer::compute_physical_overflow_ratios_per_dim_(
    const PartialPlacement& p_placement,
    const std::vector<PrimitiveVectorDim>& dimensions) const {
    std::vector<double> ratios(dimensions.size(), 0.);
    if (dimensions.empty())
        return ratios;

    size_t width = device_grid_width_;
    size_t height = device_grid_height_;
    size_t num_layers = std::max<size_t>(1, device_grid_num_layers_);
    size_t num_sites = width * height * num_layers;
    initialize_density_target_cache_(dimensions);
    auto site_index = [width, height](size_t layer, size_t x, size_t y) {
        return (layer * height + y) * width + x;
    };

    density_utilization_workspace_.resize(dimensions.size());
    for (std::vector<double>& dim_utilization : density_utilization_workspace_) {
        dim_utilization.resize(num_sites);
        std::fill(dim_utilization.begin(), dim_utilization.end(), 0.);
    }
    std::vector<std::vector<double>>& utilization = density_utilization_workspace_;
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

    for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
        double overflow_mass = 0.;
        double capacity_sum = 0.;
        for (size_t idx = 0; idx < num_sites; idx++) {
            double capacity = cached_target_capacity_[dim_idx][idx];
            capacity_sum += capacity;
            double overflow = utilization[dim_idx][idx] - capacity;
            if (overflow > 0.)
                overflow_mass += overflow;
        }
        ratios[dim_idx] = capacity_sum > kEpsilon ? overflow_mass / capacity_sum : 0.;
    }
    return ratios;
}

double NonlinearNesterovPlacer::add_proximity_gradient_(const PartialPlacement& p_placement,
                                                        const PartialPlacement& legal_anchor,
                                                        double proximity_weight,
                                                        std::optional<std::reference_wrapper<PlacementGradient>> grad) const {
    if (proximity_weight == 0.)
        return 0.;

    double proximity_penalty = 0.;
    for (APBlockId blk_id : moveable_blocks_) {
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
        for (APBlockId blk_id : moveable_blocks_) {
            double inv_precond = 1.0 / block_precond_[blk_id];
            next_placement.block_x_locs[blk_id] = y_placement.block_x_locs[blk_id] - step_size * grad.dx[blk_id] * inv_precond;
            next_placement.block_y_locs[blk_id] = y_placement.block_y_locs[blk_id] - step_size * grad.dy[blk_id] * inv_precond;
        }
    } else {
        for (APBlockId blk_id : moveable_blocks_) {
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
    for (APBlockId blk_id : moveable_blocks_) {
        y_placement.block_x_locs[blk_id] = next.block_x_locs[blk_id] + beta * (next.block_x_locs[blk_id] - current.block_x_locs[blk_id]);
        y_placement.block_y_locs[blk_id] = next.block_y_locs[blk_id] + beta * (next.block_y_locs[blk_id] - current.block_y_locs[blk_id]);
    }
    // FISTA extrapolation can overshoot the device rectangle; keep the look-ahead
    // point within the same device bounds the objective is evaluated over before
    // evaluating gradients.
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
    for (APBlockId blk_id : moveable_blocks_) {
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
    for (APBlockId blk_id : moveable_blocks_) {
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
