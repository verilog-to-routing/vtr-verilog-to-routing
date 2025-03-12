#ifndef CLUSTER_UTIL_H
#define CLUSTER_UTIL_H

#include <unordered_set>
#include <vector>
#include "cluster_legalizer.h"
#include "vtr_vector.h"

class AtomNetId;
class AttractionInfo;
class ClusterBlockId;
class ClusterLegalizer;
class ClusteredNetlist;
class PreClusterDelayCalculator;
class Prepacker;
class SetupTimingInfo;
class t_pack_molecule;
struct AtomContext;

/**
 * @file
 * @brief This file includes useful structs and functions for building and modifying clustering
 */

/***********************************/
/*   Clustering helper functions   */
/***********************************/

/*
 * @brief Calculate the initial timing at the start of packing stage.
 */
void calc_init_packing_timing(const t_packer_opts& packer_opts,
                              const t_analysis_opts& analysis_opts,
                              const Prepacker& prepacker,
                              std::shared_ptr<PreClusterDelayCalculator>& clustering_delay_calc,
                              std::shared_ptr<SetupTimingInfo>& timing_info,
                              vtr::vector<AtomBlockId, float>& atom_criticality);

/*
 * @brief Check clustering legality and output it.
 */
void check_and_output_clustering(ClusterLegalizer& cluster_legalizer,
                                 const t_packer_opts& packer_opts,
                                 const std::unordered_set<AtomNetId>& is_clock,
                                 const t_arch* arch);

/*
 * @brief Print the header for the clustering progress table.
 */
void print_pack_status_header();

/*
 * @brief Incrementally print progress updates during clustering.
 */
void print_pack_status(int tot_num_molecules,
                       int num_molecules_processed,
                       int& mols_since_last_print,
                       int device_width,
                       int device_height,
                       AttractionInfo& attraction_groups,
                       const ClusterLegalizer& cluster_legalizer);

/*
 * @brief Periodically rebuild the attraction groups to reflect which atoms in
 *        them are still available for new clusters (i.e. remove the atoms that
 *        have already been packed from the attraction group).
 */
void rebuild_attraction_groups(AttractionInfo& attraction_groups,
                               const ClusterLegalizer& cluster_legalizer);

std::map<const t_model*, std::vector<t_logical_block_type_ptr>> identify_primitive_candidate_block_types();

/**
 * @brief Identify which nets in the atom netlist are driven by the same atom
 *        block that they appear as a receiver (input) pin of.
 */
std::unordered_set<AtomNetId> identify_net_output_feeds_driving_block_input(const AtomNetlist& atom_netlist);

/**
 * @brief This function update the pb_type_count data structure by incrementing
 *        the number of used pb_types in the given packed cluster t_pb
 */
size_t update_pb_type_count(const t_pb* pb, std::map<t_pb_type*, int>& pb_type_count, size_t depth);

/*
 * @brief This function updates the le_count data structure from the given
 *        packed cluster.
 */
void update_le_count(const t_pb* pb,
                     const t_logical_block_type_ptr logic_block_type,
                     const t_pb_type* le_pb_type,
                     int& num_logic_le,
                     int& num_reg_le,
                     int& num_logic_and_reg_le);

void print_pb_type_count_recurr(t_pb_type* type, size_t max_name_chars, size_t curr_depth, std::map<t_pb_type*, int>& pb_type_count);

/**
 * Print the total number of used physical blocks for each pb type in the architecture
 */
void print_pb_type_count(const ClusteredNetlist& clb_nlist);

/*
 * @brief This function identifies the logic block type which is defined by the
 *        block type which has a lut primitive.
 */
t_logical_block_type_ptr identify_logic_block_type(const std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types);

/*
 * @brief This function returns the pb_type that is similar to Logic Element (LE)
 *        in an FPGA.
 *
 * The LE is defined as a physical block that contains a LUT primitive and
 * is found by searching a cluster type to find the first pb_type (from the top
 * of the hierarchy clb->LE) that has more than one instance within the cluster.
 */
t_pb_type* identify_le_block_type(t_logical_block_type_ptr logic_block_type);

/*
 * @brief  This function returns true if the given physical block has a primitive
 *         matching the given blif model and is used.
 */
bool pb_used_for_blif_model(const t_pb* pb, const std::string& blif_model_name);

/*
 * @brief Print the LE count data strurture.
 */
void print_le_count(int num_logic_le,
                    int num_reg_le,
                    int num_logic_and_reg_le,
                    const t_pb_type* le_pb_type);

/*
 * @brief Load the mapping between clusters and their atoms.
 */
void init_clb_atoms_lookup(vtr::vector<ClusterBlockId, std::unordered_set<AtomBlockId>>& atoms_lookup,
                           const AtomContext& atom_ctx,
                           const ClusteredNetlist& clb_nlist);
#endif
