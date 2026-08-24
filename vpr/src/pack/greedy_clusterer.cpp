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
#include <algorithm>
#include <array>
#include <cstdio>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>
#include "appack_context.h"
#include "setup_grid.h"
#include "atom_netlist.h"
#include "attraction_groups.h"
#include "cluster_legalizer.h"
#include "cluster_util.h"
#include "greedy_candidate_selector.h"
#include "greedy_seed_selector.h"
#include "globals.h"
#include "logic_types.h"
#include "physical_types.h"
#include "prepack.h"
#include "logical_ram_infer.h"
#include "vpr_context.h"
#include "vtr_math.h"

GreedyClusterer::GreedyClusterer(const t_packer_opts& packer_opts,
                                 const t_analysis_opts& analysis_opts,
                                 const AtomNetlist& atom_netlist,
                                 const t_arch& arch,
                                 const t_pack_high_fanout_thresholds& high_fanout_thresholds,
                                 const std::unordered_set<AtomNetId>& is_clock,
                                 const std::unordered_set<AtomNetId>& is_global,
                                 const PreClusterTimingManager& pre_cluster_timing_manager,
                                 const APPackContext& appack_ctx,
                                 const t_vpr_setup& vpr_setup)
    : packer_opts_(packer_opts)
    , vpr_setup_(vpr_setup)
    , analysis_opts_(analysis_opts)
    , atom_netlist_(atom_netlist)
    , arch_(arch)
    , high_fanout_thresholds_(high_fanout_thresholds)
    , is_clock_(is_clock)
    , is_global_(is_global)
    , pre_cluster_timing_manager_(pre_cluster_timing_manager)
    , appack_ctx_(appack_ctx)
    , primitive_candidate_block_types_(identify_primitive_candidate_block_types())
    , log_verbosity_(packer_opts.pack_verbosity)
    , net_output_feeds_driving_block_input_(identify_net_output_feeds_driving_block_input(atom_netlist)) {
}

std::map<t_logical_block_type_ptr, size_t>
GreedyClusterer::do_clustering(ClusterLegalizer& cluster_legalizer,
                               const Prepacker& prepacker,
                               const RamMapper& ram_mapper,
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
    clustering_stats.num_molecules = prepacker.molecules().size();

    // Check whether the RAM mapper has any groups to guard RAM-specific logic.
    has_ram_groups_ = ram_mapper.num_groups() > 0;

    // Calculate the max molecule stats, which is used for gain calculation.
    const t_molecule_stats max_molecule_stats = prepacker.calc_max_molecule_stats(atom_netlist_, arch_.models);

    // Create the greedy candidate selector. This will be used to select
    // candidate molecules to add to the clusters.
    GreedyCandidateSelector candidate_selector(atom_netlist_,
                                               prepacker,
                                               ram_mapper,
                                               packer_opts_,
                                               allow_unrelated_clustering,
                                               max_molecule_stats,
                                               primitive_candidate_block_types_,
                                               high_fanout_thresholds_,
                                               is_clock_,
                                               is_global_,
                                               net_output_feeds_driving_block_input_,
                                               pre_cluster_timing_manager_,
                                               appack_ctx_,
                                               arch_.models,
                                               log_verbosity_);

    // Create the greedy seed selector.
    GreedySeedSelector seed_selector(atom_netlist_,
                                     prepacker,
                                     packer_opts_.cluster_seed_type,
                                     max_molecule_stats,
                                     arch_.models,
                                     pre_cluster_timing_manager_,
                                     ram_mapper);

    /****************************************************************
     * Clustering
     *****************************************************************/

    print_pack_status_header();

    // Pack each relative placement group into its own cluster first, so the
    // group clusters are constructed from the constraints before any
    // unconstrained cluster can adopt or starve a group's molecules.
    if (g_vpr_ctx.floorplanning().relative_macros.get_num_macros() != 0) {
        pack_relative_groups_first(candidate_selector,
                                   cluster_legalizer,
                                   prepacker,
                                   ram_mapper,
                                   balance_block_type_utilization,
                                   attraction_groups,
                                   num_used_type_instances,
                                   mutable_device_ctx,
                                   clustering_stats);
    }

    // Pick the first seed molecule.
    PackMoleculeId seed_mol_id = seed_selector.get_next_seed(cluster_legalizer);

    // Continue clustering as long as a valid seed is returned from the seed
    // selector.
    while (seed_mol_id.is_valid()) {
        // Check to ensure that this molecule is unclustered.
        VTR_ASSERT(!cluster_legalizer.is_mol_clustered(seed_mol_id));

        // Grow a cluster from the seed molecule and report progress.
        grow_cluster_from_seed(seed_mol_id,
                               candidate_selector,
                               cluster_legalizer,
                               prepacker,
                               ram_mapper,
                               balance_block_type_utilization,
                               attraction_groups,
                               num_used_type_instances,
                               mutable_device_ctx,
                               clustering_stats);

        // Pick new seed.
        seed_mol_id = seed_selector.get_next_seed(cluster_legalizer);
    }

    // If this architecture has LE physical block, report its usage.
    report_le_physical_block_usage(cluster_legalizer);

    return num_used_type_instances;
}

void GreedyClusterer::pack_relative_groups_first(GreedyCandidateSelector& candidate_selector,
                                                 ClusterLegalizer& cluster_legalizer,
                                                 const Prepacker& prepacker,
                                                 const RamMapper& ram_mapper,
                                                 bool balance_block_type_utilization,
                                                 AttractionInfo& attraction_groups,
                                                 std::map<t_logical_block_type_ptr, size_t>& num_used_type_instances,
                                                 DeviceContext& mutable_device_ctx,
                                                 t_cluster_progress_stats& clustering_stats) {
    const UserRelativeMacros& relative_macros = g_vpr_ctx.floorplanning().relative_macros;

    // Visit the macros and their groups in index order (deterministic).
    for (size_t imacro = 0; imacro < relative_macros.get_num_macros(); imacro++) {
        UserRelativeMacroId macro_id(imacro);
        const UserRelativeMacro& macro = relative_macros.get_macro(macro_id);

        for (size_t igroup = 0; igroup < macro.groups.size(); igroup++) {
            // Seed the group's cluster with the group's largest unclustered
            // molecule
            PackMoleculeId seed_mol_id;
            size_t seed_mol_num_atoms = 0;
            for (AtomBlockId blk_id : macro.groups[igroup].atoms) {
                PackMoleculeId mol_id = prepacker.get_atom_molecule(blk_id);
                if (cluster_legalizer.is_mol_clustered(mol_id))
                    continue;
                size_t num_atoms = prepacker.get_molecule_num_valid_atoms(mol_id);
                if (num_atoms > seed_mol_num_atoms) {
                    seed_mol_id = mol_id;
                    seed_mol_num_atoms = num_atoms;
                }
            }

            // Nothing to do for a group with no unclustered molecules.
            if (!seed_mol_id.is_valid())
                continue;

            // Grow the group's cluster through the same per-seed driver the
            // main loop uses. try_grow_cluster() packs the seed's whole group
            // up front (in one pass) and then fills the leftover space with
            // unconstrained logic.
            grow_cluster_from_seed(seed_mol_id,
                                   candidate_selector,
                                   cluster_legalizer,
                                   prepacker,
                                   ram_mapper,
                                   balance_block_type_utilization,
                                   attraction_groups,
                                   num_used_type_instances,
                                   mutable_device_ctx,
                                   clustering_stats);
        }
    }
}

LegalizationClusterId GreedyClusterer::grow_cluster_from_seed(PackMoleculeId seed_mol_id,
                                                              GreedyCandidateSelector& candidate_selector,
                                                              ClusterLegalizer& cluster_legalizer,
                                                              const Prepacker& prepacker,
                                                              const RamMapper& ram_mapper,
                                                              bool balance_block_type_utilization,
                                                              AttractionInfo& attraction_groups,
                                                              std::map<t_logical_block_type_ptr, size_t>& num_used_type_instances,
                                                              DeviceContext& mutable_device_ctx,
                                                              t_cluster_progress_stats& clustering_stats) {
    // The attempts, in order. Each entry is a legalization strategy plus the
    // sequence a relative placement group's molecules are offered in:
    // 1) Skip the intra-lb route per molecule (faster, less conservative) and
    //    legalize once at the end.
    // 2) If that final legalization fails, redo it with full legalization for
    //    each molecule added.
    // 3) If that fails too, redo it once more with the group's locked
    //    molecules offered in the opposite sequence. Only a cluster hosting a
    //    relative placement group is affected by the sequence, so for every
    //    other cluster this attempt is identical to 2) and is never reached
    //    (2) cannot fail for them).
    struct t_grow_attempt {
        ClusterLegalizationStrategy strategy;
        int group_order_variant;
    };
    const std::array<t_grow_attempt, 3> attempts = {{
        {ClusterLegalizationStrategy::SKIP_INTRA_LB_ROUTE, 0},
        {ClusterLegalizationStrategy::FULL, 0},
        {ClusterLegalizationStrategy::FULL, 1},
    }};

    LegalizationClusterId new_cluster_id;
    for (size_t iattempt = 0; iattempt < attempts.size(); iattempt++) {
        new_cluster_id = try_grow_cluster(seed_mol_id,
                                          candidate_selector,
                                          attempts[iattempt].strategy,
                                          attempts[iattempt].group_order_variant,
                                          /*is_last_attempt=*/iattempt + 1 == attempts.size(),
                                          cluster_legalizer,
                                          prepacker,
                                          ram_mapper,
                                          balance_block_type_utilization,
                                          attraction_groups,
                                          num_used_type_instances,
                                          mutable_device_ctx);
        if (new_cluster_id.is_valid()) {
            VTR_LOGV(log_verbosity_ > 2 && iattempt != 0,
                     "\tCluster %zu grown on attempt %zu (strategy %s, group order variant %d)\n",
                     (size_t)new_cluster_id, iattempt + 1,
                     attempts[iattempt].strategy == ClusterLegalizationStrategy::FULL ? "full" : "skip_intra_lb_route",
                     attempts[iattempt].group_order_variant);
            break;
        }
    }

    // Ensure that the seed was packed successfully (growing with the FULL
    // strategy cannot fail).
    VTR_ASSERT(new_cluster_id.is_valid());
    VTR_ASSERT(cluster_legalizer.is_mol_clustered(seed_mol_id));

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

    return new_cluster_id;
}

LegalizationClusterId GreedyClusterer::try_grow_cluster(PackMoleculeId seed_mol_id,
                                                        GreedyCandidateSelector& candidate_selector,
                                                        ClusterLegalizationStrategy strategy,
                                                        int group_order_variant,
                                                        bool is_last_attempt,
                                                        ClusterLegalizer& cluster_legalizer,
                                                        const Prepacker& prepacker,
                                                        const RamMapper& ram_mapper,
                                                        bool balance_block_type_utilization,
                                                        AttractionInfo& attraction_groups,
                                                        std::map<t_logical_block_type_ptr, size_t>& num_used_type_instances,
                                                        DeviceContext& mutable_device_ctx) {

    // Check to ensure that this molecule is unclustered.
    VTR_ASSERT(!cluster_legalizer.is_mol_clustered(seed_mol_id));

    // Set the legalization strategy of the cluster legalizer.
    cluster_legalizer.set_legalization_strategy(strategy);

    // Use the seed to start a new cluster.
    LegalizationClusterId legalization_cluster_id = start_new_cluster(seed_mol_id,
                                                                      cluster_legalizer,
                                                                      prepacker,
                                                                      ram_mapper,
                                                                      balance_block_type_utilization,
                                                                      num_used_type_instances,
                                                                      mutable_device_ctx);

    // Create the cluster gain stats. This updates the gains in the candidate
    // selector due to a new molecule being clustered.
    ClusterGainStats cluster_gain_stats = candidate_selector.create_cluster_gain_stats(seed_mol_id,
                                                                                       legalization_cluster_id,
                                                                                       cluster_legalizer,
                                                                                       attraction_groups);

    // A cluster seeded by a relative-group molecule packs its entire group
    // right away instead of relying on the candidate selector to propose the
    // group's molecules one at a time.
    //
    // One pass per grow attempt, in the sequence selected by
    // group_order_variant. There is no search over sites: the constraints file
    // gives each atom of the group one primitive and that is the only one it is
    // ever offered, so no attempt can reseat an atom - a retry can change only
    // whether the cluster routes.
    //
    // It can change that, though, which is why the caller has more than one
    // attempt (see grow_cluster_from_seed): the intra-cluster router assigns
    // the logically equivalent LUT/crossbar inputs greedily in the order
    // molecules arrive, so one sequence can leave two nets contending for a
    // single crossbar input where another sequence routes. This is not
    // hypothetical - the rpm_fpu_addsub IP has a group that only routes on the
    // third attempt.
    //
    // A molecule that is not admitted stays unclustered for now; the legalizer
    // reports why the site could not be taken, the ordinary fill below or the
    // main loop may still admit it, and the end-of-pass check in pack.cpp turns
    // a group that never completed into a fatal error.
    pack_relative_group_into_cluster(seed_mol_id,
                                     legalization_cluster_id,
                                     group_order_variant,
                                     cluster_legalizer,
                                     prepacker);

    // Update the candidate selector's bookkeeping (gains, marked blocks,
    // cluster attraction group) for each group molecule packed above. The
    // seed molecule is skipped: create_cluster_gain_stats() already accounted
    // for it.
    for (PackMoleculeId mol_id : cluster_legalizer.get_cluster_molecules(legalization_cluster_id)) {
        if (mol_id == seed_mol_id)
            continue;
        candidate_selector.update_cluster_gain_stats_candidate_success(cluster_gain_stats,
                                                                       mol_id,
                                                                       legalization_cluster_id,
                                                                       cluster_legalizer,
                                                                       attraction_groups);
    }

    // Select the first candidate molecule to try to add to this cluster.
    PackMoleculeId candidate_mol_id = candidate_selector.get_next_candidate_for_cluster(
        cluster_gain_stats,
        legalization_cluster_id,
        cluster_legalizer,
        attraction_groups);

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
    while (candidate_mol_id.is_valid() && num_repeated_molecules < max_num_repeated_molecules) {
        // Try to cluster the candidate molecule into the cluster.
        bool success = try_add_candidate_mol_to_cluster(candidate_mol_id,
                                                        legalization_cluster_id,
                                                        cluster_legalizer,
                                                        prepacker);

        // If the candidate molecule was clustered successfully, update
        // the cluster stats.
        if (success) {
            // If the last candidate was clustered successfully, update the
            // gains in the candidate selector.
            candidate_selector.update_cluster_gain_stats_candidate_success(cluster_gain_stats,
                                                                           candidate_mol_id,
                                                                           legalization_cluster_id,
                                                                           cluster_legalizer,
                                                                           attraction_groups);
        } else {
            // If the last candidate was not clustered successfully, update the
            // gains in the candidate selector accordingly.
            candidate_selector.update_cluster_gain_stats_candidate_failed(cluster_gain_stats,
                                                                          candidate_mol_id);
        }

        // Get the next candidate molecule.
        PackMoleculeId prev_candidate_mol_id = candidate_mol_id;
        candidate_mol_id = candidate_selector.get_next_candidate_for_cluster(
            cluster_gain_stats,
            legalization_cluster_id,
            cluster_legalizer,
            attraction_groups);

        // If the next candidate molecule is the same as the previous
        // candidate molecule, increment the number of repeated
        // molecules counter.
        if (candidate_mol_id == prev_candidate_mol_id)
            num_repeated_molecules++;
    }

    // Ensure that the cluster has a legal final routing.
    //
    // If strategy == SKIP_INTRA_LB_ROUTE, then this is the first and only
    // legality check.
    //
    // If strategy == FULL, then it is possible to reach this step without a
    // routing solution if a cluster has been packed the same way before, due
    // to skipping routing on clusters that are known to be legal from the
    // PackingSignatureTree. In this case, routing must be run at least once on
    // the final cluster for later stages to use.
    bool keep_cluster = cluster_legalizer.ensure_legal_final_routing(legalization_cluster_id);

    // A cluster hosting a relative placement group has only done its job if the
    // WHOLE group is in it; an attempt that left a molecule out no longer
    // matches the sites in the constraints file, so it is not worth keeping
    // while another sequence is still untried. On the last attempt the cluster
    // is kept regardless - the leftovers are then reported as a split group by
    // the end-of-pass check.
    if (keep_cluster && !is_last_attempt
        && !relative_group_fully_clustered(seed_mol_id, legalization_cluster_id, cluster_legalizer, prepacker)) {
        VTR_LOGV(log_verbosity_ > 2,
                 "\tCluster %zu did not take its whole relative placement group; retrying with the next sequence\n",
                 (size_t)legalization_cluster_id);
        keep_cluster = false;
    }

    if (!keep_cluster) {
        // If the cluster is not legal, undo the cluster.
        // Update the used type instances.
        num_used_type_instances[cluster_legalizer.get_cluster_type(legalization_cluster_id)]--;
        // Destroy the illegal cluster.
        cluster_legalizer.destroy_cluster(legalization_cluster_id);
        cluster_legalizer.compress();
        // Cluster failed to grow.
        return LegalizationClusterId();
    }

    // A legal cluster must have been created by this point.
    VTR_ASSERT(legalization_cluster_id.is_valid());

    // After the cluster has been fully created, update internal structures
    // to improve the gain calculation.
    candidate_selector.update_candidate_selector_finalize_cluster(cluster_gain_stats,
                                                                  legalization_cluster_id);

    // Since the cluster will no longer be added to beyond this point,
    // clean the cluster of any data not strictly necessary for
    // creating the clustered netlist.
    cluster_legalizer.clean_cluster(legalization_cluster_id);

    // Cluster has been grown successfully.
    return legalization_cluster_id;
}

/**
 * @brief If the atom is a memory primitive with a pre-assigned RAM type, moves
 *        that type to the front of candidate_types using a stable partition so
 *        the packer tries it first. Has no effect for non-memory atoms or groups
 *        without a pre-assigned type.
 *
 * @param root_atom        The seed atom being clustered.
 * @param prepacker        Used to look up the expected primitive for root_atom.
 * @param ram_mapper       Used to look up the logical RAM group and pre-assigned type.
 * @param atom_netlist     Used to look up the atom name for warning messages.
 * @param candidate_types  Block types to reorder in place.
 */
static void prioritize_pre_assigned_ram_type(AtomBlockId root_atom,
                                             const Prepacker& prepacker,
                                             const RamMapper& ram_mapper,
                                             const AtomNetlist& atom_netlist,
                                             std::vector<t_logical_block_type_ptr>& candidate_types) {
    const t_pb_graph_node* prim = prepacker.get_expected_lowest_cost_pb_gnode(root_atom);
    if (!prim->pb_type->is_primitive() || prim->pb_type->class_type != MEMORY_CLASS)
        return;

    const LogicalRamGroupId group_id = ram_mapper.group_id_of(root_atom);
    VTR_ASSERT_MSG(group_id.is_valid(), "root_atom of memory class should be mapped to a LogicalRamGroup");
    const LogicalRamGroup& ram_group = ram_mapper.group(group_id);

    auto it = std::find(ram_group.atoms.begin(), ram_group.atoms.end(), root_atom);
    VTR_ASSERT_MSG(it != ram_group.atoms.end(), "Could not find root atom in the retrieved logical ram atoms");

    if (ram_group.pre_assigned_type) {
        t_logical_block_type_ptr pre_assigned_type = ram_group.pre_assigned_type;
        std::stable_partition(candidate_types.begin(), candidate_types.end(),
                              [&](t_logical_block_type_ptr p) { return p == pre_assigned_type; });
    } else {
        VTR_LOG_WARN("No pre-assigned type found for logical RAM group of atom %s\n",
                     atom_netlist.block_name(root_atom).c_str());
    }
}

LegalizationClusterId GreedyClusterer::start_new_cluster(
    PackMoleculeId seed_mol_id,
    ClusterLegalizer& cluster_legalizer,
    const Prepacker& prepacker,
    const RamMapper& ram_mapper,
    bool balance_block_type_utilization,
    std::map<t_logical_block_type_ptr, size_t>& num_used_type_instances,
    DeviceContext& mutable_device_ctx) {

    VTR_ASSERT(seed_mol_id.is_valid());
    const t_pack_molecule& seed_mol = prepacker.get_molecule(seed_mol_id);

    /* Allocate a dummy initial cluster and load a atom block as a seed and check if it is legal */
    AtomBlockId root_atom = seed_mol.atom_block_ids[seed_mol.root];
    const std::string& root_atom_name = atom_netlist_.block_name(root_atom);
    LogicalModelId root_model_id = atom_netlist_.block_model(root_atom);
    VTR_ASSERT(root_model_id.is_valid());
    VTR_ASSERT(!primitive_candidate_block_types_[root_model_id].empty());
    std::vector<t_logical_block_type_ptr> candidate_types = primitive_candidate_block_types_[root_model_id];

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

    if (has_ram_groups_)
        prioritize_pre_assigned_ram_type(root_atom, prepacker, ram_mapper, atom_netlist_, candidate_types);

    if (log_verbosity_ > 2) {
        VTR_LOG("\tSeed: '%s' (%s)", root_atom_name.c_str(), arch_.models.get_model(root_model_id).name.c_str());
        VTR_LOGV(seed_mol.pack_pattern, " molecule_type %s molecule_size %zu",
                 seed_mol.pack_pattern->name.c_str(), seed_mol.atom_block_ids.size());
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
            std::tie(pack_result, new_cluster_id) = cluster_legalizer.start_new_cluster(seed_mol_id, type, j);
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
        std::string locked_site_reason;
        const ClusterLegalizer::UnsatisfiableLockedSite& locked_site = cluster_legalizer.get_last_unsatisfiable_locked_site();
        if (locked_site.atom == seed_mol.atom_block_ids[seed_mol.root]) {
            const UserRelativeMacros& relative_macros = g_vpr_ctx.floorplanning().relative_macros;
            std::string examples;
            for (const std::string& path : locked_site.available_site_paths)
                examples += " '" + path + "'";
            locked_site_reason = "\tThe atom is locked to primitive site '" + locked_site.site_path
                                 + "' by relative macro '" + relative_macros.get_macro(locked_site.rel_group.first).name
                                 + "' group " + std::to_string(locked_site.rel_group.second)
                                 + ", and that site was not available in any candidate block type (last tried '"
                                 + locked_site.cluster_type_name + "', where compatible sites include"
                                 + (examples.empty() ? std::string(" none") : examples)
                                 + "). A site path also fixes the block type and the mode at every level, so a "
                                   "path recorded on another architecture or another mode never matches. "
                                   "Regenerate the macro against this netlist and architecture.\n";
        }

        //Explored all candidates
        if (seed_mol.type == e_pack_pattern_molecule_type::MOLECULE_FORCED_PACK) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "Can not find any logic block that can implement molecule.\n"
                            "\tPattern %s %s\n%s",
                            seed_mol.pack_pattern->name.c_str(),
                            root_atom_name.c_str(),
                            locked_site_reason.c_str());
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "Can not find any logic block that can implement molecule.\n"
                            "\tAtom %s (%s)\n%s",
                            root_atom_name.c_str(), arch_.models.model_name(root_model_id).c_str(),
                            locked_site_reason.c_str());
        }
    }

    VTR_ASSERT(success);
    VTR_ASSERT(new_cluster_id.is_valid());

    VTR_LOGV(log_verbosity_ > 2,
             "Complex block %zu: '%s' (%s) ", size_t(new_cluster_id),
             cluster_legalizer.get_cluster_name(new_cluster_id).c_str(),
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

    if (!has_fixed_device_size(vpr_setup_)
        && num_used_type_instances[block_type] > num_instances) {
        mutable_device_ctx.grid = create_device_grid(packer_opts_.device_layout,
                                                     arch_.grid_layouts,
                                                     num_used_type_instances,
                                                     packer_opts_.target_device_utilization,
                                                     vpr_setup_.device_width);
    }

    return new_cluster_id;
}

bool GreedyClusterer::try_add_candidate_mol_to_cluster(PackMoleculeId candidate_mol_id,
                                                       LegalizationClusterId legalization_cluster_id,
                                                       ClusterLegalizer& cluster_legalizer,
                                                       const Prepacker& prepacker) {
    VTR_ASSERT(candidate_mol_id.is_valid());
    VTR_ASSERT(!cluster_legalizer.is_mol_clustered(candidate_mol_id));
    VTR_ASSERT(legalization_cluster_id.is_valid());

    e_block_pack_status pack_status = cluster_legalizer.add_mol_to_cluster(candidate_mol_id,
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
            case e_block_pack_status::BLK_FAILED_RELATIVE_GROUP:
                VTR_LOG("\tFAILED_RELATIVE_GROUP_CHECK: ");
                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_PACK, "Unknown pack status thrown.");
                break;
        }
        // Get the block name and model name
        const t_pack_molecule& candidate_mol = prepacker.get_molecule(candidate_mol_id);
        AtomBlockId blk_id = candidate_mol.atom_block_ids[candidate_mol.root];
        VTR_ASSERT(blk_id.is_valid());
        std::string blk_name = atom_netlist_.block_name(blk_id);
        LogicalModelId blk_model_id = atom_netlist_.block_model(blk_id);
        std::string blk_model_name = arch_.models.model_name(blk_model_id);
        VTR_LOG("'%s' (%s)", blk_name.c_str(), blk_model_name.c_str());
        VTR_LOGV(candidate_mol.pack_pattern, " molecule %s molecule_size %zu",
                 candidate_mol.pack_pattern->name.c_str(),
                 candidate_mol.atom_block_ids.size());
        VTR_LOG("\n");
        fflush(stdout);
    }

    return pack_status == e_block_pack_status::BLK_PASSED;
}

void GreedyClusterer::pack_relative_group_into_cluster(PackMoleculeId seed_mol_id,
                                                       LegalizationClusterId legalization_cluster_id,
                                                       int order_variant,
                                                       ClusterLegalizer& cluster_legalizer,
                                                       const Prepacker& prepacker) {
    const UserRelativeMacros& relative_macros = g_vpr_ctx.floorplanning().relative_macros;
    if (relative_macros.get_num_macros() == 0)
        return;

    // Find the relative placement group of the seed molecule (if any).
    std::pair<UserRelativeMacroId, int> group = get_molecule_relative_group(prepacker.get_molecule(seed_mol_id),
                                                                            relative_macros);
    if (!group.first.is_valid())
        return;

    // Collect the group's molecules that are not clustered yet (the seed
    // molecule is already in the cluster).
    //
    // A group molecule already in a DIFFERENT cluster means the group is split
    // (an earlier cluster adopted the group but could not take all of it).
    // Ripping it out is not possible, so just log it here and let the
    // end-of-pass check in pack.cpp handle the split.
    std::vector<PackMoleculeId> order;
    std::unordered_set<PackMoleculeId> seen = {seed_mol_id};
    for (AtomBlockId blk_id : relative_macros.get_macro(group.first).groups[group.second].atoms) {
        PackMoleculeId mol_id = prepacker.get_atom_molecule(blk_id);
        if (seen.count(mol_id))
            continue;
        seen.insert(mol_id);
        if (!cluster_legalizer.is_mol_clustered(mol_id)) {
            order.push_back(mol_id);
        } else if (cluster_legalizer.get_atom_cluster(blk_id) != legalization_cluster_id) {
            // Already in a different cluster, so the group is split (see above).
            VTR_LOGV(log_verbosity_ > 1,
                     "\tRelative macro '%s' group %d is split: molecule of atom '%s' is already in "
                     "cluster %zu while cluster %zu is being seeded with the same group.\n",
                     relative_macros.get_macro(group.first).name.c_str(), group.second,
                     atom_netlist_.block_name(blk_id).c_str(),
                     (size_t)cluster_legalizer.get_atom_cluster(blk_id),
                     (size_t)legalization_cluster_id);
        }
    }
    if (order.empty())
        return;

    // Locked molecules go first, in constraints-file order: an unlocked
    // molecule can go anywhere still free, so any other order lets it squat a
    // locked molecule's site. The unlocked ones then go largest first, since
    // multi-atom molecules have the fewest legal placements left once the
    // cluster fills up and nothing can be ripped up. Both orders are
    // deterministic.
    std::stable_sort(order.begin(), order.end(), [&](PackMoleculeId lhs, PackMoleculeId rhs) {
        bool lhs_locked = is_molecule_locked_to_site(lhs, prepacker, relative_macros);
        bool rhs_locked = is_molecule_locked_to_site(rhs, prepacker, relative_macros);
        if (lhs_locked != rhs_locked)
            return lhs_locked;
        if (lhs_locked)
            return false;
        return prepacker.get_molecule_num_valid_atoms(lhs) > prepacker.get_molecule_num_valid_atoms(rhs);
    });

    // order_variant 1 reverses the locked molecules among themselves; the
    // unlocked tail keeps its order, so locked molecules still go first.
    // Reversing is the whole variation. It can matter because sites are fixed
    // but the intra-cluster router is not: it picks which of the logically
    // equivalent LUT/crossbar inputs each net enters through greedily as
    // molecules arrive, so a sequence that leaves two nets contending for one
    // input fails where another routes. No variant can move an atom off its
    // site, and verify_clustering re-checks that afterwards.
    if (order_variant == 1) {
        size_t num_locked = std::count_if(order.begin(), order.end(), [&](PackMoleculeId mol_id) {
            return is_molecule_locked_to_site(mol_id, prepacker, relative_macros);
        });
        std::reverse(order.begin(), order.begin() + num_locked);
    }

    // One deterministic pass: each molecule is offered to the cluster exactly
    // once, in the order above. This is NOT the group's only chance, and the
    // feature depends on that - the gain-driven fill that follows (see
    // try_grow_cluster) re-proposes a molecule that failed here, and the
    // legalizer accepts it if the cluster state has changed since. Removing
    // that second offer was measured to break ip_rpm_fpu_mult, where six
    // group-4 flip-flops are locked into FF slots of arithmetic-mode FLEs that
    // the group's own 40-atom carry chain claims first. Molecules that are
    // never admitted are reported as a split group by the end-of-pass check in
    // pack.cpp.
    for (PackMoleculeId mol_id : order) {
        cluster_legalizer.add_mol_to_cluster(mol_id, legalization_cluster_id);
    }
}

bool GreedyClusterer::relative_group_fully_clustered(PackMoleculeId seed_mol_id,
                                                     LegalizationClusterId legalization_cluster_id,
                                                     const ClusterLegalizer& cluster_legalizer,
                                                     const Prepacker& prepacker) {
    const UserRelativeMacros& relative_macros = g_vpr_ctx.floorplanning().relative_macros;
    if (relative_macros.get_num_macros() == 0)
        return true;

    std::pair<UserRelativeMacroId, int> group = get_molecule_relative_group(prepacker.get_molecule(seed_mol_id),
                                                                            relative_macros);
    if (!group.first.is_valid())
        return true;

    for (AtomBlockId blk_id : relative_macros.get_macro(group.first).groups[group.second].atoms) {
        if (cluster_legalizer.get_atom_cluster(blk_id) != legalization_cluster_id)
            return false;
    }
    return true;
}

bool GreedyClusterer::is_molecule_locked_to_site(PackMoleculeId mol_id,
                                                 const Prepacker& prepacker,
                                                 const UserRelativeMacros& relative_macros) {
    const t_pack_molecule& molecule = prepacker.get_molecule(mol_id);
    AtomBlockId root_blk_id = molecule.atom_block_ids[molecule.root];
    if (!root_blk_id.is_valid())
        return false;
    return !relative_macros.get_atom_site_path(root_blk_id).empty();
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
