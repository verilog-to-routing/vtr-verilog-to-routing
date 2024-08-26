#pragma once

#include <set>
#include <unordered_map>
#include "atom_netlist_fwd.h"
#include "noc_data_types.h"
#include "partition_region.h"
#include "vpr_types.h"
#include "vtr_range.h"
#include "vtr_strong_id.h"
#include "vtr_vector.h"
#include "vtr_vector_map.h"

class Prepacker;
class t_pb_graph_node;
struct t_lb_router_data;

// A special ID to identify the legalization clusters. This is separate from the
// ClusterBlockId since this legalizer is not necessarily tied to the Clustered
// netlist, but is used as a sub-routine to it.
struct legalization_cluster_id_tag;
typedef vtr::StrongId<legalization_cluster_id_tag, size_t> LegalizationClusterId;

/// @brief The different legalization strategies the cluster legalizer can perform.
///
/// Allows the user of the API to select how thorough the legalizer should be
/// when adding molecules into clusters.
enum class ClusterLegalizationStrategy {
    FULL,                   // Run the full legalizer (including intra-lb routing)
    SKIP_INTRA_LB_ROUTE     // Do all legality checks except intra-lb routing
};

/// @brief The status of the cluster legalization.
enum class e_block_pack_status {
    BLK_PASSED,                 // Passed legalization.
    BLK_FAILED_FEASIBLE,        // Failed due to block not feasibly being able to go in the cluster.
    BLK_FAILED_ROUTE,           // Failed due to intra-lb routing failure.
    BLK_FAILED_FLOORPLANNING,   // Failed due to not being compatible with the cluster's current PartitionRegion.
    BLK_FAILED_NOC_GROUP,       // Failed due to not being compatible with the cluster's NoC group.
    BLK_STATUS_UNDEFINED        // Undefined status. Something went wrong.
};

/*
 * @brief A struct containing information about the cluster.
 *
 * This contains necessary information for legalizing a cluster.
 */
struct LegalizationCluster {
    /// @brief A list of the molecules in the cluster. By design, a cluster will
    ///        only contain molecules which have been previously legalized into
    ///        the cluster using a legalization strategy.
    std::set<t_pack_molecule*> molecules;

    /// @brief The logical block of this cluster.
    /// TODO: We should be more careful with how this is allocated. Instead of
    ///       pointers, we really should use IDs and store them in a standard
    ///       container.
    t_pb* pb;

    /// @brief The logical block type this cluster represents.
    t_logical_block_type_ptr type;

    /// @brief The partition region of legal positions this cluster can be placed.
    ///        Used to detect if a molecule can physically be placed in a cluster.
    ///        It is derived from the partition region constraints on the atoms
    ///        in the cluster (not fundamental but good for performance).
    PartitionRegion pr;

    /// @brief The NoC group that this cluster is a part of. Is used to check if
    ///        a candidate primitive is in the same NoC group as the atom blocks
    ///        that have already been added to the primitive. This can be helpful
    ///        for optimization.
    NocGroupId noc_grp_id;

    /// @brief The router data of the intra lb router used for this cluster.
    ///        Contains information about the atoms in the cluster and how they
    ///        can be routed within.
    t_lb_router_data* router_data;
};

/*
 * @brief A manager class which manages the legalization of cluster. As clusters
 *        are created, this class will legalize for each molecule added. It also
 *        provides methods which are helpful for clustering.
 *
 * Usage:
 * The ClusterLegalizer class maintains the clusters within itself since the
 * legalization of a cluster depends on the molecules which have already been
 * inserted into the clusters prior.
 *
 * The class provides different legalization strategies the user may use to
 * legalize:
 *  1) SKIP_INTRA_LB_ROUTE
 *  2) FULL
 *
 * 1) SKIP_INTRA_LB_ROUTE Legalization Strategy Example:
 * This strategy will not fully route the interal connections of the clusters
 * until when the user specifies. An example of how to use this strategy would
 * look something like this. Note, this example is simplified and the result
 * of the packings should be checked and handled.
 *
 * ClusterLegalizer legalizer(...);
 *
 * std::tie(status, new_cluster_id) = legalizer.start_new_cluster(seed_mol,
 *                                                                cluster_type,
 *                                                                mode);
 * for mol in molecules_to_add:
 *      // Cheaper additions, but may pack a molecule that wouldnt route.
 *      status = legalizer.add_mol_to_cluster(mol, new_cluster_id);
 *      if (status != e_block_pack_status::BLK_PASSED)
 *          break;
 *
 * // Do the expensive check once all molecules are in.
 * if (!legalizer.check_cluster_legality(new_cluster_id))
 *      // Destroy the illegal cluster.
 *      legalizer.destroy_cluster(new_cluster_id);
 *      legalizer.compress();
 *      // Handle how to try again (maybe use FULL strategy).
 *
 * 2) FULL Legalization Strategy Example:
 * This strategy will fully route the internal connections of the clusters for
 * each molecule added. This is much more expensive to run; however, will ensure
 * that the cluster is fully legalized while it is being created. An example
 * of how to sue this strategy would look something like this:
 *
 * Clusterlegalizer legalizer(...);
 *
 * std::tie(pack_result, new_cluster_id) = legalizer.start_new_cluster(seed_mol,
 *                                                                     cluster_type,
 *                                                                     mode);
 * for mol in molecules_to_add:
 *      // Do the expensive check for each molecule added.
 *      status = legalizer.add_mol_to_cluster(mol, new_cluster_id);
 *      if (status != e_block_pack_status::BLK_PASSED)
 *          break;
 *
 * // new_cluster_id now contains a fully legalized cluster.
 */
class ClusterLegalizer {
public:
    // Iterator for the legalization cluster IDs
    typedef typename vtr::vector_map<LegalizationClusterId, LegalizationClusterId>::const_iterator cluster_iterator;

    // Range for the legalization cluster IDs
    typedef typename vtr::Range<cluster_iterator> cluster_range;

private:

    /*
     * @brief Helper method that tries to pack the given molecule into a cluster.
     *
     * This method runs all the legality checks specified by the legalization
     * strategy. If the molecule can be packed into the cluster, it will insert
     * it into the cluster.
     *
     *  @param molecule                 The molecule to insert into the cluster.
     *  @param cluster                  The cluster to try to insert the molecule into.
     *  @param cluster_id               The ID of the cluster.
     *  @param max_external_pin_util    The max external pin utilization for a
     *                                  cluster of this type.
     */
    e_block_pack_status try_pack_molecule(t_pack_molecule* molecule,
                                          LegalizationCluster& cluster,
                                          LegalizationClusterId cluster_id,
                                          const t_ext_pin_util& max_external_pin_util);

public:

    // Explicitly deleted default constructor. Need to use other constructor to
    // initialize state correctly.
    ClusterLegalizer() = delete;

    /*
     * @brief Initialize the ClusterLegalizer class.
     *
     * Allocates internal state.
     */
    ClusterLegalizer(const AtomNetlist& atom_netlist,
                     const Prepacker& prepacker,
                     const std::vector<t_logical_block_type>& logical_block_types,
                     std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
                     size_t num_models,
                     const std::vector<std::string>& target_external_pin_util_str,
                     const t_pack_high_fanout_thresholds& high_fanout_thresholds,
                     ClusterLegalizationStrategy cluster_legalization_strategy,
                     bool enable_pin_feasibility_filter,
                     int feasible_block_array_size,
                     int log_verbosity);

    // This class allocates and deallocates memory within. This class should not
    // be copied or moved to prevent it from double freeing / losing pointers.
    ClusterLegalizer(const ClusterLegalizer&) = delete;
    ClusterLegalizer& operator=(const ClusterLegalizer&) = delete;

    /*
     * @brief Start a new legalization cluster with the given molecule.
     *
     *  @param molecule         The seed molecule used to start the new cluster.
     *  @param cluster_type     The type of the cluster to start.
     *  @param cluster_mode     The mode of the new cluster for the given type.
     *
     *  @return     A pair for the status of the packing and the ID of the new
     *              cluster. If the new cluster could not be created, the pack
     *              status will return the reason and the ID would be invalid.
     */
    std::tuple<e_block_pack_status, LegalizationClusterId>
    start_new_cluster(t_pack_molecule* molecule,
                      t_logical_block_type_ptr cluster_type,
                      int cluster_mode);

    /*
     * @brief Add an unclustered molecule to the given legalization cluster.
     *
     * If the addition was unsuccessful, the molecule will remain unclustered.
     *
     *  @param molecule         The molecule to add to the cluster.
     *  @param cluster_id       The ID of the cluster to add the molecule to.
     *
     *  @return     The status of the pack (if the addition was successful and
     *              if not why).
     */
    e_block_pack_status add_mol_to_cluster(t_pack_molecule* molecule,
                                           LegalizationClusterId cluster_id);

    /*
     * @brief Destroy the given cluster.
     *
     * This unclusters all molecules in the cluster so they can be re-clustered
     * into different clusters. Should call the compress() method after destroying
     * one or more clusters.
     *
     *  @param cluster_id       The ID of the cluster to destroy.
     */
    void destroy_cluster(LegalizationClusterId cluster_id);

    /*
     * @brief Compress the internal storage of clusters. Should be called after
     *        a cluster is destroyed.
     *
     * Similar to the Netlist compress method. Will invalidate all Legalization
     * Cluster IDs.
     *
     * This method can be quite expensive, so it is a good idea to batch many
     * cluster destructions and then compress at the end.
     */
    void compress();

    /*
     * @brief A list of all cluster IDs in the legalizer.
     *
     * If the legalizer has been compressed (or no clusters have been destroyed)
     * then all cluster IDs in this list will be valid and represent a non-empty
     * legalization cluster.
     */
    cluster_range clusters() const {
        return vtr::make_range(legalization_cluster_ids_.begin(), legalization_cluster_ids_.end());
    }

    /*
     * @brief Check that the given cluster is fully legal.
     *
     * This method runs an intra_lb_route on the given cluster. This ignores
     * the cluster legalization strategy set by the user. This method will not
     * correct the problematic molecules, it will only return true if the
     * cluster is legal and false if it is not.
     *
     *  @param cluster_id       The ID of the cluster to fully legalize.
     *
     *  @return                 True if the cluster is legal, false otherwise.
     */
    bool check_cluster_legality(LegalizationClusterId cluster_id);

    /*
     * @brief Cleans the cluster of unnessary data, reducing the memory footprint.
     *
     * After this function is called, no more molecules can be added to the
     * cluster. This method will ensure that the cluster has enough information
     * to generate a clustered netlist from the legalized clusters.
     *
     * Specifically, this frees the pb stats (which is used by the clusterer
     * to compute the gain) and the router data of the cluster.
     *
     * TODO: The pb stats should really not be calculated or stored in the
     *       cluster legalizer.
     *
     *  @param cluster_id   The ID of the cluster to clean.
     */
    void clean_cluster(LegalizationClusterId cluster_id);

    /*
     * @brief Verify that all atoms have been clustered into a cluster.
     *
     * This will not verify if all the clusters are fully legal.
     */
    void verify();

    /*
     * @brief Finalize the clustering.
     *
     * Before generating a Clustered Netlist, each cluster needs to allocate and
     * load a pb_route. This method will generate a pb_route for each cluster
     * and store it into the clusters' pb.
     */
    void finalize();

    /*
     * @brief Resets the legalizer to its initial state.
     *
     * Destroys all clusters and resets the cluster placement stats.
     */
    void reset();

    /// @brief Gets the top-level pb of the given cluster.
    inline t_pb* get_cluster_pb(LegalizationClusterId cluster_id) const {
        VTR_ASSERT_SAFE(cluster_id.is_valid() && (size_t)cluster_id < legalization_clusters_.size());
        const LegalizationCluster& cluster = legalization_clusters_[cluster_id];
        return cluster.pb;
    }

    /// @brief Gets the logical block type of the given cluster.
    inline t_logical_block_type_ptr get_cluster_type(LegalizationClusterId cluster_id) const {
        VTR_ASSERT_SAFE(cluster_id.is_valid() && (size_t)cluster_id < legalization_clusters_.size());
        const LegalizationCluster& cluster = legalization_clusters_[cluster_id];
        return cluster.type;
    }

    /// @brief Gets the current partition region (the intersection of all
    ///        contained atoms) of the given cluster.
    inline const PartitionRegion& get_cluster_pr(LegalizationClusterId cluster_id) const {
        VTR_ASSERT_SAFE(cluster_id.is_valid() && (size_t)cluster_id < legalization_clusters_.size());
        const LegalizationCluster& cluster = legalization_clusters_[cluster_id];
        return cluster.pr;
    }

    /// @brief Gets the ID of the cluster that contains the given atom block.
    inline LegalizationClusterId get_atom_cluster(AtomBlockId blk_id) const {
        VTR_ASSERT_SAFE(blk_id.is_valid() && (size_t)blk_id < atom_cluster_.size());
        return atom_cluster_[blk_id];
    }

    /// @brief Gets the cluster placement stats of the given cluster.
    inline t_cluster_placement_stats* get_cluster_placement_stats(LegalizationClusterId cluster_id) const {
        VTR_ASSERT_SAFE(cluster_id.is_valid() && (size_t)cluster_id < legalization_clusters_.size());
        return &(cluster_placement_stats_[get_cluster_type(cluster_id)->index]);
    }

    /// @brief Returns true if the given atom block has been packed into a
    ///        cluster, false otherwise.
    inline bool is_atom_clustered(AtomBlockId blk_id) const {
        // Simply, if the atom is not in an invalid cluster, it has been clustered.
        return get_atom_cluster(blk_id) != LegalizationClusterId::INVALID();
    }

    /// @brief Returns a reference to the target_external_pin_util object. This
    ///        allows the user to modify the external pin utilization if needed.
    inline t_ext_pin_util_targets& get_target_external_pin_util() {
        return target_external_pin_util_;
    }

    /// @bried Gets the max size a cluster could physically be.
    inline size_t get_max_cluster_size() const {
        return max_cluster_size_;
    }

    /*
     * @brief Set the legalization strategy of the cluster legalizer.
     *
     * This allows the strategy of the cluster legalizer to change based on the
     * needs of the user. For example, one can set the legalizer to use a more
     * relaxed strategy to insert a batch of molecules in cheaply, saving the
     * full legalizerion for the end (using check_cluster_legality).
     *
     *  @param strategy     The strategy to set the cluster legalizer to.
     */
    inline void set_legalization_strategy(ClusterLegalizationStrategy strategy) {
        cluster_legalization_strategy_ = strategy;
    }

    /*
     * @brief Set how verbose the log messages should be for the cluster legalizer.
     *
     * This allows the user to set the verbosity at different points for easier
     * usability.
     *
     * Set the verbosity to 4 to see most of the log messages on how the
     * molecules move through the legalizer.
     * Set the verbosity to 5 to see all the log messages in the legalizer.
     *
     *  @param verbosity    The value to set the verbosity to.
     */
    inline void set_log_verbosity(int verbosity) {
        log_verbosity_ = verbosity;
    }

    /// @brief Destructor of the class. Frees allocated data.
    ~ClusterLegalizer();

private:
    /// @brief A vector of the legalization cluster IDs. If any of them are
    ///        invalid, then that means that the cluster has been destroyed.
    vtr::vector_map<LegalizationClusterId, LegalizationClusterId> legalization_cluster_ids_;

    /// @brief Lookup table for which cluster each molecule is in.
    std::unordered_map<t_pack_molecule*, LegalizationClusterId> molecule_cluster_;

    /// @brief List of all legalization clusters.
    vtr::vector_map<LegalizationClusterId, LegalizationCluster> legalization_clusters_;

    /// @brief A lookup-table for which cluster the given atom is packed into.
    vtr::vector_map<AtomBlockId, LegalizationClusterId> atom_cluster_;

    /// @brief Stores the NoC group ID of each atom block. Atom blocks that
    ///        belong to different NoC groups can't be clustered with each other
    ///        into the same clustered block.
    vtr::vector<AtomBlockId, NocGroupId> atom_noc_grp_id_;

    /// @brief Stats keeper for placement information during packing/clustering.
    /// TODO: This should be a vector.
    t_cluster_placement_stats* cluster_placement_stats_ = nullptr;

    /// @brief The utilization of external input/output pins during packing
    ///        (between 0 and 1).
    t_ext_pin_util_targets target_external_pin_util_;

    /// @brief The max size of any molecule. This is used to allocate a dynamic
    ///        array within the legalizer, and in its current form this is a bit
    ///        expensive to calculate from the prepacker.
    size_t max_molecule_size_;

    /// @brief The max size a cluster could physically be. This is used to
    ///        allocate dynamic arrays.
    size_t max_cluster_size_;

    /// @brief A vector of routing resource nodes within each logical block type
    ///        [0 .. num_logical_block_types-1]
    /// TODO: This really should not be a pointer to a vector... I think this is
    ///       meant to be a vector of vectors...
    std::vector<t_lb_type_rr_node>* lb_type_rr_graphs_ = nullptr;

    /// @brief The total number of models (user + library) in the architecture.
    ///        Used to allocate space in dynamic data structures.
    size_t num_models_;

    /// @brief The current legalization strategy of the cluster legalizer.
    ClusterLegalizationStrategy cluster_legalization_strategy_;

    /// @brief Controls whether the pin counting feasibility filter is used
    ///        during clustering. When enabled the clustering engine counts the
    ///        number of available pins in groups/classes of mutually connected
    ///        pins within a cluster. These counts are used to quickly filter
    ///        out candidate primitives/atoms/molecules for which the cluster
    ///        has insufficient pins to route (without performing a full
    ///        routing). This reduces packing run-time. This matches the packer
    ///        option of the same name.
    bool enable_pin_feasibility_filter_;

    /// @brief The max size of the priority queue for candidates that pass the
    ///        early filter legality test but not the more detailed routing
    ///        filter. This matches the packer option of the same name.
    int feasible_block_array_size_;

    /// @brief Used to set the verbosity of log messages in the legalizer. Used
    ///        for debugging. When log_verbosity > 3, the legalizer will print
    ///        messages when a molecule is successful during legalization. When
    ///        log_verbosity is > 4, the legalizer will print when a molecule
    ///        fails a legality check. This parameter is also passed into the
    ///        intra-lb router.
    int log_verbosity_;

    /// @brief The prepacker object that stores the molecules which will be
    ///        legalized into clusters.
    const Prepacker& prepacker_;
};

