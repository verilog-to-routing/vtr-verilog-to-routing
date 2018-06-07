#include <cmath>
#include "vpr_types.h"

t_ext_pin_util_targets::t_ext_pin_util_targets(float default_in_util, float default_out_util) {
    defaults_.input_pin_util = default_in_util;
    defaults_.output_pin_util = default_out_util;
}

t_ext_pin_util t_ext_pin_util_targets::get_pin_util(std::string block_type_name) const {
    auto itr = overrides_.find(block_type_name);
    if (itr != overrides_.end()) {
        return itr->second;
    }
    return defaults_;
}

void t_ext_pin_util_targets::set_block_pin_util(std::string block_type_name, t_ext_pin_util target) {
    overrides_[block_type_name] = target;
}

void t_ext_pin_util_targets::set_default_pin_util(t_ext_pin_util default_target) {
    defaults_ = default_target;
}
