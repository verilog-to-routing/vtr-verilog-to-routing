
#include "rr_switch.h"

e_switch_type t_rr_switch_inf::type() const {
    return type_;
}

bool t_rr_switch_inf::buffered() const {
    return switch_type_is_buffered(type());
}

bool t_rr_switch_inf::configurable() const {
    return switch_type_is_configurable(type());
}

bool t_rr_switch_inf::operator==(const t_rr_switch_inf& other) const {
    return R == other.R
           && Cin == other.Cin
           && Cout == other.Cout
           && Cinternal == other.Cinternal
           && Tdel == other.Tdel
           && mux_trans_size == other.mux_trans_size
           && buf_size == other.buf_size
           && power_buffer_type == other.power_buffer_type
           && power_buffer_size == other.power_buffer_size
           && intra_tile == other.intra_tile
           && type() == other.type();
}

std::size_t t_rr_switch_inf::Hasher::operator()(const t_rr_switch_inf& s) const {
    std::size_t hash_val = 0;

    auto hash_combine = [&hash_val](auto&& val) {
        hash_val ^= std::hash<std::decay_t<decltype(val)>>{}(val) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
    };

    hash_combine(s.R);
    hash_combine(s.Cin);
    hash_combine(s.Cout);
    hash_combine(s.Cinternal);
    hash_combine(s.Tdel);
    hash_combine(s.mux_trans_size);
    hash_combine(s.buf_size);
    hash_combine(static_cast<int>(s.power_buffer_type));
    hash_combine(s.power_buffer_size);
    hash_combine(s.intra_tile);
    hash_combine(static_cast<int>(s.type()));

    return hash_val;
}

void t_rr_switch_inf::set_type(e_switch_type type_val) {
    type_ = type_val;
}