#pragma once

#include <string>
#include <vector>

#include "switchblock_types.h"

enum class e_clock_type {
    SPINE,
    RIB,
    H_TREE,
    SWITCH_GRID
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

enum class e_clock_switch_grid_point_type {
    DRIVE,
    TAP
};

// A single <switch_point> entry within a <clock_switch_grid>. Unlike the rib/spine
// drive/tap, offsets are 2D (a switch box location on the grid) and multiple drive
// and/or tap points are allowed.
struct t_clock_switch_grid_point {
    std::string name;
    e_clock_switch_grid_point_type type;
    std::string xoffset;
    std::string yoffset;
    int arch_switch_idx = -1; // only set for DRIVE points
};

struct t_clock_switch_grid_arch {
    std::string metal_layer;
    std::string startx;
    std::string starty;
    std::string repeatx;
    std::string repeaty;
    std::string chan_w;
    std::string switch_name;
    int arch_switch_idx = -1;

    // How the wires incident to each switch box connect to one another.
    // Defaults to FULL (every incident wire mutually reachable), matching the
    // original minimal implementation. CUSTOM is not yet supported for clock
    // networks.
    e_switch_block_type switch_block_type = e_switch_block_type::FULL;

    std::vector<t_clock_switch_grid_point> switch_points;
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

    t_clock_switch_grid_arch switch_grid;
};

struct t_clock_connection_arch {
    std::string from;
    std::string to;
    int arch_switch_idx;
    std::string locationx;
    std::string locationy;
    float fc;
};
