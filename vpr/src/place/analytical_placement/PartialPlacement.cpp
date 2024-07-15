
#include "PartialPlacement.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <unordered_set>
#include <vector>
#include "globals.h"
#include "vpr_constraints_uxsdcxx.h"
#include "vpr_context.h"
#include "vpr_types.h"
#include "vtr_assert.h"

PartialPlacement::PartialPlacement(const AtomNetlist& netlist,                                                                                                                                                            
                                   const std::set<AtomBlockId>& fixed_blocks,                                                                                                                                          
                                   std::map<AtomBlockId, double>& fixed_blocks_x,                                                                                                                                             
                                   std::map<AtomBlockId, double>& fixed_blocks_y) : atom_netlist(netlist) {

    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    std::vector<std::unordered_set<t_pack_molecule*>> interesting_nets;
    interesting_nets.reserve(atom_netlist.nets().size());

    // Collect the unique moveable and fixed molecules from the netlist.
    std::unordered_set<t_pack_molecule*> moveable_mols;
    std::unordered_set<t_pack_molecule*> fixed_mols;
    for (const AtomNetId& net_id : atom_netlist.nets()) {
        if (net_is_ignored_for_placement(net_id))
            continue;
        std::unordered_set<t_pack_molecule*> net_mols;
        for (const AtomPinId& pin_id : atom_netlist.net_pins(net_id)) {
            AtomBlockId blk_id = atom_netlist.pin_block(pin_id);
            // Get the molecule for this block.
            t_pack_molecule* mol = get_mol_from_blk(blk_id, atom_ctx.atom_molecules);
            // Insert the mol into the appropriate set.
            if (fixed_blocks.find(blk_id) != fixed_blocks.end())
                fixed_mols.insert(mol);
            else
                moveable_mols.insert(mol);
            net_mols.insert(mol);
        }
        interesting_nets.push_back(std::move(net_mols));
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

    // Create the AP Netlist
    // FIXME: We should experiment with having duplicate pins! It is possible
    //        that a block would have multiple pin inputs connected to the same
    //        net. This should give that block more power...
    for (const std::unordered_set<t_pack_molecule*> &mols : interesting_nets) {
        // If the number of molecules in a net is 1 or less, we do not care.
        // This can happen when a LUT + FF are packed together and now the
        // net connects to itself.
        if (mols.size() <= 1)
            continue;
        // If the molecules connected by a net are all fixed, then we do not care
        // about this net.
        bool is_all_fixed = true;
        for (t_pack_molecule *mol : mols) {
            if (moveable_mols.find(mol) != moveable_mols.end()) {
                is_all_fixed = false;
                break;
            }
        }
        if (is_all_fixed)
            continue;
        // Insert these nodes into the AP Netlist.
        std::vector<size_t> net_nodes;
        net_nodes.reserve(mols.size());
        for (t_pack_molecule *mol : mols) {
            net_nodes.push_back(mol_to_node_id[mol]);
        }
        ap_netlist.emplace_back(std::move(net_nodes));
    }
}

double PartialPlacement::get_HPWL() {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    double hpwl = 0.0;
    for (AtomNetId net_id : atom_netlist.nets()) {
        // FIXME: Confirm if this should be here.
        if (net_is_ignored_for_placement(atom_netlist, net_id))
            continue;
        // FIXME: This is not necessarily correct for solving HPWL. The x and y
        //        positions should be clamped to the grid coordinates.
        //        However, this may make it harder to debug since we lose the
        //        granualirty of the HPWL. Perhaps create an option to get a
        //        clamped / unclamped version.
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
            if (max_x < min_x || max_y < min_y) {
                VTR_LOG("maxx:%f, minx:%f, maxy: %f, miny: %f, x: %f, y: %f\n", max_x, min_x, max_y, min_y, node_loc_x[node_id], node_loc_y[node_id]);
            }
            VTR_ASSERT(max_x >= min_x);
            VTR_ASSERT(max_y >= min_y);
        }
        hpwl += max_x - min_x + max_y - min_y;
    }
    return hpwl;
}

// This function checks whether all nodes of this placement reside within the board
bool PartialPlacement::is_valid_partial_placement(){
    if (node_loc_x.size() != node_loc_y.size()) 
        return false; 
    bool result = true;
    for (size_t node_id = 0; node_id < node_loc_x.size(); node_id++){
        result &= is_valid_node(node_id);
    } 
    return result; 
}

// Checks whether a node is well defined (not NaN) within the board 
bool PartialPlacement::is_valid_node(size_t node_id){
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    size_t grid_width = device_ctx.grid.width();
    size_t grid_height = device_ctx.grid.height();
    bool result = true;
    if (std::isnan(node_loc_x[node_id])) {
        result = false;
        VTR_LOG("node_id %zu's x value is NaN\n", node_id);
    }
    if (std::isnan(node_loc_y[node_id])) {
        result = false;
        VTR_LOG("node_id %zu's y value is NaN\n", node_id);
    }
    if (node_loc_x[node_id] >= grid_width) {
        result = false;
        VTR_LOG("Too Large! node_id %zu's x value is %f, width is %zu\n", node_id, node_loc_x[node_id], grid_width);
    }else if(node_loc_x[node_id] < 0) {
        result = false;
        VTR_LOG("Too Small! node_id %zu's x value is %f\n", node_id, node_loc_x[node_id]);
    }
    if (node_loc_y[node_id] >= grid_height) {
        result = false;
        VTR_LOG("Too Large! node_id %zu's y value is %f, height is %zu\n", node_id, node_loc_y[node_id], grid_width);
    }else if(node_loc_x[node_id] < 0) {
        result = false;
        VTR_LOG("Too Small! node_id %zu's y value is %f\n", node_id, node_loc_y[node_id]);
    }
    return result;
}

// This function print the the board with the current placement to stdout
// It does not have infinit resolutions. Nodes too close to each other will be represented by the same charactor
// Concentrated nodes are represented by brighter colors, listed in the switch statment
void PartialPlacement::unicode_art(){
    VTR_LOG("unicode_art start\n");
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    size_t device_width = device_ctx.grid.width();
    size_t device_height = device_ctx.grid.height();
    size_t board_width = device_width * 5;
    size_t board_height = device_height * 5;
    std::vector<std::vector<int>> board(board_height, std::vector<int>(board_width, 0));
    for (size_t node_id = 0; node_id < num_nodes; node_id++) {
        unsigned node_x_relative = node_loc_x[node_id]/device_width*board_width;
        unsigned node_y_relative = node_loc_y[node_id]/device_height*board_height;
        if(node_id < num_moveable_nodes){
            board[node_x_relative][node_y_relative]++;
        }else{
            board[node_x_relative][node_y_relative] = -1;
        }
    }
    int max = 0;
    for(unsigned y_board_id = 0; y_board_id < board_height; y_board_id++)
        for(unsigned x_board_id = 0; x_board_id < board_width; x_board_id++)
            if (board[x_board_id][y_board_id] > max)
                max = board[x_board_id][y_board_id];
    for(unsigned y_board_id = 0; y_board_id < board_height; y_board_id++){
        for(unsigned x_board_id = 0; x_board_id < board_width; x_board_id++){
            int density = board[y_board_id][x_board_id];
            int density_range = std::floor((double)density/(double)max*6);
            // if (density >=0 && density <=9){
            //     char digit[8] = "0️⃣";
            //     digit[0] += density;
            //     VTR_LOG(digit);
            // }else if(density >= 10){
            //     VTR_LOG("🔟");
            // }else if(density == -1){
            //     VTR_LOG("*️⃣");
            // }else{
            //     VTR_ASSERT(0 && "Unexpected range!");
            // }
            switch (density_range) {
                default:
                VTR_ASSERT(0 && "unexpected range!");
                break;
                case -1:
                VTR_LOG("⬜");
                break;
                case 0:
                VTR_LOG("⬛");
                break;
                case 1:
                VTR_LOG("🟫");
                break;
                case 2:
                VTR_LOG("🟩");
                break;
                case 3:
                VTR_LOG("🟨");
                break;
                case 4:
                VTR_LOG("🟧");
                break;
                case 5:
                VTR_LOG("🟥");
                break;
                case 6:
                VTR_LOG("🟪");
                break;
                
            }
        }
        VTR_LOG("\n");
    }
    VTR_LOG("unicode_art end\n");
    fflush(stderr);
}
