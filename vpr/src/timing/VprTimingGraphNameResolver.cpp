#include "VprTimingGraphNameResolver.h"
#include "atom_netlist.h"
#include "atom_lookup.h"


VprTimingGraphResolver::VprTimingGraphResolver(const AtomNetlist& netlist, const AtomLookup& netlist_lookup)
    : netlist_(netlist)
    , netlist_lookup_(netlist_lookup) {}

std::string VprTimingGraphResolver::node_name(tatum::NodeId node) const {
    AtomPinId pin = netlist_lookup_.tnode_atom_pin(node);

    return netlist_.pin_name(pin);
}

std::string VprTimingGraphResolver::node_type_name(tatum::NodeId node) const {
    AtomPinId pin = netlist_lookup_.tnode_atom_pin(node);
    AtomBlockId blk = netlist_.pin_block(pin);

    return netlist_.block_model(blk)->name;
}

tatum::EdgeDelayBreakdown VprTimingGraphResolver::edge_delay_breakdown(tatum::EdgeId edge, tatum::DelayType delay_type) const {
    tatum::EdgeDelayBreakdown edge_delay_breakdown;

    if (edge && detailed()) {

    }

    return edge_delay_breakdown;
}

bool VprTimingGraphResolver::detailed() const {
    return detailed_;
}

void VprTimingGraphResolver::set_detailed(bool value) {
    detailed_ = value;
}
