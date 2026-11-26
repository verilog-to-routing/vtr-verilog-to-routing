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
        load_connectivity_(atom_nlist_);
    } else {
        load_connectivity_(clb_nlist_);
    }
}

bool RoutingNetlist::matches_source() const {
    if (is_flat()) {
        return matches_connectivity_(atom_nlist_);
    }
    return matches_connectivity_(clb_nlist_);
}

template<typename SourceNetlist>
void RoutingNetlist::load_connectivity_(const SourceNetlist& src_nlist) {
    // The blocks, pins and nets of this netlist are numbered like those of the source
    // netlist, which is only possible if the source IDs are contiguous (starting from 0).
    size_t num_blocks = 0;
    for (auto src_blk_id : src_nlist.blocks()) {
        VTR_ASSERT_MSG(size_t(src_blk_id) == num_blocks, "Block IDs of the netlist to route must be contiguous");
        num_blocks++;
    }
    num_blocks_ = num_blocks;

    size_t num_pins = 0;
    for (auto src_pin_id : src_nlist.pins()) {
        VTR_ASSERT_MSG(size_t(src_pin_id) == num_pins, "Pin IDs of the netlist to route must be contiguous");
        num_pins++;
    }
    num_pins_ = num_pins;

    size_t num_nets = src_nlist.nets().size();
    net_first_pin_.reserve(num_nets + 1);
    net_pins_.reserve(num_pins);
    for (auto src_net_id : src_nlist.nets()) {
        VTR_ASSERT_MSG(size_t(src_net_id) == net_first_pin_.size(), "Net IDs of the netlist to route must be contiguous");
        VTR_ASSERT_MSG(!src_nlist.net_pins(src_net_id).empty(), "Every net must have an entry for its driver");

        net_first_pin_.push_back(net_pins_.size());
        for (auto src_pin_id : src_nlist.net_pins(src_net_id)) {
            // The driver of a net may be invalid (a net without a driver), which is kept as is.
            net_pins_.push_back(src_pin_id.is_valid() ? RoutingPinId(size_t(src_pin_id)) : RoutingPinId::INVALID());
        }
    }
    net_first_pin_.push_back(net_pins_.size());
}

template<typename SourceNetlist>
bool RoutingNetlist::matches_connectivity_(const SourceNetlist& src_nlist) const {
    if (src_nlist.blocks().size() != blocks().size()
        || src_nlist.pins().size() != pins().size()
        || src_nlist.nets().size() != nets().size()) {
        return false;
    }

    for (auto src_net_id : src_nlist.nets()) {
        if (size_t(src_net_id) >= nets().size()) {
            return false;
        }

        pin_range pins_of_net = net_pins(RoutingNetId(size_t(src_net_id)));
        auto src_pins_of_net = src_nlist.net_pins(src_net_id);
        if (src_pins_of_net.size() != pins_of_net.size()) {
            return false;
        }

        size_t ipin = 0;
        for (auto src_pin_id : src_pins_of_net) {
            RoutingPinId pin_id = pins_of_net[ipin++];
            if (src_pin_id.is_valid() != pin_id.is_valid()
                || (pin_id.is_valid() && size_t(src_pin_id) != size_t(pin_id))) {
                return false;
            }
        }
    }

    return true;
}

RoutingNetlist make_routing_netlist(bool is_flat) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    return RoutingNetlist(atom_ctx.netlist(), g_vpr_ctx.clustering().clb_nlist, atom_ctx.lookup(), is_flat);
}
