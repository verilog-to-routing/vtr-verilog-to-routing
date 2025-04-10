#include "clustered_netlist_fwd.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_optional.h"

#include "atom_lookup.h"

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
    auto iter = clb_net_to_atom_net_.find(clb_net_index);
    if (iter == clb_net_to_atom_net_.end()) {
        //Not found
        return AtomNetId::INVALID();
    }
    return iter->second;
}

vtr::optional<const std::vector<ClusterNetId>&> AtomLookup::clb_nets(const AtomNetId atom_net) const {
    auto iter = atom_net_to_clb_nets_.find(atom_net);
    if (iter == atom_net_to_clb_nets_.end()) {
        //Not found
        return vtr::nullopt;
    }
    return iter->second;
}

void AtomLookup::add_atom_clb_net(const AtomNetId atom_net, const ClusterNetId clb_net) {
    VTR_ASSERT(atom_net && clb_net);

    /* Use the default behavior of [] operator */
    atom_net_to_clb_nets_[atom_net].push_back(clb_net);
    clb_net_to_atom_net_[clb_net] = atom_net;
}

void AtomLookup::remove_clb_net(const ClusterNetId clb_net) {
    if (!clb_net_to_atom_net_.count(clb_net))
        return;

    auto atom_net = clb_net_to_atom_net_[clb_net];
    auto& all_clb_nets = atom_net_to_clb_nets_[atom_net];
    /* This is o(n), but an AtomNetId rarely has >5 ClusterNetIds */
    all_clb_nets.erase(std::remove(all_clb_nets.begin(), all_clb_nets.end(), clb_net), all_clb_nets.end());
}

/* Remove mapping for given atom net */
void AtomLookup::remove_atom_net(const AtomNetId atom_net) {
    if (!atom_net_to_clb_nets_.count(atom_net))
        return;

    auto cluster_nets = atom_net_to_clb_nets_[atom_net];
    for (auto c : cluster_nets) {
        clb_net_to_atom_net_.erase(c);
    }
    atom_net_to_clb_nets_.erase(atom_net);
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

AtomLookup::pin_tnode_range AtomLookup::atom_pin_tnodes(BlockTnode block_tnode_type) const {
    if (block_tnode_type == BlockTnode::EXTERNAL) {
        return vtr::make_range(atom_pin_tnode_external_.begin(), atom_pin_tnode_external_.end());
    } else {
        VTR_ASSERT(block_tnode_type == BlockTnode::INTERNAL);
        return vtr::make_range(atom_pin_tnode_internal_.begin(), atom_pin_tnode_internal_.end());
    }
}

void AtomLookup::set_atom_pin_tnode(const AtomPinId pin, const tatum::NodeId node, BlockTnode block_tnode_type) {
    //A pin always expands to an external tnode (i.e. its external connectivity in the netlist)
    //but some pins may expand to an additional tnode (i.e. to SOURCE/SINK to cover internal sequential paths within a block)
    if (block_tnode_type == BlockTnode::EXTERNAL) {
        atom_pin_tnode_external_[pin] = node;
    } else {
        VTR_ASSERT(block_tnode_type == BlockTnode::INTERNAL);
        atom_pin_tnode_internal_[pin] = node;
    }

    //Each tnode maps to precisely one pin at any point in time
    tnode_atom_pin_[node] = pin;
}
