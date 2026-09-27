/**
 * @file
 * @brief Implementation of RelativeMacroPacker.
 */

#include "relative_macro_packing.h"

#include <utility>

#include "atom_netlist.h"
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

t_relative_group RelativeMacroPacker::chain_owner(MoleculeChainId chain_id) const {
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

bool RelativeMacroPacker::check_long_chain_ownership(PackMoleculeId molecule_id,
                                                     const t_relative_group& group,
                                                     const t_cluster_relative_state& cluster_state,
                                                     int log_verbosity) const {
    const t_pack_molecule& molecule = prepacker_.get_molecule(molecule_id);
    const bool molecule_is_long_chain = molecule.chain_id.is_valid()
                                        && prepacker_.get_molecule_chain_info(molecule.chain_id).is_long_chain;
    const t_relative_group molecule_chain_owner = molecule_is_long_chain ? chain_owner(molecule.chain_id)
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

    if (!check_long_chain_ownership(molecule_id, verdict.cluster_group, cluster_state, log_verbosity)) {
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
        cluster_state.long_chain_owner = chain_owner(molecule.chain_id);
    }
}
