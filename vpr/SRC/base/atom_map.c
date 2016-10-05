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

const t_pb_graph_node* AtomMap::atom_pb_graph_node(const AtomBlockId blk_id) const {
    const t_pb* pb = atom_pb(blk_id);
    if(pb) {
        //Found
        return pb->pb_graph_node;
    }
    return nullptr;
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
    //If either of blk_id or pb_val are not valid, 
    //remove any mapping
    if(!blk_id) {
        //Remove
        pb_to_atom_.erase(pb_val);
        VTR_ASSERT_SAFE(atom_to_pb.count(blk_id) == 0);
    }
    if(pb_val == nullptr) {
        //Remove
        atom_to_pb_.erase(blk_id);
        VTR_ASSERT_SAFE(pb_to_atom_.count(pb_val) == 0);
    }
    
    //If both are valid store the mapping
    if(blk_id && pb_val) {
        //Store
        atom_to_pb_[blk_id] = pb_val;
        pb_to_atom_[pb_val] = blk_id;
    }
}

void AtomMap::set_atom_clb(const AtomBlockId blk_id, const int clb_index) {
    VTR_ASSERT(blk_id);
    //If either are invalid remove any mapping
    if(!blk_id) {
        //Remove
        clb_to_atom_.erase(clb_index);
        VTR_ASSERT_SAFE(atom_to_clb_.count(blk_id) == 0);
    }
    if(clb_index == NO_CLUSTER) {
        //Remove
        atom_to_clb_.erase(blk_id);
        VTR_ASSERT_SAFE(clb_to_atom_.count(clb_index) == 0);
    }
    
    //If both are valid store the mapping
    if(blk_id && clb_index != NO_CLUSTER) {
        //Store
        atom_to_clb_[blk_id] = clb_index;
        clb_to_atom_[clb_index] = blk_id;
    }
}
