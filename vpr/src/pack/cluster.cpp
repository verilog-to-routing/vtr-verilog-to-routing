/*
 * Main clustering algorithm
 * Author(s): Vaughn Betz (first revision - VPack), Alexander Marquardt (second revision - T-VPack), Jason Luu (third revision - AAPack)
 * June 8, 2011
 */

/*
 * The clusterer uses several key data structures:
 *
 *      t_pb_type (and related types):
 *          Represent the architecture as described in the architecture file.
 *
 *      t_pb_graph_node (and related types):
 *          Represents a flattened version of the architecture with t_pb_types 
 *          expanded (according to num_pb) into unique t_pb_graph_node instances, 
 *          and the routing connectivity converted to a graph of t_pb_graph_pin (nodes)
 *          and t_pb_graph_edge.
 *
 *      t_pb:
 *          Represents a clustered instance of a t_pb_graph_node containing netlist primitives
 *
 *  t_pb_type and t_pb_graph_node (and related types) describe the targetted FPGA architecture, while t_pb represents
 *  the actual clustering of the user netlist.
 *
 *  For example:
 *      Consider an architecture where CLBs contain 4 BLEs, and each BLE is a LUT + FF pair.  
 *      We wish to map a netlist of 400 LUTs and 400 FFs.
 *
 *      A BLE corresponds to one t_pb_type (which has num_pb = 4).
 *
 *      Each of the 4 BLE positions corresponds to a t_pb_graph_node (each of which references the BLE t_pb_type).
 *
 *      The output of clustering is 400 t_pb of type BLE which represent the clustered user netlist.
 *      Each of the 400 t_pb will reference one of the 4 BLE-type t_pb_graph_nodes.
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <map>
#include <algorithm>
#include <fstream>
using namespace std;

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_math.h"
#include "vtr_memory.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "atom_netlist.h"
#include "pack_types.h"
#include "cluster.h"
#include "output_clustering.h"
#include "SetupGrid.h"
#include "read_xml_arch_file.h"
#include "path_delay2.h"
#include "path_delay.h"
#include "vpr_utils.h"
#include "cluster_placement.h"
#include "echo_files.h"
#include "cluster_router.h"
#include "lb_type_rr_graph.h"

#include "timing_info.h"
#include "PreClusterDelayCalculator.h"
#include "tatum/echo_writer.hpp"
#include "tatum/report/graphviz_dot_writer.hpp"

#include "read_sdc.h"

#define AAPACK_MAX_FEASIBLE_BLOCK_ARRAY_SIZE 30      /* This value is used to determine the max size of the priority queue for candidates that pass the early filter legality test but not the more detailed routing test */
#define AAPACK_MAX_NET_SINKS_IGNORE 256				/* The packer looks at all sinks of a net when deciding what next candidate block to pack, for high-fanout nets, this is too runtime costly for marginal benefit, thus ignore those high fanout nets */
#define AAPACK_MAX_HIGH_FANOUT_EXPLORE 10			/* For high-fanout nets that are ignored, consider a maximum of this many sinks, must be less than AAPACK_MAX_FEASIBLE_BLOCK_ARRAY_SIZE */
#define AAPACK_MAX_TRANSITIVE_FANOUT_EXPLORE 4		/* When investigating transitive fanout connections in packing, this is the highest fanout net that will be explored */
#define AAPACK_MAX_TRANSITIVE_EXPLORE 4				/* When investigating transitive fanout connections in packing, consider a maximum of this many molecules, must be less tahn AAPACK_MAX_FEASIBLE_BLOCK_ARRAY_SIZE */

#define SCALE_NUM_PATHS 1e-2     /*this value is used as a multiplier to assign a    *
				  *slightly higher criticality value to nets that    *
				  *affect a large number of critical paths versus    *
				  *nets that do not have such a broad effect.        *
				  *Note that this multiplier is intentionally very   * 
				  *small compared to the total criticality because   *
				  *we want to make sure that atom net criticality is      *
				  *primarily determined by slacks, with this acting  *
				  *only as a tie-breaker between otherwise equal nets*/
#define SCALE_DISTANCE_VAL 1e-4  /*this value is used as a multiplier to assign a    *
				  *slightly higher criticality value to nets that    *
				  *are otherwise equal but one is  farther           *
				  *from sources (the farther one being made slightly *
				  *more critical)                                    */

enum e_gain_update {
	GAIN, NO_GAIN
};
enum e_feasibility {
	FEASIBLE, INFEASIBLE
};
enum e_gain_type {
	HILL_CLIMBING, NOT_HILL_CLIMBING
};
enum e_removal_policy {
	REMOVE_CLUSTERED, LEAVE_CLUSTERED
};
/* TODO: REMOVE_CLUSTERED no longer used, remove */
enum e_net_relation_to_clustered_block {
	INPUT, OUTPUT
};

enum e_detailed_routing_stages {
	E_DETAILED_ROUTE_AT_END_ONLY = 0, E_DETAILED_ROUTE_FOR_EACH_ATOM, E_DETAILED_ROUTE_END
};


/* Linked list structure.  Stores one integer (iblk). */
struct t_molecule_link {
	PackMoleculeId molecule_id;
	t_molecule_link *next;
};

/* Keeps a linked list of the unclustered blocks to speed up looking for *
 * unclustered blocks with a certain number of *external* inputs.        *
 * [0..lut_size].  Unclustered_list_head[i] points to the head of the    *
 * list of blocks with i inputs to be hooked up via external interconnect. */
static t_molecule_link *unclustered_list_head;
int unclustered_list_head_size;
static t_molecule_link *memory_pool; /*Declared here so I can free easily.*/

/* Does the atom block that drives the output of this atom net also appear as a   *
 * receiver (input) pin of the atom net? If so, then by how much? 
 *
 * This is used in the gain routines to avoid double counting the connections from   *
 * the current cluster to other blocks (hence yielding better clusterings). *
 * The only time an atom block should connect to the same atom net *
 * twice is when one connection is an output and the other is an input, *
 * so this should take care of all multiple connections.                */
static std::unordered_map<AtomNetId,int> net_output_feeds_driving_block_input;

/* Timing information for blocks */
static std::vector<AtomBlockId> critindexarray;
static unordered_map<AtomBlockId,float> block_criticality;

/* Score different seeds for blocks */
static std::vector<AtomBlockId> seed_blend_index_array;
static std::unordered_map<AtomBlockId,float> seed_blend_gain;
	


/*****************************************/
/*local functions*/
/*****************************************/

static void check_clocks(const std::unordered_set<AtomNetId>& is_clock);

#if 0
static void check_for_duplicate_inputs ();
#endif 

static bool is_atom_blk_in_pb(const AtomBlockId blk_id, const t_pb *pb);

static void add_molecule_to_pb_stats_candidates(const PackMolecules& molecules, const PackMoleculeId molecule_id, const MoleculeStats& molecule_stats,
		std::map<AtomBlockId, float> &gain, t_pb *pb, int max_queue_size);

static void alloc_and_init_clustering(const PackMolecules& molecules,
        const MoleculeStats& molecule_stats,
		std::vector<t_cluster_placement_stats>& cluster_placement_stats,
		vtr::vector<MoleculeBlockId,t_pb_graph_node*>& primitives_list);
static void free_pb_stats_recursive(t_pb *pb);
static void try_update_lookahead_pins_used(t_pb *cur_pb);
static void reset_lookahead_pins_used(t_pb *cur_pb);
static void compute_and_mark_lookahead_pins_used(const AtomBlockId blk_id);
static void compute_and_mark_lookahead_pins_used_for_pin(
		const t_pb_graph_pin *pb_graph_pin, const t_pb *primitive_pb, const AtomNetId net_id);
static void commit_lookahead_pins_used(t_pb *cur_pb);
static bool check_lookahead_pins_used(t_pb *cur_pb);
static bool primitive_feasible(const AtomBlockId blk_id, t_pb *cur_pb);
static bool primitive_memory_sibling_feasible(const AtomBlockId blk_id, const t_pb_type *cur_pb_type, const AtomBlockId sibling_memory_blk);

static PackMoleculeId get_molecule_by_num_ext_inputs(
        const PackMolecules& molecules,
        const MoleculeStats& molecule_stats,
		const int ext_inps, const enum e_removal_policy remove_flag,
		t_cluster_placement_stats& cluster_placement_stats);

static PackMoleculeId get_free_molecule_with_most_ext_inputs_for_cluster(
        const PackMolecules& molecules,
        const MoleculeStats& molecule_stats,
		t_pb *cur_pb,
		t_cluster_placement_stats& cluster_placement_stats);

static PackMoleculeId get_seed_logical_molecule_with_most_ext_inputs(const MoleculeStats& molecule_stats);

static enum e_block_pack_status try_pack_molecule(
		t_cluster_placement_stats& cluster_placement_stats,
        const PackMolecules& molecules,
        const PackMoleculeId molecule_id,
        MoleculeStats& molecule_stats,
        vtr::vector<MoleculeBlockId,t_pb_graph_node*>& primitives_list,
		t_pb * pb, const int max_models, const int max_cluster_size,
		const ClusterBlockId clb_index, const int detailed_routing_stage, t_lb_router_data *router_data,
        bool debug_clustering,
        bool enable_pin_feasibility_filter);
static enum e_block_pack_status try_place_atom_block_rec(
		const t_pb_graph_node *pb_graph_node, const AtomBlockId blk_id,
		t_pb *cb, t_pb **parent, const int max_models,
		const int max_cluster_size, const ClusterBlockId clb_index,
		const t_cluster_placement_stats& cluster_placement_stats,
		const bool is_root_of_chain, const t_pb_graph_pin *chain_root_pin, t_lb_router_data *router_data);
static void revert_place_atom_block(const AtomBlockId blk_id, t_lb_router_data *router_data, 
        const PackMolecules& molecules,
        MoleculeStats& atom_molecules);

static void update_connection_gain_values(const AtomNetId net_id, const AtomBlockId clustered_blk_id,
		t_pb * cur_pb,
		enum e_net_relation_to_clustered_block net_relation_to_clustered_block);

static void update_timing_gain_values(const AtomNetId net_id,
		t_pb* cur_pb,
		enum e_net_relation_to_clustered_block net_relation_to_clustered_block,
        const SetupTimingInfo& timing_info,
        const std::unordered_set<AtomNetId>& is_global);

static void mark_and_update_partial_gain(const AtomNetId inet, enum e_gain_update gain_flag,
		const AtomBlockId clustered_blk_id,
        bool timing_driven,
		bool connection_driven,
		enum e_net_relation_to_clustered_block net_relation_to_clustered_block, 
        const SetupTimingInfo& timing_info,
        const std::unordered_set<AtomNetId>& is_global);

static void update_total_gain(float alpha, float beta, bool timing_driven,
		bool connection_driven, t_pb *pb);

static void update_cluster_stats(
        const PackMolecules& molecules,
        const PackMoleculeId molecule_id,
		const ClusterBlockId clb_index,
        const std::unordered_set<AtomNetId>& is_clock, 
        const std::unordered_set<AtomNetId>& is_global, 
        const bool global_clocks,
		const float alpha, const float beta, const bool timing_driven,
		const bool connection_driven, 
        const SetupTimingInfo& timing_info);

static void start_new_cluster(
    std::vector<t_cluster_placement_stats>& cluster_placement_stats,
    vtr::vector<MoleculeBlockId,t_pb_graph_node*>& primitives_list,
    const PackMolecules& molecules,
    const PackMoleculeId molecule_id,
    MoleculeStats& molecule_stats,
	ClusterBlockId clb_index,
	std::map<t_type_ptr, size_t>& num_used_type_instances,
	const float target_device_utilization,
	const int num_models, const int max_cluster_size,
	const t_arch* arch,
	std::string device_layout_name,
	const vector<t_lb_type_rr_graph>& lb_type_rr_graphs,
    const std::vector<t_lb_type_rr_graph_info>& lb_type_rr_graph_infos,
	t_lb_router_data **router_data,
	const int detailed_routing_stage,
	ClusteredNetlist *clb_nlist,
    const std::map<const t_model*,std::vector<t_type_ptr>>& primitive_candidate_block_types,
    bool debug_clustering,
    bool enable_pin_feasibility_filter);

static PackMoleculeId get_highest_gain_molecule(
		t_pb *cur_pb,
        const PackMolecules& molecules,
        const MoleculeStats& molecule_stats,
		const enum e_gain_type gain_mode,
		t_cluster_placement_stats& cluster_placement_stats,
		vtr::vector_map<ClusterBlockId,std::vector<AtomNetId>> &clb_inter_blk_nets,
		const ClusterBlockId cluster_index);

static bool exists_free_primitives_for_molecule(const PackMolecule& molecule, t_cluster_placement_stats& cluster_placement_stats);

static PackMoleculeId get_molecule_for_cluster(
		t_pb *cur_pb,
        const PackMolecules& molecules,
        const MoleculeStats& molecule_stats,
		const bool allow_unrelated_clustering,
		int *num_unrelated_clustering_attempts,
		t_cluster_placement_stats& cluster_placement_stats,
		vtr::vector_map<ClusterBlockId,std::vector<AtomNetId>> &clb_inter_blk_nets,
		ClusterBlockId cluster_index);

static void check_clustering();

static void check_cluster_atom_blocks(t_pb *pb, std::unordered_set<AtomBlockId>& blocks_checked);

static PackMoleculeId get_highest_gain_seed_molecule(int * seedindex, const PackMolecules& molecules, const MoleculeStats& molecule_stats, bool getblend);

static float get_molecule_gain(const PackMolecules& molecules, const PackMoleculeId molecule_id, const MoleculeStats& molecule_stats, std::map<AtomBlockId, float> &blk_gain);
static int compare_molecule_gain(const void *a, const void *b);
int net_sinks_reachable_in_cluster(const t_pb_graph_pin* driver_pb_gpin, const int depth, const AtomNetId net_id);

static void print_block_criticalities(const char * fname);

static void load_transitive_fanout_candidates(ClusterBlockId cluster_index,
                                              const PackMolecules& molecules,
                                              const MoleculeStats& molecule_stats,
											  t_pb_stats *pb_stats,
											  vtr::vector_map<ClusterBlockId,std::vector<AtomNetId>> &clb_inter_blk_nets);

static std::map<const t_model*,std::vector<t_type_ptr>> identify_primitive_candidate_block_types();

static void revalid_molecules(const t_pb* pb, const PackMolecules& molecules, MoleculeStats& molecule_stats);

MoleculeStats build_molecule_stats(const PackMolecules& molecules, const AtomNetlist& netlist);
int count_molecule_blocks(const PackMolecule& molecule);
int count_molecule_ext_inputs(const PackMolecule& molecule, const AtomNetlist& netlist);

/*****************************************/
/*globally accessible function*/
void do_clustering(const t_arch *arch,
		int num_models, bool global_clocks, 
        const std::unordered_set<AtomNetId>& is_clock,
        const PackMolecules& molecules,
        const std::unordered_map<AtomBlockId,t_pb_graph_node*>& expected_lowest_cost_pb_gnode,
		bool hill_climbing_flag, const char *out_fname, bool timing_driven, 
		enum e_cluster_seed cluster_seed_type, float alpha, float beta,
        float inter_cluster_net_delay,
        const float target_device_utilization,
		bool allow_unrelated_clustering,
		bool connection_driven,
		enum e_packer_algorithm packer_algorithm,
        const vector<t_lb_type_rr_graph>& lb_type_rr_graphs,
		const std::vector<t_lb_type_rr_graph_info>& lb_type_rr_graph_infos,
        std::string device_layout_name,
        bool debug_clustering,
        bool enable_pin_feasibility_filter
#ifdef USE_HMETIS
		, vtr::vector_map<AtomBlockId, int>& partitions
#endif
#ifdef ENABLE_CLASSIC_VPR_STA
        , t_timing_inf timing_inf
#endif
        ) {

	/* Does the actual work of clustering multiple netlist blocks *
	 * into clusters.                                                  */

	/* Algorithm employed
	 1.  Find type that can legally hold block and create cluster with pb info
	 2.  Populate started cluster
	 3.  Repeat 1 until no more blocks need to be clustered
	 
	 */

	/****************************************************************
	* Initialization 
	*****************************************************************/
    VTR_ASSERT(packer_algorithm == PACK_GREEDY);

	int i, blocks_since_last_analysis, num_clb,
		num_blocks_hill_added, max_cluster_size, cur_cluster_size, 
		max_pb_depth, cur_pb_depth, num_unrelated_clustering_attempts,
		seedindex, savedseedindex /* index of next most timing critical block */,
		detailed_routing_stage, *hill_climbing_inputs_avail;

    std::map<t_type_ptr,size_t> num_used_type_instances;

	bool early_exit, is_cluster_legal;
	enum e_block_pack_status block_pack_status;

    vtr::vector<MoleculeBlockId,t_pb_graph_node*> primitives_list;
	t_lb_router_data *router_data = nullptr;
    PackMoleculeId istart, next_molecule, prev_molecule;

    auto& atom_ctx = g_vpr_ctx.atom();
    auto& device_ctx = g_vpr_ctx.mutable_device();
	auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    std::vector<t_cluster_placement_stats> cluster_placement_stats;
	vtr::vector_map<ClusterBlockId, std::vector<t_intra_lb_net>*> intra_lb_routing;

    std::shared_ptr<PreClusterDelayCalculator> clustering_delay_calc;
    std::shared_ptr<SetupTimingInfo> timing_info;

	num_clb = 0;
	seedindex = 0;

	/* TODO: This is memory inefficient, fix if causes problems */
	/* Store stats on nets used by packed block, useful for determining transitively connected blocks
	(eg. [A1, A2, ..]->[B1, B2, ..]->C implies cluster [A1, A2, ...] and C have a weak link) */
	vtr::vector_map<ClusterBlockId, std::vector<AtomNetId>> clb_inter_blk_nets(atom_ctx.nlist.blocks().size());

	/* determine bound on cluster size and primitive input size */
	max_cluster_size = 0;
	max_pb_depth = 0;

	for (i = 0; i < device_ctx.num_block_types; i++) {
		if (device_ctx.EMPTY_TYPE == &device_ctx.block_types[i])
			continue;
		cur_cluster_size = get_max_primitives_in_pb_type(
				device_ctx.block_types[i].pb_type);
		cur_pb_depth = get_max_depth_of_pb_type(device_ctx.block_types[i].pb_type);
		if (cur_cluster_size > max_cluster_size) {
			max_cluster_size = cur_cluster_size;
		}
		if (cur_pb_depth > max_pb_depth) {
			max_pb_depth = cur_pb_depth;
		}
	}

    MoleculeStats molecule_stats = build_molecule_stats(molecules, atom_ctx.nlist);

	if (hill_climbing_flag) {
		hill_climbing_inputs_avail = (int *) vtr::calloc(max_cluster_size + 1,
				sizeof(int));
	} else {
		hill_climbing_inputs_avail = nullptr; /* if used, die hard */
	}

	check_clocks(is_clock);
#if 0
	check_for_duplicate_inputs ();
#endif
	alloc_and_init_clustering(molecules, molecule_stats,
			cluster_placement_stats, primitives_list);
    
    auto primitive_candidate_block_types = identify_primitive_candidate_block_types();

	blocks_since_last_analysis = 0;
	early_exit = false;
	num_blocks_hill_added = 0;

	VTR_ASSERT(max_cluster_size < MAX_SHORT);
	/* Limit maximum number of elements for each cluster */

	if (timing_driven) {

        /*
         * Initialize the timing analyzer
         */
        clustering_delay_calc = std::make_shared<PreClusterDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, inter_cluster_net_delay, expected_lowest_cost_pb_gnode);
        timing_info = make_setup_timing_info(clustering_delay_calc);

        //Calculate the initial timing
        timing_info->update();

        if(isEchoFileEnabled(E_ECHO_PRE_PACKING_TIMING_GRAPH)) {
            auto& timing_ctx = g_vpr_ctx.timing();
            tatum::write_echo(getEchoFileName(E_ECHO_PRE_PACKING_TIMING_GRAPH), 
                              *timing_ctx.graph, *timing_ctx.constraints, *clustering_delay_calc, timing_info->analyzer());
        }


#ifdef ENABLE_CLASSIC_VPR_STA
        t_slack* slacks = alloc_and_load_pre_packing_timing_graph(inter_cluster_net_delay, timing_inf, expected_lowest_cost_pb_gnode);
        do_timing_analysis(slacks, timing_inf, true, true);
        std::string fname = std::string("classic.") + getEchoFileName(E_ECHO_PRE_PACKING_TIMING_GRAPH);
        print_timing_graph(fname.c_str());

        auto cpds = timing_info->critical_paths();
        auto critical_path = timing_info->least_slack_critical_path();

        float cpd_diff_ns = std::abs(get_critical_path_delay() - 1e9*critical_path.delay());
        if(cpd_diff_ns > 0.01) {
            print_classic_cpds();
            print_tatum_cpds(timing_info->critical_paths());
            compare_tatum_classic_constraints();

            vpr_throw(VPR_ERROR_TIMING, __FILE__, __LINE__, "Classic VPR and Tatum critical paths do not match (%g and %g respectively)", get_critical_path_delay(), 1e9*critical_path.delay());
        }

        free_timing_graph(slacks);
#endif

		for (auto blk_id : atom_ctx.nlist.blocks()) {
			critindexarray.push_back(blk_id);
			seed_blend_index_array.push_back(blk_id);
		}

        //Calculate criticality of each block
        for(AtomBlockId blk : atom_ctx.nlist.blocks()) {
            for(AtomPinId in_pin : atom_ctx.nlist.block_input_pins(blk)) {
                //Max criticality over incoming nets
                float crit = timing_info->setup_pin_criticality(in_pin);
                block_criticality[blk] = std::max(block_criticality[blk], crit);
            }
        }

        for(auto blk_id : atom_ctx.nlist.blocks()) {
			/* Score seed gain of each block as a weighted sum of timing criticality, 
             * number of tightly coupled blocks connected to it, and number of external inputs */
			float seed_blend_fac = 0.5;
			float max_blend_gain = 0;

            auto molecule_rng = molecules.atom_molecules.equal_range(blk_id);
            for(const auto& kv : vtr::make_range(molecule_rng.first, molecule_rng.second)) {
                PackMoleculeId molecule_id = kv.second;
			
				int inputs_of_molecule = molecule_stats.num_ext_inputs(molecule_id);
				int blocks_of_molecule = molecule_stats.num_blocks(molecule_id);
				int max_molecule_inputs = molecule_stats.max_num_blocks();

                VTR_ASSERT(max_molecule_inputs > 0);

				float blend_gain = 0;
                blend_gain += seed_blend_fac * block_criticality[blk_id];
                blend_gain += (1-seed_blend_fac) * (inputs_of_molecule / max_molecule_inputs);
                blend_gain *= (1 + 0.2 * (blocks_of_molecule - 1));
				if(blend_gain > max_blend_gain) {
					max_blend_gain = blend_gain;
				}
            }
			seed_blend_gain[blk_id] = max_blend_gain;
			
		}

        //Sort in decreasing order (i.e. most critical at index 0)
        std::sort(critindexarray.begin(), critindexarray.end(),
            [](const AtomBlockId lhs, const AtomBlockId rhs) {
                return block_criticality[lhs] > block_criticality[rhs];        
            }
        );

        //Sort in decreasing order (i.e. highest gain at index 0)
        std::sort(seed_blend_index_array.begin(), seed_blend_index_array.end(),
            [](const AtomBlockId lhs, const AtomBlockId rhs) {
                return seed_blend_gain[lhs] > seed_blend_gain[rhs];        
            }
        );
		
		if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_CLUSTERING_BLOCK_CRITICALITIES)) {
			print_block_criticalities(getEchoFileName(E_ECHO_CLUSTERING_BLOCK_CRITICALITIES));
		}

		if (cluster_seed_type == VPACK_BLEND) {
			istart = get_highest_gain_seed_molecule(&seedindex, molecules, molecule_stats, true);
		} else if (cluster_seed_type == VPACK_TIMING) {
			istart = get_highest_gain_seed_molecule(&seedindex, molecules, molecule_stats, false);
		} else {/*max input seed*/
			istart = get_seed_logical_molecule_with_most_ext_inputs(molecule_stats);
		}

	} else /*cluster seed is max input (since there is no timing information)*/ {
		istart = get_seed_logical_molecule_with_most_ext_inputs(molecule_stats);
	}

	/****************************************************************
	* Clustering
	*****************************************************************/

	while (istart) {
		is_cluster_legal = false;
		savedseedindex = seedindex;
		for (detailed_routing_stage = (int)E_DETAILED_ROUTE_AT_END_ONLY; !is_cluster_legal && detailed_routing_stage != (int)E_DETAILED_ROUTE_END; detailed_routing_stage++) {
			ClusterBlockId clb_index(num_clb);
			
			/* start a new cluster and reset all stats */
            vtr::printf("Starting new cluster with molecule %zu\n", size_t(istart));
			start_new_cluster(cluster_placement_stats, primitives_list,
					molecules, istart, molecule_stats, clb_index, 
					num_used_type_instances,
					target_device_utilization,
					num_models, max_cluster_size,
					arch, device_layout_name,
					lb_type_rr_graphs,
					lb_type_rr_graph_infos,
                    &router_data, 
					detailed_routing_stage, &cluster_ctx.clb_nlist,
                    primitive_candidate_block_types,
                    debug_clustering,
                    enable_pin_feasibility_filter);
			vtr::printf_info("Complex block %d: %s, type: %s ", 
					num_clb, cluster_ctx.clb_nlist.block_name(clb_index).c_str(), cluster_ctx.clb_nlist.block_type(clb_index)->name);
            if (debug_clustering) {
                vtr::printf("\n");
            } else {
                vtr::printf("."); //Progress dot for seed-block
            }
			fflush(stdout);
			update_cluster_stats(molecules, istart, clb_index,
                    is_clock, //Set of clock nets
                    is_clock, //Set of global nets (currently all clocks)
                    global_clocks, alpha,
					beta, timing_driven, connection_driven, 
                    *timing_info);
			num_clb++;

			if (timing_driven && !early_exit) {
				blocks_since_last_analysis++;
				/*it doesn't make sense to do a timing analysis here since there*
				 *is only one atom block clustered it would not change anything      */
			}
            t_cluster_placement_stats& cur_cluster_placement_stats = cluster_placement_stats[cluster_ctx.clb_nlist.block_type(clb_index)->index];
			num_unrelated_clustering_attempts = 0;
			next_molecule = get_molecule_for_cluster(
					cluster_ctx.clb_nlist.block_pb(clb_index),
                    molecules,
                    molecule_stats,
                    allow_unrelated_clustering,
					&num_unrelated_clustering_attempts,
					cur_cluster_placement_stats,
					clb_inter_blk_nets,
					clb_index);
			prev_molecule = istart;
			while (next_molecule && prev_molecule != next_molecule) {
                vtr::printf("Trying to pack new molecule %zu\n", size_t(next_molecule));
				block_pack_status = try_pack_molecule(
						cur_cluster_placement_stats, 
                        molecules,
                        next_molecule,
                        molecule_stats,
						primitives_list, cluster_ctx.clb_nlist.block_pb(clb_index), num_models,
						max_cluster_size, clb_index, detailed_routing_stage, router_data,
                        debug_clustering,
                        enable_pin_feasibility_filter);
				prev_molecule = next_molecule;

                auto blk_id = molecules.pack_molecules[next_molecule].root_block_atom();
                VTR_ASSERT(blk_id);

                std::string blk_name = atom_ctx.nlist.block_name(blk_id);
                const t_model* blk_model = atom_ctx.nlist.block_model(blk_id);

				if (block_pack_status != BLK_PASSED) {
					if (next_molecule) {
						if (block_pack_status == BLK_FAILED_ROUTE) {
                            if (debug_clustering) {
                                vtr::printf_direct("\tNO_ROUTE: %s type %s/n", 
                                        blk_name.c_str(), 
                                        blk_model->name);
                                fflush(stdout);
                            }
						} else {
                            if (debug_clustering) {
                                vtr::printf_direct("\tFAILED_CHECK: %s type %s check %d\n", 
                                        blk_name.c_str(), 
                                        blk_model->name, 
                                        block_pack_status);
                                fflush(stdout);
                            }
						}
					}

					next_molecule = get_molecule_for_cluster(
							cluster_ctx.clb_nlist.block_pb(clb_index),
                            molecules,
                            molecule_stats,
                            allow_unrelated_clustering,
							&num_unrelated_clustering_attempts,
							cur_cluster_placement_stats,
							clb_inter_blk_nets,
							clb_index);
					continue;
				} else {
					/* Continue packing by filling smallest cluster */
                    if (debug_clustering) {
                        vtr::printf_direct("\tPASSED: %s type %s\n", 
                                blk_name.c_str(), 
                                blk_model->name);
                    } else {
                        vtr::printf(".");
                    }
					fflush(stdout);
				}
				update_cluster_stats(molecules, next_molecule, clb_index,
                        is_clock, //Set of all clocks
                        is_clock, //Set of all global signals (currently clocks)
						global_clocks, alpha, beta, timing_driven,
						connection_driven, 
                        *timing_info);
				num_unrelated_clustering_attempts = 0;

				if (timing_driven && !early_exit) {
					blocks_since_last_analysis++; /* historically, timing slacks were recomputed after X number of blocks were packed, but this doesn't significantly alter results so I (jluu) did not port the code */
				}
				next_molecule = get_molecule_for_cluster(
						cluster_ctx.clb_nlist.block_pb(clb_index),
                        molecules,
                        molecule_stats,
                        allow_unrelated_clustering,
						&num_unrelated_clustering_attempts,
						cur_cluster_placement_stats,
						clb_inter_blk_nets,
						clb_index);
			}

            if (!debug_clustering) {
                vtr::printf("\n");
            }

			if (detailed_routing_stage == (int)E_DETAILED_ROUTE_AT_END_ONLY) {
				is_cluster_legal = try_intra_lb_route(router_data, debug_clustering);
				if (is_cluster_legal == true) {
                    if (debug_clustering) {
                        vtr::printf_info("Passed route at end.\n");
                    }
				} else {
					vtr::printf_info("Failed route at end, repack cluster trying detailed routing at each stage.\n");
				}
			} else {
				is_cluster_legal = true;
			}
			if (is_cluster_legal == true) {
				intra_lb_routing.push_back(router_data->saved_lb_nets);
				VTR_ASSERT((int)intra_lb_routing.size() == num_clb);
				router_data->saved_lb_nets = nullptr;
				if (timing_driven) {
					if (num_blocks_hill_added > 0 && !early_exit) {
						blocks_since_last_analysis += num_blocks_hill_added;
					}
					if (cluster_seed_type == VPACK_BLEND) {
						istart = get_highest_gain_seed_molecule(&seedindex, molecules, molecule_stats, true);
					} else if (cluster_seed_type == VPACK_TIMING) {
						istart = get_highest_gain_seed_molecule(&seedindex, molecules, molecule_stats, false);
					} else { /*max input seed*/
						istart = get_seed_logical_molecule_with_most_ext_inputs(molecule_stats);
					}
				} else {
					/*cluster seed is max input (since there is no timing information)*/
					istart = get_seed_logical_molecule_with_most_ext_inputs(molecule_stats);
                }
				
				/* store info that will be used later in packing from pb_stats and free the rest */
				t_pb_stats *pb_stats = cluster_ctx.clb_nlist.block_pb(clb_index)->pb_stats;
				for(const AtomNetId mnet_id : pb_stats->marked_nets) {
					int external_terminals = atom_ctx.nlist.net_pins(mnet_id).size() - pb_stats->num_pins_of_net_in_pb[mnet_id];
					/* Check if external terminals of net is within the fanout limit and that there exists external terminals */
					if(external_terminals < AAPACK_MAX_TRANSITIVE_FANOUT_EXPLORE && external_terminals > 0) {
						clb_inter_blk_nets[clb_index].push_back(mnet_id);
					}
				}
				free_pb_stats_recursive(cluster_ctx.clb_nlist.block_pb(clb_index));
			} else {
				/* Free up data structures and requeue used molecules */
				num_used_type_instances[cluster_ctx.clb_nlist.block_type(clb_index)]--;
                revalid_molecules(cluster_ctx.clb_nlist.block_pb(clb_index), molecules, molecule_stats);
				cluster_ctx.clb_nlist.remove_block(clb_index);
				cluster_ctx.clb_nlist.compress();
				num_clb--;
				seedindex = savedseedindex;
			}
			free_router_data(router_data);
			router_data = nullptr;
		}
	}

	/****************************************************************
	* Free Data Structures 
	*****************************************************************/
	VTR_ASSERT(num_clb == (int)cluster_ctx.clb_nlist.blocks().size());
	check_clustering();

	output_clustering(intra_lb_routing, global_clocks, is_clock, arch->architecture_id, out_fname, false);

	VTR_ASSERT(cluster_ctx.clb_nlist.blocks().size() == intra_lb_routing.size());
	for (auto blk_id : cluster_ctx.clb_nlist.blocks())
		free_intra_lb_nets(intra_lb_routing[blk_id]);

	intra_lb_routing.clear();

	if (hill_climbing_flag)
		free(hill_climbing_inputs_avail);

	for (auto blk_id : cluster_ctx.clb_nlist.blocks())
		cluster_ctx.clb_nlist.remove_block(blk_id);

	cluster_ctx.clb_nlist = ClusteredNetlist();

	free(unclustered_list_head);
	free(memory_pool);

	if (timing_driven) {
		block_criticality.clear();
		critindexarray.clear();
		seed_blend_gain.clear();
		seed_blend_index_array.clear();
	}
}

/*****************************************/
static void check_clocks(const std::unordered_set<AtomNetId>& is_clock) {

	/* Checks that nets used as clock inputs to latches are never also used *
	 * as VPACK_LUT inputs.  It's electrically questionable, and more importantly *
	 * would break the clustering code.                                     */
    auto& atom_ctx = g_vpr_ctx.atom();

    for(auto blk_id : atom_ctx.nlist.blocks()) {
		if (atom_ctx.nlist.block_type(blk_id) != AtomBlockType::OUTPAD) {
            for(auto pin_id : atom_ctx.nlist.block_input_pins(blk_id)) {
                auto net_id = atom_ctx.nlist.pin_net(pin_id);
                if (is_clock.count(net_id)) {
                    vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
                            "Error in check_clocks.\n"
                            "Net %s is a clock, but also connects to a logic block input on atom block %s.\n"
                            "This would break the current clustering implementation and is electrically "
                            "questionable, so clustering has been aborted.\n",
                            atom_ctx.nlist.net_name(net_id).c_str(), atom_ctx.nlist.block_name(blk_id).c_str());
                }
            }
		}
	}
}

/* Determine if atom block is in pb */
static bool is_atom_blk_in_pb(const AtomBlockId blk_id, const t_pb *pb) {
    auto& atom_ctx = g_vpr_ctx.atom();

	const t_pb* cur_pb = atom_ctx.lookup.atom_pb(blk_id);
	while (cur_pb) {
		if (cur_pb == pb) {
			return true;
		}
		cur_pb = cur_pb->parent_pb;
	}
	return false;
}

/* Add blk to list of feasible blocks sorted according to gain */
static void add_molecule_to_pb_stats_candidates(const PackMolecules& molecules, const PackMoleculeId molecule_id, const MoleculeStats& molecule_stats,
		std::map<AtomBlockId, float> &gain, t_pb *pb, int max_queue_size) {
	int i, j;

	for (i = 0; i < pb->pb_stats->num_feasible_blocks; i++) {
		if (pb->pb_stats->feasible_blocks[i] == molecule_id) {
			return; /* already in queue, do nothing */
		}
	}

	if (pb->pb_stats->num_feasible_blocks >= max_queue_size - 1) {
		/* maximum size for array, remove smallest gain element and sort */
		if (get_molecule_gain(molecules, molecule_id, molecule_stats, gain) > get_molecule_gain(molecules, pb->pb_stats->feasible_blocks[0], molecule_stats, gain)) {
			/* single loop insertion sort */
			for (j = 0; j < pb->pb_stats->num_feasible_blocks - 1; j++) {
				if (get_molecule_gain(molecules, molecule_id, molecule_stats, gain) <= get_molecule_gain(molecules, pb->pb_stats->feasible_blocks[j + 1], molecule_stats, gain)) {
					pb->pb_stats->feasible_blocks[j] = molecule_id;
					break;
				} else {
					pb->pb_stats->feasible_blocks[j] = pb->pb_stats->feasible_blocks[j + 1];
				}
			}
			if (j == pb->pb_stats->num_feasible_blocks - 1) {
				pb->pb_stats->feasible_blocks[j] = molecule_id;
			}
		}
	} else {
		/* Expand array and single loop insertion sort */
		for (j = pb->pb_stats->num_feasible_blocks - 1; j >= 0; j--) {
			if (get_molecule_gain(molecules, pb->pb_stats->feasible_blocks[j], molecule_stats, gain) > get_molecule_gain(molecules, molecule_id, molecule_stats, gain)) {
				pb->pb_stats->feasible_blocks[j + 1] = pb->pb_stats->feasible_blocks[j];
			} else {
				pb->pb_stats->feasible_blocks[j + 1] = molecule_id;
				break;
			}
		}
		if (j < 0) {
			pb->pb_stats->feasible_blocks[0] = molecule_id;
		}
		pb->pb_stats->num_feasible_blocks++;
	}
}

/*****************************************/
static void alloc_and_init_clustering(const PackMolecules& molecules,
        const MoleculeStats& molecule_stats,
		std::vector<t_cluster_placement_stats>& cluster_placement_stats,
		vtr::vector<MoleculeBlockId,t_pb_graph_node*>& primitives_list) {

	/* Allocates the main data structures used for clustering and properly *
	 * initializes them.                                                   */

    //Sort into decreasing gain order
    //Sort in decreasing order (i.e. highest gain at index 0)
    auto cmp_gain_descending = [&](const PackMoleculeId lhs, const PackMoleculeId rhs) {
        return molecule_stats.base_gain(lhs) > molecule_stats.base_gain(rhs);
    };
    std::vector<PackMoleculeId> molecule_ids_by_desc_gain = molecules.pack_molecule_ids;
    std::sort(molecule_ids_by_desc_gain.begin(), molecule_ids_by_desc_gain.end(), cmp_gain_descending);

	/* alloc and load list of molecules to pack */
	unclustered_list_head_size = molecule_stats.max_num_ext_inputs() + 1;
	unclustered_list_head = (t_molecule_link *) vtr::calloc(unclustered_list_head_size , sizeof(t_molecule_link));

	for (int i = 0; i <= unclustered_list_head_size; i++) {
		unclustered_list_head[i].next = nullptr;
	}

	memory_pool = (t_molecule_link *) vtr::malloc(molecules.pack_molecules.size() * sizeof(t_molecule_link));
	t_molecule_link* next_ptr = memory_pool;

	for (PackMoleculeId molecule_id : molecule_ids_by_desc_gain) {
		int ext_inps = molecule_stats.num_ext_inputs(molecule_id);
		next_ptr->molecule_id = molecule_id;
		next_ptr->next = unclustered_list_head[ext_inps].next;
		unclustered_list_head[ext_inps].next = next_ptr;
		next_ptr++;
	}

	/* load net info */
    auto& atom_ctx = g_vpr_ctx.atom();
    for(AtomNetId net : atom_ctx.nlist.nets()) {
        AtomPinId driver_pin = atom_ctx.nlist.net_driver(net);
        if (!driver_pin) {
            vtr::printf_warning(__FILE__, __LINE__, "Net '%s' has no driver\n", atom_ctx.nlist.net_name(net).c_str());
            continue;
        }
        AtomBlockId driver_block = atom_ctx.nlist.pin_block(driver_pin);

        for(AtomPinId sink_pin : atom_ctx.nlist.net_sinks(net)) {
            AtomBlockId sink_block = atom_ctx.nlist.pin_block(sink_pin);

            if(driver_block == sink_block) {
                net_output_feeds_driving_block_input[net]++;
            }

        }
    }

	/* alloc and load cluster placement info */
	cluster_placement_stats = alloc_and_load_cluster_placement_stats();

	/* alloc array that will store primitives that a molecule gets placed to, 
	 primitive_list is referenced by index, for example a atom block in index 2 of a molecule matches to a primitive in index 2 in primitive_list
	 this array must be the size of the biggest molecule
	 */
	primitives_list.assign(molecule_stats.max_num_blocks(), nullptr);
}

/*****************************************/
static void free_pb_stats_recursive(t_pb *pb) {

	int i, j;
	/* Releases all the memory used by clustering data structures.   */
	if (pb) {
		if (pb->pb_graph_node != nullptr) {
			if (pb->pb_graph_node->pb_type->num_modes != 0) {
				for (i = 0; i < pb->pb_graph_node->pb_type->modes[pb->mode].num_pb_type_children; i++) {
					for (j = 0; j < pb->pb_graph_node->pb_type->modes[pb->mode].pb_type_children[i].num_pb; j++) {
						if (pb->child_pbs && pb->child_pbs[i]) {
							free_pb_stats_recursive(&pb->child_pbs[i][j]);
						}
					}
				}
			}
		}
		free_pb_stats(pb);
	}
}

static bool primitive_feasible(const AtomBlockId blk_id, t_pb *cur_pb) {
	const t_pb_type *cur_pb_type = cur_pb->pb_graph_node->pb_type;

	VTR_ASSERT(cur_pb_type->num_modes == 0); /* primitive */

    auto& atom_ctx = g_vpr_ctx.atom();
    AtomBlockId cur_pb_blk_id = atom_ctx.lookup.pb_atom(cur_pb);
	if (cur_pb_blk_id && cur_pb_blk_id != blk_id) {

		/* This pb already has a different logical block */
		return false;
	}

	if (cur_pb_type->class_type == MEMORY_CLASS) {
		/* Memory class has additional feasibility requirements:
         *   - all siblings must share all nets, including open nets, with the exception of data nets */

		/* find sibling if one exists */
        AtomBlockId sibling_memory_blk_id = find_memory_sibling(cur_pb);

		if (sibling_memory_blk_id) {
            //There is a sibling, see if the current block is feasible with it
            bool sibling_feasible = primitive_memory_sibling_feasible(blk_id, cur_pb_type, sibling_memory_blk_id);
            if(!sibling_feasible) {
                return false;
            }
		}
	}

    //Generic feasibility check
	return primitive_type_feasible(blk_id, cur_pb_type);
}

static bool primitive_memory_sibling_feasible(const AtomBlockId blk_id, const t_pb_type *cur_pb_type, const AtomBlockId sibling_blk_id) {
    /* Check that the two atom blocks blk_id and sibling_blk_id (which should both be memory slices)
     * are feasible, in the sence that they have precicely the same net connections (with the
     * exception of nets in data port classes).
     *
     * Note that this routine does not check pin feasibility against the cur_pb_type; so
     * primitive_type_feasible() should also be called on blk_id before concluding it is feasible.
     */
    auto& atom_ctx = g_vpr_ctx.atom();
    VTR_ASSERT(cur_pb_type->class_type == MEMORY_CLASS);

    //First, identify the 'data' ports by looking at the cur_pb_type
    std::unordered_set<t_model_ports*> data_ports;
    for (int iport = 0; iport < cur_pb_type->num_ports; ++iport) {
        const char* port_class = cur_pb_type->ports[iport].port_class;
        if(port_class && strstr(port_class, "data") == port_class) {
            //The port_class starts with "data", so it is a data port

            //Record the port
            data_ports.insert(cur_pb_type->ports[iport].model_port);
        }
    }

    //Now verify that all nets (except those connected to data ports) are equivalent
    //between blk_id and sibling_blk_id

    //Since the atom netlist stores only in-use ports, we iterate over the model to ensure
    //all ports are compared
    const t_model* model = cur_pb_type->model;
    for(t_model_ports* port : {model->inputs, model->outputs}) {
        for(; port; port = port->next) {
            if(data_ports.count(port)) {
                //Don't check data ports
                continue;
            }

            //Note: VPR doesn't support multi-driven nets, so all outputs
            //should be data ports, otherwise the siblings will both be
            //driving the output net

            //Get the ports from each primitive
            auto blk_port_id = atom_ctx.nlist.find_atom_port(blk_id, port);
            auto sib_port_id = atom_ctx.nlist.find_atom_port(sibling_blk_id, port);

            //Check that all nets (including unconnected nets) match
            for(int ipin = 0; ipin < port->size; ++ipin) {
                //The nets are initialized as invalid (i.e. disconnected)
                AtomNetId blk_net_id;
                AtomNetId sib_net_id;

                //We can get the actual net provided the port exists
                //
                //Note that if the port did not exist, the net is left
                //as invalid/disconneced
                if(blk_port_id) {
                    blk_net_id = atom_ctx.nlist.port_net(blk_port_id, ipin);    
                }
                if(sib_port_id) {
                    sib_net_id = atom_ctx.nlist.port_net(sib_port_id, ipin);    
                }

                //The sibling and block must have the same (possibly disconnected)
                //net on this pin
                if(blk_net_id != sib_net_id) {
                    //Nets do not match, not feasible
                    return false;
                }
            }
        }
    }

	return true;
}

/*****************************************/
static PackMoleculeId get_molecule_by_num_ext_inputs(
        const PackMolecules& molecules,
        const MoleculeStats& molecule_stats,
		const int ext_inps, const enum e_removal_policy remove_flag,
		t_cluster_placement_stats& cluster_placement_stats) {

	/* This routine returns an atom block which has not been clustered, has  *
	 * no connection to the current cluster, satisfies the cluster     *
	 * clock constraints, is a valid subblock inside the cluster, does not exceed the cluster subblock units available,
	 and has ext_inps external inputs.  If        *
	 * there is no such atom block it returns ClusterBlockId::INVALID().  Remove_flag      *
	 * controls whether or not blocks that have already been clustered *
	 * are removed from the unclustered_list data structures.  NB:     *
	 * to get a atom block regardless of clock constraints just set clocks_ *
	 * avail > 0.                                                      */

	t_molecule_link* prev_ptr = &unclustered_list_head[ext_inps];
	t_molecule_link* ptr = unclustered_list_head[ext_inps].next;
	while (ptr != nullptr) {
		/* TODO: Get better candidate atom block in future, eg. return most timing critical or some other smarter metric */
        PackMoleculeId molecule_id = ptr->molecule_id;
		if (molecule_stats.valid(molecule_id)) {
            const PackMolecule& molecule = molecules.pack_molecules[molecule_id];

            if (exists_free_primitives_for_molecule(molecule, cluster_placement_stats)) {
				return molecule_id;
			}
			prev_ptr = ptr;

		} else if (remove_flag == REMOVE_CLUSTERED) {
			VTR_ASSERT(0); /* this doesn't work right now with 2 the pass packing for each complex block */
			prev_ptr->next = ptr->next;
		}

		ptr = ptr->next;
	}

	return PackMoleculeId::INVALID();
}

/*****************************************/
static PackMoleculeId get_free_molecule_with_most_ext_inputs_for_cluster(
        const PackMolecules& molecules,
        const MoleculeStats& molecule_stats,
		t_pb *cur_pb,
		t_cluster_placement_stats& cluster_placement_stats) {

	/* This routine is used to find new blocks for clustering when there are no feasible       *
	 * blocks with any attraction to the current cluster (i.e. it finds       *
	 * blocks which are unconnected from the current cluster).  It returns    *
	 * the atom block with the largest number of used inputs that satisfies the    *
	 * clocking and number of inputs constraints.  If no suitable atom block is    *
	 * found, the routine returns ClusterBlockId::INVALID().                                 
	 * TODO: Analyze if this function is useful in more detail, also, should probably not include clock in input count
	 */

	int ext_inps;
	int i, j;

	int inputs_avail = 0;

	for (i = 0; i < cur_pb->pb_graph_node->num_input_pin_class; i++) {
		for (j = 0; j < cur_pb->pb_graph_node->input_pin_class_size[i]; j++) {
			if (cur_pb->pb_stats->input_pins_used[i][j])
				inputs_avail++;
		}
	}


	if (inputs_avail >= unclustered_list_head_size) {
		inputs_avail = unclustered_list_head_size - 1;
	}

    PackMoleculeId molecule;
	for (ext_inps = inputs_avail; ext_inps >= 0; ext_inps--) {
		molecule = get_molecule_by_num_ext_inputs(molecules, molecule_stats,
				ext_inps, LEAVE_CLUSTERED, cluster_placement_stats);
		if (molecule) {
			break;
		}
	}
	return molecule;
}

/*****************************************/
static PackMoleculeId get_seed_logical_molecule_with_most_ext_inputs(const MoleculeStats& molecule_stats) {

	/* This routine is used to find the first seed atom block for the clustering.  It returns    *
	 * the atom block with the largest number of used inputs that satisfies the    *
	 * clocking and number of inputs constraints.  If no suitable atom block is    *
	 * found, the routine returns ClusterBlockId::INVALID().                                 */

	for (int ext_inps = molecule_stats.max_num_ext_inputs(); ext_inps >= 0; ext_inps--) {
		t_molecule_link* ptr = unclustered_list_head[ext_inps].next;

		while (ptr != nullptr) {
			if (molecule_stats.valid(ptr->molecule_id)) {
				return ptr->molecule_id;
			}
			ptr = ptr->next;
		}
	}
	return PackMoleculeId::INVALID();
}

/*****************************************/

/*****************************************/
static void alloc_and_load_pb_stats(t_pb *pb) {

	/* Call this routine when starting to fill up a new cluster.  It resets *
	 * the gain vector, etc.                                                */

	int i;

	pb->pb_stats = new t_pb_stats;

	/* If statement below is for speed.  If nets are reasonably low-fanout,  *
	 * only a relatively small number of blocks will be marked, and updating *
	 * only those atom block structures will be fastest.  If almost all blocks    *
	 * have been touched it should be faster to just run through them all    *
	 * in order (less addressing and better cache locality).                 */
	pb->pb_stats->input_pins_used = std::vector<std::vector<AtomNetId>>(pb->pb_graph_node->num_input_pin_class);
	pb->pb_stats->output_pins_used = std::vector<std::vector<AtomNetId>>(pb->pb_graph_node->num_output_pin_class);
	pb->pb_stats->lookahead_input_pins_used = std::vector<std::vector<AtomNetId>>(pb->pb_graph_node->num_input_pin_class);
	pb->pb_stats->lookahead_output_pins_used = std::vector<std::vector<AtomNetId>>(pb->pb_graph_node->num_output_pin_class);
	pb->pb_stats->feasible_blocks = std::vector<PackMoleculeId>(AAPACK_MAX_FEASIBLE_BLOCK_ARRAY_SIZE);
	pb->pb_stats->num_feasible_blocks = NOT_VALID;

	pb->pb_stats->tie_break_high_fanout_net = AtomNetId::INVALID();
	for (i = 0; i < pb->pb_graph_node->num_input_pin_class; i++) {
		pb->pb_stats->input_pins_used[i] = std::vector<AtomNetId>(pb->pb_graph_node->input_pin_class_size[i]);
	}

	for (i = 0; i < pb->pb_graph_node->num_output_pin_class; i++) {
		pb->pb_stats->output_pins_used[i] = std::vector<AtomNetId>(pb->pb_graph_node->output_pin_class_size[i]);
	}

	pb->pb_stats->gain.clear();
	pb->pb_stats->timinggain.clear();
	pb->pb_stats->connectiongain.clear();
	pb->pb_stats->sharinggain.clear();
	pb->pb_stats->hillgain.clear();

	pb->pb_stats->num_pins_of_net_in_pb.clear();

	pb->pb_stats->num_child_blocks_in_pb = 0;

	pb->pb_stats->explore_transitive_fanout = true;
	pb->pb_stats->transitive_fanout_candidates = nullptr;
}
/*****************************************/

/**
 * Try pack molecule into current cluster
 */
static enum e_block_pack_status try_pack_molecule(
		t_cluster_placement_stats& cluster_placement_stats,
        const PackMolecules& molecules,
        const PackMoleculeId molecule_id,
        MoleculeStats& molecule_stats,
		vtr::vector<MoleculeBlockId,t_pb_graph_node*>&  primitives_list,
		t_pb * pb, const int max_models, const int max_cluster_size,
		const ClusterBlockId clb_index, const int detailed_routing_stage, t_lb_router_data *router_data,
        bool debug_clustering,
        bool enable_pin_feasibility_filter) {
	enum e_block_pack_status block_pack_status;
	t_pb *parent;
	t_pb *cur_pb;
	bool is_root_of_chain;
	t_pb_graph_pin *chain_root_pin;
    auto& atom_ctx = g_vpr_ctx.atom();

	parent = nullptr;
	
	block_pack_status = BLK_STATUS_UNDEFINED;

    std::vector<AtomBlockId> placed_atoms;

    auto& molecule = molecules.pack_molecules[molecule_id];

	while (block_pack_status != BLK_PASSED) {
		if (get_next_primitive_list(cluster_placement_stats, molecules, molecule_id, molecule_stats, primitives_list)) {
			block_pack_status = BLK_PASSED;
			
            for (auto molecule_blk : molecule.blocks()) {
                AtomBlockId atom_blk = molecule.block_atom(molecule_blk);
                VTR_ASSERT(atom_blk);

                //TODO: remove these?
                chain_root_pin = nullptr;
                is_root_of_chain = false;

                block_pack_status = try_place_atom_block_rec(
                        primitives_list[molecule_blk],
                        atom_blk, pb, &parent,
                        max_models, max_cluster_size, clb_index,
                        cluster_placement_stats, is_root_of_chain, chain_root_pin, router_data);

                if (block_pack_status != BLK_PASSED) break;

                placed_atoms.push_back(atom_blk);
			}

			if (enable_pin_feasibility_filter && block_pack_status == BLK_PASSED) {
				/* Check if pin usage is feasible for the current packing assigment */
				reset_lookahead_pins_used(pb);
				try_update_lookahead_pins_used(pb);
				if (!check_lookahead_pins_used(pb)) {
					block_pack_status = BLK_FAILED_FEASIBLE;
				}
			}

			if (block_pack_status == BLK_PASSED) {
				/* Try to route if heuristic is to route for every atom
					Skip routing if heuristic is to route at the end of packing complex block
				*/							
				if (detailed_routing_stage == (int)E_DETAILED_ROUTE_FOR_EACH_ATOM && try_intra_lb_route(router_data, debug_clustering) == false) {
					/* Cannot pack */
					block_pack_status = BLK_FAILED_ROUTE;
				} else {
					/* Pack successful, commit 
					 TODO: SW Engineering note - may want to update cluster stats here too instead of doing it outside
					 */
					VTR_ASSERT(block_pack_status == BLK_PASSED);
					if (molecule.blocks().size() > 2) {
						/* Chained molecules often take up lots of area and are important, 
                         * if a chain is packed in, want to rename logic block to match chain name */
                        AtomBlockId chain_root_blk_id = molecule.root_block_atom();
						cur_pb = atom_ctx.lookup.atom_pb(chain_root_blk_id)->parent_pb;
						while(cur_pb != nullptr) {
						    free(cur_pb->name);
						    cur_pb->name = vtr::strdup(atom_ctx.nlist.block_name(chain_root_blk_id).c_str());
						    cur_pb = cur_pb->parent_pb;
                        }
					}
                    for (auto molecule_blk : molecule.blocks()) {
                        AtomBlockId atom_blk = molecule.block_atom(molecule_blk);

                        /* invalidate all molecules that share atom block with current molecule */
                        auto rng = molecules.atom_molecules.equal_range(atom_blk);
                        for(const auto& kv : vtr::make_range(rng.first, rng.second)) {
                            PackMoleculeId related_molecule_id = kv.second;
                            vtr::printf("Invalidating molecule %zu for atom '%s' from molecule %zu\n", related_molecule_id, atom_ctx.nlist.block_name(atom_blk).c_str(), molecule_id);
                            molecule_stats.set_valid(related_molecule_id, false);
                        }

                        commit_primitive(cluster_placement_stats, primitives_list[molecule_blk]);
					}
				}
			}
			if (block_pack_status != BLK_PASSED) {
				for (AtomBlockId atom_blk : placed_atoms) {
                    remove_atom_from_target(router_data, atom_blk);
                    revert_place_atom_block(
                            atom_blk,
                            router_data,
                            molecules,
                            molecule_stats);
				}
			}
		} else {
			block_pack_status = BLK_FAILED_FEASIBLE;
			break; /* no more candidate primitives available, this molecule will not pack, return fail */
		}
	}
	return block_pack_status;
}

/**
 * Try place atom block into current primitive location
 */

static enum e_block_pack_status try_place_atom_block_rec(
		const t_pb_graph_node *pb_graph_node, const AtomBlockId blk_id,
		t_pb *cb, t_pb **parent, const int max_models,
		const int max_cluster_size, const ClusterBlockId clb_index,
		const t_cluster_placement_stats& cluster_placement_stats,
		const bool is_root_of_chain, const t_pb_graph_pin *chain_root_pin, t_lb_router_data *router_data) {
	int i, j;
	bool is_primitive;
	enum e_block_pack_status block_pack_status;

	t_pb *my_parent;
	t_pb *pb, *parent_pb;
	const t_pb_type *pb_type;

    auto& atom_ctx = g_vpr_ctx.mutable_atom();

	my_parent = nullptr;

	block_pack_status = BLK_PASSED;

	/* Discover parent */
	if (pb_graph_node->parent_pb_graph_node != cb->pb_graph_node) {
		block_pack_status = try_place_atom_block_rec(
				pb_graph_node->parent_pb_graph_node, blk_id, cb,
				&my_parent, max_models, max_cluster_size, clb_index,
				cluster_placement_stats, is_root_of_chain, chain_root_pin, router_data);
		parent_pb = my_parent;
	} else {
		parent_pb = cb;
        vtr::printf("Placing atom primitive block '%s' in cluster '%s' of type '%s'\n",
                    atom_ctx.nlist.block_name(blk_id).c_str(), cb->name, cb->pb_graph_node->pb_type->name);
	}

	/* Create siblings if siblings are not allocated */
	if (parent_pb->child_pbs == nullptr) {
        atom_ctx.lookup.set_atom_pb(AtomBlockId::INVALID(), parent_pb);

		VTR_ASSERT(parent_pb->name == nullptr);
		parent_pb->name = vtr::strdup(atom_ctx.nlist.block_name(blk_id).c_str());
		parent_pb->mode = pb_graph_node->pb_type->parent_mode->index;
		set_reset_pb_modes(router_data, parent_pb, true);
        const t_mode* mode = &parent_pb->pb_graph_node->pb_type->modes[parent_pb->mode];
        parent_pb->child_pbs = new t_pb*[mode->num_pb_type_children];

		for (i = 0; i < mode->num_pb_type_children; i++) {
            parent_pb->child_pbs[i] = new t_pb[mode->pb_type_children[i].num_pb];

			for (j = 0; j < mode->pb_type_children[i].num_pb; j++) {
				parent_pb->child_pbs[i][j].parent_pb = parent_pb;

                atom_ctx.lookup.set_atom_pb(AtomBlockId::INVALID(), &parent_pb->child_pbs[i][j]);

				parent_pb->child_pbs[i][j].pb_graph_node =
						&(parent_pb->pb_graph_node->child_pb_graph_nodes[parent_pb->mode][i][j]);
			}
		}
	} else {
		VTR_ASSERT(parent_pb->mode == pb_graph_node->pb_type->parent_mode->index);
	}

    const t_mode* mode = &parent_pb->pb_graph_node->pb_type->modes[parent_pb->mode];
	for (i = 0; i < mode->num_pb_type_children; i++) {
		if (pb_graph_node->pb_type == &mode->pb_type_children[i]) {
			break;
		}
	}
	VTR_ASSERT(i < mode->num_pb_type_children);
	pb = &parent_pb->child_pbs[i][pb_graph_node->placement_index];
	*parent = pb; /* this pb is parent of it's child that called this function */
	VTR_ASSERT(pb->pb_graph_node == pb_graph_node);
	if (pb->pb_stats == nullptr) {
		alloc_and_load_pb_stats(pb);
	}
	pb_type = pb_graph_node->pb_type;

	is_primitive = (pb_type->num_modes == 0);

	if (is_primitive) {
		VTR_ASSERT(!atom_ctx.lookup.pb_atom(pb)
                    && atom_ctx.lookup.atom_pb(blk_id) == nullptr 
                    && atom_ctx.lookup.atom_clb(blk_id) == ClusterBlockId::INVALID());
		/* try pack to location */
        VTR_ASSERT(pb->name == nullptr);
		pb->name = vtr::strdup(atom_ctx.nlist.block_name(blk_id).c_str());

        //Update the atom netlist mappings
        atom_ctx.lookup.set_atom_clb(blk_id, clb_index);
        atom_ctx.lookup.set_atom_pb(blk_id, pb);

		add_atom_as_target(router_data, blk_id);
		if (!primitive_feasible(blk_id, pb)) {
			/* failed location feasibility check, revert pack */
			block_pack_status = BLK_FAILED_FEASIBLE;
		}

		if (block_pack_status == BLK_PASSED && is_root_of_chain == true) {
			/* is carry chain, must check if this carry chain spans multiple logic blocks or not */
            t_model_ports *root_port = chain_root_pin->port->model_port;
            auto port_id = atom_ctx.nlist.find_atom_port(blk_id, root_port);
            if(port_id) {
                auto chain_net_id = atom_ctx.nlist.port_net(port_id, chain_root_pin->pin_number);

                if(chain_net_id) {
                    /* this carry chain spans multiple logic blocks, must match up correctly with previous chain for this to route */
                    if(pb_graph_node != chain_root_pin->parent_node) {
                        /* this location does not match with the dedicated chain input from outside logic block, therefore not feasible */
                        block_pack_status = BLK_FAILED_FEASIBLE;
                    }
                }
            }
		}
	}

    if (block_pack_status != BLK_PASSED) {
        free(pb->name);
        pb->name = nullptr;
    }

	return block_pack_status;
}

/* Revert trial atom block iblock and free up memory space accordingly 
 */
static void revert_place_atom_block(const AtomBlockId blk_id, t_lb_router_data *router_data,
    const PackMolecules& molecules,
    MoleculeStats& molecule_stats) {

    auto& atom_ctx = g_vpr_ctx.mutable_atom();

    //We cast away const here since we may free the pb, and it is
    //being removed from the active mapping.
    //
    //In general most code works fine accessing cosnt t_pb*,
    //which is why we store them as such in atom_ctx.lookup
    t_pb* pb = const_cast<t_pb*>(atom_ctx.lookup.atom_pb(blk_id)); 

    //Update the atom netlist mapping
    atom_ctx.lookup.set_atom_clb(blk_id, ClusterBlockId::INVALID());
    atom_ctx.lookup.set_atom_pb(blk_id, nullptr);

	if (pb != nullptr) {
		/* When freeing molecules, the current block might already have been freed by a prior revert 
		 When this happens, no need to do anything beyond basic book keeping at the atom block
		 */

		t_pb* next = pb->parent_pb;
        revalid_molecules(pb, molecules, molecule_stats);
		free_pb(pb);
		pb = next;

		while (pb != nullptr) {
			/* If this is pb is created only for the purposes of holding new molecule, remove it
			 Must check if cluster is already freed (which can be the case)
			 */
			next = pb->parent_pb;
			
			if (pb->child_pbs != nullptr && pb->pb_stats != nullptr
					&& pb->pb_stats->num_child_blocks_in_pb == 0) {
				set_reset_pb_modes(router_data, pb, false);
				if (next != nullptr) {
					/* If the code gets here, then that means that placing the initial seed molecule 
                     * failed, don't free the actual complex block itself as the seed needs to find 
                     * another placement */
                    revalid_molecules(pb, molecules, molecule_stats);
					free_pb(pb);
				}
			}
			pb = next;
		}
	}
}

static void update_connection_gain_values(const AtomNetId net_id, const AtomBlockId clustered_blk_id,
		t_pb *cur_pb,
		enum e_net_relation_to_clustered_block net_relation_to_clustered_block) {
	/*This function is called when the connectiongain values on the net net_id*
	 *require updating.   */

	int num_internal_connections, num_open_connections, num_stuck_connections;

	num_internal_connections = num_open_connections = num_stuck_connections = 0;

    auto& atom_ctx = g_vpr_ctx.atom();
	ClusterBlockId clb_index = atom_ctx.lookup.atom_clb(clustered_blk_id);

	/* may wish to speed things up by ignoring clock nets since they are high fanout */

    for(auto pin_id : atom_ctx.nlist.net_pins(net_id)) {
        auto blk_id = atom_ctx.nlist.pin_block(pin_id);
		if (atom_ctx.lookup.atom_clb(blk_id) == clb_index
				&& is_atom_blk_in_pb(blk_id, atom_ctx.lookup.atom_pb(clustered_blk_id))) {
			num_internal_connections++;
		} else if (atom_ctx.lookup.atom_clb(blk_id) == ClusterBlockId::INVALID()) {
			num_open_connections++;
		} else {
			num_stuck_connections++;
		}
	}

	if (net_relation_to_clustered_block == OUTPUT) {
        for(auto pin_id : atom_ctx.nlist.net_sinks(net_id)) {
            auto blk_id = atom_ctx.nlist.pin_block(pin_id);
            VTR_ASSERT(blk_id);

			if (atom_ctx.lookup.atom_clb(blk_id) == ClusterBlockId::INVALID()) {
				/* TODO: Gain function accurate only if net has one connection to block, 
                 * TODO: Should we handle case where net has multi-connection to block? 
                 *       Gain computation is only off by a bit in this case */
				if(cur_pb->pb_stats->connectiongain.count(blk_id) == 0) {
					cur_pb->pb_stats->connectiongain[blk_id] = 0;
				}
				
				if (num_internal_connections > 1) {
					cur_pb->pb_stats->connectiongain[blk_id] -= 1 / (float) (num_open_connections + 1.5 * num_stuck_connections + 1 + 0.1);
				}
				cur_pb->pb_stats->connectiongain[blk_id] += 1 / (float) (num_open_connections + 1.5 * num_stuck_connections + 0.1);
			}
		}
	}

	if (net_relation_to_clustered_block == INPUT) {
		/*Calculate the connectiongain for the atom block which is driving *
		 *the atom net that is an input to an atom block in the cluster */

        auto driver_pin_id = atom_ctx.nlist.net_driver(net_id);
        auto blk_id = atom_ctx.nlist.pin_block(driver_pin_id);

		if (atom_ctx.lookup.atom_clb(blk_id) == ClusterBlockId::INVALID()) {
			if(cur_pb->pb_stats->connectiongain.count(blk_id) == 0) {
				cur_pb->pb_stats->connectiongain[blk_id] = 0;
			}
			if (num_internal_connections > 1) {
				cur_pb->pb_stats->connectiongain[blk_id] -= 1 / (float) (num_open_connections + 1.5 * num_stuck_connections + 0.1 + 1);
			}
			cur_pb->pb_stats->connectiongain[blk_id] += 1 / (float) (num_open_connections + 1.5 * num_stuck_connections + 0.1);
		}
	}
}
/*****************************************/
static void update_timing_gain_values(const AtomNetId net_id,
		t_pb *cur_pb,
		enum e_net_relation_to_clustered_block net_relation_to_clustered_block, 
        const SetupTimingInfo& timing_info,
        const std::unordered_set<AtomNetId>& is_global) {

	/*This function is called when the timing_gain values on the atom net*
	 *net_id requires updating.   */
	float timinggain;

    auto& atom_ctx = g_vpr_ctx.atom();

	/* Check if this atom net lists its driving atom block twice.  If so, avoid  *
	 * double counting this atom block by skipping the first (driving) pin. */
    auto pins = atom_ctx.nlist.net_pins(net_id);
	if (net_output_feeds_driving_block_input[net_id] != 0)
        pins = atom_ctx.nlist.net_sinks(net_id);

	if (net_relation_to_clustered_block == OUTPUT
			&& !is_global.count(net_id)) {
        for(auto pin_id : pins) {
            auto blk_id = atom_ctx.nlist.pin_block(pin_id);
			if (atom_ctx.lookup.atom_clb(blk_id) == ClusterBlockId::INVALID()) {

                timinggain = timing_info.setup_pin_criticality(pin_id);

				if(cur_pb->pb_stats->timinggain.count(blk_id) == 0) {
					cur_pb->pb_stats->timinggain[blk_id] = 0;
				}
				if (timinggain > cur_pb->pb_stats->timinggain[blk_id])
					cur_pb->pb_stats->timinggain[blk_id] = timinggain;
			}
		}
	}

	if (net_relation_to_clustered_block == INPUT
			&& !is_global.count(net_id)) {
		/*Calculate the timing gain for the atom block which is driving *
		 *the atom net that is an input to a atom block in the cluster */
        auto driver_pin = atom_ctx.nlist.net_driver(net_id);
        auto new_blk_id = atom_ctx.nlist.pin_block(driver_pin);

		if (atom_ctx.lookup.atom_clb(new_blk_id) == ClusterBlockId::INVALID()) {
			for (auto pin_id : atom_ctx.nlist.net_sinks(net_id)) {

                timinggain = timing_info.setup_pin_criticality(pin_id);

				if(cur_pb->pb_stats->timinggain.count(new_blk_id) == 0) {
					cur_pb->pb_stats->timinggain[new_blk_id] = 0;
				}
				if (timinggain > cur_pb->pb_stats->timinggain[new_blk_id])
					cur_pb->pb_stats->timinggain[new_blk_id] = timinggain;

			}
		}
	}
}

/*****************************************/
static void mark_and_update_partial_gain(const AtomNetId net_id, enum e_gain_update gain_flag,
		const AtomBlockId clustered_blk_id, 
		bool timing_driven,
		bool connection_driven,
		enum e_net_relation_to_clustered_block net_relation_to_clustered_block, 
        const SetupTimingInfo& timing_info,
        const std::unordered_set<AtomNetId>& is_global) {

	/* Updates the marked data structures, and if gain_flag is GAIN,  *
	 * the gain when an atom block is added to a cluster.  The        *
	 * sharinggain is the number of inputs that a atom block shares with   *
	 * blocks that are already in the cluster. Hillgain is the        *
	 * reduction in number of pins-required by adding a atom block to the  *
	 * cluster. The timinggain is the criticality of the most critical*
	 * atom net between this atom block and an atom block in the cluster.             */

    auto& atom_ctx = g_vpr_ctx.atom();
	t_pb* cur_pb = atom_ctx.lookup.atom_pb(clustered_blk_id)->parent_pb;

	if (atom_ctx.nlist.net_sinks(net_id).size() > AAPACK_MAX_NET_SINKS_IGNORE) {
		/* Optimization: It can be too runtime costly for marking all sinks for 
         * a high fanout-net that probably has no hope of ever getting packed, 
         * thus ignore those high fanout nets */
		if(!is_global.count(net_id)) {
			/* If no low/medium fanout nets, we may need to consider 
             * high fan-out nets for packing, so select one and store it */ 
			while(cur_pb->parent_pb != nullptr) {
				cur_pb = cur_pb->parent_pb;
			}
			AtomNetId stored_net = cur_pb->pb_stats->tie_break_high_fanout_net;
			if(!stored_net || atom_ctx.nlist.net_sinks(net_id).size() < atom_ctx.nlist.net_sinks(stored_net).size()) {
				cur_pb->pb_stats->tie_break_high_fanout_net = net_id;
			}
		}
		return;
	}

	while (cur_pb) {
		/* Mark atom net as being visited, if necessary. */

		if (cur_pb->pb_stats->num_pins_of_net_in_pb.count(net_id) == 0) {
			cur_pb->pb_stats->marked_nets.push_back(net_id);
		}

		/* Update gains of affected blocks. */

		if (gain_flag == GAIN) {

			/* Check if this net is connected to it's driver block multiple times (i.e. as both an output and input)
             * If so, avoid double counting by skipping the first (driving) pin. */

            auto pins = atom_ctx.nlist.net_pins(net_id);
			if (net_output_feeds_driving_block_input[net_id] != 0)
                //We implicitly assume here that net_output_feeds_driver_block_input[net_id] is 2
                //(i.e. the net loops back to the block only once)
			    pins = atom_ctx.nlist.net_sinks(net_id);	

			if (cur_pb->pb_stats->num_pins_of_net_in_pb.count(net_id) == 0) {
                for(auto pin_id : pins) {
                    auto blk_id = atom_ctx.nlist.pin_block(pin_id);
					if (atom_ctx.lookup.atom_clb(blk_id) == ClusterBlockId::INVALID()) {

						if (cur_pb->pb_stats->sharinggain.count(blk_id) == 0) {
							cur_pb->pb_stats->marked_blocks.push_back(blk_id);
							cur_pb->pb_stats->sharinggain[blk_id] = 1;
							cur_pb->pb_stats->hillgain[blk_id] = 1 - num_ext_inputs_atom_block(blk_id);
						} else {
							cur_pb->pb_stats->sharinggain[blk_id]++;
							cur_pb->pb_stats->hillgain[blk_id]++;
						}
					}
				}
			}

			if (connection_driven) {
				update_connection_gain_values(net_id, clustered_blk_id, cur_pb,
						net_relation_to_clustered_block);
			}

			if (timing_driven) {
				update_timing_gain_values(net_id, cur_pb,
						net_relation_to_clustered_block, 
                        timing_info,
                        is_global);
			}
		}
		if(cur_pb->pb_stats->num_pins_of_net_in_pb.count(net_id) == 0) {
			cur_pb->pb_stats->num_pins_of_net_in_pb[net_id] = 0;
		}
		cur_pb->pb_stats->num_pins_of_net_in_pb[net_id]++;
		cur_pb = cur_pb->parent_pb;
	}
}

/*****************************************/
static void update_total_gain(float alpha, float beta, bool timing_driven,
		bool connection_driven, t_pb *pb) {

	/*Updates the total  gain array to reflect the desired tradeoff between*
	 *input sharing (sharinggain) and path_length minimization (timinggain)*/
    auto& atom_ctx = g_vpr_ctx.atom();
	t_pb * cur_pb = pb;
	while (cur_pb) {

		for (AtomBlockId blk_id : cur_pb->pb_stats->marked_blocks) {

			if(cur_pb->pb_stats->connectiongain.count(blk_id) == 0) {
				cur_pb->pb_stats->connectiongain[blk_id] = 0;
			}
			if(cur_pb->pb_stats->sharinggain.count(blk_id) == 0) {
				cur_pb->pb_stats->sharinggain[blk_id] = 0;
			}

			/* Todo: This was used to explore different normalization options, can 
             * be made more efficient once we decide on which one to use*/
			int num_used_input_pins = atom_ctx.nlist.block_input_pins(blk_id).size();
			int num_used_output_pins = atom_ctx.nlist.block_output_pins(blk_id).size();
			/* end todo */

			/* Calculate area-only cost function */
            int num_used_pins = num_used_input_pins + num_used_output_pins;
            VTR_ASSERT(num_used_pins > 0);
			if (connection_driven) {
				/*try to absorb as many connections as possible*/
				cur_pb->pb_stats->gain[blk_id] = ((1 - beta)
						* (float) cur_pb->pb_stats->sharinggain[blk_id]
						+ beta * (float) cur_pb->pb_stats->connectiongain[blk_id])
						/ (num_used_pins);
			} else {
				cur_pb->pb_stats->gain[blk_id] =
						((float) cur_pb->pb_stats->sharinggain[blk_id])
								/ (num_used_pins);

			}

			/* Add in timing driven cost into cost function */
			if (timing_driven) {
				cur_pb->pb_stats->gain[blk_id] = alpha
						* cur_pb->pb_stats->timinggain[blk_id]
						+ (1.0 - alpha) * (float) cur_pb->pb_stats->gain[blk_id];
			}
		}
		cur_pb = cur_pb->parent_pb;
	}
}

/*****************************************/
static void update_cluster_stats(
        const PackMolecules& molecules,
        const PackMoleculeId molecule_id,
		const ClusterBlockId clb_index,
        const std::unordered_set<AtomNetId>& is_clock, 
        const std::unordered_set<AtomNetId>& is_global, 
        const bool global_clocks,
		const float alpha, const float beta, const bool timing_driven,
		const bool connection_driven, 
        const SetupTimingInfo& timing_info) {

	/* Updates cluster stats such as gain, used pins, and clock structures.  */
	t_pb *cur_pb, *cb;

	/* TODO: what a scary comment from Vaughn, we'll have to watch out for this causing problems */

	/* Output can be open so the check is necessary.  I don't change    *
	 * the gain for clock outputs when clocks are globally distributed  *
	 * because I assume there is no real need to pack similarly clocked *
	 * FFs together then.  Note that by updating the gain when the      *
	 * clock driver is placed in a cluster implies that the output of   *
	 * LUTs can be connected to clock inputs internally.  Probably not  *
	 * true, but it doesn't make much difference, since it will still   *
	 * make local routing of this clock very short, and none of my      *
	 * benchmarks actually generate local clocks (all come from pads).  */

    auto& atom_ctx = g_vpr_ctx.mutable_atom();
	cb = nullptr;

    auto& molecule = molecules.pack_molecules[molecule_id];
	for (auto molecule_blk : molecule.blocks()) {
        AtomBlockId blk_id = molecule.block_atom(molecule_blk);
		VTR_ASSERT(blk_id);

        //Update atom netlist mapping
        atom_ctx.lookup.set_atom_clb(blk_id, clb_index);

		cur_pb = atom_ctx.lookup.atom_pb(blk_id)->parent_pb;
		while (cur_pb) {
			/* reset list of feasible blocks */
			cur_pb->pb_stats->num_feasible_blocks = NOT_VALID;
			cur_pb->pb_stats->num_child_blocks_in_pb++;
			if (cur_pb->parent_pb == nullptr) {
				cb = cur_pb;
			}
			cur_pb = cur_pb->parent_pb;
		}

        /* Outputs first */
        for(auto pin_id : atom_ctx.nlist.block_output_pins(blk_id)) {
            auto net_id = atom_ctx.nlist.pin_net(pin_id);
            if (!is_clock.count(net_id) || !global_clocks) {
                mark_and_update_partial_gain(net_id, GAIN, blk_id,
                        timing_driven,
                        connection_driven, OUTPUT, 
                        timing_info,
                        is_global);
            } else {
                mark_and_update_partial_gain(net_id, NO_GAIN, blk_id,
                        timing_driven,
                        connection_driven, OUTPUT, 
                        timing_info,
                        is_global);
            }
        }

        /* Next Inputs */
        for(auto pin_id : atom_ctx.nlist.block_input_pins(blk_id)) {
            auto net_id = atom_ctx.nlist.pin_net(pin_id);
            mark_and_update_partial_gain(net_id, GAIN, blk_id,
                    timing_driven, connection_driven,
                    INPUT, 
                    timing_info,
                    is_global);
        }

        /* Finally Clocks */
		/* Note:  The code below ONLY WORKS when nets that go to clock inputs *
		 * NEVER go to the input of a VPACK_COMB.  It doesn't really make electrical *
		 * sense for that to happen, and I check this in the check_clocks     *
		 * function.  Don't disable that sanity check.                        */
        //TODO: lift above restriction (does happen on some circuits)
        for(auto pin_id : atom_ctx.nlist.block_clock_pins(blk_id)) {
            auto net_id = atom_ctx.nlist.pin_net(pin_id);
            if (global_clocks) {
                mark_and_update_partial_gain(net_id, NO_GAIN, blk_id,
                        timing_driven, connection_driven, INPUT, 
                        timing_info,
                        is_global);
            } else {
                mark_and_update_partial_gain(net_id, GAIN, blk_id,
                        timing_driven, connection_driven, INPUT, 
                        timing_info,
                        is_global);
            }
        }

		update_total_gain(alpha, beta, timing_driven, connection_driven,
				atom_ctx.lookup.atom_pb(blk_id)->parent_pb);

		commit_lookahead_pins_used(cb);
	}
}

static void start_new_cluster(
		std::vector<t_cluster_placement_stats>& cluster_placement_stats,
		vtr::vector<MoleculeBlockId,t_pb_graph_node*>&  primitives_list,
        const PackMolecules& molecules,
		const PackMoleculeId molecule_id,
        MoleculeStats& molecule_stats,
        ClusterBlockId clb_index,
		std::map<t_type_ptr, size_t>& num_used_type_instances,
        const float target_device_utilization,
		const int num_models, const int max_cluster_size,
		const t_arch* arch,
		std::string device_layout_name,
		const vector<t_lb_type_rr_graph>& lb_type_rr_graphs, 
		const std::vector<t_lb_type_rr_graph_info>& lb_type_rr_graph_infos,
		t_lb_router_data **router_data, 
		const int detailed_routing_stage,
		ClusteredNetlist *clb_nlist,
        const std::map<const t_model*,std::vector<t_type_ptr>>& primitive_candidate_block_types,
        bool debug_clustering,
        bool enable_pin_feasibility_filter) {
	/* Given a starting seed block, start_new_cluster determines the next cluster type to use 
	 It expands the FPGA if it cannot find a legal cluster for the atom block
	 */

    auto& atom_ctx = g_vpr_ctx.atom();
    auto& device_ctx = g_vpr_ctx.mutable_device();

	/* Allocate a dummy initial cluster and load a atom block as a seed and check if it is legal */
    auto& molecule = molecules.pack_molecules[molecule_id];
    AtomBlockId root_atom = molecule.root_block_atom();
	const std::string& root_atom_name = atom_ctx.nlist.block_name(root_atom);
	const t_model* root_model = atom_ctx.nlist.block_model(root_atom);

    auto itr = primitive_candidate_block_types.find(root_model);
    VTR_ASSERT(itr != primitive_candidate_block_types.end());
    std::vector<t_type_ptr> candidate_types = itr->second;

    //We sort the candidate types in ascending order by their current utilization.
    //This means that the packer will prefer to use types with lower utilization.
    //This is a naive approach to try balancing utilization when multiple types can 
    //support the same primitive(s).
    std::sort(candidate_types.begin(), candidate_types.end(),
        [&](t_type_ptr lhs, t_type_ptr rhs) {
            float lhs_util = float(num_used_type_instances[lhs]) / device_ctx.grid.num_instances(lhs);
            float rhs_util = float(num_used_type_instances[rhs]) / device_ctx.grid.num_instances(rhs);
            
            //Lower util first
            return lhs_util < rhs_util;
        }
    );

    //Try packing into each candidate type
	bool success = false;
    for (size_t i = 0; i < candidate_types.size(); i++) {
        auto type = candidate_types[i];

        t_pb* pb = new t_pb;
        pb->pb_graph_node = type->pb_graph_head;
        alloc_and_load_pb_stats(pb);
        pb->parent_pb = nullptr;

        *router_data = alloc_and_load_router_data(lb_type_rr_graphs[type->index], lb_type_rr_graph_infos[type->index], type);

        //Try packing into each mode
        for (int j = 0; j < type->pb_graph_head->pb_type->num_modes && !success; j++) {
            pb->mode = j;
                
            reset_cluster_placement_stats(cluster_placement_stats[type->index]);
            set_mode_cluster_placement_stats(pb->pb_graph_node, j);

            auto pack_result = try_pack_molecule(cluster_placement_stats[type->index],
                                            molecules,
                                            molecule_id,
                                            molecule_stats,
                                            primitives_list, pb,
                                            num_models, max_cluster_size, clb_index,
                                            detailed_routing_stage, *router_data,
                                            debug_clustering,
                                            enable_pin_feasibility_filter);

            success = (pack_result == BLK_PASSED);
        }

        if (success) {
            //Once clustering succeeds, add it to the clb netlist
            if (pb->name != nullptr) {
                free(pb->name);
            }
            pb->name = vtr::strdup(root_atom_name.c_str());
            clb_index = clb_nlist->create_block(root_atom_name.c_str(), pb, type);
            break;
        } else {
            //Free failed clustering and try again
            free_router_data(*router_data);
            free_pb(pb);
            delete pb;
            *router_data = nullptr;
        }
    }

    if (!success) {
        //Explored all candidates
        if (molecule.blocks().size() > 1) {
            vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
                    "Can not find any logic block that can implement molecule %s.\n",
                    root_atom_name.c_str());
        } else {
            vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
                    "Can not find any logic block that can implement molecule.\n"
                    "\tAtom %s (%s)\n",
                    root_atom_name.c_str(), root_model->name);
        }
    }

    VTR_ASSERT(success);

	//Successfully create cluster
	num_used_type_instances[clb_nlist->block_type(clb_index)]++;

	/* Expand FPGA size if needed */
	if (num_used_type_instances[clb_nlist->block_type(clb_index)] > device_ctx.grid.num_instances(clb_nlist->block_type(clb_index))) {
		device_ctx.grid = create_device_grid(device_layout_name, arch->grid_layouts, num_used_type_instances, target_device_utilization);
		vtr::printf_info("Not enough resources expand FPGA size to (%d x %d)\n",
			device_ctx.grid.width(), device_ctx.grid.height());
	}
}

/*
Get candidate molecule to pack into currently open cluster
Molecule selection priority:
	1. Find unpacked molecule based on criticality and strong connectedness (connected by low fanout nets) with current cluster
	2. Find unpacked molecule based on weak connectedness (connected by high fanout nets) with current cluster
	3. Find unpacked molecule based on transitive connections (eg. 2 hops away) with current cluster
*/
static PackMoleculeId get_highest_gain_molecule(
		t_pb *cur_pb,
        const PackMolecules& molecules,
        const MoleculeStats& molecule_stats,
		const enum e_gain_type gain_mode,
		t_cluster_placement_stats& cluster_placement_stats,
		vtr::vector_map<ClusterBlockId,std::vector<AtomNetId>> &clb_inter_blk_nets,
		const ClusterBlockId cluster_index) {

	/* This routine populates a list of feasible blocks outside the cluster then returns the best one for the list    *
	 * not currently in a cluster and satisfies the feasibility     *
	 * function passed in as is_feasible.  If there are no feasible *
	 * blocks it returns ClusterBlockId::INVALID().                                */

	int j, index, count;

    auto& atom_ctx = g_vpr_ctx.atom();

	if (gain_mode == HILL_CLIMBING) {
		vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
				"Hill climbing not supported yet, error out.\n");
	}

	// 1. Find unpacked molecule based on criticality and strong connectedness (connected by low fanout nets) with current cluster
	if (cur_pb->pb_stats->num_feasible_blocks == NOT_VALID) {
		cur_pb->pb_stats->num_feasible_blocks = 0;
		cur_pb->pb_stats->explore_transitive_fanout = true; /* If no legal molecules found, enable exploration of molecules two hops away */
		for (AtomBlockId blk_id : cur_pb->pb_stats->marked_blocks) {
			if (atom_ctx.lookup.atom_clb(blk_id) == ClusterBlockId::INVALID()) {

                auto rng = molecules.atom_molecules.equal_range(blk_id);
                for(const auto& kv : vtr::make_range(rng.first, rng.second)) {
                    PackMoleculeId molecule_id = kv.second;
					if (molecule_stats.valid(molecule_id)) {
                        auto& molecule = molecules.pack_molecules[molecule_id];

						if (exists_free_primitives_for_molecule(molecule, cluster_placement_stats)) {
							add_molecule_to_pb_stats_candidates(molecules, molecule_id, molecule_stats,
									cur_pb->pb_stats->gain, cur_pb, AAPACK_MAX_FEASIBLE_BLOCK_ARRAY_SIZE);
						}
                    }
                }
			}
		}
	}

	// 2. Find unpacked molecule based on weak connectedness (connected by high fanout nets) with current cluster
	if(cur_pb->pb_stats->num_feasible_blocks == 0 && cur_pb->pb_stats->tie_break_high_fanout_net) {
		/* Because the packer ignores high fanout nets when marking what blocks 
         * to consider, use one of the ignored high fanout net to fill up lightly 
         * related blocks */
		reset_tried_but_unused_cluster_placements(cluster_placement_stats);

		AtomNetId net_id = cur_pb->pb_stats->tie_break_high_fanout_net;

		count = 0;
        for(auto pin_id : atom_ctx.nlist.net_pins(net_id)) {
            if(count >= AAPACK_MAX_HIGH_FANOUT_EXPLORE) {
                break;
            }

            AtomBlockId blk_id = atom_ctx.nlist.pin_block(pin_id);

			if (atom_ctx.lookup.atom_clb(blk_id) == ClusterBlockId::INVALID()) {

                auto rng = molecules.atom_molecules.equal_range(blk_id);
                for(const auto& kv : vtr::make_range(rng.first, rng.second)) {
                    PackMoleculeId molecule_id = kv.second;
					if (molecule_stats.valid(molecule_id)) {
                        auto& molecule = molecules.pack_molecules[molecule_id];

						if (exists_free_primitives_for_molecule(molecule, cluster_placement_stats)) {
							add_molecule_to_pb_stats_candidates(molecules, molecule_id, molecule_stats,
									cur_pb->pb_stats->gain, cur_pb, min(AAPACK_MAX_FEASIBLE_BLOCK_ARRAY_SIZE, AAPACK_MAX_HIGH_FANOUT_EXPLORE));
							count++;
						}
					}
                }
			}
		} 
		cur_pb->pb_stats->tie_break_high_fanout_net = AtomNetId::INVALID(); /* Mark off that this high fanout net has been considered */
	}

	// 3. Find unpacked molecule based on transitive connections (eg. 2 hops away) with current cluster
	if(cur_pb->pb_stats->num_feasible_blocks == 0 && 
	   !cur_pb->pb_stats->tie_break_high_fanout_net &&
	   cur_pb->pb_stats->explore_transitive_fanout == true
	   ) {
		if(cur_pb->pb_stats->transitive_fanout_candidates == nullptr) {
			/* First time finding transitive fanout candidates therefore alloc and load them */
			cur_pb->pb_stats->transitive_fanout_candidates = new vector<PackMoleculeId>;
			load_transitive_fanout_candidates(cluster_index,
                                              molecules,
                                              molecule_stats,
											  cur_pb->pb_stats,
											  clb_inter_blk_nets);

			/* Only consider candidates that pass a very simple legality check */
			for(int i = 0; i < (int) cur_pb->pb_stats->transitive_fanout_candidates->size(); i++) {
				PackMoleculeId molecule_id = (*cur_pb->pb_stats->transitive_fanout_candidates)[i];
				if (molecule_stats.valid(molecule_id)) {
                    auto& molecule = molecules.pack_molecules[molecule_id];

                    if (exists_free_primitives_for_molecule(molecule, cluster_placement_stats)) {
						add_molecule_to_pb_stats_candidates(molecules, molecule_id, molecule_stats,
								cur_pb->pb_stats->gain, cur_pb, min(AAPACK_MAX_FEASIBLE_BLOCK_ARRAY_SIZE,AAPACK_MAX_TRANSITIVE_EXPLORE));
					}
				}
			}
		} else {
			/* Clean up, no more candidates in transitive fanout to consider */
			delete cur_pb->pb_stats->transitive_fanout_candidates;
			cur_pb->pb_stats->transitive_fanout_candidates = nullptr;
			cur_pb->pb_stats->explore_transitive_fanout = false;
		}
	}

	/* Grab highest gain molecule */
	PackMoleculeId molecule_id;
	for (j = 0; j < cur_pb->pb_stats->num_feasible_blocks; j++) {
		if (cur_pb->pb_stats->num_feasible_blocks != 0) {
			cur_pb->pb_stats->num_feasible_blocks--;
			index = cur_pb->pb_stats->num_feasible_blocks;
			molecule_id = cur_pb->pb_stats->feasible_blocks[index];
			VTR_ASSERT(molecule_stats.valid(molecule_id));
			return molecule_id;
		}
	}

	return molecule_id;
}

static bool exists_free_primitives_for_molecule(const PackMolecule& molecule, t_cluster_placement_stats& cluster_placement_stats) {
    auto& atom_ctx = g_vpr_ctx.atom();

    for (auto molecule_blk : molecule.blocks()) {
        AtomBlockId atom_blk = molecule.block_atom(molecule_blk);
        VTR_ASSERT(atom_ctx.lookup.atom_clb(atom_blk) == ClusterBlockId::INVALID());
        if (!exists_free_primitive_for_atom_block(cluster_placement_stats, atom_blk)) { 
            /* TODO: debating whether to check if placement exists for molecule 
             * (more robust) or individual atom blocks (faster) */
            return false;
        }
    }

    return true;
}

/*****************************************/
static PackMoleculeId get_molecule_for_cluster(
		t_pb *cur_pb,
        const PackMolecules& molecules,
        const MoleculeStats& molecule_stats,
		const bool allow_unrelated_clustering,
		int *num_unrelated_clustering_attempts,
		t_cluster_placement_stats& cluster_placement_stats,
		vtr::vector_map<ClusterBlockId,std::vector<AtomNetId>> &clb_inter_blk_nets,
		ClusterBlockId cluster_index) {

	/* Finds the block with the the greatest gain that satisifies the      *
	 * input, clock and capacity constraints of a cluster that are       *
	 * passed in.  If no suitable block is found it returns ClusterBlockId::INVALID().   
	 */

	PackMoleculeId best_molecule;

	/* If cannot pack into primitive, try packing into cluster */

	best_molecule = get_highest_gain_molecule(cur_pb, molecules, molecule_stats,
			NOT_HILL_CLIMBING, cluster_placement_stats, clb_inter_blk_nets, cluster_index);

	/* If no blocks have any gain to the current cluster, the code above *
	 * will not find anything.  However, another atom block with no inputs in *
	 * common with the cluster may still be inserted into the cluster.   */

	if (allow_unrelated_clustering) {
		if (!best_molecule) {
			if (*num_unrelated_clustering_attempts == 0) {
				best_molecule = get_free_molecule_with_most_ext_inputs_for_cluster(
                                    molecules,
                                    molecule_stats,
                                    cur_pb,
                                    cluster_placement_stats);
				(*num_unrelated_clustering_attempts)++;
			}
		} else {
			*num_unrelated_clustering_attempts = 0;
		}
	}

	return best_molecule;
}


/* TODO: Add more error checking! */
static void check_clustering() {
    std::unordered_set<AtomBlockId> atoms_checked;
    auto& atom_ctx = g_vpr_ctx.atom();
	auto& cluster_ctx = g_vpr_ctx.clustering();

    if(cluster_ctx.clb_nlist.blocks().size() == 0) {
        vtr::printf_warning(__FILE__, __LINE__, "Packing produced no clustered blocks");
    }

	/* 
	 * Check that each atom block connects to one physical primitive and that the primitive links up to the parent clb
	 */
    for(auto blk_id : atom_ctx.nlist.blocks()) {
        //Each atom should be part of a pb
        const t_pb* atom_pb = atom_ctx.lookup.atom_pb(blk_id);
        if(!atom_pb) {
			vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
					"Atom block %s is not mapped to a pb\n",
					atom_ctx.nlist.block_name(blk_id).c_str());
        }

        //Check the reverse mapping is consistent
		if (atom_ctx.lookup.pb_atom(atom_pb) != blk_id) {
			vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
					"pb %s does not contain atom block %s but atom block %s maps to pb.\n",
                    atom_pb->name,
					atom_ctx.nlist.block_name(blk_id).c_str(), 
                    atom_ctx.nlist.block_name(blk_id).c_str());
		}

		VTR_ASSERT(atom_ctx.nlist.block_name(blk_id) == atom_pb->name);

        const t_pb* cur_pb = atom_pb;
		while (cur_pb->parent_pb) {
			cur_pb = cur_pb->parent_pb;
			VTR_ASSERT(cur_pb->name);
		}

		ClusterBlockId clb_index = atom_ctx.lookup.atom_clb(blk_id);
        if(clb_index == ClusterBlockId::INVALID()) {
			vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
					"Atom %s is not mapped to a CLB\n",
					atom_ctx.nlist.block_name(blk_id).c_str());
        }

		if (cur_pb != cluster_ctx.clb_nlist.block_pb(clb_index)) {
			vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
					"CLB %s does not match CLB contained by pb %s.\n",
					cur_pb->name, atom_pb->name);
		}
	}

	/* Check that I do not have spurious links in children pbs */
	for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
		check_cluster_atom_blocks(cluster_ctx.clb_nlist.block_pb(blk_id), atoms_checked);
	}

	for (auto blk_id : atom_ctx.nlist.blocks()) {
		if (!atoms_checked.count(blk_id)) {
			vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
					"Atom block %s not found in any cluster.\n",
					atom_ctx.nlist.block_name(blk_id).c_str());
		}
	}
}

/* TODO: May want to check that all atom blocks are actually reached */
static void check_cluster_atom_blocks(t_pb *pb, std::unordered_set<AtomBlockId>& blocks_checked) {
	int i, j;
	const t_pb_type *pb_type;
	bool has_child = false;
    auto& atom_ctx = g_vpr_ctx.atom();

	pb_type = pb->pb_graph_node->pb_type;
	if (pb_type->num_modes == 0) {
		/* primitive */
        auto blk_id = atom_ctx.lookup.pb_atom(pb);
		if (blk_id) {
			if (blocks_checked.count(blk_id)) {
				vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
						"pb %s contains atom block %s but atom block is already contained in another pb.\n",
						pb->name, atom_ctx.nlist.block_name(blk_id).c_str());
			}
			blocks_checked.insert(blk_id);
			if (pb != atom_ctx.lookup.atom_pb(blk_id)) {
				vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
						"pb %s contains atom block %s but atom block does not link to pb.\n",
						pb->name, atom_ctx.nlist.block_name(blk_id).c_str());
			}
		}
	} else {
		/* this is a container pb, all container pbs must contain children */
		for (i = 0; i < pb_type->modes[pb->mode].num_pb_type_children; i++) {
			for (j = 0; j < pb_type->modes[pb->mode].pb_type_children[i].num_pb; j++) {
				if (pb->child_pbs[i] != nullptr) {
					if (pb->child_pbs[i][j].name != nullptr) {
						has_child = true;
						check_cluster_atom_blocks(&pb->child_pbs[i][j], blocks_checked);
					}
				}
			}
		}
		VTR_ASSERT(has_child);
	}
}

static PackMoleculeId get_highest_gain_seed_molecule(int * seedindex, const PackMolecules& molecules, const MoleculeStats& molecule_stats, bool getblend) {
	/* Do_timing_analysis must be called before this, or this function 
	 * will return garbage. Returns molecule with most critical block;
	 * if block belongs to multiple molecules, return the biggest molecule. */

	AtomBlockId blk_id;
    auto& atom_ctx = g_vpr_ctx.atom();

    VTR_ASSERT(seed_blend_index_array.size() == critindexarray.size());

    PackMoleculeId best;
	while (*seedindex < static_cast<int>(seed_blend_index_array.size())) {

		if(getblend == true) {
			blk_id = seed_blend_index_array[(*seedindex)++];
		} else {
			blk_id = critindexarray[(*seedindex)++];
		}

		if (atom_ctx.lookup.atom_clb(blk_id) == ClusterBlockId::INVALID()) {

            auto rng = molecules.atom_molecules.equal_range(blk_id);
            for(const auto& kv : vtr::make_range(rng.first, rng.second)) {
                PackMoleculeId molecule_id = kv.second;
				if (molecule_stats.valid(molecule_id)) {
					if (!best || molecule_stats.base_gain(best) < molecule_stats.base_gain(molecule_id)) {
						best = molecule_id;
					}
				}
            }
			VTR_ASSERT(best);
			return best;
		}
	}

	/*if it makes it to here , there are no more blocks available*/
	return PackMoleculeId::INVALID();
}

/* get gain of packing molecule into current cluster 
     gain is equal to:
        total_block_gain 
        + molecule_base_gain*some_factor 
        - introduced_input_nets_of_unrelated_blocks_pulled_in_by_molecule*some_other_factor
 */
static float get_molecule_gain(const PackMolecules& molecules, const PackMoleculeId molecule_id, const MoleculeStats& molecule_stats, std::map<AtomBlockId, float> &blk_gain) {
	float gain;
	int num_introduced_inputs_of_indirectly_related_block;
    auto& atom_ctx = g_vpr_ctx.atom();

    auto& molecule = molecules.pack_molecules[molecule_id];
	gain = 0;
	num_introduced_inputs_of_indirectly_related_block = 0;
    for (auto molecule_blk_id : molecule.blocks()) {
        AtomBlockId atom_blk = molecule.block_atom(molecule_blk_id);
        VTR_ASSERT(atom_blk);

        if(blk_gain.count(atom_blk) > 0) {
            gain += blk_gain[atom_blk];
        } else {
            /* This block has no connection with current cluster, penalize molecule for having this block 
             */
            for(auto pin_id : atom_ctx.nlist.block_input_pins(atom_blk)) {
                auto net_id = atom_ctx.nlist.pin_net(pin_id);
                VTR_ASSERT(net_id);

                auto driver_pin_id = atom_ctx.nlist.net_driver(net_id);
                VTR_ASSERT(driver_pin_id);

                auto driver_blk_id = atom_ctx.nlist.pin_block(driver_pin_id);

                num_introduced_inputs_of_indirectly_related_block++;
                for (auto molecule_blk_id2 : molecule.blocks()) {
                    AtomBlockId atom_blk2 = molecule.block_atom(molecule_blk_id2);
                    if (driver_blk_id == atom_blk2) {
                        //valid block which is driver (and hence not an input)
                        num_introduced_inputs_of_indirectly_related_block--;
                        break;
                    }
                }
            }
        }
	}

	gain += molecule_stats.base_gain(molecule_id) * 0.0001; /* Use base gain as tie breaker TODO: need to sweep this value and perhaps normalize */
	gain -= num_introduced_inputs_of_indirectly_related_block * (0.001);

	return gain;
}

static int compare_molecule_gain(const void *a, const void *b) {
	float base_gain_a, base_gain_b, diff;
	const t_pack_molecule *molecule_a, *molecule_b;
	molecule_a = (*(const t_pack_molecule * const *) a);
	molecule_b = (*(const t_pack_molecule * const *) b);

	base_gain_a = molecule_a->base_gain;
	base_gain_b = molecule_b->base_gain;
	diff = base_gain_a - base_gain_b;
	if (diff > 0) {
		return 1;
	}
	if (diff < 0) {
		return -1;
	}
	return 0;
}

/* Determine if speculatively packed cur_pb is pin feasible 
 * Runtime is actually not that bad for this.  It's worst case O(k^2) where k is the 
 * number of pb_graph pins.  Can use hash tables or make incremental if becomes an issue.
 */
static void try_update_lookahead_pins_used(t_pb *cur_pb) {
	int i, j;
	const t_pb_type *pb_type = cur_pb->pb_graph_node->pb_type;

	if (pb_type->num_modes > 0 && cur_pb->name != nullptr) {
		if (cur_pb->child_pbs != nullptr) {
			for (i = 0; i < pb_type->modes[cur_pb->mode].num_pb_type_children; i++) {
				if (cur_pb->child_pbs[i] != nullptr) {
					for (j = 0; j < pb_type->modes[cur_pb->mode].pb_type_children[i].num_pb; j++) {
						try_update_lookahead_pins_used(&cur_pb->child_pbs[i][j]);
					}
				}
			}
		}
	} else {

        auto& atom_ctx = g_vpr_ctx.atom();
        AtomBlockId blk_id = atom_ctx.lookup.pb_atom(cur_pb);
		if (pb_type->blif_model != nullptr && blk_id) {
			compute_and_mark_lookahead_pins_used(blk_id);
		}
	}
}

/* Resets nets used at different pin classes for determining pin feasibility */
static void reset_lookahead_pins_used(t_pb *cur_pb) {
	int i, j;
	const t_pb_type *pb_type = cur_pb->pb_graph_node->pb_type;
	if (cur_pb->pb_stats == nullptr) {
		return; /* No pins used, no need to continue */
	}

	if (pb_type->num_modes > 0 && cur_pb->name != nullptr) {
		for (i = 0; i < cur_pb->pb_graph_node->num_input_pin_class; i++) {
			cur_pb->pb_stats->lookahead_input_pins_used[i].clear();
		}

		for (i = 0; i < cur_pb->pb_graph_node->num_output_pin_class; i++) {
			cur_pb->pb_stats->lookahead_output_pins_used[i].clear();			
		}

		if (cur_pb->child_pbs != nullptr) {
			for (i = 0; i < pb_type->modes[cur_pb->mode].num_pb_type_children; i++) {
				if (cur_pb->child_pbs[i] != nullptr) {
					for (j = 0; j < pb_type->modes[cur_pb->mode].pb_type_children[i].num_pb; j++) {
						reset_lookahead_pins_used(&cur_pb->child_pbs[i][j]);
					}
				}
			}
		}
	}
}

/* Determine if pins of speculatively packed pb are legal */
static void compute_and_mark_lookahead_pins_used(const AtomBlockId blk_id) {
    auto& atom_ctx = g_vpr_ctx.atom();

	const t_pb* cur_pb = atom_ctx.lookup.atom_pb(blk_id);
    VTR_ASSERT(cur_pb != nullptr);

	/* Walk through inputs, outputs, and clocks marking pins off of the same class */
    for(auto pin_id : atom_ctx.nlist.block_pins(blk_id)) {
        auto net_id = atom_ctx.nlist.pin_net(pin_id);

        const t_pb_graph_pin* pb_graph_pin = find_pb_graph_pin(atom_ctx.nlist, atom_ctx.lookup, pin_id);
        compute_and_mark_lookahead_pins_used_for_pin(pb_graph_pin, cur_pb, net_id);
    }
}

/* Given a pin and its assigned net, mark all pin classes that are affected */
static void compute_and_mark_lookahead_pins_used_for_pin(const t_pb_graph_pin *pb_graph_pin, const t_pb *primitive_pb, const AtomNetId net_id) {
	int depth;
	int pin_class, output_port;
	t_pb * cur_pb;
	const t_pb_type *pb_type;
	t_port *prim_port;
	t_pb_graph_pin *output_pb_graph_pin;

	bool skip, found;

    auto& atom_ctx = g_vpr_ctx.atom();

	cur_pb = primitive_pb->parent_pb;

	while (cur_pb) {
		depth = cur_pb->pb_graph_node->pb_type->depth;
		pin_class = pb_graph_pin->parent_pin_class[depth];
		VTR_ASSERT(pin_class != OPEN);

        auto driver_blk_id = atom_ctx.nlist.net_driver_block(net_id);

		if (pb_graph_pin->port->type == IN_PORT) {
			/* find location of net driver if exist in clb, NULL otherwise */
            auto driver_pin_id = atom_ctx.nlist.net_driver(net_id);

            auto prim_blk_id = atom_ctx.lookup.pb_atom(primitive_pb);

            const t_pb* driver_pb = atom_ctx.lookup.atom_pb(driver_blk_id);

			output_pb_graph_pin = nullptr;
			if (atom_ctx.lookup.atom_clb(driver_blk_id) == atom_ctx.lookup.atom_clb(prim_blk_id)) {
				pb_type = driver_pb->pb_graph_node->pb_type;
				output_port = 0;
				found = false;
				for (int i = 0; i < pb_type->num_ports && !found; i++) {
					prim_port = &pb_type->ports[i];
					if (prim_port->type == OUT_PORT) {
                        auto driver_port_id = atom_ctx.nlist.pin_port(driver_pin_id);
                        auto driver_model_port = atom_ctx.nlist.port_model(driver_port_id);
						if (pb_type->ports[i].model_port == driver_model_port) {
							found = true;
							break;
						}
						output_port++;
					}
				}
				VTR_ASSERT(found);
				output_pb_graph_pin = &(driver_pb->pb_graph_node->output_pins[output_port][atom_ctx.nlist.pin_port_bit(driver_pin_id)]);
			}

			skip = false;

			/* check if driving pin for input is contained within the currently 
             * investigated cluster, if yes, do nothing since no input needs to be used */
			if (output_pb_graph_pin != nullptr) {
				const t_pb* check_pb = driver_pb;
				while (check_pb != nullptr && check_pb != cur_pb) {
					check_pb = check_pb->parent_pb;
				}
				if (check_pb != nullptr) {
					for (int i = 0; skip == false && i < output_pb_graph_pin->num_connectable_primitive_input_pins[depth]; i++) {
						if (pb_graph_pin == output_pb_graph_pin->list_of_connectable_input_pin_ptrs[depth][i]) {
							skip = true;
						}
					}
				}
			}

			/* Must use input pin */
			if (!skip) {
				/* Check if already in pin class, if yes, skip */
				skip = false;
				int la_inpins_size = (int)cur_pb->pb_stats->lookahead_input_pins_used[pin_class].size();
				for (int i = 0; i < la_inpins_size; i++) {
					if (cur_pb->pb_stats->lookahead_input_pins_used[pin_class][i] == net_id) {
						skip = true;
					}
				}
				if (!skip) {
					/* Net must take up a slot */
					cur_pb->pb_stats->lookahead_input_pins_used[pin_class].push_back(net_id);
				}
			}
		} else {
			VTR_ASSERT(pb_graph_pin->port->type == OUT_PORT);
            /*
             * Determine if this net (which is driven within this cluster) leaves this cluster
             * (and hense uses an output pin).
             */

			bool net_exits_cluster = true;
            int num_net_sinks = static_cast<int>(atom_ctx.nlist.net_sinks(net_id).size());

			if (pb_graph_pin->num_connectable_primitive_input_pins[depth] >= num_net_sinks) {
                //It is possible the net is completely absorbed in the cluster,
                //since this pin could (potentially) drive all the net's sinks

				/* Important: This runtime penalty looks a lot scarier than it really is.  
                 * For high fan-out nets, I at most look at the number of pins within the 
                 * cluster which limits runtime.
                 *
				 * DO NOT REMOVE THIS INITIAL FILTER WITHOUT CAREFUL ANALYSIS ON RUNTIME!!!
				 * 
				 * Key Observation:
				 * For LUT-based designs it is impossible for the average fanout to exceed 
                 * the number of LUT inputs so it's usually around 4-5 (pigeon-hole argument,
                 * if the average fanout is greater than the number of LUT inputs, where do 
                 * the extra connections go?  Therefore, average fanout must be capped to a 
                 * small constant where the constant is equal to the number of LUT inputs).  
                 * The real danger to runtime is when the number of sinks of a net gets doubled
				 */

                //Check if all the net sinks are, in fact, inside this cluster
                bool all_sinks_in_cur_cluster = true;
				ClusterBlockId driver_clb = atom_ctx.lookup.atom_clb(driver_blk_id);
                for(auto pin_id : atom_ctx.nlist.net_sinks(net_id)) {
                    auto sink_blk_id = atom_ctx.nlist.pin_block(pin_id);
					if (atom_ctx.lookup.atom_clb(sink_blk_id) != driver_clb) {
                        all_sinks_in_cur_cluster = false;
						break;
					}
				}

				if (all_sinks_in_cur_cluster) {
                    //All the sinks are part of this cluster, so the net may be fully absorbed.
                    //
                    //Verify this, by counting the number of net sinks reachable from the driver pin.
                    //If the count equals the number of net sinks then the net is fully absorbed and
                    //the net does not exit the cluster
					/* TODO: I should cache the absorbed outputs, once net is absorbed,
                     *       net is forever absorbed, no point in rechecking every time */
					if (net_sinks_reachable_in_cluster(pb_graph_pin, depth, net_id)) {
                        //All the sinks are reachable inside the cluster
						net_exits_cluster = false;
					}
				}
			}

			if (net_exits_cluster) {
				/* This output must exit this cluster */
				cur_pb->pb_stats->lookahead_output_pins_used[pin_class].push_back(net_id);				
			}
		}

		cur_pb = cur_pb->parent_pb;
	}
}

int net_sinks_reachable_in_cluster(const t_pb_graph_pin* driver_pb_gpin, const int depth, const AtomNetId net_id) {
    size_t num_reachable_sinks = 0;
    auto& atom_ctx = g_vpr_ctx.atom();

    //Record the sink pb graph pins we are looking for
    std::unordered_set<const t_pb_graph_pin*> sink_pb_gpins;
    for(const AtomPinId pin_id : atom_ctx.nlist.net_sinks(net_id)) {
        const t_pb_graph_pin* sink_pb_gpin = find_pb_graph_pin(atom_ctx.nlist, atom_ctx.lookup, pin_id);
        VTR_ASSERT(sink_pb_gpin);

        sink_pb_gpins.insert(sink_pb_gpin);
    }

    //Count how many sink pins are reachable
    for(int i_prim_pin = 0; i_prim_pin < driver_pb_gpin->num_connectable_primitive_input_pins[depth]; ++i_prim_pin) {
        const t_pb_graph_pin* reachable_pb_gpin = driver_pb_gpin->list_of_connectable_input_pin_ptrs[depth][i_prim_pin];

        if(sink_pb_gpins.count(reachable_pb_gpin)) {
            ++num_reachable_sinks;
            if(num_reachable_sinks == atom_ctx.nlist.net_sinks(net_id).size()) {
                return true;
            }
        }
    }

    return false;
}

/* Check if the number of available inputs/outputs for a pin class is sufficient for speculatively packed blocks */
static bool check_lookahead_pins_used(t_pb *cur_pb) {
	int i, j;
	const t_pb_type *pb_type = cur_pb->pb_graph_node->pb_type;
	bool success;

	success = true;

	if (pb_type->num_modes > 0 && cur_pb->name != nullptr) {
		for (i = 0; i < cur_pb->pb_graph_node->num_input_pin_class && success; i++) {
			if (cur_pb->pb_stats->lookahead_input_pins_used[i].size() > (unsigned int)cur_pb->pb_graph_node->input_pin_class_size[i]) {
				success = false;
			}
		}

		for (i = 0; i < cur_pb->pb_graph_node->num_output_pin_class && success;
				i++) {
			if (cur_pb->pb_stats->lookahead_output_pins_used[i].size() > (unsigned int)cur_pb->pb_graph_node->output_pin_class_size[i]) {
				success = false;
			}
		}

		if (success && cur_pb->child_pbs != nullptr) {
			for (i = 0; success && i < pb_type->modes[cur_pb->mode].num_pb_type_children; i++) {
				if (cur_pb->child_pbs[i] != nullptr) {
					for (j = 0; success && j < pb_type->modes[cur_pb->mode].pb_type_children[i].num_pb; j++) {
						success = check_lookahead_pins_used(
								&cur_pb->child_pbs[i][j]);
					}
				}
			}
		}
	}
	return success;
}

/* Speculation successful, commit input/output pins used */
static void commit_lookahead_pins_used(t_pb *cur_pb) {
	int i, j;
	int ipin;
	const t_pb_type *pb_type = cur_pb->pb_graph_node->pb_type;

	if (pb_type->num_modes > 0 && cur_pb->name != nullptr) {
		for (i = 0; i < cur_pb->pb_graph_node->num_input_pin_class; i++) {
			ipin = 0;
			VTR_ASSERT(cur_pb->pb_stats->lookahead_input_pins_used[i].size() <= (unsigned int)cur_pb->pb_graph_node->input_pin_class_size[i]);
			for (j = 0; j < (int) cur_pb->pb_stats->lookahead_input_pins_used[i].size(); j++) {
				VTR_ASSERT(cur_pb->pb_stats->lookahead_input_pins_used[i][j]);
				cur_pb->pb_stats->input_pins_used[i][ipin] = cur_pb->pb_stats->lookahead_input_pins_used[i][j];
				ipin++;
			}
		}

		for (i = 0; i < cur_pb->pb_graph_node->num_output_pin_class; i++) {
			ipin = 0;
			VTR_ASSERT(cur_pb->pb_stats->lookahead_output_pins_used[i].size() <= (unsigned int)cur_pb->pb_graph_node->output_pin_class_size[i]);
			for (j = 0; j < (int) cur_pb->pb_stats->lookahead_output_pins_used[i].size(); j++) {
				VTR_ASSERT(cur_pb->pb_stats->lookahead_output_pins_used[i][j]);
				cur_pb->pb_stats->output_pins_used[i][ipin] = cur_pb->pb_stats->lookahead_output_pins_used[i][j];
				ipin++;
			}
		}

		if (cur_pb->child_pbs != nullptr) {
			for (i = 0; i < pb_type->modes[cur_pb->mode].num_pb_type_children;
					i++) {
				if (cur_pb->child_pbs[i] != nullptr) {
					for (j = 0; j < pb_type->modes[cur_pb->mode].pb_type_children[i].num_pb; j++) {
						commit_lookahead_pins_used(&cur_pb->child_pbs[i][j]);
					}
				}
			}
		}
	}
}

/* Score unclustered atoms that are two hops away from current cluster */
static void load_transitive_fanout_candidates(ClusterBlockId clb_index,
                                              const PackMolecules& molecules,
                                              const MoleculeStats& molecule_stats,
											  t_pb_stats *pb_stats,
											  vtr::vector_map<ClusterBlockId,std::vector<AtomNetId>> &clb_inter_blk_nets) {
    auto& atom_ctx = g_vpr_ctx.atom();

    for(const auto net_id : pb_stats->marked_nets) {
		if(atom_ctx.nlist.net_pins(net_id).size() < AAPACK_MAX_TRANSITIVE_FANOUT_EXPLORE + 1) {
            for(const auto pin_id : atom_ctx.nlist.net_pins(net_id)) {
                AtomBlockId atom_blk_id = atom_ctx.nlist.pin_block(pin_id);
				ClusterBlockId tclb = atom_ctx.lookup.atom_clb(atom_blk_id); //The transitive CLB
				if(tclb != clb_index && tclb != ClusterBlockId::INVALID()) {
					/* Explore transitive connections from already packed cluster */
					for(AtomNetId tnet : clb_inter_blk_nets[tclb]) {
                        for(AtomPinId tpin : atom_ctx.nlist.net_pins(tnet)) {
                            auto blk_id = atom_ctx.nlist.pin_block(tpin);
							if(atom_ctx.lookup.atom_clb(blk_id) == ClusterBlockId::INVALID()) {
								/* This transitive atom is not packed, score and add */
								auto &transitive_fanout_candidates = *(pb_stats->transitive_fanout_candidates);

								if(pb_stats->gain.count(blk_id) == 0) {
									pb_stats->gain[blk_id] = 0.001;									
								} else {
									pb_stats->gain[blk_id] += 0.001;									
								}
                                auto rng = molecules.atom_molecules.equal_range(blk_id);
                                for(const auto& kv : vtr::make_range(rng.first, rng.second)) {
                                    PackMoleculeId molecule_id = kv.second;
									if (molecule_stats.valid(molecule_id)) {
										unsigned int imol = 0;

										/* The number of potential molecules is heavily bounded so 
                                         * this O(N) operation should be safe since N is small */
										for(imol = 0; imol < transitive_fanout_candidates.size(); imol++) {
											if(molecule_id == transitive_fanout_candidates[imol]) {
												break;
											}
										}
										if(imol == transitive_fanout_candidates.size()) {
											/* not in candidate list, add to list */
											transitive_fanout_candidates.push_back(molecule_id);
										}
									}
                                }
							}
						}
					}
				}
			}
		}
	}
}

static void print_block_criticalities(const char * fname) {
	/* Prints criticality and critindexarray for each atom block to a file. */
    auto& atom_ctx = g_vpr_ctx.atom();
	
	FILE * fp = vtr::fopen(fname, "w");

    //For prett formatting determine the maximum name length
    int max_name_len = strlen("atom_block_name");
    for(auto blk_id : atom_ctx.nlist.blocks()) {
        max_name_len = std::max(max_name_len, (int) atom_ctx.nlist.block_name(blk_id).size());
    }

    fprintf(fp, "%-*s %s %s %s %s\n", max_name_len, "atom_block_name", "criticality", "critindexarray", "seed_blend_gain", "seed_blend_gain_index");

    for(auto blk_id : atom_ctx.nlist.blocks()) {
        std::string name = atom_ctx.nlist.block_name(blk_id);
		fprintf(fp, "%-*s ", max_name_len, name.c_str());

		fprintf(fp, "%*f ", (int) strlen("criticality"), block_criticality[blk_id]);

        auto crit_idx_iter = std::find(critindexarray.begin(), critindexarray.end(), blk_id);
        VTR_ASSERT(crit_idx_iter != critindexarray.end());
        int crit_idx = crit_idx_iter - critindexarray.begin();
        fprintf(fp, "%*d ", (int) strlen("critindexarray"), crit_idx);

        fprintf(fp, "%*f ", (int) strlen("seed_blend_gain"), seed_blend_gain[blk_id]);
        auto seed_gain_iter = std::find(seed_blend_index_array.begin(), seed_blend_index_array.end(), blk_id);
        int seed_blend_gain_index = seed_gain_iter - seed_blend_index_array.begin();
        fprintf(fp, "%*d", (int) strlen("seed_blend_gain_index"), seed_blend_gain_index);

        fprintf(fp, "\n");
	}
	fclose(fp);
}

static std::map<const t_model*,std::vector<t_type_ptr>> identify_primitive_candidate_block_types() {
    std::map<const t_model*,std::vector<t_type_ptr>> model_candidates;
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& atom_nlist = atom_ctx.nlist; 
    auto& device_ctx = g_vpr_ctx.device();

    std::set<const t_model*> unique_models;
    for(auto blk : atom_nlist.blocks()) {
        auto model = atom_nlist.block_model(blk);
        unique_models.insert(model);
    }

    for (auto model : unique_models) {
        model_candidates[model] = {};

        for (int itype = 0; itype < device_ctx.num_block_types; ++itype ) {
            t_type_ptr type = &device_ctx.block_types[itype];
            if (block_type_contains_blif_model(type, model->name)) {
                model_candidates[model].push_back(type);
            }
        }
    }

    return model_candidates;
}

static void revalid_molecules(const t_pb* pb, const PackMolecules& molecules, MoleculeStats& molecule_stats) {
	const t_pb_type* pb_type = pb->pb_graph_node->pb_type;

	if (pb_type->blif_model == nullptr) {
		int mode = pb->mode;
		for (int i = 0; i < pb_type->modes[mode].num_pb_type_children && pb->child_pbs != nullptr; i++) {
			for (int j = 0; j < pb_type->modes[mode].pb_type_children[i].num_pb	&& pb->child_pbs[i] != nullptr; j++) {
				if (pb->child_pbs[i][j].name != nullptr || pb->child_pbs[i][j].child_pbs != nullptr) {
					revalid_molecules(&pb->child_pbs[i][j], molecules, molecule_stats);
				}
			}
        }
    } else {
        //Primitive
        auto& atom_ctx = g_vpr_ctx.mutable_atom();

        auto blk_id = atom_ctx.lookup.pb_atom(pb);
		if (blk_id) {
            /* If any molecules were marked invalid because of this logic block getting packed, mark them valid */

            //Update atom netlist mapping
            atom_ctx.lookup.set_atom_clb(blk_id, ClusterBlockId::INVALID());
            atom_ctx.lookup.set_atom_pb(blk_id, nullptr);

            auto rng = molecules.atom_molecules.equal_range(blk_id);
            for(const auto& kv : vtr::make_range(rng.first, rng.second)) {
                PackMoleculeId cur_molecule_id = kv.second;
                if (!molecule_stats.valid(cur_molecule_id)) {
                    auto& cur_molecule = molecules.pack_molecules[cur_molecule_id];

                    bool all_atoms_valid = true;
                    for (auto molecule_blk_id : cur_molecule.blocks()) {
                        AtomBlockId atom_blk = cur_molecule.block_atom(molecule_blk_id);
                        if (atom_ctx.lookup.atom_clb(atom_blk) != ClusterBlockId::INVALID()) {
                            all_atoms_valid = false;
                            break;
                        }
                    }

                    /* All atom blocks are open for this molecule, place back in queue */
                    if (all_atoms_valid) {
                        molecule_stats.set_valid(cur_molecule_id, true);
                    }
                }
            }
        }
    }
}


MoleculeStats build_molecule_stats(const PackMolecules& molecules, const AtomNetlist& netlist) {
    MoleculeStats stats;

    for (const PackMolecule& molecule : molecules.pack_molecules) {
        MoleculeStats::Info info;

        info.num_blocks = count_molecule_blocks(molecule);
        info.num_ext_inputs = count_molecule_ext_inputs(molecule, netlist);

        stats.info_.push_back(info);
    }

    VTR_ASSERT(molecules.pack_molecules.size() > 0);

    //Max external inputs
    auto cmp_ext_inputs = [](const MoleculeStats::Info& lhs, const MoleculeStats::Info& rhs) {
        return lhs.num_ext_inputs < rhs.num_ext_inputs;
    };
    stats.max_num_ext_inputs_ = std::max_element(stats.info_.begin(), stats.info_.end(), cmp_ext_inputs)->num_ext_inputs;

    //Max blocks
    auto cmp_num_blocks = [](const MoleculeStats::Info& lhs, const MoleculeStats::Info& rhs) {
        return lhs.num_blocks < rhs.num_blocks;
    };
    stats.max_num_blocks_ = std::max_element(stats.info_.begin(), stats.info_.end(), cmp_num_blocks)->num_blocks;

    return stats;
}

int count_molecule_blocks(const PackMolecule& molecule) {
    int count = 0;

    for (auto blk_id : molecule.blocks()) {
        if (molecule.block_type(blk_id) == PackMolecule::BlockType::INTERNAL) {
            ++count;
        }
    }

    return count;
}

int count_molecule_ext_inputs(const PackMolecule& molecule, const AtomNetlist& netlist) {

    //Count inputs of all internal blocks
    int total_inputs = 0;
    for (auto molecule_blk_id : molecule.blocks()) {
        AtomBlockId atom_blk_id = molecule.block_atom(molecule_blk_id);

        if (molecule.block_type(molecule_blk_id) != PackMolecule::BlockType::INTERNAL) continue;

        total_inputs += netlist.block_input_pins(atom_blk_id).size();
        total_inputs += netlist.block_clock_pins(atom_blk_id).size();
    }

    //Subtract away connections contained within the molecule
    int ext_inputs = total_inputs;
    for (auto molecule_edge_id : molecule.edges()) {
        auto molecule_driver_pin_id = molecule.edge_driver(molecule_edge_id);
        auto molecule_driver_blk_id = molecule.pin_block(molecule_driver_pin_id);
        if (molecule.block_type(molecule_driver_blk_id) != PackMolecule::BlockType::INTERNAL) continue;
        
        for (auto molecule_sink_pin_id : molecule.edge_sinks(molecule_edge_id)) {
            auto molecule_sink_blk_id = molecule.pin_block(molecule_sink_pin_id);
            if (molecule.block_type(molecule_sink_blk_id) != PackMolecule::BlockType::INTERNAL) continue;

            //Both driver and sink are internal to this molecule, so remove it from the external input count
            --ext_inputs;
        }
    }

    return ext_inputs;
}
