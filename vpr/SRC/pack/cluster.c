/*
 * Main clustering algorithm
 * Author(s): Vaughn Betz (first revision - VPack), Alexander Marquardt (second revision - T-VPack), Jason Luu (third revision - AAPack)
 * June 8, 2011
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <map>
using namespace std;

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_math.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "pack_types.h"
#include "cluster.h"
#include "heapsort.h"
#include "output_clustering.h"
#include "SetupGrid.h"
#include "read_xml_arch_file.h"
#include "path_delay2.h"
#include "path_delay.h"
#include "vpr_utils.h"
#include "cluster_placement.h"
#include "ReadOptions.h"
#include "cluster_router.h"
#include "lb_type_rr_graph.h"

/*#define DEBUG_FAILED_PACKING_CANDIDATES*/

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
				  *we want to make sure that g_atoms_nlist.net criticality is      *
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
struct s_molecule_link {
	t_pack_molecule *moleculeptr;
	struct s_molecule_link *next;
};

/* Store stats on nets used by packed block, useful for determining transitively connected blocks 
(eg. [A1, A2, ..]->[B1, B2, ..]->C implies cluster [A1, A2, ...] and C have a weak link) */
struct t_lb_net_stats {
	int num_nets_in_lb;
	int *nets_in_lb;
};

/* Keeps a linked list of the unclustered blocks to speed up looking for *
 * unclustered blocks with a certain number of *external* inputs.        *
 * [0..lut_size].  Unclustered_list_head[i] points to the head of the    *
 * list of blocks with i inputs to be hooked up via external interconnect. */
static struct s_molecule_link *unclustered_list_head;
int unclustered_list_head_size;
static struct s_molecule_link *memory_pool; /*Declared here so I can free easily.*/

/* Does the logical_block that drives the output of this g_atoms_nlist.net also appear as a   *
 * receiver (input) pin of the g_atoms_nlist.net?  [0..g_atoms_nlist.net.size()-1].  If so, then by how much? This is used     *
 * in the gain routines to avoid double counting the connections from   *
 * the current cluster to other blocks (hence yielding better           *
 * clusterings).  The only time a logical_block should connect to the same g_atoms_nlist.net  *
 * twice is when one connection is an output and the other is an input, *
 * so this should take care of all multiple connections.                */
static int *net_output_feeds_driving_block_input;

/* Timing information for blocks */
static float *block_criticality = NULL;
static int *critindexarray = NULL;

/* Score different seeds for blocks */
static float *seed_blend_gain = NULL;
static int *seed_blend_index_array = NULL;
	

/*****************************************/
/*local functions*/
/*****************************************/

static void check_clocks(bool *is_clock);

#if 0
static void check_for_duplicate_inputs ();
#endif 

static bool is_logical_blk_in_pb(int iblk, t_pb *pb);

static void add_molecule_to_pb_stats_candidates(t_pack_molecule *molecule,
		map<int, float> &gain, t_pb *pb, int max_queue_size);

static void alloc_and_init_clustering(bool global_clocks, float alpha,
		float beta, int max_cluster_size, int max_molecule_inputs,
		int max_pb_depth, int max_models,
		t_cluster_placement_stats **cluster_placement_stats,
		t_pb_graph_node ***primitives_list, t_pack_molecule *molecules_head,
		int num_molecules);
static void free_pb_stats_recursive(t_pb *pb);
static void try_update_lookahead_pins_used(t_pb *cur_pb);
static void reset_lookahead_pins_used(t_pb *cur_pb);
static void compute_and_mark_lookahead_pins_used(int ilogical_block);
static void compute_and_mark_lookahead_pins_used_for_pin(
		t_pb_graph_pin *pb_graph_pin, t_pb *primitive_pb, int inet);
static void commit_lookahead_pins_used(t_pb *cur_pb);
static bool check_lookahead_pins_used(t_pb *cur_pb);
static bool primitive_feasible(int iblk, t_pb *cur_pb);
static bool primitive_type_and_memory_feasible(int iblk,
		const t_pb_type *cur_pb_type, t_pb *memory_class_pb,
		int sibling_memory_blk);

static t_pack_molecule *get_molecule_by_num_ext_inputs(
		const enum e_packer_algorithm packer_algorithm, t_pb *cur_pb,
		const int ext_inps, const enum e_removal_policy remove_flag,
		t_cluster_placement_stats *cluster_placement_stats_ptr);

static t_pack_molecule* get_free_molecule_with_most_ext_inputs_for_cluster(
		const enum e_packer_algorithm packer_algorithm, t_pb *cur_pb,
		t_cluster_placement_stats *cluster_placement_stats_ptr);

static t_pack_molecule* get_seed_logical_molecule_with_most_ext_inputs(
		int max_molecule_inputs);

static enum e_block_pack_status try_pack_molecule(
		t_cluster_placement_stats *cluster_placement_stats_ptr,
		const t_pack_molecule *molecule, t_pb_graph_node **primitives_list,
		t_pb * pb, const int max_models, const int max_cluster_size,
		const int clb_index, const int max_nets_in_pb_type, const int detailed_routing_stage, t_lb_router_data *router_data);
static enum e_block_pack_status try_place_logical_block_rec(
		const t_pb_graph_node *pb_graph_node, const int ilogical_block,
		t_pb *cb, t_pb **parent, const int max_models,
		const int max_cluster_size, const int clb_index,
		const int max_nets_in_pb_type,
		const t_cluster_placement_stats *cluster_placement_stats_ptr,
		const bool is_root_of_chain, const t_pb_graph_pin *chain_root_pin, t_lb_router_data *router_data);
static void revert_place_logical_block(const int ilogical_block,
		const int max_models, t_lb_router_data *router_data);

static void update_connection_gain_values(int inet, int clustered_block,
		t_pb * cur_pb,
		enum e_net_relation_to_clustered_block net_relation_to_clustered_block);

static void update_timing_gain_values(int inet, int clustered_block,
		t_pb* cur_pb,
		enum e_net_relation_to_clustered_block net_relation_to_clustered_block,
		const t_slack * slacks);

static void mark_and_update_partial_gain(int inet, enum e_gain_update gain_flag,
		int clustered_block, int port_on_clustered_block,
		int pin_on_clustered_block, bool timing_driven,
		bool connection_driven,
		enum e_net_relation_to_clustered_block net_relation_to_clustered_block, 
		const t_slack * slacks);

static void update_total_gain(float alpha, float beta, bool timing_driven,
		bool connection_driven, bool global_clocks, t_pb *pb);

static void update_cluster_stats( const t_pack_molecule *molecule,
		const int clb_index, const bool *is_clock, const bool global_clocks,
		const float alpha, const float beta, const bool timing_driven,
		const bool connection_driven, const t_slack * slacks);

static void start_new_cluster(
		t_cluster_placement_stats *cluster_placement_stats,
		t_pb_graph_node **primitives_list, const t_arch * arch,
		t_block *new_cluster, const int clb_index,
		const t_pack_molecule *molecule, const float aspect,
		int *num_used_instances_type, int *num_instances_type,
		const int num_models, const int max_cluster_size,
		const int max_nets_in_pb_type, vector<t_lb_type_rr_node> *lb_type_rr_graphs, t_lb_router_data **router_data, const int detailed_routing_stage);

static t_pack_molecule* get_highest_gain_molecule(
		const enum e_packer_algorithm packer_algorithm, t_pb *cur_pb,
		const enum e_gain_type gain_mode,
		t_cluster_placement_stats *cluster_placement_stats_ptr,
		t_lb_net_stats *clb_inter_blk_nets,
		int cluster_index);

static t_pack_molecule* get_molecule_for_cluster(
		const enum e_packer_algorithm packer_algorithm, t_pb *cur_pb,
		const bool allow_unrelated_clustering,
		int *num_unrelated_clustering_attempts,
		t_cluster_placement_stats *cluster_placement_stats_ptr,
		t_lb_net_stats *clb_inter_blk_nets,
		const int cluster_index);

static void check_clustering(int num_clb, t_block *clb, bool *is_clock);

static void check_cluster_logical_blocks(t_pb *pb, bool *blocks_checked);

static t_pack_molecule* get_highest_gain_seed_molecule(int * seedindex, bool getblend);

static float get_molecule_gain(t_pack_molecule *molecule, map<int, float> &blk_gain);
static int compare_molecule_gain(const void *a, const void *b);
static int get_net_corresponding_to_pb_graph_pin(t_pb *cur_pb,
		t_pb_graph_pin *pb_graph_pin);

static void print_block_criticalities(const char * fname);

static void load_transitive_fanout_candidates(int cluster_index,
											  t_pb_stats *pb_stats,
											  t_lb_net_stats *clb_inter_blk_nets);

/*****************************************/
/*globally accessable function*/
void do_clustering(const t_arch *arch, t_pack_molecule *molecule_head,
		int num_models, bool global_clocks, bool *is_clock,
		bool hill_climbing_flag, const char *out_fname, bool timing_driven, 
		enum e_cluster_seed cluster_seed_type, float alpha, float beta,
		int recompute_timing_after, float block_delay,
		float intra_cluster_net_delay, float inter_cluster_net_delay,
		float aspect, bool allow_unrelated_clustering,
		bool allow_early_exit, bool connection_driven,
		enum e_packer_algorithm packer_algorithm, t_timing_inf timing_inf, vector<t_lb_type_rr_node> *lb_type_rr_graphs) {

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

	int i, iblk, num_molecules, blocks_since_last_analysis, num_clb, max_nets_in_pb_type,  
		cur_nets_in_pb_type, num_blocks_hill_added, max_cluster_size, cur_cluster_size, 
		max_molecule_inputs, max_pb_depth, cur_pb_depth, num_unrelated_clustering_attempts,
		seedindex, savedseedindex /* index of next most timing critical block */,
		detailed_routing_stage, *hill_climbing_inputs_avail;

	int *num_used_instances_type, *num_instances_type; 
	/* [0..num_types] Holds array for total number of each cluster_type available */

	bool early_exit, is_cluster_legal;
	enum e_block_pack_status block_pack_status;
	float crit;

	t_cluster_placement_stats *cluster_placement_stats, *cur_cluster_placement_stats_ptr;
	t_pb_graph_node **primitives_list;
	t_block *clb; /* [0..num_clusters-1] */
	t_slack * slacks = NULL;
	t_lb_router_data *router_data = NULL;
	t_pack_molecule *istart, *next_molecule, *prev_molecule, *cur_molecule;
	t_lb_net_stats *clb_inter_blk_nets = NULL; /* [0..num_clusters-1] */

	vector < vector <t_intra_lb_net> * > intra_lb_routing;

#ifdef PATH_COUNTING
	unsigned int inet;
	int ipin;
#else
	int inode;
	float num_paths_scaling, distance_scaling;
#endif

	/* TODO: This is memory inefficient, fix if causes problems */
	clb = (t_block*)vtr::calloc(num_logical_blocks, sizeof(t_block));
	num_clb = 0;
	clb_inter_blk_nets = (t_lb_net_stats*) vtr::calloc(num_logical_blocks, sizeof(t_lb_net_stats));

	istart = NULL;

	/* determine bound on cluster size and primitive input size */
	max_cluster_size = 0;
	max_molecule_inputs = 0;
	max_pb_depth = 0;
	max_nets_in_pb_type = 0;

	seedindex = 0;

	cur_molecule = molecule_head;
	num_molecules = 0;
	while (cur_molecule != NULL) {
		cur_molecule->valid = true;
		if (cur_molecule->num_ext_inputs > max_molecule_inputs) {
			max_molecule_inputs = cur_molecule->num_ext_inputs;
		}
		num_molecules++;
		cur_molecule = cur_molecule->next;
	}

	for (i = 0; i < num_types; i++) {
		if (EMPTY_TYPE == &type_descriptors[i])
			continue;
		cur_cluster_size = get_max_primitives_in_pb_type(
				type_descriptors[i].pb_type);
		cur_pb_depth = get_max_depth_of_pb_type(type_descriptors[i].pb_type);
		cur_nets_in_pb_type = get_max_nets_in_pb_type(
				type_descriptors[i].pb_type);
		if (cur_cluster_size > max_cluster_size) {
			max_cluster_size = cur_cluster_size;
		}
		if (cur_pb_depth > max_pb_depth) {
			max_pb_depth = cur_pb_depth;
		}
		if (cur_nets_in_pb_type > max_nets_in_pb_type) {
			max_nets_in_pb_type = cur_nets_in_pb_type;
		}
	}

	if (hill_climbing_flag) {
		hill_climbing_inputs_avail = (int *) vtr::calloc(max_cluster_size + 1,
				sizeof(int));
	} else {
		hill_climbing_inputs_avail = NULL; /* if used, die hard */
	}

	/* TODO: make better estimate for nx and ny, was initializing nx = ny = 1 */
	nx = (arch->clb_grid.IsAuto ? 1 : arch->clb_grid.W);
	ny = (arch->clb_grid.IsAuto ? 1 : arch->clb_grid.H);

	check_clocks(is_clock);
#if 0
	check_for_duplicate_inputs ();
#endif
	alloc_and_init_clustering(global_clocks, alpha, beta, max_cluster_size,
			max_molecule_inputs, max_pb_depth, num_models,
			&cluster_placement_stats, &primitives_list, molecule_head,
			num_molecules);

	blocks_since_last_analysis = 0;
	early_exit = false;
	num_blocks_hill_added = 0;
	num_used_instances_type = (int*) vtr::calloc(num_types, sizeof(int));
	num_instances_type = (int*) vtr::calloc(num_types, sizeof(int));

	VTR_ASSERT(max_cluster_size < MAX_SHORT);
	/* Limit maximum number of elements for each cluster */

	if (timing_driven) {
		slacks = alloc_and_load_pre_packing_timing_graph(block_delay,
				inter_cluster_net_delay, arch->models, timing_inf);
		do_timing_analysis(slacks, timing_inf, true, false);

		if (getEchoEnabled()) {
			if(isEchoFileEnabled(E_ECHO_PRE_PACKING_TIMING_GRAPH))
				print_timing_graph(getEchoFileName(E_ECHO_PRE_PACKING_TIMING_GRAPH));
#ifndef PATH_COUNTING
			if(isEchoFileEnabled(E_ECHO_CLUSTERING_TIMING_INFO))
				print_clustering_timing_info(getEchoFileName(E_ECHO_CLUSTERING_TIMING_INFO));
#endif
			if(isEchoFileEnabled(E_ECHO_PRE_PACKING_SLACK))
				print_slack(slacks->slack, false, getEchoFileName(E_ECHO_PRE_PACKING_SLACK));
			if(isEchoFileEnabled(E_ECHO_PRE_PACKING_CRITICALITY))
				print_criticality(slacks, getEchoFileName(E_ECHO_PRE_PACKING_CRITICALITY));
		}

		block_criticality = (float*) vtr::calloc(num_logical_blocks, sizeof(float));

		critindexarray = (int*) vtr::malloc(num_logical_blocks * sizeof(int));

		seed_blend_gain = (float*) vtr::calloc(num_logical_blocks, sizeof(float));
		seed_blend_index_array = (int*) vtr::malloc(num_logical_blocks * sizeof(int));

		for (i = 0; i < num_logical_blocks; i++) {
			VTR_ASSERT(logical_block[i].index == i);
			critindexarray[i] = i;
			seed_blend_index_array[i] = i;
		}

#ifdef PATH_COUNTING
		/* Calculate block criticality from a weighted sum of timing and path criticalities. */
		for (inet = 0; inet < g_atoms_nlist.net.size(); inet++) { 
			for (ipin = 1; ipin < (int)g_atoms_nlist.net[inet].pins.size(); ipin++) { 
			
				/* Find the logical block iblk which this pin is a sink on. */
				iblk = g_atoms_nlist.net[inet].pins[ipin].block;
					
				/* The criticality of this pin is a sum of its timing and path criticalities. */
				crit =		PACK_PATH_WEIGHT  * slacks->path_criticality[inet][ipin] 
					 + (1 - PACK_PATH_WEIGHT) * slacks->timing_criticality[inet][ipin]; 

				/* The criticality of each block is the maximum of the criticalities of all its pins. */
				if (block_criticality[iblk] < crit) {
					block_criticality[iblk] = crit;
				}
			}
		}

#else
		/* Calculate criticality based on slacks and tie breakers (# paths, distance from source) */
		for (inode = 0; inode < num_tnodes; inode++) {
			/* Only calculate for tnodes which have valid normalized values.
			Either all values will be accurate or none will, so we only have
			to check whether one particular value (normalized_T_arr) is valid 
			Tnodes that do not have both times valid were not part of the analysis. 
			Because we calloc-ed the array criticality, such nodes will have criticality 0, the lowest possible value. */
			if (has_valid_normalized_T_arr(inode)) {
				iblk = tnode[inode].block;
				num_paths_scaling = SCALE_NUM_PATHS
						* (float) tnode[inode].prepacked_data->normalized_total_critical_paths;
				distance_scaling = SCALE_DISTANCE_VAL
						* (float) tnode[inode].prepacked_data->normalized_T_arr;
				crit = (1 - tnode[inode].prepacked_data->normalized_slack) + num_paths_scaling
						+ distance_scaling;
				if (block_criticality[iblk] < crit) {
					block_criticality[iblk] = crit;
				}
			}
		}
#endif

		for (iblk = 0; iblk < num_logical_blocks; iblk++) {
			/* Score seed gain of each block as a weighted sum of timing criticality, number of tightly coupled blocks connected to it, and number of external inputs */
			float seed_blend_fac = 0.5;
			float max_blend_gain = 0;
			vtr::t_linked_vptr *blk_molecules;
			blk_molecules = logical_block[iblk].packed_molecules;
			while(blk_molecules != NULL) {
				int blocks_of_molecule = 0;
				int inputs_of_molecule = 0;
				float blend_gain = 0;
			
				t_pack_molecule *blk_mol = (t_pack_molecule*) blk_molecules->data_vptr;
				inputs_of_molecule = blk_mol->num_ext_inputs;
				blocks_of_molecule = blk_mol->num_blocks;
				blend_gain = (
				              seed_blend_fac * block_criticality[iblk] + 
							  (1-seed_blend_fac) * (inputs_of_molecule / max_molecule_inputs)
							  ) * (1 + 0.2 * (blocks_of_molecule - 1));
				if(blend_gain > max_blend_gain) {
					max_blend_gain = blend_gain;
				}
				blk_molecules = blk_molecules->next;
			}
			seed_blend_gain[iblk] = max_blend_gain;
			
		}
		heapsort(critindexarray, block_criticality, num_logical_blocks, 1);
		heapsort(seed_blend_index_array, seed_blend_gain, num_logical_blocks, 1);
		
		if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_CLUSTERING_BLOCK_CRITICALITIES)) {
			print_block_criticalities(getEchoFileName(E_ECHO_CLUSTERING_BLOCK_CRITICALITIES));
		}

		if (cluster_seed_type == VPACK_BLEND) {
			istart = get_highest_gain_seed_molecule(&seedindex, true);
		} else if (cluster_seed_type == VPACK_TIMING) {
			istart = get_highest_gain_seed_molecule(&seedindex, false);
		} else {/*max input seed*/
			istart = get_seed_logical_molecule_with_most_ext_inputs(
					max_molecule_inputs);
		}

	} else /*cluster seed is max input (since there is no timing information)*/ {
		istart = get_seed_logical_molecule_with_most_ext_inputs(
				max_molecule_inputs);
	}

	/****************************************************************
	* Clustering
	*****************************************************************/

	while (istart != NULL) {
		is_cluster_legal = false;
		savedseedindex = seedindex;
		for (detailed_routing_stage = (int)E_DETAILED_ROUTE_AT_END_ONLY; !is_cluster_legal && detailed_routing_stage != (int)E_DETAILED_ROUTE_END; detailed_routing_stage++) {

			/* start a new cluster and reset all stats */
			start_new_cluster(cluster_placement_stats, primitives_list, arch,
					&clb[num_clb], num_clb, istart, aspect, num_used_instances_type,
					num_instances_type, num_models, max_cluster_size,
					max_nets_in_pb_type, lb_type_rr_graphs, &router_data, detailed_routing_stage);
			vtr::printf_info("Complex block %d: %s, type: %s\n", 
					num_clb, clb[num_clb].name, clb[num_clb].type->name);
			vtr::printf_info("\t");
			fflush(stdout);
			update_cluster_stats(istart, num_clb, is_clock, global_clocks, alpha,
					beta, timing_driven, connection_driven, slacks);
			num_clb++;

			if (timing_driven && !early_exit) {
				blocks_since_last_analysis++;
				/*it doesn't make sense to do a timing analysis here since there*
				 *is only one logical_block clustered it would not change anything      */
			}
			cur_cluster_placement_stats_ptr = &cluster_placement_stats[clb[num_clb
					- 1].type->index];
			num_unrelated_clustering_attempts = 0;
			next_molecule = get_molecule_for_cluster(PACK_BRUTE_FORCE,
					clb[num_clb - 1].pb, allow_unrelated_clustering,
					&num_unrelated_clustering_attempts,
					cur_cluster_placement_stats_ptr,
					clb_inter_blk_nets,
					num_clb - 1);
			prev_molecule = istart;
			while (next_molecule != NULL && prev_molecule != next_molecule) {
				block_pack_status = try_pack_molecule(
						cur_cluster_placement_stats_ptr, next_molecule,
						primitives_list, clb[num_clb - 1].pb, num_models,
						max_cluster_size, num_clb - 1, max_nets_in_pb_type, detailed_routing_stage, router_data);
				prev_molecule = next_molecule;
				if (block_pack_status != BLK_PASSED) {
					if (next_molecule != NULL) {
						if (block_pack_status == BLK_FAILED_ROUTE) {
#ifdef DEBUG_FAILED_PACKING_CANDIDATES
							vtr::printf_direct("\tNO_ROUTE:%s type %s/n", 
									next_molecule->logical_block_ptrs[next_molecule->root]->name, 
									next_molecule->logical_block_ptrs[next_molecule->root]->model->name);
							fflush(stdout);
	#else
							vtr::printf_direct(".");
	#endif
						} else {
	#ifdef DEBUG_FAILED_PACKING_CANDIDATES
							vtr::printf_direct("\tFAILED_CHECK:%s type %s check %d\n", 
									next_molecule->logical_block_ptrs[next_molecule->root]->name, 
									next_molecule->logical_block_ptrs[next_molecule->root]->model->name, 
									block_pack_status);
							fflush(stdout);
	#else
							vtr::printf_direct(".");
	#endif
						}
					}

					next_molecule = get_molecule_for_cluster(PACK_BRUTE_FORCE,
							clb[num_clb - 1].pb, allow_unrelated_clustering,
							&num_unrelated_clustering_attempts,
							cur_cluster_placement_stats_ptr,
							clb_inter_blk_nets,
							num_clb - 1);
					continue;
				} else {
					/* Continue packing by filling smallest cluster */
	#ifdef DEBUG_FAILED_PACKING_CANDIDATES			
					vtr::printf_direct("\tPASSED:%s type %s\n", 
							next_molecule->logical_block_ptrs[next_molecule->root]->name, 
							next_molecule->logical_block_ptrs[next_molecule->root]->model->name);
					fflush(stdout);
	#else
					vtr::printf_direct(".");
	#endif
				}
				update_cluster_stats(next_molecule, num_clb - 1, is_clock,
						global_clocks, alpha, beta, timing_driven,
						connection_driven, slacks);
				num_unrelated_clustering_attempts = 0;

				if (timing_driven && !early_exit) {
					blocks_since_last_analysis++; /* historically, timing slacks were recomputed after X number of blocks were packed, but this doesn't significantly alter results so I (jluu) did not port the code */
				}
				next_molecule = get_molecule_for_cluster(PACK_BRUTE_FORCE,
						clb[num_clb - 1].pb, allow_unrelated_clustering,
						&num_unrelated_clustering_attempts,
						cur_cluster_placement_stats_ptr,
						clb_inter_blk_nets,
						num_clb - 1);
			}
			vtr::printf_direct("\n");
			if (detailed_routing_stage == (int)E_DETAILED_ROUTE_AT_END_ONLY) {
				is_cluster_legal = try_intra_lb_route(router_data);
				if (is_cluster_legal == true) {
					vtr::printf_info("Passed route at end.\n");
				} else {
					vtr::printf_info("Failed route at end, repack cluster trying detailed routing at each stage.\n");
				}
			} else {
				is_cluster_legal = true;
			}
			if (is_cluster_legal == true) {
				intra_lb_routing.push_back(router_data->saved_lb_nets);
				VTR_ASSERT((int)intra_lb_routing.size() == num_clb);
				router_data->saved_lb_nets = NULL;
				if (timing_driven) {
					if (num_blocks_hill_added > 0 && !early_exit) {
						blocks_since_last_analysis += num_blocks_hill_added;
					}
					if (cluster_seed_type == VPACK_BLEND) {
						istart = get_highest_gain_seed_molecule(&seedindex, true);
					} else if (cluster_seed_type == VPACK_TIMING) {
						istart = get_highest_gain_seed_molecule(&seedindex, false);
					} else { /*max input seed*/
						istart = get_seed_logical_molecule_with_most_ext_inputs(
								max_molecule_inputs);
					}
				} else
					/*cluster seed is max input (since there is no timing information)*/
					istart = get_seed_logical_molecule_with_most_ext_inputs(
							max_molecule_inputs);
				
				/* store info that will be used later in packing from pb_stats and free the rest */
				t_pb_stats *pb_stats = clb[num_clb - 1].pb->pb_stats;
				clb_inter_blk_nets[num_clb - 1].nets_in_lb = (int*)vtr::malloc(sizeof(int) * pb_stats->num_marked_nets);
				for(int inet = 0; inet < pb_stats->num_marked_nets; inet++) {
					int mnet = pb_stats->marked_nets[inet];
					int external_terminals = g_atoms_nlist.net[mnet].pins.size() - pb_stats->num_pins_of_net_in_pb[inet];
					/* Check if external terminals of net is within the fanout limit and that there exists external terminals */
					if(external_terminals < AAPACK_MAX_TRANSITIVE_FANOUT_EXPLORE && external_terminals > 0) {
						clb_inter_blk_nets[num_clb - 1].nets_in_lb[clb_inter_blk_nets[num_clb - 1].num_nets_in_lb] = mnet;
						clb_inter_blk_nets[num_clb - 1].num_nets_in_lb++;
					}
				}
				free_pb_stats_recursive(clb[num_clb - 1].pb);
			} else {
				/* Free up data structures and requeue used molecules */
				num_used_instances_type[clb[num_clb - 1].type->index]--;
				free_cb(clb[num_clb - 1].pb);
				free(clb[num_clb - 1].pb);
				free(clb[num_clb - 1].name);
				clb[num_clb - 1].name = NULL;
				clb[num_clb - 1].pb = NULL;
				num_clb--;
				seedindex = savedseedindex;
			}
			free_router_data(router_data);
			router_data = NULL;
		}
	}

	/****************************************************************
	* Free Data Structures 
	*****************************************************************/
	check_clustering(num_clb, clb, is_clock);

	block = clb;

	output_clustering(arch, clb, num_clb, intra_lb_routing, global_clocks, is_clock, out_fname, false);
	
	block = NULL;
	for(int irt = 0; irt < (int) intra_lb_routing.size(); irt++){
		free_intra_lb_nets(intra_lb_routing[irt]);
	}
	intra_lb_routing.clear();

	if (hill_climbing_flag) {
		free(hill_climbing_inputs_avail);
	}
	free_cluster_placement_stats(cluster_placement_stats);

	for (i = 0; i < num_clb; i++) {
		free_cb(clb[i].pb);
		free(clb[i].name);
		free(clb[i].nets);
		free(clb[i].pb);
	}
	free(clb);

	free(num_used_instances_type);
	free(num_instances_type);
	free(unclustered_list_head);
	free(memory_pool);
	free(net_output_feeds_driving_block_input);

	if (timing_driven) {
		free(block_criticality);
		free(critindexarray);
		free(seed_blend_gain);
		free(seed_blend_index_array);

		block_criticality = NULL;
		critindexarray = NULL;
		seed_blend_gain = NULL;
		seed_blend_index_array = NULL;
	}

	if (timing_driven) {
		free_timing_graph(slacks);
	}

	free (primitives_list);
	if(clb_inter_blk_nets != NULL) {
		for(i = 0; i < num_clb; i++) {
			free(clb_inter_blk_nets[i].nets_in_lb);
		}
	   free(clb_inter_blk_nets);
	   clb_inter_blk_nets = NULL;
	}
}

/*****************************************/
static void check_clocks(bool *is_clock) {

	/* Checks that nets used as clock inputs to latches are never also used *
	 * as VPACK_LUT inputs.  It's electrically questionable, and more importantly *
	 * would break the clustering code.                                     */

	int inet, iblk, ipin;
	t_model_ports *port;

	for (iblk = 0; iblk < num_logical_blocks; iblk++) {
		if (logical_block[iblk].type != VPACK_OUTPAD) {
			port = logical_block[iblk].model->inputs;
			while (port) {
				for (ipin = 0; ipin < port->size; ipin++) {
					inet = logical_block[iblk].input_nets[port->index][ipin];
					if (inet != OPEN) {
						if (is_clock[inet]) {
							vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
									"Error in check_clocks.\n"
									"Net %d (%s) is a clock, but also connects to a logic block input on logical_block %d (%s).\n"
									"This would break the current clustering implementation and is electrically questionable, so clustering has been aborted.\n",
									inet, g_atoms_nlist.net[inet].name, iblk, logical_block[iblk].name);
						}
					}
				}
				port = port->next;
			}
		}
	}
}

/* Determine if logical block is in pb */
static bool is_logical_blk_in_pb(int iblk, t_pb *pb) {
	t_pb * cur_pb;
	cur_pb = logical_block[iblk].pb;
	while (cur_pb) {
		if (cur_pb == pb) {
			return true;
		}
		cur_pb = cur_pb->parent_pb;
	}
	return false;
}

/* Add blk to list of feasible blocks sorted according to gain */
static void add_molecule_to_pb_stats_candidates(t_pack_molecule *molecule,
		map<int, float> &gain, t_pb *pb, int max_queue_size) {
	int i, j;

	for (i = 0; i < pb->pb_stats->num_feasible_blocks; i++) {
		if (pb->pb_stats->feasible_blocks[i] == molecule) {
			return; /* already in queue, do nothing */
		}
	}

	if (pb->pb_stats->num_feasible_blocks
			>= max_queue_size - 1) {
		/* maximum size for array, remove smallest gain element and sort */
		if (get_molecule_gain(molecule, gain)
				> get_molecule_gain(pb->pb_stats->feasible_blocks[0], gain)) {
			/* single loop insertion sort */
			for (j = 0; j < pb->pb_stats->num_feasible_blocks - 1; j++) {
				if (get_molecule_gain(molecule, gain)
						<= get_molecule_gain(
								pb->pb_stats->feasible_blocks[j + 1], gain)) {
					pb->pb_stats->feasible_blocks[j] = molecule;
					break;
				} else {
					pb->pb_stats->feasible_blocks[j] =
							pb->pb_stats->feasible_blocks[j + 1];
				}
			}
			if (j == pb->pb_stats->num_feasible_blocks - 1) {
				pb->pb_stats->feasible_blocks[j] = molecule;
			}
		}
	} else {
		/* Expand array and single loop insertion sort */
		for (j = pb->pb_stats->num_feasible_blocks - 1; j >= 0; j--) {
			if (get_molecule_gain(pb->pb_stats->feasible_blocks[j], gain)
					> get_molecule_gain(molecule, gain)) {
				pb->pb_stats->feasible_blocks[j + 1] =
						pb->pb_stats->feasible_blocks[j];
			} else {
				pb->pb_stats->feasible_blocks[j + 1] = molecule;
				break;
			}
		}
		if (j < 0) {
			pb->pb_stats->feasible_blocks[0] = molecule;
		}
		pb->pb_stats->num_feasible_blocks++;
	}
}

/*****************************************/
static void alloc_and_init_clustering(bool global_clocks, float alpha,
		float beta, int max_cluster_size, int max_molecule_inputs,
		int max_pb_depth, int max_models,
		t_cluster_placement_stats **cluster_placement_stats,
		t_pb_graph_node ***primitives_list, t_pack_molecule *molecules_head,
		int num_molecules) {

	/* Allocates the main data structures used for clustering and properly *
	 * initializes them.                                                   */

	int i, ext_inps, ipin, driving_blk;
	unsigned int inet;
	struct s_molecule_link *next_ptr;
	t_pack_molecule *cur_molecule;
	t_pack_molecule **molecule_array;
	int max_molecule_size;

	for (i = 0; i < num_logical_blocks; i++) {
		logical_block[i].clb_index = NO_CLUSTER;
	}

	/* alloc and load list of molecules to pack */
	unclustered_list_head = (struct s_molecule_link *) vtr::calloc(
			max_molecule_inputs + 1, sizeof(struct s_molecule_link));
	unclustered_list_head_size = max_molecule_inputs + 1;

	for (i = 0; i <= max_molecule_inputs; i++) {
		unclustered_list_head[i].next = NULL;
	}

	molecule_array = (t_pack_molecule **) vtr::malloc(
			num_molecules * sizeof(t_pack_molecule*));
	cur_molecule = molecules_head;
	for (i = 0; i < num_molecules; i++) {
		VTR_ASSERT(cur_molecule != NULL);
		molecule_array[i] = cur_molecule;
		cur_molecule = cur_molecule->next;
	}
	VTR_ASSERT(cur_molecule == NULL);
	qsort((void*) molecule_array, num_molecules, sizeof(t_pack_molecule*),
			compare_molecule_gain);

	memory_pool = (struct s_molecule_link *) vtr::malloc(
			num_molecules * sizeof(struct s_molecule_link));
	next_ptr = memory_pool;

	for (i = 0; i < num_molecules; i++) {
		ext_inps = molecule_array[i]->num_ext_inputs;
		next_ptr->moleculeptr = molecule_array[i];
		next_ptr->next = unclustered_list_head[ext_inps].next;
		unclustered_list_head[ext_inps].next = next_ptr;
		next_ptr++;
	}
	free(molecule_array);

	/* alloc and load net info */
	net_output_feeds_driving_block_input = (int *) vtr::malloc(
		g_atoms_nlist.net.size() * sizeof(int));

	for (inet = 0; inet < g_atoms_nlist.net.size(); inet++) {
		net_output_feeds_driving_block_input[inet] = 0;
		driving_blk = g_atoms_nlist.net[inet].pins[0].block;
		for (ipin = 1; ipin < (int) g_atoms_nlist.net[inet].pins.size(); ipin++) {
			if (g_atoms_nlist.net[inet].pins[ipin].block == driving_blk) {
				net_output_feeds_driving_block_input[inet]++;
			}
		}
	}

	/* alloc and load cluster placement info */
	*cluster_placement_stats = alloc_and_load_cluster_placement_stats();

	/* alloc array that will store primitives that a molecule gets placed to, 
	 primitive_list is referenced by index, for example a logical block in index 2 of a molecule matches to a primitive in index 2 in primitive_list
	 this array must be the size of the biggest molecule
	 */
	max_molecule_size = 1;
	cur_molecule = molecules_head;
	while (cur_molecule != NULL) {
		if (cur_molecule->num_blocks > max_molecule_size) {
			max_molecule_size = cur_molecule->num_blocks;
		}
		cur_molecule = cur_molecule->next;
	}
	*primitives_list = (t_pb_graph_node **)vtr::calloc(max_molecule_size, sizeof(t_pb_graph_node *));
}

/*****************************************/
static void free_pb_stats_recursive(t_pb *pb) {

	int i, j;
	/* Releases all the memory used by clustering data structures.   */
	if (pb) {
		if (pb->pb_graph_node != NULL) {
			if (pb->pb_graph_node->pb_type->num_modes != 0) {
				for (i = 0;
						i
								< pb->pb_graph_node->pb_type->modes[pb->mode].num_pb_type_children;
						i++) {
					for (j = 0;
							j
									< pb->pb_graph_node->pb_type->modes[pb->mode].pb_type_children[i].num_pb;
							j++) {
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

static bool primitive_feasible(int iblk, t_pb *cur_pb) {
	const t_pb_type *cur_pb_type;
	int i;
	t_pb *memory_class_pb; /* Used for memory class only, for memories, open pins must be the same among siblings */
	int sibling_memory_blk;

	cur_pb_type = cur_pb->pb_graph_node->pb_type;
	memory_class_pb = NULL;
	sibling_memory_blk = OPEN;

	VTR_ASSERT(cur_pb_type->num_modes == 0);
	/* primitive */
	if (cur_pb->logical_block != OPEN && cur_pb->logical_block != iblk) {
		/* This pb already has a different logical block */
		return false;
	}

	if (cur_pb_type->class_type == MEMORY_CLASS) {
		/* memory class is special, all siblings must share all nets, including open nets, with the exception of data nets */
		/* find sibling if one exists */
		memory_class_pb = cur_pb->parent_pb;
		for (i = 0; i < cur_pb_type->parent_mode->num_pb_type_children; i++) {
			if (memory_class_pb->child_pbs[cur_pb->mode][i].name != NULL
					&& memory_class_pb->child_pbs[cur_pb->mode][i].logical_block
							!= OPEN) {
				sibling_memory_blk =
						memory_class_pb->child_pbs[cur_pb->mode][i].logical_block;
			}
		}
		if (sibling_memory_blk == OPEN) {
			memory_class_pb = NULL;
		}
	}

	return primitive_type_and_memory_feasible(iblk, cur_pb_type,
			memory_class_pb, sibling_memory_blk);
}

static bool primitive_type_and_memory_feasible(int iblk,
		const t_pb_type *cur_pb_type, t_pb *memory_class_pb,
		int sibling_memory_blk) {
	t_model_ports *port;
	int i, j;
	bool second_pass;

	/* check if ports are big enough */
	/* for memories, also check that pins are the same with existing siblings */
	port = logical_block[iblk].model->inputs;
	second_pass = false;
	while (port || !second_pass) {
		/* TODO: This is slow if the number of ports are large, fix if becomes a problem */
		if (!port) {
			second_pass = true;
			port = logical_block[iblk].model->outputs;
		}
		for (i = 0; i < cur_pb_type->num_ports; i++) {
			if (cur_pb_type->ports[i].model_port == port) {
				/* TODO: This is slow, I only need to check from 0 if it is a memory block, other blocks I can check from port->size onwards */
				for (j = 0; j < port->size; j++) {
					if (port->dir == IN_PORT && !port->is_clock) {
						if (memory_class_pb) {
							if (cur_pb_type->ports[i].port_class == NULL
									|| strstr(cur_pb_type->ports[i].port_class,
											"data")
											!= cur_pb_type->ports[i].port_class) {
								if (logical_block[iblk].input_nets[port->index][j]
										!= logical_block[sibling_memory_blk].input_nets[port->index][j]) {
									return false;
								}
							}
						}
						if (logical_block[iblk].input_nets[port->index][j]
								!= OPEN
								&& j >= cur_pb_type->ports[i].num_pins) {
							return false;
						}
					} else if (port->dir == OUT_PORT) {
						if (memory_class_pb) {
							if (cur_pb_type->ports[i].port_class == NULL
									|| strstr(cur_pb_type->ports[i].port_class,
											"data")
											!= cur_pb_type->ports[i].port_class) {
								if (logical_block[iblk].output_nets[port->index][j]
										!= logical_block[sibling_memory_blk].output_nets[port->index][j]) {
									return false;
								}
							}
						}
						if (logical_block[iblk].output_nets[port->index][j]
								!= OPEN
								&& j >= cur_pb_type->ports[i].num_pins) {
							return false;
						}
					} else {
						VTR_ASSERT(port->dir == IN_PORT && port->is_clock);
						VTR_ASSERT(j == 0);
						if (memory_class_pb) {
							if (logical_block[iblk].clock_net
									!= logical_block[sibling_memory_blk].clock_net) {
								return false;
							}
						}
						if (logical_block[iblk].clock_net != OPEN
								&& j >= cur_pb_type->ports[i].num_pins) {
							return false;
						}
					}
				}
				break;
			}
		}
		if (i == cur_pb_type->num_ports) {
			if (((logical_block[iblk].model->inputs != NULL) && !second_pass)
					|| (logical_block[iblk].model->outputs != NULL
							&& second_pass)) {
				/* physical port not found */
				return false;
			}
		}
		if (port) {
			port = port->next;
		}
	}
	return true;
}

/*****************************************/
static t_pack_molecule *get_molecule_by_num_ext_inputs(
		const enum e_packer_algorithm packer_algorithm, t_pb *cur_pb,
		const int ext_inps, const enum e_removal_policy remove_flag,
		t_cluster_placement_stats *cluster_placement_stats_ptr) {

	/* This routine returns a logical_block which has not been clustered, has  *
	 * no connection to the current cluster, satisfies the cluster     *
	 * clock constraints, is a valid subblock inside the cluster, does not exceed the cluster subblock units available,
	 and has ext_inps external inputs.  If        *
	 * there is no such logical_block it returns NO_CLUSTER.  Remove_flag      *
	 * controls whether or not blocks that have already been clustered *
	 * are removed from the unclustered_list data structures.  NB:     *
	 * to get a logical_block regardless of clock constraints just set clocks_ *
	 * avail > 0.                                                      */

	struct s_molecule_link *ptr, *prev_ptr;
	int ilogical_blk, i;
	bool success;

	prev_ptr = &unclustered_list_head[ext_inps];
	ptr = unclustered_list_head[ext_inps].next;
	while (ptr != NULL) {
		/* TODO: Get better candidate logical block in future, eg. return most timing critical or some other smarter metric */
		if (ptr->moleculeptr->valid) {
			success = true;
			for (i = 0; i < get_array_size_of_molecule(ptr->moleculeptr); i++) {
				if (ptr->moleculeptr->logical_block_ptrs[i] != NULL) {
					ilogical_blk =
							ptr->moleculeptr->logical_block_ptrs[i]->index;
					if (!exists_free_primitive_for_logical_block(
							cluster_placement_stats_ptr, ilogical_blk)) { /* TODO: I should be using a better filtering check especially when I'm dealing with multiple clock/multiple global reset signals where the clock/reset packed in matters, need to do later when I have the circuits to check my work */
						success = false;
						break;
					}
				}
			}
			if (success == true) {
				return ptr->moleculeptr;
			}
			prev_ptr = ptr;
		}

		else if (remove_flag == REMOVE_CLUSTERED) {
			VTR_ASSERT(0); /* this doesn't work right now with 2 the pass packing for each complex block */
			prev_ptr->next = ptr->next;
		}

		ptr = ptr->next;
	}

	return NULL;
}

/*****************************************/
static t_pack_molecule *get_free_molecule_with_most_ext_inputs_for_cluster(
		const enum e_packer_algorithm packer_algorithm, t_pb *cur_pb,
		t_cluster_placement_stats *cluster_placement_stats_ptr) {

	/* This routine is used to find new blocks for clustering when there are no feasible       *
	 * blocks with any attraction to the current cluster (i.e. it finds       *
	 * blocks which are unconnected from the current cluster).  It returns    *
	 * the logical_block with the largest number of used inputs that satisfies the    *
	 * clocking and number of inputs constraints.  If no suitable logical_block is    *
	 * found, the routine returns NO_CLUSTER.                                 
	 * TODO: Analyze if this function is useful in more detail, also, should probably not include clock in input count
	 */

	int ext_inps;
	int i, j;
	t_pack_molecule *molecule;

	int inputs_avail = 0;

	for (i = 0; i < cur_pb->pb_graph_node->num_input_pin_class; i++) {
		for (j = 0; j < cur_pb->pb_graph_node->input_pin_class_size[i]; j++) {
			if (cur_pb->pb_stats->input_pins_used[i][j] == OPEN)
				inputs_avail++;
		}
	}

	molecule = NULL;

	if (inputs_avail >= unclustered_list_head_size) {
		inputs_avail = unclustered_list_head_size - 1;
	}

	for (ext_inps = inputs_avail; ext_inps >= 0; ext_inps--) {
		molecule = get_molecule_by_num_ext_inputs(packer_algorithm, cur_pb,
				ext_inps, LEAVE_CLUSTERED, cluster_placement_stats_ptr);
		if (molecule != NULL) {
			break;
		}
	}
	return molecule;
}

/*****************************************/
static t_pack_molecule* get_seed_logical_molecule_with_most_ext_inputs(
		int max_molecule_inputs) {

	/* This routine is used to find the first seed logical_block for the clustering.  It returns    *
	 * the logical_block with the largest number of used inputs that satisfies the    *
	 * clocking and number of inputs constraints.  If no suitable logical_block is    *
	 * found, the routine returns NO_CLUSTER.                                 */

	int ext_inps;
	struct s_molecule_link *ptr;

	for (ext_inps = max_molecule_inputs; ext_inps >= 0; ext_inps--) {
		ptr = unclustered_list_head[ext_inps].next;

		while (ptr != NULL) {
			if (ptr->moleculeptr->valid) {
				return ptr->moleculeptr;
			}
			ptr = ptr->next;
		}
	}
	return NULL;
}

/*****************************************/

/*****************************************/
static void alloc_and_load_pb_stats(t_pb *pb, int max_models,
		int max_nets_in_pb_type) {

	/* Call this routine when starting to fill up a new cluster.  It resets *
	 * the gain vector, etc.                                                */

	int i, j;

	pb->pb_stats = new t_pb_stats;

	/* If statement below is for speed.  If nets are reasonably low-fanout,  *
	 * only a relatively small number of blocks will be marked, and updating *
	 * only those logical_block structures will be fastest.  If almost all blocks    *
	 * have been touched it should be faster to just run through them all    *
	 * in order (less addressing and better cache locality).                 */
	pb->pb_stats->input_pins_used = (int **) vtr::malloc(
			pb->pb_graph_node->num_input_pin_class * sizeof(int*));
	pb->pb_stats->output_pins_used = (int **) vtr::malloc(
			pb->pb_graph_node->num_output_pin_class * sizeof(int*));
	pb->pb_stats->lookahead_input_pins_used = new vector<int> [pb->pb_graph_node->num_input_pin_class];
	pb->pb_stats->lookahead_output_pins_used = new vector<int> [pb->pb_graph_node->num_output_pin_class];
	pb->pb_stats->num_feasible_blocks = NOT_VALID;
	pb->pb_stats->feasible_blocks = (t_pack_molecule**) vtr::calloc(
			AAPACK_MAX_FEASIBLE_BLOCK_ARRAY_SIZE, sizeof(t_pack_molecule *));

	pb->pb_stats->tie_break_high_fanout_net = OPEN;
	for (i = 0; i < pb->pb_graph_node->num_input_pin_class; i++) {
		pb->pb_stats->input_pins_used[i] = (int*) vtr::malloc(
				pb->pb_graph_node->input_pin_class_size[i] * sizeof(int));
		for (j = 0; j < pb->pb_graph_node->input_pin_class_size[i]; j++) {
			pb->pb_stats->input_pins_used[i][j] = OPEN;
		}
	}

	for (i = 0; i < pb->pb_graph_node->num_output_pin_class; i++) {
		pb->pb_stats->output_pins_used[i] = (int*) vtr::malloc(
				pb->pb_graph_node->output_pin_class_size[i] * sizeof(int));
		for (j = 0; j < pb->pb_graph_node->output_pin_class_size[i]; j++) {
			pb->pb_stats->output_pins_used[i][j] = OPEN;
		}
	}

	pb->pb_stats->gain.clear();
	pb->pb_stats->timinggain.clear();
	pb->pb_stats->connectiongain.clear();
	pb->pb_stats->sharinggain.clear();
	pb->pb_stats->hillgain.clear();

	pb->pb_stats->num_pins_of_net_in_pb.clear();

	pb->pb_stats->marked_nets = (int *) vtr::malloc(
			max_nets_in_pb_type * sizeof(int));
	pb->pb_stats->marked_blocks = (int *) vtr::malloc(
			num_logical_blocks * sizeof(int));

	pb->pb_stats->num_marked_nets = 0;
	pb->pb_stats->num_marked_blocks = 0;

	pb->pb_stats->num_child_blocks_in_pb = 0;

	pb->pb_stats->explore_transitive_fanout = true;
	pb->pb_stats->transitive_fanout_candidates = NULL;
}
/*****************************************/

/**
 * Try pack molecule into current cluster
 */
static enum e_block_pack_status try_pack_molecule(
		t_cluster_placement_stats *cluster_placement_stats_ptr,
		const t_pack_molecule *molecule, t_pb_graph_node **primitives_list,
		t_pb * pb, const int max_models, const int max_cluster_size,
		const int clb_index, const int max_nets_in_pb_type, const int detailed_routing_stage, t_lb_router_data *router_data) {
	int molecule_size, failed_location;
	int i;
	enum e_block_pack_status block_pack_status;
	vtr::t_linked_vptr *cur_molecule;
	t_pb *parent;
	t_pb *cur_pb;
	t_logical_block *chain_root_block;
	bool is_root_of_chain;
	t_pb_graph_pin *chain_root_pin;

	
	parent = NULL;
	
	block_pack_status = BLK_STATUS_UNDEFINED;

	molecule_size = get_array_size_of_molecule(molecule);
	failed_location = 0;

	while (block_pack_status != BLK_PASSED) {
		if (get_next_primitive_list(cluster_placement_stats_ptr, molecule,
				primitives_list, clb_index)) {
			block_pack_status = BLK_PASSED;
			
			for (i = 0; i < molecule_size && block_pack_status == BLK_PASSED;
					i++) {
				VTR_ASSERT((primitives_list[i] == NULL) == (molecule->logical_block_ptrs[i] == NULL));
				failed_location = i + 1;
				if (molecule->logical_block_ptrs[i] != NULL) {
					if(molecule->type == MOLECULE_FORCED_PACK && molecule->pack_pattern->is_chain && i == molecule->pack_pattern->root_block->block_id) {
						chain_root_pin = molecule->pack_pattern->chain_root_pin;
						is_root_of_chain = true;
					} else {
						chain_root_pin = NULL;
						is_root_of_chain = false;
					}
					block_pack_status = try_place_logical_block_rec(
							primitives_list[i],
							molecule->logical_block_ptrs[i]->index, pb, &parent,
							max_models, max_cluster_size, clb_index,
							max_nets_in_pb_type, cluster_placement_stats_ptr, is_root_of_chain, chain_root_pin, router_data);
				}
			}
			if (block_pack_status == BLK_PASSED) {
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
				if (detailed_routing_stage == (int)E_DETAILED_ROUTE_FOR_EACH_ATOM && try_intra_lb_route(router_data) == false) {
					/* Cannot pack */
					block_pack_status = BLK_FAILED_ROUTE;
				}
				else{
					/* Pack successful, commit 
					 TODO: SW Engineering note - may want to update cluster stats here too instead of doing it outside
					 */
					VTR_ASSERT(block_pack_status == BLK_PASSED);
					if(molecule->type == MOLECULE_FORCED_PACK && molecule->pack_pattern->is_chain) {
						/* Chained molecules often take up lots of area and are important, if a chain is packed in, want to rename logic block to match chain name */
						chain_root_block = molecule->logical_block_ptrs[molecule->pack_pattern->root_block->block_id];
						cur_pb = chain_root_block->pb->parent_pb;
						while(cur_pb != NULL) {
							free(cur_pb->name);
							cur_pb->name = vtr::strdup(chain_root_block->name);
							cur_pb = cur_pb->parent_pb;
						}
					}
					for (i = 0; i < molecule_size; i++) {
						if (molecule->logical_block_ptrs[i] != NULL) {
							/* invalidate all molecules that share logical block with current molecule */
							cur_molecule =
									molecule->logical_block_ptrs[i]->packed_molecules;
							while (cur_molecule != NULL) {
								((t_pack_molecule*) cur_molecule->data_vptr)->valid =
										false;
								cur_molecule = cur_molecule->next;
							}
							commit_primitive(cluster_placement_stats_ptr,
									primitives_list[i]);
						}
					}
				}
			}
			if (block_pack_status != BLK_PASSED) {
				for (i = 0; i < failed_location; i++) {
					if (molecule->logical_block_ptrs[i] != NULL) {
						remove_atom_from_target(router_data, molecule->logical_block_ptrs[i]->index);
					}
				}
				for (i = 0; i < failed_location; i++) {					
					if (molecule->logical_block_ptrs[i] != NULL) {
						revert_place_logical_block(
								molecule->logical_block_ptrs[i]->index,
								max_models, router_data);
					}
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
 * Try place logical block into current primitive location
 */

static enum e_block_pack_status try_place_logical_block_rec(
		const t_pb_graph_node *pb_graph_node, const int ilogical_block,
		t_pb *cb, t_pb **parent, const int max_models,
		const int max_cluster_size, const int clb_index,
		const int max_nets_in_pb_type,
		const t_cluster_placement_stats *cluster_placement_stats_ptr,
		const bool is_root_of_chain, const t_pb_graph_pin *chain_root_pin, t_lb_router_data *router_data) {
	int i, j;
	bool is_primitive;
	enum e_block_pack_status block_pack_status;

	t_pb *my_parent;
	t_pb *pb, *parent_pb;
	const t_pb_type *pb_type;

	t_model_ports *root_port;

	my_parent = NULL;

	block_pack_status = BLK_PASSED;

	/* Discover parent */
	if (pb_graph_node->parent_pb_graph_node != cb->pb_graph_node) {
		block_pack_status = try_place_logical_block_rec(
				pb_graph_node->parent_pb_graph_node, ilogical_block, cb,
				&my_parent, max_models, max_cluster_size, clb_index,
				max_nets_in_pb_type, cluster_placement_stats_ptr, is_root_of_chain, chain_root_pin, router_data);
		parent_pb = my_parent;
	} else {
		parent_pb = cb;
	}

	/* Create siblings if siblings are not allocated */
	if (parent_pb->child_pbs == NULL) {
		VTR_ASSERT(parent_pb->name == NULL);
		parent_pb->logical_block = OPEN;
		parent_pb->name = vtr::strdup(logical_block[ilogical_block].name);
		parent_pb->mode = pb_graph_node->pb_type->parent_mode->index;
		set_reset_pb_modes(router_data, parent_pb, true);
		parent_pb->child_pbs =
				(t_pb **) vtr::calloc(
						parent_pb->pb_graph_node->pb_type->modes[parent_pb->mode].num_pb_type_children,
						sizeof(t_pb *));
		for (i = 0;
				i
						< parent_pb->pb_graph_node->pb_type->modes[parent_pb->mode].num_pb_type_children;
				i++) {
			parent_pb->child_pbs[i] =
					(t_pb *) vtr::calloc(
							parent_pb->pb_graph_node->pb_type->modes[parent_pb->mode].pb_type_children[i].num_pb,
							sizeof(t_pb));
			for (j = 0;
					j
							< parent_pb->pb_graph_node->pb_type->modes[parent_pb->mode].pb_type_children[i].num_pb;
					j++) {
				parent_pb->child_pbs[i][j].parent_pb = parent_pb;
				parent_pb->child_pbs[i][j].logical_block = OPEN;
				parent_pb->child_pbs[i][j].pb_graph_node =
						&(parent_pb->pb_graph_node->child_pb_graph_nodes[parent_pb->mode][i][j]);
			}
		}
	} else {
		VTR_ASSERT(parent_pb->mode == pb_graph_node->pb_type->parent_mode->index);
	}

	for (i = 0;
			i
					< parent_pb->pb_graph_node->pb_type->modes[parent_pb->mode].num_pb_type_children;
			i++) {
		if (pb_graph_node->pb_type
				== &parent_pb->pb_graph_node->pb_type->modes[parent_pb->mode].pb_type_children[i]) {
			break;
		}
	}
	VTR_ASSERT(i < parent_pb->pb_graph_node->pb_type->modes[parent_pb->mode].num_pb_type_children);
	pb = &parent_pb->child_pbs[i][pb_graph_node->placement_index];
	*parent = pb; /* this pb is parent of it's child that called this function */
	VTR_ASSERT(pb->pb_graph_node == pb_graph_node);
	if (pb->pb_stats == NULL) {
		alloc_and_load_pb_stats(pb, max_models, max_nets_in_pb_type);
	}
	pb_type = pb_graph_node->pb_type;

	is_primitive = (pb_type->num_modes == 0);

	if (is_primitive) {
		VTR_ASSERT(pb->logical_block == OPEN && logical_block[ilogical_block].pb == NULL && logical_block[ilogical_block].clb_index == NO_CLUSTER);
		/* try pack to location */
		pb->name = vtr::strdup(logical_block[ilogical_block].name);
		pb->logical_block = ilogical_block;
		logical_block[ilogical_block].clb_index = clb_index;
		logical_block[ilogical_block].pb = pb;

		add_atom_as_target(router_data, ilogical_block);
		if (!primitive_feasible(ilogical_block, pb)) {
			/* failed location feasibility check, revert pack */
			block_pack_status = BLK_FAILED_FEASIBLE;
		}

		if (block_pack_status == BLK_PASSED && is_root_of_chain == true) {
			/* is carry chain, must check if this carry chain spans multiple logic blocks or not */
			root_port = chain_root_pin->port->model_port;
			if(logical_block[ilogical_block].input_nets[root_port->index][chain_root_pin->pin_number] != OPEN) {
				/* this carry chain spans multiple logic blocks, must match up correctly with previous chain for this to route */
				if(pb_graph_node != chain_root_pin->parent_node) {
					/* this location does not match with the dedicated chain input from outside logic block, therefore not feasible */
					block_pack_status = BLK_FAILED_FEASIBLE;
				}
			}
		}
	}

	return block_pack_status;
}

/* Revert trial logical block iblock and free up memory space accordingly 
 */
static void revert_place_logical_block(const int iblock, const int max_models, t_lb_router_data *router_data) {
	t_pb *pb, *next;

	pb = logical_block[iblock].pb;
	logical_block[iblock].clb_index = NO_CLUSTER;
	logical_block[iblock].pb = NULL;

	if (pb != NULL) {
		/* When freeing molecules, the current block might already have been freed by a prior revert 
		 When this happens, no need to do anything beyond basic book keeping at the logical block
		 */

		next = pb->parent_pb;
		free_pb(pb);
		pb = next;

		while (pb != NULL) {
			/* If this is pb is created only for the purposes of holding new molecule, remove it
			 Must check if cluster is already freed (which can be the case)
			 */
			next = pb->parent_pb;
			
			if (pb->child_pbs != NULL && pb->pb_stats != NULL
					&& pb->pb_stats->num_child_blocks_in_pb == 0) {
				set_reset_pb_modes(router_data, pb, false);
				if (next != NULL) {
					/* If the code gets here, then that means that placing the initial seed molecule failed, don't free the actual complex block itself as the seed needs to find another placement */
					free_pb(pb);
				}
			}
			pb = next;
		}
	}
}

static void update_connection_gain_values(int inet, int clustered_block,
		t_pb *cur_pb,
		enum e_net_relation_to_clustered_block net_relation_to_clustered_block) {
	/*This function is called when the connectiongain values on the g_atoms_nlist.net*
	 *inet require updating.   */

	int iblk;
	unsigned ipin;
	int clb_index;
	int num_internal_connections, num_open_connections, num_stuck_connections;

	num_internal_connections = num_open_connections = num_stuck_connections = 0;

	clb_index = logical_block[clustered_block].clb_index;

	/* may wish to speed things up by ignoring clock nets since they are high fanout */

	for (ipin = 0; ipin < g_atoms_nlist.net[inet].pins.size(); ipin++) {
		iblk = g_atoms_nlist.net[inet].pins[ipin].block;
		if (logical_block[iblk].clb_index == clb_index
				&& is_logical_blk_in_pb(iblk,
						logical_block[clustered_block].pb)) {
			num_internal_connections++;
		} else if (logical_block[iblk].clb_index == OPEN) {
			num_open_connections++;
		} else {
			num_stuck_connections++;
		}
	}

	if (net_relation_to_clustered_block == OUTPUT) {
		for (ipin = 1; ipin < g_atoms_nlist.net[inet].pins.size(); ipin++) {
			iblk = g_atoms_nlist.net[inet].pins[ipin].block;
			if (logical_block[iblk].clb_index == NO_CLUSTER) {
				/* TODO: Gain function accurate only if net has one connection to block, TODO: Should we handle case where net has multi-connection to block? Gain computation is only off by a bit in this case */
				if(cur_pb->pb_stats->connectiongain.count(iblk) == 0) {
					cur_pb->pb_stats->connectiongain[iblk] = 0;
				}
				
				if (num_internal_connections > 1) {
					cur_pb->pb_stats->connectiongain[iblk] -= 1
							/ (float) (num_open_connections + 1.5 * num_stuck_connections + 1 + 0.1);
				}
				cur_pb->pb_stats->connectiongain[iblk] += 1
						/ (float) (num_open_connections + 1.5 * num_stuck_connections + 0.1);
			}
		}
	}

	if (net_relation_to_clustered_block == INPUT) {
		/*Calculate the connectiongain for the logical_block which is driving *
		 *the g_atoms_nlist.net that is an input to a logical_block in the cluster */

		iblk = g_atoms_nlist.net[inet].pins[0].block;
		if (logical_block[iblk].clb_index == NO_CLUSTER) {
			if(cur_pb->pb_stats->connectiongain.count(iblk) == 0) {
				cur_pb->pb_stats->connectiongain[iblk] = 0;
			}
			if (num_internal_connections > 1) {
				cur_pb->pb_stats->connectiongain[iblk] -= 1
						/ (float) (num_open_connections + 1.5 * num_stuck_connections + 0.1 + 1);
			}
			cur_pb->pb_stats->connectiongain[iblk] += 1
					/ (float) (num_open_connections + 1.5 * num_stuck_connections + 0.1);
		}
	}
}
/*****************************************/
static void update_timing_gain_values(int inet, int clustered_block,
		t_pb *cur_pb,
		enum e_net_relation_to_clustered_block net_relation_to_clustered_block, const t_slack * slacks) {

	/*This function is called when the timing_gain values on the g_atoms_nlist.net*
	 *inet requires updating.   */
	float timinggain;
	int newblk;
	int iblk;
	unsigned ipin, ifirst;

	/* Check if this g_atoms_nlist.net lists its driving logical_block twice.  If so, avoid  *
	 * double counting this logical_block by skipping the first (driving) pin. */
	if (net_output_feeds_driving_block_input[inet] == false)
		ifirst = 0;
	else
		ifirst = 1;

	if (net_relation_to_clustered_block == OUTPUT
			&& !g_atoms_nlist.net[inet].is_global) {
		for (ipin = ifirst; ipin < g_atoms_nlist.net[inet].pins.size(); ipin++) {
			iblk = g_atoms_nlist.net[inet].pins[ipin].block;
			if (logical_block[iblk].clb_index == NO_CLUSTER) {
#ifdef PATH_COUNTING
				/* Timing gain is a weighted sum of timing and path criticalities. */
				timinggain =	  TIMING_GAIN_PATH_WEIGHT  * slacks->path_criticality[inet][ipin] 
						   + (1 - TIMING_GAIN_PATH_WEIGHT) * slacks->timing_criticality[inet][ipin]; 
#else
				/* Timing gain is the timing criticality. */
				timinggain = slacks->timing_criticality[inet][ipin]; 
#endif
				if(cur_pb->pb_stats->timinggain.count(iblk) == 0) {
					cur_pb->pb_stats->timinggain[iblk] = 0;
				}
				if (timinggain > cur_pb->pb_stats->timinggain[iblk])
					cur_pb->pb_stats->timinggain[iblk] = timinggain;
			}
		}
	}

	if (net_relation_to_clustered_block == INPUT
			&& !g_atoms_nlist.net[inet].is_global) {
		/*Calculate the timing gain for the logical_block which is driving *
		 *the g_atoms_nlist.net that is an input to a logical_block in the cluster */
		newblk = g_atoms_nlist.net[inet].pins[0].block;
		if (logical_block[newblk].clb_index == NO_CLUSTER) {
			for (ipin = 1; ipin < g_atoms_nlist.net[inet].pins.size(); ipin++) {
#ifdef PATH_COUNTING
				/* Timing gain is a weighted sum of timing and path criticalities. */
				timinggain =	  TIMING_GAIN_PATH_WEIGHT  * slacks->path_criticality[inet][ipin] 
						   + (1 - TIMING_GAIN_PATH_WEIGHT) * slacks->timing_criticality[inet][ipin]; 
#else
				/* Timing gain is the timing criticality. */
				timinggain = slacks->timing_criticality[inet][ipin]; 
#endif
				if(cur_pb->pb_stats->timinggain.count(newblk) == 0) {
					cur_pb->pb_stats->timinggain[newblk] = 0;
				}
				if (timinggain > cur_pb->pb_stats->timinggain[newblk])
					cur_pb->pb_stats->timinggain[newblk] = timinggain;

			}
		}
	}
}
/*****************************************/
static void mark_and_update_partial_gain(int inet, enum e_gain_update gain_flag,
		int clustered_block, int port_on_clustered_block,
		int pin_on_clustered_block, bool timing_driven,
		bool connection_driven,
		enum e_net_relation_to_clustered_block net_relation_to_clustered_block, 
		const t_slack * slacks) {

	/* Updates the marked data structures, and if gain_flag is GAIN,  *
	 * the gain when a logic logical_block is added to a cluster.  The        *
	 * sharinggain is the number of inputs that a logical_block shares with   *
	 * blocks that are already in the cluster. Hillgain is the        *
	 * reduction in number of pins-required by adding a logical_block to the  *
	 * cluster. The timinggain is the criticality of the most critical*
	 * g_atoms_nlist.net between this logical_block and a logical_block in the cluster.             */

	int iblk, ifirst, stored_net;
	unsigned int ipin;
	t_pb *cur_pb;

	cur_pb = logical_block[clustered_block].pb->parent_pb;

	
	if (g_atoms_nlist.net[inet].num_sinks() > AAPACK_MAX_NET_SINKS_IGNORE) {
		/* Optimization: It can be too runtime costly for marking all sinks for a high fanout-net that probably has no hope of ever getting packed, thus ignore those high fanout nets */
		if(g_atoms_nlist.net[inet].is_global != true) {
			/* If no low/medium fanout nets, we may need to consider high fan-out nets for packing, so select one and store it */ 
			while(cur_pb->parent_pb != NULL) {
				cur_pb = cur_pb->parent_pb;
			}
			stored_net = cur_pb->pb_stats->tie_break_high_fanout_net;
			if(stored_net == OPEN || g_atoms_nlist.net[inet].num_sinks() < g_atoms_nlist.net[stored_net].num_sinks()) {
				cur_pb->pb_stats->tie_break_high_fanout_net = inet;
			}
		}
		return;
	}

	while (cur_pb) {
		/* Mark g_atoms_nlist.net as being visited, if necessary. */

		if (cur_pb->pb_stats->num_pins_of_net_in_pb.count(inet) == 0) {
			cur_pb->pb_stats->marked_nets[cur_pb->pb_stats->num_marked_nets] =
					inet;
			cur_pb->pb_stats->num_marked_nets++;
		}

		/* Update gains of affected blocks. */

		if (gain_flag == GAIN) {

			/* Check if this g_atoms_nlist.net lists its driving logical_block twice.  If so, avoid  *
			 * double counting this logical_block by skipping the first (driving) pin. */

			if (net_output_feeds_driving_block_input[inet] == 0)
				ifirst = 0;
			else
				ifirst = 1;

			if (cur_pb->pb_stats->num_pins_of_net_in_pb.count(inet) == 0) {
				for (ipin = ifirst; ipin < g_atoms_nlist.net[inet].pins.size(); ipin++) {
					iblk = g_atoms_nlist.net[inet].pins[ipin].block;
					if (logical_block[iblk].clb_index == NO_CLUSTER) {

						if (cur_pb->pb_stats->sharinggain.count(iblk) == 0) {
							cur_pb->pb_stats->marked_blocks[cur_pb->pb_stats->num_marked_blocks] =
									iblk;
							cur_pb->pb_stats->num_marked_blocks++;
							cur_pb->pb_stats->sharinggain[iblk] = 1;
							cur_pb->pb_stats->hillgain[iblk] = 1
									- num_ext_inputs_logical_block(iblk);
						} else {
							cur_pb->pb_stats->sharinggain[iblk]++;
							cur_pb->pb_stats->hillgain[iblk]++;
						}
					}
				}
			}

			if (connection_driven) {
				update_connection_gain_values(inet, clustered_block, cur_pb,
						net_relation_to_clustered_block);
			}

			if (timing_driven) {
				update_timing_gain_values(inet, clustered_block, cur_pb,
						net_relation_to_clustered_block, slacks);
			}
		}
		if(cur_pb->pb_stats->num_pins_of_net_in_pb.count(inet) == 0) {
			cur_pb->pb_stats->num_pins_of_net_in_pb[inet] = 0;
		}
		cur_pb->pb_stats->num_pins_of_net_in_pb[inet]++;
		cur_pb = cur_pb->parent_pb;
	}
}

/*****************************************/
static void update_total_gain(float alpha, float beta, bool timing_driven,
		bool connection_driven, bool global_clocks, t_pb *pb) {

	/*Updates the total  gain array to reflect the desired tradeoff between*
	 *input sharing (sharinggain) and path_length minimization (timinggain)*/

	int i, iblk, j, k;
	t_pb * cur_pb;
	int num_input_pins, num_output_pins;
	int num_used_input_pins, num_used_output_pins;
	t_model_ports *port;

	cur_pb = pb;
	while (cur_pb) {

		for (i = 0; i < cur_pb->pb_stats->num_marked_blocks; i++) {
			iblk = cur_pb->pb_stats->marked_blocks[i];

			if(cur_pb->pb_stats->connectiongain.count(iblk) == 0) {
				cur_pb->pb_stats->connectiongain[iblk] = 0;
			}
			if(cur_pb->pb_stats->sharinggain.count(iblk) == 0) {
				cur_pb->pb_stats->sharinggain[iblk] = 0;
			}

			/* Todo: This was used to explore different normalization options, can be made more efficient once we decide on which one to use*/
			num_input_pins = 0;
			port = logical_block[iblk].model->inputs;
			j = 0;
			num_used_input_pins = 0;
			while (port) {
				num_input_pins += port->size;
				if (!port->is_clock) {
					for (k = 0; k < port->size; k++) {
						if (logical_block[iblk].input_nets[j][k] != OPEN) {
							num_used_input_pins++;
						}
					}
					j++;
				}
				port = port->next;
			}
			if (num_input_pins == 0) {
				num_input_pins = 1;
			}

			num_used_output_pins = 0;
			j = 0;
			num_output_pins = 0;
			port = logical_block[iblk].model->outputs;
			while (port) {
				num_output_pins += port->size;
				for (k = 0; k < port->size; k++) {
					if (logical_block[iblk].output_nets[j][k] != OPEN) {
						num_used_output_pins++;
					}
				}
				port = port->next;
				j++;
			}
			/* end todo */

			/* Calculate area-only cost function */
			if (connection_driven) {
				/*try to absorb as many connections as possible*/
				/*cur_pb->pb_stats->gain[iblk] = ((1-beta)*(float)cur_pb->pb_stats->sharinggain[iblk] + beta*(float)cur_pb->pb_stats->connectiongain[iblk])/(num_input_pins + num_output_pins);*/
				cur_pb->pb_stats->gain[iblk] = ((1 - beta)
						* (float) cur_pb->pb_stats->sharinggain[iblk]
						+ beta * (float) cur_pb->pb_stats->connectiongain[iblk])
						/ (num_used_input_pins + num_used_output_pins);
			} else {
				/*cur_pb->pb_stats->gain[iblk] = ((float)cur_pb->pb_stats->sharinggain[iblk])/(num_input_pins + num_output_pins); */
				cur_pb->pb_stats->gain[iblk] =
						((float) cur_pb->pb_stats->sharinggain[iblk])
								/ (num_used_input_pins + num_used_output_pins);

			}

			/* Add in timing driven cost into cost function */
			if (timing_driven) {
				cur_pb->pb_stats->gain[iblk] = alpha
						* cur_pb->pb_stats->timinggain[iblk]
						+ (1.0 - alpha) * (float) cur_pb->pb_stats->gain[iblk];
			}
		}
		cur_pb = cur_pb->parent_pb;
	}
}

/*****************************************/
static void update_cluster_stats( const t_pack_molecule *molecule,
		const int clb_index, const bool *is_clock, const bool global_clocks,
		const float alpha, const float beta, const bool timing_driven,
		const bool connection_driven, const t_slack * slacks) {

	/* Updates cluster stats such as gain, used pins, and clock structures.  */

	int ipin, inet;
	int new_blk, molecule_size;
	int iblock;
	t_model_ports *port;
	t_pb *cur_pb, *cb;

	/* TODO: what a scary comment from Vaughn, we'll have to watch out for this causing problems */
	/* Output can be open so the check is necessary.  I don't change  *
	 * the gain for clock outputs when clocks are globally distributed  *
	 * because I assume there is no real need to pack similarly clocked *
	 * FFs together then.  Note that by updating the gain when the      *
	 * clock driver is placed in a cluster implies that the output of   *
	 * LUTs can be connected to clock inputs internally.  Probably not  *
	 * true, but it doesn't make much difference, since it will still   *
	 * make local routing of this clock very short, and none of my      *
	 * benchmarks actually generate local clocks (all come from pads).  */

	molecule_size = get_array_size_of_molecule(molecule);
	cb = NULL;

	for (iblock = 0; iblock < molecule_size; iblock++) {
		if (molecule->logical_block_ptrs[iblock] == NULL) {
			continue;
		}
		new_blk = molecule->logical_block_ptrs[iblock]->index;

		logical_block[new_blk].clb_index = clb_index;

		cur_pb = logical_block[new_blk].pb->parent_pb;
		while (cur_pb) {
			/* reset list of feasible blocks */
			cur_pb->pb_stats->num_feasible_blocks = NOT_VALID;
			cur_pb->pb_stats->num_child_blocks_in_pb++;
			if (cur_pb->parent_pb == NULL) {
				cb = cur_pb;
			}
			cur_pb = cur_pb->parent_pb;
		}

		port = logical_block[new_blk].model->outputs;
		while (port) {
			for (ipin = 0; ipin < port->size; ipin++) {
				inet = logical_block[new_blk].output_nets[port->index][ipin]; /* Output pin first. */
				if (inet != OPEN) {
					if (!is_clock[inet] || !global_clocks)
						mark_and_update_partial_gain(inet, GAIN, new_blk,
								port->index, ipin, timing_driven,
								connection_driven, OUTPUT, slacks);
					else
						mark_and_update_partial_gain(inet, NO_GAIN, new_blk,
								port->index, ipin, timing_driven,
								connection_driven, OUTPUT, slacks);
				}
			}
			port = port->next;
		}
		port = logical_block[new_blk].model->inputs;
		while (port) {
			if (port->is_clock) {
				port = port->next;
				continue;
			}
			for (ipin = 0; ipin < port->size; ipin++) { /*  VPACK_BLOCK input pins. */

				inet = logical_block[new_blk].input_nets[port->index][ipin];
				if (inet != OPEN) {
					mark_and_update_partial_gain(inet, GAIN, new_blk,
							port->index, ipin, timing_driven, connection_driven,
							INPUT, slacks);
				}
			}
			port = port->next;
		}

		/* Note:  The code below ONLY WORKS when nets that go to clock inputs *
		 * NEVER go to the input of a VPACK_COMB.  It doesn't really make electrical *
		 * sense for that to happen, and I check this in the check_clocks     *
		 * function.  Don't disable that sanity check.                        */
		inet = logical_block[new_blk].clock_net; /* Clock input pin. */
		if (inet != OPEN) {
			if (global_clocks)
				mark_and_update_partial_gain(inet, NO_GAIN, new_blk, 0, 0,
						timing_driven, connection_driven, INPUT, slacks);
			else
				mark_and_update_partial_gain(inet, GAIN, new_blk, 0, 0,
						timing_driven, connection_driven, INPUT, slacks);

		}

		update_total_gain(alpha, beta, timing_driven, connection_driven,
				global_clocks, logical_block[new_blk].pb->parent_pb);

		commit_lookahead_pins_used(cb);
	}

}

static void start_new_cluster(
		t_cluster_placement_stats *cluster_placement_stats,
		t_pb_graph_node **primitives_list, const t_arch * arch,
		t_block *new_cluster, const int clb_index,
		const t_pack_molecule *molecule, const float aspect,
		int *num_used_instances_type, int *num_instances_type,
		const int num_models, const int max_cluster_size,
		const int max_nets_in_pb_type, vector<t_lb_type_rr_node> *lb_type_rr_graphs, t_lb_router_data **router_data, const int detailed_routing_stage) {
	/* Given a starting seed block, start_new_cluster determines the next cluster type to use 
	 It expands the FPGA if it cannot find a legal cluster for the logical block
	 */
	int i, j;
	bool success;
	int count;

	VTR_ASSERT(new_cluster->name == NULL);
	/* Check if this cluster is really empty */

	/* Allocate a dummy initial cluster and load a logical block as a seed and check if it is legal */
	new_cluster->name = (char*) vtr::malloc(
			(strlen(molecule->logical_block_ptrs[molecule->root]->name) + 4) * sizeof(char));
	sprintf(new_cluster->name, "cb.%s",
			molecule->logical_block_ptrs[molecule->root]->name);
	new_cluster->nets = NULL;
	new_cluster->type = NULL;
	new_cluster->pb = NULL;
	new_cluster->x = UNDEFINED;
	new_cluster->y = UNDEFINED;
	new_cluster->z = UNDEFINED;

	if ((nx > 1) && (ny > 1)) {
		alloc_and_load_grid(num_instances_type);
		freeGrid();
	}

	success = false;
	while (!success) {
		count = 0;
		for (i = 0; i < num_types; i++) {
			if (num_used_instances_type[i] < num_instances_type[i]) {
				new_cluster->type = &type_descriptors[i];
				if (new_cluster->type == EMPTY_TYPE) {
					continue;
				}
				new_cluster->pb = (t_pb*)vtr::calloc(1, sizeof(t_pb));
				new_cluster->pb->pb_graph_node = new_cluster->type->pb_graph_head;
				alloc_and_load_pb_stats(new_cluster->pb, num_models, max_nets_in_pb_type);
				new_cluster->pb->parent_pb = NULL;

				*router_data = alloc_and_load_router_data(&lb_type_rr_graphs[i], &type_descriptors[i]);
				for (j = 0; j < new_cluster->type->pb_graph_head->pb_type->num_modes && !success; j++) {
					new_cluster->pb->mode = j;
					reset_cluster_placement_stats(&cluster_placement_stats[i]);
					set_mode_cluster_placement_stats(new_cluster->pb->pb_graph_node, j);
					success = (BLK_PASSED
							== try_pack_molecule(&cluster_placement_stats[i],
									molecule, primitives_list, new_cluster->pb,
									num_models, max_cluster_size, clb_index,
									max_nets_in_pb_type, detailed_routing_stage, *router_data));
				}
				if (success) {
					/* TODO: For now, just grab any working cluster, in the future, heuristic needed to grab best complex block based on supply and demand */
					break;
				} else {
					free_router_data(*router_data);
					free_pb_stats(new_cluster->pb);
					free(new_cluster->pb);
					*router_data = NULL;
				}
				count++;
			}
		}
		if (count == num_types - 1) {
			if (molecule->type == MOLECULE_FORCED_PACK) {
				vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
						"Can not find any logic block that can implement molecule.\n"
						"\tPattern %s %s\n", 
						molecule->pack_pattern->name,
						molecule->logical_block_ptrs[molecule->root]->name);
			} else {
				vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
						"Can not find any logic block that can implement molecule.\n"
						"\tAtom %s\n",
						molecule->logical_block_ptrs[molecule->root]->name);
			}
		}

		/* Expand FPGA size and recalculate number of available cluster types*/
		if (!success) {
			if (aspect >= 1.0) {
				ny++;
				nx = vtr::nint(ny * aspect);
			} else {
				nx++;
				ny = vtr::nint(nx / aspect);
			}
			vtr::printf_info("Not enough resources expand FPGA size to x = %d y = %d.\n",
					nx, ny);
			if ((nx > MAX_SHORT) || (ny > MAX_SHORT)) {
				vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
						"Circuit cannot pack into architecture, architecture size (nx = %d, ny = %d) exceeds packer range.\n",
						nx, ny);
			}
			alloc_and_load_grid(num_instances_type);
			freeGrid();
		}
	}
	num_used_instances_type[new_cluster->type->index]++;
}

/*
Get candidate molecule to pack into currently open cluster
Molecule selection priority:
	1. Find unpacked molecule based on criticality and strong connectedness (connected by low fanout nets) with current cluster
	2. Find unpacked molecule based on weak connectedness (connected by high fanout nets) with current cluster
	3. Find unpacked molecule based on transitive connections (eg. 2 hops away) with current cluster
*/
static t_pack_molecule *get_highest_gain_molecule(
		const enum e_packer_algorithm packer_algorithm, t_pb *cur_pb,
		const enum e_gain_type gain_mode,
		t_cluster_placement_stats *cluster_placement_stats_ptr,
		t_lb_net_stats *clb_inter_blk_nets,
		const int cluster_index) {

	/* This routine populates a list of feasible blocks outside the cluster then returns the best one for the list    *
	 * not currently in a cluster and satisfies the feasibility     *
	 * function passed in as is_feasible.  If there are no feasible *
	 * blocks it returns NO_CLUSTER.                                */

	int i, j, iblk, index, inet, count;
	bool success;
	vtr::t_linked_vptr *cur;

	t_pack_molecule *molecule;
	molecule = NULL;

	if (gain_mode == HILL_CLIMBING) {
		vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
				"Hill climbing not supported yet, error out.\n");
	}

	// 1. Find unpacked molecule based on criticality and strong connectedness (connected by low fanout nets) with current cluster
	if (cur_pb->pb_stats->num_feasible_blocks == NOT_VALID) {
		cur_pb->pb_stats->num_feasible_blocks = 0;
		cur_pb->pb_stats->explore_transitive_fanout = true; /* If no legal molecules found, enable exploration of molecules two hops away */
		for (i = 0; i < cur_pb->pb_stats->num_marked_blocks; i++) {
			iblk = cur_pb->pb_stats->marked_blocks[i];
			if (logical_block[iblk].clb_index == NO_CLUSTER) {
				cur = logical_block[iblk].packed_molecules;
				while (cur != NULL) {
					molecule = (t_pack_molecule *) cur->data_vptr;
					if (molecule->valid) {
						success = true;
						for (j = 0; j < get_array_size_of_molecule(molecule); j++) {
							if (molecule->logical_block_ptrs[j] != NULL) {
								VTR_ASSERT(molecule->logical_block_ptrs[j]->clb_index == NO_CLUSTER);
								iblk = molecule->logical_block_ptrs[j]->index;
								if (!exists_free_primitive_for_logical_block(
										cluster_placement_stats_ptr,
										iblk)) { /* TODO: debating whether to check if placement exists for molecule (more robust) or individual logical blocks (faster) */
									success = false;
									break;
								}
							}
						}
						if (success) {
							add_molecule_to_pb_stats_candidates(molecule,
									cur_pb->pb_stats->gain, cur_pb, AAPACK_MAX_FEASIBLE_BLOCK_ARRAY_SIZE);
						}
					}
					cur = cur->next;
				}
			}
		}
	}

	// 2. Find unpacked molecule based on weak connectedness (connected by high fanout nets) with current cluster
	if(cur_pb->pb_stats->num_feasible_blocks == 0 && cur_pb->pb_stats->tie_break_high_fanout_net != OPEN) {
		/* Because the packer ignores high fanout nets when marking what blocks to consider, use one of the ignored high fanout net to fill up lightly related blocks */
		reset_tried_but_unused_cluster_placements(cluster_placement_stats_ptr);
		inet = cur_pb->pb_stats->tie_break_high_fanout_net;
		count = 0;
		for (i = 0; i < (int) g_atoms_nlist.net[inet].pins.size() && count < AAPACK_MAX_HIGH_FANOUT_EXPLORE; i++) {
			iblk = g_atoms_nlist.net[inet].pins[i].block;
			if (logical_block[iblk].clb_index == NO_CLUSTER) {
				cur = logical_block[iblk].packed_molecules;
				while (cur != NULL) {
					molecule = (t_pack_molecule *) cur->data_vptr;
					if (molecule->valid) {
						success = true;
						for (j = 0; j < get_array_size_of_molecule(molecule); j++) {
							if (molecule->logical_block_ptrs[j] != NULL) {
								VTR_ASSERT(molecule->logical_block_ptrs[j]->clb_index == NO_CLUSTER);
								iblk = molecule->logical_block_ptrs[j]->index;
								if (!exists_free_primitive_for_logical_block(
										cluster_placement_stats_ptr,
										iblk)) { /* TODO: debating whether to check if placement exists for molecule (more robust) or individual logical blocks (faster) */
									success = false;
									break;
								}
							}
						}
						if (success) {
							add_molecule_to_pb_stats_candidates(molecule,
									cur_pb->pb_stats->gain, cur_pb, min(AAPACK_MAX_FEASIBLE_BLOCK_ARRAY_SIZE, AAPACK_MAX_HIGH_FANOUT_EXPLORE));
							count++;
						}
					}
					cur = cur->next;
				}
			}
		} 
		cur_pb->pb_stats->tie_break_high_fanout_net = OPEN; /* Mark off that this high fanout net has been considered */
	}

	// 3. Find unpacked molecule based on transitive connections (eg. 2 hops away) with current cluster
	if(cur_pb->pb_stats->num_feasible_blocks == 0 && 
	   cur_pb->pb_stats->tie_break_high_fanout_net == OPEN &&
	   cur_pb->pb_stats->explore_transitive_fanout == true
	   ) {
		if(cur_pb->pb_stats->transitive_fanout_candidates == NULL) {
			/* First time finding transitive fanout candidates therefore alloc and load them */
			cur_pb->pb_stats->transitive_fanout_candidates = new vector<t_pack_molecule *>;
			load_transitive_fanout_candidates(cluster_index,
											  cur_pb->pb_stats,
											  clb_inter_blk_nets);

			/* Only consider candidates that pass a very simple legality check */
			for(i = 0; i < (int) cur_pb->pb_stats->transitive_fanout_candidates->size(); i++) {
				molecule = (*cur_pb->pb_stats->transitive_fanout_candidates)[i];
				if (molecule->valid) {
					success = true;
					for (j = 0; j < get_array_size_of_molecule(molecule); j++) {
						if (molecule->logical_block_ptrs[j] != NULL) {
							VTR_ASSERT(molecule->logical_block_ptrs[j]->clb_index == NO_CLUSTER);
							iblk = molecule->logical_block_ptrs[j]->index;
							if (!exists_free_primitive_for_logical_block(
									cluster_placement_stats_ptr,
									iblk)) { /* TODO: debating whether to check if placement exists for molecule (more robust) or individual logical blocks (faster) */
								success = false;
								break;
							}
						}
					}
					if (success) {
						add_molecule_to_pb_stats_candidates(molecule,
								cur_pb->pb_stats->gain, cur_pb, min(AAPACK_MAX_FEASIBLE_BLOCK_ARRAY_SIZE,AAPACK_MAX_TRANSITIVE_EXPLORE));
					}
				}
			}
		} else {
			/* Clean up, no more candidates in transitive fanout to consider */
			delete cur_pb->pb_stats->transitive_fanout_candidates;
			cur_pb->pb_stats->transitive_fanout_candidates = NULL;
			cur_pb->pb_stats->explore_transitive_fanout = false;
		}
	}

	/* Grab highest gain molecule */
	molecule = NULL;
	for (j = 0; j < cur_pb->pb_stats->num_feasible_blocks; j++) {
		if (cur_pb->pb_stats->num_feasible_blocks != 0) {
			cur_pb->pb_stats->num_feasible_blocks--;
			index = cur_pb->pb_stats->num_feasible_blocks;
			molecule = cur_pb->pb_stats->feasible_blocks[index];
			VTR_ASSERT(molecule->valid == true);
			return molecule;
		}
	}

	return molecule;
}

/*****************************************/
static t_pack_molecule *get_molecule_for_cluster(
		const enum e_packer_algorithm packer_algorithm, t_pb *cur_pb,
		const bool allow_unrelated_clustering,
		int *num_unrelated_clustering_attempts,
		t_cluster_placement_stats *cluster_placement_stats_ptr,
		t_lb_net_stats *clb_inter_blk_nets,
		int cluster_index) {

	/* Finds the vpack block with the the greatest gain that satisifies the      *
	 * input, clock and capacity constraints of a cluster that are       *
	 * passed in.  If no suitable vpack block is found it returns NO_CLUSTER.   
	 */

	t_pack_molecule *best_molecule;

	/* If cannot pack into primitive, try packing into cluster */

	best_molecule = get_highest_gain_molecule(packer_algorithm, cur_pb,
			NOT_HILL_CLIMBING, cluster_placement_stats_ptr, clb_inter_blk_nets, cluster_index);

	/* If no blocks have any gain to the current cluster, the code above *
	 * will not find anything.  However, another logical_block with no inputs in *
	 * common with the cluster may still be inserted into the cluster.   */

	if (allow_unrelated_clustering) {
		if (best_molecule == NULL) {
			if (*num_unrelated_clustering_attempts == 0) {
				best_molecule =
						get_free_molecule_with_most_ext_inputs_for_cluster(
								packer_algorithm, cur_pb,
								cluster_placement_stats_ptr);
				(*num_unrelated_clustering_attempts)++;
			}
		} else {
			*num_unrelated_clustering_attempts = 0;
		}
	}

	return best_molecule;
}


/* TODO: Add more error checking, too light */
/*****************************************/
static void check_clustering(int num_clb, t_block *clb, bool *is_clock) {
	int i;
	t_pb * cur_pb;
	bool * blocks_checked;

	blocks_checked = (bool*)vtr::calloc(num_logical_blocks, sizeof(bool));

	/* 
	 * Check that each logical block connects to one primitive and that the primitive links up to the parent clb
	 */
	for (i = 0; i < num_blocks; i++) {
		if (logical_block[i].pb->logical_block != i) {
			vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
					"pb %s does not contain logical block %s but logical block %s #%d links to pb.\n",
					logical_block[i].pb->name, logical_block[i].name, logical_block[i].name, i);
		}
		cur_pb = logical_block[i].pb;
		VTR_ASSERT(strcmp(cur_pb->name, logical_block[i].name) == 0);
		while (cur_pb->parent_pb) {
			cur_pb = cur_pb->parent_pb;
			VTR_ASSERT(cur_pb->name);
		}
		if (cur_pb != clb[num_clb].pb) {
			vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
					"CLB %s does not match CLB contained by pb %s.\n",
					cur_pb->name, logical_block[i].pb->name);
		}
	}

	/* Check that I do not have spurious links in children pbs */
	for (i = 0; i < num_clb; i++) {
		check_cluster_logical_blocks(clb[i].pb, blocks_checked);
	}

	for (i = 0; i < num_logical_blocks; i++) {
		if (blocks_checked[i] == false) {
			vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
					"Logical block %s #%d not found in any cluster.\n",
					logical_block[i].name, i);
		}
	}

	free(blocks_checked);
}

/* TODO: May want to check that all logical blocks are actually reached (low priority, nice to have) */
static void check_cluster_logical_blocks(t_pb *pb, bool *blocks_checked) {
	int i, j;
	const t_pb_type *pb_type;
	bool has_child;

	has_child = false;
	pb_type = pb->pb_graph_node->pb_type;
	if (pb_type->num_modes == 0) {
		/* primitive */
		if (pb->logical_block != OPEN) {
			if (blocks_checked[pb->logical_block] != false) {
				vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
						"pb %s contains logical block %s #%d but logical block is already contained in another pb.\n",
						pb->name, logical_block[pb->logical_block].name, pb->logical_block);
			}
			blocks_checked[pb->logical_block] = true;
			if (pb != logical_block[pb->logical_block].pb) {
				vpr_throw(VPR_ERROR_PACK, __FILE__, __LINE__,
						"pb %s contains logical block %s #%d but logical block does not link to pb.\n",
						pb->name, logical_block[pb->logical_block].name, pb->logical_block);
			}
		}
	} else {
		/* this is a container pb, all container pbs must contain children */
		for (i = 0; i < pb_type->modes[pb->mode].num_pb_type_children; i++) {
			for (j = 0; j < pb_type->modes[pb->mode].pb_type_children[i].num_pb; j++) {
				if (pb->child_pbs[i] != NULL) {
					if (pb->child_pbs[i][j].name != NULL) {
						has_child = true;
						check_cluster_logical_blocks(&pb->child_pbs[i][j],
								blocks_checked);
					}
				}
			}
		}
		VTR_ASSERT(has_child);
	}
}

static t_pack_molecule* get_highest_gain_seed_molecule(int * seedindex, bool getblend) {
	/* Do_timing_analysis must be called before this, or this function 
	 * will return garbage. Returns molecule with most critical block;
	 * if block belongs to multiple molecules, return the biggest molecule. */

	int blkidx;
	t_pack_molecule *molecule, *best;
	vtr::t_linked_vptr *cur;

	while (*seedindex < num_logical_blocks) {

		if(getblend == true) {
			blkidx = seed_blend_index_array[(*seedindex)++];
		} else {
			blkidx = critindexarray[(*seedindex)++];
		}

		if (logical_block[blkidx].clb_index == NO_CLUSTER) {
			cur = logical_block[blkidx].packed_molecules;
			best = NULL;
			while (cur != NULL) {
				molecule = (t_pack_molecule *) cur->data_vptr;
				if (molecule->valid) {
					if (best == NULL
							|| (best->base_gain) < (molecule->base_gain)) {
						best = molecule;
					}
				}
				cur = cur->next;
			}
			VTR_ASSERT(best != NULL);
			return best;
		}
	}

	/*if it makes it to here , there are no more blocks available*/
	return NULL;
}

/* get gain of packing molecule into current cluster 
 gain is equal to total_block_gain + molecule_base_gain*some_factor - introduced_input_nets_of_unrelated_blocks_pulled_in_by_molecule*some_other_factor

 */
static float get_molecule_gain(t_pack_molecule *molecule, map<int, float> &blk_gain) {
	float gain;
	int i, ipin, iport, inet, iblk;
	int num_introduced_inputs_of_indirectly_related_block;
	t_model_ports *cur;

	gain = 0;
	num_introduced_inputs_of_indirectly_related_block = 0;
	for (i = 0; i < get_array_size_of_molecule(molecule); i++) {
		if (molecule->logical_block_ptrs[i] != NULL) {
			if(blk_gain.count(molecule->logical_block_ptrs[i]->index) > 0) {
				gain += blk_gain[molecule->logical_block_ptrs[i]->index];
			} else {
				/* This block has no connection with current cluster, penalize molecule for having this block 
				 */
				cur = molecule->logical_block_ptrs[i]->model->inputs;
				iport = 0;
				while (cur != NULL) {
					if (cur->is_clock != true) {
						for (ipin = 0; ipin < cur->size; ipin++) {
							inet = molecule->logical_block_ptrs[i]->input_nets[iport][ipin];
							if (inet != OPEN) {
								num_introduced_inputs_of_indirectly_related_block++;
								for (iblk = 0; iblk < get_array_size_of_molecule(molecule); iblk++) {
									if (molecule->logical_block_ptrs[iblk]
											!= NULL
												&& g_atoms_nlist.net[inet].pins[0].block
													== molecule->logical_block_ptrs[iblk]->index) {
										num_introduced_inputs_of_indirectly_related_block--;
										break;
									}
								}
							}
						}
						iport++;
					}
					cur = cur->next;
				}
			}
		}
	}

	gain += molecule->base_gain * 0.0001; /* Use base gain as tie breaker TODO: need to sweep this value and perhaps normalize */
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
 * Runtime is actually not that bad for this.  It's worst case O(k^2) where k is the number of pb_graph pins.  Can use hash tables or make incremental if becomes an issue.
 */
static void try_update_lookahead_pins_used(t_pb *cur_pb) {
	int i, j;
	const t_pb_type *pb_type = cur_pb->pb_graph_node->pb_type;

	if (pb_type->num_modes > 0 && cur_pb->name != NULL) {
		if (cur_pb->child_pbs != NULL) {
			for (i = 0; i < pb_type->modes[cur_pb->mode].num_pb_type_children;
					i++) {
				if (cur_pb->child_pbs[i] != NULL) {
					for (j = 0; j < pb_type->modes[cur_pb->mode].pb_type_children[i].num_pb; j++) {
						try_update_lookahead_pins_used(
								&cur_pb->child_pbs[i][j]);
					}
				}
			}
		}
	} else {
		if (pb_type->blif_model != NULL && cur_pb->logical_block != OPEN) {
			compute_and_mark_lookahead_pins_used(cur_pb->logical_block);
		}
	}
}

/* Resets nets used at different pin classes for determining pin feasibility */
static void reset_lookahead_pins_used(t_pb *cur_pb) {
	int i, j;
	const t_pb_type *pb_type = cur_pb->pb_graph_node->pb_type;
	if (cur_pb->pb_stats == NULL) {
		return; /* No pins used, no need to continue */
	}

	if (pb_type->num_modes > 0 && cur_pb->name != NULL) {
		for (i = 0; i < cur_pb->pb_graph_node->num_input_pin_class; i++) {
			cur_pb->pb_stats->lookahead_input_pins_used[i].clear();
		}

		for (i = 0; i < cur_pb->pb_graph_node->num_output_pin_class; i++) {
			cur_pb->pb_stats->lookahead_output_pins_used[i].clear();			
		}

		if (cur_pb->child_pbs != NULL) {
			for (i = 0; i < pb_type->modes[cur_pb->mode].num_pb_type_children; i++) {
				if (cur_pb->child_pbs[i] != NULL) {
					for (j = 0; j < pb_type->modes[cur_pb->mode].pb_type_children[i].num_pb; j++) {
						reset_lookahead_pins_used(&cur_pb->child_pbs[i][j]);
					}
				}
			}
		}
	}
}

/* Determine if pins of speculatively packed pb are legal */
static void compute_and_mark_lookahead_pins_used(int ilogical_block) {
	int i, j;
	t_pb *cur_pb;
	t_pb_graph_node *pb_graph_node;
	const t_pb_type *pb_type;
	t_port *prim_port;

	int input_port;
	int output_port;
	int clock_port;

	VTR_ASSERT(logical_block[ilogical_block].pb != NULL);

	cur_pb = logical_block[ilogical_block].pb;
	pb_graph_node = cur_pb->pb_graph_node;
	pb_type = pb_graph_node->pb_type;

	/* Walk through inputs, outputs, and clocks marking pins off of the same class */
	/* TODO: This is inelegant design, I should change the primitive ports in pb_type to be input, output, or clock instead of this lookup */
	input_port = output_port = clock_port = 0;
	for (i = 0; i < pb_type->num_ports; i++) {
		prim_port = &pb_type->ports[i];
		if (prim_port->is_clock) {
			VTR_ASSERT(prim_port->type == IN_PORT);
			VTR_ASSERT(prim_port->num_pins == 1 && clock_port == 0);
			/* currently support only one clock for primitives */
			if (logical_block[ilogical_block].clock_net != OPEN) {
				compute_and_mark_lookahead_pins_used_for_pin(
						&pb_graph_node->clock_pins[0][0], cur_pb,
						logical_block[ilogical_block].clock_net);
			}
			clock_port++;
		} else if (prim_port->type == IN_PORT) {
			for (j = 0; j < prim_port->num_pins; j++) {
				if (logical_block[ilogical_block].input_nets[prim_port->model_port->index][j]
						!= OPEN) {
					compute_and_mark_lookahead_pins_used_for_pin(
							&pb_graph_node->input_pins[input_port][j], cur_pb,
							logical_block[ilogical_block].input_nets[prim_port->model_port->index][j]);
				}
			}
			input_port++;
		} else if (prim_port->type == OUT_PORT) {
			for (j = 0; j < prim_port->num_pins; j++) {
				if (logical_block[ilogical_block].output_nets[prim_port->model_port->index][j]
						!= OPEN) {
					compute_and_mark_lookahead_pins_used_for_pin(
							&pb_graph_node->output_pins[output_port][j], cur_pb,
							logical_block[ilogical_block].output_nets[prim_port->model_port->index][j]);
				}
			}
			output_port++;
		} else {
			VTR_ASSERT(0);
		}
	}
}

/* Given a pin and its assigned net, mark all pin classes that are affected */
static void compute_and_mark_lookahead_pins_used_for_pin(
		t_pb_graph_pin *pb_graph_pin, t_pb *primitive_pb, int inet) {
	int depth, i;
	int pin_class, output_port;
	t_pb * cur_pb;
	t_pb * check_pb;
	const t_pb_type *pb_type;
	t_port *prim_port;
	t_pb_graph_pin *output_pb_graph_pin;
	int count;

	bool skip, found;

	cur_pb = primitive_pb->parent_pb;

	while (cur_pb) {
		depth = cur_pb->pb_graph_node->pb_type->depth;
		pin_class = pb_graph_pin->parent_pin_class[depth];
		VTR_ASSERT(pin_class != OPEN);

		if (pb_graph_pin->port->type == IN_PORT) {
			/* find location of net driver if exist in clb, NULL otherwise */
			output_pb_graph_pin = NULL;
			if (logical_block[g_atoms_nlist.net[inet].pins[0].block].clb_index
					== logical_block[primitive_pb->logical_block].clb_index) {
				pb_type =
					logical_block[g_atoms_nlist.net[inet].pins[0].block].pb->pb_graph_node->pb_type;
				output_port = 0;
				found = false;
				for (i = 0; i < pb_type->num_ports && !found; i++) {
					prim_port = &pb_type->ports[i];
					if (prim_port->type == OUT_PORT) {
						if (pb_type->ports[i].model_port->index
							== g_atoms_nlist.net[inet].pins[0].block_port) {
							found = true;
							break;
						}
						output_port++;
					}
				}
				VTR_ASSERT(found);
				output_pb_graph_pin =
					&(logical_block[g_atoms_nlist.net[inet].pins[0].block].pb->pb_graph_node->output_pins[output_port][g_atoms_nlist.net[inet].pins[0].block_pin]);
			}

			skip = false;

			/* check if driving pin for input is contained within the currently investigated cluster, if yes, do nothing since no input needs to be used */
			if (output_pb_graph_pin != NULL) {
				check_pb = logical_block[g_atoms_nlist.net[inet].pins[0].block].pb;
				while (check_pb != NULL && check_pb != cur_pb) {
					check_pb = check_pb->parent_pb;
				}
				if (check_pb != NULL) {
					for (i = 0; skip == false && i < output_pb_graph_pin->num_connectable_primtive_input_pins[depth]; i++) {
						if (pb_graph_pin
								== output_pb_graph_pin->list_of_connectable_input_pin_ptrs[depth][i]) {
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
				for (i = 0; i < la_inpins_size; i++) {
					if (cur_pb->pb_stats->lookahead_input_pins_used[pin_class][i]
							== inet) {
						skip = true;
					}
				}
				if (!skip) {
					/* Net must take up a slot */
					cur_pb->pb_stats->lookahead_input_pins_used[pin_class].push_back(inet);
				}
			}
		} else {
			VTR_ASSERT(pb_graph_pin->port->type == OUT_PORT);

			skip = false;
			if (pb_graph_pin->num_connectable_primtive_input_pins[depth]
					>= g_atoms_nlist.net[inet].num_sinks()) {
				/* Important: This runtime penalty looks a lot scarier than it really is.  For high fan-out nets, I at most look at the number of pins within the cluster which limits runtime.
				 DO NOT REMOVE THIS INITIAL FILTER WITHOUT CAREFUL ANALYSIS ON RUNTIME!!!
				 
				 Key Observation:
				 For LUT-based designs it is impossible for the average fanout to exceed the number of LUT inputs so it's usually around 4-5 (pigeon-hole argument, if the average fanout is greater than the 
				 number of LUT inputs, where do the extra connections go?  Therefore, average fanout must be capped to a small constant where the constant is equal to the number of LUT inputs).  The real danger to runtime
				 is when the number of sinks of a net gets doubled
				 
				 */
				int anet_size = (int) g_atoms_nlist.net[inet].pins.size();
				for (i = 1; i < anet_size; i++) {
					if (logical_block[g_atoms_nlist.net[inet].pins[i].block].clb_index
						!= logical_block[g_atoms_nlist.net[inet].pins[0].block].clb_index) {
						break;
					}
				}
				if (i == anet_size) {
					count = 0;
					/* TODO: I should cache the absorbed outputs, once net is absorbed, net is forever absorbed, no point in rechecking every time */
					for (i = 0; i < pb_graph_pin->num_connectable_primtive_input_pins[depth]; i++) {
						if (get_net_corresponding_to_pb_graph_pin(cur_pb,
								pb_graph_pin->list_of_connectable_input_pin_ptrs[depth][i])
								== inet) {
							count++;
						}
					}
					if (count == g_atoms_nlist.net[inet].num_sinks()) {
						skip = true;
					}
				}
			}

			if (!skip) {
				/* This output must exit this cluster */
				cur_pb->pb_stats->lookahead_output_pins_used[pin_class].push_back(inet);				
			}
		}

		cur_pb = cur_pb->parent_pb;
	}
}

/* Check if the number of available inputs/outputs for a pin class is sufficient for speculatively packed blocks */
static bool check_lookahead_pins_used(t_pb *cur_pb) {
	int i, j;
	const t_pb_type *pb_type = cur_pb->pb_graph_node->pb_type;
	bool success;

	success = true;

	if (pb_type->num_modes > 0 && cur_pb->name != NULL) {
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

		if (success && cur_pb->child_pbs != NULL) {
			for (i = 0; success && i < pb_type->modes[cur_pb->mode].num_pb_type_children; i++) {
				if (cur_pb->child_pbs[i] != NULL) {
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

	if (pb_type->num_modes > 0 && cur_pb->name != NULL) {
		for (i = 0; i < cur_pb->pb_graph_node->num_input_pin_class; i++) {
			ipin = 0;
			VTR_ASSERT(cur_pb->pb_stats->lookahead_input_pins_used[i].size() <= (unsigned int)cur_pb->pb_graph_node->input_pin_class_size[i]);
			for (j = 0; j < (int) cur_pb->pb_stats->lookahead_input_pins_used[i].size(); j++) {
				VTR_ASSERT(cur_pb->pb_stats->lookahead_input_pins_used[i][j] != OPEN);
				cur_pb->pb_stats->input_pins_used[i][ipin] =
						cur_pb->pb_stats->lookahead_input_pins_used[i][j];
				ipin++;
			}
		}

		for (i = 0; i < cur_pb->pb_graph_node->num_output_pin_class; i++) {
			ipin = 0;
			VTR_ASSERT(cur_pb->pb_stats->lookahead_output_pins_used[i].size() <= (unsigned int)cur_pb->pb_graph_node->output_pin_class_size[i]);
			for (j = 0; j < (int) cur_pb->pb_stats->lookahead_output_pins_used[i].size(); j++) {
				VTR_ASSERT(cur_pb->pb_stats->lookahead_output_pins_used[i][j] != OPEN);
				cur_pb->pb_stats->output_pins_used[i][ipin] =
						cur_pb->pb_stats->lookahead_output_pins_used[i][j];
				ipin++;
			}
		}

		if (cur_pb->child_pbs != NULL) {
			for (i = 0; i < pb_type->modes[cur_pb->mode].num_pb_type_children;
					i++) {
				if (cur_pb->child_pbs[i] != NULL) {
					for (j = 0; j < pb_type->modes[cur_pb->mode].pb_type_children[i].num_pb; j++) {
						commit_lookahead_pins_used(&cur_pb->child_pbs[i][j]);
					}
				}
			}
		}
	}
}

/* determine net at given pin location for cluster, return OPEN if none exists */
static int get_net_corresponding_to_pb_graph_pin(t_pb *cur_pb,
		t_pb_graph_pin *pb_graph_pin) {
	t_pb_graph_node *pb_graph_node;
	int i;
	t_model_ports *model_port;
	int ilogical_block;

	if (cur_pb->name == NULL) {
		return OPEN;
	}
	if (cur_pb->pb_graph_node->pb_type->num_modes != 0) {
		pb_graph_node = pb_graph_pin->parent_node;
		while (pb_graph_node->parent_pb_graph_node->pb_type->depth
				> cur_pb->pb_graph_node->pb_type->depth) {
			pb_graph_node = pb_graph_node->parent_pb_graph_node;
		}
		if (pb_graph_node->parent_pb_graph_node == cur_pb->pb_graph_node) {
			if (cur_pb->mode != pb_graph_node->pb_type->parent_mode->index) {
				return OPEN;
			}
			for (i = 0; i < cur_pb->pb_graph_node->pb_type->modes[cur_pb->mode].num_pb_type_children; i++) {
				if (pb_graph_node
						== &cur_pb->pb_graph_node->child_pb_graph_nodes[cur_pb->mode][i][pb_graph_node->placement_index]) {
					break;
				}
			}
			VTR_ASSERT(i < cur_pb->pb_graph_node->pb_type->modes[cur_pb->mode].num_pb_type_children);
			return get_net_corresponding_to_pb_graph_pin(
					&cur_pb->child_pbs[i][pb_graph_node->placement_index],
					pb_graph_pin);
		} else {
			return OPEN;
		}
	} else {
		ilogical_block = cur_pb->logical_block;
		if (ilogical_block == OPEN) {
			return OPEN;
		} else {
			model_port = pb_graph_pin->port->model_port;
			if (model_port->is_clock) {
				VTR_ASSERT(model_port->dir == IN_PORT);
				return logical_block[ilogical_block].clock_net;
			} else if (model_port->dir == IN_PORT) {
				return logical_block[ilogical_block].input_nets[model_port->index][pb_graph_pin->pin_number];
			} else {
				VTR_ASSERT(model_port->dir == OUT_PORT);
				return logical_block[ilogical_block].output_nets[model_port->index][pb_graph_pin->pin_number];
			}
		}
	}
}

/* Score unclustered atoms that are two hops away from current cluster */
static void load_transitive_fanout_candidates(int cluster_index,
											  t_pb_stats *pb_stats,
											  t_lb_net_stats *clb_inter_blk_nets) {
	for(int mnet = 0; mnet < pb_stats->num_marked_nets; mnet++) {
		int inet = pb_stats->marked_nets[mnet];
		if(g_atoms_nlist.net[inet].pins.size() < AAPACK_MAX_TRANSITIVE_FANOUT_EXPLORE + 1) {
			for(unsigned int ipin = 0; ipin < g_atoms_nlist.net[inet].pins.size(); ipin++) {
				int iatom = g_atoms_nlist.net[inet].pins[ipin].block;
				int tclb = logical_block[iatom].clb_index;
				if(tclb != cluster_index && tclb != NO_CLUSTER) {
					/* Explore transitive connections from already packed cluster */
					for(int itnet = 0; itnet < clb_inter_blk_nets[tclb].num_nets_in_lb; itnet++) {
						int tnet = clb_inter_blk_nets[tclb].nets_in_lb[itnet];
						for(unsigned int tpin = 0; tpin < g_atoms_nlist.net[tnet].pins.size(); tpin++) {
							int tatom = g_atoms_nlist.net[tnet].pins[tpin].block;
							if(logical_block[tatom].clb_index == NO_CLUSTER) {
								/* This transitive atom is not packed, score and add */
								std::vector<t_pack_molecule *> &transitive_fanout_candidates = *(pb_stats->transitive_fanout_candidates);

								if(pb_stats->gain.count(tatom) == 0) {
									pb_stats->gain[tatom] = 0.001;									
								} else {
									pb_stats->gain[tatom] += 0.001;									
								}
								vtr::t_linked_vptr *cur;
								cur = logical_block[tatom].packed_molecules;
								while (cur != NULL) {
									t_pack_molecule *molecule = (t_pack_molecule *) cur->data_vptr;
									if (molecule->valid) {
										unsigned int imol = 0;

										/* The number of potential molecules is heavily bounded so this O(N) operation should be safe since N is small */
										for(imol = 0; imol < transitive_fanout_candidates.size(); imol++) {
											if(molecule == transitive_fanout_candidates[imol]) {
												break;
											}
										}
										if(imol == transitive_fanout_candidates.size()) {
											/* not in candidate list, add to list */
											transitive_fanout_candidates.push_back(molecule);
										}
									}
									cur = cur->next;
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
	/* Prints criticality and critindexarray for each logical block to a file. */
	
	int iblock, len;
	FILE * fp;
	char * name;

	fp = vtr::fopen(fname, "w");
	fprintf(fp, "Index \tLogical block name \tCriticality \tCritindexarray\n\n");
	for (iblock = 0; iblock < num_logical_blocks; iblock++) {
		name = logical_block[iblock].name;
		len = strlen(name);
		fprintf(fp, "%d\t%s\t", logical_block[iblock].index, name);
		if (len < 8) {
			fprintf(fp, "\t\t");
		} else if (len < 16) {
			fprintf(fp, "\t");
		}
		fprintf(fp, "%f\t%d\n", block_criticality[iblock], critindexarray[iblock]);
	}
	fclose(fp);
}



