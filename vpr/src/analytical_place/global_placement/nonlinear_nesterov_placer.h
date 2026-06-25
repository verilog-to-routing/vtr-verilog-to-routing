#pragma once
/**
 * @file
 * @author  William Zhang
 * @date    June 2026
 * @brief   Declaration of a nonlinear Nesterov analytical global placer.
 */

#include <functional>
#include <memory>
#include <optional>
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
     * @brief Mobile field-only filler particles for one optimization run.
     *
     * Fillers emulate the elfPlace/RePlAce filler-cell technique: each primitive
     * dimension owns a set of massed particles that occupy whitespace so the
     * electrostatic field reaches a compact target-density equilibrium without an
     * isotropic anchor. Fillers are deposited into the density field and pushed by
     * density forces only; they never enter the AP netlist or the legalizer.
     */
    struct FillerSet {
        std::vector<std::vector<double>> x;     ///< Filler x location for each [dim][filler].
        std::vector<std::vector<double>> y;     ///< Filler y location for each [dim][filler].
        std::vector<std::vector<size_t>> layer; ///< Filler layer index for each [dim][filler].
        std::vector<double> unit_mass;          ///< Mass each filler deposits in its dimension.
    };

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
        double total = 0.;                    ///< Weighted total objective.
        double wirelength = 0.;               ///< Unweighted smooth wirelength.
        double density = 0.;                  ///< Unweighted electrostatic density energy.
        std::vector<double> density_energies; ///< Electrostatic energy for each primitive dimension.
        double proximity = 0.;                ///< Unweighted proximity penalty to a legalized anchor.
        double total_overflow = 0.;           ///< Sum of normalized tile overflows.
        double max_overflow = 0.;             ///< Largest normalized tile overflow.
    };

    /**
     * @brief Run one full accelerated optimization (epoch loop) from a fresh
     *        initial placement and return the legalized result.
     *
     * Reads @ref precond_active_ / @ref precond_alpha_active_ so the same core can
     * be run with and without the preconditioner for the portfolio.
     */
    PartialPlacement run_global_optimization_(const std::vector<PrimitiveVectorDim>& density_dimensions,
                                              double device_span,
                                              double convergence_displacement);

    /**
     * @brief Initialize all block locations before first-order optimization.
     */
    PartialPlacement initialize_placement_() const;

    /**
     * @brief Evaluate the smooth objective, optionally accumulating gradients.
     */
    ObjectiveValue evaluate_objective_(const PartialPlacement& p_placement,
                                       const std::vector<double>& density_multipliers,
                                       const std::vector<double>& density_penalties,
                                       std::optional<std::reference_wrapper<const PartialPlacement>> legal_anchor,
                                       double proximity_weight,
                                       std::optional<std::reference_wrapper<PlacementGradient>> grad) const;

    /**
     * @brief Add smooth weighted-average wirelength value and gradient.
     */
    double add_wirelength_gradient_(const PartialPlacement& p_placement,
                                    std::optional<std::reference_wrapper<PlacementGradient>> grad) const;

    /**
     * @brief Update smooth wirelength net weights from pre-cluster timing criticalities.
     */
    void update_timing_net_weights_();

    /**
     * @brief Add electrostatic density value and gradient.
     */
    double add_density_gradient_(const PartialPlacement& p_placement,
                                 const std::vector<double>& density_multipliers,
                                 const std::vector<double>& density_penalties,
                                 std::vector<double>& density_energies,
                                 double& total_overflow,
                                 double& max_overflow,
                                 std::optional<std::reference_wrapper<PlacementGradient>> grad) const;

    /**
     * @brief Compute a per-block mass inflation factor from pin density.
     *
     * Pin-dense blocks are given more electrostatic mass so they claim more area
     * and spread further, emulating the elfPlace routability/pin area-adjustment
     * step. Whitespace (and therefore filler mass) shrinks accordingly.
     */
    void compute_area_inflation_();

    /**
     * @brief Initialize mobile filler particles for each primitive dimension.
     *
     * Filler mass per dimension is the target capacity minus deposited physical
     * mass; particles are scattered uniformly across the device. Returns the
     * total number of fillers created across all dimensions.
     */
    size_t initialize_fillers_(const std::vector<PrimitiveVectorDim>& dimensions);

    /**
     * @brief Advance filler particles down the buffered density force.
     *
     * Each dimension's fillers flow along the RMS-normalized density gradient so a
     * typically forced filler moves @p move_cap tiles. Fillers are clamped to the
     * device interior and never touch the AP netlist or legalizer.
     */
    void move_fillers_(double move_cap);

    /**
     * @brief Recompute the diagonal preconditioner for the current epoch.
     *
     * Fills @ref block_precond_ with a per-block objective-curvature estimate:
     * the sum of incident net weights (wirelength Hessian diagonal) plus the
     * density multiplier times block mass summed over resource dimensions
     * (density Hessian diagonal). The preconditioned gradient step divides each
     * block's gradient by this value, giving size-independent step lengths.
     */
    void compute_preconditioner_(const std::vector<PrimitiveVectorDim>& dimensions,
                                 const std::vector<double>& density_multipliers);

    /**
     * @brief Add a quadratic proximity penalty to the latest legalized placement.
     */
    double add_proximity_gradient_(const PartialPlacement& p_placement,
                                   const PartialPlacement& legal_anchor,
                                   double proximity_weight,
                                   std::optional<std::reference_wrapper<PlacementGradient>> grad) const;

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
     * @brief Return the largest x-y displacement between two placements.
     */
    double max_block_displacement_(const PartialPlacement& from,
                                   const PartialPlacement& to) const;

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

    vtr::vector<APBlockId, double> block_mass_scale_;        ///< Per-block electrostatic mass inflation factor (default 1).
    vtr::vector<APBlockId, double> block_precond_;           ///< Per-block diagonal preconditioner (objective curvature estimate).
    bool precond_active_ = false;                            ///< Whether the preconditioner is applied in the current optimization run.
    double precond_alpha_active_ = 1.0;                      ///< Preconditioner strength exponent for the current optimization run.
    FillerSet fillers_;                                      ///< Mobile field-only filler particles per dimension.
    bool fillers_active_ = false;                            ///< When true, fillers are deposited into the density field.
    mutable std::vector<std::vector<double>> filler_grad_x_; ///< Buffered filler density force in x for each [dim][filler].
    mutable std::vector<std::vector<double>> filler_grad_y_; ///< Buffered filler density force in y for each [dim][filler].

    size_t device_grid_width_ = 0;      ///< Width of the placement region.
    size_t device_grid_height_ = 0;     ///< Height of the placement region.
    size_t device_grid_num_layers_ = 0; ///< Number of device layers.
    float ap_timing_tradeoff_ = 0.f;    ///< User timing tradeoff value.
};
