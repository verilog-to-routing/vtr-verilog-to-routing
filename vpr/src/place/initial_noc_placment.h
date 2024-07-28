
#ifndef VTR_INITIAL_NOC_PLACMENT_H
#define VTR_INITIAL_NOC_PLACMENT_H

#include "vpr_types.h"
#include "vtr_vector_map.h"

/**
 * @brief Randomly places NoC routers, then runs a quick simulated annealing
 * to minimize NoC costs.
 *
 *   @param noc_opts NoC-related options. Used to calculate NoC-related costs.
 */
void initial_noc_placement(const t_noc_opts& noc_opts,
                           const t_placer_opts& placer_opts,
                           PlaceLocVars& place_loc_vars);

#endif //VTR_INITIAL_NOC_PLACMENT_H
