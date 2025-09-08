#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include "vtr_vector.h"

enum class e_interposer_cut_dim {
    X,
    Y
};

inline const std::unordered_map<char, e_interposer_cut_dim> CHAR_INTERPOSER_DIM_MAP = {
    {'X', e_interposer_cut_dim::X},
    {'x', e_interposer_cut_dim::X},
    {'Y', e_interposer_cut_dim::Y},
    {'y', e_interposer_cut_dim::Y}};

struct t_interdie_wire_inf {
    std::string sg_name;
    std::string sg_offset;
    int offset_start;
    int offset_end;
    int offset_repeat;
    int offset_incr;
    int offset_num;
};

struct t_interposer_cut_inf {
    e_interposer_cut_dim dim; // find an enum with x and y
    int loc; // handle expressions
    std::vector<t_interdie_wire_inf> interdie_wires;
};

