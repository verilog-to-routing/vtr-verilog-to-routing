/**
 * @file
 * @author  Alex Singer
 * @date    October 2024
 * @brief   Independent verify methods to check invariants on the clustering
 *          that ensure that the given clustering is valid and can be used with
 *          the rest of the VPR flow.
 */

#pragma once

#include <unordered_set>
#include "vtr_vector.h"

// Forward declarations
class AtomBlockId;
class AtomLookup;
class AtomNetlist;
class ClusterBlockId;
class ClusteredNetlist;
class PartitionRegion;
class UserPlaceConstraints;
class VprContext;

/**
 * @brief Verify the clustering of the atom netlist.
 *
 * This verifier is independent to how the clustering was performed and checks
 * invariants that all clusterings must adhere to in order to be used in the
 * rest of the VPR flow. The assumption is that if a clustering passes this
 * verifier it can be passed into placement without issue.
 *
 * By design, this verifier uses no global variables and tries to recompute
 * anything that it does not assume.
 *
 * Invariants (are checked by this function):
 *  - All atoms are in clusters.
 *  - The atom lookup is consistent with the clb_atoms.
 *  - The pbs of the atoms are consistent with the clusters.
 *  - The floorplanning constraints on the clusters are consistent with the
 *    floorplanning constraints on the atoms within each cluster.
 *
 * Assumptions (are not checked by this function):
 *  - The atom netlist is valid (matches the input design).
 *  - The user placement constraints are valid (match what was passed by the
 *    user).
 *
 *  @param clb_nlist            The clustered netlist to verify.
 *  @param atom_nlist           The atom netlist used to generate the
 *                              clustering.
 *  @param atom_lookup          A lookup between the atoms and the clusters they
 *                              are in.
 *  @param clb_atoms            The atoms in each cluster.
 *  @param cluster_constraints  Floorplanning constraints on each cluster.
 *  @param constraints          Floorplanning constraints on each atom.
 *
 *  @return The number of errors found in the clustering. Will also print error
 *          log messages for each error found.
 */
unsigned verify_clustering(const ClusteredNetlist& clb_nlist,
                           const AtomNetlist& atom_nlist,
                           const AtomLookup& atom_lookup,
                           const vtr::vector<ClusterBlockId, std::unordered_set<AtomBlockId>>& clb_atoms,
                           const vtr::vector<ClusterBlockId, PartitionRegion>& cluster_constraints,
                           const UserPlaceConstraints& constraints);

/**
 * @brief Verifies the clustering of the atom netlist as it appears in the
 *        given context.
 *
 * This performs the verification in the method above, but performs it on the
 * given VPR context itself. This verifies that the actual clustering being used
 * through the rest of the flow is valid.
 *
 * NOTE: The verify_clustering method with more arguments is used when one
 *       wishes to verify a temporary clustering befor updating the actual
 *       global VPR context.
 *
 *  @param ctx  The global VPR context variable found in g_vpr_ctx.
 *
 *  @return The number of errors found in the clustering. Will also print error
 *          log messages for each error found.
 */
unsigned verify_clustering(const VprContext& ctx);

