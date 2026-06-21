/**
 * @file
 * @author  William Zhang
 * @date    June 2026
 * @brief   Implementation of a nonlinear Nesterov analytical global placer.
 */

#include "nonlinear_nesterov_placer.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <optional>
#include <vector>
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
 * @brief Number of Jacobi iterations used to solve each electrostatic potential.
 */
constexpr size_t kElectrostaticJacobiIterations = 40;

/**
 * @brief Initial proximity weight after the first partial legalization.
 */
constexpr double kInitialProximityWeight = 0.05;

/**
 * @brief Multiplicative proximity-weight increase between legalization epochs.
 */
constexpr double kProximityWeightGrowth = 2.0;

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
 * @brief Small value used to avoid division by zero.
 */
constexpr double kEpsilon = 1e-9;

/**
 * @brief Device-bound epsilon to keep floor-based bin lookup inside the grid.
 */
constexpr double kDeviceBoundaryEpsilon = 1e-6;

/**
 * @brief Compute a numerically stable log-sum-exp smooth approximation of a coordinate extremum.
 *
 * With @p negate false, multiplying the result by @p gamma approximates the
 * maximum value. With @p negate true, multiplying the result by @p gamma
 * approximates the negated minimum value; this lets the caller form a smooth
 * max-minus-min wirelength estimate.
 *
 * @param values Coordinate values for one net dimension. Must be non-empty.
 * @param gamma Positive smoothing factor: smaller values more closely
 *              approximate the extremum, while larger values smooth it more.
 * @param negate When true, negate each value before computing log-sum-exp to
 *               approximate the negated minimum instead of the maximum.
 * @return The log-sum-exp of the scaled coordinate values.
 */
double stable_log_sum_exp(const std::vector<double>& values, double gamma, bool negate) {
    VTR_ASSERT(!values.empty());
    VTR_ASSERT(gamma > 0.);

    double max_scaled = negate ? -values.front() / gamma : values.front() / gamma;
    for (double value : values) {
        double scaled_value = negate ? -value / gamma : value / gamma;
        max_scaled = std::max(max_scaled, scaled_value);
    }

    double exp_sum = 0.;
    for (double value : values) {
        double scaled_value = negate ? -value / gamma : value / gamma;
        exp_sum += std::exp(scaled_value - max_scaled);
    }

    return max_scaled + std::log(exp_sum);
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

    PartialPlacement current = initialize_placement_();
    ObjectiveValue initial_components = evaluate_objective_(current, 1.0, std::nullopt, 0., std::nullopt);
    double initial_density_weight = 1e-3;
    if (initial_components.density > kEpsilon) {
        initial_density_weight = kInitialDensityToWirelengthRatio * std::max(initial_components.wirelength, 1.0) / initial_components.density;
    }
    double density_weight = std::clamp(initial_density_weight, 1e-5, 1e3);
    double max_density_weight = density_weight * kMaxDensityWeightGrowth;

    double device_span = std::max<double>(device_grid_width_, device_grid_height_);
    double convergence_displacement = std::max(kMinConvergenceDisplacement,
                                               device_span * kConvergenceDisplacementFraction);

    if (log_verbosity_ >= 1) {
        VTR_LOG("Epoch  Pre HPWL  Post HPWL  Pre Oflow  Post Oflow  Pre Max  Post Max  Mean Move  Max Move  Density Wt  Prox Wt\n");
        VTR_LOG("-----  --------  ---------  ---------  ----------  -------  --------  ---------  --------  ----------  -------\n");
    }

    for (size_t epoch = 0; epoch < kNesterovEpochs; epoch++) {
        if (ap_timing_tradeoff_ != 0.f && place_delay_model_) {
            update_timing_info_with_partial_placement(pre_cluster_timing_manager_,
                                                      *place_delay_model_,
                                                      current,
                                                      ap_netlist_);
        }
        update_timing_net_weights_();

        PartialPlacement legal_anchor = current;
        PartialPlacement y_placement = current;
        PartialPlacement next = current;
        PlacementGradient grad(ap_netlist_);
        double proximity_weight = epoch == 0 ? 0. : kInitialProximityWeight * std::pow(kProximityWeightGrowth, epoch - 1);
        double step_size = std::max(0.1, device_span * kInitialStepSpanFraction);
        double nesterov_t = 1.0;
        ObjectiveValue current_obj = evaluate_objective_(current,
                                                         density_weight,
                                                         std::cref(legal_anchor),
                                                         proximity_weight,
                                                         std::nullopt);

        for (size_t iter = 0; iter < kNesterovIterationsPerEpoch; iter++) {
            ObjectiveValue y_obj = evaluate_objective_(y_placement,
                                                       density_weight,
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
                                               density_weight,
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

            double next_t = 0.5 * (1.0 + std::sqrt(1.0 + 4.0 * nesterov_t * nesterov_t));
            double beta = (nesterov_t - 1.0) / next_t;

            if (next_obj.total > current_obj.total) {
                nesterov_t = 1.0;
                copy_placement_(next, y_placement);
            } else {
                extrapolate_(current, next, beta, y_placement);
                nesterov_t = next_t;
            }

            copy_placement_(next, current);
            current_obj = next_obj;
            step_size = std::min(device_span, accepted_step * 1.05);
            density_weight = std::min(max_density_weight, density_weight * kDensityWeightGrowth);

            if (iter + 1 >= kMinNesterovIterationsPerEpoch
                && max_step_displacement <= convergence_displacement) {
                break;
            }
        }

        ObjectiveValue pre_legalization = evaluate_objective_(current,
                                                              density_weight,
                                                              std::cref(legal_anchor),
                                                              proximity_weight,
                                                              std::nullopt);
        PartialPlacement before_legalization = current;
        partial_legalizer_->legalize(current);
        ObjectiveValue post_legalization = evaluate_objective_(current,
                                                               density_weight,
                                                               std::cref(legal_anchor),
                                                               proximity_weight,
                                                               std::nullopt);

        double total_displacement = 0.;
        double max_displacement = 0.;
        for (APBlockId blk_id : optimizable_blocks_) {
            double dx = current.block_x_locs[blk_id] - before_legalization.block_x_locs[blk_id];
            double dy = current.block_y_locs[blk_id] - before_legalization.block_y_locs[blk_id];
            double displacement = std::hypot(dx, dy);
            total_displacement += displacement;
            max_displacement = std::max(max_displacement, displacement);
        }

        VTR_LOG("%5zu  %8.2f  %9.2f  %9.4f  %10.4f  %7.4f  %8.4f  %9.4f  %8.4f  %10.4g  %7.4g\n",
                epoch,
                before_legalization.get_hpwl(ap_netlist_),
                current.get_hpwl(ap_netlist_),
                pre_legalization.total_overflow,
                post_legalization.total_overflow,
                pre_legalization.max_overflow,
                post_legalization.max_overflow,
                optimizable_blocks_.empty() ? 0. : total_displacement / optimizable_blocks_.size(),
                max_displacement,
                density_weight,
                proximity_weight);
    }

    VTR_LOG("Nonlinear Nesterov Global Placer Statistics:\n");
    VTR_LOG("\tPlacement HPWL after final partial legalization: %g\n", current.get_hpwl(ap_netlist_));
    VTR_LOG("\tFinal density weight: %g\n", density_weight);
    partial_legalizer_->print_statistics();

    return current;
}

NonlinearNesterovPlacer::ObjectiveValue NonlinearNesterovPlacer::evaluate_objective_(const PartialPlacement& p_placement,
                                                                                     double density_weight,
                                                                                     std::optional<std::reference_wrapper<const PartialPlacement>> legal_anchor,
                                                                                     double proximity_weight,
                                                                                     std::optional<std::reference_wrapper<PlacementGradient>> grad) const {
    if (grad)
        grad->get().clear();

    ObjectiveValue value;
    value.wirelength = add_wirelength_gradient_(p_placement, grad);
    value.density = add_density_gradient_(p_placement,
                                          density_weight,
                                          value.total_overflow,
                                          value.max_overflow,
                                          grad);
    if (legal_anchor)
        value.proximity = add_proximity_gradient_(p_placement,
                                                  legal_anchor->get(),
                                                  proximity_weight,
                                                  grad);
    value.total = value.wirelength + density_weight * value.density + proximity_weight * value.proximity;
    return value;
}

double NonlinearNesterovPlacer::add_wirelength_gradient_(const PartialPlacement& p_placement,
                                                         std::optional<std::reference_wrapper<PlacementGradient>> grad) const {
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

        double log_pos_x = stable_log_sum_exp(x_locs, gamma, false);
        double log_neg_x = stable_log_sum_exp(x_locs, gamma, true);
        double log_pos_y = stable_log_sum_exp(y_locs, gamma, false);
        double log_neg_y = stable_log_sum_exp(y_locs, gamma, true);

        smooth_wirelength += net_weight * gamma * (log_pos_x + log_neg_x + log_pos_y + log_neg_y);

        if (!grad)
            continue;

        double sum_pos_x = 0.;
        double sum_neg_x = 0.;
        double sum_pos_y = 0.;
        double sum_neg_y = 0.;
        for (size_t pin_idx = 0; pin_idx < num_pins; pin_idx++) {
            sum_pos_x += std::exp(x_locs[pin_idx] / gamma - log_pos_x);
            sum_neg_x += std::exp(-x_locs[pin_idx] / gamma - log_neg_x);
            sum_pos_y += std::exp(y_locs[pin_idx] / gamma - log_pos_y);
            sum_neg_y += std::exp(-y_locs[pin_idx] / gamma - log_neg_y);
        }
        VTR_ASSERT_SAFE(sum_pos_x > 0. && sum_neg_x > 0. && sum_pos_y > 0. && sum_neg_y > 0.);

        size_t pin_idx = 0;
        for (APPinId pin_id : ap_netlist_.net_pins(net_id)) {
            APBlockId blk_id = ap_netlist_.pin_block(pin_id);
            if (block_is_optimizable_(blk_id)) {
                double pos_x_grad = std::exp(x_locs[pin_idx] / gamma - log_pos_x) / sum_pos_x;
                double neg_x_grad = std::exp(-x_locs[pin_idx] / gamma - log_neg_x) / sum_neg_x;
                double pos_y_grad = std::exp(y_locs[pin_idx] / gamma - log_pos_y) / sum_pos_y;
                double neg_y_grad = std::exp(-y_locs[pin_idx] / gamma - log_neg_y) / sum_neg_y;
                grad->get().dx[blk_id] += net_weight * (pos_x_grad - neg_x_grad);
                grad->get().dy[blk_id] += net_weight * (pos_y_grad - neg_y_grad);
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
                                                      double density_weight,
                                                      double& total_overflow,
                                                      double& max_overflow,
                                                      std::optional<std::reference_wrapper<PlacementGradient>> grad) const {
    total_overflow = 0.;
    max_overflow = 0.;

    const FlatPlacementBins& bins = density_manager_->flat_placement_bins();
    std::vector<PrimitiveVectorDim> dimensions = density_manager_->get_used_dims_mask().get_non_zero_dims();
    if (dimensions.empty())
        return 0.;

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

    // Builds up "charge" at every tile site, corresponding to utilization
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

        double mean_charge = charge_sum / active_sites;
        for (size_t idx = 0; idx < num_sites; idx++) {
            if (active[idx])
                charge[idx] -= mean_charge;
        }

        // Solve an approximate discrete Poisson equation using 40 Jacobi iterations
        std::vector<double> next_potential(num_sites, 0.);
        for (size_t iter = 0; iter < kElectrostaticJacobiIterations; iter++) {
            for (size_t layer = 0; layer < num_layers; layer++) {
                for (size_t x = 0; x < width; x++) {
                    for (size_t y = 0; y < height; y++) {
                        size_t idx = site_index(layer, x, y);
                        if (!active[idx]) {
                            next_potential[idx] = 0.;
                            continue;
                        }

                        double neighbor_sum = 0.;
                        if (x > 0)
                            neighbor_sum += potential[dim_idx][site_index(layer, x - 1, y)];
                        if (x + 1 < width)
                            neighbor_sum += potential[dim_idx][site_index(layer, x + 1, y)];
                        if (y > 0)
                            neighbor_sum += potential[dim_idx][site_index(layer, x, y - 1)];
                        if (y + 1 < height)
                            neighbor_sum += potential[dim_idx][site_index(layer, x, y + 1)];
                        next_potential[idx] = 0.25 * (neighbor_sum + charge[idx]);
                    }
                }
            }
            potential[dim_idx].swap(next_potential);
        }

        for (size_t idx = 0; idx < num_sites; idx++)
            density_energy += 0.5 * charge[idx] * potential[dim_idx][idx];

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

            double normalized_mass = mass / std::max(local_target, 1.0);
            grad->get().dx[blk_id] += density_weight * normalized_mass * local_field_x;
            grad->get().dy[blk_id] += density_weight * normalized_mass * local_field_y;
        }
    }

    return density_energy;
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
    for (APBlockId blk_id : optimizable_blocks_) {
        next_placement.block_x_locs[blk_id] = y_placement.block_x_locs[blk_id] - step_size * grad.dx[blk_id];
        next_placement.block_y_locs[blk_id] = y_placement.block_y_locs[blk_id] - step_size * grad.dy[blk_id];
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
