#pragma once

#include "netlist.h"

/// @brief Per-interposer-cut routing statistics collected from the current routing.
struct t_interposer_cut_routing_stats {
    int num_nets_crossing = 0;        ///< Number of nets that cross this cut
    int num_used_wires = 0;           ///< Number of used routing wires that cross this cut
    int num_nets_crossing_inc = 0;    ///< Nets crossing in the incremental direction (relative to the net source)
    int num_nets_crossing_dec = 0;    ///< Nets crossing in the decremental direction (relative to the net source)
    int num_used_wires_inc = 0;       ///< Used incremental wires crossing
    int num_used_wires_dec = 0;       ///< Used decremental wires crossing
    int num_used_wires_bidir = 0;     ///< Used bidirectional wires crossing
};

/// @brief Prints routing statistics for interposer-based devices.
///
/// For each interposer cut, reports the number of nets and used wires crossing the cut.
/// Net direction counts are relative to the net source; wire direction counts use RR wire direction.
/// Does nothing if the device has no interposer cuts.
///
/// Both routing and an rr_graph must exist when this routine is called.
void print_interposer_routing_stats(const Netlist<>& net_list);
