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
#include "ap_netlist_fwd.h"
#include "atom_netlist.h"
#include "device_grid.h"
#include "flat_placement_bins.h"
#include "flat_placement_density_manager.h"
#include "globals.h"
#include "partial_legalizer.h"
#include "partial_placement.h"
#include "physical_types.h"
#include "primitive_vector.h"
#include "vtr_log.h"
#include "vtr_time.h"

std::unique_ptr<GlobalPlacer> make_global_placer(e_ap_analytical_solver analytical_solver_type,
                                                 e_ap_partial_legalizer partial_legalizer_type,
                                                 const APNetlist& ap_netlist,
                                                 const Prepacker& prepacker,
                                                 const AtomNetlist& atom_netlist,
                                                 const DeviceGrid& device_grid,
                                                 const std::vector<t_logical_block_type>& logical_block_types,
                                                 const std::vector<t_physical_tile_type>& physical_tile_types,
                                                 int log_verbosity) {
    return std::make_unique<SimPLGlobalPlacer>(analytical_solver_type,
                                               partial_legalizer_type,
                                               ap_netlist,
                                               prepacker,
                                               atom_netlist,
                                               device_grid,
                                               logical_block_types,
                                               physical_tile_types,
                                               log_verbosity);
}

SimPLGlobalPlacer::SimPLGlobalPlacer(e_ap_analytical_solver analytical_solver_type,
                                     e_ap_partial_legalizer partial_legalizer_type,
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
    VTR_LOGV(log_verbosity_ >= 10, "\tBuilding the solver...\n");
    solver_ = make_analytical_solver(analytical_solver_type,
                                     ap_netlist_,
                                     device_grid,
                                     log_verbosity_);

    // Build the density manager used by the partial legalizer.
    VTR_LOGV(log_verbosity_ >= 10, "\tBuilding the density manager...\n");
    density_manager_ = std::make_shared<FlatPlacementDensityManager>(ap_netlist_,
                                                                     prepacker,
                                                                     atom_netlist,
                                                                     device_grid,
                                                                     logical_block_types,
                                                                     physical_tile_types,
                                                                     log_verbosity_);

    // Build the partial legalizer
    VTR_LOGV(log_verbosity_ >= 10, "\tBuilding the partial legalizer...\n");
    partial_legalizer_ = make_partial_legalizer(partial_legalizer_type,
                                                ap_netlist_,
                                                density_manager_,
                                                prepacker,
                                                log_verbosity_);
}

/**
 * @brief Helper method to print the statistics on the given partial placement.
 */
static void print_placement_stats(const PartialPlacement& p_placement,
                                  const APNetlist& ap_netlist,
                                  FlatPlacementDensityManager& density_manager) {
    // Print the placement HPWL
    VTR_LOG("\tPlacement HPWL: %f\n", p_placement.get_hpwl(ap_netlist));

    // Print density information. Need to reset the density manager to ensure
    // the data is valid.
    density_manager.import_placement_into_bins(p_placement);

    // Print the number of overfilled bins.
    size_t num_overfilled_bins = density_manager.get_overfilled_bins().size();
    VTR_LOG("\tNumber of overfilled bins: %zu\n", num_overfilled_bins);

    // Print the average overfill
    float total_overfill = 0.0f;
    for (FlatPlacementBinId bin_id : density_manager.get_overfilled_bins()) {
        total_overfill += density_manager.get_bin_overfill(bin_id).manhattan_norm();
    }
    float avg_overfill = 0.0f;
    if (num_overfilled_bins != 0)
        avg_overfill = total_overfill / static_cast<float>(num_overfilled_bins);
    VTR_LOG("\tAverage overfill magnitude: %f\n", avg_overfill);

    // Print the number of overfilled tiles per type.
    const auto& physical_tile_types = g_vpr_ctx.device().physical_tile_types;
    const auto& device_grid = g_vpr_ctx.device().grid;
    std::vector<unsigned> overfilled_tiles_by_type(physical_tile_types.size(), 0);
    for (FlatPlacementBinId bin_id : density_manager.get_overfilled_bins()) {
        const auto& bin_region = density_manager.flat_placement_bins().bin_region(bin_id);
        auto tile_loc = t_physical_tile_loc((int)bin_region.xmin(),
                                            (int)bin_region.ymin(),
                                            0);
        auto tile_type = device_grid.get_physical_type(tile_loc);
        overfilled_tiles_by_type[tile_type->index]++;
    }
    VTR_LOG("\tOverfilled bins by tile type:\n");
    for (size_t type_idx = 0; type_idx < physical_tile_types.size(); type_idx++) {
        VTR_LOG("\t\t%10s: %zu\n",
                physical_tile_types[type_idx].name.c_str(),
                overfilled_tiles_by_type[type_idx]);
    }

    // Count the number of blocks that were placed in a bin which they cannot
    // physically be placed into (according to their mass).
    unsigned num_misplaced_blocks = 0;
    for (FlatPlacementBinId bin_id : density_manager.get_overfilled_bins()) {
        for (APBlockId ap_blk_id : density_manager.flat_placement_bins().bin_contained_blocks(bin_id)) {
            // Get the blk mass and project it onto the capacity of its bin.
            PrimitiveVector blk_mass = density_manager.mass_calculator().get_block_mass(ap_blk_id);
            PrimitiveVector projected_mass = blk_mass;
            projected_mass.project(density_manager.get_bin_capacity(bin_id));
            // If the projected mass does not match its match, this implies that
            // there this block does not belong in this bin.
            if (projected_mass != blk_mass)
                num_misplaced_blocks++;
        }
    }
    VTR_LOG("\tNumber of blocks in an incompatible bin: %zu\n", num_misplaced_blocks);
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

    float total_time_spent_in_solver = 0.0f;
    float total_time_spent_in_legalizer = 0.0f;

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

        total_time_spent_in_solver += solver_end_time - solver_start_time;
        total_time_spent_in_legalizer += legalizer_end_time - legalizer_start_time;

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

    // Print statistics on the solver used.
    solver_->print_statistics();

    // Print statistics on the partial legalizer used.
    partial_legalizer_->print_statistics();

    VTR_LOG("Global Placer Statistics:\n");
    VTR_LOG("\tTime spent in solver: %g seconds\n", total_time_spent_in_solver);
    VTR_LOG("\tTime spent in legalizer: %g seconds\n", total_time_spent_in_legalizer);

    // Print some statistics on the final placement.
    VTR_LOG("Placement after Global Placement:\n");
    print_placement_stats(p_placement,
                          ap_netlist_,
                          *density_manager_);

    // Return the placement from the final iteration.
    // TODO: investigate saving the best solution found so far. It should be
    //       cheap to save a copy of the PartialPlacement object.
    return p_placement;
}
