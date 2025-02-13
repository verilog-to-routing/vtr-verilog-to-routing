/**
 * @file placer.h
 * @brief Declares the Placer class, which encapsulates the functionality, data structures,
 * and algorithms required for the (annealing-based) placement stage
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

#include "place_checkpoint.h"
#include "PlacementDelayCalculator.h"
#include "placer_state.h"
#include "noc_place_utils.h"
#include "net_cost_handler.h"
#include "placement_log_printer.h"
#include "PlacerSetupSlacks.h"
#include "PlacerCriticalities.h"
#include "NetPinTimingInvalidator.h"

class FlatPlacementInfo;
class PlaceMacros;
class PlacementAnnealer;
namespace vtr{
class ScopedStartFinishTimer;
}

class Placer {
  public:
    Placer(const Netlist<>& net_list,
           const PlaceMacros& place_macros,
           const t_placer_opts& placer_opts,
           const t_analysis_opts& analysis_opts,
           const t_noc_opts& noc_opts,
           const IntraLbPbPinLookup& pb_gpin_lookup,
           const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
           const FlatPlacementInfo& flat_placement_info,
           std::shared_ptr<PlaceDelayModel> place_delay_model,
           bool cube_bb,
           bool is_flat,
           bool quiet);

    /**
     * @brief Executes the simulated annealing algorithm to optimize placement.
     *
     * This function minimizes placement costs, including bounding box and timing costs,
     * using simulated annealing. During the process, it periodically updates timing information
     * and saves a checkpoint of the best placement encountered.
     *
     * After the simulated annealing completes, the final placement is evaluated against the
     * checkpoint. If the final placement's quality is worse than the checkpoint, the checkpoint
     * is restored. The final placement is then validated for legality.
     */
    void place();

    /**
     * @brief Copies the placement location variables into the given global placement context.
     * @param place_ctx The placement context to which location information will be copied.
     */
    void copy_locs_to_global_state(PlacementContext& place_ctx);

  private:
    const PlaceMacros& place_macros_;
    /// Holds placement algorithm parameters
    const t_placer_opts& placer_opts_;
    /// Holds timing analysis parameters
    const t_analysis_opts& analysis_opts_;
    /// Holds NoC-related parameters
    const t_noc_opts& noc_opts_;
    /// Enables fast look-up pb graph pins from block pin indices
    const IntraLbPbPinLookup& pb_gpin_lookup_;
    /// Enables fast look-up of atom pins connect to CLB pins
    const ClusteredPinAtomPinsLookup& netlist_pin_lookup_;
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
    /**
     * @brief Holds a setup timing analysis engine.
     * Other placement timing objects like PlacerSetupSlacks, PlacerCriticalities, and NetPinTimingInvalidator
     * have a pointer to timing_info. A shared pointer is used to manage the lifetime of the object.
     */
    std::shared_ptr<SetupTimingInfo> timing_info_;
    /// Post-clustering delay calculator. Its API allows extraction of delay for each timing edge.
    std::shared_ptr<PlacementDelayCalculator> placement_delay_calc_;
    /// Stores setup slack of the clustered netlist connections.
    std::unique_ptr<PlacerSetupSlacks> placer_setup_slacks_;
    /// Stores criticalities of the clustered netlist connections.
    std::unique_ptr<PlacerCriticalities> placer_criticalities_;
    /// Used to invalidate timing edges corresponding to the pins of moved blocks.
    std::unique_ptr<NetPinTimingInvalidator> pin_timing_invalidator_;
    /// Stores information about the critical path. This is usually updated after that timing info is updated.
    tatum::TimingPathInfo critical_path_;

    /// Performs random swaps and implements the simulated annealer optimizer.
    std::unique_ptr<PlacementAnnealer> annealer_;

    /* These variables store timing analysis profiling information
     * at different stages of the placement to be printed at the end
     */
    t_timing_analysis_profile_info pre_place_timing_stats_;
    t_timing_analysis_profile_info pre_quench_timing_stats_;
    t_timing_analysis_profile_info post_quench_timing_stats_;

    /* PlacementLogPrinter is made a friend of this class, so it can
     * access its private member variables without getter methods.
     * PlacementLogPrinter holds a constant reference to an object of type
     * Placer to avoid modifying its member variables.
     */
    friend class PlacementLogPrinter;

  private:
    /**
     * @brief Constructs and initializes timing-related objects.
     *
     * This function performs the following steps to set up timing analysis:
     *
     * 1. Constructs a `tatum::DelayCalculator` for post-clustering delay calculations.
     *    This calculator holds a reference to `PlacerTimingContext::connection_delay`,
     *    which contains net delays based on block locations.
     *
     * 2. Creates and stores a `SetupTimingInfo` object in `timing_info_`.
     *    This object utilizes the delay calculator to compute delays on timing edges
     *    and calculate setup times.
     *
     * 3. Constructs `PlacerSetupSlacks` and `PlacerCriticalities` objects,
     *    which translate arrival and required times into slacks and criticalities,
     *    respectively. These objects hold pointers to timing_info_.
     *
     * 4. Creates a `NetPinTimingInvalidator` object to mark timing edges
     *    corresponding to the pins of moved blocks as invalid. This object
     *    holds a pointer to timing_info_.
     *
     * 5. Performs a full timing analysis by marking all pins as invalid.
     *
     * @param net_list The netlist used for iterating over pins.
     * @param analysis_opts Analysis options, including whether to echo the timing graph.
     */
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
