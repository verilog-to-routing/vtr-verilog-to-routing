#pragma once
/**
 * @file
 * @brief Relative placement macro support for the packer.
 *
 * Each group in UserRelativeMacros becomes one placement macro member:
 *  - All atoms in a group must share one cluster.
 *  - Each cluster may contain atoms from at most one group.
 *
 * RelativeMacroPacker validates chain ownership at construction and checks
 * these rules during cluster legalization. ClusterLegalizer creates one only
 * when the design has relative macros.
 */

#include <map>

#include "atom_netlist_fwd.h"
#include "prepack.h"
#include "user_relative_macros.h"

class AtomNetlist;
class ClusterLegalizer;

/**
 * @brief Relative placement state stored in LegalizationCluster.
 */
struct t_cluster_relative_state {
    UserRelativeGroupId group; ///< Group whose constrained atoms are in this cluster.

    UserRelativeGroupId long_chain_owner; ///< Owner of the long chain; invalid if unconstrained or absent.
};

/**
 * @brief Check if adding a molecule creates conflicting placement constraints.
 *
 * A long chain (e.g., a carry chain) can span multiple clusters. The placer
 * treats these clusters as one placement macro with fixed offsets.
 *
 * If an RPM group shares a cluster with a long chain, their placement macros
 * are merged. To avoid conflicting offsets, the long chain can only contain
 * constrained atoms from one RPM group (its owner).
 *
 * This check prevents mixing different RPM groups through a long chain.
 * Chains that fit in one cluster and unconstrained chains are unaffected.
 *
 * @param molecule_is_long_chain    Whether the molecule belongs to a long chain.
 * @param molecule_chain_owner      RPM group owning the molecule's chain, if any.
 * @param cluster_group             RPM group in the cluster after adding the molecule.
 * @param cluster_has_long_chain    Whether the cluster already contains a long chain.
 * @param cluster_long_chain_owner  RPM group owning the cluster's long chain, if any.
 *
 * @return True if adding the molecule does not create conflicting constraints.
 */
bool long_chain_ownership_allows(bool molecule_is_long_chain,
                                 UserRelativeGroupId molecule_chain_owner,
                                 UserRelativeGroupId cluster_group,
                                 bool cluster_has_long_chain,
                                 UserRelativeGroupId cluster_long_chain_owner);

/**
 * @brief Result of checking whether a molecule can join a cluster.
 *
 * evaluate_molecule() produces this result for the legalizer's compatibility
 * check, pin utilization check, and state update.
 */
struct t_relative_macro_verdict {
    bool allowed = true; ///< Whether relative placement rules allow the addition.

    t_cluster_relative_state cluster_state; ///< Cluster's state after adding the molecule; assign it to the cluster on success.

    bool molecule_in_cluster_group = false; ///< Group member; bypass pin utilization target to keep the group together.
};

/**
 * @brief Enforces the relative placement macro rules during packing.
 */
class RelativeMacroPacker {
  public:
    /**
     * @brief Initialize packing checks for the given relative macros and netlist.
     *
     * Rejects molecules and chains spanning multiple groups with a fatal error
     * and records each chain's owning group.
     */
    RelativeMacroPacker(const UserRelativeMacros& relative_macros,
                        const Prepacker& prepacker,
                        const AtomNetlist& atom_netlist);

    /**
     * @brief Return the molecule's group, or an invalid group if unconstrained.
     *
     * Prepacking and construction-time validation ensure its constrained atoms
     * share one group.
     */
    UserRelativeGroupId molecule_group(PackMoleculeId molecule_id) const;

    /**
     * @brief Decide whether a molecule may be added to a cluster.
     *
     * Checks that the cluster contains at most one group and respects long chain
     * ownership.
     *
     *  @param molecule_id             The molecule to add.
     *  @param cluster_state           The cluster's current relative macro state.
     *  @param cluster_has_long_chain  Whether the cluster already holds part of a long chain.
     *  @param log_verbosity           Packer log verbosity; > 3 logs group changes and conflicts.
     */
    t_relative_macro_verdict evaluate_molecule(PackMoleculeId molecule_id,
                                               const t_cluster_relative_state& cluster_state,
                                               bool cluster_has_long_chain,
                                               int log_verbosity) const;

  private:
    /**
     * @brief Record each chain's owning group: the group of its first constrained atom.
     */
    void init_chain_owners_();

    /**
     * @brief Reject molecules or chains spanning multiple groups with a fatal error.
     *
     * Prepacking prevents non-chain molecules from spanning groups, so only chains
     * can fail this check.
     *
     * Unconstrained chain atoms may share the group's cluster or extend into nearby
     * clusters. PlaceMacros merges those clusters with the group's placement macro.
     */
    void validate_molecule_groups_() const;

    /**
     * @brief Return the chain's owning group, or an invalid group if unconstrained.
     */
    UserRelativeGroupId chain_owner_(MoleculeChainId chain_id) const;

    /**
     * @brief Check long chain ownership and log any conflict.
     *
     * new_state holds the cluster's group after adding the molecule. On success,
     * if the molecule is part of a long chain, its owner is recorded in new_state.
     */
    bool check_long_chain_ownership_(PackMoleculeId molecule_id,
                                     bool cluster_has_long_chain,
                                     t_cluster_relative_state& new_state,
                                     int log_verbosity) const;

    const UserRelativeMacros& relative_macros_; ///< User-defined relative placement macros.

    const Prepacker& prepacker_; ///< Prepacked molecules and chains.

    const AtomNetlist& atom_netlist_; ///< Atom netlist being packed.

    std::map<MoleculeChainId, UserRelativeGroupId> chain_owners_; ///< Owning group of each chain with constrained atoms.
};

/**
 * @brief Report relative placement groups whose atoms occupy multiple clusters.
 *
 * Call after the final packing pass. All atoms in each group must share a
 * cluster so the group can become a single placement macro member.
 *
 * If packing otherwise succeeded, report split groups with a fatal error.
 * If packing failed, log a warning and let the caller report the packing error.
 * Do nothing if no groups are split.
 *
 *  @param cluster_legalizer     The cluster legalizer holding the final packing result.
 *  @param relative_macros       User-defined relative placement macros.
 *  @param atom_netlist          The atom netlist being packed.
 *  @param clustering_succeeded  Whether the packing pass otherwise succeeded.
 */
void report_split_relative_groups(const ClusterLegalizer& cluster_legalizer,
                                  const UserRelativeMacros& relative_macros,
                                  const AtomNetlist& atom_netlist,
                                  bool clustering_succeeded);
