/**
 * @file
 * @brief Implementation of RelativeMacroPacker.
 */

#include "relative_macro_packing.h"

#include <algorithm>
#include <string>
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

// Limit the examples printed for a block type that lacks the required site.
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
    warn_non_root_locked_sites_();
}

void RelativeMacroPacker::init_chain_owners_() {
    // Any constrained atom identifies the chain's owner. Validate agreement
    // among all constrained atoms after every chain's owner has been recorded.
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
    // Chain molecules must agree with the owner found across the entire chain;
    // other molecules only need agreement among their own constrained atoms.
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
                                "belong to the same prepacked %s but are assigned to different groups. "
                                "All constrained atoms of a molecule or chain must belong to one relative placement "
                                "group. Update the group assignments in the constraints file; other atoms of the "
                                "molecule or chain may be left unconstrained.\n",
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
    for (PackMoleculeId mol_id : prepacker_.molecules()) {
        const t_pack_molecule& molecule = prepacker_.get_molecule(mol_id);
        AtomBlockId root_blk_id = prepacker_.get_molecule_root_atom(mol_id);
        if (relative_macros_.get_atom_locked_site(root_blk_id) != nullptr)
            continue;

        // Without a root lock, the packer cannot use a non-root site constraint
        // to select the molecule's placement.
        for (AtomBlockId blk_id : molecule.atom_block_ids) {
            if (!blk_id.is_valid() || blk_id == root_blk_id)
                continue;
            const t_pb_graph_node* site = relative_macros_.get_atom_locked_site(blk_id);
            if (site == nullptr)
                continue;

            UserRelativeGroupId atom_group = relative_macros_.get_atom_group(blk_id);
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "Atom '%s' of relative macro '%s' group %zu is locked to primitive site '%s', but "
                            "its molecule's root atom '%s' has no site constraint. VPR chooses the molecule's "
                            "placement through its root. Add a compatible site_path for the root atom, or "
                            "remove this non-root atom's site_path.\n",
                            atom_netlist_.block_name(blk_id).c_str(),
                            relative_macros_.get_group_macro_name(atom_group).c_str(),
                            size_t(atom_group),
                            site->hierarchical_type_name().c_str(),
                            atom_netlist_.block_name(root_blk_id).c_str());
        }
    }
}

void RelativeMacroPacker::warn_non_root_locked_sites_() const {
    // The root selects the molecule's placement; its pack pattern determines
    // the remaining sites. Report all non-root locks in a single warning.
    size_t num_non_root_sites = 0;
    AtomBlockId first_non_root_site;

    for (PackMoleculeId mol_id : prepacker_.molecules()) {
        const t_pack_molecule& molecule = prepacker_.get_molecule(mol_id);
        AtomBlockId root_blk_id = prepacker_.get_molecule_root_atom(mol_id);

        for (AtomBlockId blk_id : molecule.atom_block_ids) {
            if (!blk_id.is_valid() || blk_id == root_blk_id)
                continue;
            if (relative_macros_.get_atom_locked_site(blk_id) == nullptr)
                continue;

            num_non_root_sites++;
            if (!first_non_root_site.is_valid())
                first_non_root_site = blk_id;
        }
    }

    VTR_LOGV_WARN(num_non_root_sites > 0,
                  "VPR cannot select sites independently for %zu constrained atom(s) (first: '%s'). Each is packed "
                  "with other atoms according to an architecture pack pattern; the root atom's site determines "
                  "the others. These site_path constraints will be checked after packing, and a mismatch will "
                  "stop VPR. Check that the constraints agree with the pack pattern.\n",
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

    // Also check the existing chain when a non-chain molecule introduces a group.
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

    // The compatibility query also uses this check, before attempting placement.
    VTR_LOGV(log_verbosity > 3,
             "\t\t\t Cannot combine this molecule and cluster: their long chains or relative placement groups "
             "have conflicting ownership\n");
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
                     "\t\t\t Relative group check passed: cluster would adopt group %zu if packing succeeds\n",
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

const t_pb_graph_node* RelativeMacroPacker::molecule_locked_site(PackMoleculeId molecule_id) const {
    return relative_macros_.get_atom_locked_site(prepacker_.get_molecule_root_atom(molecule_id));
}

bool site_in_cluster_type(const t_pb_graph_node* site, t_logical_block_type_ptr cluster_type) {
    VTR_ASSERT_SAFE(site != nullptr && cluster_type != nullptr);
    // Each logical block type has its own pb graph, so the site's root node
    // identifies its type.
    const t_pb_graph_node* node = site;
    while (!node->is_root())
        node = node->parent_pb_graph_node;
    return node == cluster_type->pb_graph_head;
}

void RelativeMacroPacker::log_locked_site_failure(PackMoleculeId molecule_id,
                                                  const LegalizationCluster& cluster,
                                                  const AtomPBBimap& atom_pb_lookup,
                                                  const t_pb_graph_node* site) const {
    VTR_ASSERT(site != nullptr);
    AtomBlockId root_blk_id = prepacker_.get_molecule_root_atom(molecule_id);
    const UserRelativeGroupId group = molecule_group(molecule_id);
    const std::string& macro_name = relative_macros_.get_group_macro_name(group);
    const std::string site_path = site->hierarchical_type_name();

    // An occupied site gives a specific conflict to report.
    for (PackMoleculeId cluster_mol_id : cluster.molecules) {
        for (AtomBlockId blk_id : prepacker_.get_molecule(cluster_mol_id).atom_block_ids) {
            if (!blk_id.is_valid())
                continue;
            const t_pb* atom_pb = atom_pb_lookup.atom_pb(blk_id);
            if (atom_pb != nullptr && atom_pb->pb_graph_node == site) {
                VTR_LOG("\t\tRelative macro '%s' group %zu: atom '%s' is locked to primitive site '%s', which is already "
                        "occupied by atom '%s'. These atoms cannot share this site in the current cluster.\n",
                        macro_name.c_str(), size_t(group),
                        atom_netlist_.block_name(root_blk_id).c_str(), site_path.c_str(),
                        atom_netlist_.block_name(blk_id).c_str());
                return;
            }
        }
    }

    // No occupant was found. The failed attempt does not identify which of
    // the remaining placement, mode, pin, or routing constraints rejected it.
    VTR_LOG("\t\tRelative macro '%s' group %zu: cannot pack the molecule rooted at atom '%s' at its required "
            "primitive site '%s' in block type '%s'. No atom occupies the site; other packing constraints "
            "prevented this placement.\n",
            macro_name.c_str(), size_t(group),
            atom_netlist_.block_name(root_blk_id).c_str(), site_path.c_str(), cluster.type->name.c_str());
}

std::string RelativeMacroPacker::describe_locked_site_failure(PackMoleculeId seed_molecule_id,
                                                              const std::vector<t_logical_block_type_ptr>& candidate_types) const {
    const t_pb_graph_node* site = molecule_locked_site(seed_molecule_id);
    if (site == nullptr)
        return {};

    const t_pack_molecule& seed_mol = prepacker_.get_molecule(seed_molecule_id);
    AtomBlockId root_blk_id = prepacker_.get_molecule_root_atom(seed_molecule_id);
    const UserRelativeGroupId group = molecule_group(seed_molecule_id);

    std::string message = "\tPrimitive site '" + site->hierarchical_type_name()
                          + "' required by relative macro '" + relative_macros_.get_group_macro_name(group)
                          + "' group " + std::to_string(size_t(group))
                          + " could not be used to start a cluster. Check this site_path along with the design's "
                            "other packing constraints.\n"
                            "\tCandidate block types:\n";

    // Each attempt started an empty cluster. Distinguish a missing site from
    // a failed packing attempt in the type that contains it.
    bool any_does_not_fit = false;
    for (t_logical_block_type_ptr cluster_type : candidate_types) {
        const std::string& type_name = cluster_type->name;
        if (!site_in_cluster_type(site, cluster_type)) {
            // Suggest sites compatible with the root atom. These examples do
            // not guarantee that the whole molecule can be packed there.
            std::vector<std::string> available_paths;
            for (const t_pb_graph_node& candidate : cluster_type->pb_graph_head->subtree()) {
                if (available_paths.size() == MAX_REPORTED_SITE_PATHS)
                    break;
                if (candidate.is_primitive() && primitive_type_feasible(root_blk_id, candidate.pb_type))
                    available_paths.push_back(candidate.hierarchical_type_name());
            }
            std::string examples;
            for (const std::string& path : available_paths)
                examples += " '" + path + "'";
            message += "\t\t'" + type_name + "': does not contain the required site. Alternative sites compatible "
                                             "with the root atom:"
                       + (examples.empty() ? std::string(" none") : examples) + "\n";
        } else {
            any_does_not_fit = true;
            message += "\t\t'" + type_name + "': contains the required site, but packing failed with this constraint\n";
        }
    }

    // Show the atoms that must accompany the root when checking alternative sites.
    std::string other_atoms;
    if (any_does_not_fit && seed_mol.pack_pattern != nullptr) {
        for (AtomBlockId blk_id : seed_mol.atom_block_ids) {
            if (!blk_id.is_valid() || blk_id == root_blk_id)
                continue;
            other_atoms += " '" + atom_netlist_.block_name(blk_id) + "'";
        }
    }
    if (!other_atoms.empty()) {
        message += "\tAtom '" + atom_netlist_.block_name(root_blk_id) + "' is the root of a '"
                   + seed_mol.pack_pattern->name + "' molecule, which must also pack these atoms:" + other_atoms;
        message += "\n\tThe root's site must allow all of these atoms to pack together. Any alternative sites "
                   "shown above were checked only for the root atom.\n";
    }

    return message;
}

void RelativeMacroPacker::prioritize_seeds(std::vector<PackMoleculeId>& seeds) const {
    // Starting these groups first reduces the risk of their molecules being
    // absorbed by clusters seeded with unconstrained atoms.
    auto relative_end = std::stable_partition(seeds.begin(), seeds.end(), [&](PackMoleculeId mol_id) {
        return molecule_group(mol_id).is_valid();
    });

    // The seed is placed before its group members. Use a locked root when
    // possible so an unlocked seed cannot take a group member's required site.
    std::stable_partition(seeds.begin(), relative_end, [&](PackMoleculeId mol_id) {
        return molecule_locked_site(mol_id) != nullptr;
    });
}

std::vector<PackMoleculeId> RelativeMacroPacker::group_molecules_to_pack(UserRelativeGroupId group,
                                                                         LegalizationClusterId cluster_id,
                                                                         const ClusterLegalizer& cluster_legalizer,
                                                                         int log_verbosity) const {
    VTR_ASSERT(group.is_valid());
    std::vector<PackMoleculeId> order;

    // A molecule may contain several group atoms; collect it only once.
    // Members already packed elsewhere cannot be moved into this cluster.
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

    // A split may be temporary during a packing attempt. Final packing checks
    // report an error if it persists.
    VTR_LOGV(log_verbosity > 1 && num_elsewhere > 0,
             "\tRelative macro '%s' group %zu is split: %zu of its molecules are already in other clusters "
             "(first: atom '%s' in cluster %zu) while cluster %zu is being seeded with the same group.\n",
             relative_macros_.get_group_macro_name(group).c_str(), size_t(group),
             num_elsewhere,
             atom_netlist_.block_name(first_elsewhere).c_str(),
             size_t(cluster_legalizer.get_atom_cluster(first_elsewhere)),
             size_t(cluster_id));

    // Place locked roots before other molecules can occupy their sites.
    // Stable partition preserves their order from the constraints file.
    auto unlocked_begin = std::stable_partition(order.begin(), order.end(), [&](PackMoleculeId mol_id) {
        return molecule_locked_site(mol_id) != nullptr;
    });
    // Larger unlocked molecules have fewer placement options as the cluster fills.
    std::stable_sort(unlocked_begin, order.end(), [&](PackMoleculeId lhs, PackMoleculeId rhs) {
        return prepacker_.get_molecule_num_valid_atoms(lhs) > prepacker_.get_molecule_num_valid_atoms(rhs);
    });

    return order;
}

/**
 * @brief Find relative placement groups whose atoms occupy multiple clusters.
 *
 * Unclustered atoms are left to the final clustering verifier.
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
            const t_pb_graph_node* site = relative_macros.get_atom_locked_site(blk_id);
            report += " '" + atom_netlist.block_name(blk_id) + "' ("
                      + (site != nullptr ? "site '" + site->hierarchical_type_name() + "', " : "")
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
                        "Each group must fit into one cluster, including its site constraints and pack patterns. "
                        "Reduce the groups or revise their site_path constraints to allow them to pack together.\n",
                        split_groups.size(),
                        report.c_str());
    } else {
        VTR_LOG_WARN("%zu relative placement group(s) are split across clusters:\n%s"
                     "Packing also failed to satisfy the device or floorplanning limits; "
                     "see the following error before revising the relative placement groups.\n",
                     split_groups.size(),
                     report.c_str());
    }
}
