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
 */

/**
 * @brief A function that returns the block ID in the clustered netlist
 *        from its ID in the atom netlist.
 */
ClusterBlockId atom_to_cluster(AtomBlockId atom);

/**
 * @brief A function that return a list of atoms in a cluster
 * @note This function can be called only after cluster/packing is done or
 * the clustered netlist is created.
 * @return Atoms in the given cluster. The returned set is immutable.
 */
const std::unordered_set<AtomBlockId>& cluster_to_atoms(ClusterBlockId cluster);

/**
 * @brief A function that return a list of atoms in a cluster
 * @note This function can be called only after cluster/packing is done or
 * the clustered netlist is created.
 * @return Atoms in the given cluster. The returned set is mutable.
 */
std::unordered_set<AtomBlockId>& cluster_to_mutable_atoms(ClusterBlockId cluster);

/**
 * @brief A function that loads the intra-cluster router data of one cluster
 */
t_lb_router_data* lb_load_router_data(std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
                                      ClusterBlockId clb_index,
                                      const std::unordered_set<AtomBlockId>& clb_atoms);

/**
 * @brief A function that removes a molecule from a cluster and checks legality of
 * the old cluster. 
 *
 * It returns true if the removal is done and the old cluster is legal.
 * It aborts the removal and returns false if the removal will make the old cluster 
 * illegal.
 *
 * This function updates the intra-logic block router data structure (router_data) and
 * remove all the atoms of the molecule from old_clb_atoms vector.
 *
 * @param old_clb: The original cluster of this molecule
 * @param old_clb_atoms: A vector containing the list of atoms in the old cluster of the molecule.
 * It will be updated in the function to remove the atoms of molecule from it.
 * @param router_data: returns the intra logic block router data.
 */
void remove_mol_from_cluster(const t_pack_molecule* molecule,
                             int molecule_size,
                             ClusterBlockId& old_clb,
                             std::unordered_set<AtomBlockId>& old_clb_atoms,
                             bool router_data_ready,
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
 * @param temp_cluster_noc_grp_id returns the NoC group ID of the new cluster
 * @param detailed_routing_stage: options are E_DETAILED_ROUTE_FOR_EACH_ATOM (default) and E_DETAILED_ROUTE_AT_END_ONLY.
 *                                This argument specifies whether or not to run an intra-cluster routing-based legality
 *                                check after adding the molecule to the cluster; default is the more conservative option.
 *                                This argument is passed down to try_pack_mol; if E_DETAILED_ROUTE_AT_END_ONLY is passed,
 *                                the function does not run a detailed intra-cluster routing-based legality check.
 *                                If many molecules will be added to a cluster, this option enables use of a single
 *                                routing check on the completed cluster (vs many incremental checks).
 * @param force_site: optional user-specified primitive site on which to place the molecule; this is passed to
 *                    try_pack_molecule and then to get_next_primitive_site. If a force_site argument is provided,
 *                    the molecule is either placed on the specified site or fails to add to the cluster.
 *                    If the force_site argument is set to its default value (-1), vpr selects an available site.
 */
bool start_new_cluster_for_mol(t_pack_molecule* molecule,
                               t_logical_block_type_ptr type,
                               int mode,
                               int feasible_block_array_size,
                               bool enable_pin_feasibility_filter,
                               ClusterBlockId clb_index,
                               bool during_packing,
                               int verbosity,
                               t_clustering_data& clustering_data,
                               t_lb_router_data** router_data,
                               PartitionRegion& temp_cluster_pr,
                               NocGroupId& temp_cluster_noc_grp_id,
                               enum e_detailed_routing_stages detailed_routing_stage = E_DETAILED_ROUTE_FOR_EACH_ATOM,
                               int force_site = -1);

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
 * @param temp_cluster_noc_grp_id returns the NoC group ID of the new cluster
 * @param detailed_routing_stage: options are E_DETAILED_ROUTE_FOR_EACH_ATOM (default) and E_DETAILED_ROUTE_AT_END_ONLY.
 *                                This argument specifies whether or not to run an intra-cluster routing-based legality
 *                                check after adding the molecule to the cluster; default is the more conservative option.
 *                                This argument is passed down to try_pack_mol; if E_DETAILED_ROUTE_AT_END_ONLY is passed,
 *                                the function does not run a detailed intra-cluster routing-based legality check.
 *                                If many molecules will be added to a cluster, this option enables use of a single
 *                                routing check on the completed cluster (vs many incremental checks).
 * @param enable_pin_feasibility_filter: do a pin couting based legality check (before or in place of intra-cluster routing check).
 * @param force_site: optional user-specified primitive site on which to place the molecule; this is passed to
 *                    try_pack_molecule and then to get_next_primitive_site. If a force_site argument is provided,
 *                    the molecule is either placed on the specified site or fails to add to the cluster.
 *                    If the force_site argument is set to its default value (-1), vpr selects an available site.
 */
bool pack_mol_in_existing_cluster(t_pack_molecule* molecule,
                                  int molecule_size,
                                  ClusterBlockId new_clb,
                                  std::unordered_set<AtomBlockId>& new_clb_atoms,
                                  bool during_packing,
                                  t_clustering_data& clustering_data,
                                  t_lb_router_data*& router_data,
                                  enum e_detailed_routing_stages detailed_routing_stage = E_DETAILED_ROUTE_FOR_EACH_ATOM,
                                  bool enable_pin_feasibility_filter = true,
                                  int force_site = -1);

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
                           ClusterBlockId old_clb,
                           ClusterBlockId new_clb);

/**
 * @brief A function that commits the molecule move if it is legal
 *
 * @params during_packing: true if this function is called during packing, false if it is called during placement
 * @params new_clb_created: true if the move is creating a new cluster (e.g. move_mol_to_new_cluster)
 */
void commit_mol_move(ClusterBlockId old_clb,
                     ClusterBlockId new_clb,
                     bool during_packing,
                     bool new_clb_created);

/**
 * @brief A function that reverts the molecule move if it is illegal
 *
 * @params during_packing: true if this function is called during packing, false if it is called during placement
 * @params new_clb_created: true if the move is creating a new cluster (e.g. move_mol_to_new_cluster)
 * @params
 */
void revert_mol_move(ClusterBlockId old_clb,
                     t_pack_molecule* molecule,
                     t_lb_router_data*& old_router_data,
                     bool during_packing,
                     t_clustering_data& clustering_data);

/**
 * @brief A function that checks the legality of a cluster by running the intra-cluster routing
 */
bool is_cluster_legal(t_lb_router_data*& router_data);

/**
 * @brief A function that commits the molecule removal if it is legal
 *
 * @params during_packing: true if this function is called during packing, false if it is called during placement
 * @params new_clb_created: true if the move is creating a new cluster (e.g. move_mol_to_new_cluster)
 */
void commit_mol_removal(const t_pack_molecule* molecule,
                        int molecule_size,
                        ClusterBlockId old_clb,
                        bool during_packing,
                        t_lb_router_data*& router_data,
                        t_clustering_data& clustering_data);

/**
 * @brief A function that check that two clusters are of the same type and in the same mode of operation
 */
bool check_type_and_mode_compatibility(ClusterBlockId old_clb,
                                       ClusterBlockId new_clb,
                                       int verbosity);

#endif
