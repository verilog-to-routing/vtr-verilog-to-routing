#include "physical_types.h"
#include "vtr_math.h"

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
    return !(   type == SwitchType::SHORT
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
 * t_type_descriptor
 */

std::vector<int> t_type_descriptor::get_clock_pins_indices () const {

    VTR_ASSERT(this->pb_type); // assert not a nullptr

    std::vector<int> indices;  // function return vector

    // Temporary variables
    int num_input_pins  = this->pb_type->num_input_pins;
    int num_output_pins = this->pb_type->num_output_pins;
    int num_clock_pins  = this->pb_type->num_clock_pins;

    int clock_pins_start_idx = 0;
    int clock_pins_stop_idx  = 0;

    for (int capacity_num = 0; capacity_num < this->capacity; capacity_num++) {

        // Ranges are picked on the basis that pins are ordered: inputs, outputs, then clock pins
        // This is because ProcessPb_type assigns pb_type port indices in that order and
        // SetupPinLocationsAndPinClasses assigns t_type_ptr pin indices in the order of port indices
        // TODO: This pin ordering assumption is also used functions such as load_external_nets_and_cb
        //       either remove this assumption all togther and create a better mapping or make use of
        //       the same functions throughout the code that return the pin ranges.
        clock_pins_start_idx = num_input_pins + num_output_pins + clock_pins_stop_idx;
        clock_pins_stop_idx  = clock_pins_start_idx + num_clock_pins;

        for (int pin_idx = clock_pins_start_idx; pin_idx < clock_pins_stop_idx; pin_idx++){
            indices.push_back(pin_idx);
        }
    }

    // assert that indices are not out of bounds by checking the last index inserted
    if(!indices.empty()) {
        VTR_ASSERT(indices.back() < this->num_pins);
    }

    return indices;
}


/**
 * t_pb_graph_pin
 */

/**
 *  Returns true if this pin belongs to a primtive
 */

bool t_pb_graph_pin::is_primitive_pin() const {
    return this->parent_node->pb_type->num_modes == 0;
}


/**
 *  Returns true if this pin belongs to a root pb_block
 *  which is a pb_block that has no parent block
 */

bool t_pb_graph_pin::is_root_block_pin() const {
    return this->parent_node->pb_type->parent_mode == nullptr;
}


std::string t_pb_graph_pin::pin_to_string() const {

    std::string parent_name  = this->parent_node->pb_type->name;
    std::string parent_index = std::to_string(this->parent_node->placement_index);
    std::string port_name    = this->port->name;
    std::string pin_index    = std::to_string(this->pin_number);

    std::string pin_string = parent_name + "[" + parent_index + "]";
    pin_string     += "." + port_name + "[" + pin_index + "]";

    for (auto pb_node = this->parent_node->parent_pb_graph_node; pb_node != nullptr; pb_node = pb_node->parent_pb_graph_node) {
        std::string parent = pb_node->pb_type->name;
        parent += "[" + std::to_string(pb_node->placement_index) + "]";
        pin_string = parent + "/" + pin_string;
    }
    return pin_string;
}


/**
 * t_pb_graph_edge
 */

/**
 * Returns true if this edge is annotated with the packing
 * pattern having the index pattern_index
 */

bool t_pb_graph_edge::annotated_with_pattern(int pattern_index) const {

    for(int ipattern = 0; ipattern < this->num_pack_patterns; ipattern++) {
        if (this->pack_pattern_indices[ipattern] == pattern_index) {
            return true;
        }
    }

    return false;
}
