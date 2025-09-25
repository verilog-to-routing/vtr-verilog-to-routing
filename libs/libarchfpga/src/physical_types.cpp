#include "physical_types.h"
#include "arch_types.h"
#include "vtr_math.h"
#include "vtr_util.h"

#include "arch_util.h"

/*
 * t_arch_switch_inf
 */

e_switch_type t_arch_switch_inf::type() const {
    return type_;
}

bool t_arch_switch_inf::buffered() const {
    return switch_type_is_buffered(type());
}

bool t_arch_switch_inf::configurable() const {
    return switch_type_is_configurable(type());
}

e_directionality t_arch_switch_inf::directionality() const {
    return switch_type_directionality(type());
}

float t_arch_switch_inf::Tdel(int fanin) const {
    if (fixed_Tdel()) {
        auto itr = Tdel_map_.find(UNDEFINED_FANIN);
        VTR_ASSERT(itr != Tdel_map_.end());
        return itr->second;
    } else {
        VTR_ASSERT(fanin >= 0);
        return vtr::linear_interpolate_or_extrapolate(&Tdel_map_, fanin);
    }
}

bool t_arch_switch_inf::fixed_Tdel() const {
    return Tdel_map_.size() == 1 && Tdel_map_.count(UNDEFINED_FANIN);
}

void t_arch_switch_inf::set_Tdel(int fanin, float delay) {
    Tdel_map_[fanin] = delay;
}

void t_arch_switch_inf::set_type(e_switch_type type_val) {
    type_ = type_val;
}

bool switch_type_is_buffered(e_switch_type type) {
    //Muxes and Tristates isolate their input and output into
    //separate DC connected sub-circuits
    return type == e_switch_type::MUX
           || type == e_switch_type::TRISTATE
           || type == e_switch_type::BUFFER;
}

bool switch_type_is_configurable(e_switch_type type) {
    //Shorts and buffers are non-configurable
    return !(type == e_switch_type::SHORT
             || type == e_switch_type::BUFFER);
}

e_directionality switch_type_directionality(e_switch_type type) {
    if (type == e_switch_type::SHORT || type == e_switch_type::PASS_GATE) {
        //Shorts and pass gates can conduct in either direction
        return e_directionality::BI_DIRECTIONAL;
    } else {
        VTR_ASSERT_SAFE(type == e_switch_type::MUX
                        || type == e_switch_type::TRISTATE
                        || type == e_switch_type::BUFFER);
        //Buffered switches can only drive in one direction
        return e_directionality::UNI_DIRECTIONAL;
    }
}

/*
 * t_physical_tile_type
 */
std::vector<int> t_physical_tile_type::get_clock_pins_indices() const {
    for (auto pin_index : this->clock_pin_indices) {
        VTR_ASSERT(pin_index < this->num_pins);
    }

    return this->clock_pin_indices;
}

int t_physical_tile_type::get_sub_tile_loc_from_pin(int pin_num) const {
    VTR_ASSERT(pin_num < this->num_pins);

    for (auto sub_tile : this->sub_tiles) {
        auto max_inst_pins = sub_tile.num_phy_pins / sub_tile.capacity.total();

        for (int pin = 0; pin < sub_tile.num_phy_pins; pin++) {
            if (sub_tile.sub_tile_to_tile_pin_indices[pin] == pin_num) {
                //If the physical tile pin matches pin_num, return the
                //corresponding absolute capacity location of the sub_tile
                return pin / max_inst_pins + sub_tile.capacity.low;
            }
        }
    }

    return ARCH_FPGA_UNDEFINED_VAL;
}

bool t_physical_tile_type::is_empty() const {
    return name == std::string(EMPTY_BLOCK_NAME);
}

int t_physical_tile_type::find_pin(std::string_view port_name, int pin_index_in_port) const {
    int ipin = ARCH_FPGA_UNDEFINED_VAL;
    int port_base_ipin = 0;
    int num_port_pins = ARCH_FPGA_UNDEFINED_VAL;
    int pin_offset = 0;

    bool port_found = false;
    for (const t_sub_tile& sub_tile : sub_tiles) {
        for (const t_physical_tile_port& port : sub_tile.ports) {
            if (port_name == port.name) {
                port_found = true;
                num_port_pins = port.num_pins;
                break;
            }

            port_base_ipin += port.num_pins;
        }

        if (port_found) {
            break;
        }

        port_base_ipin = 0;
        pin_offset += sub_tile.num_phy_pins;
    }

    if (num_port_pins != ARCH_FPGA_UNDEFINED_VAL) {
        VTR_ASSERT(pin_index_in_port < num_port_pins);

        ipin = port_base_ipin + pin_index_in_port + pin_offset;
    }

    return ipin;
}

int t_physical_tile_type::find_pin_class(std::string_view port_name, int pin_index_in_port, e_pin_type pin_type) const {
    int iclass = ARCH_FPGA_UNDEFINED_VAL;

    int ipin = find_pin(port_name, pin_index_in_port);

    if (ipin != ARCH_FPGA_UNDEFINED_VAL) {
        iclass = pin_class[ipin];

        if (iclass != ARCH_FPGA_UNDEFINED_VAL) {
            VTR_ASSERT(class_inf[iclass].type == pin_type);
        }
    }
    return iclass;
}

/*
 * t_logical_block_type
 */

bool t_logical_block_type::is_empty() const {
    return name == std::string(EMPTY_BLOCK_NAME);
}

bool t_logical_block_type::is_io() const {
    // Iterate over all equivalent tiles and return true if any
    // of them are IO tiles
    for (t_physical_tile_type_ptr tile : equivalent_tiles) {
        if (tile->is_io()) {
            return true;
        }
    }
    return false;
}

const t_port* t_logical_block_type::get_port(std::string_view port_name) const {
    for (int i = 0; i < pb_type->num_ports; i++) {
        const t_port& port = pb_type->ports[i];
        if (port_name == port.name) {
            return &pb_type->ports[port.index];
        }
    }

    return nullptr;
}

const t_port* t_logical_block_type::get_port_by_pin(int pin) const {
    for (int i = 0; i < pb_type->num_ports; i++) {
        const t_port& port = pb_type->ports[i];
        if (pin >= port.absolute_first_pin_index && pin < port.absolute_first_pin_index + port.num_pins) {
            return &pb_type->ports[port.index];
        }
    }

    return nullptr;
}

/*
 * t_pb_type
 */

int t_pb_type::get_max_primitives() const {
    int max_size;

    if (modes == nullptr) {
        max_size = 1;
    } else {
        max_size = 0;
        int temp_size = 0;
        for (int i = 0; i < num_modes; i++) {
            for (int j = 0; j < modes[i].num_pb_type_children; j++) {
                temp_size += modes[i].pb_type_children[j].num_pb * modes[i].pb_type_children[j].get_max_primitives();
            }
            if (temp_size > max_size) {
                max_size = temp_size;
            }
        }
    }

    return max_size;
}

/* finds maximum number of nets that can be contained in pb_type, this is bounded by the number of driving pins */
int t_pb_type::get_max_nets() const {
    int max_nets;
    if (modes == nullptr) {
        max_nets = num_output_pins;
    } else {
        max_nets = 0;

        for (int i = 0; i < num_modes; i++) {
            int temp_nets = 0;
            for (int j = 0; j < modes[i].num_pb_type_children; j++) {
                temp_nets += modes[i].pb_type_children[j].num_pb * modes[i].pb_type_children[j].get_max_nets();
            }

            if (temp_nets > max_nets) {
                max_nets = temp_nets;
            }
        }
    }

    if (is_root()) {
        max_nets += num_input_pins + num_output_pins + num_clock_pins;
    }

    return max_nets;
}

int t_pb_type::get_max_depth() const {
    int max_depth = depth;

    for (int i = 0; i < num_modes; i++) {
        for (int j = 0; j < modes[i].num_pb_type_children; j++) {
            int temp_depth = modes[i].pb_type_children[j].get_max_depth();
            max_depth = std::max(max_depth, temp_depth);
        }
    }
    return max_depth;
}

/*
 * t_pb_graph_node
 */

int t_pb_graph_node::num_pins() const {
    int npins = 0;

    for (int iport = 0; iport < num_input_ports; ++iport) {
        npins += num_input_pins[iport];
    }

    for (int iport = 0; iport < num_output_ports; ++iport) {
        npins += num_output_pins[iport];
    }

    for (int iport = 0; iport < num_clock_ports; ++iport) {
        npins += num_clock_pins[iport];
    }

    return npins;
}

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

/*
 * t_pb_graph_edge
 */

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
        for (int ipin = 0; ipin < this->num_output_pins; ipin++) {
            for (int iedge = 0; iedge < this->output_pins[ipin]->num_output_edges; iedge++) {
                if (this->output_pins[ipin]->output_edges[iedge]->belongs_to_pattern(pattern_index)) {
                    return true;
                }
            }
        }
    }

    // return false otherwise
    return false;
}

/*
 * t_sub_tile
 */

int t_sub_tile::total_num_internal_pins() const {
    int num_pins = 0;

    for (t_logical_block_type_ptr eq_site : equivalent_sites) {
        num_pins += (int)eq_site->pin_logical_num_to_pb_pin_mapping.size();
    }

    num_pins *= capacity.total();

    return num_pins;
}

const t_physical_tile_port* t_sub_tile::get_port(std::string_view port_name) {
    for (const t_physical_tile_port& port : ports) {
        if (port_name == port.name) {
            return &ports[port.index];
        }
    }

    return nullptr;
}

const t_physical_tile_port* t_sub_tile::get_port_by_pin(int pin) const {
    for (const t_physical_tile_port& port : ports) {
        if (pin >= port.absolute_first_pin_index && pin < port.absolute_first_pin_index + port.num_pins) {
            return &ports[port.index];
        }
    }

    return nullptr;
}
