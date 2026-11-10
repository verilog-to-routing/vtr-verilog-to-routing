#pragma once
/**
 * @file
 * @brief Packing checks and molecule ordering for user-defined relative
 *        placement macros.
 *
 * A relative placement group is a set of atoms that must share one cluster.
 * Different groups must use different clusters. A site lock requires an atom
 * to use a specific primitive within its cluster.
 *
 * The prepacker combines atoms into molecules, each with a root atom. Packing
 * chooses a primitive site for the root and uses the molecule's pack pattern
 * to place its other atoms.
 */

#include <string>
#include <vector>

#include "atom_netlist_fwd.h"
#include "cluster_legalizer_fwd.h"
#include "physical_types.h"
#include "prepack.h"
#include "user_relative_macros.h"
#include "vtr_vector.h"

class AtomNetlist;
class AtomPBBimap;
class ClusterLegalizer;
struct LegalizationCluster;

/**
 * @brief Group and long chain ownership recorded for a legalization cluster.
 */
struct t_cluster_relative_state {
    UserRelativeGroupId group; ///< Group with atoms in this cluster; invalid if none.

    UserRelativeGroupId long_chain_owner; ///< Group with atoms in the cluster's long chain; invalid if the chain is unconstrained or absent.
};

/**
 * @brief Check whether adding a molecule would mix incompatible long-chain owners.
 *
 * A long chain spans multiple clusters. Its owner is the relative placement
 * group containing its constrained atoms; a chain without constrained atoms
 * has no owner. If the resulting cluster belongs to group G, any long chain
 * in it must also be owned by G. Existing and incoming long chains must have
 * matching owners. These are group checks only; the legalizer separately
 * prohibits adding a second long-chain molecule to a cluster.
 *
 * @param molecule_is_long_chain    Whether the molecule belongs to a long chain.
 * @param molecule_chain_owner      Group owning the molecule's chain, if any.
 * @param cluster_group             Group in the cluster after adding the molecule.
 * @param cluster_has_long_chain    Whether the cluster already contains a long chain.
 * @param cluster_long_chain_owner  Group owning the cluster's long chain, if any.
 *
 * @return True if the addition respects long chain ownership.
 */
bool long_chain_ownership_allows(bool molecule_is_long_chain,
                                 UserRelativeGroupId molecule_chain_owner,
                                 UserRelativeGroupId cluster_group,
                                 bool cluster_has_long_chain,
                                 UserRelativeGroupId cluster_long_chain_owner);

/**
 * @brief Return whether a primitive site belongs to the given cluster type's
 *        pb graph.
 */
bool site_in_cluster_type(const t_pb_graph_node* site, t_logical_block_type_ptr cluster_type);

/**
 * @brief Relative placement compatibility and proposed state for a molecule
 *        being added to a cluster.
 */
struct t_relative_macro_verdict {
    bool allowed = true; ///< Whether relative placement rules allow the addition.

    t_cluster_relative_state cluster_state; ///< Group and chain owner to record if the addition is allowed and packing succeeds.

    bool molecule_in_cluster_group = false; ///< For an allowed addition, true if this molecule has group constraints; lets it use full pin utilization.
};

/**
 * @brief Helps the legalizer and greedy clusterer satisfy relative placement
 *        constraints during packing.
 *
 * Each relative placement group must occupy one cluster, distinct from the
 * clusters of other groups. This class checks group and chain compatibility,
 * supplies required primitive sites, and prioritizes group molecules so they
 * can be packed together. Final clustering checks verify the group and site
 * constraints.
 */
class RelativeMacroPacker {
  public:
    /**
     * @brief Validate relative placement constraints against the prepacked
     *        molecules and record chain ownership.
     *
     * Requires at least one macro. Rejects molecules or chains whose atoms
     * belong to different groups. Also rejects a site lock on a non-root atom
     * when the molecule's root has no site lock.
     */
    RelativeMacroPacker(const UserRelativeMacros& relative_macros,
                        const Prepacker& prepacker,
                        const AtomNetlist& atom_netlist);

    /**
     * @brief Return the group containing the molecule's constrained atoms,
     *        or an invalid group if it has none.
     *
     * Only this molecule's atoms are considered, even when it belongs to a
     * longer chain.
     */
    UserRelativeGroupId molecule_group(PackMoleculeId molecule_id) const;

    /**
     * @brief Check whether the molecule's group and chain may share this cluster.
     *
     * The returned cluster_state contains the group and chain owner to record
     * if packing succeeds. This function does not place the molecule or modify
     * the cluster.
     *
     *  @param molecule_id             The molecule to add.
     *  @param cluster_state           The cluster's current relative macro state.
     *  @param cluster_has_long_chain  Whether the cluster already holds part of a long chain.
     *  @param log_verbosity           Packer log verbosity; > 3 logs compatibility decisions.
     */
    t_relative_macro_verdict evaluate_molecule(PackMoleculeId molecule_id,
                                               const t_cluster_relative_state& cluster_state,
                                               bool cluster_has_long_chain,
                                               int log_verbosity) const;

    /**
     * @brief Return the primitive site the molecule's root atom is locked to,
     *        or nullptr if it is unlocked.
     *
     * The legalizer places the root at this site and uses the pack pattern to
     * place the remaining atoms. Locks on those atoms are checked after packing.
     */
    const t_pb_graph_node* molecule_locked_site(PackMoleculeId molecule_id) const;

    /**
     * @brief Log a rejected site-lock attempt, identifying any atom already
     *        occupying the required site.
     *
     * Used for verbose legalizer diagnostics. If the site is unoccupied, reports
     * a packing failure without claiming a specific cause.
     *
     *  @param molecule_id     The locked molecule that was rejected.
     *  @param cluster         The cluster that rejected it.
     *  @param atom_pb_lookup  Maps the cluster's atoms to their primitives.
     *  @param site            The primitive the molecule is locked to.
     */
    void log_locked_site_failure(PackMoleculeId molecule_id,
                                 const LegalizationCluster& cluster,
                                 const AtomPBBimap& atom_pb_lookup,
                                 const t_pb_graph_node* site) const;

    /**
     * @brief Add site-constraint details to the error for a seed that could not
     *        start a cluster.
     *
     * Lists which candidate block types contain the required site and gives
     * alternative site examples for the root atom. Call after all candidate
     * types have failed; this method does not identify why packing failed.
     *
     * @return The failure details, or an empty string if the seed is unlocked.
     */
    std::string describe_locked_site_failure(PackMoleculeId seed_molecule_id,
                                             const std::vector<t_logical_block_type_ptr>& candidate_types) const;

    /**
     * @brief Prioritize group molecules as seeds so their groups can be packed
     *        before other clusters take their atoms.
     *
     * Molecules with locked roots lead, followed by other group molecules and
     * then unconstrained molecules. Preserves the input order within each set.
     */
    void prioritize_seeds(std::vector<PackMoleculeId>& seeds) const;

    /**
     * @brief Return the group's unclustered molecules to try before ordinary
     *        cluster candidates.
     *
     * Lists molecules with a required root site first, then all other molecules
     * in decreasing atom count.
     * Already clustered molecules are excluded; members in other clusters are
     * reported in the verbose log because they cannot be moved into this one.
     *
     *  @param group              The relative placement group being packed.
     *  @param cluster_id         The cluster being grown.
     *  @param cluster_legalizer  Used to skip molecules already clustered.
     *  @param log_verbosity      Packer log verbosity; > 1 reports members in other clusters.
     */
    std::vector<PackMoleculeId> group_molecules_to_pack(UserRelativeGroupId group,
                                                        LegalizationClusterId cluster_id,
                                                        const ClusterLegalizer& cluster_legalizer,
                                                        int log_verbosity) const;

  private:
    /**
     * @brief Identify each chain's owning group from its constrained atoms.
     *        Chains without constrained atoms have no owner.
     */
    void init_chain_owners_();

    /**
     * @brief Reject molecules or chains spanning multiple groups with a fatal error.
     *
     * All group-constrained atoms in a molecule must belong to the same group.
     * For chains, this rule applies across all of their molecules. Atoms without
     * group constraints do not affect this check.
     */
    void validate_molecule_groups_() const;

    /**
     * @brief Reject site locks on non-root atoms when the molecule root is unlocked.
     *
     * The packer places molecules using their roots, so it cannot enforce
     * a non-root atom's site lock unless the root is also locked.
     */
    void validate_locked_sites_() const;

    /**
     * @brief Warn once when non-root atoms have site locks.
     *
     * The pack pattern determines where non-root atoms are placed.
     * verify_clustering() checks whether their site locks are satisfied
     * after packing.
     */
    void warn_non_root_locked_sites_() const;

    /**
     * @brief Return the chain's owning group, or an invalid group if unconstrained.
     */
    UserRelativeGroupId chain_owner_(MoleculeChainId chain_id) const;

    /**
     * @brief Check long chain ownership and record the incoming chain's owner
     *        in new_state if allowed.
     *
     * new_state.group must be the group the cluster would contain after adding
     * the molecule. Logs conflicts at verbosity > 3.
     */
    bool check_long_chain_ownership_(PackMoleculeId molecule_id,
                                     bool cluster_has_long_chain,
                                     t_cluster_relative_state& new_state,
                                     int log_verbosity) const;

    const UserRelativeMacros& relative_macros_; ///< User-defined relative placement macros.

    const Prepacker& prepacker_; ///< Prepacked molecules and chains.

    const AtomNetlist& atom_netlist_; ///< Atom netlist being packed.

    vtr::vector<MoleculeChainId, UserRelativeGroupId> chain_owners_; ///< Owning group of each chain; invalid if the chain has no constrained atoms.
};

/**
 * @brief Report relative placement groups whose atoms occupy multiple clusters.
 *
 * Call after the final packing pass. A split violates the requirement that
 * each group occupy one cluster. Raises a packing error if clustering otherwise
 * succeeded; otherwise warns so the caller can report the primary packing error.
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
