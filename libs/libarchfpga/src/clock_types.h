#pragma once

#include <string>
#include <vector>

#include "switchblock_types.h"

/// @brief The topology of a dedicated clock network, as declared by a <clock_network>'s type attribute.
enum class e_clock_type {
    SPINE,      ///< A single horizontal wire, tapped by vertical spines (see e_clock_type::RIB below).
    RIB,        ///< A single vertical wire, tapped from/tapping into horizontal spines.
    H_TREE,     ///< Not yet implemented.
    SWITCH_GRID ///< A grid of clock switch boxes; see t_clock_switch_grid_arch.
};

/// @brief Metal layer electrical properties for a clock network's wires.
struct t_metal_layer {
    float r_metal; ///< Resistance per unit length.
    float c_metal; ///< Capacitance per unit length.
};

/// @brief How a rib/spine's wire repeats (tiles) across the device.
struct t_wire_repeat {
    std::string x; ///< Repeat pitch in the x direction.
    std::string y; ///< Repeat pitch in the y direction.

    /// @brief Upper bound for how far this network repeats, in whichever axis is its own
    /// tiling direction (x for a spine's column-tiling repeatx, y for a rib's row-tiling
    /// repeaty). Defaults to the full device width/height, preserving the "tile all the way
    /// to the device edge" behavior; a smaller bound lets a rib/spine repeat within only part
    /// of the device (e.g. one clock quadrant) instead of needing a network that spans the
    /// whole device and is shared/cheated across quadrants.
    std::string end_x = "W";
    std::string end_y = "H";
};

/// @brief The extent of a single rib/spine wire segment.
struct t_wire {
    std::string start;    ///< Start coordinate along the wire's own axis.
    std::string end;      ///< End coordinate along the wire's own axis.
    std::string position; ///< Coordinate along the wire's perpendicular axis.
};

/// @brief Where and how a rib/spine is driven from another clock network.
struct t_clock_drive {
    std::string name;    ///< Name of the switch point other networks connect to when driving this one.
    std::string offset;  ///< Coordinate, along the wire's own axis, of the drive point.
    int arch_switch_idx; ///< Index into the architecture's switch list of the driving switch.
};

/// @brief Where and how often a rib/spine exposes tap points to other clock networks.
struct t_clock_taps {
    std::string name;      ///< Name of the switch point other networks tap into.
    std::string offset;    ///< Coordinate, along the wire's own axis, of the first tap.
    std::string increment; ///< Spacing between repeated taps; empty/absent means a single tap.
};

/// @brief Whether a <switch_point> in a <clock_switch_grid> is a drive or a tap point.
enum class e_clock_switch_grid_point_type {
    DRIVE,
    TAP
};

/// @brief A single <switch_point> entry within a <clock_switch_grid>. Unlike the rib/spine
/// drive/tap, offsets are 2D (a switch box location on the grid) and multiple drive and/or
/// tap points are allowed.
struct t_clock_switch_grid_point {
    std::string name;                    ///< Name other clock connections reference this point by.
    e_clock_switch_grid_point_type type; ///< DRIVE or TAP.
    std::string xoffset;                 ///< Switch box column this point sits at.
    std::string yoffset;                 ///< Switch box row this point sits at.

    /// @brief Repeat this tap point across the grid every xincr/yincr switch boxes (like the
    /// rib/spine tap xincr/yincr), instead of at just one location. "0" (the default) means no
    /// repeat, i.e. a single point. Only meaningful for TAP points; DRIVE points are always a
    /// single location.
    std::string xincr = "0";
    std::string yincr = "0";

    int arch_switch_idx = -1; ///< Index into the architecture's switch list; only set for DRIVE points.
};

/// @brief One <switch_pattern> under a custom clock_switch_grid: which built-in switch-block
/// permutation type applies, and where. Formula strings are resolved later (setup_clocks.cpp)
/// against the same W/H vars as the rest of this grid. This is the same deferred-formula convention as
/// startx/repeatx/chan_w below, so this still works with "auto" device layouts (general
/// routing's own <switchblock_location> XY_SPECIFIED explicitly rejects "auto" layouts; clock
/// networks don't need that restriction). Fully independent of general routing's
/// <switchblocklist>/<switch_block type="custom">: different XML location, different struct,
/// no shared namespace.
struct t_clock_switch_pattern {
    std::string name;                                     ///< Name for error messages; not otherwise referenced.
    e_switch_block_type switch_block_type;                ///< WILTON/SUBSET/UNIVERSAL/FULL/CUSTOM.
    e_sb_location location = e_sb_location::E_EVERYWHERE; ///< Which switch boxes this pattern applies to.

    // Only meaningful when location == E_XY_SPECIFIED.
    std::string x, y;                                                   ///< Exact location; empty means "use the region below".
    std::string startx = "0", endx = "W-1", repeatx = "0", incrx = "1"; ///< X region, when location == E_XY_SPECIFIED and x/y are empty.
    std::string starty = "0", endy = "H-1", repeaty = "0", incry = "1"; ///< Y region, when location == E_XY_SPECIFIED and x/y are empty.

    /// @brief Turn permutation formulas parsed from this pattern's <switchfuncs> child, reusing
    /// general routing's own <switchblock> grammar/types verbatim (see parse_switchblocks.h's
    /// read_sb_switchfuncs/t_permutation_map). Formulas stay as strings here; t/W are only
    /// known per-track at RR-graph build time, same as general routing. Only meaningful when
    /// switch_block_type == CUSTOM.
    t_permutation_map permutation_map;
};

/// @brief Architecture description of a grid of clock switch boxes: at every
/// (repeatx, repeaty)-spaced location, clock wires connect to their adjacent switch boxes'
/// wires according to switch_block_type (or switch_patterns, if CUSTOM).
struct t_clock_switch_grid_arch {
    std::string metal_layer;  ///< Name of the metal layer this grid's wires are drawn on.
    std::string startx;       ///< X coordinate of the grid's first switch box.
    std::string starty;       ///< Y coordinate of the grid's first switch box.
    std::string repeatx;      ///< Switch box column pitch.
    std::string repeaty;      ///< Switch box row pitch.
    std::string chan_w;       ///< Number of tracks per inter-switch-box wire segment.
    std::string switch_name;  ///< Name of the switch used for wire-to-wire connections within a switch box.
    int arch_switch_idx = -1; ///< Index into the architecture's switch list, resolved from switch_name.

    /// @brief How the wires incident to each switch box connect to one another. Defaults to
    /// FULL (every incident wire mutually reachable), matching the original minimal
    /// implementation. CUSTOM picks a per-location built-in type from switch_patterns below
    /// instead of a single type for the whole grid.
    e_switch_block_type switch_block_type = e_switch_block_type::FULL;

    /// @brief Only populated when switch_block_type == CUSTOM. Matched in list order; first
    /// match wins.
    std::vector<t_clock_switch_pattern> switch_patterns;

    /// @brief Wire length, in switch-box hops (not tiles), i.e. how many repeatx/repeaty
    /// pitches a hop wire spans before terminating at a switch box. Defaults to "1", matching
    /// the original one-hop-per-switch-box implementation. Expressed in hop units (rather than
    /// tiles) so it stays independent of repeatx/repeaty: changing the switch-box pitch doesn't
    /// require also rescaling length to keep the same topology.
    std::string length = "1";

    /// @brief Whether the grid's hop wires are BI_DIRECTIONAL (one node per track, entered and
    /// exited from either end; the original/default behavior) or UNI_DIRECTIONAL (each track
    /// flows one way, like general routing's unidirectional segments). Unidirectional requires
    /// an even chan_w (half the tracks INC, half DEC).
    e_directionality directionality = BI_DIRECTIONAL;

    std::vector<t_clock_switch_grid_point> switch_points; ///< This grid's <switch_point> drive/tap entries.
};

/// @brief Architecture description of one <clock_network>, as parsed from the arch XML.
struct t_clock_network_arch {
    std::string name; ///< Unique name, referenced by <clock_routing> connections.
    int num_inst;     ///< Number of instances of this network to create (e.g. one per clock quadrant).

    e_clock_type type; ///< SPINE, RIB, H_TREE, or SWITCH_GRID; determines which of the fields below apply.

    std::string metal_layer; ///< Name of the metal layer this network's wire is drawn on. Unused when type == SWITCH_GRID.
    t_wire wire;             ///< Wire extent. Unused when type == SWITCH_GRID.
    t_wire_repeat repeat;    ///< Repeat/tiling parameters. Unused when type == SWITCH_GRID.
    t_clock_drive drive;     ///< Drive point. Unused when type == SWITCH_GRID.
    t_clock_taps tap;        ///< Tap point(s). Unused when type == SWITCH_GRID.

    t_clock_switch_grid_arch switch_grid; ///< Switch grid parameters; only used when type == SWITCH_GRID.
};

/// @brief Architecture description of one <clock_routing> connection, wiring a clock network's
/// tap/drive point either to/from general-purpose routing or to/from another clock network.
struct t_clock_connection_arch {
    std::string from;      ///< Source: a clock network's switch point name, or a routing/tile pin reference.
    std::string to;        ///< Destination: a clock network's switch point name, or a routing/tile pin reference.
    int arch_switch_idx;   ///< Index into the architecture's switch list of the connecting switch.
    std::string locationx; ///< X coordinate this connection is made at.
    std::string locationy; ///< Y coordinate this connection is made at.
    float fc;              ///< Fraction of source tracks/pins each destination connects to.
};
