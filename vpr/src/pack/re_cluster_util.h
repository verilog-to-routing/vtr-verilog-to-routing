#ifndef RE_CLUSTER_UTIL_H
#define RE_CLUSTER_UTIL_H

#include "clustered_netlist_fwd.h"
#include "atom_netlist_fwd.h"
#include "globals.h"
/**
 * @file
 * @brief This files defines some helper functions for the re-clustering
 * API that uses to move atoms between clusters after the cluster is done.
 * Note: Some of the helper functions defined here might be useful in different places in VPR
 * 
 */

/**
 * @brief A function that returns the cluster ID of an atom ID
 */
ClusterBlockId atom_to_cluster(const AtomBlockId& atom);

/**
 * @brief A function that return a list of atoms in a cluster
 * @note This finction can be called only after cluster/packing is done or
 * the clustered netlist is created
 */
std::vector<AtomBlockId> cluster_to_atoms(const ClusterBlockId& cluster);

/**
 * @brief A function that removes an atom from a cluster and check legality of
 * the old cluster. It returns true if the removal is done and the old cluster is legal.
 * It aborts the removal and returns false if the removal will make the old cluster 
 * illegal
 */
bool remove_mol_from_cluster(const t_pack_molecule* molecule);

void move_atom_to_new_cluster(const AtomBlockId& moving_atom);

#endif