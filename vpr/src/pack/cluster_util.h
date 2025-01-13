#ifndef CLUSTER_UTIL_H
#define CLUSTER_UTIL_H

#include <unordered_set>
#include <vector>
#include "cluster_legalizer.h"
#include "pack_types.h"
#include "vtr_vector.h"

class AtomNetId;
class ClusterBlockId;
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

constexpr int AAPACK_MAX_HIGH_FANOUT_EXPLORE = 10; /* For high-fanout nets that are ignored, consider a maximum of this many sinks, must be less than packer_opts.feasible_block_array_size */
constexpr int AAPACK_MAX_TRANSITIVE_EXPLORE = 40;  /* When investigating transitive fanout connections in packing, consider a maximum of this many molecules, must be less than packer_opts.feasible_block_array_size */

enum e_gain_update {
    GAIN,
    NO_GAIN
};
enum e_feasibility {
    FEASIBLE,
    INFEASIBLE
};
enum e_gain_type {
    HILL_CLIMBING,
    NOT_HILL_CLIMBING
};
enum e_removal_policy {
    REMOVE_CLUSTERED,
    LEAVE_CLUSTERED
};
/* TODO: REMOVE_CLUSTERED no longer used, remove */
enum e_net_relation_to_clustered_block {
    INPUT,
    OUTPUT
};

/* Linked list structure.  Stores one integer (iblk). */
struct t_molecule_link {
    t_pack_molecule* moleculeptr;
    t_molecule_link* next;
};

struct t_molecule_stats {
    int num_blocks = 0; //Number of blocks across all primitives in molecule

    int num_pins = 0;        //Number of pins across all primitives in molecule
    int num_input_pins = 0;  //Number of input pins across all primitives in molecule
    int num_output_pins = 0; //Number of output pins across all primitives in molecule

    int num_used_ext_pins = 0;    //Number of *used external* pins across all primitives in molecule
    int num_used_ext_inputs = 0;  //Number of *used external* input pins across all primitives in molecule
    int num_used_ext_outputs = 0; //Number of *used external* output pins across all primitives in molecule
};

/* Useful data structures for creating or modifying clusters */
struct t_clustering_data {
    int unclustered_list_head_size = 0;
    /* Keeps a linked list of the unclustered blocks to speed up looking for *                                                                  
     * unclustered blocks with a certain number of *external* inputs.        *
     * [0..lut_size].  Unclustered_list_head[i] points to the head of the    *
     * list of blocks with i inputs to be hooked up via external interconnect. */
    t_molecule_link* unclustered_list_head = nullptr;

    //Maintaining a linked list of free molecule data for speed
    t_molecule_link* memory_pool = nullptr;
};

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
 * @brief Free the clustering data structures.
 */
void free_clustering_data(t_clustering_data& clustering_data);

/*
 * @brief Check clustering legality and output it.
 */
void check_and_output_clustering(ClusterLegalizer& cluster_legalizer,
                                 const t_packer_opts& packer_opts,
                                 const std::unordered_set<AtomNetId>& is_clock,
                                 const t_arch* arch);

/*
 * @brief Determine if atom block is in pb.
 */
bool is_atom_blk_in_pb(const AtomBlockId blk_id, const t_pb* pb);

/*
 * @brief Add blk to list of feasible blocks sorted according to gain.
 */
void add_molecule_to_pb_stats_candidates(t_pack_molecule* molecule,
                                         std::map<AtomBlockId, float>& gain,
                                         t_pb* pb,
                                         int max_queue_size,
                                         AttractionInfo& attraction_groups);

/*
 * @brief Remove blk from list of feasible blocks sorted according to gain.
 *
 * Useful for removing blocks that are repeatedly failing. If a block
 * has been found to be illegal, we don't repeatedly consider it.
 */
void remove_molecule_from_pb_stats_candidates(t_pack_molecule* molecule,
                                              t_pb* pb);
/*
 * @brief Allocates and inits the data structures used for clustering.
 *
 * This method initializes the list of molecules to pack, the clustering data,
 * and the net info.
 */
void alloc_and_init_clustering(const t_molecule_stats& max_molecule_stats,
                               const Prepacker& prepacker,
                               t_clustering_data& clustering_data,
                               int num_molecules);

/*
 * @brief This routine returns an atom block which has not been clustered, has
 *        no connection to the current cluster, satisfies the cluster clock
 *        constraints, is a valid subblock inside the cluster, does not exceed
 *        the cluster subblock units available, and has ext_inps external inputs.
 *        Remove_flag controls whether or not blocks that have already been
 *        clustered are removed from the unclustered_list data structures.
 *        NB: to get a atom block regardless of clock constraints just set
 *            clocks_avail > 0.
 */
t_pack_molecule* get_molecule_by_num_ext_inputs(const int ext_inps,
                                                const enum e_removal_policy remove_flag,
                                                t_molecule_link* unclustered_list_head,
                                                LegalizationClusterId legalization_cluster_id,
                                                const ClusterLegalizer& cluster_legalizer);

/* @brief This routine is used to find new blocks for clustering when there are
 *        no feasible blocks with any attraction to the current cluster (i.e.
 *        it finds blocks which are unconnected from the current cluster).  It
 *        returns the atom block with the largest number of used inputs that
 *        satisfies the clocking and number of inputs constraints.  If no
 *        suitable atom block is found, the routine returns nullptr.
 */
t_pack_molecule* get_free_molecule_with_most_ext_inputs_for_cluster(t_pb* cur_pb,
                                                                    t_molecule_link* unclustered_list_head,
                                                                    const int& unclustered_list_head_size,
                                                                    LegalizationClusterId legalization_cluster_id,
                                                                    const ClusterLegalizer& cluster_legalizer);

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

void update_connection_gain_values(const AtomNetId net_id,
                                   const AtomBlockId clustered_blk_id,
                                   t_pb* cur_pb,
                                   const ClusterLegalizer& cluster_legalizer,
                                   enum e_net_relation_to_clustered_block net_relation_to_clustered_block);

void update_timing_gain_values(const AtomNetId net_id,
                               t_pb* cur_pb,
                               const ClusterLegalizer& cluster_legalizer,
                               enum e_net_relation_to_clustered_block net_relation_to_clustered_block,
                               const SetupTimingInfo& timing_info,
                               const std::unordered_set<AtomNetId>& is_global,
                               const std::unordered_set<AtomNetId>& net_output_feeds_driving_block_input);

/*
 * @brief Updates the marked data structures, and if gain_flag is GAIN, the gain
 *        when an atom block is added to a cluster.  The sharinggain is the
 *        number of inputs that a atom block shares with blocks that are already
 *        in the cluster. Hillgain is the reduction in number of pins-required
 *        by adding a atom block to the cluster. The timinggain is the
 *        criticality of the most critical atom net between this atom block and
 *        an atom block in the cluster.
 */
void mark_and_update_partial_gain(const AtomNetId net_id,
                                  enum e_gain_update gain_flag,
                                  const AtomBlockId clustered_blk_id,
                                  const ClusterLegalizer& cluster_legalizer,
                                  bool timing_driven,
                                  bool connection_driven,
                                  enum e_net_relation_to_clustered_block net_relation_to_clustered_block,
                                  const SetupTimingInfo& timing_info,
                                  const std::unordered_set<AtomNetId>& is_global,
                                  const int high_fanout_net_threshold,
                                  const std::unordered_set<AtomNetId>& net_output_feeds_driving_block_input);

/*
 * @brief Updates the total  gain array to reflect the desired tradeoff between
 *        input sharing (sharinggain) and path_length minimization (timinggain)
 *        input each time a new molecule is added to the cluster.
 */
void update_total_gain(float alpha, float beta, bool timing_driven, bool connection_driven, t_pb* pb, AttractionInfo& attraction_groups);

/*
 * @brief Routine that is called each time a new molecule is added to the cluster.
 *
 * Makes calls to update cluster stats such as the gain map for atoms, used pins,
 * and clock structures, in order to reflect the new content of the cluster.
 * Also keeps track of which attraction group the cluster belongs to.
 */
void update_cluster_stats(const t_pack_molecule* molecule,
                          const ClusterLegalizer& cluster_legalizer,
                          const std::unordered_set<AtomNetId>& is_clock,
                          const std::unordered_set<AtomNetId>& is_global,
                          const bool global_clocks,
                          const float alpha,
                          const float beta,
                          const bool timing_driven,
                          const bool connection_driven,
                          const int high_fanout_net_threshold,
                          const SetupTimingInfo& timing_info,
                          AttractionInfo& attraction_groups,
                          const std::unordered_set<AtomNetId>& net_output_feeds_driving_block_input);

/*
 * @brief Get candidate molecule to pack into currently open cluster
 *
 * Molecule selection priority:
 * 1. Find unpacked molecules based on criticality and strong connectedness
 *    (connected by low fanout nets) with current cluster.
 * 2. Find unpacked molecules based on transitive connections (eg. 2 hops away)
 *    with current cluster.
 * 3. Find unpacked molecules based on weak connectedness (connected by high
 *    fanout nets) with current cluster.
 * 4. Find unpacked molecules based on attraction group of the current cluster
 *    (if the cluster has an attraction group).
 */
t_pack_molecule* get_highest_gain_molecule(t_pb* cur_pb,
                                           AttractionInfo& attraction_groups,
                                           const enum e_gain_type gain_mode,
                                           const Prepacker& prepacker,
                                           const ClusterLegalizer& cluster_legalizer,
                                           vtr::vector<LegalizationClusterId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                                           const LegalizationClusterId cluster_index,
                                           bool prioritize_transitive_connectivity,
                                           int transitive_fanout_threshold,
                                           const int feasible_block_array_size,
                                           const std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types);

/*
 * @brief Add molecules with strong connectedness to the current cluster to the
 *        list of feasible blocks.
 */
void add_cluster_molecule_candidates_by_connectivity_and_timing(t_pb* cur_pb,
                                                                LegalizationClusterId legalization_cluster_id,
                                                                const Prepacker& prepacker,
                                                                const ClusterLegalizer& cluster_legalizer,
                                                                const int feasible_block_array_size,
                                                                AttractionInfo& attraction_groups);

/*
 * @brief Add molecules based on weak connectedness (connected by high fanout
 *        nets) with current cluster.
 */
void add_cluster_molecule_candidates_by_highfanout_connectivity(t_pb* cur_pb,
                                                                LegalizationClusterId legalization_cluster_id,
                                                                const Prepacker& prepacker,
                                                                const ClusterLegalizer& cluster_legalizer,
                                                                const int feasible_block_array_size,
                                                                AttractionInfo& attraction_groups);

/*
 * @brief If the current cluster being packed has an attraction group associated
 *        with it (i.e. there are atoms in it that belong to an attraction group),
 *        this routine adds molecules from the associated attraction group to
 *        the list of feasible blocks for the cluster.
 *
 * Attraction groups can be very large, so we only add some randomly selected
 * molecules for efficiency if the number of atoms in the group is greater than
 * 500. Therefore, the molecules added to the candidates will vary each time you
 * call this function.
 */
void add_cluster_molecule_candidates_by_attraction_group(t_pb* cur_pb,
                                                         const Prepacker& prepacker,
                                                         const ClusterLegalizer& cluster_legalizer,
                                                         AttractionInfo& attraction_groups,
                                                         const int feasible_block_array_size,
                                                         LegalizationClusterId clb_index,
                                                         const std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types);

/*
 * @brief Add molecules based on transitive connections (eg. 2 hops away) with
 *        current cluster.
 */
void add_cluster_molecule_candidates_by_transitive_connectivity(t_pb* cur_pb,
                                                                const Prepacker& prepacker,
                                                                const ClusterLegalizer& cluster_legalizer,
                                                                vtr::vector<LegalizationClusterId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                                                                const LegalizationClusterId cluster_index,
                                                                int transitive_fanout_threshold,
                                                                const int feasible_block_array_size,
                                                                AttractionInfo& attraction_groups);

t_pack_molecule* get_molecule_for_cluster(t_pb* cur_pb,
                                          AttractionInfo& attraction_groups,
                                          const bool allow_unrelated_clustering,
                                          const bool prioritize_transitive_connectivity,
                                          const int transitive_fanout_threshold,
                                          const int feasible_block_array_size,
                                          int* num_unrelated_clustering_attempts,
                                          const Prepacker& prepacker,
                                          const ClusterLegalizer& cluster_legalizer,
                                          vtr::vector<LegalizationClusterId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                                          LegalizationClusterId cluster_index,
                                          int verbosity,
                                          t_molecule_link* unclustered_list_head,
                                          const int& unclustered_list_head_size,
                                          const std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types);

/*
 * @brief Calculates molecule statistics for a single molecule.
 */
t_molecule_stats calc_molecule_stats(const t_pack_molecule* molecule, const AtomNetlist& atom_nlist);

/*
 * @brief Get gain of packing molecule into current cluster.
 *
 * gain is equal to:
 * total_block_gain
 * + molecule_base_gain*some_factor
 * - introduced_input_nets_of_unrelated_blocks_pulled_in_by_molecule*some_other_factor
 */
float get_molecule_gain(t_pack_molecule* molecule, std::map<AtomBlockId, float>& blk_gain, AttractGroupId cluster_attraction_group_id, AttractionInfo& attraction_groups, int num_molecule_failures);

/**
 * @brief Score unclustered atoms that are two hops away from current cluster
 *
 * For example, consider a cluster that has a FF feeding an adder in another
 * cluster. Since this FF is feeding an adder that is packed in another cluster
 * this function should find other FFs that are feeding other inputs of this adder
 * since they are two hops away from the FF packed in this cluster
 */
void load_transitive_fanout_candidates(LegalizationClusterId cluster_index,
                                       t_pb_stats* pb_stats,
                                       const Prepacker& prepacker,
                                       const ClusterLegalizer& cluster_legalizer,
                                       vtr::vector<LegalizationClusterId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                                       int transitive_fanout_threshold);

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
 * @brief Given a pointer to a pb in a cluster, this routine returns a pointer
 *        to the top-level pb of the given pb.
 *
 * This is needed when updating the gain for a cluster.
 */
t_pb* get_top_level_pb(t_pb* pb);

/*
 * @brief Load the mapping between clusters and their atoms.
 */
void init_clb_atoms_lookup(vtr::vector<ClusterBlockId, std::unordered_set<AtomBlockId>>& atoms_lookup,
                           const AtomContext& atom_ctx,
                           const ClusteredNetlist& clb_nlist);
#endif
