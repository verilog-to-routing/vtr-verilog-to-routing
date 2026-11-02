#pragma once
/**
 * @file
 * @brief Relative placement macro support for the packer.
 *
 * Each group in UserRelativeMacros becomes one placement macro member:
 *  - All atoms in a group must share one cluster.
 *  - Each cluster may contain atoms from at most one group.
 *  - An atom may be locked to a particular primitive site in its cluster.
 *
 * RelativeMacroPacker validates chain ownership and site constraints at
 * construction, checks additions to clusters, and orders group molecules for
 * the greedy clusterer. ClusterLegalizer creates one when the design has
 * relative macros.
 */

#include <map>
#include <string>
#include <unordered_map>
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
 * @brief Relative placement state stored in LegalizationCluster.
 */
struct t_cluster_relative_state {
    UserRelativeGroupId group; ///< Group whose constrained atoms are in this cluster.

    UserRelativeGroupId long_chain_owner; ///< Owner of the long chain; invalid if unconstrained or absent.
};

/**
 * @brief Check that adding a molecule to a cluster respects long chain ownership.
 *
 * A long chain spans clusters that move together as one placement macro. If a
 * relative placement group shares a cluster with the chain, their macros are
 * merged. The group must own the chain to avoid conflicting placement offsets.
 * Long chains sharing a cluster must have the same owner. These checks do not
 * apply to chains that fit in one cluster.
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
 * @brief Result of checking whether a molecule can join a cluster.
 *
 * evaluate_molecule() produces this result for the legalizer's compatibility
 * check, pin utilization check, and state update.
 */
struct t_relative_macro_verdict {
    bool allowed = true; ///< Whether relative placement rules allow the addition.

    t_cluster_relative_state cluster_state; ///< Proposed cluster state; apply only if packing succeeds.

    bool molecule_in_cluster_group = false; ///< Group member allowed full pin utilization to help keep the group together.
};

/**
 * @brief Enforces the relative placement macro rules during packing.
 */
class RelativeMacroPacker {
  public:
    /**
     * @brief Initialize packing checks for the given relative macros and netlist.
     *
     * Records each chain's owning group. Reports a fatal error for molecules or
     * chains spanning groups, or for non-root site locks with an unlocked root.
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

    /**
     * @brief Result of looking up a molecule's required site in a cluster type.
     */
    enum class e_locked_site {
        NOT_LOCKED,  ///< No site constraint; any compatible primitive is allowed.
        RESOLVED,    ///< The required site exists; t_locked_site::node points to it.
        NOT_IN_TYPE, ///< The cluster type does not contain the required site.
    };

    /**
     * @brief Site lookup result and the matching primitive, if found.
     */
    struct t_locked_site {
        e_locked_site status = e_locked_site::NOT_LOCKED; ///< Whether a site is required and present in this type.

        const t_pb_graph_node* node = nullptr; ///< Required primitive; non-null only when status is RESOLVED.
    };

    /**
     * @brief Find the molecule root's required primitive in the given cluster type.
     *
     * The legalizer can force only the root's site. The pack pattern determines
     * where the other atoms in a multi-atom molecule are placed.
     *
     * NOT_LOCKED allows any compatible primitive. NOT_IN_TYPE means the molecule
     * cannot be placed in this cluster type.
     *
     * Builds and caches the primitive-site lookup table on the first request
     * for each cluster type.
     */
    t_locked_site resolve_locked_site(PackMoleculeId molecule_id,
                                      t_logical_block_type_ptr cluster_type);

    /**
     * @brief Return whether the molecule's root atom is locked to a primitive site.
     */
    bool molecule_is_locked_to_site(PackMoleculeId molecule_id) const;

    /**
     * @brief Log a locked-site failure and record it if the cluster is empty.
     *
     * Only seed failures are saved for describe_unsatisfiable_locked_site().
     * Failures in populated clusters, such as an occupied site, do not explain
     * why the molecule could not seed a new cluster.
     *
     * During ClusterLegalizer::start_new_cluster(), the cluster has not yet
     * been stored, so it is passed by reference.
     *
     *  @param molecule_id     The locked molecule that was rejected.
     *  @param cluster         The cluster that rejected it.
     *  @param atom_pb_lookup  Maps the cluster's atoms to their primitives.
     *  @param resolved_site   The primitive the molecule is locked to in this
     *                         cluster type, or nullptr if the required site is absent.
     *  @param log_verbosity   Packer log verbosity; > 2 logs the reason.
     */
    void report_unsatisfiable_locked_site(PackMoleculeId molecule_id,
                                          const LegalizationCluster& cluster,
                                          const AtomPBBimap& atom_pb_lookup,
                                          const t_pb_graph_node* resolved_site,
                                          int log_verbosity);

    /**
     * @brief Describe recorded site failures for an error about the given seed.
     *
     * @return The failure details, or an empty string if this seed has no record.
     */
    std::string describe_unsatisfiable_locked_site(PackMoleculeId seed_molecule_id) const;

    /**
     * @brief Return unclustered group molecules in the order to try adding them.
     *
     * Locked molecules go first, in constraints-file order, so unlocked
     * molecules cannot take their sites. Unlocked molecules follow in descending
     * atom count because larger molecules are harder to place as the cluster fills.
     *
     *  @param seed_molecule_id   The molecule the cluster was seeded with.
     *  @param cluster_id         The cluster being grown.
     *  @param cluster_legalizer  Used to skip molecules already clustered.
     *  @param log_verbosity      Packer log verbosity; > 1 reports a group that
     *                            an earlier cluster already split.
     *
     *  @return The molecules to offer, or an empty vector if the seed belongs
     *          to no group.
     */
    std::vector<PackMoleculeId> group_molecules_to_pack(PackMoleculeId seed_molecule_id,
                                                        LegalizationClusterId cluster_id,
                                                        const ClusterLegalizer& cluster_legalizer,
                                                        int log_verbosity) const;

  private:
    /**
     * @brief Reason a locked molecule could not be placed in a cluster type.
     */
    enum class e_locked_site_failure {
        NOT_IN_TYPE,           ///< The type has no primitive with the required path.
        SITE_OCCUPIED,         ///< Another atom occupies the required primitive.
        MOLECULE_DOES_NOT_FIT, ///< The site is unoccupied, but the molecule cannot be placed there.
    };

    /**
     * @brief Locked-site failure for one candidate cluster type.
     */
    struct t_locked_site_failure {
        t_logical_block_type_ptr cluster_type = nullptr; ///< The cluster type that was tried.

        e_locked_site_failure reason = e_locked_site_failure::NOT_IN_TYPE; ///< Reason the placement attempt failed.

        AtomBlockId occupying_atom; ///< Atom occupying the site; valid only for SITE_OCCUPIED.
    };

    /**
     * @brief Site failures for a seed molecule, grouped by candidate cluster type.
     *
     * GreedyClusterer::start_new_cluster() includes these details when no block
     * type can place the seed. Different types may fail for different reasons.
     */
    struct t_unsatisfiable_locked_site {
        AtomBlockId atom; ///< Root atom whose site failed; invalid if there is no record.

        UserRelativeGroupId group; ///< The group the atom belongs to.

        std::string site_path; ///< The hierarchical path of the site the atom is locked to.

        std::vector<t_locked_site_failure> failures; ///< One entry per cluster type tried, in the order tried.
    };

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
     * @brief Validate site locks on non-root atoms in prepacked molecules.
     *
     * A non-root site lock requires a locked root; otherwise packing fails
     * because the lock cannot be enforced. With a locked root, the pack pattern
     * determines the other sites. Warn once about these indirect constraints;
     * verify_clustering() checks that the final sites match.
     */
    void validate_locked_sites_() const;

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

    /**
     * @brief Return the site path the molecule's root atom is locked to, or an
     *        empty string if it is unlocked.
     */
    const std::string& molecule_site_path_(PackMoleculeId molecule_id) const;

    /**
     * @brief Return a cluster type's cached map from full primitive paths to nodes.
     *        Build the map on the first request for the type.
     */
    const std::unordered_map<std::string, const t_pb_graph_node*>& site_table_(t_logical_block_type_ptr cluster_type);

    const UserRelativeMacros& relative_macros_; ///< User-defined relative placement macros.

    const Prepacker& prepacker_; ///< Prepacked molecules and chains.

    const AtomNetlist& atom_netlist_; ///< Atom netlist being packed.

    vtr::vector<MoleculeChainId, UserRelativeGroupId> chain_owners_; ///< Owning group of each chain; invalid if the chain has no constrained atoms.

    std::map<t_logical_block_type_ptr, std::unordered_map<std::string, const t_pb_graph_node*>> site_tables_; ///< Cached primitive paths and nodes for each cluster type.

    t_unsatisfiable_locked_site last_unsatisfiable_locked_site_; ///< Site failures for the most recently rejected locked seed.
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
