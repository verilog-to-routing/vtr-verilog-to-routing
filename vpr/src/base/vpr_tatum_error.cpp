#include "vpr_tatum_error.h"
#include "globals.h"

std::string format_tatum_error(const tatum::Error& error) {
    std::string msg;

    msg += "STA Engine: ";
    msg += error.what();

    auto& timing_ctx = g_vpr_ctx.timing();
    auto& atom_ctx = g_vpr_ctx.atom();

    if (error.node || error.edge) {
        msg += " (";
    }

    if (error.node) {
        
        AtomPinId pin = atom_ctx.lookup.tnode_atom_pin(error.node);
        if (pin) {
            msg += "Netlist Pin: '" + atom_ctx.nlist.pin_name(pin) + "', ";
        }
        msg += "Timing Graph Node: " + std::to_string(size_t(error.node));
    }

    if (error.edge && error.node) {
        msg += "; ";
    }

    if (error.edge) {
        tatum::NodeId src_node = timing_ctx.graph->edge_src_node(error.edge);
        tatum::NodeId sink_node = timing_ctx.graph->edge_sink_node(error.edge);

        AtomPinId src_pin = atom_ctx.lookup.tnode_atom_pin(src_node);
        AtomPinId sink_pin = atom_ctx.lookup.tnode_atom_pin(sink_node);

        if (src_pin && sink_pin) {
            msg += "Between netlist pins ";

            msg += "'" + atom_ctx.nlist.pin_name(src_pin) + "' -> '" + atom_ctx.nlist.pin_name(sink_pin) + "'";

            AtomNetId src_net = atom_ctx.nlist.pin_net(src_pin);
            AtomNetId sink_net = atom_ctx.nlist.pin_net(sink_pin);
            if (src_net && src_net == sink_net) {
                msg += " via net '" + atom_ctx.nlist.net_name(src_net) + "'";
            }

            msg += ", ";
        }

        msg += "Timing Graph Edge: " + std::to_string(size_t(error.edge));
    }

    if (error.node || error.edge) {
        msg += ")";
    }

    return msg;
}
