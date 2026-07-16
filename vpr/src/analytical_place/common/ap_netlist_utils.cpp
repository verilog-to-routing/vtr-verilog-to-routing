/**
 * @file
 * @author  Kevin Xing
 * @date    July 2026
 * @brief   Definition of methods in the utility class AtomBlockAPBlockLookup.
 */

#include "ap_netlist_utils.h"
#include "atom_netlist.h"
#include "ap_netlist.h"

AtomBlockAPBlockLookup::AtomBlockAPBlockLookup(const AtomNetlist& atom_netlist, const APNetlist& ap_netlist, const Prepacker& prepacker) {
    atom_block_ap_block_.resize(atom_netlist.blocks().size());
    for (APBlockId ap_block_id : ap_netlist.blocks()) {
        for (PackMoleculeId block_mol_id : ap_netlist.block_molecules(ap_block_id)) {
            const t_pack_molecule& block_mol = prepacker.get_molecule(block_mol_id);
            for (AtomBlockId atom_block_id : block_mol.atom_block_ids) {
                // See issue #2791, some of the atom_block_ids may be invalid. They
                // can safely be ignored.
                if (!atom_block_id.is_valid())
                    continue;
                // Safety check.
                VTR_ASSERT_SAFE(static_cast<std::size_t>(atom_block_id) < atom_block_ap_block_.size());
                // Ensure that this block is not in any other AP block. That would be weird.
                VTR_ASSERT_SAFE(!atom_block_ap_block_[atom_block_id].is_valid());
                atom_block_ap_block_[atom_block_id] = ap_block_id;
            }
        }
    }
}

unsigned AtomBlockAPBlockLookup::verify(const APNetlist& ap_netlist, const Prepacker& prepacker) {
    unsigned num_errors = 0;

    for (const auto& [atom_block_id, ap_block_id] : atom_block_ap_block_.pairs()) {
        // An invalid AP block id should not have been inserted during construction.
        if (!ap_block_id.is_valid()) {
            VTR_LOG_ERROR("Atom block id %zu is mapped to an invalid AP block id.\n", size_t(atom_block_id));
            num_errors++;
            continue;
        }

        bool atom_block_id_found = false;
        // Loop through all molecules in the AP block and look for a matching atom block id.
        for (PackMoleculeId block_mol_id : ap_netlist.block_molecules(ap_block_id)) {
            const t_pack_molecule& block_mol = prepacker.get_molecule(block_mol_id);
            for (AtomBlockId curr_atom_block_id : block_mol.atom_block_ids) {
                if (atom_block_id == curr_atom_block_id) {
                    atom_block_id_found = true;
                    break;
                }
            }
            // Early exit.
            if (atom_block_id_found)
                break;
        }

        if (!atom_block_id_found) {
            VTR_LOG_ERROR("Atom block id %zu is not found in AP block id %zu.\n", size_t(atom_block_id), size_t(ap_block_id));
            num_errors++;
        }
    }

    return num_errors;
}

APBlockId AtomBlockAPBlockLookup::get_ap_block(const AtomBlockId atom_block_id) const {
    // Safety check.
    VTR_ASSERT_SAFE(atom_block_id.is_valid());
    VTR_ASSERT_SAFE(static_cast<std::size_t>(atom_block_id) < atom_block_ap_block_.size());

    return atom_block_ap_block_[atom_block_id];
}
