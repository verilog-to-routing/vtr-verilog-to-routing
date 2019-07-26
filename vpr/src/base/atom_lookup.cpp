#include "vtr_assert.h"
#include "vtr_log.h"

#include "atom_lookup.h"
/*
 * PB
 */
const t_pb* AtomLookup::atom_pb(const AtomBlockId blk_id) const {
    auto iter = atom_to_pb_.find(blk_id);
    if (iter == atom_to_pb_.end()) {
        //Not found
        return nullptr;
    }
    return iter->second;
}

AtomBlockId AtomLookup::pb_atom(const t_pb* pb) const {
    auto iter = atom_to_pb_.find(pb);
    if (iter == atom_to_pb_.inverse_end()) {
        //Not found
        return AtomBlockId::INVALID();
    }
    return iter->second;
}

const t_pb_graph_node* AtomLookup::atom_pb_graph_node(const AtomBlockId blk_id) const {
    const t_pb* pb = atom_pb(blk_id);
    if (pb) {
        //Found
        return pb->pb_graph_node;
    }
    return nullptr;
}

void AtomLookup::set_atom_pb(const AtomBlockId blk_id, const t_pb* pb) {
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

/*
 * PB Pins
 */
const t_pb_graph_pin* AtomLookup::atom_pin_pb_graph_pin(AtomPinId atom_pin) const {
    if (atom_pin_to_pb_graph_pin_.empty()) {
        return nullptr;
    }
    return atom_pin_to_pb_graph_pin_[atom_pin];
}

void AtomLookup::set_atom_pin_pb_graph_pin(AtomPinId atom_pin, const t_pb_graph_pin* gpin) {
    atom_pin_to_pb_graph_pin_.insert(atom_pin, gpin);
}

/*
 * Blocks
 */
ClusterBlockId AtomLookup::atom_clb(const AtomBlockId blk_id) const {
    VTR_ASSERT(blk_id);
    auto iter = atom_to_clb_.find(blk_id);
    if (iter == atom_to_clb_.end()) {
        return ClusterBlockId::INVALID();
    }

    return *iter;
}

void AtomLookup::set_atom_clb(const AtomBlockId blk_id, const ClusterBlockId clb) {
    VTR_ASSERT(blk_id);

    atom_to_clb_.update(blk_id, clb);
}

/*
 * Nets
 */
AtomNetId AtomLookup::atom_net(const ClusterNetId clb_net_index) const {
    auto iter = atom_net_to_clb_net_.find(clb_net_index);
    if (iter == atom_net_to_clb_net_.inverse_end()) {
        //Not found
        return AtomNetId::INVALID();
    }
    return iter->second;
}

ClusterNetId AtomLookup::clb_net(const AtomNetId net_id) const {
    auto iter = atom_net_to_clb_net_.find(net_id);
    if (iter == atom_net_to_clb_net_.end()) {
        //Not found
        return ClusterNetId::INVALID();
    }
    return iter->second;
}

void AtomLookup::set_atom_clb_net(const AtomNetId net_id, const ClusterNetId clb_net_index) {
    VTR_ASSERT(net_id);
    //If either are invalid remove any mapping
    if (!net_id && clb_net_index != ClusterNetId::INVALID()) {
        //Remove
        atom_net_to_clb_net_.erase(clb_net_index);
    } else if (net_id && clb_net_index == ClusterNetId::INVALID()) {
        //Remove
        atom_net_to_clb_net_.erase(net_id);
    } else if (net_id && clb_net_index != ClusterNetId::INVALID()) {
        //Store
        atom_net_to_clb_net_.update(net_id, clb_net_index);
    }
}

/*
 * Timing Nodes
 */
tatum::NodeId AtomLookup::atom_pin_tnode(const AtomPinId pin, BlockTnode block_tnode_type) const {
    if (block_tnode_type == BlockTnode::EXTERNAL) {
        auto iter = atom_pin_tnode_external_.find(pin);
        if (iter != atom_pin_tnode_external_.end()) {
            return iter->second;
        }
    } else {
        VTR_ASSERT(block_tnode_type == BlockTnode::INTERNAL);
        auto iter = atom_pin_tnode_internal_.find(pin);
        if (iter != atom_pin_tnode_internal_.end()) {
            return iter->second;
        }
    }

    return tatum::NodeId::INVALID(); //Not found
}

AtomPinId AtomLookup::tnode_atom_pin(const tatum::NodeId tnode) const {
    auto iter = tnode_atom_pin_.find(tnode);
    if (iter != tnode_atom_pin_.end()) {
        return iter->second;
    }

    return AtomPinId::INVALID(); //Not found
}

AtomLookup::tnode_pin_range AtomLookup::tnode_atom_pins() const {
    return vtr::make_range(tnode_atom_pin_.begin(), tnode_atom_pin_.end());
}

void AtomLookup::set_atom_pin_tnode(const AtomPinId pin, const tatum::NodeId node, BlockTnode block_tnode_type) {
    //A pin always expands to an external tnode (i.e. it's external connectivity in the netlist)
    //but some pins may expand to an additional tnode (i.e. to SOURCE/SINK to cover internal sequential paths within a block)
    if (block_tnode_type == BlockTnode::EXTERNAL) {
        atom_pin_tnode_external_[pin] = node;
    } else {
        VTR_ASSERT(block_tnode_type == BlockTnode::INTERNAL);
        atom_pin_tnode_internal_[pin] = node;
    }

    //Each tnode maps to precisely one pin
    tnode_atom_pin_[node] = pin;
}
