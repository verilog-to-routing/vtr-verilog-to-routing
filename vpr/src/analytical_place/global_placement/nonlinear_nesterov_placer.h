#pragma once
/**
 * @file
 * @author  William Zhang
 * @date    June 2026
 * @brief   Declaration of a nonlinear Nesterov analytical global placer.
 */

#include <memory>
#include <string>
#include <vector>
#include "flat_placement_density_manager.h"
#include "global_placer.h"
#include "partial_legalizer.h"
#include "vtr_vector.h"

class AtomNetlist;
class DeviceGrid;
class LogicalModels;
class PlaceDelayModel;
class PreClusterTimingManager;
class Prepacker;
struct t_logical_block_type;
struct t_physical_tile_type;

/**
 * @brief Analytical global placer using accelerated first-order updates.
 *
 * This placer directly optimizes a smooth nonlinear objective consisting of
 * weighted-average wirelength and a continuous bin-density penalty. The result
 * is then passed through the existing partial legalizer to clean up discrete
 * architecture constraints before full legalization.
 */
class NonlinearNesterovPlacer : public GlobalPlacer {
  public:
    /**
     * @brief Construct the nonlinear Nesterov global placer and its density/legalization helpers.
     */
    NonlinearNesterovPlacer(const APNetlist& ap_netlist,
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
                            int log_verbosity);

    /**
     * @brief Run accelerated smooth global placement and final partial legalization.
     */
    PartialPlacement place() final;

  private:
    /**
     * @brief Per-block placement gradient.
     */
    struct PlacementGradient {
        vtr::vector<APBlockId, double> dx; ///< Objective derivative with respect to x.
        vtr::vector<APBlockId, double> dy; ///< Objective derivative with respect to y.

        /**
         * @brief Construct a zero gradient sized for the AP netlist.
         */
        explicit PlacementGradient(const APNetlist& ap_netlist);

        /**
         * @brief Reset all gradient entries to zero.
         */
        void clear();
    };

    /**
     * @brief Objective components from a smooth placement evaluation.
     */
    struct ObjectiveValue {
        double total = 0.;          ///< Weighted total objective.
        double wirelength = 0.;     ///< Unweighted smooth wirelength.
        double density = 0.;        ///< Unweighted electrostatic density energy.
        double proximity = 0.;      ///< Unweighted proximity penalty to a legalized anchor.
        double total_overflow = 0.; ///< Sum of normalized tile overflows.
        double max_overflow = 0.;   ///< Largest normalized tile overflow.
    };

    /**
     * @brief Initialize all block locations before first-order optimization.
     */
    PartialPlacement initialize_placement_() const;

    /**
     * @brief Evaluate the smooth objective, optionally accumulating gradients.
     */
    ObjectiveValue evaluate_objective_(const PartialPlacement& p_placement,
                                       double density_weight,
                                       const PartialPlacement* legal_anchor,
                                       double proximity_weight,
                                       PlacementGradient* grad) const;

    /**
     * @brief Add smooth weighted-average wirelength value and gradient.
     */
    double add_wirelength_gradient_(const PartialPlacement& p_placement,
                                    PlacementGradient* grad) const;

    /**
     * @brief Update smooth wirelength net weights from pre-cluster timing criticalities.
     */
    void update_timing_net_weights_();

    /**
     * @brief Update pre-cluster timing delays and criticalities from a placement.
     */
    void update_timing_info_with_placement_(const PartialPlacement& p_placement);

    /**
     * @brief Estimate one tile of delay for missing delay-model entries.
     */
    float delay_per_tile_() const;

    /**
     * @brief Add electrostatic density value and gradient.
     */
    double add_density_gradient_(const PartialPlacement& p_placement,
                                 double density_weight,
                                 double* total_overflow,
                                 double* max_overflow,
                                 PlacementGradient* grad) const;

    /**
     * @brief Add a quadratic proximity penalty to the latest legalized placement.
     */
    double add_proximity_gradient_(const PartialPlacement& p_placement,
                                   const PartialPlacement& legal_anchor,
                                   double proximity_weight,
                                   PlacementGradient* grad) const;

    /**
     * @brief Project all block locations into device bounds and restore fixed blocks.
     */
    void project_placement_(PartialPlacement& p_placement) const;

    /**
     * @brief Copy a placement.
     */
    void copy_placement_(const PartialPlacement& src, PartialPlacement& dst) const;

    /**
     * @brief Apply x = y - step * grad for movable blocks.
     */
    void gradient_step_(const PartialPlacement& y_placement,
                        const PlacementGradient& grad,
                        double step_size,
                        PartialPlacement& next_placement) const;

    /**
     * @brief Apply the Nesterov extrapolation y = x_next + beta * (x_next - x).
     */
    void extrapolate_(const PartialPlacement& current,
                      const PartialPlacement& next,
                      double beta,
                      PartialPlacement& y_placement) const;

    /**
     * @brief Compute the squared norm of a placement gradient.
     */
    double gradient_norm_squared_(const PlacementGradient& grad) const;

    /**
     * @brief Return true if the block should be moved by the continuous optimizer.
     */
    bool block_is_optimizable_(APBlockId blk_id) const;

    std::shared_ptr<FlatPlacementDensityManager> density_manager_; ///< Architecture-aware density metadata.
    std::unique_ptr<PartialLegalizer> partial_legalizer_;          ///< Existing partial legalizer used after smooth optimization.
    const AtomNetlist& atom_netlist_;                              ///< Atom netlist used for timing criticality lookup.
    PreClusterTimingManager& pre_cluster_timing_manager_;          ///< Timing manager used to weight smooth wirelength nets.
    std::shared_ptr<PlaceDelayModel> place_delay_model_;           ///< Delay model used to update timing criticalities.

    std::vector<APBlockId> optimizable_blocks_; ///< Movable AP blocks touched by the optimizer.
    vtr::vector<APNetId, double> net_weights_;  ///< Smooth wirelength weight for each AP net.
    size_t device_grid_width_ = 0;              ///< Width of the placement region.
    size_t device_grid_height_ = 0;             ///< Height of the placement region.
    size_t device_grid_num_layers_ = 0;         ///< Number of device layers.
    float ap_timing_tradeoff_ = 0.f;            ///< User timing tradeoff value.
};
