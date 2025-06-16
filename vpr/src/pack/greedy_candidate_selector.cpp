/**
 * @file
 * @author  Alex Singer
 * @date    January 2024
 * @brief   The definitions of the Greedy Candidate Selector class.
 */

#include "greedy_candidate_selector.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <vector>
#include "PreClusterTimingManager.h"
#include "appack_context.h"
#include "flat_placement_types.h"
#include "flat_placement_utils.h"
#include "atom_netlist.h"
#include "attraction_groups.h"
#include "cluster_legalizer.h"
#include "cluster_placement.h"
#include "greedy_clusterer.h"
#include "logic_types.h"
#include "prepack.h"
#include "timing_info.h"
#include "vpr_types.h"
#include "vtr_assert.h"
#include "vtr_ndmatrix.h"
#include "vtr_vector.h"
#include "vtr_vector_map.h"

/*
 * @brief Get gain of packing molecule into current cluster.
 *
 * gain is equal to:
 * total_block_gain
 * + molecule_base_gain*some_factor
 * - introduced_input_nets_of_unrelated_blocks_pulled_in_by_molecule*some_other_factor
 *
 * TODO: Confirm that this comment is correct.
 */
static float get_molecule_gain(PackMoleculeId molecule_id,
                               ClusterGainStats& cluster_gain_stats,
                               AttractGroupId cluster_attraction_group_id,
                               AttractionInfo& attraction_groups,
                               int num_molecule_failures,
                               const Prepacker& prepacker,
                               const AtomNetlist& atom_netlist,
                               const APPackContext& appack_ctx);

/*
 * @brief Remove blk from list of feasible blocks sorted according to gain.
 *
 * Useful for removing blocks that are repeatedly failing. If a block
 * has been found to be illegal, we don't repeatedly consider it.
 */
static void remove_molecule_from_pb_stats_candidates(
    PackMoleculeId molecule_id,
    ClusterGainStats& cluster_gain_stats);

/*
 * @brief Add blk to list of feasible blocks sorted according to gain.
 */
static void add_molecule_to_pb_stats_candidates(
    PackMoleculeId molecule_id,
    ClusterGainStats& cluster_gain_stats,
    t_logical_block_type_ptr cluster_type,
    AttractionInfo& attraction_groups,
    const Prepacker& prepacker,
    const AtomNetlist& atom_netlist,
    const APPackContext& appack_ctx);

/**
 * @brief Get the flat placement position of the given molecule.
 */
static t_flat_pl_loc get_molecule_pos(PackMoleculeId molecule_id,
                                      const Prepacker& prepacker,
                                      const APPackContext& appack_ctx) {
    VTR_ASSERT_SAFE_MSG(appack_ctx.appack_options.use_appack, "APPack is not enabled");
    VTR_ASSERT_SAFE_MSG(molecule_id.is_valid(), "Molecule ID is invalid");
    AtomBlockId root_blk_id = prepacker.get_molecule_root_atom(molecule_id);
    return appack_ctx.flat_placement_info.get_pos(root_blk_id);
}

GreedyCandidateSelector::GreedyCandidateSelector(
    const AtomNetlist& atom_netlist,
    const Prepacker& prepacker,
    const t_packer_opts& packer_opts,
    bool allow_unrelated_clustering,
    const t_molecule_stats& max_molecule_stats,
    const vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types,
    const t_pack_high_fanout_thresholds& high_fanout_thresholds,
    const std::unordered_set<AtomNetId>& is_clock,
    const std::unordered_set<AtomNetId>& is_global,
    const std::unordered_set<AtomNetId>& net_output_feeds_driving_block_input,
    const PreClusterTimingManager& pre_cluster_timing_manager,
    const APPackContext& appack_ctx,
    const LogicalModels& models,
    int log_verbosity)
    : atom_netlist_(atom_netlist)
    , prepacker_(prepacker)
    , packer_opts_(packer_opts)
    , allow_unrelated_clustering_(allow_unrelated_clustering)
    , log_verbosity_(log_verbosity)
    , primitive_candidate_block_types_(primitive_candidate_block_types)
    , high_fanout_thresholds_(high_fanout_thresholds)
    , is_clock_(is_clock)
    , is_global_(is_global)
    , net_output_feeds_driving_block_input_(net_output_feeds_driving_block_input)
    , pre_cluster_timing_manager_(pre_cluster_timing_manager)
    , appack_ctx_(appack_ctx)
    , rng_(0) {

    // Initialize unrelated clustering data if unrelated clustering is enabled.
    if (allow_unrelated_clustering_) {
        initialize_unrelated_clustering_data(max_molecule_stats, models);
    }

    /* TODO: This is memory inefficient, fix if causes problems */
    /* Store stats on nets used by packed block, useful for determining transitively connected blocks
     * (eg. [A1, A2, ..]->[B1, B2, ..]->C implies cluster [A1, A2, ...] and C have a weak link) */
    clb_inter_blk_nets_.resize(atom_netlist.blocks().size());
}

void GreedyCandidateSelector::initialize_unrelated_clustering_data(const t_molecule_stats& max_molecule_stats, const LogicalModels& models) {
    // Create a sorted list of molecules, sorted on decreasing molecule base
    // gain. (Highest gain).
    std::vector<PackMoleculeId> molecules_vector;
    molecules_vector.assign(prepacker_.molecules().begin(), prepacker_.molecules().end());
    std::stable_sort(molecules_vector.begin(),
                     molecules_vector.end(),
                     [&](PackMoleculeId a_id, PackMoleculeId b_id) {
                         const t_pack_molecule& a = prepacker_.get_molecule(a_id);
                         const t_pack_molecule& b = prepacker_.get_molecule(b_id);

                         return a.base_gain > b.base_gain;
                     });

    if (appack_ctx_.appack_options.use_appack) {
        /**
         * For APPack, we build a spatial data structure where for each 1x1 grid
         * position on the FPGA, we maintain lists of molecule candidates.
         * The lists are in order of number of used external pins by the molecule.
         * Within each list, the molecules are sorted by their base gain.
         */
        // Get the max x, y, and layer from the flat placement.
        t_flat_pl_loc max_loc({0.0f, 0.0f, 0.0f});
        for (PackMoleculeId mol_id : molecules_vector) {
            t_flat_pl_loc mol_pos = get_molecule_pos(mol_id, prepacker_, appack_ctx_);
            max_loc.x = std::max(max_loc.x, mol_pos.x);
            max_loc.y = std::max(max_loc.y, mol_pos.y);
            max_loc.layer = std::max(max_loc.layer, mol_pos.layer);
        }

        VTR_ASSERT_MSG(max_loc.layer == 0,
                       "APPack unrelated clustering does not support 3D "
                       "FPGAs yet");

        // Initialize the data structure with empty arrays with enough space
        // for each molecule.
        size_t flat_grid_width = max_loc.x + 1;
        size_t flat_grid_height = max_loc.y + 1;
        appack_unrelated_clustering_data_ =
            vtr::NdMatrix<std::vector<std::vector<PackMoleculeId>>, 2>({flat_grid_width,
                                                                        flat_grid_height});
        for (size_t x = 0; x < flat_grid_width; x++) {
            for (size_t y = 0; y < flat_grid_height; y++) {
                // Resize to the maximum number of used external pins. This is
                // to ensure that every molecule below can be inserted into a
                // valid list based on their number of external pins.
                appack_unrelated_clustering_data_[x][y].resize(max_molecule_stats.num_used_ext_pins + 1);
            }
        }

        // Fill the grid with molecule information.
        // Note: These molecules are sorted based on their base gain. They are
        //       inserted in such a way that the highest gain molecules appear
        //       first in the lists below.
        for (PackMoleculeId mol_id : molecules_vector) {
            t_flat_pl_loc mol_pos = get_molecule_pos(mol_id, prepacker_, appack_ctx_);

            //Figure out how many external inputs are used by this molecule
            t_molecule_stats molecule_stats = prepacker_.calc_molecule_stats(mol_id, atom_netlist_, models);
            int ext_inps = molecule_stats.num_used_ext_inputs;

            //Insert the molecule into the unclustered lists by number of external inputs
            auto& tile_uc_data = appack_unrelated_clustering_data_[mol_pos.x][mol_pos.y];
            tile_uc_data[ext_inps].push_back(mol_id);
        }
    } else {
        // When not performing APPack, allocate and load a similar data structure
        // without spatial information.

        /* alloc and load list of molecules to pack */
        unrelated_clustering_data_.resize(max_molecule_stats.num_used_ext_inputs + 1);

        // Push back the each molecule into the unrelated clustering data vector
        // for their external inputs. This creates individual sorted lists of
        // molecules for each number of used external inputs.
        for (PackMoleculeId mol_id : molecules_vector) {
            //Figure out how many external inputs are used by this molecule
            t_molecule_stats molecule_stats = prepacker_.calc_molecule_stats(mol_id, atom_netlist_, models);
            int ext_inps = molecule_stats.num_used_ext_inputs;

            //Insert the molecule into the unclustered lists by number of external inputs
            unrelated_clustering_data_[ext_inps].push_back(mol_id);
        }
    }
}

GreedyCandidateSelector::~GreedyCandidateSelector() {
}

ClusterGainStats GreedyCandidateSelector::create_cluster_gain_stats(
    PackMoleculeId cluster_seed_mol_id,
    LegalizationClusterId cluster_id,
    const ClusterLegalizer& cluster_legalizer,
    AttractionInfo& attraction_groups) {
    // Initialize the cluster gain stats.
    ClusterGainStats cluster_gain_stats;
    cluster_gain_stats.seed_molecule_id = cluster_seed_mol_id;
    cluster_gain_stats.has_done_connectivity_and_timing = false;
    cluster_gain_stats.initial_search_for_feasible_blocks = true;
    cluster_gain_stats.num_candidates_proposed = 0;
    cluster_gain_stats.candidates_propose_limit = packer_opts_.feasible_block_array_size;
    cluster_gain_stats.feasible_blocks.clear();
    cluster_gain_stats.tie_break_high_fanout_net = AtomNetId::INVALID();
    cluster_gain_stats.explore_transitive_fanout = true;

    // Update the cluster gain stats based on the addition of the seed mol to
    // the cluster.
    // TODO: We may want to update the cluster gain stats different, knowing
    //       that this candidate was the seed molecule.
    update_cluster_gain_stats_candidate_success(cluster_gain_stats,
                                                cluster_seed_mol_id,
                                                cluster_id,
                                                cluster_legalizer,
                                                attraction_groups);

    // If a flat placement is provided, set the centroid to be the location of
    // the seed molecule.
    if (appack_ctx_.appack_options.use_appack) {
        t_flat_pl_loc seed_mol_pos = get_molecule_pos(cluster_seed_mol_id,
                                                      prepacker_,
                                                      appack_ctx_);
        cluster_gain_stats.flat_cluster_position = seed_mol_pos;
        cluster_gain_stats.mol_pos_sum = seed_mol_pos;
    }

    // Mark if this cluster is a memory block. We detect this by checking if the
    // class type of the seed primitive pb is a memory class.
    // This is used by APPack to turn off certain optimizations which interfere
    // with RAM packing.
    const auto& seed_mol = prepacker_.get_molecule(cluster_seed_mol_id);
    AtomBlockId seed_atom = seed_mol.atom_block_ids[seed_mol.root];
    const auto seed_pb = cluster_legalizer.atom_pb_lookup().atom_pb(seed_atom);
    cluster_gain_stats.is_memory = seed_pb->pb_graph_node->pb_type->class_type == MEMORY_CLASS;

    // Return the cluster gain stats.
    return cluster_gain_stats;
}

void GreedyCandidateSelector::update_cluster_gain_stats_candidate_success(
    ClusterGainStats& cluster_gain_stats,
    PackMoleculeId successful_mol_id,
    LegalizationClusterId cluster_id,
    const ClusterLegalizer& cluster_legalizer,
    AttractionInfo& attraction_groups) {
    VTR_ASSERT(successful_mol_id.is_valid());
    // TODO: If this threshold lookup gets expensive, move outside.
    int high_fanout_net_threshold = high_fanout_thresholds_.get_threshold(cluster_legalizer.get_cluster_type(cluster_id)->name);

    // Mark and update the gain stats for each block in the succesfully
    // clustered molecule.
    // Makes calls to update cluster stats such as the gain map for atoms, used
    // pins, and clock structures, in order to reflect the new content of the
    // cluster. Also keeps track of which attraction group the cluster belongs
    // to.
    const t_pack_molecule& successful_mol = prepacker_.get_molecule(successful_mol_id);
    for (AtomBlockId blk_id : successful_mol.atom_block_ids) {
        if (!blk_id) {
            continue;
        }

        //Update attraction group
        AttractGroupId atom_grp_id = attraction_groups.get_atom_attraction_group(blk_id);

        /* reset list of feasible blocks */
        cluster_gain_stats.has_done_connectivity_and_timing = false;
        cluster_gain_stats.initial_search_for_feasible_blocks = true;
        cluster_gain_stats.num_candidates_proposed = 0;
        cluster_gain_stats.feasible_blocks.clear();
        /* TODO: Allow clusters to have more than one attraction group. */
        if (atom_grp_id.is_valid())
            cluster_gain_stats.attraction_grp_id = atom_grp_id;

        /* Outputs first */
        for (AtomPinId pin_id : atom_netlist_.block_output_pins(blk_id)) {
            AtomNetId net_id = atom_netlist_.pin_net(pin_id);

            e_gain_update gain_flag = e_gain_update::NO_GAIN;
            if (!is_clock_.count(net_id))
                gain_flag = e_gain_update::GAIN;

            mark_and_update_partial_gain(cluster_gain_stats,
                                         net_id,
                                         gain_flag,
                                         blk_id,
                                         cluster_legalizer,
                                         high_fanout_net_threshold,
                                         e_net_relation_to_clustered_block::OUTPUT);
        }

        /* Next Inputs */
        for (AtomPinId pin_id : atom_netlist_.block_input_pins(blk_id)) {
            AtomNetId net_id = atom_netlist_.pin_net(pin_id);
            mark_and_update_partial_gain(cluster_gain_stats,
                                         net_id,
                                         e_gain_update::GAIN,
                                         blk_id,
                                         cluster_legalizer,
                                         high_fanout_net_threshold,
                                         e_net_relation_to_clustered_block::INPUT);
        }

        /* Finally Clocks */
        for (AtomPinId pin_id : atom_netlist_.block_clock_pins(blk_id)) {
            AtomNetId net_id = atom_netlist_.pin_net(pin_id);

            mark_and_update_partial_gain(cluster_gain_stats,
                                         net_id,
                                         e_gain_update::NO_GAIN,
                                         blk_id,
                                         cluster_legalizer,
                                         high_fanout_net_threshold,
                                         e_net_relation_to_clustered_block::INPUT);
        }

        // TODO: For flat placement reconstruction, should we mark the molecules
        //       in the same tile as the seed of this cluster?

        update_total_gain(cluster_gain_stats, attraction_groups);
    }

    // if this molecule came from the transitive fanout candidates remove it
    cluster_gain_stats.transitive_fanout_candidates.erase(successful_mol.atom_block_ids[successful_mol.root]);
    cluster_gain_stats.explore_transitive_fanout = true;

    // Whenever a new molecule has been clustered, reset the number of
    // unrelated clustering attempts.
    num_unrelated_clustering_attempts_ = 0;

    // If using APPack, update the position of the cluster based on the addition
    // of this molecule.
    if (appack_ctx_.appack_options.use_appack) {
        t_flat_pl_loc successful_mol_pos = get_molecule_pos(successful_mol_id,
                                                            prepacker_,
                                                            appack_ctx_);
        // Accumulate the molecules position into the position sum.
        cluster_gain_stats.mol_pos_sum += successful_mol_pos;

        if (appack_ctx_.appack_options.cluster_location_ty == t_appack_options::e_cl_loc_ty::CENTROID) {
            // If the cluster's position is defined as the centroid of the
            // positions of the mols it contains, compute the new centroid.
            cluster_gain_stats.flat_cluster_position = cluster_gain_stats.mol_pos_sum;
            size_t num_mols_in_cluster = cluster_legalizer.get_num_molecules_in_cluster(cluster_id);
            cluster_gain_stats.flat_cluster_position /= static_cast<float>(num_mols_in_cluster);
        }
    }
}

void GreedyCandidateSelector::mark_and_update_partial_gain(
    ClusterGainStats& cluster_gain_stats,
    AtomNetId net_id,
    e_gain_update gain_flag,
    AtomBlockId clustered_blk_id,
    const ClusterLegalizer& cluster_legalizer,
    int high_fanout_net_threshold,
    e_net_relation_to_clustered_block net_relation_to_clustered_block) {

    if (int(atom_netlist_.net_sinks(net_id).size()) > high_fanout_net_threshold) {
        /* Optimization: It can be too runtime costly for marking all sinks for
         * a high fanout-net that probably has no hope of ever getting packed,
         * thus ignore those high fanout nets */
        if (!is_global_.count(net_id)) {
            /* If no low/medium fanout nets, we may need to consider
             * high fan-out nets for packing, so select one and store it */
            AtomNetId stored_net = cluster_gain_stats.tie_break_high_fanout_net;
            if (!stored_net || atom_netlist_.net_sinks(net_id).size() < atom_netlist_.net_sinks(stored_net).size()) {
                cluster_gain_stats.tie_break_high_fanout_net = net_id;
            }
        }
        return;
    }

    /* Mark atom net as being visited, if necessary. */
    if (cluster_gain_stats.num_pins_of_net_in_pb.count(net_id) == 0) {
        cluster_gain_stats.marked_nets.push_back(net_id);
    }

    /* Update gains of affected blocks. */
    if (gain_flag == e_gain_update::GAIN) {
        /* Check if this net is connected to it's driver block multiple times (i.e. as both an output and input)
         * If so, avoid double counting by skipping the first (driving) pin. */
        auto pins = atom_netlist_.net_pins(net_id);
        if (net_output_feeds_driving_block_input_.count(net_id) != 0)
            //We implicitly assume here that net_output_feeds_driver_block_input[net_id] is 2
            //(i.e. the net loops back to the block only once)
            pins = atom_netlist_.net_sinks(net_id);

        if (cluster_gain_stats.num_pins_of_net_in_pb.count(net_id) == 0) {
            for (AtomPinId pin_id : pins) {
                AtomBlockId blk_id = atom_netlist_.pin_block(pin_id);
                if (!cluster_legalizer.is_atom_clustered(blk_id)) {
                    if (cluster_gain_stats.sharing_gain.count(blk_id) == 0) {
                        cluster_gain_stats.marked_blocks.push_back(blk_id);
                        cluster_gain_stats.sharing_gain[blk_id] = 1;
                    } else {
                        cluster_gain_stats.sharing_gain[blk_id]++;
                    }
                }
            }
        }

        if (packer_opts_.connection_driven) {
            update_connection_gain_values(cluster_gain_stats,
                                          net_id,
                                          clustered_blk_id,
                                          cluster_legalizer,
                                          net_relation_to_clustered_block);
        }

        if (packer_opts_.timing_driven) {
            update_timing_gain_values(cluster_gain_stats,
                                      net_id,
                                      cluster_legalizer,
                                      net_relation_to_clustered_block);
        }
    }
    if (cluster_gain_stats.num_pins_of_net_in_pb.count(net_id) == 0) {
        cluster_gain_stats.num_pins_of_net_in_pb[net_id] = 0;
    }
    cluster_gain_stats.num_pins_of_net_in_pb[net_id]++;
}

/**
 * @brief Determine if pb is a child of cluster_pb.
 */
static bool is_pb_in_cluster_pb(const t_pb* pb, const t_pb* cluster_pb) {
    const t_pb* cur_pb = pb;
    while (cur_pb) {
        if (cur_pb == cluster_pb) {
            return true;
        }
        cur_pb = cur_pb->parent_pb;
    }
    return false;
}

void GreedyCandidateSelector::update_connection_gain_values(
    ClusterGainStats& cluster_gain_stats,
    AtomNetId net_id,
    AtomBlockId clustered_blk_id,
    const ClusterLegalizer& cluster_legalizer,
    e_net_relation_to_clustered_block net_relation_to_clustered_block) {

    /*This function is called when the connection_gain values on the net net_id
     *require updating.   */

    int num_internal_connections, num_open_connections, num_stuck_connections;
    num_internal_connections = num_open_connections = num_stuck_connections = 0;

    LegalizationClusterId legalization_cluster_id = cluster_legalizer.get_atom_cluster(clustered_blk_id);

    /* may wish to speed things up by ignoring clock nets since they are high fanout */
    for (AtomPinId pin_id : atom_netlist_.net_pins(net_id)) {
        AtomBlockId blk_id = atom_netlist_.pin_block(pin_id);
        // TODO: Should investigate this. Using the atom pb bimap through is_atom_blk_in_cluster_block
        // in this class is very strange
        const t_pb* pin_block_pb = cluster_legalizer.atom_pb_lookup().atom_pb(blk_id);
        const t_pb* cluster_pb = cluster_legalizer.atom_pb_lookup().atom_pb(clustered_blk_id);

        if (cluster_legalizer.get_atom_cluster(blk_id) == legalization_cluster_id && is_pb_in_cluster_pb(pin_block_pb, cluster_pb)) {
            num_internal_connections++;
        } else if (!cluster_legalizer.is_atom_clustered(blk_id)) {
            num_open_connections++;
        } else {
            num_stuck_connections++;
        }
    }

    if (net_relation_to_clustered_block == e_net_relation_to_clustered_block::OUTPUT) {
        for (AtomPinId pin_id : atom_netlist_.net_sinks(net_id)) {
            AtomBlockId blk_id = atom_netlist_.pin_block(pin_id);
            VTR_ASSERT(blk_id);

            if (!cluster_legalizer.is_atom_clustered(blk_id)) {
                /* TODO: Gain function accurate only if net has one connection to block,
                 * TODO: Should we handle case where net has multi-connection to block?
                 *       Gain computation is only off by a bit in this case */
                if (cluster_gain_stats.connection_gain.count(blk_id) == 0) {
                    cluster_gain_stats.connection_gain[blk_id] = 0;
                }

                if (num_internal_connections > 1) {
                    cluster_gain_stats.connection_gain[blk_id] -= 1 / (float)(num_open_connections + 1.5 * num_stuck_connections + 1 + 0.1);
                }
                cluster_gain_stats.connection_gain[blk_id] += 1 / (float)(num_open_connections + 1.5 * num_stuck_connections + 0.1);
            }
        }
    }

    if (net_relation_to_clustered_block == e_net_relation_to_clustered_block::INPUT) {
        /*Calculate the connection_gain for the atom block which is driving *
         *the atom net that is an input to an atom block in the cluster */

        AtomPinId driver_pin_id = atom_netlist_.net_driver(net_id);
        AtomBlockId blk_id = atom_netlist_.pin_block(driver_pin_id);

        if (!cluster_legalizer.is_atom_clustered(blk_id)) {
            if (cluster_gain_stats.connection_gain.count(blk_id) == 0) {
                cluster_gain_stats.connection_gain[blk_id] = 0;
            }
            if (num_internal_connections > 1) {
                cluster_gain_stats.connection_gain[blk_id] -= 1 / (float)(num_open_connections + 1.5 * num_stuck_connections + 0.1 + 1);
            }
            cluster_gain_stats.connection_gain[blk_id] += 1 / (float)(num_open_connections + 1.5 * num_stuck_connections + 0.1);
        }
    }
}

void GreedyCandidateSelector::update_timing_gain_values(
    ClusterGainStats& cluster_gain_stats,
    AtomNetId net_id,
    const ClusterLegalizer& cluster_legalizer,
    e_net_relation_to_clustered_block net_relation_to_clustered_block) {

    /*This function is called when the timing_gain values on the atom net
     *net_id requires updating.   */

    /* Check if this atom net lists its driving atom block twice.  If so, avoid  *
     * double counting this atom block by skipping the first (driving) pin. */
    auto pins = atom_netlist_.net_pins(net_id);
    if (net_output_feeds_driving_block_input_.count(net_id) != 0)
        pins = atom_netlist_.net_sinks(net_id);

    // Get the setup timing info used to compute timing gain terms.
    const SetupTimingInfo& timing_info = pre_cluster_timing_manager_.get_timing_info();

    if (net_relation_to_clustered_block == e_net_relation_to_clustered_block::OUTPUT
        && !is_global_.count(net_id)) {
        for (AtomPinId pin_id : pins) {
            AtomBlockId blk_id = atom_netlist_.pin_block(pin_id);
            if (!cluster_legalizer.is_atom_clustered(blk_id)) {
                double timing_gain = timing_info.setup_pin_criticality(pin_id);

                if (cluster_gain_stats.timing_gain.count(blk_id) == 0) {
                    cluster_gain_stats.timing_gain[blk_id] = 0;
                }
                if (timing_gain > cluster_gain_stats.timing_gain[blk_id])
                    cluster_gain_stats.timing_gain[blk_id] = timing_gain;
            }
        }
    }

    if (net_relation_to_clustered_block == e_net_relation_to_clustered_block::INPUT
        && !is_global_.count(net_id)) {
        /*Calculate the timing gain for the atom block which is driving *
         *the atom net that is an input to a atom block in the cluster */
        AtomPinId driver_pin = atom_netlist_.net_driver(net_id);
        AtomBlockId new_blk_id = atom_netlist_.pin_block(driver_pin);

        if (!cluster_legalizer.is_atom_clustered(new_blk_id)) {
            for (AtomPinId pin_id : atom_netlist_.net_sinks(net_id)) {
                double timing_gain = timing_info.setup_pin_criticality(pin_id);

                if (cluster_gain_stats.timing_gain.count(new_blk_id) == 0) {
                    cluster_gain_stats.timing_gain[new_blk_id] = 0;
                }
                if (timing_gain > cluster_gain_stats.timing_gain[new_blk_id])
                    cluster_gain_stats.timing_gain[new_blk_id] = timing_gain;
            }
        }
    }
}

void GreedyCandidateSelector::update_total_gain(ClusterGainStats& cluster_gain_stats,
                                                AttractionInfo& attraction_groups) {
    AttractGroupId cluster_att_grp_id = cluster_gain_stats.attraction_grp_id;

    for (AtomBlockId blk_id : cluster_gain_stats.marked_blocks) {
        //Initialize connection_gain and sharing_gain if
        //they have not previously been updated for the block
        if (cluster_gain_stats.connection_gain.count(blk_id) == 0) {
            cluster_gain_stats.connection_gain[blk_id] = 0;
        }
        if (cluster_gain_stats.sharing_gain.count(blk_id) == 0) {
            cluster_gain_stats.sharing_gain[blk_id] = 0;
        }
        if (cluster_gain_stats.timing_gain.count(blk_id) == 0) {
            cluster_gain_stats.timing_gain[blk_id] = 0;
        }

        AttractGroupId atom_grp_id = attraction_groups.get_atom_attraction_group(blk_id);
        if (atom_grp_id != AttractGroupId::INVALID() && atom_grp_id == cluster_att_grp_id) {
            //increase gain of atom based on attraction group gain
            float att_grp_gain = attraction_groups.get_attraction_group_gain(atom_grp_id);
            cluster_gain_stats.gain[blk_id] += att_grp_gain;
        }

        /* Todo: This was used to explore different normalization options, can
         * be made more efficient once we decide on which one to use */
        int num_used_input_pins = atom_netlist_.block_input_pins(blk_id).size();
        int num_used_output_pins = atom_netlist_.block_output_pins(blk_id).size();
        /* end todo */

        /* Calculate area-only cost function */
        int num_used_pins = num_used_input_pins + num_used_output_pins;
        VTR_ASSERT(num_used_pins > 0);
        if (packer_opts_.connection_driven) {
            /*try to absorb as many connections as possible*/
            cluster_gain_stats.gain[blk_id] = ((1 - packer_opts_.connection_gain_weight)
                                                   * (float)cluster_gain_stats.sharing_gain[blk_id]
                                               + packer_opts_.connection_gain_weight * (float)cluster_gain_stats.connection_gain[blk_id])
                                              / (num_used_pins);
        } else {
            cluster_gain_stats.gain[blk_id] = ((float)cluster_gain_stats.sharing_gain[blk_id])
                                              / (num_used_pins);
        }

        /* Add in timing driven cost into cost function */
        if (packer_opts_.timing_driven) {
            cluster_gain_stats.gain[blk_id] = packer_opts_.timing_gain_weight
                                                  * cluster_gain_stats.timing_gain[blk_id]
                                              + (1.0 - packer_opts_.timing_gain_weight) * (float)cluster_gain_stats.gain[blk_id];
        }
    }
}

void GreedyCandidateSelector::update_cluster_gain_stats_candidate_failed(
    ClusterGainStats& cluster_gain_stats,
    PackMoleculeId failed_mol_id) {
    VTR_ASSERT(failed_mol_id.is_valid());
    auto got = cluster_gain_stats.mol_failures.find(failed_mol_id);
    if (got == cluster_gain_stats.mol_failures.end()) {
        cluster_gain_stats.mol_failures.insert({failed_mol_id, 1});
    } else {
        got->second++;
    }
}

PackMoleculeId GreedyCandidateSelector::get_next_candidate_for_cluster(
    ClusterGainStats& cluster_gain_stats,
    LegalizationClusterId cluster_id,
    const ClusterLegalizer& cluster_legalizer,
    AttractionInfo& attraction_groups) {
    /* Finds the block with the greatest gain that satisfies the
     * input, clock and capacity constraints of a cluster that are
     * passed in.  If no suitable block is found it returns nullptr.
     */

    /*
     * This routine populates a list of feasible blocks outside the cluster,
     * then returns the best candidate for the cluster.
     * If there are no feasible blocks it returns a nullptr.
     */

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

    // 1. Find unpacked molecules based on criticality and strong connectedness (connected by low fanout nets) with current cluster
    if (cluster_gain_stats.initial_search_for_feasible_blocks) {
        cluster_gain_stats.initial_search_for_feasible_blocks = false;
        add_cluster_molecule_candidates_by_connectivity_and_timing(cluster_gain_stats,
                                                                   cluster_id,
                                                                   cluster_legalizer,
                                                                   attraction_groups);
        cluster_gain_stats.has_done_connectivity_and_timing = true;
    }

    if (packer_opts_.prioritize_transitive_connectivity) {
        // 2. Find unpacked molecules based on transitive connections (eg. 2 hops away) with current cluster
        if (cluster_gain_stats.feasible_blocks.empty() && cluster_gain_stats.explore_transitive_fanout) {
            add_cluster_molecule_candidates_by_transitive_connectivity(cluster_gain_stats,
                                                                       cluster_id,
                                                                       cluster_legalizer,
                                                                       attraction_groups);
        }

        // 3. Find unpacked molecules based on weak connectedness (connected by high fanout nets) with current cluster
        if (cluster_gain_stats.feasible_blocks.empty() && cluster_gain_stats.tie_break_high_fanout_net) {
            add_cluster_molecule_candidates_by_highfanout_connectivity(cluster_gain_stats,
                                                                       cluster_id,
                                                                       cluster_legalizer,
                                                                       attraction_groups);
        }
    } else { //Reverse order
        // 3. Find unpacked molecules based on weak connectedness (connected by high fanout nets) with current cluster
        if (cluster_gain_stats.feasible_blocks.empty() && cluster_gain_stats.tie_break_high_fanout_net) {
            add_cluster_molecule_candidates_by_highfanout_connectivity(cluster_gain_stats,
                                                                       cluster_id,
                                                                       cluster_legalizer,
                                                                       attraction_groups);
        }

        // 2. Find unpacked molecules based on transitive connections (eg. 2 hops away) with current cluster
        if (cluster_gain_stats.feasible_blocks.empty() && cluster_gain_stats.explore_transitive_fanout) {
            add_cluster_molecule_candidates_by_transitive_connectivity(cluster_gain_stats,
                                                                       cluster_id,
                                                                       cluster_legalizer,
                                                                       attraction_groups);
        }
    }

    // 4. Find unpacked molecules based on attraction group of the current cluster (if the cluster has an attraction group)
    if (cluster_gain_stats.feasible_blocks.empty()) {
        add_cluster_molecule_candidates_by_attraction_group(cluster_gain_stats,
                                                            cluster_id,
                                                            cluster_legalizer,
                                                            attraction_groups);
    }

    /* Grab highest gain molecule */
    PackMoleculeId best_molecule = PackMoleculeId::INVALID();
    // If there are feasible blocks being proposed and the number of suggestions did not reach the limit.
    // Get the block with highest gain from the top of the priority queue.
    if (!cluster_gain_stats.feasible_blocks.empty() && !cluster_gain_stats.current_stage_candidates_proposed_limit_reached()) {
        best_molecule = cluster_gain_stats.feasible_blocks.pop().first;
        VTR_ASSERT(best_molecule != PackMoleculeId::INVALID());
        cluster_gain_stats.num_candidates_proposed++;
        VTR_ASSERT(!cluster_legalizer.is_mol_clustered(best_molecule));
    }

    // If we have no feasible blocks, or we have reached the limit of number of pops,
    // then we need to clear the feasible blocks list and reset the number of pops.
    // This ensures that we can continue searching for feasible blocks for the remaining
    // steps (2.transitive, 3.high fanout, 4.attraction group).
    if (cluster_gain_stats.feasible_blocks.empty() || cluster_gain_stats.current_stage_candidates_proposed_limit_reached()) {
        cluster_gain_stats.feasible_blocks.clear();
        cluster_gain_stats.num_candidates_proposed = 0;
    }

    // If we are allowing unrelated clustering and no molecule has been found,
    // get unrelated candidate for cluster.
    if (allow_unrelated_clustering_ && best_molecule == PackMoleculeId::INVALID()) {
        const t_appack_options& appack_options = appack_ctx_.appack_options;
        if (appack_options.use_appack) {
            if (num_unrelated_clustering_attempts_ < appack_options.max_unrelated_clustering_attempts) {
                best_molecule = get_unrelated_candidate_for_cluster_appack(cluster_gain_stats,
                                                                           cluster_id,
                                                                           cluster_legalizer);
                num_unrelated_clustering_attempts_++;
            }
        } else {
            if (num_unrelated_clustering_attempts_ < max_unrelated_clustering_attempts_) {
                best_molecule = get_unrelated_candidate_for_cluster(cluster_id,
                                                                    cluster_legalizer);
                num_unrelated_clustering_attempts_++;
            }
        }
        VTR_LOGV(best_molecule && log_verbosity_ > 2,
                 "\tFound unrelated molecule to cluster\n");
    } else {
        VTR_LOGV(!best_molecule && log_verbosity_ > 2,
                 "\tNo related molecule found and unrelated clustering disabled\n");
    }

    return best_molecule;
}

void GreedyCandidateSelector::add_cluster_molecule_candidates_by_connectivity_and_timing(
    ClusterGainStats& cluster_gain_stats,
    LegalizationClusterId legalization_cluster_id,
    const ClusterLegalizer& cluster_legalizer,
    AttractionInfo& attraction_groups) {

    cluster_gain_stats.explore_transitive_fanout = true;                                  /* If no legal molecules found, enable exploration of molecules two hops away */
    cluster_gain_stats.candidates_propose_limit = packer_opts_.feasible_block_array_size; // set the limit of candidates to propose

    for (AtomBlockId blk_id : cluster_gain_stats.marked_blocks) {
        // Get the molecule that contains this block.
        PackMoleculeId molecule_id = prepacker_.get_atom_molecule(blk_id);
        // Add the molecule as a candidate if the molecule is not clustered and
        // is compatible with this cluster (using simple checks).
        if (!cluster_legalizer.is_mol_clustered(molecule_id) && cluster_legalizer.is_molecule_compatible(molecule_id, legalization_cluster_id)) {
            add_molecule_to_pb_stats_candidates(molecule_id,
                                                cluster_gain_stats,
                                                cluster_legalizer.get_cluster_type(legalization_cluster_id),
                                                attraction_groups,
                                                prepacker_,
                                                atom_netlist_,
                                                appack_ctx_);
        }
    }
}

void GreedyCandidateSelector::add_cluster_molecule_candidates_by_transitive_connectivity(
    ClusterGainStats& cluster_gain_stats,
    LegalizationClusterId legalization_cluster_id,
    const ClusterLegalizer& cluster_legalizer,
    AttractionInfo& attraction_groups) {
    //TODO: For now, only done by fan-out; should also consider fan-in
    cluster_gain_stats.explore_transitive_fanout = false;
    cluster_gain_stats.candidates_propose_limit = std::min(packer_opts_.feasible_block_array_size, AAPACK_MAX_TRANSITIVE_EXPLORE); // set the limit of candidates to propose

    /* First time finding transitive fanout candidates therefore alloc and load them */
    load_transitive_fanout_candidates(cluster_gain_stats,
                                      legalization_cluster_id,
                                      cluster_legalizer);

    /* Only consider candidates that pass a very simple legality check */
    for (const auto& transitive_candidate : cluster_gain_stats.transitive_fanout_candidates) {
        PackMoleculeId molecule_id = transitive_candidate.second;
        if (!cluster_legalizer.is_mol_clustered(molecule_id) && cluster_legalizer.is_molecule_compatible(molecule_id, legalization_cluster_id)) {
            add_molecule_to_pb_stats_candidates(molecule_id,
                                                cluster_gain_stats,
                                                cluster_legalizer.get_cluster_type(legalization_cluster_id),
                                                attraction_groups,
                                                prepacker_,
                                                atom_netlist_,
                                                appack_ctx_);
        }
    }
}

void GreedyCandidateSelector::add_cluster_molecule_candidates_by_highfanout_connectivity(
    ClusterGainStats& cluster_gain_stats,
    LegalizationClusterId legalization_cluster_id,
    const ClusterLegalizer& cluster_legalizer,
    AttractionInfo& attraction_groups) {
    /* Because the packer ignores high fanout nets when marking what blocks
     * to consider, use one of the ignored high fanout net to fill up lightly
     * related blocks */

    AtomNetId net_id = cluster_gain_stats.tie_break_high_fanout_net;
    cluster_gain_stats.candidates_propose_limit = std::min(packer_opts_.feasible_block_array_size, AAPACK_MAX_TRANSITIVE_EXPLORE); // set the limit of candidates to propose

    int count = 0;
    for (AtomPinId pin_id : atom_netlist_.net_pins(net_id)) {
        if (count >= AAPACK_MAX_HIGH_FANOUT_EXPLORE) {
            break;
        }

        AtomBlockId blk_id = atom_netlist_.pin_block(pin_id);

        PackMoleculeId molecule_id = prepacker_.get_atom_molecule(blk_id);
        if (!cluster_legalizer.is_mol_clustered(molecule_id) && cluster_legalizer.is_molecule_compatible(molecule_id, legalization_cluster_id)) {
            add_molecule_to_pb_stats_candidates(molecule_id,
                                                cluster_gain_stats,
                                                cluster_legalizer.get_cluster_type(legalization_cluster_id),
                                                attraction_groups,
                                                prepacker_,
                                                atom_netlist_,
                                                appack_ctx_);
            count++;
        }
    }
    cluster_gain_stats.tie_break_high_fanout_net = AtomNetId::INVALID(); /* Mark off that this high fanout net has been considered */
}

void GreedyCandidateSelector::add_cluster_molecule_candidates_by_attraction_group(
    ClusterGainStats& cluster_gain_stats,
    LegalizationClusterId legalization_cluster_id,
    const ClusterLegalizer& cluster_legalizer,
    AttractionInfo& attraction_groups) {
    auto cluster_type = cluster_legalizer.get_cluster_type(legalization_cluster_id);

    /*
     * For each cluster, we want to explore the attraction group molecules as potential
     * candidates for the cluster a limited number of times. This limit is imposed because
     * if the cluster belongs to a very large attraction group, we could potentially search
     * through its attraction group molecules for a very long time.
     * Defining a number of times to search through the attraction groups (i.e. number of
     * attraction group pulls) determines how many times we search through the cluster's attraction
     * group molecules for candidate molecules.
     */
    AttractGroupId grp_id = cluster_gain_stats.attraction_grp_id;
    cluster_gain_stats.candidates_propose_limit = packer_opts_.feasible_block_array_size; // set the limit of candidates to propose
    if (grp_id == AttractGroupId::INVALID()) {
        return;
    }

    AttractionGroup& group = attraction_groups.get_attraction_group_info(grp_id);
    std::vector<AtomBlockId> available_atoms;
    for (AtomBlockId atom_id : group.group_atoms) {
        LogicalModelId atom_model = atom_netlist_.block_model(atom_id);
        VTR_ASSERT(atom_model.is_valid());
        VTR_ASSERT(!primitive_candidate_block_types_[atom_model].empty());
        const auto& candidate_types = primitive_candidate_block_types_[atom_model];

        //Only consider molecules that are unpacked and of the correct type
        if (!cluster_legalizer.is_atom_clustered(atom_id)
            && std::find(candidate_types.begin(), candidate_types.end(), cluster_type) != candidate_types.end()) {
            available_atoms.push_back(atom_id);
        }
    }

    int num_available_atoms = available_atoms.size();
    if (num_available_atoms == 0) {
        return;
    }

    if (num_available_atoms < attraction_group_num_atoms_threshold_) {
        for (AtomBlockId atom_id : available_atoms) {
            //Only consider molecules that are unpacked and of the correct type
            PackMoleculeId molecule_id = prepacker_.get_atom_molecule(atom_id);
            if (!cluster_legalizer.is_mol_clustered(molecule_id) && cluster_legalizer.is_molecule_compatible(molecule_id, legalization_cluster_id)) {
                add_molecule_to_pb_stats_candidates(molecule_id,
                                                    cluster_gain_stats,
                                                    cluster_legalizer.get_cluster_type(legalization_cluster_id),
                                                    attraction_groups,
                                                    prepacker_,
                                                    atom_netlist_,
                                                    appack_ctx_);
            }
        }
        return;
    }

    for (int j = 0; j < attraction_group_num_atoms_threshold_; j++) {
        //Get a random atom between 0 and the number of available atoms - 1
        int selected_atom = rng_.irand(num_available_atoms - 1);

        AtomBlockId blk_id = available_atoms[selected_atom];

        //Only consider molecules that are unpacked and of the correct type
        PackMoleculeId molecule_id = prepacker_.get_atom_molecule(blk_id);
        if (!cluster_legalizer.is_mol_clustered(molecule_id) && cluster_legalizer.is_molecule_compatible(molecule_id, legalization_cluster_id)) {
            add_molecule_to_pb_stats_candidates(molecule_id,
                                                cluster_gain_stats,
                                                cluster_legalizer.get_cluster_type(legalization_cluster_id),
                                                attraction_groups,
                                                prepacker_,
                                                atom_netlist_,
                                                appack_ctx_);
        }
    }
}

/*
 * @brief Add blk to list of feasible blocks sorted according to gain.
 */
static void add_molecule_to_pb_stats_candidates(PackMoleculeId molecule_id,
                                                ClusterGainStats& cluster_gain_stats,
                                                t_logical_block_type_ptr cluster_type,
                                                AttractionInfo& attraction_groups,
                                                const Prepacker& prepacker,
                                                const AtomNetlist& atom_netlist,
                                                const APPackContext& appack_ctx) {

    // If using APPack, before adding this molecule to the candidates, check to
    // see if the molecule is too far away from the position of the cluster.
    // If so, do not add it to the list of candidates.
    if (appack_ctx.appack_options.use_appack) {
        // Get the max dist for this block type.
        float max_dist = appack_ctx.max_distance_threshold_manager.get_max_dist_threshold(*cluster_type);

        // If the distance from the cluster to the candidate is too large,
        // do not add this molecule to the list of candidates.
        const t_flat_pl_loc mol_loc = get_molecule_pos(molecule_id,
                                                       prepacker,
                                                       appack_ctx);
        float dist = get_manhattan_distance(mol_loc, cluster_gain_stats.flat_cluster_position);
        if (dist > max_dist)
            return;
    }

    int num_molecule_failures = 0;

    AttractGroupId cluster_att_grp = cluster_gain_stats.attraction_grp_id;

    /* When the clusterer packs with attraction groups the goal is to
     * pack more densely. Removing failed molecules to make room for the exploration of
     * more molecules helps to achieve this purpose.
     */
    if (attraction_groups.num_attraction_groups() > 0) {
        VTR_ASSERT(molecule_id.is_valid());
        auto got = cluster_gain_stats.mol_failures.find(molecule_id);
        if (got == cluster_gain_stats.mol_failures.end()) {
            num_molecule_failures = 0;
        } else {
            num_molecule_failures = got->second;
        }

        if (num_molecule_failures > 0) {
            remove_molecule_from_pb_stats_candidates(molecule_id, cluster_gain_stats);
            return;
        }
    }

    // if already in queue, do nothing
    if (cluster_gain_stats.feasible_blocks.contains(molecule_id)) {
        return;
    }

    for (std::pair<PackMoleculeId, float>& feasible_block : cluster_gain_stats.feasible_blocks.heap) {
        VTR_ASSERT_DEBUG(get_molecule_gain(feasible_block.first, cluster_gain_stats, cluster_att_grp, attraction_groups, num_molecule_failures, prepacker, atom_netlist, appack_ctx) == feasible_block.second);
    }

    // Insert the molecule into the queue sorted by gain, and maintain the heap property
    float molecule_gain = get_molecule_gain(molecule_id, cluster_gain_stats, cluster_att_grp, attraction_groups, num_molecule_failures, prepacker, atom_netlist, appack_ctx);
    cluster_gain_stats.feasible_blocks.push(molecule_id, molecule_gain);
}

/*
 * @brief Remove blk from list of feasible blocks sorted according to gain.
 *
 * Useful for removing blocks that are repeatedly failing. If a block
 * has been found to be illegal, we don't repeatedly consider it.
 */
static void remove_molecule_from_pb_stats_candidates(PackMoleculeId molecule_id,
                                                     ClusterGainStats& cluster_gain_stats) {
    cluster_gain_stats.feasible_blocks.remove_at_pop_time(molecule_id);
}

/*
 * @brief Get gain of packing molecule into current cluster.
 *
 * gain is equal to:
 * total_block_gain
 * + molecule_base_gain*some_factor
 * - introduced_input_nets_of_unrelated_blocks_pulled_in_by_molecule*some_other_factor
 */
static float get_molecule_gain(PackMoleculeId molecule_id,
                               ClusterGainStats& cluster_gain_stats,
                               AttractGroupId cluster_attraction_group_id,
                               AttractionInfo& attraction_groups,
                               int num_molecule_failures,
                               const Prepacker& prepacker,
                               const AtomNetlist& atom_netlist,
                               const APPackContext& appack_ctx) {
    VTR_ASSERT(molecule_id.is_valid());
    const t_pack_molecule& molecule = prepacker.get_molecule(molecule_id);

    float gain = 0;
    constexpr float attraction_group_penalty = 0.1;

    int num_introduced_inputs_of_indirectly_related_block = 0;
    for (AtomBlockId blk_id : molecule.atom_block_ids) {
        if (!blk_id.is_valid())
            continue;

        if (cluster_gain_stats.gain.count(blk_id) > 0) {
            gain += cluster_gain_stats.gain[blk_id];
        } else {
            /* This block has no connection with current cluster, penalize molecule for having this block
             */
            for (auto pin_id : atom_netlist.block_input_pins(blk_id)) {
                auto net_id = atom_netlist.pin_net(pin_id);
                VTR_ASSERT(net_id);

                auto driver_pin_id = atom_netlist.net_driver(net_id);
                VTR_ASSERT(driver_pin_id);

                auto driver_blk_id = atom_netlist.pin_block(driver_pin_id);

                num_introduced_inputs_of_indirectly_related_block++;
                for (AtomBlockId blk_id_2 : molecule.atom_block_ids) {
                    if (blk_id_2.is_valid() && driver_blk_id == blk_id_2) {
                        //valid block which is driver (and hence not an input)
                        num_introduced_inputs_of_indirectly_related_block--;
                        break;
                    }
                }
            }
        }
        AttractGroupId atom_grp_id = attraction_groups.get_atom_attraction_group(blk_id);
        if (atom_grp_id == cluster_attraction_group_id && cluster_attraction_group_id != AttractGroupId::INVALID()) {
            float att_grp_gain = attraction_groups.get_attraction_group_gain(atom_grp_id);
            gain += att_grp_gain;
        } else if (cluster_attraction_group_id != AttractGroupId::INVALID() && atom_grp_id != cluster_attraction_group_id) {
            gain -= attraction_group_penalty;
        }
    }

    gain += molecule.base_gain * 0.0001; /* Use base gain as tie breaker TODO: need to sweep this value and perhaps normalize */
    gain -= num_introduced_inputs_of_indirectly_related_block * (0.001);

    if (num_molecule_failures > 0 && attraction_groups.num_attraction_groups() > 0) {
        gain -= 0.1 * num_molecule_failures;
    }

    // If using APPack, attenuate the gain.
    // NOTE: We do not perform gain attenuation if the current cluster is a memory.
    //       It was found that attenuation caused RAMs to have issues packing.
    // FIXME: What to do when the gain is negative? Should we divide by the attenuation?
    // Also what happens when we try to merge in atoms from the flat placement
    // which are not connected to anything inside the cluster?
    const t_appack_options& appack_options = appack_ctx.appack_options;
    if (appack_options.use_appack && gain > 0.f && !cluster_gain_stats.is_memory) {
        // Get the position of the molecule
        t_flat_pl_loc target_loc = get_molecule_pos(molecule_id, prepacker, appack_ctx);

        // Compute the gain attenuatation term.
        float dist = get_manhattan_distance(cluster_gain_stats.flat_cluster_position, target_loc);
        float gain_mult = 1.0f;
        if (dist < appack_options.dist_th) {
            gain_mult = 1.0f - (appack_options.quad_fac_sqr * dist * dist);
        } else {
            gain_mult = 1.0f / std::sqrt(dist - appack_options.sqrt_offset);
        }
        VTR_ASSERT_SAFE(gain_mult >= 0.0f && gain_mult <= 1.0f);

        // Update the gain.
        gain *= gain_mult;
    }

    return gain;
}

void GreedyCandidateSelector::load_transitive_fanout_candidates(
    ClusterGainStats& cluster_gain_stats,
    LegalizationClusterId legalization_cluster_id,
    const ClusterLegalizer& cluster_legalizer) {
    // iterate over all the nets that have pins in this cluster
    for (AtomNetId net_id : cluster_gain_stats.marked_nets) {
        // only consider small nets to constrain runtime
        if (int(atom_netlist_.net_pins(net_id).size()) > packer_opts_.transitive_fanout_threshold)
            continue;

        // iterate over all the pins of the net
        for (AtomPinId pin_id : atom_netlist_.net_pins(net_id)) {
            AtomBlockId atom_blk_id = atom_netlist_.pin_block(pin_id);
            // get the transitive cluster
            LegalizationClusterId tclb = cluster_legalizer.get_atom_cluster(atom_blk_id);
            // Only consider blocks connected to this pin that are packed in
            // another cluster.
            if (tclb == legalization_cluster_id || tclb == LegalizationClusterId::INVALID())
                continue;

            // explore transitive nets from already packed cluster
            for (AtomNetId tnet : clb_inter_blk_nets_[tclb]) {
                // iterate over all the pins of the net
                for (AtomPinId tpin : atom_netlist_.net_pins(tnet)) {
                    AtomBlockId blk_id = atom_netlist_.pin_block(tpin);
                    // Ignore blocks which have already been packed.
                    if (cluster_legalizer.is_atom_clustered(blk_id))
                        continue;

                    // This transitive atom is not packed, score and add
                    auto& transitive_fanout_candidates = cluster_gain_stats.transitive_fanout_candidates;

                    if (cluster_gain_stats.gain.count(blk_id) == 0) {
                        cluster_gain_stats.gain[blk_id] = 0.001;
                    } else {
                        cluster_gain_stats.gain[blk_id] += 0.001;
                    }
                    PackMoleculeId molecule_id = prepacker_.get_atom_molecule(blk_id);
                    VTR_ASSERT(!cluster_legalizer.is_mol_clustered(molecule_id));
                    const t_pack_molecule& molecule = prepacker_.get_molecule(molecule_id);
                    transitive_fanout_candidates.insert({molecule.atom_block_ids[molecule.root], molecule_id});
                }
            }
        }
    }
}

PackMoleculeId GreedyCandidateSelector::get_unrelated_candidate_for_cluster(
    LegalizationClusterId cluster_id,
    const ClusterLegalizer& cluster_legalizer) {
    // Necessary data structures are only allocated in unrelated clustering is
    // on.
    VTR_ASSERT(allow_unrelated_clustering_);

    /*
     * TODO: Analyze if this function is useful in more detail, also, should
     *       probably not include clock in input count
     */

    size_t inputs_avail = cluster_legalizer.get_num_cluster_inputs_available(cluster_id);
    if (inputs_avail >= unrelated_clustering_data_.size()) {
        inputs_avail = unrelated_clustering_data_.size() - 1;
    }

    for (int ext_inps = inputs_avail; ext_inps >= 0; ext_inps--) {
        // Get the molecule by the number of external inputs.
        PackMoleculeId molecule = PackMoleculeId::INVALID();
        for (PackMoleculeId mol_id : unrelated_clustering_data_[ext_inps]) {
            /* TODO: Get better candidate atom block in future, eg. return most timing critical or some other smarter metric */
            if (!cluster_legalizer.is_mol_clustered(mol_id)) {
                /* TODO: I should be using a better filtering check especially when I'm
                 * dealing with multiple clock/multiple global reset signals where the clock/reset
                 * packed in matters, need to do later when I have the circuits to check my work */
                if (cluster_legalizer.is_molecule_compatible(mol_id, cluster_id)) {
                    molecule = mol_id;
                    break;
                }
            }
        }
        // If a molecule could be found, return it.
        if (molecule.is_valid())
            return molecule;
    }

    // If no molecule could be found, return nullptr.
    return PackMoleculeId::INVALID();
}

PackMoleculeId GreedyCandidateSelector::get_unrelated_candidate_for_cluster_appack(
    ClusterGainStats& cluster_gain_stats,
    LegalizationClusterId cluster_id,
    const ClusterLegalizer& cluster_legalizer) {

    /**
     * For APPack, we want to find a close candidate with the highest number
     * of available inputs which could be packed into the given cluster.
     * We will search for candidates in a BFS manner, where we will search in
     * the same 1x1 grid location of the cluster for a compatible candidate, and
     * will then search out if none can be found.
     *
     * Here, a molecule is compatible if:
     *  - It has not been clustered already
     *  - The number of inputs it has available is less than or equal to the
     *    number of inputs available in the cluster.
     *  - It has not tried to be packed in this cluster before.
     *  - It is compatible with the cluster.
     */

    VTR_ASSERT_MSG(allow_unrelated_clustering_,
                   "Cannot get unrelated candidates when unrelated clustering "
                   "is disabled");

    VTR_ASSERT_MSG(appack_ctx_.appack_options.use_appack,
                   "APPack is disabled, cannot get unrelated clusters using "
                   "flat placement information");

    // The cluster will likely have more inputs available than a single molecule
    // would have available (clusters have more pins). Clamp the inputs available
    // to the max number of inputs a molecule could have.
    size_t inputs_avail = cluster_legalizer.get_num_cluster_inputs_available(cluster_id);
    VTR_ASSERT_SAFE(!appack_unrelated_clustering_data_.empty());
    size_t max_molecule_inputs_avail = appack_unrelated_clustering_data_[0][0].size() - 1;
    if (inputs_avail >= max_molecule_inputs_avail) {
        inputs_avail = max_molecule_inputs_avail;
    }

    // Create a queue of locations to search and a map of visited grid locations.
    std::queue<t_flat_pl_loc> search_queue;
    vtr::NdMatrix<bool, 2> visited({appack_unrelated_clustering_data_.dim_size(0),
                                    appack_unrelated_clustering_data_.dim_size(1)},
                                   false);
    // Push the position of the cluster to the queue.
    search_queue.push(cluster_gain_stats.flat_cluster_position);

    while (!search_queue.empty()) {
        // Pop a position to search from the queue.
        const t_flat_pl_loc& node_loc = search_queue.front();
        VTR_ASSERT_SAFE(node_loc.layer == 0);

        // If this position is too far from the source, skip it.
        float dist = get_manhattan_distance(node_loc, cluster_gain_stats.flat_cluster_position);
        if (dist > 1) {
            search_queue.pop();
            continue;
        }

        // If this position has been visited, skip it.
        if (visited[node_loc.x][node_loc.y]) {
            search_queue.pop();
            continue;
        }
        visited[node_loc.x][node_loc.y] = true;

        // Explore this position from highest number of inputs available to lowest.
        const auto& uc_data = appack_unrelated_clustering_data_[node_loc.x][node_loc.y];
        VTR_ASSERT_SAFE(inputs_avail < uc_data.size());
        for (int ext_inps = inputs_avail; ext_inps >= 0; ext_inps--) {
            // Get the molecule by the number of external inputs.
            for (PackMoleculeId mol_id : uc_data[ext_inps]) {
                // If this molecule has been clustered, skip it.
                if (cluster_legalizer.is_mol_clustered(mol_id))
                    continue;
                // If this molecule has tried to be packed before and failed
                // do not try it. This also means that this molecule may be
                // related to this cluster in some way.
                if (cluster_gain_stats.mol_failures.find(mol_id) != cluster_gain_stats.mol_failures.end())
                    continue;
                // If this molecule is not compatible with the current cluster
                // skip it.
                if (!cluster_legalizer.is_molecule_compatible(mol_id, cluster_id))
                    continue;
                // Return this molecule as the unrelated candidate.
                return mol_id;
            }
        }

        // Push the neighbors of the position to the queue.
        // Note: Here, we are using the manhattan distance, so we do not push
        //       the diagonals. We also want to try the direct neighbors first
        //       since they should be closer.
        if (node_loc.x >= 1.0f)
            search_queue.push({node_loc.x - 1, node_loc.y, node_loc.layer});
        if (node_loc.x <= visited.dim_size(0) - 2)
            search_queue.push({node_loc.x + 1, node_loc.y, node_loc.layer});
        if (node_loc.y >= 1.0f)
            search_queue.push({node_loc.x, node_loc.y - 1, node_loc.layer});
        if (node_loc.y <= visited.dim_size(1) - 2)
            search_queue.push({node_loc.x, node_loc.y + 1, node_loc.layer});

        // Pop the position off the queue.
        search_queue.pop();
    }

    // No molecule could be found. Return an invalid ID.
    return PackMoleculeId::INVALID();
}

void GreedyCandidateSelector::update_candidate_selector_finalize_cluster(
    ClusterGainStats& cluster_gain_stats,
    LegalizationClusterId cluster_id) {
    // store info that will be used later in packing.
    for (const AtomNetId mnet_id : cluster_gain_stats.marked_nets) {
        int external_terminals = atom_netlist_.net_pins(mnet_id).size() - cluster_gain_stats.num_pins_of_net_in_pb[mnet_id];
        // Check if external terminals of net is within the fanout limit and
        // that there exists external terminals.
        if (external_terminals < packer_opts_.transitive_fanout_threshold && external_terminals > 0) {
            // TODO: This should really not use the cluster_id, it can be a bit
            //       dangerous since the legalizer may change the IDs of any
            //       cluster if it wants. Maybe store this information in the
            //       legalizer instead.
            clb_inter_blk_nets_[cluster_id].push_back(mnet_id);
        }
    }
}
