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
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <random>
#include <set>
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
#include "logic_types.h"
#include "partial_placement.h"
#include "physical_types.h"
#include "place_delay_model.h"
#include "preconditioner_math.h"
#include "prepack.h"
#include "primitive_dim_manager.h"
#include "primitive_vector.h"
#include "timing_info.h"
#include "vtr_assert.h"
#include "vtr_thread_pool.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace {
// --------------------------------------------------------------------------
// Iteration and epoch budget
// --------------------------------------------------------------------------

/**
 * @brief Maximum number of accelerated first-order iterations.
 *
 * Split evenly across @ref kNesterovEpochs epochs. Keep synchronized with the
 * configuration line emitted by optimize_from_seed_ so run metadata identifies
 * the policy in use.
 */
constexpr size_t kMaxNesterovIterations = 400;

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

// --------------------------------------------------------------------------
// Step control and convergence
// --------------------------------------------------------------------------

/**
 * @brief Lower clamp bound on the Barzilai-Borwein step size.
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
 * @brief Maximum per-iteration growth of the Barzilai-Borwein step.
 *
 * Divergence guard. A secant estimate can spike when successive preconditioned
 * gradients nearly coincide; capping growth bounds the damage to one iteration
 * without needing a function evaluation to detect it.
 */
constexpr double kBarzilaiBorweinGrowthCap = 2.0;

/**
 * @brief Maximum fraction of the device span a block should move in one step.
 *
 * Initial step for designs that take the raw-gradient path. The raw gradient does
 * not carry position units, so the step must be scaled by the device span. The
 * Barzilai-Borwein secant adapts from this starting point.
 */
constexpr double kInitialStepSpanFraction = 0.02;

/**
 * @brief Movable-block count at or above which the preconditioner is applied.
 */
constexpr size_t kPreconditionSizeThreshold = 30000;

// --------------------------------------------------------------------------
// Wirelength smoothing (gamma)
// --------------------------------------------------------------------------

/**
 * @brief Smooth wirelength gamma as a fraction of the larger device dimension.
 */
constexpr double kWirelengthGammaFraction = 0.02;

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

// --------------------------------------------------------------------------
// Density and field formulation
// --------------------------------------------------------------------------

/**
 * @brief Minimum target capacity used when normalizing electrostatic charge.
 *
 * FPGA resource capacities are often fractional after target-density and
 * footprint spreading. The density field should normalize by those fractional
 * capacities, not by one full block. This floor only prevents numerical spikes at
 * interpolation points that barely touch a legal site for a sparse resource.
 */
constexpr double kDensityTargetFloorFraction = 0.01;

// The preconditioner tuning constants and the diagonal assembly itself live in
// preconditioner_math.h, so the unit tests exercise the shipped formulas rather
// than a copy of them.
using vtr::ap::jacobi_precond_diagonal;
using vtr::ap::kPreconditionAlpha;
using vtr::ap::kPreconditionFloor;

/**
 * @brief Initial target ratio of density pressure to wirelength pressure.
 *
 * A small fixed linear density weight keeps the first Nesterov pass close to the
 * warm-start seed while still letting the electrostatic field relieve overlap.
 *
 * Scales an *energy* ratio: lambda_0 = ratio * W / D, so 0.05 means "the
 * density energy starts at 5% of the wirelength energy".
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

// --------------------------------------------------------------------------
// Warm start and seeding
// --------------------------------------------------------------------------

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

// --------------------------------------------------------------------------
// Dynamic fillers
// --------------------------------------------------------------------------

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

// --------------------------------------------------------------------------
// Net cohesion and affinity springs
// --------------------------------------------------------------------------

/**
 * @brief Long-chain pack-pattern affinity-spring weight for I/O-chain designs.
 *
 * Probing showed that ungated pack springs worsen general QoR, while gating the
 * 0.02 weight to designs with long direct I/O-chain nets is required to recover
 * the win on the designs that motivated it.
 */
constexpr double kPackPatternCohesionWeight = 0.02;

// --------------------------------------------------------------------------
// Numerics
// --------------------------------------------------------------------------

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

} // namespace

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
                                                 unsigned num_threads,
                                                 int log_verbosity)
    : GlobalPlacer(ap_netlist, log_verbosity)
    , atom_netlist_(atom_netlist)
    , pre_cluster_timing_manager_(pre_cluster_timing_manager)
    , place_delay_model_(place_delay_model)
    , models_(models)
    , net_weights_(ap_netlist.nets().size(), 1.0)
    , device_grid_width_(device_grid.width())
    , device_grid_height_(device_grid.height())
    , device_grid_num_layers_(device_grid.get_num_layers())
    , ap_timing_tradeoff_(ap_timing_tradeoff)
    , pack_pattern_cohesion_weight_(kPackPatternCohesionWeight) {

    vtr::ScopedStartFinishTimer nonlinear_nesterov_placer_building_timer("Constructing Nonlinear Nesterov Global Placer");

    prepacker_ = &prepacker;
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

    cohesion_ = std::make_unique<NetCohesion>(ap_netlist_,
                                              *density_manager_,
                                              device_grid_width_,
                                              device_grid_height_,
                                              device_grid_num_layers_,
                                              log_verbosity_);

    affinity_term_ = std::make_unique<AffinitySpringTerm>(ap_netlist_,
                                                          pack_pattern_cohesion_weight_);

    num_pack_pattern_affinity_groups_ = 0;
    initialize_pack_pattern_affinity_groups_(prepacker);

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
        for (const AffinityGroup& group : affinity_term_->groups())
            affinity_blocks += group.blocks.size();
        VTR_LOG("Nonlinear Nesterov adaptive policy: blocks=%zu pins/block=%.2f warm-start-floor=%zu timing=%g.\n",
                moveable_blocks_.size(),
                pins_per_moveable_block,
                warmstart_iters_,
                ap_timing_tradeoff_);

        VTR_LOG("Nonlinear Nesterov affinity springs: pack_groups=%zu weight=%g; blocks=%zu.\n",
                num_pack_pattern_affinity_groups_,
                pack_pattern_cohesion_weight_,
                affinity_blocks);
        VTR_LOG("Nonlinear Nesterov pin-density inflation: reference=%.2f pins/block max_inflation=%.3g.\n",
                pin_density_inflation_reference,
                max_pin_density_inflation);
    }

    // Independent per-resource field solves can use the configured AP worker
    // count. A null pool (the num_threads <= 1 default) keeps the historical
    // serial loop; per-dimension results are computed identically either way and
    // reduced in fixed dimension order, so thread count never changes the output.
    if (num_threads > 1) {
        field_thread_pool_ = std::make_unique<vtr::thread_pool>(num_threads);
        if (log_verbosity_ >= 1)
            VTR_LOG("Nonlinear Nesterov field solves: using %u worker threads.\n", num_threads);
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
                                               ap_timing_tradeoff_,
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
        // molecules into shared logic blocks, inflating routed wirelength. The
        // cure is to keep compacting with
        // more B2B solve+legalize cycles (what SimPL does
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
    cohesion_->identify_boundary_confined_dims(density_dimensions);

    double device_span = std::max<double>(device_grid_width_, device_grid_height_);
    double convergence_displacement = std::max(kMinConvergenceDisplacement,
                                               device_span * kConvergenceDisplacementFraction);

    // The preconditioner is always computed; this decides whether it is applied.
    // Applying it and using a unit step are one regime, not two knobs: a
    // preconditioned gradient carries position units so its natural step is ~1,
    // while a raw gradient needs the span-scaled step.
    //
    // ABLATION GATE (temporary): VPR_ABL_PRECOND=always preconditions every design.
    const char* abl_precond = std::getenv("VPR_ABL_PRECOND");
    bool precond_always = abl_precond && std::string(abl_precond) == "always";
    if (precond_always)
        VTR_LOG("ABL: PRECOND=always (size gate bypassed; unit step everywhere).\n");
    large_design_ = precond_always || moveable_blocks_.size() >= kPreconditionSizeThreshold;

    return run_global_optimization_(density_dimensions, device_span, convergence_displacement);
}

PartialPlacement NonlinearNesterovPlacer::run_global_optimization_(const std::vector<PrimitiveVectorDim>& density_dimensions,
                                                                   double device_span,
                                                                   double convergence_displacement) {
    vtr::Timer warmstart_timer;
    PartialPlacement seed = initialize_placement_();
    if (log_verbosity_ >= 1)
        VTR_LOG("Nonlinear Nesterov phase time: warm start took %.2f seconds.\n", warmstart_timer.elapsed_sec());
    cohesion_->update_periphery_pair_nets(density_dimensions);
    // ABLATION GATE (temporary): VPR_ABL_PACKPAT=off disables chain springs
    // entirely; =ungated keeps them on regardless of the periphery-net gate.
    const char* abl_packpat = std::getenv("VPR_ABL_PACKPAT");
    bool abl_off = abl_packpat && std::string(abl_packpat) == "off";
    bool abl_ungated = abl_packpat && std::string(abl_packpat) == "ungated";
    if (abl_off) {
        VTR_LOG("ABL: PACKPAT=off (chain affinity springs disabled).\n");
        pack_pattern_cohesion_weight_ = 0.;
        affinity_term_->set_pack_pattern_weight(0.);
    } else if (abl_ungated) {
        VTR_LOG("ABL: PACKPAT=ungated (periphery gate bypassed).\n");
    } else if (pack_pattern_cohesion_weight_ > 0.
        && cohesion_->num_periphery_pair_nets() == 0) {
        if (log_verbosity_ >= 1) {
            VTR_LOG("Nonlinear Nesterov pack-pattern affinity disabled: no two-pin periphery nets were found.\n");
        }
        pack_pattern_cohesion_weight_ = 0.;
        affinity_term_->set_pack_pattern_weight(0.);
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
    // legalizes the result, then increases the fixed density weight through a
    // short continuation schedule.
    PartialPlacement current(ap_netlist_);
    current = seed;

    vtr::Timer epoch_phase_timer;
    double legalizer_time_sec = 0.;

    // Sparse-seed guard: the seed already satisfies the density stop target, so
    // the full filler/epoch schedule can only waste runtime (its result loses the
    // HPWL selection to the seed on these designs). Run a short filler-free
    // wirelength-refinement probe instead; the seed-vs-epoch selection below
    // still protects quality either way.
    //
    // Under the closed loop the epoch count is a *ceiling* the overflow stop cuts
    // short, and each epoch gets a fixed iteration slice rather than a share of a
    // fixed total -- dividing a fixed budget is what pinned lambda's total growth
    // to kFinalDensityWeightMultiplier regardless of the placement's actual legality.
    size_t num_epochs = sparse_seed_ ? kSparseSeedMaxEpochs : kNesterovEpochs;
    size_t iterations_per_epoch = sparse_seed_
                                      ? kSparseSeedProbeIterations
                                      : (kMaxNesterovIterations + num_epochs - 1) / num_epochs;
    // Equivalent to the previous `num_epochs == kNesterovEpochs` test (that was
    // exactly "not sparse-seed capped"), but stated directly so it keeps holding
    // now that a non-sparse run can have an epoch count other than kNesterovEpochs.
    const size_t min_epochs_before_overflow_stop = sparse_seed_
                                                       ? num_epochs
                                                       : kMinEpochsBeforeOverflowStop;
    if (sparse_seed_ && log_verbosity_ >= 1) {
        VTR_LOG("Nonlinear Nesterov sparse-seed guard: capping electrostatic phase to %zu filler-free epoch(s) of %zu iterations.\n",
                num_epochs, iterations_per_epoch);
    }

    // --- Ablation gates (env, default OFF -> bit-identical). Load-bearing campaign. ---

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
        // lambda_0 is set from the *energy* ratio,
        // `ratio * WA_wirelength_value / density_energy`, and deliberately not
        // from ePlace/elfPlace's gradient-norm ratio ||grad W||_1 / ||grad D||_1.
        //
        // The energy form is in principle a feedback trap: as a placement
        // concentrates, WA wirelength collapses toward zero (coincident pins
        // have no span) while density energy blows up (all mass in few bins), so
        // lambda_0 -> 0 exactly when the spreading force is most needed. That is
        // why the run always starts from a spread seed rather than a cold one.
        //
        // The gradient form was implemented and is not adopted: switching the
        // units while holding kInitialDensityToWirelengthRatio at its
        // energy-tuned value moves lambda_0 by up to an order of magnitude
        // either way depending on the circuit, which is an uncontrolled level
        // change rather than a normalization fix. Any future attempt must first
        // recalibrate the ratio so the geomean lambda_0 is unchanged; only then
        // does a measurement attribute to the *shape* of the normalization.
        initial_density_weight = 1e-3;
        {
            ObjectiveValue components = evaluate_objective_(placement,
                                                            density_multipliers,
                                                            std::nullopt,
                                                            current_fillers,
                                                            std::nullopt);
            if (components.density > kEpsilon)
                initial_density_weight = kInitialDensityToWirelengthRatio * std::max(components.wirelength, 1.0) / components.density;
        }

        initial_density_weight = std::clamp(initial_density_weight, 1e-5, 1e3);
        for (size_t dim_idx = 0; dim_idx < density_dimensions.size(); dim_idx++) {
            density_multipliers[dim_idx] = initial_density_weight;
        }
    };
    // The B2B wirelength model must exist before the initial density-weight
    // normalization evaluates the objective (an empty model would read
    // wirelength = 0 and mis-normalize lambda_0). The epoch loop relinearizes
    // it at each epoch start.
    reset_density_weights(current);

    /// Count of FISTA adaptive restarts, reported at the end of the run.
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
        VTR_LOG("Epoch  Pre HPWL  Post HPWL  Pre Oflow  Post Oflow  Pre Max  Post Max  Mean Move  Max Move  Density Wt\n");
        VTR_LOG("-----  --------  ---------  ---------  ----------  -------  --------  ---------  --------  ----------\n");
    }

    std::vector<PartialPlacement> checkpoints;
    std::vector<double> checkpoint_hpwls;
    std::vector<int> checkpoint_sources;
    checkpoints.push_back(seed);
    checkpoint_hpwls.push_back(seed.get_hpwl(ap_netlist_));
    checkpoint_sources.push_back(-1); // -1 = warm-start seed, otherwise epoch index.
    VTR_ASSERT(checkpoints.size() == checkpoint_hpwls.size());
    VTR_ASSERT(checkpoints.size() == checkpoint_sources.size());
    PartialPlacement y_placement(ap_netlist_);
    PartialPlacement next(ap_netlist_);
    PartialPlacement before_legalization(ap_netlist_);
    FillerState y_fillers;
    FillerState next_fillers;
    VTR_LOG("Nonlinear Nesterov configuration: iterations=%zu epochs=%zu.\n",
            kMaxNesterovIterations,
            num_epochs);
    auto apply_continuation_schedule = [&](double schedule) {
        // Ablation: fix gamma to a constant mid value (removes the coarse->sharp anneal); density
        // weight still follows the real schedule.
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
    // The step size is in preconditioned (near-Newton) units: the gradient is
    // divided by the Jacobi preconditioner diagonal h_xi (elfPlace Eq. 16) in
    // gradient_step_, so step_size=1.0 gives a near-Newton step of O(1) tiles.
    // This matches elfPlace's initial t(0) = 1/αH ≈ 0.94. Subsequent steps come
    // from the Barzilai-Borwein secant estimate. Step resets to 1.0 each epoch
    // (matching the original VTR nesterov) so a fresh density weight starts
    // from a conservative step.
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
        // Timing net weights were just refreshed against the epoch-start placement
        // (post-legalization from the prior epoch). The weighted-average wirelength
        // gradient below is evaluated directly each iteration, so nothing is
        // relinearized here -- B2B is used only for the warm-start seed.
        if (sparse_seed_) {
            for (size_t dim_idx = 0; dim_idx < density_multipliers.size(); dim_idx++) {
                density_multipliers[dim_idx] = initial_density_weight;
            }
        } else {
            double schedule = num_epochs > 1
                                  ? static_cast<double>(epoch) / static_cast<double>(num_epochs - 1)
                                  : 0.;
            apply_continuation_schedule(schedule);
        }
        for (size_t dim_idx = 0; dim_idx < density_multipliers.size(); dim_idx++)
            density_multipliers[dim_idx] *= adaptive_density_boosts[dim_idx];
        compute_preconditioner_(density_dimensions, density_multipliers);

        y_placement = current;
        next = current;
        y_fillers = current_fillers;
        next_fillers = current_fillers;
        PlacementGradient grad(ap_netlist_);
        FillerGradient filler_grad;
        // A preconditioned gradient carries position units (a near-Newton step),
        // so its natural step length is ~1; the raw gradient instead needs a
        // span-scaled step. The Barzilai-Borwein secant adapts from either.
        double step_size = large_design_
                               ? 1.0
                               : std::max(0.1, device_span * kInitialStepSpanFraction);
        double nesterov_t = 1.0;
        // Objective-inert telemetry. Each accepted iteration costs one gradient
        // evaluation at the look-ahead point, and every gradient evaluation carries
        // a Poisson solve per resource -- that solve is the dominant per-iteration
        // cost worth making cheaper.
        size_t num_grad_evals = 0;
        size_t num_accepted_iters = 0;
        size_t num_nonfinite_observations = 0;
        size_t num_bb_secant_updates = 0;
        size_t epoch_restart_count_begin = num_objective_restarts;
        const char* convergence_stop_reason = "iteration-budget";
        double min_raw_bb_step = std::numeric_limits<double>::infinity();
        double max_raw_bb_step = 0.;
        double min_accepted_step = std::numeric_limits<double>::infinity();
        double max_accepted_step = 0.;
        double max_lookahead_displacement = 0.;
        double max_iterate_displacement = 0.;
        double final_projected_gradient_max = std::numeric_limits<double>::quiet_NaN();
        // Nesterov accelerated-gradient inner solve. The gradient is taken at the
        // extrapolated look-ahead point y_placement and the step is a Barzilai-Borwein
        // secant estimate, accepted without an objective descent test. The scheme is
        // deliberately non-monotone; it is safeguarded by the O'Donoghue gradient
        // restart below, not by a line search.
        size_t this_epoch_iters = iterations_per_epoch;
        // Barzilai-Borwein state. The secant estimate is taken in the
        // preconditioned metric, because that is the space the step is actually
        // applied in (gradient_step_ divides by the Jacobi diagonal), so a raw
        // gradient difference would estimate the wrong curvature.
        PlacementGradient prev_precond_grad(ap_netlist_);
        PartialPlacement prev_y(ap_netlist_);
        PlacementGradient precond_grad(ap_netlist_);
        std::vector<std::vector<double>> precond_filler_dx, precond_filler_dy;
        std::vector<std::vector<double>> prev_precond_filler_dx, prev_precond_filler_dy;
        FillerState prev_y_fillers;
        bool have_prev_bb_state = false;

        for (size_t iter = 0; iter < this_epoch_iters; iter++) {
            num_grad_evals++;
            ObjectiveValue y_obj = evaluate_objective_(y_placement,
                                                       density_multipliers,
                                                       std::ref(grad),
                                                       y_fillers,
                                                       std::ref(filler_grad));
            double grad_norm_sq = gradient_norm_squared_(grad) + filler_gradient_norm_squared_(filler_grad);
            if (!std::isfinite(y_obj.total) || !std::isfinite(grad_norm_sq)) {
                // The BB step accepts unconditionally, so without this a
                // non-finite look-ahead gradient would be turned straight into
                // an accepted step; project_placement_ would not catch it,
                // since std::clamp(NaN, lo, hi) is NaN.
                num_nonfinite_observations++;
                convergence_stop_reason = "nonfinite";
                break;
            }
            if (grad_norm_sq < kEpsilon) {
                convergence_stop_reason = "zero-gradient";
                break;
            }

            double accepted_step = step_size;
            bool accepted = false;
            bool gradient_restart = false;
            {
                // Preconditioned gradient: the space gradient_step_ actually
                // moves in, and therefore the space the secant estimate must be
                // taken in.
                precond_grad.clear();
                bool use_precond = large_design_ && !block_precond_.empty();
                for (APBlockId blk_id : moveable_blocks_) {
                    double inv_precond = use_precond ? 1.0 / block_precond_[blk_id] : 1.0;
                    precond_grad.dx[blk_id] = grad.dx[blk_id] * inv_precond;
                    precond_grad.dy[blk_id] = grad.dy[blk_id] * inv_precond;
                }
                precond_filler_dx.assign(filler_grad.dx.size(), {});
                precond_filler_dy.assign(filler_grad.dy.size(), {});
                for (size_t dim_idx = 0; dim_idx < filler_grad.dx.size(); dim_idx++) {
                    double inv_precond = (dim_idx < filler_precond_.size() && filler_precond_[dim_idx] > 0.)
                                             ? 1.0 / filler_precond_[dim_idx]
                                             : 1.0;
                    precond_filler_dx[dim_idx].resize(filler_grad.dx[dim_idx].size());
                    precond_filler_dy[dim_idx].resize(filler_grad.dy[dim_idx].size());
                    for (size_t filler_idx = 0; filler_idx < filler_grad.dx[dim_idx].size(); filler_idx++) {
                        precond_filler_dx[dim_idx][filler_idx] = filler_grad.dx[dim_idx][filler_idx] * inv_precond;
                        precond_filler_dy[dim_idx][filler_idx] = filler_grad.dy[dim_idx][filler_idx] * inv_precond;
                    }
                }

                if (have_prev_bb_state) {
                    // t = ||dy|| / ||dg||, the inverse-Lipschitz (secant) estimate.
                    double dy_norm_sq = 0., dg_norm_sq = 0.;
                    for (APBlockId blk_id : moveable_blocks_) {
                        double sx = y_placement.block_x_locs[blk_id] - prev_y.block_x_locs[blk_id];
                        double sy = y_placement.block_y_locs[blk_id] - prev_y.block_y_locs[blk_id];
                        double gx = precond_grad.dx[blk_id] - prev_precond_grad.dx[blk_id];
                        double gy = precond_grad.dy[blk_id] - prev_precond_grad.dy[blk_id];
                        dy_norm_sq += sx * sx + sy * sy;
                        dg_norm_sq += gx * gx + gy * gy;
                    }
                    for (size_t dim_idx = 0; dim_idx < precond_filler_dx.size() && dim_idx < prev_precond_filler_dx.size(); dim_idx++) {
                        size_t n = std::min(precond_filler_dx[dim_idx].size(), prev_precond_filler_dx[dim_idx].size());
                        for (size_t filler_idx = 0; filler_idx < n; filler_idx++) {
                            double sx = y_fillers.x[dim_idx][filler_idx] - prev_y_fillers.x[dim_idx][filler_idx];
                            double sy = y_fillers.y[dim_idx][filler_idx] - prev_y_fillers.y[dim_idx][filler_idx];
                            double gx = precond_filler_dx[dim_idx][filler_idx] - prev_precond_filler_dx[dim_idx][filler_idx];
                            double gy = precond_filler_dy[dim_idx][filler_idx] - prev_precond_filler_dy[dim_idx][filler_idx];
                            dy_norm_sq += sx * sx + sy * sy;
                            dg_norm_sq += gx * gx + gy * gy;
                        }
                    }
                    if (dy_norm_sq > kEpsilon && dg_norm_sq > kEpsilon) {
                        double bb_step = std::sqrt(dy_norm_sq / dg_norm_sq);
                        if (!std::isfinite(bb_step))
                            num_nonfinite_observations++;
                        num_bb_secant_updates++;
                        min_raw_bb_step = std::min(min_raw_bb_step, bb_step);
                        max_raw_bb_step = std::max(max_raw_bb_step, bb_step);
                        // Growth clamp: bounds how fast the secant estimate can
                        // expand between iterations.
                        accepted_step = std::clamp(std::min(bb_step, step_size * kBarzilaiBorweinGrowthCap),
                                                   kMinStepSize,
                                                   device_span);
                    }
                }

                gradient_step_(y_placement, grad, y_fillers, filler_grad, accepted_step, next, next_fillers);
                accepted = true;

                // O'Donoghue & Candes gradient restart: <g(y_k), x_{k+1} - x_k> > 0
                // means the accelerated step is heading uphill. Uses the gradient
                // already in hand, so it replaces the objective-value restart test
                // at zero extra cost.
                double restart_dot = 0.;
                for (APBlockId blk_id : moveable_blocks_) {
                    restart_dot += grad.dx[blk_id] * (next.block_x_locs[blk_id] - current.block_x_locs[blk_id])
                                   + grad.dy[blk_id] * (next.block_y_locs[blk_id] - current.block_y_locs[blk_id]);
                }
                gradient_restart = restart_dot > 0.;

                prev_y = y_placement;
                prev_y_fillers = y_fillers;
                prev_precond_grad = precond_grad;
                prev_precond_filler_dx = precond_filler_dx;
                prev_precond_filler_dy = precond_filler_dy;
                have_prev_bb_state = true;
            }

            if (!accepted) {
                convergence_stop_reason = "no-accepted-step";
                break;
            }

            double max_step_displacement = std::max(max_block_displacement_(y_placement, next),
                                                    max_filler_displacement_(y_fillers, next_fillers));
            max_lookahead_displacement = std::max(max_lookahead_displacement, max_step_displacement);
            min_accepted_step = std::min(min_accepted_step, accepted_step);
            max_accepted_step = std::max(max_accepted_step, accepted_step);
            if (!std::isfinite(accepted_step) || !std::isfinite(max_step_displacement))
                num_nonfinite_observations++;

            // Diagnostic projected-gradient mapping at the look-ahead point:
            // max ||y - project(y - step * P^-1 g)|| / step. The numerator is
            // the displacement already computed for the existing convergence
            // test, so this adds no objective/gradient evaluation or extra scan.
            if (accepted_step > 0.)
                final_projected_gradient_max = max_step_displacement / accepted_step;
            double max_current_to_next_displacement = std::max(max_block_displacement_(current, next),
                                                               max_filler_displacement_(current_fillers, next_fillers));
            max_iterate_displacement = std::max(max_iterate_displacement, max_current_to_next_displacement);
            if (!std::isfinite(final_projected_gradient_max)
                || !std::isfinite(max_current_to_next_displacement)) {
                num_nonfinite_observations++;
            }

            // FISTA momentum: the t-sequence sets the extrapolation weight beta.
            double next_t = 0.5 * (1.0 + std::sqrt(1.0 + 4.0 * nesterov_t * nesterov_t));
            double beta = (nesterov_t - 1.0) / next_t;

            // Adaptive restart drops momentum after an uphill accelerated step.
            // The accepted point is retained so the short epoch keeps progressing.
            bool objective_restart = gradient_restart;
            if (objective_restart)
                num_objective_restarts++;

            if (objective_restart) {
                nesterov_t = 1.0;
                y_placement = next;
                y_fillers = next_fillers;
            } else {
                // Extrapolate the look-ahead point ahead of the accepted step.
                extrapolate_(current, next, current_fillers, next_fillers, beta, y_placement, y_fillers);
                nesterov_t = next_t;
            }

            current = next;
            current_fillers = next_fillers;
            num_accepted_iters++;

            // The secant estimate sets the next step directly.
            step_size = accepted_step;

            if (iter + 1 >= kMinNesterovIterationsPerEpoch
                && max_step_displacement <= convergence_displacement) {
                convergence_stop_reason = "displacement";
                break;
            }
        }

        VTR_LOG("  Nesterov convergence (epoch %zu): stop=%s accepted=%zu/%zu grad_evals=%zu restarts=%zu bb_updates=%zu raw_bb_step=[%.6g,%.6g] accepted_step=[%.6g,%.6g] max_y_step=%.6g max_x_step=%.6g projected_grad_max=%.6g nonfinite=%zu\n",
                epoch,
                convergence_stop_reason,
                num_accepted_iters,
                this_epoch_iters,
                num_grad_evals,
                num_objective_restarts - epoch_restart_count_begin,
                num_bb_secant_updates,
                num_bb_secant_updates == 0 ? 0. : min_raw_bb_step,
                num_bb_secant_updates == 0 ? 0. : max_raw_bb_step,
                num_accepted_iters == 0 ? 0. : min_accepted_step,
                num_accepted_iters == 0 ? 0. : max_accepted_step,
                max_lookahead_displacement,
                max_iterate_displacement,
                num_accepted_iters == 0 ? 0. : final_projected_gradient_max,
                num_nonfinite_observations);

        ObjectiveValue pre_legalization = evaluate_objective_(current,
                                                              density_multipliers,
                                                              std::nullopt,
                                                              current_fillers,
                                                              std::nullopt);
        // Partially legalize the smooth result: this cleans up overlap and produces
        // both the next epoch's starting placement and the checkpoint that ships.
        before_legalization = current;
        if (log_verbosity_ >= 1) {
            VTR_LOG("  Nesterov epoch %zu cost: %zu accepted iters, %zu gradient evals (%.2f per iter)\n",
                    epoch, num_accepted_iters, num_grad_evals,
                    num_accepted_iters ? static_cast<double>(num_grad_evals) / num_accepted_iters : 0.);
        }
        double pre_leg_overflow = compute_physical_overflow_ratio_(before_legalization, density_dimensions);
        density_manager_->import_placement_into_bins(before_legalization);
        size_t pre_leg_overfilled_bins = density_manager_->get_overfilled_bins().size();
        vtr::Timer legalizer_timer;
        partial_legalizer_->legalize(current);
        legalizer_time_sec += legalizer_timer.elapsed_sec();
        ObjectiveValue post_legalization = evaluate_objective_(current,
                                                               density_multipliers,
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

        double post_legalization_hpwl = current.get_hpwl(ap_netlist_);
        checkpoints.push_back(current);
        checkpoint_hpwls.push_back(post_legalization_hpwl);
        checkpoint_sources.push_back(static_cast<int>(epoch));
        VTR_ASSERT(checkpoints.size() == checkpoint_hpwls.size());
        VTR_ASSERT(checkpoints.size() == checkpoint_sources.size());
        VTR_LOG("%5zu  %8.2f  %9.2f  %9.4f  %10.4f  %7.4f  %8.4f  %9.4f  %8.4f  %10.4g\n",
                epoch,
                before_legalization.get_hpwl(ap_netlist_),
                post_legalization_hpwl,
                pre_legalization.total_overflow,
                post_legalization.total_overflow,
                pre_legalization.max_overflow,
                post_legalization.max_overflow,
                mean_displacement,
                max_displacement,
                density_multipliers.empty() ? 0. : density_multipliers.front());

        std::vector<double> phys_oflows = compute_physical_overflow_ratios_per_dim_(before_legalization, density_dimensions);
        VTR_LOG("  Nesterov density dims (epoch %zu): pre_overfilled_bins=%zu mean_pl_disp=%.4f pre_leg_overflow=%.4f\n",
                epoch, pre_leg_overfilled_bins, mean_displacement, pre_leg_overflow);
        for (size_t dim_idx = 0; dim_idx < density_dimensions.size(); dim_idx++) {
            VTR_LOG("    dim=%-20s mult=%.4g densE=%.4g oflow=%.4f mass=%.4g max=%.4f phys=%.4f boost=%.3g\n",
                    dim_manager.get_dim_name(density_dimensions[dim_idx]).c_str(),
                    density_multipliers[dim_idx],
                    pre_legalization.density_energies[dim_idx],
                    pre_legalization.dim_overflow_ratios[dim_idx],
                    pre_legalization.dim_overflow_mass[dim_idx],
                    pre_legalization.dim_max_overflow[dim_idx],
                    phys_oflows[dim_idx],
                    adaptive_density_boosts[dim_idx]);
        }
        for (size_t dim_idx = 0; dim_idx < density_dimensions.size(); dim_idx++) {
            const std::string& dim_name = dim_manager.get_dim_name(density_dimensions[dim_idx]);
            if (!dim_allows_adaptive_density_boost(dim_name))
                continue;
            double phys_oflow = phys_oflows[dim_idx];
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

    // Checkpoint selection is pure minimum-HPWL.
    VTR_ASSERT(!checkpoints.empty());
    VTR_ASSERT(checkpoints.size() == checkpoint_hpwls.size());
    VTR_ASSERT(checkpoints.size() == checkpoint_sources.size());
    size_t best_checkpoint_idx = std::distance(checkpoint_hpwls.begin(),
                                               std::min_element(checkpoint_hpwls.begin(), checkpoint_hpwls.end()));
    VTR_LOG("Nonlinear Nesterov: selecting min-HPWL checkpoint.\n");

    for (size_t checkpoint_idx = 0; checkpoint_idx < checkpoints.size(); checkpoint_idx++) {
        const char* source_name = checkpoint_sources[checkpoint_idx] < 0 ? "seed" : "epoch";
        VTR_LOG("Nonlinear Nesterov checkpoint: source=%s index=%d hpwl=%g selected=%s.\n",
                source_name,
                checkpoint_sources[checkpoint_idx],
                checkpoint_hpwls[checkpoint_idx],
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
        VTR_LOG("Nonlinear Nesterov phase time: epoch loop took %.2f seconds (partial legalization %.2f).\n",
                epoch_phase_timer.elapsed_sec(), legalizer_time_sec);
    }
    partial_legalizer_->print_statistics();

    return checkpoints[best_checkpoint_idx];
}

NonlinearNesterovPlacer::ObjectiveValue NonlinearNesterovPlacer::evaluate_objective_(const PartialPlacement& p_placement,
                                                                                     const std::vector<double>& density_multipliers,
                                                                                     std::optional<std::reference_wrapper<PlacementGradient>> grad,
                                                                                     const FillerState& fillers,
                                                                                     std::optional<std::reference_wrapper<FillerGradient>> filler_grad) const {
    if (grad)
        grad->get().clear();

    ObjectiveValue value;
    value.wirelength = add_wirelength_gradient_(p_placement, grad);
    add_density_gradient_(p_placement, density_multipliers, value, grad, fillers, filler_grad);
    value.affinity_spring = affinity_term_->evaluate(p_placement, grad);
    value.total = value.wirelength
                  + value.affinity_spring;
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
    bool use_timing_weights = ap_timing_tradeoff_ != 0.f && pre_cluster_timing_manager_.is_valid();

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
            // Interpolate between unit weight and net criticality.
            weight = ap_timing_tradeoff_ * crit + (1.0 - ap_timing_tradeoff_);
        }

        net_weights_[net_id] = weight;

        total_weight += weight;
        min_weight = std::min(min_weight, weight);
        max_weight = std::max(max_weight, weight);
        weighted_nets++;
    }

    // Telemetry only. Rebalancing the density weight by this was tried and made
    // no measurable difference, though note lambda_0 is normalized against
    // unit-weight wirelength while the epoch loop applies multipliers well
    // above 1, so the wirelength/density balance does carry a design-dependent
    // factor.
    double avg_net_weight = weighted_nets > 0 ? total_weight / weighted_nets : 1.0;
    if (log_verbosity_ >= 1 && weighted_nets > 0) {
        VTR_LOG("Nonlinear Nesterov timing net weights: tradeoff=%g min=%g avg=%g max=%g nets=%zu\n",
                ap_timing_tradeoff_,
                min_weight,
                avg_net_weight,
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
        affinity.blocks = std::move(group);
        affinity_term_->add_group(std::move(affinity));
        num_pack_pattern_affinity_groups_++;
    }
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
    cached_charge_scale_.assign(dimensions.size(), 1.);

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
            cached_charge_scale_[dim_idx] = std::max(kEpsilon, target_sum / static_cast<double>(target_sites));
        }
    }
}

NonlinearNesterovPlacer::GridPosition NonlinearNesterovPlacer::clamp_to_grid_(double x, double y, double layer) const {
    return {std::clamp(x, 0., device_grid_width_ - kDeviceBoundaryEpsilon),
            std::clamp(y, 0., device_grid_height_ - kDeviceBoundaryEpsilon),
            static_cast<size_t>(std::clamp(std::round(layer), 0., static_cast<double>(device_grid_num_layers_ - 1)))};
}

void NonlinearNesterovPlacer::add_density_gradient_(const PartialPlacement& p_placement,
                                                    const std::vector<double>& density_multipliers,
                                                    ObjectiveValue& value,
                                                    std::optional<std::reference_wrapper<PlacementGradient>> grad,
                                                    const FillerState& fillers,
                                                    std::optional<std::reference_wrapper<FillerGradient>> filler_grad) const {
    std::vector<double>& density_energies = value.density_energies;
    std::vector<double>& dim_overflow_ratios = value.dim_overflow_ratios;
    std::vector<double>& dim_overflow_mass = value.dim_overflow_mass;
    std::vector<double>& dim_max_overflow = value.dim_max_overflow;
    double& total_overflow = value.total_overflow;
    double& max_overflow = value.max_overflow;
    total_overflow = 0.;
    max_overflow = 0.;
    // ePlace legality signal: overflow mass (mass exceeding tile target) over total
    // deposited mass, tracked per dimension in dim_overflow_ratios. Normalizing by
    // movable mass (not total grid capacity) avoids diluting local overfill on
    // designs that occupy a small fraction of the grid.

    std::vector<PrimitiveVectorDim> dimensions = density_manager_->get_used_dims_mask().get_non_zero_dims();
    if (dimensions.empty())
        return;
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
    reset_grid_workspace(density_utilization_workspace_);
    std::vector<std::vector<double>>& utilization = density_utilization_workspace_;

    // The field carries filler charge that overflow accounting must not see, so
    // it needs its own utilization array even though it shares the tile grid.
    // The potential only needs a size check, not a zero-fill: every dimension in
    // `dimensions` comes from get_used_dims_mask().get_non_zero_dims(), so
    // active_bins > 0 below and the Poisson write-back overwrites every site
    // before potential is read.
    density_field_utilization_workspace_.resize(dimensions.size());
    density_potential_workspace_.resize(dimensions.size());
    for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
        density_field_utilization_workspace_[dim_idx].assign(num_sites, 0.);
        if (density_potential_workspace_[dim_idx].size() != num_sites)
            density_potential_workspace_[dim_idx].assign(num_sites, 0.);
    }
    std::vector<std::vector<double>>& field_utilization = density_field_utilization_workspace_;
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
    //
    //  - cached_charge_scale_: expresses (utilization - target) in units of
    //    "typical capacity for this resource" rather than raw mass, so resource
    //    dimensions with very different natural magnitudes on a heterogeneous
    //    grid contribute comparably scaled charge.
    const std::vector<std::vector<double>>& target_capacity = cached_target_capacity_;
    const std::vector<double>& target_norm_floor = cached_target_norm_floor_;

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

        auto [x, y, layer] = clamp_to_grid_(p_placement.block_x_locs[blk_id], p_placement.block_y_locs[blk_id], p_placement.block_layer_nums[blk_id]);
        BilinearDensityStencil stencil = make_bilinear_density_stencil(x, y, width, height);

        // Traverses dimensions, i.e. resource types of the mass abstraction
        for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
            double mass = block_mass.get_dim_val(dimensions[dim_idx]);
            if (mass == 0.)
                continue;
            // Overflow/legality accounting, and the charge that shapes the field.
            deposit_bilinear_density(utilization[dim_idx], layer, width, height, stencil, mass);
            deposit_bilinear_density(field_utilization[dim_idx], layer, width, height, stencil, mass);
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
    }

    // Deposit dynamic fillers into the electrostatic field. They carry density
    // charge but are deliberately excluded from overflow accounting above, so
    // they only reach the field grid.
    for (size_t dim_idx = 0; dim_idx < dimensions.size() && dim_idx < fillers.x.size(); dim_idx++) {
        double unit_mass = dim_idx < filler_unit_mass_.size() ? filler_unit_mass_[dim_idx] : 0.;
        if (unit_mass <= 0.)
            continue;
        size_t n = fillers.x[dim_idx].size();
        for (size_t filler_idx = 0; filler_idx < n; filler_idx++) {
            auto [x, y, layer] = clamp_to_grid_(fillers.x[dim_idx][filler_idx],
                                                fillers.y[dim_idx][filler_idx],
                                                fillers.layer[dim_idx][filler_idx]);
            BilinearDensityStencil stencil = make_bilinear_density_stencil(x, y, width, height);
            deposit_bilinear_density(field_utilization[dim_idx], layer, width, height, stencil, unit_mass);
        }
    }

    // Electrostatic density model (ePlace/elfPlace), solved independently per
    // resource dimension on that resource's own field domain. Treat the excess
    // block density in each field bin as electric charge; solving Poisson's
    // equation gives a potential whose negative gradient is a force that pushes
    // blocks from dense to sparse regions. The residual-charge mode uses
    // (utilization - target) in area units, normalized by the average bin
    // capacity for this resource. This avoids over-amplifying fractional-capacity
    // bins on heterogeneous FPGA grids.
    double density_energy = 0.;
    density_charge_workspace_.resize(dimensions.size());
    density_layer_charge_workspace_.resize(dimensions.size());
    density_layer_potential_workspace_.resize(dimensions.size());
    // One resource dimension's charge -> Poisson solve -> potential/energy. The
    // body touches only dim_idx-owned state (its own charge/layer workspaces,
    // potential[dim_idx], density_energies[dim_idx]) plus read-only shared
    // inputs, so the calls are independent across dimensions.
    auto solve_dim_field = [&](size_t dim_idx) {
        const std::vector<double>& dim_target_capacity = target_capacity[dim_idx];
        double charge_scale = cached_charge_scale_[dim_idx];
        auto site_index = [width, height](size_t layer, size_t x, size_t y) {
            return (layer * height + y) * width + x;
        };

        if (density_charge_workspace_[dim_idx].size() != num_sites)
            density_charge_workspace_[dim_idx].resize(num_sites);
        std::fill(density_charge_workspace_[dim_idx].begin(), density_charge_workspace_[dim_idx].end(), 0.);
        std::vector<double>& charge = density_charge_workspace_[dim_idx];
        size_t active_bins = 0;

        for (size_t idx = 0; idx < num_sites; idx++) {
            double target = dim_target_capacity[idx];
            double utilization_at_bin = field_utilization[dim_idx][idx];
            bool has_capacity = target > kEpsilon;
            if (!has_capacity) {
                // A block in a bin without capacity for its primitive type is
                // an overfill source, not an empty destination.
                if (utilization_at_bin <= kEpsilon)
                    continue;
                charge[idx] = utilization_at_bin / charge_scale;
            } else {
                // Residual charge, (utilization - target) / average_target, follows
                // ePlace: object area and target area are balanced in the same units
                // with only a per-resource normalization, rather than a relative
                // utilization/target - 1 charge that over-amplifies
                // fractional-capacity sites on heterogeneous devices.
                charge[idx] = (utilization_at_bin - target) / charge_scale;
            }
            active_bins++;
        }

        if (active_bins == 0)
            return;

        // Rebalance charge over the fixed capacity-bin mask. Bilinear deposition
        // conserves total mass, so both the mask and the subtracted amount are
        // placement-invariant and contribute no missing derivative. On a domain
        // that still has capacity holes this also acts as an
        // architecture-attraction field; it is not merely the uniform DC
        // projection already performed by the Poisson solve.
        rebalance_density_charge_on_capacity_sites(charge, dim_target_capacity, kEpsilon);

        density_layer_charge_workspace_[dim_idx].resize(width * height);
        std::vector<double>& layer_charge = density_layer_charge_workspace_[dim_idx];
        std::vector<double>& layer_potential = density_layer_potential_workspace_[dim_idx];
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

        // Charge is normalized per tile, so the energy is the plain quadratic
        // form of this resource's field.
        for (size_t idx = 0; idx < num_sites; idx++)
            density_energies[dim_idx] += 0.5 * charge[idx] * potential[dim_idx][idx];
    };

    if (field_thread_pool_ != nullptr && dimensions.size() > 1) {
        for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++)
            field_thread_pool_->schedule_work([&solve_dim_field, dim_idx]() { solve_dim_field(dim_idx); });
        field_thread_pool_->wait_for_all();
    } else {
        for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++)
            solve_dim_field(dim_idx);
    }

    // Final energy reduction in fixed dimension order, outside the parallel
    // region, so the sum is deterministic and independent of thread count.
    for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++)
        density_energy += density_energies[dim_idx];

    value.density = density_energy;
    if (!grad)
        return;

    // Turn the grid field into a block gradient
    for (APBlockId blk_id : moveable_blocks_) {
        PrimitiveVector block_mass = density_manager_->mass_calculator().get_block_mass(blk_id);
        if (block_mass.is_zero())
            continue;
        // Same pin-density inflation as the deposition pass above, so the force
        // extracted here matches the (inflated) mass that shaped the field.
        block_mass *= pin_density_inflation_[blk_id];

        auto [x, y, layer] = clamp_to_grid_(p_placement.block_x_locs[blk_id], p_placement.block_y_locs[blk_id], p_placement.block_layer_nums[blk_id]);
        // Accumulate density-only force so along-rim damping does not touch WL.
        double density_dx = 0.;
        double density_dy = 0.;
        for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
            double mass = block_mass.get_dim_val(dimensions[dim_idx]);
            if (mass == 0.)
                continue;
            // Same packing-aware deflation as the deposition pass, so the
            // force extracted matches the mass that shaped the field.

            BilinearDensityStencil stencil = make_bilinear_density_stencil(x, y, width, height);
            double local_field_x = 0.;
            double local_field_y = 0.;
            {
                // For E = 0.5*q^T*A*q with symmetric Poisson inverse A, the
                // deposition-consistent derivative is m*sum_i(dw_i/dx)*Phi_i.
                // Differentiating the bilinear potential directly is essential;
                // interpolating grid-point finite differences is a different
                // discretization. Degenerate cells correctly produce zero.
                std::pair<double, double> local_field = gradient_bilinear_density(potential[dim_idx], layer, width, height, stencil);
                local_field_x = local_field.first;
                local_field_y = local_field.second;
            }

            double normalized_mass = mass / cached_charge_scale_[dim_idx];
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
            double normalized_mass = unit_mass / cached_charge_scale_[dim_idx];
            for (size_t filler_idx = 0; filler_idx < n; filler_idx++) {
                auto [x, y, layer] = clamp_to_grid_(fillers.x[dim_idx][filler_idx],
                                                    fillers.y[dim_idx][filler_idx],
                                                    fillers.layer[dim_idx][filler_idx]);
                BilinearDensityStencil stencil = make_bilinear_density_stencil(x, y, width, height);
                // Same deposition-consistent derivative as the block gradient.
                auto [local_field_x, local_field_y] = gradient_bilinear_density(potential[dim_idx], layer, width, height, stencil);
                filler_grad->get().dx[dim_idx][filler_idx] = coefficient * normalized_mass * local_field_x;
                filler_grad->get().dy[dim_idx][filler_idx] = coefficient * normalized_mass * local_field_y;
            }
        }
    }
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
        auto [x, y, layer] = clamp_to_grid_(seed.block_x_locs[blk_id], seed.block_y_locs[blk_id], seed.block_layer_nums[blk_id]);
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

void NonlinearNesterovPlacer::compute_preconditioner_(const std::vector<PrimitiveVectorDim>& dimensions,
                                                      const std::vector<double>& density_multipliers) {
    block_precond_.assign(ap_netlist_.blocks().size(), 0.0);
    // ABLATION GATE (temporary): VPR_ABL_PRECOND_ALPHA overrides the softening
    // exponent. kPreconditionAlpha = 0.5 has no board record; every published
    // electrostatic placer uses 1.0. The exponent changes the SHAPE of the
    // diagonal (the per-block ratios), which is the only property the
    // Barzilai-Borwein secant cannot absorb by rescaling.
    static const double precondition_alpha = [] {
        const char* e = std::getenv("VPR_ABL_PRECOND_ALPHA");
        double a = e ? std::atof(e) : kPreconditionAlpha;
        if (e)
            VTR_LOG("ABL: PRECOND_ALPHA=%g (default %g).\n", a, kPreconditionAlpha);
        return a;
    }();

    // Wirelength Hessian diagonal. Under the B2B model this is exact: the
    // quadratic's diagonal is the block's incident edge-weight sum (mean of
    // the two axes, since the preconditioner is one scalar per block). Under
    // WA it is the standard elfPlace estimate: the sum of incident net weights.
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
    // Affinity-spring Hessian diagonal (frozen-centroid approximation; see
    // affinity_spring_curvature() for why the exact (1 - 1/n) factor is not used).
    affinity_term_->add_curvature(block_precond_);

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
        // Floor, then soften toward uniform with an exponent < 1 to reduce
        // over-correction on high-curvature blocks (dense DSP/RAM in
        // heterogeneous FPGAs have much wider curvature range than ASIC cells).
        // No non-curvature damping terms remain in the objective, so the
        // damping argument is zero; it stays in the signature because the
        // separation of true curvature from step-control damping is the
        // invariant that made this diagonal reasonable about (see
        // preconditioner_math.h).
        block_precond_[blk_id] = jacobi_precond_diagonal(block_precond_[blk_id] + density_curvature,
                                                         0.,
                                                         kPreconditionFloor,
                                                         precondition_alpha);
    }

    filler_precond_.assign(dimensions.size(), kPreconditionFloor);
    for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
        double unit_mass = dim_idx < filler_unit_mass_.size() ? filler_unit_mass_[dim_idx] : 0.;
        // Fillers carry density mass only: no nets, no affinity, no incompatibility.
        filler_precond_[dim_idx] = jacobi_precond_diagonal(density_multipliers[dim_idx] * unit_mass,
                                                           0.,
                                                           kPreconditionFloor,
                                                           precondition_alpha);
    }
}

/**
 * @brief Per-dimension (overflow mass, target capacity) from nearest-tile deposition.
 *
 * Single source of the physical-overflow measurement. Both the aggregate ratio
 * (which drives the epoch overflow stop and the sparse-seed gate) and the
 * per-dimension ratios (which drive the adaptive density boosts) are derived
 * from this, so the two can never disagree about what "overflow" means.
 */
std::vector<std::pair<double, double>> NonlinearNesterovPlacer::compute_physical_overflow_totals_(
    const PartialPlacement& p_placement,
    const std::vector<PrimitiveVectorDim>& dimensions) const {
    std::vector<std::pair<double, double>> totals(dimensions.size(), {0., 0.});
    if (dimensions.empty())
        return totals;

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
        auto [x, y, layer] = clamp_to_grid_(p_placement.block_x_locs[blk_id], p_placement.block_y_locs[blk_id], p_placement.block_layer_nums[blk_id]);
        size_t idx = site_index(layer, static_cast<size_t>(std::floor(x)), static_cast<size_t>(std::floor(y)));
        for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
            double mass = block_mass.get_dim_val(dimensions[dim_idx]);
            if (mass != 0.)
                utilization[dim_idx][idx] += mass;
        }
    }

    for (size_t dim_idx = 0; dim_idx < dimensions.size(); dim_idx++) {
        for (size_t idx = 0; idx < num_sites; idx++) {
            double capacity = cached_target_capacity_[dim_idx][idx];
            totals[dim_idx].second += capacity;
            double overflow = utilization[dim_idx][idx] - capacity;
            if (overflow > 0.)
                totals[dim_idx].first += overflow;
        }
    }
    return totals;
}

double NonlinearNesterovPlacer::compute_physical_overflow_ratio_(const PartialPlacement& p_placement,
                                                                 const std::vector<PrimitiveVectorDim>& dimensions) const {
    double mass = 0.;
    double capacity = 0.;
    for (const auto& [dim_mass, dim_capacity] : compute_physical_overflow_totals_(p_placement, dimensions)) {
        mass += dim_mass;
        capacity += dim_capacity;
    }
    return capacity > kEpsilon ? mass / capacity : 0.;
}

std::vector<double> NonlinearNesterovPlacer::compute_physical_overflow_ratios_per_dim_(
    const PartialPlacement& p_placement,
    const std::vector<PrimitiveVectorDim>& dimensions) const {
    std::vector<std::pair<double, double>> totals = compute_physical_overflow_totals_(p_placement, dimensions);
    std::vector<double> ratios(totals.size(), 0.);
    for (size_t dim_idx = 0; dim_idx < totals.size(); dim_idx++) {
        const auto& [mass, capacity] = totals[dim_idx];
        ratios[dim_idx] = capacity > kEpsilon ? mass / capacity : 0.;
    }
    return ratios;
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
    next_placement = y_placement;
    if (large_design_ && !block_precond_.empty()) {
        // Preconditioned (near-Newton) step: divide each block's gradient by its
        // objective-curvature estimate so step direction is size-independent.
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
    // wirelength terms and FISTA look-ahead trial points. Clamp here
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
    y_placement = next;
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
