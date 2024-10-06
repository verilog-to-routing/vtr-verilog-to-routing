
#ifndef VTR_INITIAL_NOC_PLACEMENT_H
#define VTR_INITIAL_NOC_PLACEMENT_H

struct t_noc_opts;
struct t_placer_opts;
class BlkLocRegistry;
class NocCostHandler;

/**
 * @brief Randomly places NoC routers, then runs a quick simulated annealing
 * to minimize NoC costs.
 *
 *   @param noc_opts NoC-related options. Used to calculate NoC-related costs.
 *   @param placer_opts Contain the placement algorithm options including the seed.
 *   @param blk_loc_registry Placement block location information. To be filled
 *   with the location where pl_macro is placed.
 */
void initial_noc_placement(const t_noc_opts& noc_opts,
                           const t_placer_opts& placer_opts,
                           BlkLocRegistry& blk_loc_registry,
                           NocCostHandler& noc_cost_handler);

#endif //VTR_INITIAL_NOC_PLACEMENT_H
