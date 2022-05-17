#ifndef CLUSTER_UTIL_H
#define CLUSTER_UTIL_H

#include "globals.h"
#include "atom_netlist.h"
#include "pack_types.h"
#include "echo_files.h"
#include "vpr_utils.h"
#include "constraints_report.h"

#include "timing_info.h"
#include "PreClusterDelayCalculator.h"
#include "PreClusterTimingGraphResolver.h"
#include "tatum/echo_writer.hpp"
#include "tatum/TimingReporter.hpp"

#define AAPACK_MAX_HIGH_FANOUT_EXPLORE 10 /* For high-fanout nets that are ignored, consider a maximum of this many sinks, must be less than packer_opts.feasible_block_array_size */
#define AAPACK_MAX_TRANSITIVE_EXPLORE 40  /* When investigating transitive fanout connections in packing, consider a maximum of this many molecules, must be less than packer_opts.feasible_block_array_size */

//Constant allowing all cluster pins to be used
const t_ext_pin_util FULL_EXTERNAL_PIN_UTIL(1., 1.);

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

enum e_detailed_routing_stages {
    E_DETAILED_ROUTE_AT_END_ONLY = 0,
    E_DETAILED_ROUTE_FOR_EACH_ATOM,
    E_DETAILED_ROUTE_INVALID
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

/* Useful data structures for packing */
struct t_clustering_data {
    vtr::vector<ClusterBlockId, std::vector<t_intra_lb_net>*> intra_lb_routing;
    int* hill_climbing_inputs_avail;

    /* Keeps a linked list of the unclustered blocks to speed up looking for *                                                                  
     * unclustered blocks with a certain number of *external* inputs.        *
     * [0..lut_size].  Unclustered_list_head[i] points to the head of the    *
     * list of blocks with i inputs to be hooked up via external interconnect. */
    t_molecule_link* unclustered_list_head = nullptr;

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

void check_clustering();

//calculate the initial timing at the start of packing stage
void calc_init_packing_timing(const t_packer_opts& packer_opts,
                              const t_analysis_opts& analysis_opts,
                              const std::unordered_map<AtomBlockId, t_pb_graph_node*>& expected_lowest_cost_pb_gnode,
                              std::shared_ptr<PreClusterDelayCalculator>& clustering_delay_calc,
                              std::shared_ptr<SetupTimingInfo>& timing_info,
                              vtr::vector<AtomBlockId, float>& atom_criticality);

//free the clustering data structures
void free_clustering_data(const t_packer_opts& packer_opts,
                          t_clustering_data& clustering_data);

//check clustering legality and output it
void check_and_output_clustering(const t_packer_opts& packer_opts,
                                 const std::unordered_set<AtomNetId>& is_clock,
                                 const t_arch* arch,
                                 const int& num_clb,
                                 const vtr::vector<ClusterBlockId, std::vector<t_intra_lb_net>*>& intra_lb_routing);

void get_max_cluster_size_and_pb_depth(int& max_cluster_size,
                                       int& max_pb_depth);

bool check_cluster_legality(const int& verbosity,
                            const int& detailed_routing_stage,
                            t_lb_router_data* router_data);

bool is_atom_blk_in_pb(const AtomBlockId blk_id, const t_pb* pb);

void add_molecule_to_pb_stats_candidates(t_pack_molecule* molecule,
                                         std::map<AtomBlockId, float>& gain,
                                         t_pb* pb,
                                         int max_queue_size,
                                         AttractionInfo& attraction_groups);

void remove_molecule_from_pb_stats_candidates(t_pack_molecule* molecule,
                                              t_pb* pb);

void alloc_and_init_clustering(const t_molecule_stats& max_molecule_stats,
                               t_cluster_placement_stats** cluster_placement_stats,
                               t_pb_graph_node*** primitives_list,
                               t_pack_molecule* molecules_head,
                               t_clustering_data& clustering_data,
                               std::unordered_map<AtomNetId, int>& net_output_feeds_driving_block_input,
                               int& unclustered_list_head_size,
                               int num_molecules);

void free_pb_stats_recursive(t_pb* pb);

void try_update_lookahead_pins_used(t_pb* cur_pb);

void reset_lookahead_pins_used(t_pb* cur_pb);

void compute_and_mark_lookahead_pins_used(const AtomBlockId blk_id);

void compute_and_mark_lookahead_pins_used_for_pin(const t_pb_graph_pin* pb_graph_pin,
                                                  const t_pb* primitive_pb,
                                                  const AtomNetId net_id);

void commit_lookahead_pins_used(t_pb* cur_pb);

bool check_lookahead_pins_used(t_pb* cur_pb, t_ext_pin_util max_external_pin_util);

bool primitive_feasible(const AtomBlockId blk_id, t_pb* cur_pb);

bool primitive_memory_sibling_feasible(const AtomBlockId blk_id, const t_pb_type* cur_pb_type, const AtomBlockId sibling_memory_blk);

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
                       AttractionInfo& attraction_groups);

void rebuild_attraction_groups(AttractionInfo& attraction_groups);

void record_molecule_failure(t_pack_molecule* molecule, t_pb* pb);

enum e_block_pack_status try_pack_molecule(t_cluster_placement_stats* cluster_placement_stats_ptr,
                                           t_pack_molecule* molecule,
                                           t_pb_graph_node** primitives_list,
                                           t_pb* pb,
                                           const int max_models,
                                           const int max_cluster_size,
                                           const ClusterBlockId clb_index,
                                           const int detailed_routing_stage,
                                           t_lb_router_data* router_data,
                                           int verbosity,
                                           bool enable_pin_feasibility_filter,
                                           const int feasible_block_array_size,
                                           t_ext_pin_util max_external_pin_util,
                                           PartitionRegion& temp_cluster_pr);

void try_fill_cluster(const t_packer_opts& packer_opts,
                      t_cluster_placement_stats* cur_cluster_placement_stats_ptr,
                      t_pack_molecule*& prev_molecule,
                      t_pack_molecule*& next_molecule,
                      int& num_same_molecules,
                      t_pb_graph_node** primitives_list,
                      t_cluster_progress_stats& cluster_stats,
                      int num_clb,
                      const int num_models,
                      const int max_cluster_size,
                      const ClusterBlockId clb_index,
                      const int detailed_routing_stage,
                      AttractionInfo& attraction_groups,
                      vtr::vector<ClusterBlockId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                      bool allow_unrelated_clustering,
                      const int& high_fanout_threshold,
                      const std::unordered_set<AtomNetId>& is_clock,
                      const std::shared_ptr<SetupTimingInfo>& timing_info,
                      t_lb_router_data* router_data,
                      t_ext_pin_util target_ext_pin_util,
                      PartitionRegion& temp_cluster_pr,
                      e_block_pack_status& block_pack_status,
                      t_molecule_link* unclustered_list_head,
                      const int& unclustered_list_head_size,
                      std::unordered_map<AtomNetId, int>& net_output_feeds_driving_block_input,
                      std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types);

t_pack_molecule* save_cluster_routing_and_pick_new_seed(const t_packer_opts& packer_opts,
                                                        const int& num_clb,
                                                        const std::vector<AtomBlockId>& seed_atoms,
                                                        const int& num_blocks_hill_added,
                                                        vtr::vector<ClusterBlockId, std::vector<t_intra_lb_net>*>& intra_lb_routing,
                                                        int& seedindex,
                                                        t_cluster_progress_stats& cluster_stats,
                                                        t_lb_router_data* router_data);

void store_cluster_info_and_free(const t_packer_opts& packer_opts,
                                 const ClusterBlockId& clb_index,
                                 const t_logical_block_type_ptr logic_block_type,
                                 const t_pb_type* le_pb_type,
                                 std::vector<int>& le_count,
                                 vtr::vector<ClusterBlockId, std::vector<AtomNetId>>& clb_inter_blk_nets);

void free_data_and_requeue_used_mols_if_illegal(const ClusterBlockId& clb_index,
                                                const int& savedseedindex,
                                                std::map<t_logical_block_type_ptr, size_t>& num_used_type_instances,
                                                int& num_clb,
                                                int& seedindex);

enum e_block_pack_status try_place_atom_block_rec(const t_pb_graph_node* pb_graph_node,
                                                  const AtomBlockId blk_id,
                                                  t_pb* cb,
                                                  t_pb** parent,
                                                  const int max_models,
                                                  const int max_cluster_size,
                                                  const ClusterBlockId clb_index,
                                                  const t_cluster_placement_stats* cluster_placement_stats_ptr,
                                                  const t_pack_molecule* molecule,
                                                  t_lb_router_data* router_data,
                                                  int verbosity,
                                                  const int feasible_block_array_size);

enum e_block_pack_status atom_cluster_floorplanning_check(const AtomBlockId blk_id,
                                                          const ClusterBlockId clb_index,
                                                          const int verbosity,
                                                          PartitionRegion& temp_cluster_pr,
                                                          bool& cluster_pr_needs_update);

void revert_place_atom_block(const AtomBlockId blk_id, t_lb_router_data* router_data);

void update_connection_gain_values(const AtomNetId net_id, const AtomBlockId clustered_blk_id, t_pb* cur_pb, enum e_net_relation_to_clustered_block net_relation_to_clustered_block);

void update_timing_gain_values(const AtomNetId net_id,
                               t_pb* cur_pb,
                               enum e_net_relation_to_clustered_block net_relation_to_clustered_block,
                               const SetupTimingInfo& timing_info,
                               const std::unordered_set<AtomNetId>& is_global,
                               std::unordered_map<AtomNetId, int>& net_output_feeds_driving_block_input);

void mark_and_update_partial_gain(const AtomNetId net_id,
                                  enum e_gain_update gain_flag,
                                  const AtomBlockId clustered_blk_id,
                                  bool timing_driven,
                                  bool connection_driven,
                                  enum e_net_relation_to_clustered_block net_relation_to_clustered_block,
                                  const SetupTimingInfo& timing_info,
                                  const std::unordered_set<AtomNetId>& is_global,
                                  const int high_fanout_net_threshold,
                                  std::unordered_map<AtomNetId, int>& net_output_feeds_driving_block_input);

void update_total_gain(float alpha, float beta, bool timing_driven, bool connection_driven, t_pb* pb, AttractionInfo& attraction_groups);

void update_cluster_stats(const t_pack_molecule* molecule,
                          const ClusterBlockId clb_index,
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

void start_new_cluster(t_cluster_placement_stats* cluster_placement_stats,
                       t_pb_graph_node** primitives_list,
                       ClusterBlockId clb_index,
                       t_pack_molecule* molecule,
                       std::map<t_logical_block_type_ptr, size_t>& num_used_type_instances,
                       const float target_device_utilization,
                       const int num_models,
                       const int max_cluster_size,
                       const t_arch* arch,
                       std::string device_layout_name,
                       std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
                       t_lb_router_data** router_data,
                       const int detailed_routing_stage,
                       ClusteredNetlist* clb_nlist,
                       const std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types,
                       int verbosity,
                       bool enable_pin_feasibility_filter,
                       bool balance_block_type_utilization,
                       const int feasible_block_array_size,
                       PartitionRegion& temp_cluster_pr);

t_pack_molecule* get_highest_gain_molecule(t_pb* cur_pb,
                                           AttractionInfo& attraction_groups,
                                           const enum e_gain_type gain_mode,
                                           t_cluster_placement_stats* cluster_placement_stats_ptr,
                                           vtr::vector<ClusterBlockId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                                           const ClusterBlockId cluster_index,
                                           bool prioritize_transitive_connectivity,
                                           int transitive_fanout_threshold,
                                           const int feasible_block_array_size,
                                           std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types);

void add_cluster_molecule_candidates_by_connectivity_and_timing(t_pb* cur_pb,
                                                                t_cluster_placement_stats* cluster_placement_stats_ptr,
                                                                const int feasible_block_array_size,
                                                                AttractionInfo& attraction_groups);

void add_cluster_molecule_candidates_by_highfanout_connectivity(t_pb* cur_pb,
                                                                t_cluster_placement_stats* cluster_placement_stats_ptr,
                                                                const int feasible_block_array_size,
                                                                AttractionInfo& attraction_groups);

void add_cluster_molecule_candidates_by_attraction_group(t_pb* cur_pb,
                                                         t_cluster_placement_stats* cluster_placement_stats_ptr,
                                                         AttractionInfo& attraction_groups,
                                                         const int feasible_block_array_size,
                                                         ClusterBlockId clb_index,
                                                         std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types);

void add_cluster_molecule_candidates_by_transitive_connectivity(t_pb* cur_pb,
                                                                t_cluster_placement_stats* cluster_placement_stats_ptr,
                                                                vtr::vector<ClusterBlockId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                                                                const ClusterBlockId cluster_index,
                                                                int transitive_fanout_threshold,
                                                                const int feasible_block_array_size,
                                                                AttractionInfo& attraction_groups);

bool check_free_primitives_for_molecule_atoms(t_pack_molecule* molecule, t_cluster_placement_stats* cluster_placement_stats_ptr);

t_pack_molecule* get_molecule_for_cluster(t_pb* cur_pb,
                                          AttractionInfo& attraction_groups,
                                          const bool allow_unrelated_clustering,
                                          const bool prioritize_transitive_connectivity,
                                          const int transitive_fanout_threshold,
                                          const int feasible_block_array_size,
                                          int* num_unrelated_clustering_attempts,
                                          t_cluster_placement_stats* cluster_placement_stats_ptr,
                                          vtr::vector<ClusterBlockId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                                          ClusterBlockId cluster_index,
                                          int verbosity,
                                          t_molecule_link* unclustered_list_head,
                                          const int& unclustered_list_head_size,
                                          std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types);

void mark_all_molecules_valid(t_pack_molecule* molecule_head);

int count_molecules(t_pack_molecule* molecule_head);

t_molecule_stats calc_molecule_stats(const t_pack_molecule* molecule);

t_molecule_stats calc_max_molecules_stats(const t_pack_molecule* molecule_head);

std::vector<AtomBlockId> initialize_seed_atoms(const e_cluster_seed seed_type,
                                               const t_molecule_stats& max_molecule_stats,
                                               const vtr::vector<AtomBlockId, float>& atom_criticality);

t_pack_molecule* get_highest_gain_seed_molecule(int* seedindex, const std::vector<AtomBlockId> seed_atoms);

float get_molecule_gain(t_pack_molecule* molecule, std::map<AtomBlockId, float>& blk_gain, AttractGroupId cluster_attraction_group_id, AttractionInfo& attraction_groups, int num_molecule_failures);

int compare_molecule_gain(const void* a, const void* b);
int net_sinks_reachable_in_cluster(const t_pb_graph_pin* driver_pb_gpin, const int depth, const AtomNetId net_id);

void print_seed_gains(const char* fname, const std::vector<AtomBlockId>& seed_atoms, const vtr::vector<AtomBlockId, float>& atom_gain, const vtr::vector<AtomBlockId, float>& atom_criticality);

void load_transitive_fanout_candidates(ClusterBlockId cluster_index,
                                       t_pb_stats* pb_stats,
                                       vtr::vector<ClusterBlockId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                                       int transitive_fanout_threshold);

std::map<const t_model*, std::vector<t_logical_block_type_ptr>> identify_primitive_candidate_block_types();

void update_molecule_chain_info(t_pack_molecule* chain_molecule, const t_pb_graph_node* root_primitive);

enum e_block_pack_status check_chain_root_placement_feasibility(const t_pb_graph_node* pb_graph_node,
                                                                const t_pack_molecule* molecule,
                                                                const AtomBlockId blk_id);

t_pb_graph_pin* get_driver_pb_graph_pin(const t_pb* driver_pb, const AtomPinId driver_pin_id);

size_t update_pb_type_count(const t_pb* pb, std::map<t_pb_type*, int>& pb_type_count, size_t depth);

void update_le_count(const t_pb* pb, const t_logical_block_type_ptr logic_block_type, const t_pb_type* le_pb_type, std::vector<int>& le_count);

void print_pb_type_count_recurr(t_pb_type* type, size_t max_name_chars, size_t curr_depth, std::map<t_pb_type*, int>& pb_type_count);

t_logical_block_type_ptr identify_logic_block_type(std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types);

t_pb_type* identify_le_block_type(t_logical_block_type_ptr logic_block_type);

bool pb_used_for_blif_model(const t_pb* pb, std::string blif_model_name);

void print_le_count(std::vector<int>& le_count, const t_pb_type* le_pb_type);

t_pb* get_top_level_pb(t_pb* pb);

bool cleanup_pb(t_pb* pb);

void alloc_and_load_pb_stats(t_pb* pb, const int feasible_block_array_size);

#endif