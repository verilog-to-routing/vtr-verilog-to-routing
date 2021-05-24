#include "clustered_netlist_utils.h"
#include "globals.h"
ClusteredPinAtomPinsLookup::ClusteredPinAtomPinsLookup(const ClusteredNetlist& clustered_netlist, const AtomNetlist& atom_netlist, const IntraLbPbPinLookup& pb_gpin_lookup) {
    init_lookup(clustered_netlist, atom_netlist, pb_gpin_lookup);
}

ClusteredPinAtomPinsLookup::atom_pin_range ClusteredPinAtomPinsLookup::connected_atom_pins(ClusterPinId clustered_pin) const {
    VTR_ASSERT(clustered_pin);

    return vtr::make_range(clustered_pin_connected_atom_pins_[clustered_pin].begin(),
                           clustered_pin_connected_atom_pins_[clustered_pin].end());
}

ClusterPinId ClusteredPinAtomPinsLookup::connected_clb_pin(AtomPinId atom_pin) const {
    return atom_pin_connected_cluster_pin_[atom_pin];
}

void ClusteredPinAtomPinsLookup::init_lookup(const ClusteredNetlist& clustered_netlist, const AtomNetlist& atom_netlist, const IntraLbPbPinLookup& pb_gpin_lookup) {
    auto clustered_pins = clustered_netlist.pins();

    clustered_pin_connected_atom_pins_.clear();
    clustered_pin_connected_atom_pins_.resize(clustered_pins.size());

    atom_pin_connected_cluster_pin_.clear();
    atom_pin_connected_cluster_pin_.resize(atom_netlist.pins().size());

    for (ClusterPinId clustered_pin : clustered_pins) {
        auto clustered_block = clustered_netlist.pin_block(clustered_pin);
        int logical_pin_index = clustered_netlist.pin_logical_index(clustered_pin);
        clustered_pin_connected_atom_pins_[clustered_pin] = find_clb_pin_connected_atom_pins(clustered_block, logical_pin_index, pb_gpin_lookup);

        for (AtomPinId atom_pin : clustered_pin_connected_atom_pins_[clustered_pin]) {
            atom_pin_connected_cluster_pin_[atom_pin] = clustered_pin;
        }
    }
}

ClusterAtomsLookup::ClusterAtomsLookup() {
    init_lookup();
}

void ClusterAtomsLookup::init_lookup() {
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    cluster_atoms.resize(cluster_ctx.clb_nlist.blocks().size());

    for (auto atom_blk_id : atom_ctx.nlist.blocks()) {
        ClusterBlockId clb_index = atom_ctx.lookup.atom_clb(atom_blk_id);

        cluster_atoms[clb_index].push_back(atom_blk_id);
    }
}

std::vector<AtomBlockId> ClusterAtomsLookup::atoms_in_cluster(ClusterBlockId blk_id) {
    std::vector<AtomBlockId> atoms = cluster_atoms[blk_id];
    return atoms;
}
