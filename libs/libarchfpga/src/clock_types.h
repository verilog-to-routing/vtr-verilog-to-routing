#ifndef CLOCK_TYPES_H
#define CLOCK_TYPES_H

#include <string>
#include <vector>

enum class e_clock_type {
    SPINE,
    RIB,
    H_TREE
};

struct t_metal_layer {
    float r_metal;
    float c_metal;
};

struct t_wire_repeat {
    std::string x;
    std::string y;
};

struct t_wire {
    std::string start;
    std::string end;
    std::string position;
};

struct t_clock_drive {
    std::string name;
    std::string offset;
    int arch_switch_idx;
};

struct t_clock_taps {
    std::string name;
    std::string offset;
    std::string increment;
};

struct t_clock_network_arch {
    std::string name;
    int num_inst;

    e_clock_type type;

    std::string metal_layer;
    t_wire wire;
    t_wire_repeat repeat;
    t_clock_drive drive;
    t_clock_taps tap;
};

struct t_clock_connection_arch {
    std::string from;
    std::string to;
    int arch_switch_idx;
    std::string locationx;
    std::string locationy;
    float fc;
};

#endif
