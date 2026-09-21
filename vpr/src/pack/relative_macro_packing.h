#pragma once
/**
 * @file
 * @brief Relative placement macro support for the packer.
 *
 * Each group in UserRelativeMacros becomes one placement macro member:
 *  - All atoms in a group must share one cluster.
 *  - Each cluster may contain atoms from at most one group.
 *
 * RelativeMacroPacker checks these rules during cluster legalization.
 * Callers skip the checks when is_active() is false.
 */

#include <map>

#include "atom_netlist_fwd.h"
#include "prepack.h"
#include "user_relative_macros.h"

class AtomNetlist;

/**
 * @brief Identifies a group in a relative placement macro.
 *
 * The default, invalid value means "no group".
 */
struct t_relative_group {
    UserRelativeMacroId macro_id; ///< Macro containing the group.

    int group_idx = -1; ///< Index in t_user_relative_macro::groups; 0 is the reference group, -1 means no group.

    inline bool is_valid() const { return macro_id.is_valid(); }

    bool operator==(const t_relative_group& other) const = default;
};

/**
 * @brief Relative placement state stored in LegalizationCluster.
 */
struct t_cluster_relative_state {
    t_relative_group group; ///< Group whose constrained atoms are in this cluster.

    bool has_long_chain_mols = false; ///< Contains part of a chain that spans multiple clusters.

    t_relative_group long_chain_owner; ///< Owner of the long chain; invalid if unconstrained or absent.
};

/**
 * @brief Check whether adding a molecule respects long chain ownership.
 *
 * PlaceMacros joins a long chain's clusters into one placement macro. To preserve
 * user offsets, each cluster may contain constrained atoms only from the chain's
 * owning group. Long chains sharing a cluster must have the same owner.
 *
 * Single-cluster chains have no ownership restriction. Unconstrained long chains
 * share an invalid owner and are compatible under this rule. The packer separately
 * limits each cluster to one long chain.
 *
 *  @param molecule_is_long_chain     Whether the molecule belongs to a long chain.
 *  @param molecule_chain_owner       The group owning that chain, if any.
 *  @param cluster_group              The group the cluster hosts after the addition.
 *  @param cluster_has_long_chain     Whether the cluster already holds long chain molecules.
 *  @param cluster_long_chain_owner   The group owning the cluster's long chain, if any.
 *
 *  @return Whether the addition respects long chain ownership.
 */
bool long_chain_ownership_allows(bool molecule_is_long_chain,
                                 const t_relative_group& molecule_chain_owner,
                                 const t_relative_group& cluster_group,
                                 bool cluster_has_long_chain,
                                 const t_relative_group& cluster_long_chain_owner);

/**
 * @brief Result of checking whether a molecule can join a cluster.
 *
 * evaluate_molecule() produces this result for the legalizer's compatibility
 * check, pin utilization check, and state update.
 */
struct t_relative_macro_verdict {
    bool allowed = true; ///< Whether relative placement rules allow the addition.

    t_relative_group cluster_group; ///< Cluster's group after adding the molecule.

    bool molecule_in_cluster_group = false; ///< Group member; bypass pin utilization target to keep the group together.
};

/**
 * @brief Enforces the relative placement macro rules during packing.
 *
 * Owned by ClusterLegalizer. Callers must check is_active() before evaluating
 * or committing molecules.
 */
class RelativeMacroPacker {
  public:
    /** @brief Initialize packing checks for the given relative macros and netlist. */
    RelativeMacroPacker(const UserRelativeMacros& relative_macros,
                        const Prepacker& prepacker,
                        const AtomNetlist& atom_netlist);

    /**
     * @brief Whether the design has user-defined relative placement macros.
     */
    inline bool is_active() const { return active_; }

    /**
     * @brief Record which group owns each prepacked chain.
     *
     * validate_relative_group_molecules() computes ownership and rejects chains
     * spanning multiple groups before clustering.
     */
    void set_chain_owners(std::map<MoleculeChainId, t_relative_group> chain_owners);

    /**
     * @brief Return the molecule's group, or an invalid group if unconstrained.
     *
     * Prepacking and validation ensure its constrained atoms share one group.
     */
    t_relative_group molecule_group(PackMoleculeId molecule_id) const;

    /**
     * @brief Decide whether a molecule may be added to a cluster.
     *
     * Checks that the cluster contains at most one group and respects long chain
     * ownership. Requires is_active().
     *
     *  @param molecule_id    The molecule to add.
     *  @param cluster_state  The cluster's current relative macro state.
     *  @param log_verbosity  Packer log verbosity; > 3 logs group changes and conflicts.
     */
    t_relative_macro_verdict evaluate_molecule(PackMoleculeId molecule_id,
                                               const t_cluster_relative_state& cluster_state,
                                               int log_verbosity) const;

    /**
     * @brief Update the cluster's relative macro state after adding a molecule.
     *
     * Requires is_active() and an allowed verdict from evaluate_molecule().
     */
    void commit_molecule(PackMoleculeId molecule_id,
                         const t_relative_macro_verdict& verdict,
                         t_cluster_relative_state& cluster_state) const;

  private:
    /**
     * @brief Return the chain's owning group, or an invalid group if unconstrained.
     */
    t_relative_group chain_owner(MoleculeChainId chain_id) const;

    /**
     * @brief Check long chain ownership and log any conflict.
     */
    bool check_long_chain_ownership(PackMoleculeId molecule_id,
                                    const t_relative_group& group,
                                    const t_cluster_relative_state& cluster_state,
                                    int log_verbosity) const;

    const UserRelativeMacros& relative_macros_; ///< User-defined relative placement macros.

    const Prepacker& prepacker_; ///< Prepacked molecules and chains.

    const AtomNetlist& atom_netlist_; ///< Atom netlist being packed.

    bool active_; ///< Cached flag indicating whether relative macros are present.

    std::map<MoleculeChainId, t_relative_group> chain_owners_; ///< Owning group of each chain with constrained atoms.
};
