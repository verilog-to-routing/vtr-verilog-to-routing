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
 * @brief This files defines some helper functions for the re-clustering API
 *
 * Re-clustering API is used to move atoms between clusters after the cluster is done.
 * This can be very used in iteratively improve the packed solution after the initial clustering is done.
 * It can also be used during placement to allow fine-grained moves that can move a BLE or a single FF/LUT.
 * 
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
                                      const ClusterBlockId& clb_index,
                                      const std::vector<AtomBlockId>& clb_atoms);

/**
 * @brief A function that removes a molecule from a cluster and checks legality of
 * the old cluster. 
 *
 * It returns true if the removal is done and the old cluster is legal.
 * It aborts the removal and returns false if the removal will make the old cluster 
 * illegal.
 *
 * This function updates the intra-logic block router data structure (router_data) and
 * remove all the atoms of the moecule from old_clb_atoms vector.
 *
 * @param old_clb: The original cluster of this molecule
 * @param old_clb_atoms: A vector containing the list of atoms in the old cluster of the molecule.
 * It will be updated in the function to remove the atoms of molecule from it.
 * @param router_data: returns the intra logic block router data.
 */
void remove_mol_from_cluster(const t_pack_molecule* molecule,
                             int molecule_size,
                             ClusterBlockId& old_clb,
                             std::vector<AtomBlockId>& old_clb_atoms,
                             t_lb_router_data*& router_data);

/**
 * @brief A function that starts a new cluster for one specific molecule
 * 
 * It places the molecule in a specific type and mode that should be passed by
 * the higher level routine.
 *
 * @param type: the cluster block type needed
 * @param mode: the mode of the new cluster
 * @param clb_index: the cluster block Id of the newly created cluster block
 * @param during_packing: true if this function is called during packing, false if it is called during placement
 * @param clustering_data: A data structure containing helper data for the clustering process 
 *                          (is updated if this function is called during packing, especially intra_lb_routing data member).
 * @param router_data: returns the intra logic block router data.
 * @param temp_cluster_pr: returns the partition region of the new cluster.
 */
bool start_new_cluster_for_mol(t_pack_molecule* molecule,
                               const t_logical_block_type_ptr& type,
                               const int mode,
                               const int feasible_block_array_size,
                               bool enable_pin_feasibility_filter,
                               ClusterBlockId clb_index,
                               bool during_packing,
                               int verbosity,
                               t_clustering_data& clustering_data,
                               t_lb_router_data** router_data,
                               PartitionRegion& temp_cluster_pr);

/**
 * @brief A function that packs a molecule into an existing cluster
 *
 * @param clb_index: the cluster block Id of the new cluster that we need to pack the molecule in.
 * @param: clb_atoms: A vector containing the list of atoms in the new cluster block before adding the molecule.
 * @param during_packing: true if this function is called during packing, false if it is called during placement
 * @param is_swap: true if this function is called during swapping two molecules. False if the called during a single molecule move
 * @param clustering_data: A data structure containing helper data for the clustering process
 *                          (is updated if this function is called during packing, especially intra_lb_routing data member).
 * @param router_data: returns the intra logic block router data.
 */
bool pack_mol_in_existing_cluster(t_pack_molecule* molecule,
                                  const ClusterBlockId clb_index,
                                  const std::vector<AtomBlockId>& clb_atoms,
                                  bool during_packing,
                                  bool is_swap,
                                  t_clustering_data& clustering_data,
                                  t_lb_router_data*& router_data);

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
 *
 * @during_packing: true if this function is called during packing, false if it is called during placement
 * @new_clb_created: true if the move is creating a new cluster (e.g. move_mol_to_new_cluster)
 */
void commit_mol_move(const ClusterBlockId& old_clb,
                     const ClusterBlockId& new_clb,
                     bool during_packing,
                     bool new_clb_created);

void revert_mol_move(const ClusterBlockId& old_clb,
                     t_pack_molecule* molecule,
                     t_lb_router_data*& old_router_data,
                     bool during_packing,
                     t_clustering_data& clustering_data);

bool is_cluster_legal(t_lb_router_data*& router_data);

void commit_mol_removal(const t_pack_molecule* molecule,
                        const int& molecule_size,
                        const ClusterBlockId& old_clb,
                        bool during_packing,
                        t_lb_router_data*& router_data,
                        t_clustering_data& clustering_data);

bool check_type_and_mode_compitability(const ClusterBlockId& old_clb,
                                       const ClusterBlockId& new_clb,
                                       int verbosity);

#endif
