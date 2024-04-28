
#pragma once

#include <vector>
#include "atom_netlist.h"
#include "vtr_log.h"

// The goal of this class is to contain all of the information about the blocks
// that gets passed around during analytical placement.

class PartialPlacement {
public:
    PartialPlacement(const AtomNetlist& netlist,
                     const std::set<AtomBlockId>& fixed_blocks,
                     std::map<AtomBlockId, double>& fixed_blocks_x,
                     std::map<AtomBlockId, double>& fixed_blocks_y);

    inline bool is_moveable_node(size_t node_id) {
        return node_id < num_moveable_nodes;
    }

    static inline bool net_is_ignored_for_placement(const AtomNetlist& netlist,
                                                    const AtomNetId& net_id) {
        // Nets that are not routed (like clocks) can be ignored for placement.
        if (netlist.net_is_ignored(net_id))
            return true;
        // TODO: should we need to check for fanout <= 1 nets?
        // TODO: do we need to check if the driver is invalid?
        // TODO: do we need to check if the net has sinks?
        return false;
    }

    inline bool net_is_ignored_for_placement(const AtomNetId& net_id) {
        return net_is_ignored_for_placement(atom_netlist, net_id);
    }

    double get_HPWL();

    inline void print_stats() {
        VTR_LOG("Number of moveable blocks: %zu\n", num_moveable_nodes);
        VTR_LOG("Number of fixed blocks: %zu\n", num_nodes - num_moveable_nodes);
        VTR_LOG("Number of placeable nodes: %zu\n", num_nodes);
    }

    const AtomNetlist& atom_netlist;
    std::map<AtomBlockId, size_t> block_id_to_node_id;
    std::vector<AtomBlockId> node_id_to_block_id;
    std::vector<double> node_loc_x;
    std::vector<double> node_loc_y;
    size_t num_nodes = 0;
    size_t num_moveable_nodes = 0;
};

