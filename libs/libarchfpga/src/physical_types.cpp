#include "physical_types.h"
#include "vtr_math.h"
#include "vtr_util.h"

static bool switch_type_is_buffered(SwitchType type);
static bool switch_type_is_configurable(SwitchType type);
static e_directionality switch_type_directionaity(SwitchType type);

//Ensure the constant has external linkage to avoid linking errors
constexpr int t_arch_switch_inf::UNDEFINED_FANIN;

/*
 * t_arch_switch_inf
 */

SwitchType t_arch_switch_inf::type() const {
    return type_;
}

bool t_arch_switch_inf::buffered() const {
    return switch_type_is_buffered(type());
}

bool t_arch_switch_inf::configurable() const {
    return switch_type_is_configurable(type());
}

e_directionality t_arch_switch_inf::directionality() const {
    return switch_type_directionaity(type());
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

void t_arch_switch_inf::set_type(SwitchType type_val) {
    type_ = type_val;
}

/*
 * t_rr_switch_inf
 */

SwitchType t_rr_switch_inf::type() const {
    return type_;
}

bool t_rr_switch_inf::buffered() const {
    return switch_type_is_buffered(type());
}

bool t_rr_switch_inf::configurable() const {
    return switch_type_is_configurable(type());
}

void t_rr_switch_inf::set_type(SwitchType type_val) {
    type_ = type_val;
}

static bool switch_type_is_buffered(SwitchType type) {
    //Muxes and Tristates isolate thier input and output into
    //seperate DC connected sub-circuits
    return type == SwitchType::MUX
           || type == SwitchType::TRISTATE
           || type == SwitchType::BUFFER;
}

static bool switch_type_is_configurable(SwitchType type) {
    //Shorts and buffers are non-configurable
    return !(type == SwitchType::SHORT
             || type == SwitchType::BUFFER);
}

static e_directionality switch_type_directionaity(SwitchType type) {
    if (type == SwitchType::SHORT
        || type == SwitchType::PASS_GATE) {
        //Shorts and pass gates can conduct in either direction
        return e_directionality::BI_DIRECTIONAL;
    } else {
        VTR_ASSERT_SAFE(type == SwitchType::MUX
                        || type == SwitchType::TRISTATE
                        || type == SwitchType::BUFFER);
        //Buffered switches can only drive in one direction
        return e_directionality::UNI_DIRECTIONAL;
    }
}

/*
 * t_physical_tile_type
 */
std::vector<int> t_physical_tile_type::get_clock_pins_indices() const {
    std::vector<int> indices; // function return vector

    // Temporary variables
    int clock_pins_start_idx = 0;
    int clock_pins_stop_idx = 0;

    for (int capacity_num = 0; capacity_num < this->capacity; capacity_num++) {
        // Ranges are picked on the basis that pins are ordered: inputs, outputs, then clock pins
        // This is because ProcessPb_type assigns pb_type port indices in that order and
        // SetupPinLocationsAndPinClasses assigns t_logical_block_type_ptr pin indices in the order of port indices
        // TODO: This pin ordering assumption is also used functions such as load_external_nets_and_cb
        //       either remove this assumption all togther and create a better mapping or make use of
        //       the same functions throughout the code that return the pin ranges.
        clock_pins_start_idx = this->num_input_pins + this->num_output_pins + clock_pins_stop_idx;
        clock_pins_stop_idx = clock_pins_start_idx + this->num_clock_pins;

        for (int pin_idx = clock_pins_start_idx; pin_idx < clock_pins_stop_idx; pin_idx++) {
            indices.push_back(pin_idx);
        }
    }

    // assert that indices are not out of bounds by checking the last index inserted
    if (!indices.empty()) {
        VTR_ASSERT(indices.back() < this->num_pins);
    }

    return indices;
}

/**
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

/**
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
