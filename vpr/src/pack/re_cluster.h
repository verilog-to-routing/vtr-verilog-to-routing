#ifndef RE_CLUSTER_H
#define RE_CLUSTER_H
/**
 * @file This file includes an API function that updates clustering after its done
 * 
 * To optimize the clustering decisions, this file provides an API that can open up already
 * packed clusters and change them. The functions in this API can be used in 2 locations:
 *   - During packing after the clusterer is done
 *   - During placement after the initial placement is done
 *
 */

#include "pack_types.h"
#include "clustered_netlist_utils.h"
#include "cluster_util.h"

/**
 * @brief This function moves a molecule out of its cluster and creates a new cluster for it
 * 
 * This function can be called from 2 spots in the vpr flow. 
 *   - First, during packing to optimize the initial clustered netlist 
 *             (during_packing variable should be true.)
 *   - Second, during placement (during_packing variable should be false). In this case, the clustered
 *              netlist is updated.
 */
bool move_mol_to_new_cluster(t_pack_molecule* molecule,
                             bool during_packing,
                             int verbosity,
                             t_clustering_data& clustering_data);

/**
 * @brief This function moves a molecule out of its cluster to another cluster that already exists.
 * 
 * This function can be called from 2 spots in the vpr flow. 
 *   - First, during packing to optimize the initial clustered netlist 
 *             (during_packing variable should be true.)
 *   - Second, during placement (during_packing variable should be false). In this case, the clustered
 *              netlist is updated.
 */
bool move_mol_to_existing_cluster(t_pack_molecule* molecule,
                                  const ClusterBlockId& new_clb,
                                  bool during_packing,
                                  int verbosity,
                                  t_clustering_data& clustering_data);

/**
 * @brief This function swap two molecules between two different clusters.
 * 
 * This function can be called from 2 spots in the vpr flow. 
 *   - First, during packing to optimize the initial clustered netlist 
 *             (during_packing variable should be true.)
 *   - Second, during placement (during_packing variable should be false). In this case, the clustered
 *              netlist is updated.
 */
bool swap_two_molecules(t_pack_molecule* molecule_1,
                        t_pack_molecule* molecule_2,
                        bool during_packing,
                        int verbosity,
                        t_clustering_data& clustering_data);
#endif
