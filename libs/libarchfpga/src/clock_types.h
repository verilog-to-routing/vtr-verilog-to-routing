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
    // Upper bound for how far this network repeats/tiles, in whichever axis is
    // its own tiling direction (x for a spine's column-tiling repeatx, y for a
    // rib's row-tiling repeaty). Defaults to the full device width/height,
    // preserving the historical "tile all the way to the device edge" behavior;
    // a smaller bound lets a rib/spine repeat within only part of the device
    // (e.g. one clock quadrant) instead of needing a network that spans the
    // whole device and is shared/cheated across quadrants.
    std::string end_x = "W";
    std::string end_y = "H";
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
    // Repeat this tap point across the grid every xincr/yincr switch boxes
    // (like the rib/spine tap xincr/yincr), instead of at just one location.
    // "0" (the default) means no repeat, i.e. a single point. Only meaningful
    // for TAP points; DRIVE points are always a single location.
    std::string xincr = "0";
    std::string yincr = "0";
    int arch_switch_idx = -1; // only set for DRIVE points
};

// One <switch_pattern> under a custom clock_switch_grid: which built-in
// permutation type applies, and where. Formula strings resolved later
// (setup_clocks.cpp) against the same W/H vars as the rest of this grid --
// same deferred-formula convention as startx/repeatx/chan_w below, so this
// still works with "auto" device layouts (general routing's own
// <switchblock_location> XY_SPECIFIED explicitly rejects "auto" layouts;
// clock networks don't need that restriction). Fully independent of general
// routing's <switchblocklist>/<switch_block type="custom">: different XML
// location, different struct, no shared namespace.
struct t_clock_switch_pattern {
    std::string name;
    e_switch_block_type switch_block_type; // WILTON/SUBSET/UNIVERSAL/FULL/CUSTOM
    e_sb_location location = e_sb_location::E_EVERYWHERE;

    // Only meaningful when location == E_XY_SPECIFIED.
    std::string x, y; // exact location; empty means "use the region below"
    std::string startx = "0", endx = "W-1", repeatx = "0", incrx = "1";
    std::string starty = "0", endy = "H-1", repeaty = "0", incry = "1";

    // Only meaningful when switch_block_type == CUSTOM: turn permutation
    // formulas parsed from this pattern's <switchfuncs> child, reusing
    // general routing's own <switchblock> grammar/types verbatim (see
    // parse_switchblocks.h's read_sb_switchfuncs/t_permutation_map). Formulas
    // stay as strings here -- t/W are only known per-track at RR-graph build
    // time, same as general routing.
    t_permutation_map permutation_map;
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
    // original minimal implementation. CUSTOM picks a per-location built-in
    // type from switch_patterns below (see t_clock_switch_pattern) instead of
    // a single type for the whole grid.
    e_switch_block_type switch_block_type = e_switch_block_type::FULL;

    // Only populated when switch_block_type == CUSTOM. Matched in list order;
    // first match wins.
    std::vector<t_clock_switch_pattern> switch_patterns;

    // Wire length, in switch-box hops (not tiles), i.e. how many repeatx/repeaty
    // pitches a hop wire spans before terminating at a switch box. Defaults to
    // "1", matching the original one-hop-per-switch-box implementation. Expressed
    // in hop units (rather than tiles) so it stays independent of repeatx/repeaty:
    // changing the switch-box pitch doesn't require also rescaling length to keep
    // the same topology.
    std::string length = "1";

    // Whether the grid's hop wires are BI_DIRECTIONAL (one node per track, entered
    // and exited from either end -- the original/default behavior) or UNI_DIRECTIONAL
    // (each track flows one way, like general routing's unidirectional segments).
    // Unidirectional requires an even chan_w (half the tracks INC, half DEC).
    e_directionality directionality = BI_DIRECTIONAL;

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
