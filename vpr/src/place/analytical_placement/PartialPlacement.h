
#pragma once

#include <vector>
#include "atom_netlist.h"
#include "vpr_types.h"
#include "vtr_log.h"

// The goal of this class is to contain all of the information about the blocks
// that gets passed around during analytical placement.

// FIXME: Should also store the graph in node space (nets and such). Iterating
//        over the atom netlist is not very stable (nor efficient).
//        This may require making our own Netlist.
//          - This netlist should probably be a separate class.

class PartialPlacement {
public:
    PartialPlacement(const AtomNetlist& netlist,
                     const std::set<AtomBlockId>& fixed_blocks,
                     std::map<AtomBlockId, double>& fixed_blocks_x,
                     std::map<AtomBlockId, double>& fixed_blocks_y);

    static inline t_pack_molecule* get_mol_from_blk(
            AtomBlockId blk_id,
            const std::multimap<AtomBlockId, t_pack_molecule*> &atom_molecules) {
        VTR_ASSERT(atom_molecules.count(blk_id) == 1 && "Only expect one molecule for a given block.");
        auto mol_range = atom_molecules.equal_range(blk_id);
        return mol_range.first->second;
    }

    inline size_t get_node_id_from_blk(AtomBlockId blk_id, const std::multimap<AtomBlockId, t_pack_molecule*> &atom_molecules) {
        t_pack_molecule* mol = get_mol_from_blk(blk_id, atom_molecules);
        return mol_to_node_id[mol];
    }

    inline bool is_moveable_node(size_t node_id) const {
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

    bool is_valid_partial_placement();

    bool is_valid_node(size_t node_id);

    void unicode_art();

    // Export the PartialPlacement to a file following the VIPER Flat Placement
    // File Format: https://doi.org/10.1145/3665283.3665300
    bool export_to_flat_placement_file(std::string file_name);

    inline void print_stats() {
        VTR_LOG("Number of moveable nodes: %zu\n", num_moveable_nodes);
        VTR_LOG("Number of fixed nodes: %zu\n", num_nodes - num_moveable_nodes);
        VTR_LOG("Number of total nodes: %zu\n", num_nodes);
        VTR_LOG("Number of AP nets: %zu\n", ap_netlist.size());
    }

    const AtomNetlist& atom_netlist;
    // Analytical Placement-specific Netlist
    // This is a netlist containing the nets that AP cares about, using the node
    // type that AP uses. It also removes duplicate nodes from the net. This
    // will not include all nets.
    // Nets which are ignored:
    //  - nets "ignored for placement", see net_is_ignored_for_placement
    //  - nets that only connect to 1 (or less) nodes
    //  - nets that do not contain any moveable nodes
    // TODO: Eventually we should use the actual Netlist class to contain this
    // information. vector of vectors is inefficient and may be dangerous.
    std::vector<std::vector<size_t>> ap_netlist;
    std::map<t_pack_molecule*, size_t> mol_to_node_id;
    std::vector<t_pack_molecule*> node_id_to_mol;
    std::vector<double> node_loc_x;
    std::vector<double> node_loc_y;
    size_t num_nodes = 0;
    size_t num_moveable_nodes = 0;
};

