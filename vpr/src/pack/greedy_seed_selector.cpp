/**
 * @file
 * @author  Alex Singer
 * @date    November 2024
 * @brief   The definitions of the Greedy Seed Selector class.
 */

#include "greedy_seed_selector.h"

#include <algorithm>
#include "atom_netlist.h"
#include "cluster_legalizer.h"
#include "cluster_util.h"
#include "echo_files.h"
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
        case e_cluster_seed::MAX_INPUTS:
        {
            const t_pack_molecule* blk_mol = prepacker.get_atom_molecule(blk_id);
            const t_molecule_stats molecule_stats = calc_molecule_stats(blk_mol, atom_netlist);
            return molecule_stats.num_used_ext_inputs;
        }
        // By blended gain (criticality and inputs used).
        case e_cluster_seed::BLEND:
        {
            // Score seed gain of each block as a weighted sum of timing
            // criticality, number of tightly coupled blocks connected to
            // it, and number of external inputs.
            float seed_blend_fac = 0.5f;

            const t_pack_molecule* blk_mol = prepacker.get_atom_molecule(blk_id);
            const t_molecule_stats molecule_stats = calc_molecule_stats(blk_mol, atom_netlist);
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
        case e_cluster_seed::MAX_PINS:
        {
            const t_pack_molecule* blk_mol = prepacker.get_atom_molecule(blk_id);
            const t_molecule_stats molecule_stats = calc_molecule_stats(blk_mol, atom_netlist);
            return molecule_stats.num_pins;
        }
        // By input pins per molecule (i.e. available pins on primitives, not pins in use).
        // Intuition (a weak one): primitive types with more input pins might be
        //                         harder to pack.
        case e_cluster_seed::MAX_INPUT_PINS:
        {
            const t_pack_molecule* blk_mol = prepacker.get_atom_molecule(blk_id);
            const t_molecule_stats molecule_stats = calc_molecule_stats(blk_mol, atom_netlist);
            return molecule_stats.num_input_pins;
        }
        case e_cluster_seed::BLEND2:
        {
            const t_pack_molecule* mol = prepacker.get_atom_molecule(blk_id);
            const t_molecule_stats molecule_stats = calc_molecule_stats(mol, atom_netlist);

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
                                    const std::vector<AtomBlockId>& seed_atoms,
                                    const vtr::vector<AtomBlockId, float>& atom_gain,
                                    const vtr::vector<AtomBlockId, float>& atom_criticality,
                                    const AtomNetlist& atom_netlist) {
    FILE* fp = vtr::fopen(fname, "w");

    // For pretty formatting determine the maximum name length
    int max_name_len = strlen("atom_block_name");
    int max_type_len = strlen("atom_block_type");
    for (auto blk_id : atom_netlist.blocks()) {
        max_name_len = std::max(max_name_len, (int)atom_netlist.block_name(blk_id).size());

        const t_model* model = atom_netlist.block_model(blk_id);
        max_type_len = std::max(max_type_len, (int)strlen(model->name));
    }

    fprintf(fp, "%-*s %-*s %8s %8s\n", max_name_len, "atom_block_name", max_type_len, "atom_block_type", "gain", "criticality");
    fprintf(fp, "\n");
    for (auto blk_id : seed_atoms) {
        std::string name = atom_netlist.block_name(blk_id);
        fprintf(fp, "%-*s ", max_name_len, name.c_str());

        const t_model* model = atom_netlist.block_model(blk_id);
        fprintf(fp, "%-*s ", max_type_len, model->name);

        fprintf(fp, "%*f ", std::max((int)strlen("gain"), 8), atom_gain[blk_id]);
        fprintf(fp, "%*f ", std::max((int)strlen("criticality"), 8), atom_criticality[blk_id]);
        fprintf(fp, "\n");
    }

    fclose(fp);
}

GreedySeedSelector::GreedySeedSelector(const AtomNetlist& atom_netlist,
                                       const Prepacker& prepacker,
                                       const e_cluster_seed seed_type,
                                       const t_molecule_stats& max_molecule_stats,
                                       const vtr::vector<AtomBlockId, float>& atom_criticality)
                : seed_atoms_(atom_netlist.blocks().begin(), atom_netlist.blocks().end()) {
    // Seed atoms list is initialized with all atoms in the atom netlist.

    // Maintain a lookup table of the seed gain for each atom. This will be
    // used to sort the seed atoms.
    // Initially all gains are zero.
    vtr::vector<AtomBlockId, float> atom_gains(atom_netlist.blocks().size(), 0.f);

    // Get the seed gain of each atom.
    for (AtomBlockId blk_id : atom_netlist.blocks()) {
        atom_gains[blk_id] = get_seed_gain(blk_id,
                                           atom_netlist,
                                           prepacker,
                                           seed_type,
                                           max_molecule_stats,
                                           atom_criticality);
    }

    // Sort seeds in descending order of seed gain (i.e. highest seed gain first)
    //
    // Note that we use a *stable* sort here. It has been observed that different
    // standard library implementations (e.g. gcc-4.9 vs gcc-5) use sorting algorithms
    // which produce different orderings for seeds of equal gain (which is allowed with
    // std::sort which does not specify how equal values are handled). Using a stable
    // sort ensures that regardless of the underlying sorting algorithm the same seed
    // order is produced regardless of compiler.
    auto by_descending_gain = [&](const AtomBlockId lhs, const AtomBlockId rhs) {
        return atom_gains[lhs] > atom_gains[rhs];
    };
    std::stable_sort(seed_atoms_.begin(), seed_atoms_.end(), by_descending_gain);

    // Print the seed gains if requested.
    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_CLUSTERING_BLOCK_CRITICALITIES)) {
        print_seed_gains(getEchoFileName(E_ECHO_CLUSTERING_BLOCK_CRITICALITIES),
                         seed_atoms_, atom_gains, atom_criticality, atom_netlist);
    }

    // Set the starting seed index (the index of the first molecule to propose).
    // The index of the first seed to propose is the first molecule in the
    // seed atoms vector (i.e. the one with the highest seed gain).
    seed_index_ = 0;
}

t_pack_molecule* GreedySeedSelector::get_next_seed(const Prepacker& prepacker,
                                                   const ClusterLegalizer& cluster_legalizer) {
    while (seed_index_ < seed_atoms_.size()) {
        // Get the current seed atom at the seed index and increment the
        // seed index.
        // All previous seed indices have been either proposed already or
        // are already clustered. This process assumes that once an atom
        // is clustered it will never become unclustered.
        AtomBlockId seed_blk_id = seed_atoms_[seed_index_++];

        // If this atom has been clustered, it cannot be proposed as a seed.
        // Skip to the next seed.
        if (cluster_legalizer.is_atom_clustered(seed_blk_id))
            continue;

        // Get the molecule that contains this atom and return it as the
        // next seed.
        t_pack_molecule* seed_molecule = prepacker.get_atom_molecule(seed_blk_id);
        VTR_ASSERT(!cluster_legalizer.is_mol_clustered(seed_molecule));
        return seed_molecule;
    }

    // If the previous loop does not return a molecule, it implies that all
    // atoms have been clustered or have already been proposed as a seed.
    // Return nullptr to signify that there are no further seeds.
    return nullptr;
}

