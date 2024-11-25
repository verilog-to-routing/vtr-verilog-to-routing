/**
 * @file
 * @author  Vaughn Betz (first revision - VPack),
 *          Alexander Marquardt (second revision - T-VPack),
 *          Jason Luu (third revision - AAPack),
 *          Alex Singer (fourth revision - APPack)
 * @date    June 8, 2011
 * @brief   Main clustering algorithm
 *
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
 *  t_pb_type and t_pb_graph_node (and related types) describe the targeted FPGA architecture, while t_pb represents
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

#include "greedy_clusterer.h"
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include "SetupGrid.h"
#include "atom_netlist.h"
#include "attraction_groups.h"
#include "cluster_legalizer.h"
#include "cluster_util.h"
#include "constraints_report.h"
#include "greedy_seed_selector.h"
#include "pack_types.h"
#include "physical_types.h"
#include "prepack.h"
#include "vpr_context.h"
#include "vtr_math.h"
#include "vtr_vector.h"

namespace {

/**
 * @brief Struct to hold statistics on the progress of clustering.
 */
struct t_cluster_progress_stats {
    // The total number of molecules in the design.
    int num_molecules = 0;
    // The number of molecules which have been clustered.
    int num_molecules_processed = 0;
    // The number of molecules clustered since the last time the status was
    // logged.
    int mols_since_last_print = 0;
};

} // namespace

GreedyClusterer::GreedyClusterer(const t_packer_opts& packer_opts,
                                 const t_analysis_opts& analysis_opts,
                                 const AtomNetlist& atom_netlist,
                                 const t_arch& arch,
                                 const t_pack_high_fanout_thresholds& high_fanout_thresholds,
                                 const std::unordered_set<AtomNetId>& is_clock,
                                 const std::unordered_set<AtomNetId>& is_global)
        : packer_opts_(packer_opts),
          analysis_opts_(analysis_opts),
          atom_netlist_(atom_netlist),
          arch_(arch),
          high_fanout_thresholds_(high_fanout_thresholds),
          is_clock_(is_clock),
          is_global_(is_global),
          primitive_candidate_block_types_(identify_primitive_candidate_block_types()),
          log_verbosity_(packer_opts.pack_verbosity),
          net_output_feeds_driving_block_input_(identify_net_output_feeds_driving_block_input(atom_netlist)) {

}

std::map<t_logical_block_type_ptr, size_t>
GreedyClusterer::do_clustering(ClusterLegalizer& cluster_legalizer,
                               Prepacker& prepacker,
                               bool allow_unrelated_clustering,
                               bool balance_block_type_utilization,
                               AttractionInfo& attraction_groups,
                               DeviceContext& mutable_device_ctx) {
    // This routine returns a map that details the number of used block type
    // instances.
    std::map<t_logical_block_type_ptr, size_t> num_used_type_instances;

    /****************************************************************
     * Initialization
     *****************************************************************/

    // The clustering stats holds information used for logging the progress
    // of the clustering to the user.
    t_cluster_progress_stats clustering_stats;
    clustering_stats.num_molecules = prepacker.get_num_molecules();

    // TODO: Create a ClusteringTimingManager class.
    //       This code relies on the prepacker, once the prepacker is moved to
    //       the constructor, this code can also move to the constructor.
    std::shared_ptr<PreClusterDelayCalculator> clustering_delay_calc;
    std::shared_ptr<SetupTimingInfo> timing_info;
    // Default criticalities set to zero (e.g. if not timing driven)
    vtr::vector<AtomBlockId, float> atom_criticality(atom_netlist_.blocks().size(), 0.f);
    if (packer_opts_.timing_driven) {
        calc_init_packing_timing(packer_opts_, analysis_opts_, prepacker,
                                 clustering_delay_calc, timing_info, atom_criticality);
    }

    // Calculate the max molecule stats, which is used for gain calculation.
    const t_molecule_stats max_molecule_stats = prepacker.calc_max_molecule_stats(atom_netlist_);

    // Initialize the information for the greedy candidate selector.
    // TODO: Abstract into a candidate selector class.
    /* TODO: This is memory inefficient, fix if causes problems */
    /* Store stats on nets used by packed block, useful for determining transitively connected blocks
     * (eg. [A1, A2, ..]->[B1, B2, ..]->C implies cluster [A1, A2, ...] and C have a weak link) */
    vtr::vector<LegalizationClusterId, std::vector<AtomNetId>> clb_inter_blk_nets(atom_netlist_.blocks().size());
    // FIXME: This should be abstracted into a selector class. This is only used
    //        for gain calculation and selecting candidate molecules.
    t_clustering_data clustering_data;
    alloc_and_init_clustering(max_molecule_stats,
                              prepacker,
                              clustering_data,
                              clustering_stats.num_molecules);

    // Create the greedy seed selector.
    GreedySeedSelector seed_selector(atom_netlist_,
                                     prepacker,
                                     packer_opts_.cluster_seed_type,
                                     max_molecule_stats,
                                     atom_criticality);

    // Pick the first seed molecule.
    t_pack_molecule* seed_mol = seed_selector.get_next_seed(prepacker,
                                                            cluster_legalizer);

    /****************************************************************
     * Clustering
     *****************************************************************/

    print_pack_status_header();

    // Continue clustering as long as a valid seed is returned from the seed
    // selector.
    while (seed_mol != nullptr) {
        // Check to ensure that this molecule is unclustered.
        VTR_ASSERT(!cluster_legalizer.is_mol_clustered(seed_mol));

        // The basic algorithm:
        // 1) Try to put all the molecules in that you can without doing the
        //    full intra-lb route. Then do full legalization at the end.
        // 2) If the legalization at the end fails, try again, but this time
        //    do full legalization for each molecule added to the cluster.

        // Try to grow a cluster from the seed molecule without doing intra-lb
        // route for each molecule (i.e. just use faster but not fully
        // conservative legality checks).
        LegalizationClusterId new_cluster_id = try_grow_cluster(seed_mol,
                                        ClusterLegalizationStrategy::SKIP_INTRA_LB_ROUTE,
                                        cluster_legalizer,
                                        prepacker,
                                        allow_unrelated_clustering,
                                        balance_block_type_utilization,
                                        *timing_info,
                                        clb_inter_blk_nets,
                                        clustering_data,
                                        attraction_groups,
                                        num_used_type_instances,
                                        mutable_device_ctx);

        if (!new_cluster_id.is_valid()) {
            // If the previous strategy failed, try to grow the cluster again,
            // but this time perform full legalization for each molecule added
            // to the cluster.
            new_cluster_id = try_grow_cluster(seed_mol,
                                       ClusterLegalizationStrategy::FULL,
                                       cluster_legalizer,
                                       prepacker,
                                       allow_unrelated_clustering,
                                       balance_block_type_utilization,
                                       *timing_info,
                                       clb_inter_blk_nets,
                                       clustering_data,
                                       attraction_groups,
                                       num_used_type_instances,
                                       mutable_device_ctx);
        }

        // Ensure that at the seed was packed successfully.
        VTR_ASSERT(new_cluster_id.is_valid());
        VTR_ASSERT(cluster_legalizer.is_mol_clustered(seed_mol));

        // Update the clustering progress stats.
        size_t num_molecules_in_cluster = cluster_legalizer.get_num_molecules_in_cluster(new_cluster_id);
        clustering_stats.num_molecules_processed += num_molecules_in_cluster;
        clustering_stats.mols_since_last_print += num_molecules_in_cluster;

        // Print the current progress of the packing after a cluster has been
        // successfully created.
        print_pack_status(clustering_stats.num_molecules,
                          clustering_stats.num_molecules_processed,
                          clustering_stats.mols_since_last_print,
                          mutable_device_ctx.grid.width(),
                          mutable_device_ctx.grid.height(),
                          attraction_groups,
                          cluster_legalizer);

        // Pick new seed.
        seed_mol = seed_selector.get_next_seed(prepacker,
                                               cluster_legalizer);
    }

    // If this architecture has LE physical block, report its usage.
    report_le_physical_block_usage(cluster_legalizer);

    // Free the clustering data.
    // FIXME: This struct should use standard data structures so it does not
    //        have to be freed like this. This is also specific to the candidate
    //        gain calculation.
    free_clustering_data(clustering_data);

    return num_used_type_instances;
}

LegalizationClusterId GreedyClusterer::try_grow_cluster(
                                       t_pack_molecule* seed_mol,
                                       ClusterLegalizationStrategy strategy,
                                       ClusterLegalizer& cluster_legalizer,
                                       Prepacker& prepacker,
                                       bool allow_unrelated_clustering,
                                       bool balance_block_type_utilization,
                                       SetupTimingInfo& timing_info,
                                       vtr::vector<LegalizationClusterId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                                       t_clustering_data& clustering_data,
                                       AttractionInfo& attraction_groups,
                                       std::map<t_logical_block_type_ptr, size_t>& num_used_type_instances,
                                       DeviceContext& mutable_device_ctx) {

    // Check to ensure that this molecule is unclustered.
    VTR_ASSERT(!cluster_legalizer.is_mol_clustered(seed_mol));

    // Set the legalization strategy of the cluster legalizer.
    cluster_legalizer.set_legalization_strategy(strategy);

    // Use the seed to start a new cluster.
    LegalizationClusterId legalization_cluster_id = start_new_cluster(seed_mol,
                                                                      cluster_legalizer,
                                                                      balance_block_type_utilization,
                                                                      num_used_type_instances,
                                                                      mutable_device_ctx);

    int high_fanout_threshold = high_fanout_thresholds_.get_threshold(cluster_legalizer.get_cluster_type(legalization_cluster_id)->name);
    update_cluster_stats(seed_mol,
                         cluster_legalizer,
                         is_clock_,  //Set of clock nets
                         is_global_, //Set of global nets (currently all clocks)
                         packer_opts_.global_clocks,
                         packer_opts_.alpha, packer_opts_.beta,
                         packer_opts_.timing_driven, packer_opts_.connection_driven,
                         high_fanout_threshold,
                         timing_info,
                         attraction_groups,
                         net_output_feeds_driving_block_input_);

    int num_unrelated_clustering_attempts = 0;
    t_pack_molecule *candidate_mol;
    candidate_mol = get_molecule_for_cluster(cluster_legalizer.get_cluster_pb(legalization_cluster_id),
                                             attraction_groups,
                                             allow_unrelated_clustering,
                                             packer_opts_.prioritize_transitive_connectivity,
                                             packer_opts_.transitive_fanout_threshold,
                                             packer_opts_.feasible_block_array_size,
                                             &num_unrelated_clustering_attempts,
                                             prepacker,
                                             cluster_legalizer,
                                             clb_inter_blk_nets,
                                             legalization_cluster_id,
                                             log_verbosity_,
                                             clustering_data.unclustered_list_head,
                                             clustering_data.unclustered_list_head_size,
                                             primitive_candidate_block_types_);

    /*
     * When attraction groups are created, the purpose is to pack more densely by adding more molecules
     * from the cluster's attraction group to the cluster. In a normal flow, (when attraction groups are
     * not on), the cluster keeps being packed until the get_molecule routines return either a repeated
     * molecule or a nullptr. When attraction groups are on, we want to keep exploring molecules for the
     * cluster until a nullptr is returned. So, the number of repeated molecules allowed is increased to a
     * large value.
     */
    int max_num_repeated_molecules = 1;
    if (attraction_groups.num_attraction_groups() > 0)
        max_num_repeated_molecules = attraction_groups_max_repeated_molecules_;

    // Continuously try to cluster candidate molecules into the cluster
    // until one of the following occurs:
    //  1) No candidate molecule is proposed.
    //  2) The same candidate was proposed multiple times.
    int num_repeated_molecules = 0;
    while (candidate_mol != nullptr && num_repeated_molecules < max_num_repeated_molecules) {
        // Try to cluster the candidate molecule into the cluster.
        bool success = try_add_candidate_mol_to_cluster(candidate_mol,
                                                        legalization_cluster_id,
                                                        cluster_legalizer);

        // If the candidate molecule was clustered successfully, update
        // the cluster stats.
        if (success) {
            update_cluster_stats(candidate_mol,
                                 cluster_legalizer,
                                 is_clock_,  //Set of all clocks
                                 is_global_, //Set of all global signals (currently clocks)
                                 packer_opts_.global_clocks,
                                 packer_opts_.alpha,
                                 packer_opts_.beta,
                                 packer_opts_.timing_driven,
                                 packer_opts_.connection_driven,
                                 high_fanout_threshold,
                                 timing_info,
                                 attraction_groups,
                                 net_output_feeds_driving_block_input_);
            num_unrelated_clustering_attempts = 0;
        }

        // Get the next candidate molecule.
        t_pack_molecule* prev_candidate_mol = candidate_mol;
        candidate_mol = get_molecule_for_cluster(cluster_legalizer.get_cluster_pb(legalization_cluster_id),
                                                 attraction_groups,
                                                 allow_unrelated_clustering,
                                                 packer_opts_.prioritize_transitive_connectivity,
                                                 packer_opts_.transitive_fanout_threshold,
                                                 packer_opts_.feasible_block_array_size,
                                                 &num_unrelated_clustering_attempts,
                                                 prepacker,
                                                 cluster_legalizer,
                                                 clb_inter_blk_nets,
                                                 legalization_cluster_id,
                                                 log_verbosity_,
                                                 clustering_data.unclustered_list_head,
                                                 clustering_data.unclustered_list_head_size,
                                                 primitive_candidate_block_types_);

        // If the next candidate molecule is the same as the previous
        // candidate molecule, increment the number of repreated
        // molecules counter.
        if (candidate_mol == prev_candidate_mol)
            num_repeated_molecules++;
    }

    // Ensure that the cluster is legal. When the cluster legalization
    // strategy is full, it must be legal.
    if (strategy != ClusterLegalizationStrategy::FULL) {
        // If the legalizer did not check everything for every molecule,
        // need to check that the full cluster is legal (need to perform
        // intra-lb routing).
        bool is_cluster_legal = cluster_legalizer.check_cluster_legality(legalization_cluster_id);

        if (!is_cluster_legal) {
            // If the cluster is not legal, undo the cluster.
            // Update the used type instances.
            num_used_type_instances[cluster_legalizer.get_cluster_type(legalization_cluster_id)]--;
            // Destroy the illegal cluster.
            cluster_legalizer.destroy_cluster(legalization_cluster_id);
            cluster_legalizer.compress();
            // Cluster failed to grow.
            return LegalizationClusterId();
        }
    }

    VTR_ASSERT(legalization_cluster_id.is_valid());

    // Legal cluster was created. Store cluster info and clean cluster.

    // store info that will be used later in packing from pb_stats.
    // FIXME: If this is used for gain, it should be moved into the selector
    //        class. Perhaps a finalize_cluster_gain method.
    t_pb* cur_pb = cluster_legalizer.get_cluster_pb(legalization_cluster_id);
    t_pb_stats* pb_stats = cur_pb->pb_stats;
    for (const AtomNetId mnet_id : pb_stats->marked_nets) {
        int external_terminals = atom_netlist_.net_pins(mnet_id).size() - pb_stats->num_pins_of_net_in_pb[mnet_id];
        // Check if external terminals of net is within the fanout limit and
        // that there exists external terminals.
        if (external_terminals < packer_opts_.transitive_fanout_threshold && external_terminals > 0) {
            clb_inter_blk_nets[legalization_cluster_id].push_back(mnet_id);
        }
    }

    // Since the cluster will no longer be added to beyond this point,
    // clean the cluster of any data not strictly necessary for
    // creating the clustered netlist.
    cluster_legalizer.clean_cluster(legalization_cluster_id);

    // Cluster has been grown successfully.
    return legalization_cluster_id;
}

LegalizationClusterId GreedyClusterer::start_new_cluster(
            t_pack_molecule* seed_mol,
            ClusterLegalizer& cluster_legalizer,
            bool balance_block_type_utilization,
            std::map<t_logical_block_type_ptr, size_t>& num_used_type_instances,
            DeviceContext& mutable_device_ctx) {

    /* Allocate a dummy initial cluster and load a atom block as a seed and check if it is legal */
    AtomBlockId root_atom = seed_mol->atom_block_ids[seed_mol->root];
    const std::string& root_atom_name = atom_netlist_.block_name(root_atom);
    const t_model* root_model = atom_netlist_.block_model(root_atom);

    auto itr = primitive_candidate_block_types_.find(root_model);
    VTR_ASSERT(itr != primitive_candidate_block_types_.end());
    std::vector<t_logical_block_type_ptr> candidate_types = itr->second;

    if (balance_block_type_utilization) {
        //We sort the candidate types in ascending order by their current utilization.
        //This means that the packer will prefer to use types with lower utilization.
        //This is a naive approach to try balancing utilization when multiple types can
        //support the same primitive(s).
        std::stable_sort(candidate_types.begin(), candidate_types.end(),
                         [&](t_logical_block_type_ptr lhs, t_logical_block_type_ptr rhs) {
                             int lhs_num_instances = 0;
                             int rhs_num_instances = 0;
                             // Count number of instances for each type
                             for (auto type : lhs->equivalent_tiles)
                                 lhs_num_instances += mutable_device_ctx.grid.num_instances(type, -1);
                             for (auto type : rhs->equivalent_tiles)
                                 rhs_num_instances += mutable_device_ctx.grid.num_instances(type, -1);

                             float lhs_util = vtr::safe_ratio<float>(num_used_type_instances[lhs], lhs_num_instances);
                             float rhs_util = vtr::safe_ratio<float>(num_used_type_instances[rhs], rhs_num_instances);
                             //Lower util first
                             return lhs_util < rhs_util;
                         });
    }

    if (log_verbosity_ > 2) {
        VTR_LOG("\tSeed: '%s' (%s)", root_atom_name.c_str(), root_model->name);
        VTR_LOGV(seed_mol->pack_pattern, " molecule_type %s molecule_size %zu",
                 seed_mol->pack_pattern->name, seed_mol->atom_block_ids.size());
        VTR_LOG("\n");
    }

    //Try packing into each candidate type
    bool success = false;
    t_logical_block_type_ptr block_type;
    LegalizationClusterId new_cluster_id;
    for (auto type : candidate_types) {
        //Try packing into each mode
        e_block_pack_status pack_result = e_block_pack_status::BLK_STATUS_UNDEFINED;
        for (int j = 0; j < type->pb_graph_head->pb_type->num_modes && !success; j++) {
            std::tie(pack_result, new_cluster_id) = cluster_legalizer.start_new_cluster(seed_mol, type, j);
            success = (pack_result == e_block_pack_status::BLK_PASSED);
        }

        if (success) {
            VTR_LOGV(log_verbosity_ > 2, "\tPASSED_SEED: Block Type %s\n", type->name.c_str());
            // If clustering succeeds return the new_cluster_id and type.
            block_type = type;
            break;
        } else {
            VTR_LOGV(log_verbosity_ > 2, "\tFAILED_SEED: Block Type %s\n", type->name.c_str());
        }
    }

    if (!success) {
        //Explored all candidates
        if (seed_mol->type == MOLECULE_FORCED_PACK) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "Can not find any logic block that can implement molecule.\n"
                            "\tPattern %s %s\n",
                            seed_mol->pack_pattern->name,
                            root_atom_name.c_str());
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "Can not find any logic block that can implement molecule.\n"
                            "\tAtom %s (%s)\n",
                            root_atom_name.c_str(), root_model->name);
        }
    }

    VTR_ASSERT(success);
    VTR_ASSERT(new_cluster_id.is_valid());

    VTR_LOGV(log_verbosity_ > 2,
             "Complex block %zu: '%s' (%s) ", size_t(new_cluster_id),
             cluster_legalizer.get_cluster_pb(new_cluster_id)->name,
             cluster_legalizer.get_cluster_type(new_cluster_id)->name.c_str());
    VTR_LOGV(log_verbosity_ > 2, ".");
    //Progress dot for seed-block
    fflush(stdout);

    // TODO: Below may make more sense in its own method.

    // Successfully created cluster
    num_used_type_instances[block_type]++;

    /* Expand FPGA size if needed */
    // Check used type instances against the possible equivalent physical locations
    unsigned int num_instances = 0;
    for (auto equivalent_tile : block_type->equivalent_tiles) {
        num_instances += mutable_device_ctx.grid.num_instances(equivalent_tile, -1);
    }

    if (num_used_type_instances[block_type] > num_instances) {
        mutable_device_ctx.grid = create_device_grid(packer_opts_.device_layout,
                                                     arch_.grid_layouts,
                                                     num_used_type_instances,
                                                     packer_opts_.target_device_utilization);
    }

    return new_cluster_id;
}

bool GreedyClusterer::try_add_candidate_mol_to_cluster(t_pack_molecule* candidate_mol,
                                                       LegalizationClusterId legalization_cluster_id,
                                                       ClusterLegalizer& cluster_legalizer) {
    VTR_ASSERT(candidate_mol != nullptr);
    VTR_ASSERT(!cluster_legalizer.is_mol_clustered(candidate_mol));
    VTR_ASSERT(legalization_cluster_id.is_valid());

    e_block_pack_status pack_status = cluster_legalizer.add_mol_to_cluster(candidate_mol,
                                                                      legalization_cluster_id);

    // Print helpful debugging log messages.
    if (log_verbosity_ > 2) {
        switch (pack_status) {
            case e_block_pack_status::BLK_PASSED:
                VTR_LOG("\tPassed: ");
                break;
            case e_block_pack_status::BLK_FAILED_ROUTE:
                VTR_LOG("\tNO_ROUTE: ");
                break;
            case e_block_pack_status::BLK_FAILED_FLOORPLANNING:
                VTR_LOG("\tFAILED_FLOORPLANNING_CONSTRAINTS_CHECK: ");
                break;
            case e_block_pack_status::BLK_FAILED_FEASIBLE:
                VTR_LOG("\tFAILED_FEASIBILITY_CHECK: ");
                break;
            case e_block_pack_status::BLK_FAILED_NOC_GROUP:
                VTR_LOG("\tFAILED_NOC_GROUP_CHECK: ");
                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_PACK, "Unknown pack status thrown.");
                break;
        }
        // Get the block name and model name
        AtomBlockId blk_id = candidate_mol->atom_block_ids[candidate_mol->root];
        VTR_ASSERT(blk_id.is_valid());
        std::string blk_name = atom_netlist_.block_name(blk_id);
        const t_model* blk_model = atom_netlist_.block_model(blk_id);
        VTR_LOG("'%s' (%s)", blk_name.c_str(), blk_model->name);
        VTR_LOGV(candidate_mol->pack_pattern, " molecule %s molecule_size %zu",
                 candidate_mol->pack_pattern->name,
                 candidate_mol->atom_block_ids.size());
        VTR_LOG("\n");
        fflush(stdout);
    }

    return pack_status == e_block_pack_status::BLK_PASSED;
}

void GreedyClusterer::report_le_physical_block_usage(const ClusterLegalizer& cluster_legalizer) {
    // find the cluster type that has lut primitives
    auto logic_block_type = identify_logic_block_type(primitive_candidate_block_types_);
    // find a LE pb_type within the found logic_block_type
    auto le_pb_type = identify_le_block_type(logic_block_type);

    // If this architecture does not have an LE physical block, cannot report
    // its usage.
    if (le_pb_type == nullptr)
        return;

    // Track the number of Logic Elements (LEs) used. This is populated only for
    // architectures which has LEs. The architecture is assumed to have LEs iff
    // it has a logic block that contains LUT primitives and is the first
    // pb_block to have more than one instance from the top of the hierarchy
    // (All parent pb_block have one instance only and one mode only).

    // The number of LEs that are used for logic (LUTs/adders) only.
    int num_logic_le = 0;
    // The number of LEs that are used for registers only.
    int num_reg_le = 0;
    // The number of LEs that are used for both logic (LUTs/adders) and registers.
    int num_logic_and_reg_le = 0;

    for (LegalizationClusterId cluster_id : cluster_legalizer.clusters()) {
        // Update the data structure holding the LE counts
        update_le_count(cluster_legalizer.get_cluster_pb(cluster_id),
                        logic_block_type,
                        le_pb_type,
                        num_logic_le,
                        num_reg_le,
                        num_logic_and_reg_le);
    }

    // if this architecture has LE physical block, report its usage
    if (le_pb_type) {
        print_le_count(num_logic_le, num_reg_le, num_logic_and_reg_le, le_pb_type);
    }
}

