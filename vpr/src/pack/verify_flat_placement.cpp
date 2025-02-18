
#include "verify_flat_placement.h"
#include "FlatPlacementInfo.h"
#include "atom_netlist.h"
#include "atom_netlist_fwd.h"
#include "prepack.h"
#include "vpr_types.h"
#include "vtr_log.h"

unsigned verify_flat_placement_for_packing(const FlatPlacementInfo& flat_placement_info,
                                           const AtomNetlist& atom_netlist,
                                           const Prepacker& prepacker) {
    unsigned num_errors = 0;

    // Quick check to ensure that the flat placement info has the correct size
    // for each piece of information.
    if (flat_placement_info.blk_x_pos.size() != atom_netlist.blocks().size() ||
        flat_placement_info.blk_y_pos.size() != atom_netlist.blocks().size() ||
        flat_placement_info.blk_layer.size() != atom_netlist.blocks().size() ||
        flat_placement_info.blk_sub_tile.size() != atom_netlist.blocks().size() ||
        flat_placement_info.blk_site_idx.size() != atom_netlist.blocks().size()) {
        VTR_LOG_ERROR(
            "The number of blocks in the flat placement does not match the "
            "number of blocks in the atom netlist.\n");
        num_errors++;
        // Return here since this error can cause issues below.
        return num_errors;
    }

    // 1. Verify that every atom has an (x, y, layer) position on the device.
    //
    // TODO: In the future, we may be able to allow some blocks to have
    //       undefined positions.
    for (AtomBlockId blk_id : atom_netlist.blocks()) {
        if (flat_placement_info.blk_x_pos[blk_id] == FlatPlacementInfo::UNDEFINED_POS ||
            flat_placement_info.blk_y_pos[blk_id] == FlatPlacementInfo::UNDEFINED_POS ||
            flat_placement_info.blk_layer[blk_id] == FlatPlacementInfo::UNDEFINED_POS) {
            VTR_LOG_ERROR(
                "Atom block %s has an undefined position in the flat placement.\n",
                atom_netlist.block_name(blk_id).c_str());
            num_errors++;
        }
    }

    // 2. Verify that every atom block has non-negative position values.
    //
    // Since the device may not be sized yet, we cannot check if the positions
    // are within the bounds of the device, but if any position value is
    // negative (and is not undefined) we know that it is invalid.
    for (AtomBlockId blk_id : atom_netlist.blocks()) {
        float blk_x_pos = flat_placement_info.blk_x_pos[blk_id];
        float blk_y_pos = flat_placement_info.blk_y_pos[blk_id];
        float blk_layer = flat_placement_info.blk_layer[blk_id];
        int blk_sub_tile = flat_placement_info.blk_sub_tile[blk_id];
        int blk_site_idx = flat_placement_info.blk_site_idx[blk_id];
        if ((blk_x_pos < 0.f && blk_x_pos != FlatPlacementInfo::UNDEFINED_POS) ||
            (blk_y_pos < 0.f && blk_y_pos != FlatPlacementInfo::UNDEFINED_POS) ||
            (blk_layer < 0.f && blk_layer != FlatPlacementInfo::UNDEFINED_POS) ||
            (blk_sub_tile < 0 && blk_sub_tile != FlatPlacementInfo::UNDEFINED_SUB_TILE) ||
            (blk_site_idx < 0 && blk_site_idx != FlatPlacementInfo::UNDEFINED_SITE_IDX)) {
            VTR_LOG_ERROR(
                "Atom block %s is placed at an invalid position on the FPGA.\n",
                atom_netlist.block_name(blk_id).c_str());
            num_errors++;
        }
    }

    // 3. Verify that every atom in each molecule has the same position.
    //
    // TODO: In the future, we can support if some of the atoms are undefined,
    //       but that can be fixed-up before calling this method.
    for (PackMoleculeId mol_id : prepacker.molecules()) {
        const t_pack_molecule& mol = prepacker.get_molecule(mol_id);
        AtomBlockId root_blk_id = mol.atom_block_ids[mol.root];
        float root_pos_x = flat_placement_info.blk_x_pos[root_blk_id];
        float root_pos_y = flat_placement_info.blk_y_pos[root_blk_id];
        float root_layer = flat_placement_info.blk_layer[root_blk_id];
        int root_sub_tile = flat_placement_info.blk_sub_tile[root_blk_id];
        for (AtomBlockId mol_blk_id : mol.atom_block_ids) {
            if (!mol_blk_id.is_valid())
                continue;
            if (flat_placement_info.blk_x_pos[mol_blk_id] != root_pos_x ||
                flat_placement_info.blk_y_pos[mol_blk_id] != root_pos_y ||
                flat_placement_info.blk_layer[mol_blk_id] != root_layer ||
                flat_placement_info.blk_sub_tile[mol_blk_id] != root_sub_tile) {
                VTR_LOG_ERROR(
                    "Molecule with root atom block %s contains atom block %s "
                    "which is not at the same position as the root atom "
                    "block.\n",
                    atom_netlist.block_name(root_blk_id).c_str(),
                    atom_netlist.block_name(mol_blk_id).c_str());
                num_errors++;
            }
        }
    }

    // TODO: May want to verify that the layer is all 0 in the case of 2D FPGAs.

    // TODO: Should verify that the fixed block constraints are observed.
    //       It is ill-formed for a flat placement to disagree with the constraints.

    return num_errors;
}

