/**
 * @file
 * @author  Alex Singer
 * @date    January 2025
 * @brief   The declaration of the greedy candidate selector class which selects
 *          candidate molecules to pack into the given cluster. This class also
 *          maintains the gains of packing molecules into clusters.
 */

#pragma once

#include <map>
#include <unordered_set>
#include <vector>
#include "attraction_groups.h"
#include "cluster_legalizer.h"
#include "physical_types.h"
#include "prepack.h"
#include "vtr_ndmatrix.h"
#include "vtr_vector.h"
#include "vtr_random.h"

// Forward declarations
class AtomNetlist;
class AttractionInfo;
class FlatPlacementInfo;
class Prepacker;
class SetupTimingInfo;
class t_pack_high_fanout_thresholds;
struct t_model;
struct t_molecule_stats;
struct t_packer_opts;

/**
 * @brief Stats on the gain of a cluster.
 *
 * This contains information that is updated whenever a molecule is packed into
 * a cluster. This information is used to select candidate molecules to pack
 * into the given cluster.
 */
struct ClusterGainStats {
    /// @brief The seed molecule used to create this cluster.
    PackMoleculeId seed_molecule_id = PackMoleculeId::INVALID();

    /// @brief Has this cluster tried to get candidates by connectivity and
    ///        timing yet. This helps ensure that we only do that once per
    ///        cluster candidate proposal.
    bool has_done_connectivity_and_timing = false;

    /// @brief Attraction (inverse of cost) function.
    std::unordered_map<AtomBlockId, float> gain;

    /// @brief The timing criticality score of this atom.
    ///        Determined by the most critical atom net between this atom
    ///        and any atom in the current pb.
    std::unordered_map<AtomBlockId, float> timing_gain;
    /// @brief Weighted sum of connections to attraction function.
    std::unordered_map<AtomBlockId, float> connection_gain;
    /// @brief How many nets on an atom are already in the pb under
    ///        consideration.
    std::unordered_map<AtomBlockId, float> sharing_gain;

    /// @brief Stores the number of times molecules have failed to be packed
    ///        into the cluster.
    ///
    /// key: molecule id, value: number of times the molecule has failed to be
    ///                          packed into the cluster.
    std::unordered_map<PackMoleculeId, int> mol_failures;

    /// @brief List of nets with the num_pins_of_net_in_pb and gain entries
    ///        altered (i.e. have some gain-related connection to the current
    ///        cluster).
    std::vector<AtomNetId> marked_nets;
    /// @brief List of blocks with the num_pins_of_net_in_pb and gain entries altered.
    std::vector<AtomBlockId> marked_blocks;

    /// @brief If no marked candidate molecules, use this high fanout net to
    ///        determine the next candidate atom.
    AtomNetId tie_break_high_fanout_net;
    /// @brief If no marked candidate molecules and no high fanout nets to
    ///        determine next candidate molecule then explore molecules on
    ///        transitive fanout.
    bool explore_transitive_fanout;
    /// @brief Holding transitive fanout candidates key: root block id of the
    ///        molecule, value: pointer to the molecule.
    // TODO: This should be an unordered map, unless stability is desired.
    std::map<AtomBlockId, PackMoleculeId> transitive_fanout_candidates;

    /// @brief How many pins of each atom net are contained in the currently open pb?
    std::unordered_map<AtomNetId, int> num_pins_of_net_in_pb;

    /// @brief The attraction group associated with the cluster. Will be
    ///        AttractGroupId::INVALID() if no attraction group is associated
    ///        with the cluster.
    AttractGroupId attraction_grp_id;

    /// @brief Array of feasible blocks to select from [0..max_array_size-1]
    ///
    /// Sorted in ascending gain order so that the last cluster_ctx.blocks is
    /// the most desirable (this makes it easy to pop blocks off the list.
    std::vector<PackMoleculeId> feasible_blocks;
    int num_feasible_blocks;
};

/**
 * @brief A selector class which will propose good candidate molecules to pack
 *        into the given cluster. This is used to grow clusters in a greedy
 *        clusterer.
 *
 * In greedy clustering algorithms, clusters are grown by selecting the
 * candidate molecule with the highest gain to pack into it. This class
 * calculates and maintains the gains on clusters and their candidates to allow
 * it to select which candidate to try next.
 *
 * Usage:
 *
 * GreedyCandidateSelector candidate_selector(...);
 *
 * // ... (Start a new cluster using the cluster legalizer)
 *
 * // Create an object to hold the gain statistics for the new cluster.
 * ClusterGainStats cluster_gain_stats = candidate_selector.create_cluster_gain_stats(...);
 *
 * // Select a candidate to pack into the cluster using the gain stats.
 * PackMoleculeId candidate_mol = candidate_selector.get_next_candidate_for_cluster(cluster_gain_stats, ...);
 *
 * // ... (Try to pack the candidate into the cluster)
 *
 * // Update the cluster gain stats based on if the pack was successful or not.
 * if (pack succeeded):
 *   candidate_selector.update_cluster_gain_stats_candidate_success(cluster_gain_stats, candidate_mol, ...);
 * else:
 *   candidate_selector.candidate_selector.update_cluster_gain_stats_candidate_failed(cluster_gain_stats, candidate_mol);
 *
 * // Pick a new candidate and continue
 * candidate_mol = candidate_selector.get_next_candidate_for_cluster(cluster_gain_stats, ...);
 * ...
 *
 * // Once the cluster is fully packed, finalize the cluster.
 * candidate_selector.update_candidate_selector_finalize_cluster(cluster_gain_stats, ...);
 */
class GreedyCandidateSelector {
private:
    /// @brief How many unrelated candidates can be proposed and not clustered
    ///        in a row. So if an unrelated candidate is successfully clustered,
    ///        the counter is reset.
    static constexpr int max_unrelated_clustering_attempts_ = 1;

    /// @brief For high-fanout nets that are ignored, consider a maximum of this
    ///        many sinks, must be less than packer_opts.feasible_block_array_size.
    static constexpr int AAPACK_MAX_HIGH_FANOUT_EXPLORE = 10;

    /// @brief When investigating transitive fanout connections in packing,
    ///        consider a maximum of this many molecules, must be less than
    ///        packer_opts.feasible_block_array_size.
    static constexpr int AAPACK_MAX_TRANSITIVE_EXPLORE = 40;

    /// @brief When adding cluster molecule candidates by attraction groups,
    ///        only investigate this many candidates. Some attraction groups can
    ///        get very large; so this threshold decides when to explore all
    ///        atoms in the group, or a randomly selected number of them.
    static constexpr int attraction_group_num_atoms_threshold_ = 500;

public:
    ~GreedyCandidateSelector();

    /**
     * @brief Constructor of the Greedy Candidate Selector class. Pre-computes
     *        data used by the candidate selector.
     *
     *  @param atom_netlist
     *              The netlist of atoms to cluster.
     *  @param prepacker
     *              The prepacker used to generate pack-pattern molecules of the
     *              atoms in the netlist.
     *  @param packer_opts
     *              Options passed by the user to configure the packing
     *              algorithm. Changes how the candidates are selected.
     *  @param allow_unrelated_clustering
     *              Enables an algorithm in the selector to look for good
     *              candidates which are not necessarily connected to the
     *              cluster.
     *  @param max_molecule_stats
     *              The maximum stats over all molecules. Used for normalizing
     *              terms in the gain.
     *  @param primitive_candidate_block_types
     *              Candidate logical block types which are compatible with the
     *              given primitive model.
     *  @param high_fanout_thresholds
     *              The thresholds for what to consider as a high-fanout net
     *              for each logical block type. The clusterer will not consider
     *              nets with fanout higher than this to be important.
     *  @param is_clock
     *              The set of clock nets in the Atom Netlist.
     *  @param is_global
     *              The set of global nets in the Atom Netlist. These will be
     *              routed on special dedicated networks, and hence are less
     *              relavent to locality / attraction.
     *  @param net_output_feeds_driving_block_input
     *              The set of nets whose output feeds the block that drives
     *              itself. This may cause double-counting in the gain
     *              calculations and needs special handling.
     *  @param timing_info
     *              Setup timing info for this Atom Netlist. Used to incorporate
     *              timing / criticality into the gain calculation.
     *  @param flat_placement_info
     *              Placement information known about the atoms before they are
     *              clustered. If defined, helps inform the clusterer to build
     *              clusters.
     *  @param log_verbosity
     *              The verbosity of log messages in the candidate selector.
     */
    GreedyCandidateSelector(const AtomNetlist& atom_netlist,
                            const Prepacker& prepacker,
                            const t_packer_opts& packer_opts,
                            bool allow_unrelated_clustering,
                            const t_molecule_stats& max_molecule_stats,
                            const std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types,
                            const t_pack_high_fanout_thresholds& high_fanout_thresholds,
                            const std::unordered_set<AtomNetId>& is_clock,
                            const std::unordered_set<AtomNetId>& is_global,
                            const std::unordered_set<AtomNetId>& net_output_feeds_driving_block_input,
                            const SetupTimingInfo& timing_info,
                            const FlatPlacementInfo& flat_placement_info,
                            int log_verbosity);

    /**
     * @brief Create and initialize the gain stats for the cluster with the
     *        given cluster_id which was created with the given cluster_seed_mol.
     *
     * Used when a seed is used to create a new cluster in a greedy clusterer.
     *
     * The returned ClusterGainStats object is used to store statistics on the
     * cluster for selecting the candidate with the highest gain and keep track
     * of which molecules have been tried.
     *
     *  @param cluster_seed_mol
     *              The seed molecule which was used to create the cluster.
     *  @param cluster_id
     *              The legalization cluster ID of the cluster.
     *  @param cluster_legalizer
     *              The legalizer used to create the cluster.
     *  @param attraction_groups
     *              Groups of primitives that have extra attraction to each
     *              other.
     */
    ClusterGainStats create_cluster_gain_stats(
                                    PackMoleculeId cluster_seed_mol_id,
                                    LegalizationClusterId cluster_id,
                                    const ClusterLegalizer& cluster_legalizer,
                                    AttractionInfo& attraction_groups);

    /**
     * @brief Update the cluster gain stats given that the successful_mol was
     *        packed successfully into the cluster.
     *
     * This marks and updates the gain stats for each block in the successfully
     * clustered molecule.
     *
     *  @param cluster_gain_stats
     *              The cluster gain stats to update.
     *  @param successful_mol
     *              The molecule which was successfully packed into the cluster.
     *  @param cluster_id
     *              The legalization cluster ID of the cluster.
     *  @param cluster_legalizer
     *              The legalizer used to create the cluster.
     *  @param attraction_groups
     *              Groups of primitives that have extra attraction to each
     *              other.
     */
    void update_cluster_gain_stats_candidate_success(
                                    ClusterGainStats& cluster_gain_stats,
                                    PackMoleculeId successful_mol_id,
                                    LegalizationClusterId cluster_id,
                                    const ClusterLegalizer& cluster_legalizer,
                                    AttractionInfo& attraction_groups);

    /**
     * @brief Update the cluster gain stats given that the failed_mol was not
     *        packed successfully into the cluster.
     *
     * This tracks the failed molecule to help decide future molecules to
     * select.
     *
     *  @param cluster_gain_stats
     *              The cluster gain stats to update.
     *  @param failed_mol
     *              The molecule that failed to pack into the cluster.
     */
    void update_cluster_gain_stats_candidate_failed(
                                        ClusterGainStats& cluster_gain_stats,
                                        PackMoleculeId failed_mol_id);

    /**
     * @brief Given the cluster_gain_stats, select the next candidate molecule
     *        to pack into the given cluster.
     *
     * This uses the gain stats to find the unclustered molecule which will
     * likely have the highest gain.
     *
     *  @param cluster_gain_stats
     *              The cluster gain stats maintained for this cluster.
     *  @param cluster_id
     *              The legalization cluster id for the cluster.
     *  @param cluster_legalizer
     *              The legalizer used to create the cluster.
     *  @param attraction_groups
     *              Groups of primitives that have extra attraction to each
     *              other.
     */
    PackMoleculeId get_next_candidate_for_cluster(
                                    ClusterGainStats& cluster_gain_stats,
                                    LegalizationClusterId cluster_id,
                                    const ClusterLegalizer& cluster_legalizer,
                                    AttractionInfo& attraction_groups);

    /**
     * @brief Finalize the creation of a cluster.
     *
     * This should be called after all molecules have been packed into a cluster.
     *
     * This updates internal lookup tables in the candidate selector. For
     * example, what inter-clb nets exist on a cluster are stored by this
     * routine to make later transistive gain function calculations more
     * efficient.
     *
     *  @param cluster_gain_stats
     *              The cluster gain stats for the cluster to finalize.
     *  @param cluster_id
     *              The legalization cluster id of the cluster to finalize.
     */
    void update_candidate_selector_finalize_cluster(
                                        ClusterGainStats& cluster_gain_stats,
                                        LegalizationClusterId cluster_id);

private:
    // ===================================================================== //
    //                      Cluster Gain Stats Updating
    // ===================================================================== //

    /**
     * @brief Flag used to decide if the gains of affected blocks should be
     *        updated when a block is marked.
     */
    enum class e_gain_update : bool {
        GAIN,           // Update the gains of affected blocks.
        NO_GAIN         // Do not update the gains of affected blocks.
    };

    /**
     * @brief Flag used to indicate if the net is an input or output when
     *        updating the connection gain values.
     */
    enum class e_net_relation_to_clustered_block : bool {
        INPUT,          // This is an input net.
        OUTPUT          // This is an output net.
    };

    /**
     * @brief Updates the marked data structures, and, if gain_flag is GAIN, the
     *        gain when an atom block is added to a cluster.
     */
    void mark_and_update_partial_gain(ClusterGainStats& cluster_gain_stats,
                                      AtomNetId net_id,
                                      e_gain_update gain_flag,
                                      AtomBlockId clustered_blk_id,
                                      const ClusterLegalizer& cluster_legalizer,
                                      int high_fanout_net_threshold,
                                      e_net_relation_to_clustered_block net_relation_to_clustered_block);

    /**
     * @brief Updates the connection_gain in the cluster_gain_stats.
     */
    void update_connection_gain_values(ClusterGainStats& cluster_gain_stats,
                                       AtomNetId net_id,
                                       AtomBlockId clustered_blk_id,
                                       const ClusterLegalizer& cluster_legalizer,
                                       e_net_relation_to_clustered_block net_relation_to_clustered_block);

    /**
     * Updates the timing_gain in the cluster_gain_stats.
     */
    void update_timing_gain_values(ClusterGainStats& cluster_gain_stats,
                                   AtomNetId net_id,
                                   const ClusterLegalizer& cluster_legalizer,
                                   e_net_relation_to_clustered_block net_relation_to_clustered_block);

    /**
     * @brief Updates the total gain array to reflect the desired tradeoff
     *        between input sharing (sharing_gain) and path_length minimization
     *        (timing_gain) input each time a new molecule is added to the
     *        cluster.
     */
    void update_total_gain(ClusterGainStats& cluster_gain_stats,
                           AttractionInfo& attraction_groups);

    // ===================================================================== //
    //                      Cluster Candidate Selection
    // ===================================================================== //

    /**
     * @brief Add molecules that are "close" to the seed molecule in the flat
     *        placement to the list of feasible blocks.
     */
    void add_cluster_molecule_candidates_by_flat_placement(
                                ClusterGainStats& cluster_gain_stats,
                                LegalizationClusterId legalization_cluster_id,
                                const ClusterLegalizer& cluster_legalizer,
                                AttractionInfo& attraction_groups);
    /*
     * @brief Add molecules with strong connectedness to the current cluster to
     *        the list of feasible blocks.
     */
    void add_cluster_molecule_candidates_by_connectivity_and_timing(
                                ClusterGainStats& cluster_gain_stats,
                                LegalizationClusterId legalization_cluster_id,
                                const ClusterLegalizer& cluster_legalizer,
                                AttractionInfo& attraction_groups);

    /**
     * @brief Score unclustered atoms that are two hops away from current
     *        cluster
     *
     * For example, consider a cluster that has a FF feeding an adder in another
     * cluster. Since this FF is feeding an adder that is packed in another
     * cluster this function should find other FFs that are feeding other inputs
     * of this adder since they are two hops away from the FF packed in this
     * cluster
     *
     * This is used when adding molecule candidates by transistive connectivity.
     */
    void load_transitive_fanout_candidates(
                                ClusterGainStats& cluster_gain_stats,
                                LegalizationClusterId legalization_cluster_id,
                                const ClusterLegalizer& cluster_legalizer);

    /*
     * @brief Add molecules based on transitive connections (eg. 2 hops away)
     *        with current cluster.
     */
    void add_cluster_molecule_candidates_by_transitive_connectivity(
                                ClusterGainStats& cluster_gain_stats,
                                LegalizationClusterId legalization_cluster_id,
                                const ClusterLegalizer& cluster_legalizer,
                                AttractionInfo& attraction_groups);

    /*
     * @brief Add molecules based on weak connectedness (connected by high
     *        fanout nets) with current cluster.
     */
    void add_cluster_molecule_candidates_by_highfanout_connectivity(
                                ClusterGainStats& cluster_gain_stats,
                                LegalizationClusterId legalization_cluster_id,
                                const ClusterLegalizer& cluster_legalizer,
                                AttractionInfo& attraction_groups);

    /*
     * @brief If the current cluster being packed has an attraction group
     *        associated with it (i.e. there are atoms in it that belong to an
     *        attraction group), this routine adds molecules from the associated
     *        attraction group to the list of feasible blocks for the cluster.
     *
     * Attraction groups can be very large, so we only add some randomly
     * selected molecules for efficiency if the number of atoms in the group is
     * greater than some threshold. Therefore, the molecules added to the
     * candidates will vary each time you call this function.
     */
    void add_cluster_molecule_candidates_by_attraction_group(
                                ClusterGainStats& cluster_gain_stats,
                                LegalizationClusterId legalization_cluster_id,
                                const ClusterLegalizer& cluster_legalizer,
                                AttractionInfo& attraction_groups);

    /**
     * @brief Finds a molecule to propose which is unrelated but may be good to
     *        cluster.
     */
    PackMoleculeId get_unrelated_candidate_for_cluster(
                                    LegalizationClusterId cluster_id,
                                    const ClusterLegalizer& cluster_legalizer);

    // ===================================================================== //
    //                      Internal Variables
    // ===================================================================== //

    /// @brief The atom netlist to cluster over.
    const AtomNetlist& atom_netlist_;

    /// @brief The prepacker used to pack atoms into molecule pack patterns.
    const Prepacker& prepacker_;

    /// @brief The packer options used to configure the clusterer.
    const t_packer_opts& packer_opts_;

    /// @brief Whether unrelated clustering should be performed or not.
    const bool allow_unrelated_clustering_;

    /// @brief The verbosity of log messages in the candidate selector.
    const int log_verbosity_;

    /// @brief Pre-computed vector of logical block types that could implement
    ///        the given model in the architecture.
    const std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types_;

    /// @brief The high-fanout thresholds per logical block type. Used to ignore
    ///        certain nets when calculating the gain for the next candidate
    ///        molecule to cluster.
    // TODO: This should really be a map from the logical block type to the
    //       threshold.
    const t_pack_high_fanout_thresholds& high_fanout_thresholds_;

    /// @brief A set of atom nets which are considered as clocks.
    const std::unordered_set<AtomNetId>& is_clock_;

    /// @brief A set of atom nets which are considered as global nets.
    const std::unordered_set<AtomNetId>& is_global_;

    /// @brief A set of atom nets which have outputs that feed the block that
    ///        drive them.
    const std::unordered_set<AtomNetId>& net_output_feeds_driving_block_input_;

    /// @brief Setup timing info used to help select critical candidates to pack.
    const SetupTimingInfo& timing_info_;

    /// @brief Inter-block nets within a finalized cluster. Used for finding
    ///        transitive candidates.
    vtr::vector<LegalizationClusterId, std::vector<AtomNetId>> clb_inter_blk_nets_;

    /// @brief Data pre-computed to help select unrelated molecules. This is a
    ///        list of list of molecules sorted by their gain, where the first
    ///        dimension is the number of external outputs of the molecule.
    std::vector<std::vector<PackMoleculeId>> unrelated_clustering_data_;

    /// @brief Placement information of the atoms in the netlist known before
    ///        packing.
    const FlatPlacementInfo& flat_placement_info_;

    /**
     * @brief Bins containing molecules in the same tile. Used for pre-computing
     *        flat placement information for clustering.
     */
    struct FlatTileMoleculeList {
        // A list of molecule in each sub_tile at this tile. Where each index
        // in the first dimension is the subtile [0, num_sub_tiles - 1].
        std::vector<std::vector<PackMoleculeId>> sub_tile_mols;

        // A list of molecules in the undefined sub-tile at this tile. An
        // undefined sub-tile is the location molecules go when the sub-tile
        // in the flat placement is unspecified for this atom.
        // Currently unused, but can be used to support that feature.
        std::vector<PackMoleculeId> undefined_sub_tile_mols;
    };

    /// @brief Pre-computed information on the flat placement. Lists all of the
    ///        molecules at the given location according to the flat placement
    ///        information. For example: flat_tile_placement[x][y][layer] would
    ///        return lists of molecules at each of the sub-tiles at that
    ///        location.
    vtr::NdMatrix<FlatTileMoleculeList, 3> flat_tile_placement_;

    /// @brief A count on the number of unrelated clustering attempts which
    ///        have been performed.
    int num_unrelated_clustering_attempts_ = 0;

    /// @brief Random number generator used by the clusterer. Currently this
    ///        is used only when selecting atoms from attraction groups, but
    ///        could be used for other purposes in the future.
    vtr::RngContainer rng_;
};

