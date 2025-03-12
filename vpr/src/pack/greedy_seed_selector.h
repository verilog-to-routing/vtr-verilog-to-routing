/**
 * @file
 * @author  Alex Singer
 * @date    November 2024
 * @brief   The declaration of the greedy seed selector class which selects the
 *          seed molecules for starting new clusters in the greedy clusterer.
 */

#pragma once

#include "vpr_types.h"

// Forward declarations
class AtomNetlist;
class ClusterLegalizer;
class Prepacker;
struct t_molecule_stats;

/**
 * @brief A selector class which will propose good seed values to start new
 *        clusters in the greedy clusterer.
 *
 * In greedy clustering algorithms, the order in which clusters are generated
 * can have an effect on the quality of the clustering. This class proposes
 * good seed molecules based on heuristics which give each molecule a "seed
 * gain". This class will not propose a molecule which it has already proposed
 * or has already been clustered.
 */
class GreedySeedSelector {
public:
    /**
     * @brief Constructor of the Greedy Seed Selector class. Pre-computes the
     *        gains of each molecule internally to make getting seeds later very
     *        quick.
     *
     *  @param atom_netlist
     *              The netlist of atoms to cluster.
     *  @param prepacker
     *              The prepacker used to generate pack-pattern molecules of the
     *              atoms in the netlist.
     *  @param seed_type
     *              Controls the algorithm used to compute the seed gain for
     *              each molecule.
     *  @param max_molecule_stats
     *              The maximum stats over all molecules. Used for normalizing
     *              terms in the gain.
     *  @param atom_criticality
     *              The timing criticality of each atom.
     */
    GreedySeedSelector(const AtomNetlist& atom_netlist,
                       const Prepacker& prepacker,
                       const e_cluster_seed seed_type,
                       const t_molecule_stats& max_molecule_stats,
                       const vtr::vector<AtomBlockId, float>& atom_criticality);

    /**
     * @brief Propose a new seed molecule to start a new cluster with. If no
     *        unclustered molecules exist, will return nullptr.
     *
     * This method will never return a molecule which has already been clustered
     * (according to the cluster legalizer) and will never propose a molecule
     * that it already proposed.
     *
     * This method assumes that once a molecule is clustered, it will never be
     * unclustered.
     *
     *  @param prepacker
     *              The prepacker object that stores the molecules.
     *  @param cluster_legalizer
     *              The cluster legalizer object that is used to create the
     *              clusters. This is used to check if a molecule has already
     *              been clustered or not.
     */
    t_pack_molecule* get_next_seed(const Prepacker& prepacker,
                                   const ClusterLegalizer& cluster_legalizer);

    // TODO: Maybe create an update_seed_gains method to update the seed atoms
    //       list using current clustering information.

private:

    /// @brief The index of the next seed to propose in the seed_atoms vector.
    ///        This is set to 0 in the constructor and incremented as more seeds
    ///        are proposed.
    size_t seed_index_;

    /// @brief A list of seed atoms, sorted in decreasing order of gain. This
    ///        is computed in the constructor and is traversed when a new seed
    ///        is being proposed.
    // FIXME: This should really be seed molecules. It looks like the only
    //        reason it isn't is because of the atom criticality. May want to
    //        create the concept of molecule criticality. Currently, the max
    //        criticality of any block in the molecule is technically being
    //        used.
    std::vector<AtomBlockId> seed_atoms_;
};

