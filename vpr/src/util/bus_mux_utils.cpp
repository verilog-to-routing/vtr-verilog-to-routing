#include "bus_mux_utils.h"

#include "vtr_assert.h"

bool is_bus_mux_edge(const t_pb_graph_edge* edge) {
    return edge->interconnect->type == MUX_INTERC && edge->interconnect->bus;
}

bool is_bus_mux_output_pin(const t_pb_graph_pin* pin) {
    for (int iedge = 0; iedge < pin->num_input_edges; iedge++) {
        if (is_bus_mux_edge(pin->input_edges[iedge])) {
            return true;
        }
    }
    return false;
}

const t_pb_graph_node* get_bus_mux_owner(const t_pb_graph_edge* edge) {
    VTR_ASSERT(edge->num_output_pins == 1);
    const t_pb_graph_node* out_node = edge->output_pins[0]->parent_node;
    const t_pb_type* owner_type = edge->interconnect->parent_mode->parent_pb_type;

    // The mux output is either an output port of the owner itself or an input
    // port of one of the owner's children.
    if (out_node->pb_type == owner_type) {
        return out_node;
    }
    VTR_ASSERT(out_node->parent_pb_graph_node != nullptr
               && out_node->parent_pb_graph_node->pb_type == owner_type);
    return out_node->parent_pb_graph_node;
}

bool pb_type_has_bus_mux(const t_pb_type* pb_type) {
    for (int imode = 0; imode < pb_type->num_modes; imode++) {
        const t_mode& mode = pb_type->modes[imode];
        for (int iinterc = 0; iinterc < mode.num_interconnect; iinterc++) {
            if (mode.interconnect[iinterc].type == MUX_INTERC && mode.interconnect[iinterc].bus) {
                return true;
            }
        }
        for (int ichild = 0; ichild < mode.num_pb_type_children; ichild++) {
            if (pb_type_has_bus_mux(&mode.pb_type_children[ichild])) {
                return true;
            }
        }
    }
    return false;
}
