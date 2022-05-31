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
 * @brief A function that loads the router data for a cluster
 */
t_lb_router_data* lb_load_router_data(std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
                                      const ClusterBlockId& clb_index);

/**
 * @brief A function that removes an atom from a cluster and check legality of
 * the old cluster. 
 *
 * It returns true if the removal is done and the old cluster is legal.
 * It aborts the removal and returns false if the removal will make the old cluster 
 * illegal
 */
bool remove_atom_from_cluster(const AtomBlockId& atom_id,
                              std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
                              ClusterBlockId& old_clb,
                              t_lb_router_data*& router_data);

/**
 * @brief A function that starts a new cluster for one specific molecule
 * 
 * It places the molecule in a specific type and mode that should be passed by
 * the higher level routine.
 */
bool start_new_cluster_for_atom(const AtomBlockId atom_id,
                                const t_logical_block_type_ptr& type,
                                const int mode,
                                const int feasible_block_array_size,
                                bool enable_pin_feasibility_filter,
                                ClusterBlockId clb_index,
                                t_lb_router_data** router_data,
                                std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
                                PartitionRegion& temp_cluster_pr,
                                t_clustering_data& clustering_data,
                                bool during_packing);

/**
 * @brief A function that fix the clustered netlist if the move is performed
 * after the packing is done and clustered netlist is built
 */
void fix_clustered_netlist(const AtomBlockId& atom_id,
                           const ClusterBlockId& old_clb,
                           const ClusterBlockId& new_clb);

/**
 * @brief A function that commits the molecule move if it is legal
 */
void commit_atom_move(const ClusterBlockId& old_clb,
                      const ClusterBlockId& new_clb,
                      t_pb* old_pb,
                      t_lb_router_data*& old_router_data,
                      t_clustering_data& clustering_data,
                      bool during_packing);

#endif
