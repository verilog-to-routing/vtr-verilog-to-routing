#pragma once

struct t_noc_opts;
struct t_placer_opts;
class BlkLocRegistry;
class PlaceMacros;
class NocCostHandler;

namespace vtr {
class RngContainer;
}

/**
 * @brief Randomly places NoC routers, then runs a quick simulated annealing
 * to minimize NoC costs.
 *
 *   @param noc_opts NoC-related options. Used to calculate NoC-related costs.
 *   @param blk_loc_registry Placement block location information. To be filled
 *   with the location where pl_macro is placed.
 *   @param rng A random number generator used during simulated annealing.
 */
void initial_noc_placement(const t_noc_opts& noc_opts,
                           BlkLocRegistry& blk_loc_registry,
                           const PlaceMacros& place_macros,
                           NocCostHandler& noc_cost_handler,
                           vtr::RngContainer& rng);
