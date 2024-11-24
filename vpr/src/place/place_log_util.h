/**
 * @file placement_log_printer.h
 * @brief Declares the PlacementLogPrinter class and associated utilities for logging
 * and reporting placement-related statistics and timing analysis results.
 *
 * This file provides tools to monitor and report the progress and results of the placement stage.
 *
 * ### Key Components:
 * - **PlacementLogPrinter**:
 *   - A utility class for logging placement status, resource utilization, and swap statistics.
 *   - Prints detailed statistics during the placement process, including initial and post-placement states.
 *   - Supports a "quiet mode" to suppress output.
 *
 * ### Integration:
 * The tools in this file integrate with the Placer class to provide information about
 * the placement process for debugging, optimization, and analysis purposes.
 */

#pragma once

#include <cstddef>
#include <vector>

#include "timing_info_fwd.h"
#include "PlacementDelayCalculator.h"

class t_annealing_state;
class t_placer_statistics;
struct t_placer_opts;
struct t_analysis_opts;
struct NocCostTerms;
struct t_swap_stats;
class BlkLocRegistry;
class Placer;

class PlacementLogPrinter {
  public:
    explicit PlacementLogPrinter(const Placer& placer, bool quiet);
    PlacementLogPrinter(const PlacementLogPrinter&) = delete;
    PlacementLogPrinter& operator=(const PlacementLogPrinter&) = delete;

    void print_place_status_header() const;
    void print_resources_utilization() const;
    void print_placement_swaps_stats() const;
    void print_place_status(float elapsed_sec) const;
    void print_initial_placement_stats() const;
    void print_post_placement_stats() const;

  private:
    const Placer& placer_;
    const bool quiet_;
    mutable std::vector<char> msg_;
};

void generate_post_place_timing_reports(const t_placer_opts& placer_opts,
                                        const t_analysis_opts& analysis_opts,
                                        const SetupTimingInfo& timing_info,
                                        const PlacementDelayCalculator& delay_calc,
                                        bool is_flat,
                                        const BlkLocRegistry& blk_loc_registry);
