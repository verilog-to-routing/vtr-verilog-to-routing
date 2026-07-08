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

class AnalyticalSolver;
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
 * This placer directly optimizes a differentiable nonlinear objective consisting of
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
     * @brief Out-of-line destructor.
     *
     * Defined in the .cpp so the unique_ptr<AnalyticalSolver> member (the type is
     * only forward-declared here) is destroyed where AnalyticalSolver is complete.
     */
    ~NonlinearNesterovPlacer();

    /**
     * @brief Run global placement with weighted average smooth/differentiable wirelength
     *        and electrostatic density formulation using Nesterov Accelerated Gradient optimizer
     *        then hand the result to the partial legalizer.
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
     * @brief Objective components from a placement evaluation.
     */
    struct ObjectiveValue {
        double total = 0.;                    ///< Weighted total objective.
        double wirelength = 0.;               ///< Unweighted differentiable wirelength.
        double density = 0.;                  ///< Unweighted electrostatic density energy.
        std::vector<double> density_energies; ///< Electrostatic energy for each primitive dimension.
        double pack_pattern_cohesion = 0.;    ///< Unweighted cohesion penalty for multi-block pack-pattern chains.
        double proximity = 0.;                ///< Unweighted proximity penalty to a legalized anchor.
        double total_overflow = 0.;           ///< Sum of normalized tile overflows.
        double max_overflow = 0.;             ///< Largest normalized tile overflow.
        double overflow_ratio = 0.;           ///< Overflow mass / total deposited mass (legality in [0,~1]).
    };

    /**
     * @brief Dynamic elfPlace-style filler locations.
     *
     * Fillers are per-resource movable whitespace particles. They carry density
     * mass but no nets and never enter legalization, packing, or timing.
     */
    struct FillerState {
        std::vector<std::vector<double>> x;  ///< [dim][filler] x coordinate.
        std::vector<std::vector<double>> y;  ///< [dim][filler] y coordinate.
        std::vector<std::vector<int>> layer; ///< [dim][filler] fixed device layer.
    };

    /**
     * @brief Density-only gradients for dynamic fillers.
     */
    struct FillerGradient {
        std::vector<std::vector<double>> dx; ///< [dim][filler] derivative wrt x.
        std::vector<std::vector<double>> dy; ///< [dim][filler] derivative wrt y.
    };

    /**
     * @brief Run one full accelerated optimization (epoch loop) from a fresh
     *        initial placement and return the legalized result.
     *
     * Reads @ref precond_active_ / @ref precond_alpha_active_ to size-gate the
     * preconditioner for the current design.
     */
    PartialPlacement run_global_optimization_(const std::vector<PrimitiveVectorDim>& density_dimensions,
                                              double device_span,
                                              double convergence_displacement);

    /**
     * @brief Run the Nesterov epoch loop from a fixed seed placement.
     */
    PartialPlacement optimize_from_seed_(const PartialPlacement& seed,
                                         const std::vector<PrimitiveVectorDim>& density_dimensions,
                                         double device_span,
                                         double convergence_displacement);

    /**
     * @brief Initialize all block locations before first-order optimization.
     *
     * Sets @ref sparse_seed_ when the warm-start seed's physical overflow is
     * already below the sparse gate (electrostatic-inert design).
     */
    PartialPlacement initialize_placement_();

    /**
     * @brief Evaluate the differentiable objective, optionally accumulating gradients.
     */
    ObjectiveValue evaluate_objective_(const PartialPlacement& p_placement,
                                       const std::vector<double>& density_multipliers,
                                       std::optional<std::reference_wrapper<const PartialPlacement>> legal_anchor,
                                       double proximity_weight,
                                       std::optional<std::reference_wrapper<PlacementGradient>> grad,
                                       const FillerState& fillers,
                                       std::optional<std::reference_wrapper<FillerGradient>> filler_grad) const;

    /**
     * @brief Add differentiable weighted-average wirelength value and gradient.
     */
    double add_wirelength_gradient_(const PartialPlacement& p_placement,
                                    std::optional<std::reference_wrapper<PlacementGradient>> grad) const;

    /**
     * @brief Update differentiable wirelength net weights from pre-cluster timing criticalities.
     */
    void update_timing_net_weights_();

    /**
     * @brief Build groups of AP blocks that belong to the same long prepacker chain.
     */
    void initialize_pack_pattern_cohesion_groups_(const Prepacker& prepacker);

    /**
     * @brief Add a quadratic cohesion term for long prepacker chains spanning multiple AP blocks.
     */
    double add_pack_pattern_cohesion_gradient_(const PartialPlacement& p_placement,
                                               std::optional<std::reference_wrapper<PlacementGradient>> grad) const;

    /**
     * @brief Add electrostatic density value and gradient.
     */
    double add_density_gradient_(const PartialPlacement& p_placement,
                                 const std::vector<double>& density_multipliers,
                                 std::vector<double>& density_energies,
                                 double& total_overflow,
                                 double& max_overflow,
                                 double& overflow_ratio,
                                 std::optional<std::reference_wrapper<PlacementGradient>> grad,
                                 const FillerState& fillers,
                                 std::optional<std::reference_wrapper<FillerGradient>> filler_grad) const;

    /**
     * @brief Build dynamic filler particles from seed whitespace.
     *
     * Fillers approximate movable whitespace in the electrostatic system: each
     * resource dimension receives particles carrying @p whitespace_fraction of
     * target_capacity - movable_mass, initialized into capacity-bearing seed
     * whitespace. A non-positive fraction disables fillers (empty state).
     */
    void initialize_dynamic_fillers_(const PartialPlacement& seed,
                                     const std::vector<PrimitiveVectorDim>& dimensions,
                                     double whitespace_fraction,
                                     FillerState& fillers);

    /**
     * @brief Identify resource dimensions whose target capacity is almost entirely on the device boundary.
     */
    std::vector<bool> identify_boundary_confined_dims_(const std::vector<PrimitiveVectorDim>& dimensions) const;

    /**
     * @brief Return true if a block has any mass in boundary-confined dimensions.
     */
    bool block_has_boundary_mass_(APBlockId blk_id,
                                  const std::vector<PrimitiveVectorDim>& dimensions) const;

    /**
     * @brief Mark long I/O-related nets that should receive extra wirelength cohesion.
     */
    void update_boundary_net_flags_(const std::vector<PrimitiveVectorDim>& dimensions,
                                    const PartialPlacement& seed);

    /**
     * @brief Return true if all atom primitives represented by the AP block are I/O-chain primitives.
     */
    bool block_is_io_chain_block_(APBlockId blk_id) const;

    /**
     * @brief Project dynamic filler locations into device bounds.
     */
    void project_fillers_(FillerState& fillers) const;

    /**
     * @brief Physical-overflow ratio: mass exceeding tile capacity over capacity.
     *
     * Bins physical block mass onto the tile grid and returns
     * sum(max(0, utilization - target)) / sum(target) across dimensions.
     * Used as the WL-favoring penalty stop signal; cheap (no Poisson solve).
     */
    double compute_physical_overflow_ratio_(const PartialPlacement& p_placement,
                                            const std::vector<PrimitiveVectorDim>& dimensions) const;

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
     * @brief Apply x = y - step * grad for movable blocks.
     */
    void gradient_step_(const PartialPlacement& y_placement,
                        const PlacementGradient& grad,
                        const FillerState& y_fillers,
                        const FillerGradient& filler_grad,
                        double step_size,
                        PartialPlacement& next_placement,
                        FillerState& next_fillers) const;

    /**
     * @brief Apply the Nesterov extrapolation y = x_next + beta * (x_next - x).
     */
    void extrapolate_(const PartialPlacement& current,
                      const PartialPlacement& next,
                      const FillerState& current_fillers,
                      const FillerState& next_fillers,
                      double beta,
                      PartialPlacement& y_placement,
                      FillerState& y_fillers) const;

    /**
     * @brief Compute the squared norm of a placement gradient.
     */
    double gradient_norm_squared_(const PlacementGradient& grad) const;

    /**
     * @brief Compute the squared norm of a filler gradient.
     */
    double filler_gradient_norm_squared_(const FillerGradient& grad) const;

    /**
     * @brief Return the largest x-y displacement between two placements.
     */
    double max_block_displacement_(const PartialPlacement& from,
                                   const PartialPlacement& to) const;

    /**
     * @brief Return the largest x-y displacement between two filler states.
     */
    double max_filler_displacement_(const FillerState& from,
                                    const FillerState& to) const;

    /**
     * @brief Return true if the block should be moved by the continuous optimizer.
     */
    bool block_is_optimizable_(APBlockId blk_id) const;

    std::shared_ptr<FlatPlacementDensityManager> density_manager_; ///< Owns the per-tile capacity/mass model that add_density_gradient_ deposits charge onto and legalization consumes.
    std::unique_ptr<PartialLegalizer> partial_legalizer_;          ///< Repeatedly cleans up discrete architecture-legality violations (overlaps, resource mismatches) the continuous optimizer's placement leaves behind during "epochs", before full legalization.
    const AtomNetlist& atom_netlist_;                              ///< Atom netlist used for timing criticality lookup.
    PreClusterTimingManager& pre_cluster_timing_manager_;          ///< Timing manager used to weight differentiable wirelength nets.
    std::shared_ptr<PlaceDelayModel> place_delay_model_;           ///< Delay model used to update timing criticalities.
    const LogicalModels& models_;                                  ///< Logical models used to classify I/O-chain primitives.

    std::vector<APBlockId> optimizable_blocks_; ///< Movable AP blocks touched by the optimizer.
    vtr::vector<APNetId, double> net_weights_;  ///< Per-net weight applied to the weighted-average (WA) wirelength term computed in add_wirelength_gradient_.

    vtr::vector<APBlockId, double> block_precond_;                     ///< Per-block diagonal preconditioner (objective curvature estimate).
    bool precond_active_ = false;                                      ///< Whether the preconditioner is applied in the current optimization run.
    double precond_alpha_active_ = 1.0;                                ///< Preconditioner strength exponent for the current optimization run.
    std::vector<bool> boundary_confined_dims_;                         ///< [dim index] true if target capacity lies almost entirely on the device boundary.
    vtr::vector<APNetId, bool> boundary_cohesion_nets_;                ///< AP nets that receive I/O-only cohesion weight.
    vtr::vector<APNetId, bool> io_chain_cohesion_nets_;                ///< Direct I/O-chain AP nets that receive targeted cohesion weight.
    std::vector<std::vector<APBlockId>> pack_pattern_cohesion_groups_; ///< Multi-block long-chain AP block groups.
    size_t num_io_chain_cohesion_nets_ = 0;                            ///< Number of flagged direct I/O-chain AP nets.
    std::vector<double> filler_unit_mass_;                             ///< [dim] density mass per dynamic filler.
    std::vector<double> filler_precond_;                               ///< [dim] density-only filler preconditioner.

    size_t device_grid_width_ = 0;             ///< Width of the placement region.
    size_t device_grid_height_ = 0;            ///< Height of the placement region.
    size_t device_grid_num_layers_ = 0;        ///< Number of device layers.
    float ap_timing_tradeoff_ = 0.f;           ///< User timing tradeoff value.
    float effective_timing_tradeoff_ = 0.f;    ///< Timing tradeoff after design-size adaptation.
    double io_chain_net_cohesion_weight_ = 2.; ///< Weight multiplier for direct I/O-chain AP nets.
    double pack_pattern_cohesion_weight_ = 0.; ///< Weight of the optional pack-pattern chain cohesion objective.

    /// @brief B2B/QP warm-start solver. initialize_placement_ seeds the nonlinear
    ///        optimizer from a wirelength-aware analytical solve (elfPlace/ePlace
    ///        QP initialization) instead of a block-ID grid spread. Always built in
    ///        the constructor.
    std::unique_ptr<AnalyticalSolver> warmstart_solver_;
    size_t warmstart_iters_ = 0;     ///< Minimum solve+legalize cycles (warm-start floor).
    size_t warmstart_max_iters_ = 0; ///< Cap on the convergence-based warm-start loop.

    /// @brief Active wirelength-smoothing fraction (gamma / device span). Seeded at
    ///        the fixed default, then annealed coarse->sharp per epoch by
    ///        run_global_optimization_ (gamma continuation).
    double current_gamma_fraction_ = 0.02;

    /// @brief True when the warm-start seed's physical overflow is already below
    ///        the sparse gate. The electrostatic field then has nothing to spread,
    ///        so the epoch loop is capped to a cheap filler-free probe instead of
    ///        the full schedule (whose result was measured to be discarded in
    ///        favor of the seed on sparse Titanium designs, at up to 11x the
    ///        lp-b2b global-placement runtime).
    bool sparse_seed_ = false;
};
