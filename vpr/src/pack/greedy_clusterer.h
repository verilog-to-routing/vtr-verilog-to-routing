/**
 * @file
 * @author  Alex Singer
 * @date    November 2024
 * @brief   The declarations of the Greedy Clusterer class which is used to
 *          encapsulate the process of greedy clustering.
 */

#pragma once

#include <map>
#include <unordered_set>
#include "physical_types.h"

// Forward declarations
class AtomNetId;
class AtomNetlist;
class AttractionInfo;
class ClusterLegalizer;
class Prepacker;
struct t_analysis_opts;
struct t_clustering_data;
struct t_pack_high_fanout_thresholds;
struct t_packer_opts;

/**
 * @brief A clusterer that generates clusters by greedily choosing the clusters
 *        which appear to have the best gain for a given neighbor.
 *
 * This clusterer generates one cluster at a time by finding candidate molecules
 * and selecting the molecule with the highest gain.
 *
 * The clusterer takes an Atom Netlist which has be pre-packed into pack
 * patterns (e.g. carry chains) as input and produces a set of legal clusters
 * of these pack molecules as output. Legality here means that it was able to
 * find a valid intra-lb route for the inputs of the clusters, through the
 * internal molecules, and to the outputs of the clusters.
 */
class GreedyClusterer {
public:
    /**
     * @brief Constructor of the Greedy Clusterer class.
     *
     * The clusterer may be invoked many times during the packing flow. This
     * constructor will pre-compute information before clustering which can
     * improve the performance of the clusterer.
     *
     *  @param packer_opts
     *              Options passed by the user to configure the packing and
     *              clustering algorithms.
     *  @param analysis_opts
     *              Options passed by the user to configure timing analysis in
     *              the clusterer.
     *  @param atom_netlist
     *              The atom netlist to cluster over.
     *  @param arch
     *              The architecture to cluster over.
     *  @param high_fanout_thresholds
     *              The thresholds for what to consider as a high-fanout net
     *              for each logical block type. The clusterer will not consider
     *              nets with fanout higher than this to be important in
     *              candidate block selection (gain computation).
     *              A reason for it being per block type is that some blocks,
     *              like RAMs, have weak gains to other RAM primitives due to
     *              fairly high fanout address nets, so a higher fanout
     *              threshold for them is useful in generating a more dense
     *              packing.
     *  @param is_clock
     *              The set of clock nets in the Atom Netlist.
     *  @param is_global
     *              The set of global nets in the Atom Netlist. These will be
     *              routed on special dedicated networks, and hence are less
     *              relavent to locality / attraction.
     */
    GreedyClusterer(const t_packer_opts& packer_opts,
                    const t_analysis_opts& analysis_opts,
                    const AtomNetlist& atom_netlist,
                    const t_arch* arch,
                    const t_pack_high_fanout_thresholds& high_fanout_thresholds,
                    const std::unordered_set<AtomNetId>& is_clock,
                    const std::unordered_set<AtomNetId>& is_global);

    /**
     * @brief Performs clustering on the pack molecules formed by the prepacker.
     *
     * The clustering is contained within the Cluster Legalizer.
     *
     *  @param cluster_legalizer
     *              The cluster legalizer which is used to create clusters and
     *              grow clusters by adding molecules to a cluster.
     *  @param prepacker
     *              The prepacker object which contains the pack molecules that
     *              are atoms which are pre-packed before the main clustering
     *              (due to pack patterns, e.g. carry chains).
     *  @param allow_unrelated_clustering
     *              Allows primitives which have no attraction to the given
     *              cluster to be packed into it. This can lead to a denser
     *              packing, but tends to be bad for wirelength and timing.
     *  @param balance_block_type_utilization
     *              When true, tries to create clusters that balance the logical
     *              block type utilization. This is useful when some primitives
     *              have multiple logical block types to which they can cluster,
     *              e.g. multiple sizes of physical RAMs exist on the chip.
     *  @param attraction_groups
     *              Information on the attraction groups used during the
     *              clustering process. These are groups of primitives that have
     *              extra attraction to each other; currently they are used to
     *              guide the clusterer when it must cluster some parts of a
     *              design densely due to user placement/floorplanning
     *              constraints. They are created if some floorplan regions are
     *              overfilled after a clustering attempt.
     *
     *  @return num_used_type_instances
     *              The number of used logical blocks of each type by the
     *              clustering. This information may be useful when detecting
     *              if the clustering can fit on the device.
     */
    std::map<t_logical_block_type_ptr, size_t>
    do_clustering(ClusterLegalizer& cluster_legalizer,
                  Prepacker& prepacker,
                  bool allow_unrelated_clustering,
                  bool balance_block_type_utilization,
                  AttractionInfo& attraction_groups);

private:
    /*
     * When attraction groups are created, the purpose is to pack more densely by adding more molecules
     * from the cluster's attraction group to the cluster. In a normal flow, (when attraction groups are
     * not on), the cluster keeps being packed until the get_molecule routines return either a repeated
     * molecule or a nullptr. When attraction groups are on, we want to keep exploring molecules for the
     * cluster until a nullptr is returned. So, the number of repeated molecules is changed from 1 to 500,
     * effectively making the clusterer pack a cluster until a nullptr is returned.
     */
    static constexpr int attraction_groups_max_repeated_molecules_ = 500;

    /// @brief The packer options used to configure the clusterer.
    const t_packer_opts& packer_opts_;

    /// @brief The analysis options used to configure timing analysis within the
    ///        clusterer.
    const t_analysis_opts& analysis_opts_;

    /// @brief The atom netlist to cluster over.
    const AtomNetlist& atom_netlist_;

    /// @brief The device architecture to cluster onto.
    const t_arch* arch_ = nullptr;

    /// @brief The high-fanout thresholds per logical block type. Used to ignore
    ///        certain nets when calculating the gain for the next candidate
    ///        molecule to cluster.
    const t_pack_high_fanout_thresholds& high_fanout_thresholds_;

    /// @brief A set of atom nets which are considered as clocks.
    const std::unordered_set<AtomNetId>& is_clock_;

    /// @brief A set of atom nets which are considered as global nets.
    const std::unordered_set<AtomNetId>& is_global_;

    /// @brief Pre-computed logical block types for each model in the architecture.
    std::map<const t_model*, std::vector<t_logical_block_type_ptr>> primitive_candidate_block_types_;
};

