/**
 * @file
 * @author  Alex Singer
 * @date    October 2024
 * @brief   The definitions of the global placers used in the AP flow and their
 *          base class.
 */

#include "global_placer.h"
#include <cstdio>
#include <memory>
#include <vector>
#include "analytical_solver.h"
#include "ap_flow_enums.h"
#include "ap_netlist.h"
#include "atom_netlist.h"
#include "device_grid.h"
#include "flat_placement_density_manager.h"
#include "partial_legalizer.h"
#include "partial_placement.h"
#include "physical_types.h"
#include "vpr_error.h"
#include "vtr_log.h"
#include "vtr_time.h"

std::unique_ptr<GlobalPlacer> make_global_placer(e_ap_global_placer placer_type,
                                                 const APNetlist& ap_netlist,
                                                 const Prepacker& prepacker,
                                                 const AtomNetlist& atom_netlist,
                                                 const DeviceGrid& device_grid,
                                                 const std::vector<t_logical_block_type>& logical_block_types,
                                                 const std::vector<t_physical_tile_type>& physical_tile_types,
                                                 int log_verbosity) {
    // Based on the placer type passed in, build the global placer.
    switch (placer_type) {
        case e_ap_global_placer::SimPL_BiParitioning:
            return std::make_unique<SimPLGlobalPlacer>(e_partial_legalizer::BI_PARTITIONING,
                                                       ap_netlist,
                                                       prepacker,
                                                       atom_netlist,
                                                       device_grid,
                                                       logical_block_types,
                                                       physical_tile_types,
                                                       log_verbosity);
        case e_ap_global_placer::SimPL_FlowBased:
            return std::make_unique<SimPLGlobalPlacer>(e_partial_legalizer::FLOW_BASED,
                                                       ap_netlist,
                                                       prepacker,
                                                       atom_netlist,
                                                       device_grid,
                                                       logical_block_types,
                                                       physical_tile_types,
                                                       log_verbosity);
        default:
            VPR_FATAL_ERROR(VPR_ERROR_AP,
                            "Unrecognized global placer type");

    }
}

SimPLGlobalPlacer::SimPLGlobalPlacer(e_partial_legalizer partial_legalizer_type,
                                     const APNetlist& ap_netlist,
                                     const Prepacker& prepacker,
                                     const AtomNetlist& atom_netlist,
                                     const DeviceGrid& device_grid,
                                     const std::vector<t_logical_block_type>& logical_block_types,
                                     const std::vector<t_physical_tile_type>& physical_tile_types,
                                     int log_verbosity)
                                        : GlobalPlacer(ap_netlist, log_verbosity) {
    // This can be a long method. Good to time this to see how long it takes to
    // construct the global placer.
    vtr::ScopedStartFinishTimer global_placer_building_timer("Constructing Global Placer");
    // Build the solver.
    solver_ = make_analytical_solver(e_analytical_solver::QP_HYBRID,
                                     ap_netlist_);
    // Build the density manager used by the partial legalizer.
    density_manager_ = std::make_shared<FlatPlacementDensityManager>(ap_netlist_,
                                                                     prepacker,
                                                                     atom_netlist,
                                                                     device_grid,
                                                                     logical_block_types,
                                                                     physical_tile_types,
                                                                     log_verbosity_);
    // Build the partial legalizer
    partial_legalizer_ = make_partial_legalizer(partial_legalizer_type,
                                                ap_netlist_,
                                                density_manager_,
                                                log_verbosity_);
}

/**
 * @brief Helper method to print the header of the per-iteration status updates
 *        of the global placer.
 */
static void print_SimPL_status_header() {
    VTR_LOG("----  ----------------  ----------------  -----------  --------------  ----------\n");
    VTR_LOG("Iter  Lower Bound HPWL  Upper Bound HPWL  Solver Time  Legalizer Time  Total Time\n");
    VTR_LOG("                                                (sec)           (sec)       (sec)\n");
    VTR_LOG("----  ----------------  ----------------  -----------  --------------  ----------\n");
}

/**
 * @brief Helper method to print the per-iteration status of the global placer.
 */
static void print_SimPL_status(size_t iteration,
                               double lb_hpwl,
                               double ub_hpwl,
                               float solver_time,
                               float legalizer_time,
                               float total_time) {
    // Iteration
    VTR_LOG("%4zu", iteration);

    // Lower Bound HPWL
    VTR_LOG("  %16.2f", lb_hpwl);

    // Upper Bound HPWL
    VTR_LOG("  %16.2f", ub_hpwl);

    // Solver runtime
    VTR_LOG("  %11.3f", solver_time);

    // Legalizer runtime
    VTR_LOG("  %14.3f", legalizer_time);

    // Total runtime
    VTR_LOG("  %10.3f", total_time);

    VTR_LOG("\n");

    fflush(stdout);
}

PartialPlacement SimPLGlobalPlacer::place() {
    // Create a timer to time the entire global placement time.
    vtr::ScopedStartFinishTimer global_placer_time("AP Global Placer");
    // Create a timer to keep track of how long the solver and legalizer take.
    vtr::Timer runtime_timer;
    // Print the status header.
    if (log_verbosity_ >= 1)
        print_SimPL_status_header();
    // Initialialize the partial placement object.
    PartialPlacement p_placement(ap_netlist_);
    // Run the global placer.
    for (size_t i = 0; i < max_num_iterations_; i++) {
        float iter_start_time = runtime_timer.elapsed_sec();

        // Run the solver.
        float solver_start_time = runtime_timer.elapsed_sec();
        solver_->solve(i, p_placement);
        float solver_end_time = runtime_timer.elapsed_sec();
        double lb_hpwl = p_placement.get_hpwl(ap_netlist_);

        // Run the legalizer.
        float legalizer_start_time = runtime_timer.elapsed_sec();
        partial_legalizer_->legalize(p_placement);
        float legalizer_end_time = runtime_timer.elapsed_sec();
        double ub_hpwl = p_placement.get_hpwl(ap_netlist_);

        // Print some stats
        if (log_verbosity_ >= 1) {
            float iter_end_time = runtime_timer.elapsed_sec();
            print_SimPL_status(i, lb_hpwl, ub_hpwl,
                               solver_end_time - solver_start_time,
                               legalizer_end_time - legalizer_start_time,
                               iter_end_time - iter_start_time);
        }

        // Exit condition: If the upper-bound and lower-bound HPWLs are
        // sufficiently close together then stop.
        double hpwl_relative_gap = (ub_hpwl - lb_hpwl) / ub_hpwl;
        if (hpwl_relative_gap < target_hpwl_relative_gap_)
            break;
    }
    // Return the placement from the final iteration.
    // TODO: investigate saving the best solution found so far. It should be
    //       cheap to save a copy of the PartialPlacement object.
    return p_placement;
}

