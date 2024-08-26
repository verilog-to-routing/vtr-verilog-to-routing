#ifndef CLUSTER_UTIL_H
#define CLUSTER_UTIL_H

#include <vector>
#include "cluster_legalizer.h"
#include "pack_types.h"
#include "vtr_vector.h"

class AtomNetId;
class ClusterBlockId;
class PreClusterDelayCalculator;
class Prepacker;
class SetupTimingInfo;
class t_pack_molecule;

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

struct t_cluster_progress_stats {
    int num_molecules = 0;
    int num_molecules_processed = 0;
    int mols_since_last_print = 0;
    int blocks_since_last_analysis = 0;
    int num_unrelated_clustering_attempts = 0;
};

/* Useful data structures for creating or modifying clusters */
struct t_clustering_data {
    int* hill_climbing_inputs_avail;

    /* Keeps a linked list of the unclustered blocks to speed up looking for *                                                                  
     * unclustered blocks with a certain number of *external* inputs.        *
     * [0..lut_size].  Unclustered_list_head[i] points to the head of the    *
     * list of blocks with i inputs to be hooked up via external interconnect. */
    t_molecule_link* unclustered_list_head = nullptr;

    //Maintaining a linked list of free molecule data for speed
    t_molecule_link* memory_pool = nullptr;

    /* Does the atom block that drives the output of this atom net also appear as a   *
     * receiver (input) pin of the atom net? If so, then by how much?
     *
     * This is used in the gain routines to avoid double counting the connections from   *
     * the current cluster to other blocks (hence yielding better clusterings). *
     * The only time an atom block should connect to the same atom net *
     * twice is when one connection is an output and the other is an input, *
     * so this should take care of all multiple connections.                */
    std::unordered_map<AtomNetId, int> net_output_feeds_driving_block_input;
};

/***********************************/
/*   Clustering helper functions   */
/***********************************/

//calculate the initial timing at the start of packing stage
void calc_init_packing_timing(const t_packer_opts& packer_opts,
                              const t_analysis_opts& analysis_opts,
                              const Prepacker& prepacker,
                              std::shared_ptr<PreClusterDelayCalculator>& clustering_delay_calc,
                              std::shared_ptr<SetupTimingInfo>& timing_info,
                              vtr::vector<AtomBlockId, float>& atom_criticality);

//free the clustering data structures
void free_clustering_data(const t_packer_opts& packer_opts,
                          t_clustering_data& clustering_data);

//check clustering legality and output it
void check_and_output_clustering(ClusterLegalizer& cluster_legalizer,
                                 const t_packer_opts& packer_opts,
                                 const std::unordered_set<AtomNetId>& is_clock,
                                 const t_arch* arch);

bool is_atom_blk_in_pb(const AtomBlockId blk_id, const t_pb* pb);

void add_molecule_to_pb_stats_candidates(t_pack_molecule* molecule,
                                         std::map<AtomBlockId, float>& gain,
                                         t_pb* pb,
                                         int max_queue_size,
                                         AttractionInfo& attraction_groups);

void remove_molecule_from_pb_stats_candidates(t_pack_molecule* molecule,
                                              t_pb* pb);

void alloc_and_init_clustering(const t_molecule_stats& max_molecule_stats,
                               const Prepacker& prepacker,
                               t_clustering_data& clustering_data,
                               std::unordered_map<AtomNetId, int>& net_output_feeds_driving_block_input,
                               int& unclustered_list_head_size,
                               int num_molecules);

t_pack_molecule* get_molecule_by_num_ext_inputs(const int ext_inps,
                                                const enum e_removal_policy remove_flag,
                                                t_cluster_placement_stats* cluster_placement_stats_ptr,
                                                t_molecule_link* unclustered_list_head);

t_pack_molecule* get_free_molecule_with_most_ext_inputs_for_cluster(t_pb* cur_pb,
                                                                    t_cluster_placement_stats* cluster_placement_stats_ptr,
                                                                    t_molecule_link* unclustered_list_head,
                                                                    const int& unclustered_list_head_size);

void print_pack_status_header();

void print_pack_status(int num_clb,
                       int tot_num_molecules,
                       int num_molecules_processed,
                       int& mols_since_last_print,
                       int device_width,
                       int device_height,
                       AttractionInfo& attraction_groups,
                       const ClusterLegalizer& cluster_legalizer);

void rebuild_attraction_groups(AttractionInfo& attraction_groups,
                               const ClusterLegalizer& cluster_legalizer);

void try_fill_cluster(ClusterLegalizer& cluster_legalizer,
                      const Prepacker& prepacker,
                      const t_packer_opts& packer_opts,
                      t_cluster_placement_stats* cur_cluster_placement_stats_ptr,
                      t_pack_molecule*& prev_molecule,
                      t_pack_molecule*& next_molecule,
                      int& num_same_molecules,
                      t_cluster_progress_stats& cluster_stats,
                      int num_clb,
                      const LegalizationClusterId legalization_cluster_id,
                      AttractionInfo& attraction_groups,
                      vtr::vector<LegalizationClusterId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                      bool allow_unrelated_clustering,
                      const int& high_fanout_threshold,
                      const std::unordered_set<AtomNetId>& is_clock,
                      const std::unordered_set<AtomNetId>& is_global,
                      const std::shared_ptr<SetupTimingInfo>& timing_info,
                      e_block_pack_status& block_pack_status,
                      t_molecule_link* unclustered_list_head,
                      const int& unclustered_list_head_size,
                      std::unordered_map<AtomNetId, int>& net_output_feeds_driving_block_input,
                      std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types);

void store_cluster_info_and_free(const t_packer_opts& packer_opts,
                                 const LegalizationClusterId clb_index,
                                 const t_logical_block_type_ptr logic_block_type,
                                 const t_pb_type* le_pb_type,
                                 std::vector<int>& le_count,
                                 const ClusterLegalizer& cluster_legalizer,
                                 vtr::vector<LegalizationClusterId, std::vector<AtomNetId>>& clb_inter_blk_nets);

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
                               std::unordered_map<AtomNetId, int>& net_output_feeds_driving_block_input);

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
                                  std::unordered_map<AtomNetId, int>& net_output_feeds_driving_block_input);

void update_total_gain(float alpha, float beta, bool timing_driven, bool connection_driven, t_pb* pb, AttractionInfo& attraction_groups);

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
                          std::unordered_map<AtomNetId, int>& net_output_feeds_driving_block_input);

void start_new_cluster(ClusterLegalizer& cluster_legalizer,
                       LegalizationClusterId& legalization_cluster_id,
                       t_pack_molecule* molecule,
                       std::map<t_logical_block_type_ptr, size_t>& num_used_type_instances,
                       const float target_device_utilization,
                       const t_arch* arch,
                       const std::string& device_layout_name,
                       const std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types,
                       int verbosity,
                       bool balance_block_type_utilization);

t_pack_molecule* get_highest_gain_molecule(t_pb* cur_pb,
                                           AttractionInfo& attraction_groups,
                                           const enum e_gain_type gain_mode,
                                           t_cluster_placement_stats* cluster_placement_stats_ptr,
                                           const Prepacker& prepacker,
                                           const ClusterLegalizer& cluster_legalizer,
                                           vtr::vector<LegalizationClusterId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                                           const LegalizationClusterId cluster_index,
                                           bool prioritize_transitive_connectivity,
                                           int transitive_fanout_threshold,
                                           const int feasible_block_array_size,
                                           std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types);

void add_cluster_molecule_candidates_by_connectivity_and_timing(t_pb* cur_pb,
                                                                t_cluster_placement_stats* cluster_placement_stats_ptr,
                                                                const Prepacker& prepacker,
                                                                const ClusterLegalizer& cluster_legalizer,
                                                                const int feasible_block_array_size,
                                                                AttractionInfo& attraction_groups);

void add_cluster_molecule_candidates_by_highfanout_connectivity(t_pb* cur_pb,
                                                                t_cluster_placement_stats* cluster_placement_stats_ptr,
                                                                const Prepacker& prepacker,
                                                                const ClusterLegalizer& cluster_legalizer,
                                                                const int feasible_block_array_size,
                                                                AttractionInfo& attraction_groups);

void add_cluster_molecule_candidates_by_attraction_group(t_pb* cur_pb,
                                                         t_cluster_placement_stats* cluster_placement_stats_ptr,
                                                         const Prepacker& prepacker,
                                                         const ClusterLegalizer& cluster_legalizer,
                                                         AttractionInfo& attraction_groups,
                                                         const int feasible_block_array_size,
                                                         LegalizationClusterId clb_index,
                                                         std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types);

void add_cluster_molecule_candidates_by_transitive_connectivity(t_pb* cur_pb,
                                                                t_cluster_placement_stats* cluster_placement_stats_ptr,
                                                                const Prepacker& prepacker,
                                                                const ClusterLegalizer& cluster_legalizer,
                                                                vtr::vector<LegalizationClusterId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                                                                const LegalizationClusterId cluster_index,
                                                                int transitive_fanout_threshold,
                                                                const int feasible_block_array_size,
                                                                AttractionInfo& attraction_groups);

bool check_free_primitives_for_molecule_atoms(t_pack_molecule* molecule,
                                              t_cluster_placement_stats* cluster_placement_stats_ptr,
                                              const ClusterLegalizer& cluster_legalizer);

t_pack_molecule* get_molecule_for_cluster(t_pb* cur_pb,
                                          AttractionInfo& attraction_groups,
                                          const bool allow_unrelated_clustering,
                                          const bool prioritize_transitive_connectivity,
                                          const int transitive_fanout_threshold,
                                          const int feasible_block_array_size,
                                          int* num_unrelated_clustering_attempts,
                                          t_cluster_placement_stats* cluster_placement_stats_ptr,
                                          const Prepacker& prepacker,
                                          const ClusterLegalizer& cluster_legalizer,
                                          vtr::vector<LegalizationClusterId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                                          LegalizationClusterId cluster_index,
                                          int verbosity,
                                          t_molecule_link* unclustered_list_head,
                                          const int& unclustered_list_head_size,
                                          std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types);

t_molecule_stats calc_molecule_stats(const t_pack_molecule* molecule, const AtomNetlist& atom_nlist);

std::vector<AtomBlockId> initialize_seed_atoms(const e_cluster_seed seed_type,
                                               const t_molecule_stats& max_molecule_stats,
                                               const Prepacker& prepacker,
                                               const vtr::vector<AtomBlockId, float>& atom_criticality);

t_pack_molecule* get_highest_gain_seed_molecule(int& seed_index,
                                                const std::vector<AtomBlockId>& seed_atoms,
                                                const Prepacker& prepacker,
                                                const ClusterLegalizer& cluster_legalizer);

float get_molecule_gain(t_pack_molecule* molecule, std::map<AtomBlockId, float>& blk_gain, AttractGroupId cluster_attraction_group_id, AttractionInfo& attraction_groups, int num_molecule_failures);

void print_seed_gains(const char* fname, const std::vector<AtomBlockId>& seed_atoms, const vtr::vector<AtomBlockId, float>& atom_gain, const vtr::vector<AtomBlockId, float>& atom_criticality);

void load_transitive_fanout_candidates(LegalizationClusterId cluster_index,
                                       t_pb_stats* pb_stats,
                                       const Prepacker& prepacker,
                                       const ClusterLegalizer& cluster_legalizer,
                                       vtr::vector<LegalizationClusterId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                                       int transitive_fanout_threshold);

std::map<const t_model*, std::vector<t_logical_block_type_ptr>> identify_primitive_candidate_block_types();

size_t update_pb_type_count(const t_pb* pb, std::map<t_pb_type*, int>& pb_type_count, size_t depth);

void update_le_count(const t_pb* pb, const t_logical_block_type_ptr logic_block_type, const t_pb_type* le_pb_type, std::vector<int>& le_count);

void print_pb_type_count_recurr(t_pb_type* type, size_t max_name_chars, size_t curr_depth, std::map<t_pb_type*, int>& pb_type_count);

t_logical_block_type_ptr identify_logic_block_type(std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types);

t_pb_type* identify_le_block_type(t_logical_block_type_ptr logic_block_type);

bool pb_used_for_blif_model(const t_pb* pb, const std::string& blif_model_name);

void print_le_count(std::vector<int>& le_count, const t_pb_type* le_pb_type);

t_pb* get_top_level_pb(t_pb* pb);

void init_clb_atoms_lookup(vtr::vector<ClusterBlockId, std::unordered_set<AtomBlockId>>& atoms_lookup);
#endif
