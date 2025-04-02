/**
 * @file
 * @author  Alex Singer
 * @date    October 2024
 * @brief   The declarations of the Global Placer base class which is used to
 *          define the functionality of all global placers in the AP flow.
 *
 * A Global Placer creates a Partial Placement given only the netlist and the
 * architecture. It uses analytical techniques (i.e. efficient numerical
 * minimization of an objective function of a placement) to find a placement
 * that optimizes for objectives subject to some of the constraints of the FPGA
 * architecture.
 */

#pragma once

#include <memory>
#include "ap_flow_enums.h"
#include "flat_placement_density_manager.h"
#include "partial_legalizer.h"

// Forward declarations
class APNetlist;
class AnalyticalSolver;
class PartialLegalizer;
class Prepacker;
struct PartialPlacement;

/**
 * @brief The Global Placer base class
 *
 * This declares the functionality that all Global Placers will use. This
 * provides a standard interface for the global placers so they can be used
 * interchangably. This makes it very easy to test and compare different global
 * placers.
 */
class GlobalPlacer {
  public:
    virtual ~GlobalPlacer() {}

    /**
     * @brief Constructor of the base GlobalPlacer class
     *
     *  @param netlist      Netlist of the design at some abstraction level;
     *                      typically this would have some atoms and groups of
     *                      atoms (in a pack pattern).
     *  @param log_verbosity    The verbosity of log messages in the Global
     *                          Placer.
     */
    GlobalPlacer(const APNetlist& ap_netlist, int log_verbosity)
        : ap_netlist_(ap_netlist)
        , log_verbosity_(log_verbosity) {}

    /**
     * @brief Perform global placement on the given netlist.
     *
     * The role of a global placer is to try and find a placement for the given
     * netlist which optimizes some objective function and is mostly legal.
     */
    virtual PartialPlacement place() = 0;

  protected:
    /// @brief The APNetlist the global placer is placing.
    const APNetlist& ap_netlist_;

    /// @brief The setting of how verbose the log messages should be in the
    ///        global placer. Anything larger than zero will display per
    ///        iteration status messages.
    int log_verbosity_;
};

/**
 * @brief A factory method which creates a Global Placer of the given type.
 */
std::unique_ptr<GlobalPlacer> make_global_placer(e_ap_analytical_solver analytical_solver_type,
                                                 e_ap_partial_legalizer partial_legalizer_type,
                                                 const APNetlist& ap_netlist,
                                                 const Prepacker& prepacker,
                                                 const AtomNetlist& atom_netlist,
                                                 const DeviceGrid& device_grid,
                                                 const std::vector<t_logical_block_type>& logical_block_types,
                                                 const std::vector<t_physical_tile_type>& physical_tile_types,
                                                 int log_verbosity);

/**
 * @brief A Global Placer based on the SimPL work for analytical ASIC placement.
 *          https://doi.org/10.1145/2461256.2461279
 *
 * This placement technique uses a solver to generate a placement that optimizes
 * over some objective function and is likely very illegal (has many overlapping
 * blocks and blocks in the wrong places). This solution represents the "lower-
 * bound" on the solution quality.
 *
 * This technique passes this "lower-bound" solution into a legalizer, which
 * tries to find the closest legal solution to the lower-bound solution (by
 * spreading out blocks and placing them in legal positions). This often
 * destroys the quality of the lower-bound solution, and is considered an
 * "upper-bound" on the solution quality.
 *
 * Each iteration of this global placer, the upper-bound solution is fed into
 * the solver as a "hint" to what a legal solution looks like. This allows the
 * solver to produce another placement which will make decisions knowing where
 * the blocks will end-up in the legal solution. This worstens the quality of
 * the lower-bound solution; however, after passing this solution back into
 * the legalizer, this will likely improve the quality of the upper-bound
 * solution.
 *
 * Over several iterations the upper-bound and lower-bound solutions will
 * approach each other until a good quality, mostly-legal solution is found.
 */
class SimPLGlobalPlacer : public GlobalPlacer {
  private:
    /// @brief The maximum number of iterations the global placer can perform.
    static constexpr size_t max_num_iterations_ = 100;

    /// @brief The target relative gap between the HPWL of the upper-bound and
    ///        lower-bound placements. The placer will stop if the difference
    ///        between the two bounds, normalized to the upper-bound, is smaller
    ///        than this number.
    ///        This number was empircally found to work well.
    static constexpr double target_hpwl_relative_gap_ = 0.05;

    /// @brief The solver which generates the lower-bound placement.
    std::unique_ptr<AnalyticalSolver> solver_;

    /// @brief The denisty manager the partial legalizer will optimize over.
    std::shared_ptr<FlatPlacementDensityManager> density_manager_;

    /// @brief The legalizer which generates the upper-bound placement.
    std::unique_ptr<PartialLegalizer> partial_legalizer_;

  public:
    /**
     * @brief Constructor for the SimPL Global Placer
     *
     * Constructs the solver and partial legalizer.
     */
    SimPLGlobalPlacer(e_ap_analytical_solver analytical_solver_type,
                      e_ap_partial_legalizer partial_legalizer_type,
                      const APNetlist& ap_netlist,
                      const Prepacker& prepacker,
                      const AtomNetlist& atom_netlist,
                      const DeviceGrid& device_grid,
                      const std::vector<t_logical_block_type>& logical_block_types,
                      const std::vector<t_physical_tile_type>& physical_tile_types,
                      int log_verbosity);

    /**
     * @brief Run a SimPL-like global placement algorithm
     *
     * This iteratively runs the solver and legalizer until a good quality and
     * mostly-legal placement is found.
     */
    PartialPlacement place() final;
};
