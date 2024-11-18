
#ifndef VTR_PLACE_LOG_UTIL_H
#define VTR_PLACE_LOG_UTIL_H

#include <cstddef>

class t_annealing_state;
class t_placer_statistics;
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

#endif //VTR_PLACE_LOG_UTIL_H
