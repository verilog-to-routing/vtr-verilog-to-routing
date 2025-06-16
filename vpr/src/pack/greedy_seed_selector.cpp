/**
 * @file
 * @author  Alex Singer
 * @date    November 2024
 * @brief   The definitions of the Greedy Seed Selector class.
 */

#include "greedy_seed_selector.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include "PreClusterTimingManager.h"
#include "atom_netlist.h"
#include "cluster_legalizer.h"
#include "echo_files.h"
#include "greedy_clusterer.h"
#include "logic_types.h"
#include "prepack.h"
#include "vpr_error.h"
#include "vpr_types.h"
#include "vtr_assert.h"
#include "vtr_math.h"
#include "vtr_vector.h"

/**
 * @brief Helper method that computes the seed gain of the given atom block.
 *
 * The seed_type variable selects which algorithm to use to compute the seed
 * gain.
 */
static inline float get_seed_gain(AtomBlockId blk_id,
                                  const AtomNetlist& atom_netlist,
                                  const Prepacker& prepacker,
                                  const LogicalModels& models,
                                  const e_cluster_seed seed_type,
                                  const t_molecule_stats& max_molecule_stats,
                                  const vtr::vector<AtomBlockId, float>& atom_criticality) {
    switch (seed_type) {
        // By criticality.
        // Intuition: starting a cluster with primitives that have timing-
        //            critical connections may help timing.
        case e_cluster_seed::TIMING:
            return atom_criticality[blk_id];
        // By number of used molecule input pins.
        // Intuition: molecules that use more inputs can be difficult to legally
        //            pack into partially full clusters. Use them as seeds
        //            instead.
        case e_cluster_seed::MAX_INPUTS: {
            PackMoleculeId blk_mol_id = prepacker.get_atom_molecule(blk_id);
            const t_molecule_stats molecule_stats = prepacker.calc_molecule_stats(blk_mol_id, atom_netlist, models);
            return molecule_stats.num_used_ext_inputs;
        }
        // By blended gain (criticality and inputs used).
        case e_cluster_seed::BLEND: {
            // Score seed gain of each block as a weighted sum of timing
            // criticality, number of tightly coupled blocks connected to
            // it, and number of external inputs.
            float seed_blend_fac = 0.5f;

            PackMoleculeId blk_mol_id = prepacker.get_atom_molecule(blk_id);
            const t_molecule_stats molecule_stats = prepacker.calc_molecule_stats(blk_mol_id, atom_netlist, models);
            VTR_ASSERT(max_molecule_stats.num_used_ext_inputs > 0);

            float used_ext_input_pin_ratio = vtr::safe_ratio<float>(molecule_stats.num_used_ext_inputs, max_molecule_stats.num_used_ext_inputs);
            float blend_gain = (seed_blend_fac * atom_criticality[blk_id]
                                + (1 - seed_blend_fac) * used_ext_input_pin_ratio);
            blend_gain *= (1 + 0.2 * (molecule_stats.num_blocks - 1));
            return blend_gain;
        }
        // By pins per molecule (i.e. available pins on primitives, not pins in use).
        // Intuition (a weak one): primitive types with more pins might be
        //                         harder to pack.
        case e_cluster_seed::MAX_PINS: {
            PackMoleculeId blk_mol_id = prepacker.get_atom_molecule(blk_id);
            const t_molecule_stats molecule_stats = prepacker.calc_molecule_stats(blk_mol_id, atom_netlist, models);
            return molecule_stats.num_pins;
        }
        // By input pins per molecule (i.e. available pins on primitives, not pins in use).
        // Intuition (a weak one): primitive types with more input pins might be
        //                         harder to pack.
        case e_cluster_seed::MAX_INPUT_PINS: {
            PackMoleculeId blk_mol_id = prepacker.get_atom_molecule(blk_id);
            const t_molecule_stats molecule_stats = prepacker.calc_molecule_stats(blk_mol_id, atom_netlist, models);
            return molecule_stats.num_input_pins;
        }
        case e_cluster_seed::BLEND2: {
            PackMoleculeId mol_id = prepacker.get_atom_molecule(blk_id);
            const t_molecule_stats molecule_stats = prepacker.calc_molecule_stats(mol_id, atom_netlist, models);

            float pin_ratio = vtr::safe_ratio<float>(molecule_stats.num_pins, max_molecule_stats.num_pins);
            float input_pin_ratio = vtr::safe_ratio<float>(molecule_stats.num_input_pins, max_molecule_stats.num_input_pins);
            float output_pin_ratio = vtr::safe_ratio<float>(molecule_stats.num_output_pins, max_molecule_stats.num_output_pins);
            float used_ext_pin_ratio = vtr::safe_ratio<float>(molecule_stats.num_used_ext_pins, max_molecule_stats.num_used_ext_pins);
            float used_ext_input_pin_ratio = vtr::safe_ratio<float>(molecule_stats.num_used_ext_inputs, max_molecule_stats.num_used_ext_inputs);
            float used_ext_output_pin_ratio = vtr::safe_ratio<float>(molecule_stats.num_used_ext_outputs, max_molecule_stats.num_used_ext_outputs);
            float num_blocks_ratio = vtr::safe_ratio<float>(molecule_stats.num_blocks, max_molecule_stats.num_blocks);
            float criticality = atom_criticality[blk_id];

            constexpr float PIN_WEIGHT = 0.;
            constexpr float INPUT_PIN_WEIGHT = 0.5;
            constexpr float OUTPUT_PIN_WEIGHT = 0.;
            constexpr float USED_PIN_WEIGHT = 0.;
            constexpr float USED_INPUT_PIN_WEIGHT = 0.2;
            constexpr float USED_OUTPUT_PIN_WEIGHT = 0.;
            constexpr float BLOCKS_WEIGHT = 0.2;
            constexpr float CRITICALITY_WEIGHT = 0.1;

            float gain = PIN_WEIGHT * pin_ratio
                         + INPUT_PIN_WEIGHT * input_pin_ratio
                         + OUTPUT_PIN_WEIGHT * output_pin_ratio

                         + USED_PIN_WEIGHT * used_ext_pin_ratio
                         + USED_INPUT_PIN_WEIGHT * used_ext_input_pin_ratio
                         + USED_OUTPUT_PIN_WEIGHT * used_ext_output_pin_ratio

                         + BLOCKS_WEIGHT * num_blocks_ratio
                         + CRITICALITY_WEIGHT * criticality;

            return gain;
        }
        default:
            VPR_FATAL_ERROR(VPR_ERROR_PACK, "Unrecognized cluster seed type");
            return 0.f;
    }
}

/**
 * @brief Helper method to print the seed gains of each Atom block and their
 *        criticalities.
 */
static inline void print_seed_gains(const char* fname,
                                    const std::vector<PackMoleculeId>& seed_mols,
                                    const vtr::vector<PackMoleculeId, float>& molecule_gain,
                                    const vtr::vector<AtomBlockId, float>& atom_criticality,
                                    const AtomNetlist& atom_netlist,
                                    const LogicalModels& models,
                                    const Prepacker& prepacker) {
    FILE* fp = vtr::fopen(fname, "w");

    // For pretty formatting determine the maximum name length
    int max_name_len = strlen("atom_block_name");
    int max_type_len = strlen("atom_block_type");
    for (auto blk_id : atom_netlist.blocks()) {
        max_name_len = std::max(max_name_len, (int)atom_netlist.block_name(blk_id).size());

        std::string model_name = models.model_name(atom_netlist.block_model(blk_id));
        max_type_len = std::max<int>(max_type_len, model_name.size());
    }

    fprintf(fp, "%-*s %-*s %8s %8s\n", max_name_len, "atom_block_name", max_type_len, "atom_block_type", "gain", "criticality");
    fprintf(fp, "\n");
    for (auto mol_id : seed_mols) {
        for (AtomBlockId blk_id : prepacker.get_molecule(mol_id).atom_block_ids) {
            std::string name = atom_netlist.block_name(blk_id);
            fprintf(fp, "%-*s ", max_name_len, name.c_str());

            std::string model_name = models.model_name(atom_netlist.block_model(blk_id));
            fprintf(fp, "%-*s ", max_type_len, model_name.c_str());

            fprintf(fp, "%*f ", std::max((int)strlen("gain"), 8), molecule_gain[mol_id]);
            fprintf(fp, "%*f ", std::max((int)strlen("criticality"), 8), atom_criticality[blk_id]);
            fprintf(fp, "\n");
        }
    }

    fclose(fp);
}

GreedySeedSelector::GreedySeedSelector(const AtomNetlist& atom_netlist,
                                       const Prepacker& prepacker,
                                       const e_cluster_seed seed_type,
                                       const t_molecule_stats& max_molecule_stats,
                                       const LogicalModels& models,
                                       const PreClusterTimingManager& pre_cluster_timing_manager)
    : seed_mols_(prepacker.molecules().begin(), prepacker.molecules().end()) {
    // Seed molecule list is initialized with all molecule in the netlist.

    // Pre-compute the criticality of each atom
    // Default criticalities set to zero (e.g. if not timing driven)
    vtr::vector<AtomBlockId, float> atom_criticality(atom_netlist.blocks().size(), 0.0f);
    if (pre_cluster_timing_manager.is_valid()) {
        // If the timing manager is valid (meaning the packing is timing driven)
        // compute the criticality of each atom.
        for (AtomBlockId atom_blk_id : atom_netlist.blocks()) {
            atom_criticality[atom_blk_id] = pre_cluster_timing_manager.calc_atom_setup_criticality(atom_blk_id, atom_netlist);
        }
    }

    // Maintain a lookup table of the seed gain for each molecule. This will be
    // used to sort the seed molecules.
    // Initially all gains are zero.
    vtr::vector<PackMoleculeId, float> molecule_gains(seed_mols_.size(), 0.f);

    // Get the seed gain of each molecule.
    for (PackMoleculeId mol_id : seed_mols_) {
        // Gain of each molecule is the maximum gain of its atoms
        float mol_gain = std::numeric_limits<float>::lowest();
        const std::vector<AtomBlockId>& molecule_atoms = prepacker.get_molecule(mol_id).atom_block_ids;
        for (AtomBlockId blk_id : molecule_atoms) {
            // If the molecule does not fit the entire pack pattern, it's possible to have invalid block ids in the molecule_atoms vector
            if (blk_id == AtomBlockId::INVALID()) {
                continue;
            }
            float atom_gain = get_seed_gain(blk_id,
                                            atom_netlist,
                                            prepacker,
                                            models,
                                            seed_type,
                                            max_molecule_stats,
                                            atom_criticality);
            mol_gain = std::max(mol_gain, atom_gain);
        }
        molecule_gains[mol_id] = mol_gain;
    }

    // Sort seeds in descending order of seed gain (i.e. highest seed gain first)
    //
    // Note that we use a *stable* sort here. It has been observed that different
    // standard library implementations (e.g. gcc-4.9 vs gcc-5) use sorting algorithms
    // which produce different orderings for seeds of equal gain (which is allowed with
    // std::sort which does not specify how equal values are handled). Using a stable
    // sort ensures that regardless of the underlying sorting algorithm the same seed
    // order is produced regardless of compiler.
    auto by_descending_gain = [&](const PackMoleculeId lhs, const PackMoleculeId rhs) {
        return molecule_gains[lhs] > molecule_gains[rhs];
    };
    std::stable_sort(seed_mols_.begin(), seed_mols_.end(), by_descending_gain);

    // Print the seed gains if requested.
    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_CLUSTERING_BLOCK_CRITICALITIES)) {
        print_seed_gains(getEchoFileName(E_ECHO_CLUSTERING_BLOCK_CRITICALITIES),
                         seed_mols_, molecule_gains, atom_criticality, atom_netlist, models, prepacker);
    }

    // Set the starting seed index (the index of the first molecule to propose).
    // The index of the first seed to propose is the first molecule in the
    // seed molecules vector (i.e. the one with the highest seed gain).
    seed_index_ = 0;
}

PackMoleculeId GreedySeedSelector::get_next_seed(const ClusterLegalizer& cluster_legalizer) {
    while (seed_index_ < seed_mols_.size()) {
        // Get the current seed molecule at the seed index and increment the
        // seed index.
        // All previous seed indices have been either proposed already or
        // are already clustered. This process assumes that once an atom
        // is clustered it will never become unclustered.
        PackMoleculeId seed_molecule_id = seed_mols_[seed_index_++];

        // If this molecule has been clustered, it cannot be proposed as a seed.
        // Skip to the next seed.
        if (cluster_legalizer.is_mol_clustered(seed_molecule_id)) {
            continue;
        }
        return seed_molecule_id;
    }

    // If the previous loop does not return a molecule, it implies that all
    // molecule have been clustered or have already been proposed as a seed.
    // Return nullptr to signify that there are no further seeds.
    return PackMoleculeId::INVALID();
}
