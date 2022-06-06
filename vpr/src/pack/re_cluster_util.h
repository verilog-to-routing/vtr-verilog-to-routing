#ifndef RE_CLUSTER_UTIL_H
#define RE_CLUSTER_UTIL_H

#include "clustered_netlist_fwd.h"
#include "clustered_netlist_utils.h"
#include "atom_netlist_fwd.h"
#include "globals.h"
#include "pack_types.h"
#include "cluster_util.h"
/**
 * @file
 * @brief This files defines some helper functions for the re-clustering
 *
 * API that uses to move atoms between clusters after the cluster is done.
 * Note: Some of the helper functions defined here might be useful in different places in VPR.
 * 
 */

/**
 * @brief A function that returns the block ID in the clustered netlist
 *        from its ID in the atom netlist.
 */
ClusterBlockId atom_to_cluster(const AtomBlockId& atom);

/**
 * @brief A function that return a list of atoms in a cluster
 * @note This finction can be called only after cluster/packing is done or
 * the clustered netlist is created
 */
std::vector<AtomBlockId> cluster_to_atoms(const ClusterBlockId& cluster);

/**
 * @brief A function that loads the intra-cluster router data of one cluster
 */
t_lb_router_data* lb_load_router_data(std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
                                      const ClusterBlockId& clb_index);

/**
 * @brief A function that removes a molecule from a cluster and check legality of
 * the old cluster. 
 *
 * It returns true if the removal is done and the old cluster is legal.
 * It aborts the removal and returns false if the removal will make the old cluster 
 * illegal
 */
bool remove_mol_from_cluster(const t_pack_molecule* molecule,
                             int molecule_size,
                             ClusterBlockId& old_clb,
                             t_lb_router_data*& router_data);

/**
 * @brief A function that starts a new cluster for one specific molecule
 * 
 * It places the molecule in a specific type and mode that should be passed by
 * the higher level routine.
 */
bool start_new_cluster_for_mol(t_pack_molecule* molecule,
                               const t_logical_block_type_ptr& type,
                               const int mode,
                               const int feasible_block_array_size,
                               bool enable_pin_feasibility_filter,
                               ClusterBlockId clb_index,
                               bool during_packing,
                               t_clustering_data& clustering_data,
                               t_lb_router_data** router_data,
                               PartitionRegion& temp_cluster_pr);

/**
 * @brief A function that fix the clustered netlist if the move is performed
 * after the packing is done and clustered netlist is built
 *
 * If you are changing clustering after packing is done, you need to update the clustered netlist by
 * deleting the newly absorbed nets and creating nets for the atom nets that become unabsorbed. It also
 * fixes the cluster ports for both the old and new clusters.
 */
void fix_clustered_netlist(t_pack_molecule* molecule,
                           int molecule_size,
                           const ClusterBlockId& old_clb,
                           const ClusterBlockId& new_clb);

/**
 * @brief A function that commits the molecule move if it is legal
 */
void commit_mol_move(const ClusterBlockId& old_clb,
                     const ClusterBlockId& new_clb,
                     std::vector<t_pb*>& old_mol_pbs,
                     t_lb_router_data*& old_router_data,
                     t_clustering_data& clustering_data,
                     bool during_packing);

/**
 * @brief A function that packs a molecule into an existing cluster
 */
bool pack_mol_in_existing_cluster(t_pack_molecule* molecule,
                                  const ClusterBlockId clb_index,
                                  bool during_packing,
                                  t_clustering_data& clustering_data);

#endif
