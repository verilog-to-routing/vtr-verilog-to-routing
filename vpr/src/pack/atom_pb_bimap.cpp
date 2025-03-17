/**
 * @file 
 * @author  Amir Poolad
 * @date    March 2025
 * @brief   The code for the AtomPBBimap class.
 * 
 * This file implements the various functions of the AtomPBBimap class.
 */

#include "atom_pb_bimap.h"
#include "atom_netlist.h"

AtomPBBimap::AtomPBBimap(const vtr::bimap<AtomBlockId, const t_pb*, vtr::linear_map, std::unordered_map>& atom_to_pb) {
    atom_to_pb_ = atom_to_pb;
}

const t_pb* AtomPBBimap::atom_pb(const AtomBlockId blk_id) const {
    auto iter = atom_to_pb_.find(blk_id);
    if (iter == atom_to_pb_.end()) {
        //Not found
        return nullptr;
    }
    return iter->second;
}

AtomBlockId AtomPBBimap::pb_atom(const t_pb* pb) const {
    auto iter = atom_to_pb_.find(pb);
    if (iter == atom_to_pb_.inverse_end()) {
        //Not found
        return AtomBlockId::INVALID();
    }
    return iter->second;
}

const t_pb_graph_node* AtomPBBimap::atom_pb_graph_node(const AtomBlockId blk_id) const {
    const t_pb* pb = atom_pb(blk_id);
    if (pb) {
        //Found
        return pb->pb_graph_node;
    }
    return nullptr;
}

void AtomPBBimap::set_atom_pb(const AtomBlockId blk_id, const t_pb* pb) {
    //If either of blk_id or pb are not valid,
    //remove any mapping
    if (!blk_id && pb) {
        //Remove
        atom_to_pb_.erase(pb);
    } else if (blk_id && !pb) {
        //Remove
        atom_to_pb_.erase(blk_id);
    } else if (blk_id && pb) {
        //If both are valid store the mapping
        atom_to_pb_.update(blk_id, pb);
    }
}

void AtomPBBimap::reset_bimap(const AtomNetlist &netlist) {
    for (auto blk : netlist.blocks()) {
        set_atom_pb(blk, nullptr);
    }
}
