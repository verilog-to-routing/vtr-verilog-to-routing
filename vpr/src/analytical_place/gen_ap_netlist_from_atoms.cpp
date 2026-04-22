/**
 * @file
 * @author  Alex Singer
 * @date    September 2024
 * @brief   Definition of the gen_ap_netlist_from_atoms method, used for
 *          generating an APNetlist from the results of the Prepacker.
 */

#include "gen_ap_netlist_from_atoms.h"
#include "ap_netlist.h"
#include "atom_netlist.h"
#include "atom_netlist_fwd.h"
#include "logical_ram_infer.h"
#include "netlist_fwd.h"
#include "partition.h"
#include "partition_region.h"
#include "prepack.h"
#include "region.h"
#include "user_place_constraints.h"
#include "vtr_assert.h"
#include "vtr_geometry.h"
#include "vtr_time.h"
#include "vtr_vector.h"
#include <unordered_set>
#include <vector>

/**
 * @brief Returns true of the given partition region covers a single point on
 *        the grid, false otherwise.
 */
static bool is_single_point_pr(const PartitionRegion& pr) {
    // If there are multiple regions in this partition region, we assume that
    // they point to different regions.
    // TODO: Although very strange, it is possible that a user made multiple
    //       regions point to the same point. Should probably be cleaned up in
    //       a pre-processing step for the partition regions.
    if (pr.get_regions().size() != 1)
        return false;
    // Get the region.
    const Region& region = pr.get_regions()[0];

    // Check that the region covers exactly one point in x, y, and layer.
    const vtr::Rect<int>& region_rect = region.get_rect();
    if (region_rect.xmin() != region_rect.xmax())
        return false;
    if (region_rect.ymin() != region_rect.ymax())
        return false;
    if (region.get_layer_range().first != region.get_layer_range().second)
        return false;

    // If all prior passes, this is a single-point partition region.
    return true;
}

APNetlist gen_ap_netlist_from_atoms(const AtomNetlist& atom_netlist,
                                    const Prepacker& prepacker,
                                    const RamMapper& ram_mapper,
                                    const UserPlaceConstraints& constraints,
                                    int high_fanout_threshold) {
    // Create a scoped timer for reading the atom netlist.
    vtr::ScopedStartFinishTimer timer("Read Atom Netlist to AP Netlist");

    // FIXME: What to do about the name and ID in this context? For now just
    //        using empty strings.
    APNetlist ap_netlist;

    // Pre-create one AP block per physical RAM group. Each group's molecules
    // are packed into a single super-block so the global placer treats all
    // atoms in the group as one moveable unit.
    // Build a map from each RAM atom to the AP block that represents its group.
    vtr::vector<AtomBlockId, APBlockId> ram_atom_to_ap_block(atom_netlist.blocks().size());
    for (const PhysicalRamGroup& phys_group : ram_mapper.physical_ram_groups()) {
        VTR_ASSERT(!phys_group.atoms.empty());
        // Name the super-block after the first valid atom in the group.
        const std::string& blk_name = atom_netlist.block_name(phys_group.atoms[0]);
        APBlockId ram_ap_blk_id = ap_netlist.create_block(blk_name, phys_group.molecules);
        for (AtomBlockId atom_id : phys_group.atoms) {
            ram_atom_to_ap_block[atom_id] = ram_ap_blk_id;
        }
    }

    // Add the APBlocks based on the atom block molecules. This essentially
    // creates supernodes.
    // Each AP block has the name of the first atom block in the molecule.
    // Each port is named "<atom_blk_name>_<atom_port_name>"
    // Each net has the exact same name as in the atom netlist
    for (AtomBlockId atom_blk_id : atom_netlist.blocks()) {
        APBlockId ap_blk_id;
        if (ram_atom_to_ap_block[atom_blk_id].is_valid()) {
            // RAM atom: use the pre-created super-block for its physical group.
            ap_blk_id = ram_atom_to_ap_block[atom_blk_id];
        } else {
            // Non-RAM atom: Get the molecule of this block and create the AP
            // block (if not already done)
            PackMoleculeId molecule_id = prepacker.get_atom_molecule(atom_blk_id);
            const t_pack_molecule& mol = prepacker.get_molecule(molecule_id);
            const std::string& first_blk_name = atom_netlist.block_name(mol.atom_block_ids[0]);
            ap_blk_id = ap_netlist.create_block(first_blk_name, {molecule_id});
        }
        // Add the ports and pins of this block to the supernode
        for (AtomPortId atom_port_id : atom_netlist.block_ports(atom_blk_id)) {
            BitIndex port_width = atom_netlist.port_width(atom_port_id);
            PortType port_type = atom_netlist.port_type(atom_port_id);
            const std::string& port_name = atom_netlist.port_name(atom_port_id);
            const std::string& block_name = atom_netlist.block_name(atom_blk_id);
            // The port name needs to be made unique for the supernode (two
            // joined blocks may have the same port name)
            std::string ap_port_name = block_name + "_" + port_name;
            APPortId ap_port_id = ap_netlist.create_port(ap_blk_id, ap_port_name, port_width, port_type);
            for (AtomPinId atom_pin_id : atom_netlist.port_pins(atom_port_id)) {
                BitIndex port_bit = atom_netlist.pin_port_bit(atom_pin_id);
                PinType pin_type = atom_netlist.pin_type(atom_pin_id);
                bool pin_is_const = atom_netlist.pin_is_constant(atom_pin_id);
                AtomNetId pin_atom_net_id = atom_netlist.pin_net(atom_pin_id);
                const std::string& pin_atom_net_name = atom_netlist.net_name(pin_atom_net_id);
                APNetId pin_ap_net_id = ap_netlist.create_net(pin_atom_net_name, pin_atom_net_id);
                ap_netlist.create_pin(ap_port_id, port_bit, pin_ap_net_id, pin_type, atom_pin_id, pin_is_const);
            }
        }
    }

    // Fix the block locations given by the VPR constraints
    for (APBlockId ap_blk_id : ap_netlist.blocks()) {
        for (PackMoleculeId molecule_id : ap_netlist.block_molecules(ap_blk_id)) {
            const t_pack_molecule& mol = prepacker.get_molecule(molecule_id);
            for (AtomBlockId mol_atom_blk_id : mol.atom_block_ids) {
                PartitionId part_id = constraints.get_atom_partition(mol_atom_blk_id);
                if (!part_id.is_valid())
                    continue;
                // We should not fix a block twice. This would imply that a molecule
                // contains two fixed blocks. This would only make sense if the blocks
                // were fixed to the same location. I am not sure if that is even
                // possible.
                VTR_ASSERT(ap_netlist.block_mobility(ap_blk_id) == APBlockMobility::MOVEABLE);
                // Get the partition region.
                const PartitionRegion& partition_pr = constraints.get_partition_pr(part_id);
                if (!is_single_point_pr(partition_pr)) {
                    // The code below currently assumes that the partition region is
                    // a single point.
                    // TODO: AP should understand larger partition regions so it
                    //       can optimize them better. For now we just ignore them
                    //       and let the legalizer deal with them.
                    continue;
                }
                // TODO: Either handle the union of legal locations or turn into a
                //       proper error.
                VTR_ASSERT(partition_pr.get_regions().size() == 1 && "AP: Each partition should contain only one region for AP right now.");
                const Region& region = partition_pr.get_regions()[0];
                // Get the x and y.
                const vtr::Rect<int>& region_rect = region.get_rect();
                VTR_ASSERT(region_rect.xmin() == region_rect.xmax() && "AP: Expect each region to be a single point in x!");
                VTR_ASSERT(region_rect.ymin() == region_rect.ymax() && "AP: Expect each region to be a single point in y!");
                // Here we offset by 0.5 to put the fixed point in the center of the
                // tile (assuming the tile is 1x1).
                // TODO: Think about what to do when the user fixes blocks to large
                //       tiles. However, this solution will at least keep the atoms
                //       away from the edge of tiles.
                float blk_x_loc = region_rect.xmin() + 0.5f;
                float blk_y_loc = region_rect.ymin() + 0.5f;
                // Get the layer.
                VTR_ASSERT(region.get_layer_range().first == region.get_layer_range().second && "AP: Expect each region to be a single point in layer!");
                int blk_layer_num = region.get_layer_range().first;
                // Get the sub_tile (if fixed).
                int blk_sub_tile = APFixedBlockLoc::UNFIXED_DIM;
                if (region.get_sub_tile() != NO_SUBTILE)
                    blk_sub_tile = region.get_sub_tile();
                // Set the fixed block location.
                APFixedBlockLoc loc = {blk_x_loc, blk_y_loc, blk_layer_num, blk_sub_tile};
                ap_netlist.set_block_loc(ap_blk_id, loc);
            }
        }
    }

    // Cleanup the netlist by marking undesirable nets.
    // Currently undesirable nets are nets that are:
    //  - ignored for placement
    //  - a global net
    //  - connected to 1 or fewer unique blocks
    //  - connected to only fixed blocks
    //  - having fanout higher than threshold
    for (APNetId ap_net_id : ap_netlist.nets()) {
        // Is the net ignored for placement, if so mark as ignored for AP.
        const std::string& net_name = ap_netlist.net_name(ap_net_id);
        AtomNetId atom_net_id = atom_netlist.find_net(net_name);
        VTR_ASSERT(atom_net_id.is_valid());
        if (atom_netlist.net_is_ignored(atom_net_id)) {
            ap_netlist.set_net_is_ignored(ap_net_id, true);
            continue;
        }

        // Is the net global, if so mark as global for AP (also ignored)
        if (atom_netlist.net_is_global(atom_net_id)) {
            ap_netlist.set_net_is_global(ap_net_id, true);
            // Global nets are also ignored by the AP flow.
            ap_netlist.set_net_is_ignored(ap_net_id, true);
            continue;
        }

        // Prior to AP, it is likely that the nets in the Atom Netlist have not
        // been annotated with being global or ignored. To get around this, we
        // annotate the AP Netlist speculatively.
        // We label a net as being global if one of its pin connect to a clock
        // port or a non-clock global model port.
        bool is_global = false;
        for (AtomPinId pin_id : atom_netlist.net_pins(atom_net_id)) {
            AtomPortId port_id = atom_netlist.pin_port(pin_id);
            if (atom_netlist.port_type(port_id) == PortType::CLOCK) {
                is_global = true;
                break;
            }
            if (atom_netlist.port_model(port_id)->is_non_clock_global) {
                is_global = true;
                break;
            }
        }
        if (is_global) {
            ap_netlist.set_net_is_global(ap_net_id, true);
            // Global nets are also ignored in the AP flow.
            ap_netlist.set_net_is_ignored(ap_net_id, true);
        }

        // Get the unique blocks connectioned to this net
        std::unordered_set<APBlockId> net_blocks;
        for (APPinId ap_pin_id : ap_netlist.net_pins(ap_net_id)) {
            net_blocks.insert(ap_netlist.pin_block(ap_pin_id));
        }
        // If connected to 1 or fewer unique blocks, mark as ignored for AP.
        if (net_blocks.size() <= 1) {
            ap_netlist.set_net_is_ignored(ap_net_id, true);
            continue;
        }
        // If all the connected blocks are fixed, mark as ignored for AP.
        bool is_all_fixed = true;
        for (APBlockId ap_blk_id : net_blocks) {
            if (ap_netlist.block_mobility(ap_blk_id) == APBlockMobility::MOVEABLE) {
                is_all_fixed = false;
                break;
            }
        }
        if (is_all_fixed) {
            ap_netlist.set_net_is_ignored(ap_net_id, true);
            continue;
        }
        // If fanout number of the net is higher than the threshold, mark as ignored for AP.
        size_t num_pins = ap_netlist.net_pins(ap_net_id).size();
        VTR_ASSERT_DEBUG(num_pins > 1);
        if (num_pins - 1 > static_cast<size_t>(high_fanout_threshold)) {
            ap_netlist.set_net_is_ignored(ap_net_id, true);
            continue;
        }
    }
    ap_netlist.compress();

    // TODO: Should we cleanup the blocks? For example if there is no path
    //       from a fixed block to a given moveable block, then that moveable
    //       block can be removed (since it can literally go anywhere).
    //  - This would be useful to detect and use throughout; but may cause some
    //    issues if we just remove them. When and where will they eventually
    //    be placed?
    //  - Perhaps we can add a flag in the netlist to each of these blocks and
    //    during the solving stage we can ignore them.
    //       For now, leave this alone; but should check if the matrix becomes
    //       ill-formed and causes problems.

    // Verify that the netlist was created correctly.
    VTR_ASSERT(ap_netlist.verify());

    return ap_netlist;
}