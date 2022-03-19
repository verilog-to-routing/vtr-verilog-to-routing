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
                          vtr::vector<ClusterBlockId, std::vector<t_intra_lb_net>*>& intra_lb_routing,
                          int* hill_climbing_inputs_avail,
                          t_cluster_placement_stats* cluster_placement_stats,
                          t_molecule_link* unclustered_list_head,
                          t_molecule_link* memory_pool,
                          t_pb_graph_node** primitives_list);

//check clustering legality and output it
void check_and_output_clustering(const t_packer_opts& packer_opts,
                                 const std::unordered_set<AtomNetId>& is_clock,
                                 const t_arch* arch,
                                 const int& num_clb,
                                 const vtr::vector<ClusterBlockId, std::vector<t_intra_lb_net>*>& intra_lb_routing,
                                 bool& floorplan_regions_overfull);

void get_max_cluster_size_and_pb_depth(int& max_cluster_size,
                                       int& max_pb_depth);

bool check_cluster_legality(const int& verbosity,
                            const int& detailed_routing_stage,
                            t_lb_router_data* router_data);
