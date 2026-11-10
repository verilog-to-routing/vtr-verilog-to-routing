/**
 * @file
 * @brief Implementation of RelativeMacroPacker.
 */

#include "relative_macro_packing.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "atom_netlist.h"
#include "atom_pb_bimap.h"
#include "cluster_legalizer.h"
#include "vpr_error.h"
#include "vpr_utils.h"
#include "vtr_assert.h"
#include "vtr_log.h"

// Maximum number of alternative site paths to include in an error message.
static constexpr size_t MAX_REPORTED_SITE_PATHS = 3;

RelativeMacroPacker::RelativeMacroPacker(const UserRelativeMacros& relative_macros,
                                         const Prepacker& prepacker,
                                         const AtomNetlist& atom_netlist)
    : relative_macros_(relative_macros)
    , prepacker_(prepacker)
    , atom_netlist_(atom_netlist)
    , chain_owners_(prepacker.get_num_molecule_chains(), UserRelativeGroupId::INVALID()) {
    VTR_ASSERT(relative_macros.get_num_macros() != 0);
    init_chain_owners_();
    validate_molecule_groups_();
    validate_locked_sites_();
}

void RelativeMacroPacker::init_chain_owners_() {
    // A chain is owned by the group of its first constrained atom. Chains whose
    // atoms span several groups are rejected by validate_molecule_groups_().
    for (PackMoleculeId mol_id : prepacker_.molecules()) {
        const t_pack_molecule& molecule = prepacker_.get_molecule(mol_id);
        if (!molecule.chain_id.is_valid() || chain_owners_[molecule.chain_id].is_valid())
            continue;

        for (AtomBlockId blk_id : molecule.atom_block_ids) {
            if (!blk_id.is_valid())
                continue;

            UserRelativeGroupId atom_group = relative_macros_.get_atom_group(blk_id);
            if (atom_group.is_valid()) {
                chain_owners_[molecule.chain_id] = atom_group;
                break;
            }
        }
    }
}

void RelativeMacroPacker::validate_molecule_groups_() const {
    // Check each molecule's atoms against each other and against its chain's owner.
    for (PackMoleculeId mol_id : prepacker_.molecules()) {
        const t_pack_molecule& molecule = prepacker_.get_molecule(mol_id);

        UserRelativeGroupId mol_group;
        AtomBlockId mol_group_atom;
        // True if the group came from the molecule's chain rather than its own atoms.
        bool mol_group_from_chain = false;

        if (molecule.chain_id.is_valid()) {
            mol_group = chain_owner_(molecule.chain_id);
            mol_group_from_chain = mol_group.is_valid();
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
                // Find an atom of the chain's owning group for the error message.
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
                VPR_FATAL_ERROR(VPR_ERROR_PACK,
                                "Atoms '%s' (relative macro '%s', group %zu) and '%s' (relative macro '%s', group %zu) "
                                "belong to the same prepacked %s (typically a carry chain, whose atoms are connected "
                                "through dedicated routing and cannot be separated); however, atoms of different "
                                "relative placement groups must be packed into different clusters. Adjust the "
                                "constraints so the chain's atoms are in at most one group (the rest of the chain "
                                "may be left unconstrained).\n",
                                atom_netlist_.block_name(mol_group_atom).c_str(),
                                relative_macros_.get_group_macro_name(mol_group).c_str(),
                                size_t(mol_group),
                                atom_netlist_.block_name(blk_id).c_str(),
                                relative_macros_.get_group_macro_name(atom_group).c_str(),
                                size_t(atom_group),
                                mol_group_from_chain ? "chain" : "molecule");
            }
        }
    }
}

void RelativeMacroPacker::validate_locked_sites_() const {
    // Report non-root site locks in one warning. Constraints generated from a
    // placement can contain many of them, but only root sites are forced directly.
    size_t num_non_root_sites = 0;
    AtomBlockId first_non_root_site;

    for (PackMoleculeId mol_id : prepacker_.molecules()) {
        const t_pack_molecule& molecule = prepacker_.get_molecule(mol_id);
        AtomBlockId root_blk_id = prepacker_.get_molecule_root_atom(mol_id);
        const bool root_is_locked = !relative_macros_.get_atom_locked_site_path(root_blk_id).empty();

        for (AtomBlockId blk_id : molecule.atom_block_ids) {
            if (!blk_id.is_valid() || blk_id == root_blk_id)
                continue;
            const std::string& site_path = relative_macros_.get_atom_locked_site_path(blk_id);
            if (site_path.empty())
                continue;

            if (!root_is_locked) {
                // An unlocked root leaves this atom's site unenforced. The
                // molecule may differ from the one used to record the sites.
                UserRelativeGroupId atom_group = relative_macros_.get_atom_group(blk_id);
                VPR_FATAL_ERROR(VPR_ERROR_PACK,
                                "Atom '%s' of relative macro '%s' group %zu is locked to primitive site '%s', but it "
                                "is not the root of its prepacked molecule (the root is atom '%s', which is not "
                                "locked). A site can only be forced through a molecule's root primitive, so this site "
                                "would be ignored. The netlist most likely forms different pack patterns than the "
                                "netlist the sites were recorded from; regenerate the macro against this netlist.\n",
                                atom_netlist_.block_name(blk_id).c_str(),
                                relative_macros_.get_group_macro_name(atom_group).c_str(),
                                size_t(atom_group),
                                site_path.c_str(),
                                atom_netlist_.block_name(root_blk_id).c_str());
            }

            num_non_root_sites++;
            if (!first_non_root_site.is_valid())
                first_non_root_site = blk_id;
        }
    }

    VTR_LOGV_WARN(num_non_root_sites > 0,
                  "%zu atom(s) of relative placement macros carry a primitive site but are not the root of their "
                  "prepacked molecule (first: '%s'). Only a molecule's root can be placed on a given site; the "
                  "others follow from the pack pattern. These sites are not enforced directly, but are checked "
                  "against the final clustering, which will report any that were not met.\n",
                  num_non_root_sites,
                  atom_netlist_.block_name(first_non_root_site).c_str());
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
    VTR_ASSERT_SAFE(chain_id.is_valid());
    return chain_owners_[chain_id];
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
                                                      bool cluster_has_long_chain,
                                                      t_cluster_relative_state& new_state,
                                                      int log_verbosity) const {
    const t_pack_molecule& molecule = prepacker_.get_molecule(molecule_id);
    const bool molecule_is_long_chain = molecule.chain_id.is_valid()
                                        && prepacker_.get_molecule_chain_info(molecule.chain_id).is_long_chain;
    const UserRelativeGroupId molecule_chain_owner = molecule_is_long_chain ? chain_owner_(molecule.chain_id)
                                                                            : UserRelativeGroupId::INVALID();

    if (long_chain_ownership_allows(molecule_is_long_chain,
                                    molecule_chain_owner,
                                    new_state.group,
                                    cluster_has_long_chain,
                                    new_state.long_chain_owner)) {
        // Record long chain ownership for checking later additions.
        if (molecule_is_long_chain)
            new_state.long_chain_owner = molecule_chain_owner;
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
    verdict.cluster_state = cluster_state;

    const UserRelativeGroupId molecule_rel_group = molecule_group(molecule_id);

    if (molecule_rel_group.is_valid()) {
        if (!cluster_state.group.is_valid()) {
            // An ungrouped cluster adopts the molecule's group if packing succeeds.
            VTR_LOGV(log_verbosity > 3,
                     "\t\t\t Relative Group: molecule passed relative group check, cluster adopted its group %zu\n",
                     size_t(molecule_rel_group));
            verdict.cluster_state.group = molecule_rel_group;
        } else if (cluster_state.group != molecule_rel_group) {
            VTR_LOGV(log_verbosity > 3,
                     "\t\t\t Relative Group: molecule failed relative group check. Cluster's group: %zu, molecule's group: %zu\n",
                     size_t(cluster_state.group), size_t(molecule_rel_group));
            verdict.allowed = false;
            return verdict;
        }
        verdict.molecule_in_cluster_group = true;
    }

    if (!check_long_chain_ownership_(molecule_id, cluster_has_long_chain, verdict.cluster_state, log_verbosity)) {
        verdict.allowed = false;
        return verdict;
    }

    return verdict;
}

const std::string& RelativeMacroPacker::molecule_site_path_(PackMoleculeId molecule_id) const {
    return relative_macros_.get_atom_locked_site_path(prepacker_.get_molecule_root_atom(molecule_id));
}

bool RelativeMacroPacker::molecule_is_locked_to_site(PackMoleculeId molecule_id) const {
    return !molecule_site_path_(molecule_id).empty();
}

const std::unordered_map<std::string, const t_pb_graph_node*>& RelativeMacroPacker::site_table_(t_logical_block_type_ptr cluster_type) {
    auto table_itr = site_tables_.find(cluster_type);
    if (table_itr == site_tables_.end()) {
        // Cache each primitive by its full hierarchical path, using the same
        // names as the constraints loader.
        std::unordered_map<std::string, const t_pb_graph_node*> table;
        for (const t_pb_graph_node& node : cluster_type->pb_graph_head->subtree()) {
            if (node.is_primitive())
                table.emplace(node.hierarchical_type_name(), &node);
        }
        table_itr = site_tables_.emplace(cluster_type, std::move(table)).first;
    }
    return table_itr->second;
}

RelativeMacroPacker::t_locked_site RelativeMacroPacker::resolve_locked_site(PackMoleculeId molecule_id,
                                                                            t_logical_block_type_ptr cluster_type) {
    const std::string& site_path = molecule_site_path_(molecule_id);
    if (site_path.empty())
        return {e_locked_site::NOT_LOCKED, nullptr};
    VTR_ASSERT(cluster_type != nullptr);

    const std::unordered_map<std::string, const t_pb_graph_node*>& table = site_table_(cluster_type);
    auto node_itr = table.find(site_path);
    if (node_itr == table.end())
        return {e_locked_site::NOT_IN_TYPE, nullptr};
    return {e_locked_site::RESOLVED, node_itr->second};
}

void RelativeMacroPacker::report_unsatisfiable_locked_site(PackMoleculeId molecule_id,
                                                           const LegalizationCluster& cluster,
                                                           const AtomPBBimap& atom_pb_lookup,
                                                           const t_pb_graph_node* resolved_site,
                                                           int log_verbosity) {
    AtomBlockId root_blk_id = prepacker_.get_molecule_root_atom(molecule_id);
    const std::string& site_path = molecule_site_path_(molecule_id);
    VTR_ASSERT(!site_path.empty());
    const UserRelativeGroupId group = molecule_group(molecule_id);
    const std::string& macro_name = relative_macros_.get_group_macro_name(group);

    // Check the cluster's atoms for an occupant. Searching the full pb graph
    // for alternative sites is deferred until the fatal error is formatted.
    t_locked_site_failure failure;
    failure.cluster_type = cluster.type;
    if (resolved_site == nullptr) {
        failure.reason = e_locked_site_failure::NOT_IN_TYPE;
    } else {
        failure.reason = e_locked_site_failure::MOLECULE_DOES_NOT_FIT;
        for (PackMoleculeId cluster_mol_id : cluster.molecules) {
            for (AtomBlockId blk_id : prepacker_.get_molecule(cluster_mol_id).atom_block_ids) {
                if (!blk_id.is_valid())
                    continue;
                const t_pb* atom_pb = atom_pb_lookup.atom_pb(blk_id);
                if (atom_pb == nullptr || atom_pb->pb_graph_node != resolved_site)
                    continue;
                failure.reason = e_locked_site_failure::SITE_OCCUPIED;
                failure.occupying_atom = blk_id;
                break;
            }
            if (failure.reason == e_locked_site_failure::SITE_OCCUPIED)
                break;
        }
    }

    // Save only failures in empty clusters for the seed error message. Keep one
    // entry per cluster type, and reset the record when a different seed fails.
    if (cluster.molecules.empty()) {
        if (last_unsatisfiable_locked_site_.atom != root_blk_id)
            last_unsatisfiable_locked_site_ = {root_blk_id, group, site_path, {}};
        std::vector<t_locked_site_failure>& failures = last_unsatisfiable_locked_site_.failures;
        auto same_type = std::find_if(failures.begin(), failures.end(), [&](const t_locked_site_failure& seen) {
            return seen.cluster_type == failure.cluster_type;
        });
        if (same_type == failures.end())
            failures.push_back(failure);
        else
            *same_type = failure;
    }

    if (log_verbosity <= 2)
        return;

    switch (failure.reason) {
        case e_locked_site_failure::NOT_IN_TYPE:
            VTR_LOG("\t\tRelative macro '%s' group %zu: atom '%s' is locked to primitive site '%s', which does not "
                    "exist in cluster type '%s'\n",
                    macro_name.c_str(), size_t(group),
                    atom_netlist_.block_name(root_blk_id).c_str(), site_path.c_str(), cluster.type->name.c_str());
            break;
        case e_locked_site_failure::SITE_OCCUPIED:
            VTR_LOG("\t\tRelative macro '%s' group %zu: atom '%s' is locked to primitive site '%s', which is already "
                    "occupied by atom '%s'\n",
                    macro_name.c_str(), size_t(group),
                    atom_netlist_.block_name(root_blk_id).c_str(), site_path.c_str(),
                    atom_netlist_.block_name(failure.occupying_atom).c_str());
            break;
        case e_locked_site_failure::MOLECULE_DOES_NOT_FIT:
            VTR_LOG("\t\tRelative macro '%s' group %zu: the molecule rooted at atom '%s' could not be placed on its "
                    "primitive site '%s' in cluster type '%s' (the site is not occupied; it is unavailable in the "
                    "cluster's current modes, or the molecule does not fit or route there)\n",
                    macro_name.c_str(), size_t(group),
                    atom_netlist_.block_name(root_blk_id).c_str(), site_path.c_str(), cluster.type->name.c_str());
            break;
        default:
            VTR_ASSERT_MSG(false, "Unknown locked site failure reason");
            break;
    }
}

std::string RelativeMacroPacker::describe_unsatisfiable_locked_site(PackMoleculeId seed_molecule_id) const {
    const t_pack_molecule& seed_mol = prepacker_.get_molecule(seed_molecule_id);
    const t_unsatisfiable_locked_site& record = last_unsatisfiable_locked_site_;
    if (record.atom != prepacker_.get_molecule_root_atom(seed_molecule_id))
        return {};

    std::string message = "\tThe atom is locked to primitive site '" + record.site_path
                          + "' by relative macro '" + relative_macros_.get_group_macro_name(record.group)
                          + "' group " + std::to_string(size_t(record.group))
                          + ", and no candidate block type could honour it:\n";

    bool any_not_in_type = false;
    bool any_does_not_fit = false;
    for (const t_locked_site_failure& failure : record.failures) {
        const std::string& type_name = failure.cluster_type->name;
        switch (failure.reason) {
            case e_locked_site_failure::NOT_IN_TYPE: {
                any_not_in_type = true;
                // Find compatible site examples for the fatal error message.
                std::vector<std::string> available_paths;
                for (const t_pb_graph_node& candidate : failure.cluster_type->pb_graph_head->subtree()) {
                    if (available_paths.size() == MAX_REPORTED_SITE_PATHS)
                        break;
                    if (candidate.is_primitive() && primitive_type_feasible(record.atom, candidate.pb_type))
                        available_paths.push_back(candidate.hierarchical_type_name());
                }
                std::string examples;
                for (const std::string& path : available_paths)
                    examples += " '" + path + "'";
                message += "\t\t'" + type_name + "': has no such primitive; sites it does offer this atom include"
                           + (examples.empty() ? std::string(" none") : examples) + "\n";
                break;
            }
            case e_locked_site_failure::MOLECULE_DOES_NOT_FIT:
                any_does_not_fit = true;
                message += "\t\t'" + type_name + "': the site is free, but the molecule rooted at this atom does not "
                                                 "fit or route there\n";
                break;
            default:
                // Seed failures come from empty clusters, so SITE_OCCUPIED
                // cannot appear in this record.
                VTR_ASSERT_MSG(false, "Unexpected locked site failure reason for a seed molecule");
                break;
        }
    }

    // A root's site must accommodate the whole pack pattern. List the other
    // atoms to show why a site compatible with the root alone may still fail.
    std::string other_atoms;
    if (any_does_not_fit && seed_mol.pack_pattern != nullptr) {
        for (AtomBlockId blk_id : seed_mol.atom_block_ids) {
            if (!blk_id.is_valid() || blk_id == record.atom)
                continue;
            other_atoms += " '" + atom_netlist_.block_name(blk_id) + "'";
        }
    }
    if (!other_atoms.empty()) {
        message += "\tAtom '" + atom_netlist_.block_name(record.atom) + "' roots a '"
                   + seed_mol.pack_pattern->name + "' molecule, which must also find primitives for:" + other_atoms;
        message += "\n\tA site for the root only works where the whole molecule fits. If this atom did not root "
                   "this molecule when the sites were recorded, the netlist has changed; regenerate the macro "
                   "against it.\n";
    }

    // Suggest regenerating the path only if no candidate type contains it.
    // The constraints loader currently rejects such paths, but keep the
    // diagnostic here in case that validation changes.
    const bool site_exists_somewhere = std::any_of(record.failures.begin(), record.failures.end(),
                                                   [](const t_locked_site_failure& failure) {
                                                       return failure.reason != e_locked_site_failure::NOT_IN_TYPE;
                                                   });
    if (any_not_in_type && !site_exists_somewhere) {
        message += "\tA site path fixes the block type and the mode at every level, so a path recorded on another "
                   "architecture or another mode never matches. Regenerate the macro against this netlist and "
                   "architecture.\n";
    }

    return message;
}

void RelativeMacroPacker::prioritize_seeds(std::vector<PackMoleculeId>& seeds) const {
    // Seed relative placement groups first so other clusters cannot take their
    // molecules before the groups are packed together.
    auto relative_end = std::stable_partition(seeds.begin(), seeds.end(), [&](PackMoleculeId mol_id) {
        return molecule_group(mol_id).is_valid();
    });

    // Within the constrained molecules, put locked seeds first. The seed is
    // placed before the rest of its group, so an unlocked seed could occupy
    // a site needed by a locked group member.
    std::stable_partition(seeds.begin(), relative_end, [&](PackMoleculeId mol_id) {
        return molecule_is_locked_to_site(mol_id);
    });
}

std::vector<PackMoleculeId> RelativeMacroPacker::group_molecules_to_pack(UserRelativeGroupId group,
                                                                         LegalizationClusterId cluster_id,
                                                                         const ClusterLegalizer& cluster_legalizer,
                                                                         int log_verbosity) const {
    VTR_ASSERT(group.is_valid());
    std::vector<PackMoleculeId> order;

    // Collect unclustered group molecules. Molecules in other clusters cannot
    // be moved here; count them for the split-group log. The end-of-pass check
    // reports any split as an error.
    std::unordered_set<PackMoleculeId> seen;
    size_t num_elsewhere = 0;
    AtomBlockId first_elsewhere;
    for (AtomBlockId blk_id : relative_macros_.get_group(group).atoms) {
        PackMoleculeId mol_id = prepacker_.get_atom_molecule(blk_id);
        if (!seen.insert(mol_id).second)
            continue;

        if (!cluster_legalizer.is_mol_clustered(mol_id)) {
            order.push_back(mol_id);
        } else if (cluster_legalizer.get_atom_cluster(blk_id) != cluster_id) {
            num_elsewhere++;
            if (!first_elsewhere.is_valid())
                first_elsewhere = blk_id;
        }
    }

    // Summarize the split once per attempt instead of logging every atom.
    VTR_LOGV(log_verbosity > 1 && num_elsewhere > 0,
             "\tRelative macro '%s' group %zu is split: %zu of its molecules are already in other clusters "
             "(first: atom '%s' in cluster %zu) while cluster %zu is being seeded with the same group.\n",
             relative_macros_.get_group_macro_name(group).c_str(), size_t(group),
             num_elsewhere,
             atom_netlist_.block_name(first_elsewhere).c_str(),
             size_t(cluster_legalizer.get_atom_cluster(first_elsewhere)),
             size_t(cluster_id));

    // Reserve locked sites first, preserving constraints-file order. Try larger
    // unlocked molecules before smaller ones.
    std::stable_sort(order.begin(), order.end(), [&](PackMoleculeId lhs, PackMoleculeId rhs) {
        bool lhs_locked = molecule_is_locked_to_site(lhs);
        bool rhs_locked = molecule_is_locked_to_site(rhs);
        if (lhs_locked != rhs_locked)
            return lhs_locked;
        if (lhs_locked)
            return false;
        return prepacker_.get_molecule_num_valid_atoms(lhs) > prepacker_.get_molecule_num_valid_atoms(rhs);
    });

    return order;
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
            // Include locked sites to help diagnose why the group was split.
            const std::string& site_path = relative_macros.get_atom_locked_site_path(blk_id);
            report += " '" + atom_netlist.block_name(blk_id) + "' ("
                      + (!site_path.empty() ? "site '" + site_path + "', " : "")
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
