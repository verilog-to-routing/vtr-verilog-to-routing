
#include "PartialPlacement.h"
#include <limits>

PartialPlacement::PartialPlacement(const AtomNetlist& netlist,                                                                                                                                                            
                                   const std::set<AtomBlockId>& fixed_blocks,                                                                                                                                          
                                   std::map<AtomBlockId, double>& fixed_blocks_x,                                                                                                                                             
                                   std::map<AtomBlockId, double>& fixed_blocks_y) : atom_netlist(netlist) {

    std::set<AtomBlockId> moveable_blocks;

    for (const AtomNetId& net_id : atom_netlist.nets()) {
        if (net_is_ignored_for_placement(net_id))
            continue;
        for (const AtomPinId& pin_id : atom_netlist.net_pins(net_id)) {
            AtomBlockId blk_id = netlist.pin_block(pin_id);
            if (fixed_blocks.find(blk_id) != fixed_blocks.end())
                continue;
            moveable_blocks.insert(blk_id);
        }
    }

    num_nodes = moveable_blocks.size() + fixed_blocks.size();
    node_id_to_block_id.resize(num_nodes);
    size_t curr_node_id = 0;
    for (const AtomBlockId& moveable_block_id : moveable_blocks) {
        node_id_to_block_id[curr_node_id] = moveable_block_id;
        block_id_to_node_id[moveable_block_id] = curr_node_id;
        curr_node_id++;
    }
    for (const AtomBlockId& fixed_blk_id : fixed_blocks) {
        node_id_to_block_id[curr_node_id] = fixed_blk_id;
        block_id_to_node_id[fixed_blk_id] = curr_node_id;
        curr_node_id++;
    }

    node_loc_x.resize(num_nodes);
    node_loc_y.resize(num_nodes);

    for (const AtomBlockId& fixed_blk_id : fixed_blocks) {
        size_t fixed_node_id = block_id_to_node_id[fixed_blk_id];
        VTR_ASSERT(fixed_blocks_x.find(fixed_blk_id) != fixed_blocks_x.end());
        VTR_ASSERT(fixed_blocks_y.find(fixed_blk_id) != fixed_blocks_y.end());
        node_loc_x[fixed_node_id] = fixed_blocks_x[fixed_blk_id];
        node_loc_y[fixed_node_id] = fixed_blocks_y[fixed_blk_id];
    }

    num_moveable_nodes = moveable_blocks.size();
}

double PartialPlacement::get_HPWL() {
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
            size_t node_id = block_id_to_node_id[blk_id];
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

