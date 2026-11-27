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
    if (is_flat_) {
        check_source_(atom_nlist_);
    } else {
        check_source_(clb_nlist_);
    }
}

template<typename SourceNetlist>
void RoutingNetlist::check_source_(const SourceNetlist& src_nlist) {
    // The blocks, pins and nets of this netlist are numbered like those of the source
    // netlist, which is only possible if the source IDs are contiguous (starting from 0).
    size_t num_blocks = 0;
    for (auto src_blk_id : src_nlist.blocks()) {
        VTR_ASSERT_MSG(size_t(src_blk_id) == num_blocks, "Block IDs of the netlist to route must be contiguous");
        num_blocks++;
    }

    size_t num_pins = 0;
    for (auto src_pin_id : src_nlist.pins()) {
        VTR_ASSERT_MSG(size_t(src_pin_id) == num_pins, "Pin IDs of the netlist to route must be contiguous");
        num_pins++;
    }

    size_t num_nets = 0;
    for (auto src_net_id : src_nlist.nets()) {
        VTR_ASSERT_MSG(size_t(src_net_id) == num_nets, "Net IDs of the netlist to route must be contiguous");
        VTR_ASSERT_MSG(!src_nlist.net_pins(src_net_id).empty(), "Every net must have an entry for its driver");
        num_nets++;
    }
}

RoutingNetlist make_routing_netlist(bool is_flat) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    return RoutingNetlist(atom_ctx.netlist(), g_vpr_ctx.clustering().clb_nlist, atom_ctx.lookup(), is_flat);
}
