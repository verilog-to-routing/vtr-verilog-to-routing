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
 * @brief This function moves an atom out of its cluster and create a new cluster for it
 * 
 * This function can be called from 2 spots in the vpr flow. 
 *   - First, during packing to optimize the initial clustered netlist 
 *             (during_packing variable should be true.)
 *   - Second, during placement (during_packing variable should be false)
 */
bool move_atom_to_new_cluster(const AtomBlockId& atom_id,
                              const enum e_pad_loc_type& pad_loc_type,
                              std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
                              t_clustering_data& clustering_data,
                              bool during_packing);
#endif