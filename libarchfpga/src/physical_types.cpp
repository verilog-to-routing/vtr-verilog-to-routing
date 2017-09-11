#include "physical_types.h"
#include "vtr_math.h"

//Ensure the constant has external linkage to avoid linking errors
constexpr int t_arch_switch_inf::UNDEFINED_FANIN;

float t_arch_switch_inf::Tdel(int fanin) const {
    if (fixed_Tdel()) {
        auto itr = Tdel_map.find(UNDEFINED_FANIN);
        VTR_ASSERT(itr != Tdel_map.end());
        return itr->second;
    } else {
        VTR_ASSERT(fanin > 0);
        return vtr::linear_interpolate_or_extrapolate(&Tdel_map, fanin); 
    }
}

bool t_arch_switch_inf::fixed_Tdel() const {
    return Tdel_map.size() == 1 && Tdel_map.count(UNDEFINED_FANIN);
}

void t_arch_switch_inf::set_Tdel(int fanin, float delay) {
    Tdel_map[fanin] = delay;
}
