/**
 * @file
 * @author  Alex Singer
 * @date    October 2024
 * @brief   Definitions of the independent verifier.
 *
 * By design, these methods should not use any global variables and anything
 * that it does not assume should be re-computed from scratch. This ensures that
 * the clustering is actually valid.
 */

#include "verify_clustering.h"
#include <unordered_set>
#include "atom_lookup.h"
#include "atom_netlist.h"
#include "clustered_netlist.h"
#include "clustered_netlist_fwd.h"
#include "partition.h"
#include "physical_types.h"
#include "region.h"
#include "user_place_constraints.h"
#include "vpr_context.h"
#include "vtr_log.h"
#include "vtr_vector.h"

/**
 * @brief Checks that the atom_lookup is consistent with the clb_atoms.
 *
 * This checks the following invariants:
 *  - Every atom has a clb in the atom_lookup.
 *  - Every atom is in the clb_atoms of the cluster in the atom_lookup.
 *  - Every cluster has some atoms in its clb_atoms.
 *  - Every cluster's clb_atoms agree that they are in that cluster.
 *
 * This assumes:
 *  - The atom netlist was generated correctly.
 *
 *  @param clb_nlist    The clustered netlist being checked.
 *  @param atom_nlist   The atom netlist used to create the clustering.
 *  @param atom_lookup  The atom lookup between the atoms and their clusters.
 *  @param clb_atoms    The atoms in each cluster.
 *
 *  @return The number of errors in the clustering atoms.
 */
static unsigned check_clustering_atom_consistency(const ClusteredNetlist& clb_nlist,
                                                  const AtomNetlist& atom_nlist,
                                                  const AtomLookup& atom_lookup,
                                                  const vtr::vector<ClusterBlockId, std::unordered_set<AtomBlockId>>& clb_atoms) {
    unsigned num_errors = 0;
    // Make sure that every atom is in a cluster.
    for (AtomBlockId atom_blk_id : atom_nlist.blocks()) {
        // Check that the atom_lookup has a clb for this atom.
        ClusterBlockId atom_clb_blk_id = atom_lookup.atom_clb(atom_blk_id);
        if (!atom_clb_blk_id.is_valid()) {
            VTR_LOG_ERROR(
                "Atom block %zu does not have a clb in the atom_lookup.\n",
                size_t(atom_blk_id));
            num_errors++;
        }
        // Check that this atom is in the cluster according to clb_atoms
        const std::unordered_set<AtomBlockId>& atoms_in_clb = clb_atoms[atom_clb_blk_id];
        if (atoms_in_clb.find(atom_blk_id) == atoms_in_clb.end()) {
            VTR_LOG_ERROR(
                "Atom block %zu is in clb %zu according to the atom lookup, "
                "howeever it is not in the cluster according to the cluster.\n",
                size_t(atom_blk_id), size_t(atom_clb_blk_id));
            num_errors++;
        }
    }
    // Check that every cluster is being used. Clusters with no atoms in them
    // do not make sense.
    for (ClusterBlockId clb_blk_id : clb_nlist.blocks()) {
        const std::unordered_set<AtomBlockId>& atoms_in_clb = clb_atoms[clb_blk_id];
        if (atoms_in_clb.size() == 0) {
            VTR_LOG_ERROR(
                "Cluster block %zu found to be empty (has no atoms in it).\n",
                size_t(clb_blk_id));
            num_errors++;
        }
        for (AtomBlockId atom_blk_id : atoms_in_clb) {
            if (!atom_blk_id.is_valid()) {
                VTR_LOG_ERROR(
                    "Cluster block %zu has an invalid block in it.\n",
                    size_t(clb_blk_id));
                num_errors++;
            }
            // Check that there are no duplicates (an atom block in multiple
            // clusters).
            if (atom_lookup.atom_clb(atom_blk_id) != clb_blk_id) {
                VTR_LOG_ERROR(
                    "Cluster block %zu constains atom block %zu which does "
                    "not appear to be in it according to the atom lookup.\n",
                    size_t(clb_blk_id), size_t(atom_blk_id));
                num_errors++;
            }
        }
    }
    return num_errors;
}

/**
 * @brief Helper method to check if the given atom block's pb is a descendent
 *        of the given cluster's pb.
 */
static bool is_atom_pb_in_cluster_pb(AtomBlockId atom_blk_id,
                                     ClusterBlockId clb_blk_id,
                                     const AtomLookup& atom_lookup,
                                     const ClusteredNetlist& clb_nlist) {
    // Get the pbs
    const t_pb* atom_pb = atom_lookup.atom_pb(atom_blk_id);
    const t_pb* cluster_pb = clb_nlist.block_pb(clb_blk_id);
    // For the atom pb to be a part of the cluster pb, the atom pb must be a
    // descendent of the cluster pb (the cluster pb is the ancestor to all atom
    // pbs it contains).
    const t_pb* cur_pb = atom_pb;
    while (cur_pb != nullptr) {
        if (cur_pb == cluster_pb)
            return true;
        cur_pb = cur_pb->parent_pb;
    }
    return false;
}

/**
 * @brief Check consistency between the cluster pbs and the atom pbs.
 *
 * This checks the following invariants:
 *  - Every cluster has a root pb.
 *  - Every cluster's pb matches the pb of the cluster block type.
 *  - Every atom has a primitive pb.
 *  - Every atom's primitive pb is a descendent of the atom's cluster pb.
 *
 *  @param clb_nlist    The clustered netlist being checked.
 *  @param atom_nlist   The atom netlist used to create the clustering.
 *  @param atom_lookup  The atom lookup between the atoms and their clusters.
 *
 *  @return The number of errors in the clustering pbs.
 */
static unsigned check_clustering_pb_consistency(const ClusteredNetlist& clb_nlist,
                                                const AtomNetlist& atom_nlist,
                                                const AtomLookup& atom_lookup) {

    unsigned num_errors = 0;
    // Make sure that every cluster has a root pb.
    for (ClusterBlockId clb_blk_id : clb_nlist.blocks()) {
        const t_pb* clb_pb = clb_nlist.block_pb(clb_blk_id);
        if (clb_pb == nullptr) {
            VTR_LOG_ERROR(
                "Cluster block %zu does not have a pb.\n",
                size_t(clb_blk_id));
            num_errors++;
            continue;
        }
        if (!clb_pb->is_root()) {
            VTR_LOG_ERROR(
                "Cluster block %zu has a pb which is not a root pb.\n",
                size_t(clb_blk_id));
            num_errors++;
        }
        // Make sure that every cluster pb's block type matches the pb. We can
        // check this by seeing if the pb_graph_node of the pb matches the pb
        // graph head of the type.
        t_logical_block_type_ptr clb_blk_type = clb_nlist.block_type(clb_blk_id);
        if (clb_blk_type->pb_graph_head != clb_pb->pb_graph_node) {
            VTR_LOG_ERROR(
                "Cluster block %zu has a pb whose graph node does not match "
                "the pb_graph_head of its block type: %s.\n",
                size_t(clb_blk_id), clb_blk_type->name.c_str());
            num_errors++;
        }
        // TODO: Should check the pb_route. Tried checking that the pb_route
        //       always exists, but there appear to be cases when it does not
        //       exist (which may be ok).
    }
    // Make sure that every atom is a primitive pb and is in its cluster's pb.
    for (AtomBlockId atom_blk_id : atom_nlist.blocks()) {
        // If this atom is not in a cluster, another error will be produced
        // elsewhere. Cannot get the pb of NULL, so skip this atom.
        ClusterBlockId atom_clb_blk_id = atom_lookup.atom_clb(atom_blk_id);
        if (!atom_clb_blk_id.is_valid())
            continue;
        const t_pb* atom_pb = atom_lookup.atom_pb(atom_blk_id);
        // Make sure the atom's pb exists
        if (atom_pb == nullptr) {
            VTR_LOG_ERROR(
                "Atom block %zu in cluster block %zu does not have a pb.\n",
                size_t(atom_blk_id), size_t(atom_clb_blk_id));
            num_errors++;
        } else {
            // Sanity check: atom_pb == pb_atom
            if (atom_lookup.pb_atom(atom_pb) != atom_blk_id) {
                VTR_LOG_ERROR(
                    "Atom block %zu in cluster block %zu has a pb which "
                    "belongs to another atom.\n",
                    size_t(atom_blk_id), size_t(atom_clb_blk_id));
                num_errors++;
            }
            // Make sure it is a primitve
            if (!atom_pb->is_primitive()) {
                VTR_LOG_ERROR(
                    "Atom block %zu in cluster block %zu has a pb which is not "
                    "a primitive pb.\n",
                    size_t(atom_blk_id), size_t(atom_clb_blk_id));
                num_errors++;
            }
            // Check that the atom's primitive pb is in its cluster's pb.
            if (!is_atom_pb_in_cluster_pb(atom_blk_id, atom_clb_blk_id,
                                          atom_lookup, clb_nlist)) {
                VTR_LOG_ERROR(
                    "Atom block %zu in cluster block  %zu is not in its "
                    "cluster's pb.\n",
                    size_t(atom_blk_id), size_t(atom_clb_blk_id));
                num_errors++;
            }
        }
    }
    return num_errors;
}

/**
 * @brief Check that the floorplanning constraints on the cluster is consistent
 *        with the floorplanning constraints on the atoms.
 *
 * This checks the following invariants:
 *  - If a cluster is unconstrained, all of its atoms are unconstrained.
 *  - If a cluster is constrained, at least one of its atoms are constrained.
 *  - If a cluster is constrained, each of its constrained atoms can be placed
 *    within its constrained region.
 *  - If a cluster is constrained, all of its atoms have some points on the grid
 *    that is legal for all of the atoms to exist there.
 *  - If a cluster is constrained, its constrained region is equal to or smaller
 *    than the region that is defined by the intersection of all atom constraints.
 *
 * This makes the following assumptions:
 *  - The cluster atoms are consistent (see the other method in this file).
 *  - The user place constraints are correct (correctly read from user).
 *
 *  @param clb_nlist            The clustered netlist being checked.
 *  @param clb_atoms            The atoms in each cluster.
 *  @param cluster_constraints  The constraints on the clusters.
 *  @param constraints          The user placement constraints on the atoms.
 *
 *  @return The number of errors in the clustering floorplanning.
 */
static unsigned check_clustering_floorplanning_consistency(
                    const ClusteredNetlist& clb_nlist,
                    const vtr::vector<ClusterBlockId, std::unordered_set<AtomBlockId>>& clb_atoms,
                    const vtr::vector<ClusterBlockId, PartitionRegion>& cluster_constraints,
                    const UserPlaceConstraints& constraints) {
    unsigned num_errors = 0;
    // Check that each cluster has a constraint.
    if (cluster_constraints.size() != clb_nlist.blocks().size()) {
        VTR_LOG_ERROR(
            "The size of the cluster constraints vector does not match the "
            "number of clusters in the netlist.\n");
        num_errors++;
        // Returning here since this could cause issues below.
        return num_errors;
    }
    // Make sure that every atom in each cluster can be placed in that cluster.
    for (ClusterBlockId clb_blk_id : clb_nlist.blocks()) {
        const PartitionRegion& cluster_pr = cluster_constraints[clb_blk_id];
        const std::unordered_set<AtomBlockId>& atoms_in_clb = clb_atoms[clb_blk_id];
        if (cluster_pr.empty()) {
            // If the cluster is unconstrained, make sure all the atoms it
            // contains are unconstrained.
            for (AtomBlockId atom_blk_id : atoms_in_clb) {
                PartitionId atom_part_id = constraints.get_atom_partition(atom_blk_id);
                if (atom_part_id.is_valid()) {
                    VTR_LOG_ERROR(
                        "Cluster block %zu is unconstrained but contains "
                        "constrained atom block %zu.\n",
                        size_t(clb_blk_id), size_t(atom_blk_id));
                    num_errors++;
                }
            }
        } else {
            // If the cluster is constrained:
            // At least one of the atoms in the cluster must be constrained.
            bool an_atom_is_constrained = false;
            for (AtomBlockId atom_blk_id : atoms_in_clb) {
                PartitionId atom_part_id = constraints.get_atom_partition(atom_blk_id);
                if (atom_part_id.is_valid()) {
                    an_atom_is_constrained = true;
                    break;
                }
            }
            if (!an_atom_is_constrained) {
                VTR_LOG_ERROR(
                    "Cluster block %zu is constrained but does not contain any "
                    "constrained atoms.\n",
                    size_t(clb_blk_id));
                num_errors++;
            }
            // Check that the intersection of each atom's PR with the cluster
            // is non-empty. This implies that a placement could theoretically
            // exist.
            for (AtomBlockId atom_blk_id : atoms_in_clb) {
                PartitionId atom_part_id = constraints.get_atom_partition(atom_blk_id);
                // If the atom is not constrained, continue.
                if (!atom_part_id.is_valid())
                    continue;
                // Check if an intersection exists between the atom's PR and the
                // cluster's PR.
                bool intersection_exists = false;
                const PartitionRegion& atom_pr = constraints.get_partition_pr(atom_part_id);
                for (const auto& cluster_region : cluster_pr.get_regions()) {
                    for (const auto& atom_region : atom_pr.get_regions()) {
                        Region intersect_region = intersection(cluster_region, atom_region);
                        if (!intersect_region.empty()) {
                            intersection_exists = true;
                            break;
                        }
                    }
                    if (intersection_exists)
                        break;
                }
                if (!intersection_exists) {
                    VTR_LOG_ERROR(
                        "Cluster block %zu contains the atom block %zu which "
                        "cannot be placed within its placement constraints.\n",
                        size_t(clb_blk_id), size_t(atom_blk_id));
                    num_errors++;
                }
            }
            // Compute the intersection of all the atom PRs in the cluster.
            PartitionRegion calc_cluster_pr;
            for (AtomBlockId atom_blk_id : atoms_in_clb) {
                PartitionId atom_part_id = constraints.get_atom_partition(atom_blk_id);
                // If the atom is not constrained, continue.
                if (!atom_part_id.is_valid())
                    continue;
                // Get the intersection of the atom's PR and the intersection of
                // all atom PRs that came before.
                const PartitionRegion& atom_pr = constraints.get_partition_pr(atom_part_id);
                if (calc_cluster_pr.empty()) {
                    calc_cluster_pr = atom_pr;
                    continue;
                }
                std::vector<Region> int_regions;
                for (const auto& cluster_region : calc_cluster_pr.get_regions()) {
                    for (const auto& atom_region : atom_pr.get_regions()) {
                        Region intersection_region = intersection(cluster_region, atom_region);
                        if (!intersection_region.empty()) {
                            int_regions.push_back(intersection_region);
                        }
                    }
                }
                calc_cluster_pr.set_partition_region(int_regions);
            }
            // If the calculated cluster pr is empty, then the atoms' PRs
            // conflict with each other and cannot be in the same cluster.
            if (calc_cluster_pr.empty()) {
                VTR_LOG_ERROR(
                    "Cluster block %zu contains constrained atoms whose "
                    "constraints conflict.\n",
                    size_t(clb_blk_id));
                num_errors++;
            }
            // Check that the calculate cluster PR matches the actual cluster
            // PR.
            // NOTE: This check can be very expensive.
            // This is not checking true equality between the PRs. This is
            // checking that cluster_pr is a subset of calc_cluster_pr. Since
            // we know that calc_cluster_pr is the smallest PR set that
            // constrains all the atoms within the cluster, if cluster_region
            // is a subset of that, it should be equal to or smaller (which
            // are both legal).
            for (const auto& cluster_region : cluster_pr.get_regions()) {
                bool found_region = false;
                for (const auto& calc_cluster_region : calc_cluster_pr.get_regions()) {
                    // This equality is a deep check. it checks that the actual
                    // rectangles of the regions are equal.
                    if (calc_cluster_region == cluster_region) {
                        found_region = true;
                        break;
                    }
                }
                if (!found_region) {
                    VTR_LOG_ERROR(
                        "Cluster block %zu cluster constraint does not match "
                        "the intersection of all constrained blocks in it\n",
                        size_t(clb_blk_id));
                    num_errors++;
                    break;
                }
            }
        }
    }
    return num_errors;
}

unsigned verify_clustering(const ClusteredNetlist& clb_nlist,
                           const AtomNetlist& atom_nlist,
                           const AtomLookup& atom_lookup,
                           const vtr::vector<ClusterBlockId, std::unordered_set<AtomBlockId>>& clb_atoms,
                           const vtr::vector<ClusterBlockId, PartitionRegion>& cluster_constraints,
                           const UserPlaceConstraints& constraints) {
    unsigned num_errors = 0;
    // Check that every cluster has an entry in the clb_atoms vector.
    if (clb_atoms.size() != clb_nlist.blocks().size()) {
        VTR_LOG_ERROR(
            "The number of clusters in the clb_atoms does not match the number "
            "of clusters in the netlist.\n");
        num_errors++;
        // Return here since this error can cause serious issues below.
        return num_errors;
    }
    // Check conssitency between which clusters the atom's think thet are in and
    // which atoms the clusters think they have.
    num_errors += check_clustering_atom_consistency(clb_nlist,
                                                    atom_nlist,
                                                    atom_lookup,
                                                    clb_atoms);
    // Check that the cluster pbs and atom pbs are consistent with each other.
    // Each cluster should have a root pb, each atom should be a leaf descendent
    // of its cluster's pb.
    num_errors += check_clustering_pb_consistency(clb_nlist,
                                                  atom_nlist,
                                                  atom_lookup);
    // Check that all clusters are clustering atoms which are constrained to
    // overlapping regions and the constraints of the clusters are consistent
    // with the atoms they contain.
    num_errors += check_clustering_floorplanning_consistency(clb_nlist,
                                                             clb_atoms,
                                                             cluster_constraints,
                                                             constraints);
    // TODO: There exists more checks for the clustering in base/check_netlist.cpp
    //       These checks check for duplicate names and that the nets between
    //       cluster blocks make sense. May be a good idea to bring this in here
    //       eventually.
    return num_errors;
}

unsigned verify_clustering(const VprContext& ctx) {
    // Verify the clustering within the given context.
    return verify_clustering(ctx.clustering().clb_nlist,
                             ctx.atom().nlist,
                             ctx.atom().lookup,
                             ctx.clustering().atoms_lookup,
                             ctx.floorplanning().cluster_constraints,
                             ctx.floorplanning().constraints);
}

