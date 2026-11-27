#include "routing_netlist.h"

#include "globals.h"

RoutingNetlist::RoutingNetlist(const AtomNetlist& atom_nlist,
                               const ClusteredNetlist& clb_nlist,
                               const AtomLookup& atom_lookup,
                               bool is_flat)
    : atom_nlist_(atom_nlist)
    , clb_nlist_(clb_nlist)
    , atom_lookup_(atom_lookup)
    , is_flat_(is_flat) {
    // The blocks, pins and nets of this netlist are numbered like those of the source
    // netlist, which is only possible if the source IDs are contiguous (starting from 0),
    // i.e. if the source netlist is compressed.
    VTR_ASSERT_MSG(is_flat_ ? atom_nlist_.is_compressed() : clb_nlist_.is_compressed(),
                   "The netlist to route must be compressed");
}

RoutingNetlist make_routing_netlist(bool is_flat) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    return RoutingNetlist(atom_ctx.netlist(), g_vpr_ctx.clustering().clb_nlist, atom_ctx.lookup(), is_flat);
}
