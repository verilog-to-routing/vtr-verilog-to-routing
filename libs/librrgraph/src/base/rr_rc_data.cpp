#include "rr_rc_data.h"

#include <bit>

/// @brief Bit pattern of a float, with negative zero folded into positive zero so equal floats give equal keys.
static uint32_t float_key(float value);

t_rr_rc_data::t_rr_rc_data(float Rval, float Cval) noexcept
    : R(Rval)
    , C(Cval) {}

NodeRCIndex RRRCData::find_create(float R, float C) {
    uint64_t key = (uint64_t(float_key(R)) << 32) | float_key(C);

    auto [itr, inserted] = index_.try_emplace(key, NodeRCIndex(values_.size()));
    if (inserted) {
        values_.emplace_back(R, C);
    }
    return itr->second;
}

static uint32_t float_key(float value) {
    if (value == 0.0f) {
        value = 0.0f;
    }
    return std::bit_cast<uint32_t>(value);
}
