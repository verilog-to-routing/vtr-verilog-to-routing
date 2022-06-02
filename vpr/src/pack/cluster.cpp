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
#include "cluster_util.h"
#include "output_clustering.h"
#include "SetupGrid.h"
#include "read_xml_arch_file.h"
#include "vpr_utils.h"
#include "cluster_placement.h"
#include "echo_files.h"
#include "cluster_router.h"
#include "lb_type_rr_graph.h"

#include "timing_info.h"
#include "timing_reports.h"
#include "PreClusterDelayCalculator.h"
#include "PreClusterTimingGraphResolver.h"
#include "tatum/echo_writer.hpp"
#include "tatum/report/graphviz_dot_writer.hpp"
#include "tatum/TimingReporter.hpp"

#include "re_cluster_util.h"
#include "constraints_report.h"

/*
 * When attraction groups are created, the purpose is to pack more densely by adding more molecules
 * from the cluster's attraction group to the cluster. In a normal flow, (when attraction groups are
 * not on), the cluster keeps being packed until the get_molecule routines return either a repeated
 * molecule or a nullptr. When attraction groups are on, we want to keep exploring molecules for the
 * cluster until a nullptr is returned. So, the number of repeated molecules is changed from 1 to 500,
 * effectively making the clusterer pack a cluster until a nullptr is returned.
 */
#define ATTRACTION_GROUPS_MAX_REPEATED_MOLECULES 500

std::map<t_logical_block_type_ptr, size_t> do_clustering(const t_packer_opts& packer_opts,
                                                         const t_analysis_opts& analysis_opts,
                                                         const t_arch* arch,
                                                         t_pack_molecule* molecule_head,
                                                         int num_models,
                                                         const std::unordered_set<AtomNetId>& is_clock,
                                                         const std::unordered_map<AtomBlockId, t_pb_graph_node*>& expected_lowest_cost_pb_gnode,
                                                         bool allow_unrelated_clustering,
                                                         bool balance_block_type_utilization,
                                                         std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
                                                         const t_ext_pin_util_targets& ext_pin_util_targets,
                                                         const t_pack_high_fanout_thresholds& high_fanout_thresholds,
                                                         AttractionInfo& attraction_groups,
                                                         bool& floorplan_regions_overfull,
                                                         t_clustering_data& clustering_data) {
    /* Does the actual work of clustering multiple netlist blocks *
     * into clusters.                                                  */

    /* Algorithm employed
     * 1.  Find type that can legally hold block and create cluster with pb info
     * 2.  Populate started cluster
     * 3.  Repeat 1 until no more blocks need to be clustered
     *
     */

    /* This routine returns a map that details the number of used block type instances.
     * The bool floorplan_regions_overfull also acts as a return value - it is set to
     * true when one or more floorplan regions have more blocks assigned to them than
     * they can fit.
     */

    /****************************************************************
     * Initialization
     *****************************************************************/
    VTR_ASSERT(packer_opts.packer_algorithm == PACK_GREEDY);

    t_cluster_progress_stats cluster_stats;

    //int num_molecules, num_molecules_processed, mols_since_last_print, blocks_since_last_analysis,
    int num_blocks_hill_added, max_pb_depth,
        seedindex, savedseedindex /* index of next most timing critical block */,
        detailed_routing_stage;

    const int verbosity = packer_opts.pack_verbosity;

    int unclustered_list_head_size;
    std::unordered_map<AtomNetId, int> net_output_feeds_driving_block_input;

    cluster_stats.num_molecules_processed = 0;
    cluster_stats.mols_since_last_print = 0;

    std::map<t_logical_block_type_ptr, size_t> num_used_type_instances;

    bool is_cluster_legal;
    enum e_block_pack_status block_pack_status;

    t_cluster_placement_stats* cur_cluster_placement_stats_ptr;
    t_lb_router_data* router_data = nullptr;
    t_pack_molecule *istart, *next_molecule, *prev_molecule;

    auto& atom_ctx = g_vpr_ctx.atom();
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
    auto& helper_ctx = g_vpr_ctx.mutable_helper();

    helper_ctx.enable_pin_feasibility_filter = packer_opts.enable_pin_feasibility_filter;
    helper_ctx.feasible_block_array_size = packer_opts.feasible_block_array_size;

    std::shared_ptr<PreClusterDelayCalculator> clustering_delay_calc;
    std::shared_ptr<SetupTimingInfo> timing_info;

    // this data structure tracks the number of Logic Elements (LEs) used. It is
    // populated only for architectures which has LEs. The architecture is assumed
    // to have LEs only iff it has a logic block that contains LUT primitives and is
    // the first pb_block to have more than one instance from the top of the hierarchy
    // (All parent pb_block have one instance only and one mode only). Index 0 holds
    // the number of LEs that are used for both logic (LUTs/adders) and registers.
    // Index 1 holds the number of LEs that are used for logic (LUTs/adders) only.
    // Index 2 holds the number of LEs that are used for registers only.
    std::vector<int> le_count(3, 0);

    helper_ctx.total_clb_num = 0;

    /* TODO: This is memory inefficient, fix if causes problems */
    /* Store stats on nets used by packed block, useful for determining transitively connected blocks
     * (eg. [A1, A2, ..]->[B1, B2, ..]->C implies cluster [A1, A2, ...] and C have a weak link) */
    vtr::vector<ClusterBlockId, std::vector<AtomNetId>> clb_inter_blk_nets(atom_ctx.nlist.blocks().size());

    istart = nullptr;

    /* determine bound on cluster size and primitive input size */
    helper_ctx.max_cluster_size = 0;
    max_pb_depth = 0;

    seedindex = 0;

    const t_molecule_stats max_molecule_stats = calc_max_molecules_stats(molecule_head);

    mark_all_molecules_valid(molecule_head);

    cluster_stats.num_molecules = count_molecules(molecule_head);

    get_max_cluster_size_and_pb_depth(helper_ctx.max_cluster_size, max_pb_depth);

    if (packer_opts.hill_climbing_flag) {
        clustering_data.hill_climbing_inputs_avail = (int*)vtr::calloc(helper_ctx.max_cluster_size + 1,
                                                                       sizeof(int));
    } else {
        clustering_data.hill_climbing_inputs_avail = nullptr; /* if used, die hard */
    }

#if 0
	check_for_duplicate_inputs ();
#endif
    alloc_and_init_clustering(max_molecule_stats,
                              &(helper_ctx.cluster_placement_stats), &(helper_ctx.primitives_list), molecule_head,
                              clustering_data, net_output_feeds_driving_block_input,
                              unclustered_list_head_size, cluster_stats.num_molecules);

    auto primitive_candidate_block_types = identify_primitive_candidate_block_types();
    // find the cluster type that has lut primitives
    auto logic_block_type = identify_logic_block_type(primitive_candidate_block_types);
    // find a LE pb_type within the found logic_block_type
    auto le_pb_type = identify_le_block_type(logic_block_type);

    cluster_stats.blocks_since_last_analysis = 0;
    num_blocks_hill_added = 0;

    VTR_ASSERT(helper_ctx.max_cluster_size < MAX_SHORT);
    /* Limit maximum number of elements for each cluster */

    //Default criticalities set to zero (e.g. if not timing driven)
    vtr::vector<AtomBlockId, float> atom_criticality(atom_ctx.nlist.blocks().size(), 0.);

    if (packer_opts.timing_driven) {
        calc_init_packing_timing(packer_opts, analysis_opts, expected_lowest_cost_pb_gnode,
                                 clustering_delay_calc, timing_info, atom_criticality);
    }

    auto seed_atoms = initialize_seed_atoms(packer_opts.cluster_seed_type, max_molecule_stats, atom_criticality);

    istart = get_highest_gain_seed_molecule(&seedindex, seed_atoms);

    print_pack_status_header();

    /****************************************************************
     * Clustering
     *****************************************************************/

    while (istart != nullptr) {
        is_cluster_legal = false;
        savedseedindex = seedindex;
        for (detailed_routing_stage = (int)E_DETAILED_ROUTE_AT_END_ONLY; !is_cluster_legal && detailed_routing_stage != (int)E_DETAILED_ROUTE_INVALID; detailed_routing_stage++) {
            ClusterBlockId clb_index(helper_ctx.total_clb_num);

            VTR_LOGV(verbosity > 2, "Complex block %d:\n", helper_ctx.total_clb_num);

            /*Used to store cluster's PartitionRegion as primitives are added to it.
             * Since some of the primitives might fail legality, this structure temporarily
             * stores PartitionRegion information while the cluster is packed*/
            PartitionRegion temp_cluster_pr;

            start_new_cluster(helper_ctx.cluster_placement_stats, helper_ctx.primitives_list,
                              clb_index, istart,
                              num_used_type_instances,
                              packer_opts.target_device_utilization,
                              num_models, helper_ctx.max_cluster_size,
                              arch, packer_opts.device_layout,
                              lb_type_rr_graphs, &router_data,
                              detailed_routing_stage, &cluster_ctx.clb_nlist,
                              primitive_candidate_block_types,
                              verbosity,
                              packer_opts.enable_pin_feasibility_filter,
                              balance_block_type_utilization,
                              packer_opts.feasible_block_array_size,
                              temp_cluster_pr);

            //initial molecule in cluster has been processed
            cluster_stats.num_molecules_processed++;
            cluster_stats.mols_since_last_print++;
            print_pack_status(helper_ctx.total_clb_num,
                              cluster_stats.num_molecules,
                              cluster_stats.num_molecules_processed,
                              cluster_stats.mols_since_last_print,
                              device_ctx.grid.width(),
                              device_ctx.grid.height(),
                              attraction_groups);

            VTR_LOGV(verbosity > 2,
                     "Complex block %d: '%s' (%s) ", helper_ctx.total_clb_num,
                     cluster_ctx.clb_nlist.block_name(clb_index).c_str(),
                     cluster_ctx.clb_nlist.block_type(clb_index)->name);
            VTR_LOGV(verbosity > 2, ".");
            //Progress dot for seed-block
            fflush(stdout);

            t_ext_pin_util target_ext_pin_util = ext_pin_util_targets.get_pin_util(cluster_ctx.clb_nlist.block_type(clb_index)->name);
            int high_fanout_threshold = high_fanout_thresholds.get_threshold(cluster_ctx.clb_nlist.block_type(clb_index)->name);
            update_cluster_stats(istart, clb_index,
                                 is_clock, //Set of clock nets
                                 is_clock, //Set of global nets (currently all clocks)
                                 packer_opts.global_clocks,
                                 packer_opts.alpha, packer_opts.beta,
                                 packer_opts.timing_driven, packer_opts.connection_driven,
                                 high_fanout_threshold,
                                 *timing_info,
                                 attraction_groups,
                                 net_output_feeds_driving_block_input);
            helper_ctx.total_clb_num++;

            if (packer_opts.timing_driven) {
                cluster_stats.blocks_since_last_analysis++;
                /*it doesn't make sense to do a timing analysis here since there*
                 *is only one atom block clustered it would not change anything      */
            }
            cur_cluster_placement_stats_ptr = &(helper_ctx.cluster_placement_stats[cluster_ctx.clb_nlist.block_type(clb_index)->index]);
            cluster_stats.num_unrelated_clustering_attempts = 0;
            next_molecule = get_molecule_for_cluster(cluster_ctx.clb_nlist.block_pb(clb_index),
                                                     attraction_groups,
                                                     allow_unrelated_clustering,
                                                     packer_opts.prioritize_transitive_connectivity,
                                                     packer_opts.transitive_fanout_threshold,
                                                     packer_opts.feasible_block_array_size,
                                                     &cluster_stats.num_unrelated_clustering_attempts,
                                                     cur_cluster_placement_stats_ptr,
                                                     clb_inter_blk_nets,
                                                     clb_index,
                                                     verbosity,
                                                     clustering_data.unclustered_list_head,
                                                     unclustered_list_head_size,
                                                     primitive_candidate_block_types);
            prev_molecule = istart;

            /*
             * When attraction groups are created, the purpose is to pack more densely by adding more molecules
             * from the cluster's attraction group to the cluster. In a normal flow, (when attraction groups are
             * not on), the cluster keeps being packed until the get_molecule routines return either a repeated
             * molecule or a nullptr. When attraction groups are on, we want to keep exploring molecules for the
             * cluster until a nullptr is returned. So, the number of repeated molecules allowed is increased to a
             * large value.
             */
            int max_num_repeated_molecules = 0;
            if (attraction_groups.num_attraction_groups() > 0) {
                max_num_repeated_molecules = ATTRACTION_GROUPS_MAX_REPEATED_MOLECULES;
            } else {
                max_num_repeated_molecules = 1;
            }
            int num_repeated_molecules = 0;

            while (next_molecule != nullptr && num_repeated_molecules < max_num_repeated_molecules) {
                prev_molecule = next_molecule;

                try_fill_cluster(packer_opts,
                                 cur_cluster_placement_stats_ptr,
                                 prev_molecule,
                                 next_molecule,
                                 num_repeated_molecules,
                                 helper_ctx.primitives_list,
                                 cluster_stats,
                                 helper_ctx.total_clb_num,
                                 num_models,
                                 helper_ctx.max_cluster_size,
                                 clb_index,
                                 detailed_routing_stage,
                                 attraction_groups,
                                 clb_inter_blk_nets,
                                 allow_unrelated_clustering,
                                 high_fanout_threshold,
                                 is_clock,
                                 timing_info,
                                 router_data,
                                 target_ext_pin_util,
                                 temp_cluster_pr,
                                 block_pack_status,
                                 clustering_data.unclustered_list_head,
                                 unclustered_list_head_size,
                                 net_output_feeds_driving_block_input,
                                 primitive_candidate_block_types);
            }

            is_cluster_legal = check_cluster_legality(verbosity, detailed_routing_stage, router_data);

            if (is_cluster_legal) {
                istart = save_cluster_routing_and_pick_new_seed(packer_opts, helper_ctx.total_clb_num, seed_atoms, num_blocks_hill_added, clustering_data.intra_lb_routing, seedindex, cluster_stats, router_data);
                store_cluster_info_and_free(packer_opts, clb_index, logic_block_type, le_pb_type, le_count, clb_inter_blk_nets);
            } else {
                free_data_and_requeue_used_mols_if_illegal(clb_index, savedseedindex, num_used_type_instances, helper_ctx.total_clb_num, seedindex);
            }
            free_router_data(router_data);
            router_data = nullptr;
        }
    }

    // if this architecture has LE physical block, report its usage
    if (le_pb_type) {
        print_le_count(le_count, le_pb_type);
    }

    //check_floorplan_regions(floorplan_regions_overfull);
    floorplan_regions_overfull = floorplan_constraints_regions_overfull();

    return num_used_type_instances;
}

/**
 * Print the total number of used physical blocks for each pb type in the architecture
 */
void print_pb_type_count(const ClusteredNetlist& clb_nlist) {
    auto& device_ctx = g_vpr_ctx.device();

    std::map<t_pb_type*, int> pb_type_count;

    size_t max_depth = 0;
    for (ClusterBlockId blk : clb_nlist.blocks()) {
        size_t pb_max_depth = update_pb_type_count(clb_nlist.block_pb(blk), pb_type_count, 0);

        max_depth = std::max(max_depth, pb_max_depth);
    }

    size_t max_pb_type_name_chars = 0;
    for (auto& pb_type : pb_type_count) {
        max_pb_type_name_chars = std::max(max_pb_type_name_chars, strlen(pb_type.first->name));
    }

    VTR_LOG("\nPb types usage...\n");
    for (const auto& logical_block_type : device_ctx.logical_block_types) {
        if (!logical_block_type.pb_type) continue;

        print_pb_type_count_recurr(logical_block_type.pb_type, max_pb_type_name_chars + max_depth, 0, pb_type_count);
    }
    VTR_LOG("\n");
}
