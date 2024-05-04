
#include "PartialPlacement.h"
#include <limits>
#include <map>
#include <unordered_set>
#include <vector>
#include "globals.h"
#include "vpr_context.h"
#include "vpr_types.h"

PartialPlacement::PartialPlacement(const AtomNetlist& netlist,                                                                                                                                                            
                                   const std::set<AtomBlockId>& fixed_blocks,                                                                                                                                          
                                   std::map<AtomBlockId, double>& fixed_blocks_x,                                                                                                                                             
                                   std::map<AtomBlockId, double>& fixed_blocks_y) : atom_netlist(netlist) {

    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    // Collect the unique moveable and fixed molecules from the netlist.
    std::unordered_set<t_pack_molecule*> moveable_mols;
    std::unordered_set<t_pack_molecule*> fixed_mols;
    for (const AtomNetId& net_id : atom_netlist.nets()) {
        if (net_is_ignored_for_placement(net_id))
            continue;
        for (const AtomPinId& pin_id : atom_netlist.net_pins(net_id)) {
            AtomBlockId blk_id = atom_netlist.pin_block(pin_id);
            // Get the molecule for this block.
            t_pack_molecule* mol = get_mol_from_blk(blk_id, atom_ctx.atom_molecules);
            // Insert the mol into the appropriate set.
            if (fixed_blocks.find(blk_id) != fixed_blocks.end())
                fixed_mols.insert(mol);
            else
                moveable_mols.insert(mol);
        }
    }

    // Ensure that no fixed molecules are moveable (safety check)
    // Not sure if this is possible, but if it is, we can just remove the fixed
    // molecules from the moveable ones.
    for (t_pack_molecule* mol : fixed_mols) {
        VTR_ASSERT(moveable_mols.find(mol) == moveable_mols.end() && "A fixed molecule cannot also be moveable.");
    }

    // The number of nodes is the number of unique moveable and fixed molecules.
    num_nodes = moveable_mols.size() + fixed_mols.size();
    num_moveable_nodes = moveable_mols.size();

    // Initialize node_id to/from molecule data structures.
    node_id_to_mol.resize(num_nodes);
    size_t curr_node_id = 0;
    for (t_pack_molecule* moveable_mol : moveable_mols) {
        node_id_to_mol[curr_node_id] = moveable_mol;
        mol_to_node_id[moveable_mol] = curr_node_id;
        curr_node_id++;
    }
    for (t_pack_molecule* fixed_mol : fixed_mols) {
        node_id_to_mol[curr_node_id] = fixed_mol;
        mol_to_node_id[fixed_mol] = curr_node_id;
        curr_node_id++;
    }

    // Set the fixed molecule positions.
    node_loc_x.resize(num_nodes);
    node_loc_y.resize(num_nodes);
    for (const AtomBlockId& fixed_blk_id : fixed_blocks) {
        size_t fixed_node_id = get_node_id_from_blk(fixed_blk_id, atom_ctx.atom_molecules);
        VTR_ASSERT(fixed_blocks_x.find(fixed_blk_id) != fixed_blocks_x.end());
        VTR_ASSERT(fixed_blocks_y.find(fixed_blk_id) != fixed_blocks_y.end());
        node_loc_x[fixed_node_id] = fixed_blocks_x[fixed_blk_id];
        node_loc_y[fixed_node_id] = fixed_blocks_y[fixed_blk_id];
    }

}

double PartialPlacement::get_HPWL() {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    double hpwl = 0.0;
    for (AtomNetId net_id : atom_netlist.nets()) {
        // FIXME: Confirm if this should be here.
        if (net_is_ignored_for_placement(atom_netlist, net_id))
            continue;
        double min_x = std::numeric_limits<double>::max();
        double max_x = std::numeric_limits<double>::lowest();
        double min_y = std::numeric_limits<double>::max();
        double max_y = std::numeric_limits<double>::lowest();
        for (AtomPinId pin_id : atom_netlist.net_pins(net_id)) {
            AtomBlockId blk_id = atom_netlist.pin_block(pin_id);
            size_t node_id = get_node_id_from_blk(blk_id, atom_ctx.atom_molecules);
            if (node_loc_x[node_id] > max_x)
                max_x = node_loc_x[node_id];
            if (node_loc_x[node_id] < min_x)
                min_x = node_loc_x[node_id];
            if (node_loc_y[node_id] > max_y)
                max_y = node_loc_y[node_id];
            if (node_loc_y[node_id] < min_y)
                min_y = node_loc_y[node_id];
        }
        VTR_ASSERT(max_x >= min_x);
        VTR_ASSERT(max_y >= min_y);
        hpwl += max_x - min_x + max_y - min_y;
    }
    return hpwl;
}

