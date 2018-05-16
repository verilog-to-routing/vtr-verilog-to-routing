#include "VprTimingGraphNameResolver.h"
#include "atom_netlist.h"
#include "atom_lookup.h"


VprTimingGraphNameResolver::VprTimingGraphNameResolver(const AtomNetlist& netlist, const AtomLookup& netlist_lookup)
    : netlist_(netlist)
    , netlist_lookup_(netlist_lookup) {}

std::string VprTimingGraphNameResolver::node_name(tatum::NodeId node) const {
    AtomPinId pin = netlist_lookup_.tnode_atom_pin(node);

    return netlist_.pin_name(pin);
}

std::string VprTimingGraphNameResolver::node_type_name(tatum::NodeId node) const {
    AtomPinId pin = netlist_lookup_.tnode_atom_pin(node);
    AtomBlockId blk = netlist_.pin_block(pin);

    return netlist_.block_model(blk)->name;
}
