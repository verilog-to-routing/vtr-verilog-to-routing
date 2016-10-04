#include "vtr_assert.h"

#include "atom_map.h"

const t_pb* AtomMap::atom_pb(const AtomBlockId blk_id) const {
    auto iter = atom_to_pb_.find(blk_id);
    if(iter == atom_to_pb_.end()) {
        //Not found
        return nullptr;
    }
    return iter->second;
}

AtomBlockId AtomMap::pb_atom(const t_pb* pb_val) const {
    auto iter = pb_to_atom_.find(pb_val);
    if(iter == pb_to_atom_.end()) {
        //Not found
        return AtomBlockId::INVALID();
    }
    return iter->second;
}

int AtomMap::atom_clb(const AtomBlockId blk_id) const {
    auto iter = atom_to_clb_.find(blk_id);
    if(iter == atom_to_clb_.end()) {
        //Not found
        return NO_CLUSTER;
    }
    return iter->second;
}

AtomBlockId AtomMap::clb_atom(const int clb_index) const {
    auto iter = clb_to_atom_.find(clb_index);
    if(iter == clb_to_atom_.end()) {
        //Not found
        return AtomBlockId::INVALID();
    }
    return iter->second;
}

void AtomMap::set_atom_pb(const AtomBlockId blk_id, const t_pb* pb_val) {
    atom_to_pb_[blk_id] = pb_val;
    pb_to_atom_[pb_val] = blk_id;
}

void AtomMap::set_atom_clb(const AtomBlockId blk_id, const int clb_index) {
    atom_to_clb_[blk_id] = clb_index;
    clb_to_atom_[clb_index] = blk_id;
}
