/**
 * @file
 * @brief Implementation of RelativeMacroPacker.
 */

#include "relative_macro_packing.h"

#include <string>
#include <vector>

#include "atom_netlist.h"
#include "cluster_legalizer.h"
#include "vpr_error.h"
#include "vtr_assert.h"
#include "vtr_log.h"

RelativeMacroPacker::RelativeMacroPacker(const UserRelativeMacros& relative_macros,
                                         const Prepacker& prepacker,
                                         const AtomNetlist& atom_netlist)
    : relative_macros_(relative_macros)
    , prepacker_(prepacker)
    , atom_netlist_(atom_netlist) {
    VTR_ASSERT(relative_macros.get_num_macros() != 0);
    validate_molecule_groups_();
}

void RelativeMacroPacker::validate_molecule_groups_() {
    VTR_ASSERT(chain_owners_.empty());

    // Check each molecule against its atoms' groups and any known chain owner.
    for (PackMoleculeId mol_id : prepacker_.molecules()) {
        const t_pack_molecule& molecule = prepacker_.get_molecule(mol_id);

        UserRelativeGroupId mol_group;
        AtomBlockId mol_group_atom;
        // True if the group came from an earlier molecule in this chain.
        bool mol_group_from_chain = false;

        if (molecule.chain_id.is_valid()) {
            auto chain_it = chain_owners_.find(molecule.chain_id);
            if (chain_it != chain_owners_.end()) {
                mol_group = chain_it->second;
                mol_group_from_chain = true;
            }
        }

        for (AtomBlockId blk_id : molecule.atom_block_ids) {
            if (!blk_id.is_valid())
                continue;

            UserRelativeGroupId atom_group = relative_macros_.get_atom_group(blk_id);
            if (!atom_group.is_valid())
                continue;

            if (!mol_group.is_valid()) {
                mol_group = atom_group;
                mol_group_atom = blk_id;
                continue;
            }

            if (mol_group != atom_group) {
                // Find an atom from the chain's earlier group for the error message.
                if (mol_group_from_chain) {
                    for (AtomBlockId group_blk_id : relative_macros_.get_group(mol_group).atoms) {
                        PackMoleculeId group_mol_id = prepacker_.get_atom_molecule(group_blk_id);
                        if (prepacker_.get_molecule(group_mol_id).chain_id == molecule.chain_id) {
                            mol_group_atom = group_blk_id;
                            break;
                        }
                    }
                    VTR_ASSERT(mol_group_atom.is_valid());
                }
                const t_user_relative_macro& mol_macro = relative_macros_.get_macro(relative_macros_.get_group(mol_group).macro_id);
                const t_user_relative_macro& atom_macro = relative_macros_.get_macro(relative_macros_.get_group(atom_group).macro_id);
                VPR_FATAL_ERROR(VPR_ERROR_PACK,
                                "Atoms '%s' (relative macro '%s', group %zu) and '%s' (relative macro '%s', group %zu) "
                                "belong to the same prepacked %s (typically a carry chain, whose atoms are connected "
                                "through dedicated routing and cannot be separated); however, atoms of different "
                                "relative placement groups must be packed into different clusters. Adjust the "
                                "constraints so the chain's atoms are in at most one group (the rest of the chain "
                                "may be left unconstrained).\n",
                                atom_netlist_.block_name(mol_group_atom).c_str(),
                                mol_macro.name.c_str(),
                                size_t(mol_group),
                                atom_netlist_.block_name(blk_id).c_str(),
                                atom_macro.name.c_str(),
                                size_t(atom_group),
                                mol_group_from_chain ? "chain" : "molecule");
            }
        }

        if (molecule.chain_id.is_valid() && mol_group.is_valid()) {
            chain_owners_.emplace(molecule.chain_id, mol_group);
        }
    }
}

UserRelativeGroupId RelativeMacroPacker::molecule_group(PackMoleculeId molecule_id) const {
    VTR_ASSERT_SAFE(molecule_id.is_valid());

    const t_pack_molecule& molecule = prepacker_.get_molecule(molecule_id);
    for (AtomBlockId blk_id : molecule.atom_block_ids) {
        if (!blk_id.is_valid())
            continue;
        // All constrained atoms share a group, so the first one identifies it.
        UserRelativeGroupId group = relative_macros_.get_atom_group(blk_id);
        if (group.is_valid())
            return group;
    }
    return UserRelativeGroupId::INVALID();
}

UserRelativeGroupId RelativeMacroPacker::chain_owner_(MoleculeChainId chain_id) const {
    auto owner_itr = chain_owners_.find(chain_id);
    if (owner_itr != chain_owners_.end())
        return owner_itr->second;
    return UserRelativeGroupId::INVALID();
}

bool long_chain_ownership_allows(bool molecule_is_long_chain,
                                 UserRelativeGroupId molecule_chain_owner,
                                 UserRelativeGroupId cluster_group,
                                 bool cluster_has_long_chain,
                                 UserRelativeGroupId cluster_long_chain_owner) {
    if (molecule_is_long_chain) {
        // The cluster's group must own the incoming long chain.
        if (cluster_group.is_valid() && cluster_group != molecule_chain_owner)
            return false;

        // Long chains with different owners may not share a cluster.
        if (cluster_has_long_chain && cluster_long_chain_owner != molecule_chain_owner)
            return false;
    }

    // The resulting group must own the cluster's existing long chain.
    if (cluster_has_long_chain && cluster_group.is_valid() && cluster_group != cluster_long_chain_owner)
        return false;

    return true;
}

bool RelativeMacroPacker::check_long_chain_ownership_(PackMoleculeId molecule_id,
                                                      UserRelativeGroupId group,
                                                      const t_cluster_relative_state& cluster_state,
                                                      bool cluster_has_long_chain,
                                                      int log_verbosity) const {
    const t_pack_molecule& molecule = prepacker_.get_molecule(molecule_id);
    const bool molecule_is_long_chain = molecule.chain_id.is_valid()
                                        && prepacker_.get_molecule_chain_info(molecule.chain_id).is_long_chain;
    const UserRelativeGroupId molecule_chain_owner = molecule_is_long_chain ? chain_owner_(molecule.chain_id)
                                                                            : UserRelativeGroupId::INVALID();

    if (long_chain_ownership_allows(molecule_is_long_chain,
                                    molecule_chain_owner,
                                    group,
                                    cluster_has_long_chain,
                                    cluster_state.long_chain_owner)) {
        return true;
    }

    // try_pack_molecule() rejects a second long chain earlier;
    // is_molecule_compatible() relies on this check for ownership conflicts.
    VTR_LOGV(log_verbosity > 3,
             "\t\t\t Long Chain: molecule and cluster do not agree on which relative group owns the long chain\n");
    return false;
}

t_relative_macro_verdict RelativeMacroPacker::evaluate_molecule(PackMoleculeId molecule_id,
                                                                const t_cluster_relative_state& cluster_state,
                                                                bool cluster_has_long_chain,
                                                                int log_verbosity) const {
    t_relative_macro_verdict verdict;
    verdict.cluster_group = cluster_state.group;

    const UserRelativeGroupId molecule_rel_group = molecule_group(molecule_id);

    if (molecule_rel_group.is_valid()) {
        if (!cluster_state.group.is_valid()) {
            // An ungrouped cluster adopts the molecule's group if packing succeeds.
            VTR_LOGV(log_verbosity > 3,
                     "\t\t\t Relative Group: molecule passed relative group check, cluster adopted its group %zu\n",
                     size_t(molecule_rel_group));
            verdict.cluster_group = molecule_rel_group;
        } else if (cluster_state.group != molecule_rel_group) {
            VTR_LOGV(log_verbosity > 3,
                     "\t\t\t Relative Group: molecule failed relative group check. Cluster's group: %zu, molecule's group: %zu\n",
                     size_t(cluster_state.group), size_t(molecule_rel_group));
            verdict.allowed = false;
            return verdict;
        }
        verdict.molecule_in_cluster_group = true;
    }

    if (!check_long_chain_ownership_(molecule_id, verdict.cluster_group, cluster_state, cluster_has_long_chain, log_verbosity)) {
        verdict.allowed = false;
        return verdict;
    }

    return verdict;
}

void RelativeMacroPacker::commit_molecule(PackMoleculeId molecule_id,
                                          const t_relative_macro_verdict& verdict,
                                          t_cluster_relative_state& cluster_state) const {
    VTR_ASSERT_SAFE(verdict.allowed);

    cluster_state.group = verdict.cluster_group;

    // Record long chain ownership for checking later additions.
    const t_pack_molecule& molecule = prepacker_.get_molecule(molecule_id);
    if (molecule.chain_id.is_valid() && prepacker_.get_molecule_chain_info(molecule.chain_id).is_long_chain) {
        cluster_state.long_chain_owner = chain_owner_(molecule.chain_id);
    }
}

/**
 * @brief Find relative placement groups whose atoms occupy multiple clusters.
 *
 * All atoms in a group must share a cluster so the group can become a single
 * placement macro member.
 */
static std::vector<UserRelativeGroupId> find_split_relative_groups(const ClusterLegalizer& cluster_legalizer,
                                                                   const UserRelativeMacros& relative_macros) {
    std::vector<UserRelativeGroupId> split_groups;

    for (UserRelativeGroupId group_id : relative_macros.groups()) {
        LegalizationClusterId group_cluster_id;
        for (AtomBlockId blk_id : relative_macros.get_group(group_id).atoms) {
            LegalizationClusterId cluster_id = cluster_legalizer.get_atom_cluster(blk_id);
            // Final verification checks for unclustered atoms.
            if (!cluster_id.is_valid())
                continue;
            if (!group_cluster_id.is_valid()) {
                group_cluster_id = cluster_id;
            } else if (cluster_id != group_cluster_id) {
                split_groups.push_back(group_id);
                break;
            }
        }
    }

    return split_groups;
}

void report_split_relative_groups(const ClusterLegalizer& cluster_legalizer,
                                  const UserRelativeMacros& relative_macros,
                                  const AtomNetlist& atom_netlist,
                                  bool clustering_succeeded) {
    std::vector<UserRelativeGroupId> split_groups = find_split_relative_groups(cluster_legalizer, relative_macros);
    if (split_groups.empty())
        return;

    std::string report;
    for (UserRelativeGroupId group_id : split_groups) {
        const t_user_relative_group& group = relative_macros.get_group(group_id);
        const t_user_relative_macro& macro = relative_macros.get_macro(group.macro_id);
        report += "  Relative macro '" + macro.name + "', group " + std::to_string(size_t(group_id)) + ":";
        for (AtomBlockId blk_id : group.atoms) {
            LegalizationClusterId cluster_id = cluster_legalizer.get_atom_cluster(blk_id);
            report += " '" + atom_netlist.block_name(blk_id) + "' ("
                      + (cluster_id.is_valid() ? "cluster " + std::to_string((size_t)cluster_id)
                                               : std::string("unclustered"))
                      + ")";
        }
        report += "\n";
    }

    if (clustering_succeeded) {
        VPR_FATAL_ERROR(VPR_ERROR_PACK,
                        "Failed to pack each relative placement group into a single cluster "
                        "(%zu group(s) split):\n%s"
                        "The group(s) may be too large to fit into one cluster or may conflict "
                        "with pack patterns. Consider making the group(s) smaller.\n",
                        split_groups.size(),
                        report.c_str());
    } else {
        VTR_LOG_WARN("%zu relative placement group(s) are split across clusters:\n%s"
                     "This is likely a symptom of the clustering not fitting the device "
                     "(see the following error).\n",
                     split_groups.size(),
                     report.c_str());
    }
}
