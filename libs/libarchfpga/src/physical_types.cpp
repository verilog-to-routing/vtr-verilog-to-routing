#include "physical_types.h"
#include "vtr_math.h"

static bool switch_type_is_buffered(SwitchType type);
static bool switch_type_is_configurable(SwitchType type);

//Ensure the constant has external linkage to avoid linking errors
constexpr int t_arch_switch_inf::UNDEFINED_FANIN;

SwitchType t_arch_switch_inf::type() const {
    return type_;
}

bool t_arch_switch_inf::buffered() const {
    return switch_type_is_buffered(type());
}

bool t_arch_switch_inf::configurable() const {
    return switch_type_is_configurable(type());
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

static bool switch_type_is_buffered(SwitchType type) {
    //Muxes and Tristates isolate thier input and output into
    //seperate DC connected sub-circuits
    return type == SwitchType::MUX 
        || type == SwitchType::TRISTATE;
}

static bool switch_type_is_configurable(SwitchType type) {
    //Muxes and Tristates isolate thier input and output into
    //seperate DC connected sub-circuits
    return !(   type == SwitchType::SHORT 
             || type == SwitchType::BUFFER);
}
