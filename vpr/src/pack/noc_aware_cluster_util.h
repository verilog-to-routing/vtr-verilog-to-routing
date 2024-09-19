#ifndef VTR_NOC_AWARE_CLUSTER_UTIL_H
#define VTR_NOC_AWARE_CLUSTER_UTIL_H

/**
 * @file This file includes helper functions used to find NoC groups
 * in the atom netlist and assign NoC group IDs to atom blocks.
 *
 * A NoC group is a set of atom blocks that are reachable from a NoC router
 * through low fanout nets. During packing, atom blocks that belong to two different
 * NoC group IDs cannot be packed with each other into the same clustered block.
 * This prevents atom blocks that belong to two separate NoC-attached modules from
 * being packed with each other, and helps with more localized placement of NoC-attached
 * modules around their corresponding NoC routers.
 *
 * For more details refer to the following paper:
 * The Road Less Traveled: Congestion-Aware NoC Placement and Packet Routing for FPGAs
 */

#include <vector>
#include "noc_data_types.h"
#include "vtr_vector.h"

class AtomNetlist;
class AtomBlockId;
class t_pack_high_fanout_thresholds;

/**
 * @brief Iterates over all atom blocks and check whether
 * their blif model is the same as a NoC routers.
 *
 * @return The atom block IDs of the NoC router blocks in the netlist.
 */
std::vector<AtomBlockId> find_noc_router_atoms(const AtomNetlist& atom_netlist);


/**
 * @brief Runs BFS starting from NoC routers to find all connected
 * components that include a NoC router. Each connected component
 * containing a NoC router is marked as a NoC group. The NoC group ID
 * for each atom block is updated in the global state.
 *
 * @param noc_atoms The atom block IDs of the NoC router blocks in the netlist.
 */
void update_noc_reachability_partitions(const std::vector<AtomBlockId>& noc_atoms,
                                        const AtomNetlist& atom_netlist,
                                        const t_pack_high_fanout_thresholds& high_fanout_threshold,
                                        vtr::vector<AtomBlockId, NocGroupId>& atom_noc_grp_id);

#endif
