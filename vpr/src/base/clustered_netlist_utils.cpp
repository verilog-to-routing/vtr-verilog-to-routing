#include "clustered_netlist_utils.h"
ClusteredPinAtomPinsLookup::ClusteredPinAtomPinsLookup(const ClusteredNetlist& clustered_netlist, const IntraLbPbPinLookup& pb_gpin_lookup) {
    init_lookup(clustered_netlist, pb_gpin_lookup);
}

ClusteredPinAtomPinsLookup::atom_pin_range ClusteredPinAtomPinsLookup::connected_atom_pins(ClusterPinId clustered_pin) const {
    VTR_ASSERT(clustered_pin);
    //return vtr::make_range(clustered_pin_connected_atom_pins_[clustered_pin]);
    return vtr::make_range(clustered_pin_connected_atom_pins_[clustered_pin].begin(),
                           clustered_pin_connected_atom_pins_[clustered_pin].end());
    //return atom_pin_range(clustered_pin_connected_atom_pins_[clustered_pin].begin(), clustered_pin_connected_atom_pins_[clustered_pin].end());
}

void ClusteredPinAtomPinsLookup::init_lookup(const ClusteredNetlist& clustered_netlist, const IntraLbPbPinLookup& pb_gpin_lookup) {
    auto clustered_pins = clustered_netlist.pins();
    clustered_pin_connected_atom_pins_.clear();
    clustered_pin_connected_atom_pins_.resize(clustered_pins.size());
    for (ClusterPinId clustered_pin : clustered_pins) {
        auto clustered_block = clustered_netlist.pin_block(clustered_pin);
        int phys_pin_index = clustered_netlist.pin_physical_index(clustered_pin);
        clustered_pin_connected_atom_pins_[clustered_pin] = find_clb_pin_connected_atom_pins(clustered_block, phys_pin_index, pb_gpin_lookup);
    }
}
