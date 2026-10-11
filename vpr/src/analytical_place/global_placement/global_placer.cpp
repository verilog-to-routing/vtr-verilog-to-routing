/**
 * @file
 * @author  Alex Singer
 * @date    October 2024
 * @brief   The definitions of the global placers used in the AP flow and their
 *          base class.
 */

#include "global_placer.h"
#include <cstdio>
#include <limits>
#include <memory>
#include <vector>
#include "ap_draw_manager.h"
#include "PreClusterDelayCalculator.h"
#include "PreClusterTimingManager.h"
#include "analytical_solver.h"
#include "ap_flow_enums.h"
#include "ap_netlist.h"
#include "ap_netlist_fwd.h"
#include "ap_timing_estimation.h"
#include "atom_netlist.h"
#include "device_grid.h"
#include "flat_placement_bins.h"
#include "flat_placement_density_manager.h"
#include "globals.h"
#include "gp_solution_quality_estimation.h"
#include "logic_types.h"
#include "partial_legalizer.h"
#include "partial_placement.h"
#include "physical_types.h"
#include "place_delay_model.h"
#include "primitive_vector.h"
#include "timing_info.h"
#include "vtr_log.h"
#include "vtr_time.h"

std::unique_ptr<GlobalPlacer> make_global_placer(e_ap_analytical_solver analytical_solver_type,
                                                 e_ap_solver_threading solver_threading,
                                                 e_ap_partial_legalizer partial_legalizer_type,
                                                 const APNetlist& ap_netlist,
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
                                                 unsigned num_threads,
                                                 int log_verbosity) {
    return std::make_unique<SimPLGlobalPlacer>(analytical_solver_type,
                                               solver_threading,
                                               partial_legalizer_type,
                                               ap_netlist,
                                               prepacker,
                                               atom_netlist,
                                               device_grid,
                                               logical_block_types,
                                               physical_tile_types,
                                               models,
                                               pre_cluster_timing_manager,
                                               place_delay_model,
                                               ap_timing_tradeoff,
                                               generate_mass_report,
                                               target_density_arg_strs,
                                               num_threads,
                                               log_verbosity);
}

SimPLGlobalPlacer::SimPLGlobalPlacer(e_ap_analytical_solver analytical_solver_type,
                                     e_ap_solver_threading solver_threading,
                                     e_ap_partial_legalizer partial_legalizer_type,
                                     const APNetlist& ap_netlist,
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
                                     unsigned num_threads,
                                     int log_verbosity)
    : GlobalPlacer(ap_netlist, log_verbosity)
    , pre_cluster_timing_manager_(pre_cluster_timing_manager)
    , place_delay_model_(place_delay_model)
    , atom_netlist_(atom_netlist)
    , prepacker_(prepacker) {
    // This can be a long method. Good to time this to see how long it takes to
    // construct the global placer.
    vtr::ScopedStartFinishTimer global_placer_building_timer("Constructing Global Placer");

    // Build the arc delay estimator used to update the timing information.
    if (pre_cluster_timing_manager_.is_valid()) {
        VTR_LOGV(log_verbosity_ >= 10, "\tBuilding the arc delay estimator...\n");
        arc_delay_estimator_ = std::make_unique<FlatPlacementArcDelayEstimator>(ap_netlist_,
                                                                                *pre_cluster_timing_manager_.get_delay_calculator_ptr(),
                                                                                *place_delay_model_,
                                                                                device_grid);
    }

    // Build the solver.
    VTR_LOGV(log_verbosity_ >= 10, "\tBuilding the solver...\n");
    solver_ = make_analytical_solver(analytical_solver_type,
                                     ap_netlist_,
                                     device_grid,
                                     atom_netlist,
                                     pre_cluster_timing_manager_,
                                     place_delay_model_,
                                     ap_timing_tradeoff,
                                     num_threads,
                                     solver_threading,
                                     log_verbosity_);

    // Build the density manager used by the partial legalizer.
    VTR_LOGV(log_verbosity_ >= 10, "\tBuilding the density manager...\n");
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

    // Build the partial legalizer
    VTR_LOGV(log_verbosity_ >= 10, "\tBuilding the partial legalizer...\n");
    partial_legalizer_ = make_partial_legalizer(partial_legalizer_type,
                                                ap_netlist_,
                                                density_manager_,
                                                prepacker,
                                                models,
                                                log_verbosity_);
}

/**
 * @brief Helper method to print the statistics on the given partial placement.
 */
static void print_placement_stats(const PartialPlacement& p_placement,
                                  const APNetlist& ap_netlist,
                                  FlatPlacementDensityManager& density_manager,
                                  PreClusterTimingManager& pre_cluster_timing_manager,
                                  const FlatPlacementArcDelayEstimator* arc_delay_estimator) {
    // Print the placement HPWL
    VTR_LOG("\tPlacement objective HPWL: %f\n", p_placement.get_hpwl(ap_netlist));
    VTR_LOG("\tPlacement estimated wirelength: %g\n", estimate_post_routing_wire_usage(p_placement, ap_netlist, g_vpr_ctx.device().grid));

    // Print the estimated post-routing timing.
    // NOTE: The estimate restores the arc delays in the delay calculator before
    //       returning, so this does not affect the timing manager's timing info.
    if (pre_cluster_timing_manager.is_valid()) {
        VTR_ASSERT(arc_delay_estimator != nullptr);
        t_ap_timing_estimate timing_estimate = estimate_post_routing_timing(p_placement,
                                                                            *arc_delay_estimator,
                                                                            pre_cluster_timing_manager.get_delay_calculator_ptr());
        VTR_LOG("\tPlacement estimated CPD: %f ns\n", timing_estimate.cpd * 1e9);
        VTR_LOG("\tPlacement estimated sTNS: %f ns\n", timing_estimate.stns * 1e9);
        VTR_LOG("\tPlacement estimated sWNS: %f ns\n", timing_estimate.swns * 1e9);
    }

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
    VTR_LOG("----  ----------------  ----------------  -----------  ------------  -----------  --------------  ------------------  ----------\n");
    VTR_LOG("Iter  Lower Bound HPWL  Upper Bound HPWL  UB CPD (ns)  UB sTNS (ns)  Solver Time  Legalizer Time  Timing Update Time  Total Time\n");
    VTR_LOG("                                                                           (sec)           (sec)               (sec)       (sec)\n");
    VTR_LOG("----  ----------------  ----------------  -----------  ------------  -----------  --------------  ------------------  ----------\n");
}

/**
 * @brief Helper method to print the per-iteration status of the global placer.
 */
static void print_SimPL_status(size_t iteration,
                               double lb_hpwl,
                               double ub_hpwl,
                               float cpd,
                               float stns,
                               float solver_time,
                               float legalizer_time,
                               float timing_update_time,
                               float total_time) {
    // Iteration
    VTR_LOG("%4zu", iteration);

    // Lower Bound HPWL
    VTR_LOG("  %16.2f", lb_hpwl);

    // Upper Bound HPWL
    VTR_LOG("  %16.2f", ub_hpwl);

    // Upper Bound CPD (ns)
    VTR_LOG("  %11.3f", cpd);

    // Upper Bound sTNS (ns)
    VTR_LOG("  %12g", stns);

    // Solver runtime
    VTR_LOG("  %11.3f", solver_time);

    // Legalizer runtime
    VTR_LOG("  %14.3f", legalizer_time);

    // Timing update runtime
    VTR_LOG("  %18.3f", timing_update_time);

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
    // Initialize the partial placement object.
    PartialPlacement p_placement(ap_netlist_);

    float total_time_spent_in_solver = 0.0f;
    float total_time_spent_in_legalizer = 0.0f;
    float total_time_spent_updating_timing = 0.0f;

    // Create a partial placement object to store the best placement found during
    // global placement. It is possible for the global placement to hit a minimum
    // in the middle of its iterations, this lets us keep that solution.
    PartialPlacement best_p_placement(ap_netlist_);
    double best_ub_hpwl = std::numeric_limits<double>::max();

    // Initialize graphics for analytical placement, setting the references in the draw state.
    APDrawManager draw_manager(atom_netlist_, ap_netlist_, prepacker_, p_placement);

    // Pause to show initial FPGA state before any solving begins. Also pass in the pre-cluster timing manager
    // so that the drawing code can prepare for critical path drawing.
    draw_manager.pause_at_initial_scene("Analytical Placement: Starting Global Placement", pre_cluster_timing_manager_);

    // Run the global placer.
    for (size_t i = 0; i < max_num_iterations_; i++) {
        float iter_start_time = runtime_timer.elapsed_sec();

        // Run the solver.
        float solver_start_time = runtime_timer.elapsed_sec();
        solver_->solve(i, p_placement);
        float solver_end_time = runtime_timer.elapsed_sec();
        double lb_hpwl = p_placement.get_hpwl(ap_netlist_);

        // Update graphics after analytical solver
        draw_manager.update_graphics(i, APDrawType::Solver);

        // Run the legalizer.
        float legalizer_start_time = runtime_timer.elapsed_sec();
        partial_legalizer_->legalize(p_placement);
        float legalizer_end_time = runtime_timer.elapsed_sec();
        double ub_hpwl = p_placement.get_hpwl(ap_netlist_);

        // Perform a timing update
        float timing_update_start_time = runtime_timer.elapsed_sec();
        update_timing_info_with_flat_placement(pre_cluster_timing_manager_,
                                               arc_delay_estimator_.get(),
                                               p_placement);
        solver_->update_net_weights(pre_cluster_timing_manager_);
        float timing_update_end_time = runtime_timer.elapsed_sec();

        // Update graphics after legalizer and timing update
        draw_manager.update_graphics(i, APDrawType::Legalizer);

        total_time_spent_in_solver += solver_end_time - solver_start_time;
        total_time_spent_in_legalizer += legalizer_end_time - legalizer_start_time;
        total_time_spent_updating_timing += timing_update_end_time - timing_update_start_time;

        // Print some stats
        if (log_verbosity_ >= 1) {
            float cpd_ns = -1.0f;
            float stns_ns = -1.0f;
            if (pre_cluster_timing_manager_.is_valid()) {
                cpd_ns = pre_cluster_timing_manager_.get_timing_info().least_slack_critical_path().delay() * 1e9;
                stns_ns = pre_cluster_timing_manager_.get_timing_info().setup_total_negative_slack() * 1e9;
            }

            float iter_end_time = runtime_timer.elapsed_sec();
            print_SimPL_status(i, lb_hpwl, ub_hpwl,
                               cpd_ns,
                               stns_ns,
                               solver_end_time - solver_start_time,
                               legalizer_end_time - legalizer_start_time,
                               timing_update_end_time - timing_update_start_time,
                               iter_end_time - iter_start_time);
        }

        // If this placement is better than the best we have seen, save it.
        // TODO: This is not correct for timing. We want to use a costing
        //       function that takes timing into account as well. We do not just
        //       want the lowest HPWL.
        if (ub_hpwl < best_ub_hpwl) {
            best_ub_hpwl = ub_hpwl;
            best_p_placement = p_placement;
        }

        // Exit condition: If the upper-bound and lower-bound HPWLs are
        // sufficiently close together then stop.
        double hpwl_gap = ub_hpwl - lb_hpwl;
        double hpwl_relative_gap;
        if (ub_hpwl != 0.0)
            hpwl_relative_gap = hpwl_gap / ub_hpwl;
        else if (lb_hpwl != 0.0)
            hpwl_relative_gap = hpwl_gap / lb_hpwl;
        else
            hpwl_relative_gap = 0.0;

        if (hpwl_relative_gap < target_hpwl_relative_gap_)
            break;
    }

    // Update the setup slacks. This is performed down here (as well as being
    // inside the GP loop) since the best_p_placement may not be the p_placement
    // from the last iteration of GP.
    update_timing_info_with_flat_placement(pre_cluster_timing_manager_,
                                           arc_delay_estimator_.get(),
                                           best_p_placement);

    // Print statistics on the solver used.
    solver_->print_statistics();

    // Print statistics on the partial legalizer used.
    partial_legalizer_->print_statistics();

    VTR_LOG("Global Placer Statistics:\n");
    VTR_LOG("\tTime spent in solver: %g seconds\n", total_time_spent_in_solver);
    VTR_LOG("\tTime spent in legalizer: %g seconds\n", total_time_spent_in_legalizer);
    VTR_LOG("\tTime spent updating timing: %g seconds\n", total_time_spent_updating_timing);

    // Pause to show the final global placement result before handing off.
    draw_manager.pause_at_final_scene("Analytical Placement: Global Placement Complete");

    // Print some statistics on the final placement.
    VTR_LOG("Placement after Global Placement:\n");
    print_placement_stats(best_p_placement,
                          ap_netlist_,
                          *density_manager_,
                          pre_cluster_timing_manager_,
                          arc_delay_estimator_.get());

    // Return the placement from the final iteration.
    return best_p_placement;
}
