#pragma once
/**
 * @file placement_log_printer.h
 * @brief Declares the PlacementLogPrinter class and associated utilities for logging
 * and reporting placement-related statistics and timing analysis results.
 *
 * ### Integration:
 * The PlacementLogPrinter class integrates with the Placer class to provide information about
 * the placement process for debugging, optimization, and analysis purposes.
 */

#include <functional>
#include <string>
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

/**
 * @brief One placement status column: header formatting plus a callable
 * that returns the value as a formatted string.
 */
struct PlaceStatusColumn {
    std::string dash;
    std::string header1;
    std::string header2;
    std::function<std::string(float elapsed_sec)> format_value;
};

/**
 * @class PlacementLogPrinter
 * @brief A utility class for logging placement status and
 * updating the screen view when graphics are enabled.
 */
class PlacementLogPrinter {
  public:
    /**
     * @param placer The placer object from which the placement status is retrieved.
     * @param quiet When set true, the logger doesn't print any information.
     */
    PlacementLogPrinter(const Placer& placer,
                        bool quiet);

    /**
     * @brief Prints the placement status header that shows which metrics are reported
     * in each iteration of the annealer's outer loop.
     * @details This method should be called once before the first call to print_place_status().
     */
    void print_place_status_header() const;

    /**
     * @brief Print placement metrics and elapsed time after each outer loop iteration of the annealer.
     * If graphics are on, the function will the screen view.
     * @param elapsed_sec Time spent in the latest outer loop iteration.
     */
    void print_place_status(float elapsed_sec) const;

    /// Reports the resource utilization for each block type.
    void print_resources_utilization() const;
    /// Reports the number of tried temperatures, total swaps, and how many were accepted or rejected.
    void print_placement_swaps_stats() const;
    /// Reports placement metrics after the initial placement.
    void print_initial_placement_stats() const;
    /// Prints final placement metrics and generates timing reports.
    void print_post_placement_stats() const;
    /// Returns a bool to indicate whether the instance is in quiet mode.
    bool quiet() const { return quiet_; }

  private:
    /// Builds the list of columns to print in the placement status table (called once in ctor).
    std::vector<PlaceStatusColumn> get_place_status_columns() const;

    /**
     * @brief A constant reference to the Placer object to access the placement status.
     * @details PlacementLogPrinter is a friend class for the Placer class, so it can
     * access all its private data members. This reference is made constant to avoid
     * any accidental modification of the Placer object.
     */
    const Placer& placer_;
    /// Specifies whether this object prints logs and updates the graphics.
    const bool quiet_;
    /// A string buffer to carry the message to shown in the graphical interface.
    mutable std::vector<char> msg_;
    /// Cached column list for placement status; populated once in constructor.
    std::vector<PlaceStatusColumn> place_status_columns_;
};

void generate_post_place_timing_reports(const t_placer_opts& placer_opts,
                                        const t_analysis_opts& analysis_opts,
                                        const SetupTimingInfo& timing_info,
                                        const PlacementDelayCalculator& delay_calc,
                                        bool is_flat,
                                        const BlkLocRegistry& blk_loc_registry);
