#include "vtr_assert.h"

#include "atom_map.h"
/*
 * PB
 */
const t_pb* AtomMap::atom_pb(const AtomBlockId blk_id) const {
    auto iter = atom_to_pb_.find(blk_id);
    if(iter == atom_to_pb_.end()) {
        //Not found
        return nullptr;
    }
    return iter->second;
}

AtomBlockId AtomMap::pb_atom(const t_pb* pb) const {
    auto iter = atom_to_pb_.find(pb);
    if(iter == atom_to_pb_.inverse_end()) {
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

void AtomMap::set_atom_pb(const AtomBlockId blk_id, const t_pb* pb) {
    //If either of blk_id or pb are not valid, 
    //remove any mapping
    if(!blk_id && pb) {
        //Remove
        atom_to_pb_.erase(pb);
        VTR_ASSERT_SAFE(atom_to_pb_.count(blk_id) == 0);
    } else if(blk_id && !pb) {
        //Remove
        atom_to_pb_.erase(blk_id);
        VTR_ASSERT_SAFE(pb_to_atom_.count(pb) == 0);
    } else if(blk_id &&  pb) {
        //If both are valid store the mapping
        VTR_ASSERT(blk_id && pb);
        atom_to_pb_.update(blk_id, pb);
    }
}

/*
 * PB Pins
 */
const t_pb_graph_pin* AtomMap::atom_pin_pb_graph_pin(AtomPinId atom_pin) const {
    return atom_to_pb_graph_pin_[atom_pin];
}

void AtomMap::set_atom_pin_pb_graph_pin(AtomPinId atom_pin, const t_pb_graph_pin* gpin) {
    atom_to_pb_graph_pin_.insert(atom_pin, gpin);
}

/*
 * Blocks
 */
int AtomMap::atom_clb(const AtomBlockId blk_id) const {
    if(blk_id && size_t(blk_id) < atom_to_clb_.size()) {
        return atom_to_clb_[size_t(blk_id)];
    } else {
        return NO_CLUSTER;
    }
}

AtomBlockId AtomMap::clb_atom(const int clb_index) const {
    auto iter = clb_to_atom_.find(clb_index);
    if(iter == clb_to_atom_.end()) {
        //Not found
        return AtomBlockId::INVALID();
    }
    return iter->second;
}

void AtomMap::set_atom_clb(const AtomBlockId blk_id, const int clb_index) {

    if(blk_id && size_t(blk_id) >= atom_to_clb_.size()) {
        atom_to_clb_.resize(size_t(blk_id)+1, NO_CLUSTER);
    }

    //If either are invalid remove any mapping
    if(clb_index != NO_CLUSTER && !blk_id) {
        //Remove
        clb_to_atom_.erase(clb_index);
    }
    if(blk_id && clb_index == NO_CLUSTER) {
        //Remove
        atom_to_clb_[size_t(blk_id)] = NO_CLUSTER;
        VTR_ASSERT_SAFE(clb_to_atom_.count(clb_index) == 0);
    }
    
    //If both are valid store the mapping
    if(blk_id && clb_index != NO_CLUSTER) {
        //Store
        atom_to_clb_[size_t(blk_id)] = clb_index;
        clb_to_atom_[clb_index] = blk_id;
    }
}

/*
 * Nets
 */
AtomNetId AtomMap::atom_net(const int clb_net_index) const {
    auto iter = clb_to_atom_net_.find(clb_net_index);
    if(iter == clb_to_atom_net_.end()) {
        //Not found
        return AtomNetId::INVALID();
    }
    return iter->second;
}

int AtomMap::clb_net(const AtomNetId net_id) const {
    auto iter = atom_to_clb_net_.find(net_id);
    if(iter == atom_to_clb_net_.end()) {
        //Not found
        return OPEN;
    }
    return iter->second;

}


void AtomMap::set_atom_clb_net(const AtomNetId net_id, const int clb_net_index) {
    VTR_ASSERT(net_id);
    //If either are invalid remove any mapping
    if(!net_id) {
        //Remove
        clb_to_atom_net_.erase(clb_net_index);
        VTR_ASSERT_SAFE(atom_to_clb_net_.count(net_id) == 0);
    }
    if(clb_net_index == OPEN) {
        //Remove
        atom_to_clb_net_.erase(net_id);
        VTR_ASSERT_SAFE(clb_to_atom_net_.count(clb_net_index) == 0);
    }
    
    //If both are valid store the mapping
    if(net_id && clb_net_index != OPEN) {
        //Store
        atom_to_clb_net_[net_id] = clb_net_index;
        clb_to_atom_net_[clb_net_index] = net_id;
    }
}

/*
 * Classic Timing nodes
 */
AtomPinId AtomMap::classic_tnode_atom_pin(const int tnode_index) const {
    auto iter = tnode_to_atom_pin_.find(tnode_index);
    if(iter == tnode_to_atom_pin_.end()) {
        //Not found
        return AtomPinId::INVALID();
    }
    return iter->second;
}

int AtomMap::atom_pin_classic_tnode(const AtomPinId pin_id) const {
    auto iter = atom_pin_to_tnode_.find(pin_id);
    if(iter == atom_pin_to_tnode_.end()) {
        //Not found
        return OPEN;
    }
    return iter->second;

}


void AtomMap::set_atom_pin_classic_tnode(const AtomPinId pin_id, const int tnode_index) {
    VTR_ASSERT(pin_id);
    //If either are invalid remove any mapping
    if(!pin_id) {
        //Remove
        tnode_to_atom_pin_.erase(tnode_index);
        VTR_ASSERT_SAFE(atom_pin_to_tnode_.count(pin_id) == 0);
    }
    if(tnode_index == OPEN) {
        //Remove
        atom_pin_to_tnode_.erase(pin_id);
        VTR_ASSERT_SAFE(tnode_to_atom_pin_.count(tnode_index) == 0);
    }
    
    //If both are valid store the mapping
    if(pin_id && tnode_index != OPEN) {
        //Store
        atom_pin_to_tnode_[pin_id] = tnode_index;
        tnode_to_atom_pin_[tnode_index] = pin_id;
    }
}

/*
 * Timing Nodes
 */
tatum::NodeId AtomMap::pin_tnode(const AtomPinId pin) const {
    return pin_tnode_[pin];
}

AtomPinId AtomMap::tnode_pin(const tatum::NodeId tnode) const {
    return pin_tnode_[tnode];
}

AtomMap::pin_tnode_range AtomMap::pin_tnodes() const {
    return vtr::make_range(pin_tnode_.begin(), pin_tnode_.end());
}

void AtomMap::set_pin_tnode(const AtomPinId pin, const tatum::NodeId node) {
    pin_tnode_.update(pin, node);
}
