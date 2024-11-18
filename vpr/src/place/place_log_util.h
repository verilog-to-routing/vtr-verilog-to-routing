
#ifndef VTR_PLACE_LOG_UTIL_H
#define VTR_PLACE_LOG_UTIL_H

#include <cstddef>

#include "timing_info_fwd.h"
#include "PlacementDelayCalculator.h"

class t_annealing_state;
class t_placer_statistics;
struct t_placer_opts;
struct t_analysis_opts;
struct NocCostTerms;
struct t_swap_stats;
class BlkLocRegistry;

void print_place_status_header(bool noc_enabled);

void print_place_status(const t_annealing_state& state,
                        const t_placer_statistics& stats,
                        float elapsed_sec,
                        float cpd,
                        float sTNS,
                        float sWNS,
                        size_t tot_moves,
                        bool noc_enabled,
                        const NocCostTerms& noc_cost_terms);

void print_resources_utilization(const BlkLocRegistry& blk_loc_registry);

void print_placement_swaps_stats(const t_annealing_state& state, const t_swap_stats& swap_stats);

void generate_post_place_timing_reports(const t_placer_opts& placer_opts,
                                        const t_analysis_opts& analysis_opts,
                                        const SetupTimingInfo& timing_info,
                                        const PlacementDelayCalculator& delay_calc,
                                        bool is_flat,
                                        const BlkLocRegistry& blk_loc_registry);

#endif //VTR_PLACE_LOG_UTIL_H
