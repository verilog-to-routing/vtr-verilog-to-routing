#include "pb_graph_types.h"

#include <string>
#include <vector>

#include "vtr_assert.h"
#include "vtr_util.h"

/*
 * t_pb_graph_node
 */

std::string t_pb_graph_node::hierarchical_type_name() const {
    std::vector<std::string> names;
    std::string child_mode_name;

    for (auto curr_node = this; curr_node != nullptr; curr_node = curr_node->parent_pb_graph_node) {
        std::string type_name;

        //get name and type of physical block
        type_name = curr_node->pb_type->name;
        type_name += "[" + std::to_string(curr_node->placement_index) + "]";

        if (!curr_node->is_primitive()) {
            // primitives have no modes
            type_name += "[" + child_mode_name + "]";
        }

        if (!curr_node->is_root()) {
            // get the mode of this child
            child_mode_name = curr_node->pb_type->parent_mode->name;
        }

        names.push_back(type_name);
    }

    //We walked up from the leaf to root, so we join in reverse order
    return vtr::join(names.rbegin(), names.rend(), "/");
}

/**
 * t_pb_graph_pin
 */

std::string t_pb_graph_pin::to_string(const bool full_description) const {
    std::string parent_name = this->parent_node->pb_type->name;
    std::string parent_index = std::to_string(this->parent_node->placement_index);
    std::string port_name = this->port->name;
    std::string pin_index = std::to_string(this->pin_number);

    std::string pin_string = parent_name + "[" + parent_index + "]";
    pin_string += "." + port_name + "[" + pin_index + "]";

    if (!full_description) return pin_string;

    // Traverse upward through the pb_type hierarchy, constructing
    // name that represents the whole hierarchy to reach this pin.
    auto parent_parent_node = this->parent_node->parent_pb_graph_node;
    for (auto pb_node = parent_parent_node; pb_node != nullptr; pb_node = pb_node->parent_pb_graph_node) {
        std::string parent = pb_node->pb_type->name;
        parent += "[" + std::to_string(pb_node->placement_index) + "]";
        pin_string = parent + "/" + pin_string;
    }
    return pin_string;
}

bool t_pb_graph_pin::is_bus_mux_output() const {
    for (int iedge = 0; iedge < num_input_edges; iedge++) {
        if (input_edges[iedge]->is_bus_mux()) {
            return true;
        }
    }
    return false;
}

const t_pb_graph_edge* t_pb_graph_pin::edge_to(const t_pb_graph_pin* sink_pin) const {
    return output_edges[sink_pin_edge_idx_map.at(sink_pin)];
}

/*
 * t_pb_graph_edge
 */

bool t_pb_graph_edge::is_bus_mux() const {
    return interconnect->type == MUX_INTERC && interconnect->bus;
}

const t_pb_graph_node* t_pb_graph_edge::bus_mux_owner() const {
    const t_pb_graph_node* out_node = output_pin()->parent_node;
    const t_pb_type* owner_type = interconnect->parent_mode->parent_pb_type;

    // The mux output is either an output port of the owner itself or an input
    // port of one of the owner's children.
    if (out_node->pb_type == owner_type) {
        return out_node;
    }
    VTR_ASSERT(out_node->parent_pb_graph_node != nullptr
               && out_node->parent_pb_graph_node->pb_type == owner_type);
    return out_node->parent_pb_graph_node;
}

bool t_pb_graph_edge::annotated_with_pattern(int pattern_index) const {
    for (int ipattern = 0; ipattern < this->num_pack_patterns; ipattern++) {
        if (this->pack_pattern_indices[ipattern] == pattern_index) {
            return true;
        }
    }

    return false;
}

bool t_pb_graph_edge::belongs_to_pattern(int pattern_index) const {
    // return true if this edge is annotated with this pattern
    if (this->annotated_with_pattern(pattern_index)) {
        return true;
        // if not annotated check if its pattern should be inferred
    } else if (this->infer_pattern) {
        // if pattern should be inferred try to infer it from all connected output edges
        for (const t_pb_graph_edge* out_edge : this->output_pin()->output_edges) {
            if (out_edge->belongs_to_pattern(pattern_index)) {
                return true;
            }
        }
    }

    // return false otherwise
    return false;
}
