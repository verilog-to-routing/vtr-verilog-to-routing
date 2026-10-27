/**
 * @file
 * @brief Implementation of RelativeMacroPacker.
 */

#include "relative_macro_packing.h"

#include <string>
#include <utility>
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
    , atom_netlist_(atom_netlist)
    , active_(relative_macros.get_num_macros() != 0) {
    if (active_)
        validate_molecule_groups_();
}

void RelativeMacroPacker::validate_molecule_groups_() {
    VTR_ASSERT(active_);
    VTR_ASSERT(chain_owners_.empty());

    // Check each molecule against its atoms' groups and any known chain owner.
    for (PackMoleculeId mol_id : prepacker_.molecules()) {
        const t_pack_molecule& molecule = prepacker_.get_molecule(mol_id);

        t_relative_group mol_group;
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

            std::pair<UserRelativeMacroId, int> atom_pair = relative_macros_.get_atom_group(blk_id);
            if (!atom_pair.first.is_valid())
                continue;
            t_relative_group atom_group{atom_pair.first, atom_pair.second};

            if (!mol_group.is_valid()) {
                mol_group = atom_group;
                mol_group_atom = blk_id;
                continue;
            }

            if (mol_group != atom_group) {
                // Find an atom from the chain's earlier group for the error message.
                if (mol_group_from_chain) {
                    for (AtomBlockId group_blk_id : relative_macros_.get_macro(mol_group.macro_id).groups[mol_group.group_idx].atoms) {
                        PackMoleculeId group_mol_id = prepacker_.get_atom_molecule(group_blk_id);
                        if (prepacker_.get_molecule(group_mol_id).chain_id == molecule.chain_id) {
                            mol_group_atom = group_blk_id;
                            break;
                        }
                    }
                    VTR_ASSERT(mol_group_atom.is_valid());
                }
                VPR_FATAL_ERROR(VPR_ERROR_PACK,
                                "Atoms '%s' (relative macro '%s', group %d) and '%s' (relative macro '%s', group %d) "
                                "belong to the same prepacked %s (typically a carry chain, whose atoms are connected "
                                "through dedicated routing and cannot be separated); however, atoms of different "
                                "relative placement groups must be packed into different clusters. Adjust the "
                                "constraints so the chain's atoms are in at most one group (the rest of the chain "
                                "may be left unconstrained).\n",
                                atom_netlist_.block_name(mol_group_atom).c_str(),
                                relative_macros_.get_macro(mol_group.macro_id).name.c_str(),
                                mol_group.group_idx,
                                atom_netlist_.block_name(blk_id).c_str(),
                                relative_macros_.get_macro(atom_group.macro_id).name.c_str(),
                                atom_group.group_idx,
                                mol_group_from_chain ? "chain" : "molecule");
            }
        }

        if (molecule.chain_id.is_valid() && mol_group.is_valid()) {
            chain_owners_.emplace(molecule.chain_id, mol_group);
        }
    }
}

t_relative_group RelativeMacroPacker::molecule_group(PackMoleculeId molecule_id) const {
    if (!active_ || !molecule_id.is_valid())
        return {};

    const t_pack_molecule& molecule = prepacker_.get_molecule(molecule_id);
    for (AtomBlockId blk_id : molecule.atom_block_ids) {
        if (!blk_id.is_valid())
            continue;
        // All constrained atoms share a group, so the first one identifies it.
        std::pair<UserRelativeMacroId, int> group = relative_macros_.get_atom_group(blk_id);
        if (group.first.is_valid())
            return {group.first, group.second};
    }
    return {};
}

t_relative_group RelativeMacroPacker::chain_owner_(MoleculeChainId chain_id) const {
    auto owner_itr = chain_owners_.find(chain_id);
    if (owner_itr != chain_owners_.end())
        return owner_itr->second;
    return {};
}

bool long_chain_ownership_allows(bool molecule_is_long_chain,
                                 const t_relative_group& molecule_chain_owner,
                                 const t_relative_group& cluster_group,
                                 bool cluster_has_long_chain,
                                 const t_relative_group& cluster_long_chain_owner) {
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
                                                      const t_relative_group& group,
                                                      const t_cluster_relative_state& cluster_state,
                                                      int log_verbosity) const {
    const t_pack_molecule& molecule = prepacker_.get_molecule(molecule_id);
    const bool molecule_is_long_chain = molecule.chain_id.is_valid()
                                        && prepacker_.get_molecule_chain_info(molecule.chain_id).is_long_chain;
    const t_relative_group molecule_chain_owner = molecule_is_long_chain ? chain_owner_(molecule.chain_id)
                                                                         : t_relative_group();

    if (long_chain_ownership_allows(molecule_is_long_chain,
                                    molecule_chain_owner,
                                    group,
                                    cluster_state.has_long_chain_mols,
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
                                                                int log_verbosity) const {
    VTR_ASSERT_SAFE(active_);

    t_relative_macro_verdict verdict;
    verdict.cluster_group = cluster_state.group;

    const t_relative_group molecule_rel_group = molecule_group(molecule_id);

    if (molecule_rel_group.is_valid()) {
        if (!cluster_state.group.is_valid()) {
            // An ungrouped cluster adopts the molecule's group if packing succeeds.
            VTR_LOGV(log_verbosity > 3,
                     "\t\t\t Relative Group: molecule passed relative group check, cluster adopted its group (macro %zu, group %d)\n",
                     (size_t)molecule_rel_group.macro_id, molecule_rel_group.group_idx);
            verdict.cluster_group = molecule_rel_group;
        } else if (cluster_state.group != molecule_rel_group) {
            VTR_LOGV(log_verbosity > 3,
                     "\t\t\t Relative Group: molecule failed relative group check. Cluster's group: (macro %zu, group %d), molecule's group: (macro %zu, group %d)\n",
                     (size_t)cluster_state.group.macro_id, cluster_state.group.group_idx,
                     (size_t)molecule_rel_group.macro_id, molecule_rel_group.group_idx);
            verdict.allowed = false;
            return verdict;
        }
        verdict.molecule_in_cluster_group = true;
    }

    if (!check_long_chain_ownership_(molecule_id, verdict.cluster_group, cluster_state, log_verbosity)) {
        verdict.allowed = false;
        return verdict;
    }

    return verdict;
}

void RelativeMacroPacker::commit_molecule(PackMoleculeId molecule_id,
                                          const t_relative_macro_verdict& verdict,
                                          t_cluster_relative_state& cluster_state) const {
    VTR_ASSERT_SAFE(active_ && verdict.allowed);

    cluster_state.group = verdict.cluster_group;

    // Record long chain ownership for checking later additions.
    const t_pack_molecule& molecule = prepacker_.get_molecule(molecule_id);
    if (molecule.chain_id.is_valid() && prepacker_.get_molecule_chain_info(molecule.chain_id).is_long_chain) {
        cluster_state.has_long_chain_mols = true;
        cluster_state.long_chain_owner = chain_owner_(molecule.chain_id);
    }
}

/**
 * @brief Find relative placement groups whose atoms occupy multiple clusters.
 *
 * All atoms in a group must share a cluster so the group can become a single
 * placement macro member.
 */
static std::vector<t_relative_group> find_split_relative_groups(const ClusterLegalizer& cluster_legalizer,
                                                                const UserRelativeMacros& relative_macros) {
    std::vector<t_relative_group> split_groups;

    for (UserRelativeMacroId macro_id : relative_macros.macros()) {
        const t_user_relative_macro& macro = relative_macros.get_macro(macro_id);

        for (size_t igroup = 0; igroup < macro.groups.size(); igroup++) {
            LegalizationClusterId group_cluster_id;
            for (AtomBlockId blk_id : macro.groups[igroup].atoms) {
                LegalizationClusterId cluster_id = cluster_legalizer.get_atom_cluster(blk_id);
                // Final verification checks for unclustered atoms.
                if (!cluster_id.is_valid())
                    continue;
                if (!group_cluster_id.is_valid()) {
                    group_cluster_id = cluster_id;
                } else if (cluster_id != group_cluster_id) {
                    split_groups.push_back({macro_id, (int)igroup});
                    break;
                }
            }
        }
    }

    return split_groups;
}

void report_split_relative_groups(const ClusterLegalizer& cluster_legalizer,
                                  const UserRelativeMacros& relative_macros,
                                  const AtomNetlist& atom_netlist,
                                  bool clustering_succeeded) {
    std::vector<t_relative_group> split_groups = find_split_relative_groups(cluster_legalizer, relative_macros);
    if (split_groups.empty())
        return;

    std::string report;
    for (const t_relative_group& group : split_groups) {
        const t_user_relative_macro& macro = relative_macros.get_macro(group.macro_id);
        report += "  Relative macro '" + macro.name + "', group " + std::to_string(group.group_idx) + ":";
        for (AtomBlockId blk_id : macro.groups[group.group_idx].atoms) {
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
