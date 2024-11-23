
#ifndef VTR_PLACE_LOG_UTIL_H
#define VTR_PLACE_LOG_UTIL_H

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

#endif //VTR_PLACE_LOG_UTIL_H
