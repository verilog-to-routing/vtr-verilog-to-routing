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
                VTR_ASSERT(static_cast<std::size_t>(atom_block_id) < atom_block_ap_block_.size());
                // Ensure that this block is not in any other AP block. That would be weird.
                VTR_ASSERT(!atom_block_ap_block_[atom_block_id].is_valid());
                atom_block_ap_block_[atom_block_id] = ap_block_id;
            }
        }
    }
}

void AtomBlockAPBlockLookup::verify(const APNetlist& ap_netlist, const Prepacker& prepacker) {
    for (const auto& [atom_block_id, ap_block_id] : atom_block_ap_block_.pairs()) {
        // An invalid AP block id should not have been inserted during construction.
        VTR_ASSERT(ap_block_id.is_valid());

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
        // Something unexpected must have happened if we did not find the atom block id.
        VTR_ASSERT(atom_block_id_found);
    }

    verified_ = true;
}

APBlockId AtomBlockAPBlockLookup::get_ap_block(const AtomBlockId atom_block_id) const {
    // We should not use this getter without having verified the lookup.
    VTR_ASSERT_MSG(verified_, "Must verify the lookup using verify() first!");

    // Safety check.
    VTR_ASSERT_SAFE(atom_block_id.is_valid());
    VTR_ASSERT_SAFE(static_cast<std::size_t>(atom_block_id) < atom_block_ap_block_.size());

    return atom_block_ap_block_[atom_block_id];
}

