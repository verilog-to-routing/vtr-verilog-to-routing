/**
 * @file placer.h
 * @brief Declares the Placer class, which encapsulates the functionality, data structures,
 * and algorithms required for the placement stage.
 *
 * The Placer class initializes necessary objects, performs an initial placement,
 * and runs simulated annealing optimization. This optimization minimizes
 * wirelength (bounding box) and timing costs to achieve an efficient placement solution.
 *
 * Key features of the Placer class:
 * - Encapsulates all placement-related variables, cost functions, and data structures.
 * - Supports optional NoC (Network-on-Chip) cost optimizations if enabled.
 * - Interfaces with timing analysis, placement delay calculation.
 * - Provides a mechanism for checkpointing the placement state.
 * - Includes debugging and validation utilities to verify the correctness of placement.
 */

#pragma once

#include <memory>
#include <optional>

#include "timing_place.h"
#include "place_checkpoint.h"
#include "PlacementDelayCalculator.h"
#include "placer_state.h"
#include "noc_place_utils.h"
#include "net_cost_handler.h"
#include "place_log_util.h"

class PlacementAnnealer;
namespace vtr{
class ScopedStartFinishTimer;
}

class Placer {
  public:
    Placer(const Netlist<>& net_list,
           const t_placer_opts& placer_opts,
           const t_analysis_opts& analysis_opts,
           const t_noc_opts& noc_opts,
           const std::vector<t_direct_inf>& directs,
           std::shared_ptr<PlaceDelayModel> place_delay_model,
           bool cube_bb,
           bool is_flat,
           bool quiet);

    void place();

    /**
     * @brief Copies the placement location variables into the given global placement context.
     * @param place_ctx The placement context to which location information will be copied.
     */
    void copy_locs_to_global_state(PlacementContext& place_ctx);

    /*
     * Getters
     */
    const PlacementAnnealer& annealer() const;

    const t_placer_opts& placer_opts() const;

    const t_noc_opts& noc_opts() const;

    const t_placer_costs& costs() const;

    const tatum::TimingPathInfo& critical_path() const;

    std::shared_ptr<const SetupTimingInfo> timing_info() const;

    const PlacerState& placer_state() const;

    const std::optional<NocCostHandler>& noc_cost_handler() const;

  private:
    /// Holds placement algorithm parameters
    const t_placer_opts& placer_opts_;
    /// Holds timing analysis parameters
    const t_analysis_opts& analysis_opts_;
    /// Holds NoC-related parameters
    const t_noc_opts& noc_opts_;
    /// Placement cost terms with their normalization factors and total cost
    t_placer_costs costs_;
    /// Holds timing, runtime, and block location information
    PlacerState placer_state_;
    /// Random number generator used to select random blocks and locations
    vtr::RngContainer rng_;
    /// Computes and updates net bounding box cost
    NetCostHandler net_cost_handler_;
    /// Compute and updates NoC-related cost terms if NoC optimization is enabled
    std::optional<NocCostHandler> noc_cost_handler_;
    /// A delay model shared between multiple instances of this class.
    std::shared_ptr<PlaceDelayModel> place_delay_model_;
    /// Prints logs during placement
    const PlacementLogPrinter log_printer_;
    /// Indicates if flat routing resource graph and delay model is used. It should be false.
    const bool is_flat_;

    /// Stores a placement state as a retrievable checkpoint in case the placement quality deteriorates later.
    t_placement_checkpoint placement_checkpoint_;

    std::shared_ptr<SetupTimingInfo> timing_info_;
    std::shared_ptr<PlacementDelayCalculator> placement_delay_calc_;
    std::unique_ptr<PlacerSetupSlacks> placer_setup_slacks_;
    std::unique_ptr<PlacerCriticalities> placer_criticalities_;
    std::unique_ptr<NetPinTimingInvalidator> pin_timing_invalidator_;
    tatum::TimingPathInfo critical_path_;

    std::unique_ptr<vtr::ScopedStartFinishTimer> timer_;

    IntraLbPbPinLookup pb_gpin_lookup_;
    ClusteredPinAtomPinsLookup netlist_pin_lookup_;

    std::unique_ptr<PlacementAnnealer> annealer_;

    t_timing_analysis_profile_info pre_place_timing_stats_;
    t_timing_analysis_profile_info pre_quench_timing_stats_;
    t_timing_analysis_profile_info post_quench_timing_stats_;

    friend void PlacementLogPrinter::print_post_placement_stats() const;

  private:
    void alloc_and_init_timing_objects_(const Netlist<>& net_list,
                                        const t_analysis_opts& analysis_opts);

    /**
     * Checks that the placement has not confused our data structures.
     * i.e. the clb and block structures agree about the locations of
     * every block, blocks are in legal spots, etc.  Also recomputes
     * the final placement cost from scratch and makes sure it is
     * within round-off of what we think the cost is.
     */
    void check_place_();

    /**
     * Computes bounding box and timing cost to ensure it is
     * within a small error margin what we thing the cost is.
     * @return Number cost elements, i.e. BB and timing, that falls
     * outside the acceptable round-off error margin.
     */
    int check_placement_costs_();
};

