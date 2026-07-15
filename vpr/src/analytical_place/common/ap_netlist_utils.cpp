#include "ap_netlist_utils.h"

#include "atom_netlist.h"
#include "ap_netlist.h"
#include <iostream>


AtomBlockAPBlockLookup::AtomBlockAPBlockLookup(const AtomNetlist& atom_netlist, const Prepacker& prepacker, const APNetlist& ap_netlist) {
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
void AtomBlockAPBlockLookup::verify(const Prepacker& prepacker, const APNetlist& ap_netlist) {
    for (const auto& [atom_block_id, ap_block_id] : atom_block_ap_block_.pairs()) {
        VTR_ASSERT(ap_block_id.is_valid());

        bool atom_block_id_found = false;
        for (PackMoleculeId block_mol_id : ap_netlist.block_molecules(ap_block_id)) {
            const t_pack_molecule& block_mol = prepacker.get_molecule(block_mol_id);
            for (AtomBlockId curr_atom_block_id : block_mol.atom_block_ids) {
                if (atom_block_id == curr_atom_block_id) {
                    //std::cout << "hohoho" << std::endl;
                    atom_block_id_found = true;
                    break;
                }
            }
            if (atom_block_id_found)
                break;
        }
        VTR_ASSERT(atom_block_id_found);
    }

    verified_ = true;
}

APBlockId AtomBlockAPBlockLookup::get_ap_block(const AtomBlockId atom_block_id) const {
    // We do not want people to use this getter without having verified the lookup.
    VTR_ASSERT_MSG(verified_, "Must verify the lookup using verify() first!");

    // Safety check.
    VTR_ASSERT_SAFE(atom_block_id.is_valid());
    VTR_ASSERT_SAFE(static_cast<std::size_t>(atom_block_id) < atom_block_ap_block_.size());

    return atom_block_ap_block_[atom_block_id];
}

