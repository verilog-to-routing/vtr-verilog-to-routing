/**
 * @file
 * @author  William Zhang
 * @date    June 2026
 * @brief   Implementation of a Nesterov-style analytical global placer.
 */

#include "nesterov_global_placer.h"
#include <algorithm>
#include <cmath>
#include <limits>
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
 * @brief Minimum line-search step size before accepting a non-improving move.
 */
constexpr double kMinStepSize = 1e-6;

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
 * @brief Numerically stable log(sum(exp(value / gamma))).
 */
static double stable_log_sum_exp(const std::vector<double>& values, double gamma, bool negate) {
    VTR_ASSERT(!values.empty());

    double max_scaled = -std::numeric_limits<double>::infinity();
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

NesterovGlobalPlacer::PlacementGradient::PlacementGradient(const APNetlist& ap_netlist)
    : dx(ap_netlist.blocks().size(), 0.)
    , dy(ap_netlist.blocks().size(), 0.) {}

void NesterovGlobalPlacer::PlacementGradient::clear() {
    std::fill(dx.begin(), dx.end(), 0.);
    std::fill(dy.begin(), dy.end(), 0.);
}

NesterovGlobalPlacer::NesterovGlobalPlacer(const APNetlist& ap_netlist,
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
    vtr::ScopedStartFinishTimer nesterov_placer_building_timer("Constructing Nesterov Global Placer");

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

bool NesterovGlobalPlacer::block_is_optimizable_(APBlockId blk_id) const {
    return ap_netlist_.block_mobility(blk_id) == APBlockMobility::MOVEABLE;
}

PartialPlacement NesterovGlobalPlacer::initialize_placement_() const {
    PartialPlacement p_placement(ap_netlist_);

    if (optimizable_blocks_.empty()) {
        project_placement_(p_placement);
        return p_placement;
    }

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

PartialPlacement NesterovGlobalPlacer::place() {
    vtr::ScopedStartFinishTimer global_placer_time("AP Nesterov Global Placer");

    PartialPlacement current = initialize_placement_();
    update_timing_info_with_placement_(current);
    update_timing_net_weights_();

    PartialPlacement y_placement = current;
    PartialPlacement next = current;
    PlacementGradient grad(ap_netlist_);

    ObjectiveValue initial_components = evaluate_objective_(current, 1.0, nullptr);
    double initial_density_weight = 1e-3;
    if (initial_components.density > kEpsilon) {
        initial_density_weight = kInitialDensityToWirelengthRatio * std::max(initial_components.wirelength, 1.0) / initial_components.density;
    }
    double density_weight = std::clamp(initial_density_weight, 1e-5, 1e3);
    double max_density_weight = density_weight * kMaxDensityWeightGrowth;

    double device_span = std::max<double>(device_grid_width_, device_grid_height_);
    double step_size = std::max(0.1, device_span * kInitialStepSpanFraction);
    double nesterov_t = 1.0;
    ObjectiveValue current_obj = evaluate_objective_(current, density_weight, nullptr);

    if (log_verbosity_ >= 1) {
        VTR_LOG("----  ------------  --------------  ------------  ------------  -----------\n");
        VTR_LOG("Iter  Smooth Obj    Smooth WL       Density       HPWL          Step\n");
        VTR_LOG("----  ------------  --------------  ------------  ------------  -----------\n");
    }

    for (size_t iter = 0; iter < kMaxNesterovIterations; iter++) {
        ObjectiveValue y_obj = evaluate_objective_(y_placement, density_weight, &grad);
        double grad_norm_sq = gradient_norm_squared_(grad);
        if (grad_norm_sq < kEpsilon)
            break;

        double accepted_step = step_size;
        ObjectiveValue next_obj;
        bool accepted = false;
        while (accepted_step >= kMinStepSize) {
            gradient_step_(y_placement, grad, accepted_step, next);
            next_obj = evaluate_objective_(next, density_weight, nullptr);
            if (next_obj.total <= y_obj.total || accepted_step == kMinStepSize) {
                accepted = true;
                break;
            }
            accepted_step *= 0.5;
        }

        if (!accepted)
            break;

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

        if (log_verbosity_ >= 1 && (iter % 10 == 0 || iter + 1 == kMaxNesterovIterations)) {
            VTR_LOG("%4zu  %12.4g  %14.4g  %12.4g  %12.2f  %11.4g\n",
                    iter,
                    current_obj.total,
                    current_obj.wirelength,
                    current_obj.density,
                    current.get_hpwl(ap_netlist_),
                    accepted_step);
        }
    }

    VTR_LOG("Nesterov Global Placer Statistics:\n");
    VTR_LOG("\tSmooth placement HPWL before partial legalization: %g\n", current.get_hpwl(ap_netlist_));
    VTR_LOG("\tFinal density weight: %g\n", density_weight);

    partial_legalizer_->legalize(current);
    VTR_LOG("\tPlacement HPWL after partial legalization: %g\n", current.get_hpwl(ap_netlist_));
    partial_legalizer_->print_statistics();

    return current;
}

NesterovGlobalPlacer::ObjectiveValue NesterovGlobalPlacer::evaluate_objective_(const PartialPlacement& p_placement,
                                                                               double density_weight,
                                                                               PlacementGradient* grad) const {
    if (grad)
        grad->clear();

    ObjectiveValue value;
    value.wirelength = add_wirelength_gradient_(p_placement, grad);
    value.density = add_density_gradient_(p_placement, density_weight, grad);
    value.total = value.wirelength + density_weight * value.density;
    return value;
}

double NesterovGlobalPlacer::add_wirelength_gradient_(const PartialPlacement& p_placement,
                                                      PlacementGradient* grad) const {
    double gamma = std::max(1.0, std::max<double>(device_grid_width_, device_grid_height_) * kWirelengthGammaFraction);
    double smooth_wirelength = 0.;

    std::vector<double> x_locs;
    std::vector<double> y_locs;
    x_locs.reserve(32);
    y_locs.reserve(32);

    for (APNetId net_id : ap_netlist_.nets()) {
        if (ap_netlist_.net_is_ignored(net_id))
            continue;

        size_t num_pins = ap_netlist_.net_pins(net_id).size();
        if (num_pins < 2)
            continue;

        double net_weight = net_weights_[net_id];
        x_locs.clear();
        y_locs.clear();
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
                grad->dx[blk_id] += net_weight * (pos_x_grad - neg_x_grad);
                grad->dy[blk_id] += net_weight * (pos_y_grad - neg_y_grad);
            }
            pin_idx++;
        }
    }

    return smooth_wirelength;
}

void NesterovGlobalPlacer::update_timing_net_weights_() {
    std::fill(net_weights_.begin(), net_weights_.end(), 1.0);

    if (ap_timing_tradeoff_ == 0.f)
        return;

    if (!pre_cluster_timing_manager_.is_valid()) {
        VTR_LOG_WARN("Nesterov analytical placement requested timing tradeoff %g, but pre-cluster timing is unavailable; using unit net weights.\n",
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
        VTR_LOG("Nesterov timing net weights: tradeoff=%g min=%g avg=%g max=%g nets=%zu\n",
                ap_timing_tradeoff_,
                min_weight,
                total_weight / weighted_nets,
                max_weight,
                weighted_nets);
    }
}

void NesterovGlobalPlacer::update_timing_info_with_placement_(const PartialPlacement& p_placement) {
    if (ap_timing_tradeoff_ == 0.f || !pre_cluster_timing_manager_.is_valid() || !place_delay_model_)
        return;

    for (APPinId ap_pin_id : ap_netlist_.pins()) {
        if (ap_netlist_.pin_type(ap_pin_id) != PinType::SINK)
            continue;

        APNetId ap_net_id = ap_netlist_.pin_net(ap_pin_id);
        APBlockId ap_driver_block_id = ap_netlist_.net_driver_block(ap_net_id);
        APBlockId ap_sink_block_id = ap_netlist_.pin_block(ap_pin_id);

        t_physical_tile_loc driver_block_loc(p_placement.block_x_locs[ap_driver_block_id],
                                             p_placement.block_y_locs[ap_driver_block_id],
                                             p_placement.block_layer_nums[ap_driver_block_id]);
        t_physical_tile_loc sink_block_loc(p_placement.block_x_locs[ap_sink_block_id],
                                           p_placement.block_y_locs[ap_sink_block_id],
                                           p_placement.block_layer_nums[ap_sink_block_id]);

        float delay = place_delay_model_->delay(driver_block_loc,
                                                0,
                                                sink_block_loc,
                                                0);
        if (delay >= ROUTER_LOOKAHEAD_NO_PATH_SENTINEL) {
            int manhattan_dist = std::abs(driver_block_loc.x - sink_block_loc.x)
                                 + std::abs(driver_block_loc.y - sink_block_loc.y);
            delay = manhattan_dist * delay_per_tile_();
        }

        AtomPinId atom_sink_pin_id = ap_netlist_.pin_atom_pin(ap_pin_id);
        pre_cluster_timing_manager_.set_timing_arc_delay(atom_sink_pin_id, delay);
    }

    if (pre_cluster_timing_manager_.get_timing_update_type() == e_timing_update_type::INCREMENTAL) {
        for (tatum::EdgeId edge : pre_cluster_timing_manager_.get_timing_info().timing_graph()->edges()) {
            pre_cluster_timing_manager_.get_timing_info_ptr()->invalidate_delay(edge);
        }
    }

    pre_cluster_timing_manager_.update_timing_info();
    pre_cluster_timing_manager_.get_timing_info_ptr()->set_warn_unconstrained(false);
}

float NesterovGlobalPlacer::delay_per_tile_() const {
    VTR_ASSERT_SAFE(place_delay_model_);

    int cx = static_cast<int>(device_grid_width_) / 2;
    int cy = static_cast<int>(device_grid_height_) / 2;
    int tx = std::min<int>(cx + 1, device_grid_width_ - 1);
    int ty = cy;
    if (tx == cx)
        ty = std::min<int>(cy + 1, device_grid_height_ - 1);
    if (tx == cx && ty == cy)
        return 0.0f;

    t_physical_tile_loc from_loc(cx, cy, 0);
    t_physical_tile_loc to_loc(tx, ty, 0);
    float delay = place_delay_model_->delay(from_loc, 0, to_loc, 0);
    if (delay >= ROUTER_LOOKAHEAD_NO_PATH_SENTINEL)
        return 0.0f;
    return delay;
}

double NesterovGlobalPlacer::add_density_gradient_(const PartialPlacement& p_placement,
                                                   double density_weight,
                                                   PlacementGradient* grad) const {
    const FlatPlacementBins& bins = density_manager_->flat_placement_bins();
    vtr::vector<FlatPlacementBinId, PrimitiveVector> bin_utilization(density_manager_->flat_placement_bins().bins().size());

    struct BlockBinContribution {
        APBlockId blk_id;
        FlatPlacementBinId bin_id;
        double dweight_dx;
        double dweight_dy;
        PrimitiveVector block_mass;
    };

    std::vector<BlockBinContribution> contributions;
    contributions.reserve(optimizable_blocks_.size() * 4);

    for (APBlockId blk_id : ap_netlist_.blocks()) {
        PrimitiveVector block_mass = density_manager_->mass_calculator().get_block_mass(blk_id);
        if (block_mass.is_zero())
            continue;

        double x = std::clamp(p_placement.block_x_locs[blk_id], 0., device_grid_width_ - kDeviceBoundaryEpsilon);
        double y = std::clamp(p_placement.block_y_locs[blk_id], 0., device_grid_height_ - kDeviceBoundaryEpsilon);
        double layer = std::clamp(std::round(p_placement.block_layer_nums[blk_id]), 0., static_cast<double>(device_grid_num_layers_ - 1));

        if (!block_is_optimizable_(blk_id)) {
            FlatPlacementBinId bin_id = density_manager_->get_bin(x, y, layer);
            bin_utilization[bin_id] += block_mass;
            continue;
        }

        int x0 = static_cast<int>(std::floor(x));
        int y0 = static_cast<int>(std::floor(y));
        int x1 = std::min<int>(x0 + 1, device_grid_width_ - 1);
        int y1 = std::min<int>(y0 + 1, device_grid_height_ - 1);
        double fx = x - x0;
        double fy = y - y0;

        double wx[2] = {1.0 - fx, fx};
        double wy[2] = {1.0 - fy, fy};
        double dwx[2] = {-1.0, 1.0};
        double dwy[2] = {-1.0, 1.0};
        int xs[2] = {x0, x1};
        int ys[2] = {y0, y1};

        if (x0 == x1) {
            wx[0] = 1.0;
            wx[1] = 0.0;
            dwx[0] = 0.0;
            dwx[1] = 0.0;
        }
        if (y0 == y1) {
            wy[0] = 1.0;
            wy[1] = 0.0;
            dwy[0] = 0.0;
            dwy[1] = 0.0;
        }

        for (size_t xi = 0; xi < 2; xi++) {
            for (size_t yi = 0; yi < 2; yi++) {
                double weight = wx[xi] * wy[yi];
                if (weight == 0.)
                    continue;
                FlatPlacementBinId bin_id = density_manager_->get_bin(xs[xi], ys[yi], layer);
                PrimitiveVector weighted_mass = block_mass;
                weighted_mass *= weight;
                bin_utilization[bin_id] += weighted_mass;
                contributions.push_back({blk_id,
                                         bin_id,
                                         dwx[xi] * wy[yi],
                                         wx[xi] * dwy[yi],
                                         block_mass});
            }
        }
    }

    vtr::vector<FlatPlacementBinId, PrimitiveVector> bin_util_derivative(bins.bins().size());
    double density_penalty = 0.;
    for (FlatPlacementBinId bin_id : bins.bins()) {
        PrimitiveVector target_capacity = density_manager_->get_bin_capacity(bin_id);
        target_capacity *= density_manager_->get_bin_target_density(bin_id);

        for (PrimitiveVectorDim dim : bin_utilization[bin_id].get_non_zero_dims()) {
            double util = bin_utilization[bin_id].get_dim_val(dim);
            double capacity = target_capacity.get_dim_val(dim);
            double overflow = std::max(0.0, util - capacity);
            double norm_capacity = std::max(capacity, 1.0);
            double normalized_overflow = overflow / norm_capacity;
            density_penalty += 0.5 * normalized_overflow * normalized_overflow;
            if (overflow > 0.)
                bin_util_derivative[bin_id].set_dim_val(dim, overflow / (norm_capacity * norm_capacity));
        }
    }

    if (grad) {
        for (const BlockBinContribution& contribution : contributions) {
            double density_derivative = 0.;
            for (PrimitiveVectorDim dim : contribution.block_mass.get_non_zero_dims()) {
                density_derivative += bin_util_derivative[contribution.bin_id].get_dim_val(dim)
                                      * contribution.block_mass.get_dim_val(dim);
            }
            density_derivative *= density_weight;
            grad->dx[contribution.blk_id] += density_derivative * contribution.dweight_dx;
            grad->dy[contribution.blk_id] += density_derivative * contribution.dweight_dy;
        }
    }

    return density_penalty;
}

void NesterovGlobalPlacer::project_placement_(PartialPlacement& p_placement) const {
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

void NesterovGlobalPlacer::copy_placement_(const PartialPlacement& src, PartialPlacement& dst) const {
    dst.block_x_locs = src.block_x_locs;
    dst.block_y_locs = src.block_y_locs;
    dst.block_layer_nums = src.block_layer_nums;
    dst.block_sub_tiles = src.block_sub_tiles;
}

void NesterovGlobalPlacer::gradient_step_(const PartialPlacement& y_placement,
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

void NesterovGlobalPlacer::extrapolate_(const PartialPlacement& current,
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

double NesterovGlobalPlacer::gradient_norm_squared_(const PlacementGradient& grad) const {
    double norm_squared = 0.;
    for (APBlockId blk_id : optimizable_blocks_) {
        norm_squared += grad.dx[blk_id] * grad.dx[blk_id];
        norm_squared += grad.dy[blk_id] * grad.dy[blk_id];
    }
    return norm_squared;
}
