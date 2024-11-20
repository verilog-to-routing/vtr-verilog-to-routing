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
     *              for each logical block type.
     *  @param is_clock
     *              The set of clock nets in the Atom Netlist.
     *  @param is_global
     *              The set of global nets in the Atom Netlist.
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
     *              atoms are pre-packed into before clustering.
     *  @param allow_unrelated_clustering
     *              Allows primitives which have no attraction to the given
     *              cluster to be packed into it.
     *  @param balance_block_type_utilization
     *              When true, tries to create clusters that balance the logical
     *              block type utilization.
     *  @param attraction_groups
     *              Information on the attraction groups used during the
     *              clustering process.
     *
     *  @return num_used_type_instances
     *              The number of used logical block types by the clustering.
     *              This information may be useful when detecting if the
     *              clustering can fit on the device.
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

    const t_packer_opts& packer_opts_;
    const t_analysis_opts& analysis_opts_;
    const AtomNetlist& atom_netlist_;
    const t_arch* arch_ = nullptr;
    const t_pack_high_fanout_thresholds& high_fanout_thresholds_;
    const std::unordered_set<AtomNetId>& is_clock_;
    const std::unordered_set<AtomNetId>& is_global_;

    /// @brief Pre-computed logical block types for each model in the architecture.
    std::map<const t_model*, std::vector<t_logical_block_type_ptr>> primitive_candidate_block_types_;
};

