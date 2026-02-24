/**
 * @file
 * @brief Utility functions for routing statistics.
 *
 * This module contains helper functions extracted from stats.cpp, including:
 *   - Tap utilization statistics for routed wire segments
 *   - Channel occupancy helpers
 *   - Switch block count file I/O helpers
 *   - Wire length and bend statistics
 */

#pragma once

#include <string>
#include <vector>

#include "netlist.h"
#include "physical_types.h"

/**
 * @brief Prints tap utilization statistics for all routed wire segments.
 *
 * Assumes a unidirectional architecture; aborts with a warning if a BIDIR
 * wire is encountered.
 *
 * @param net_list    The netlist.
 * @param segment_inf Segment type information from the architecture.
 */
void print_tap_utilization(const Netlist<>& net_list,
                           const std::vector<t_segment_inf>& segment_inf);

/**
 * @brief Figures out maximum, minimum and average number of bends
 *        and net length in the routing, and prints the results.
 */
void length_and_bends_stats(const Netlist<>& net_list, bool is_flat);

/**
 * @brief Determines how many tracks are used in each channel and prints
 *        out statistics.  Also writes per-channel occupancy tables to files.
 */
void get_channel_occupancy_stats(const Netlist<>& net_list);

/// @brief Helper struct to parse switch block template ID keys.
struct SBKeyParts {
    std::string filename;
    int row;
    int col;
};

/**
 * @brief Parses a switch block key string to extract filename, row, and column.
 */
SBKeyParts parse_sb_key(const std::string& key);

/**
 * @brief Reads a CSV file and trims it, keeping only the header rows in full
 *        and the first few columns of all other rows.
 */
std::vector<std::vector<std::string>> read_and_trim_csv(const std::string& filepath);

/**
 * @brief Writes a 2D vector of strings to a CSV file.
 */
void write_csv(const std::string& filepath, const std::vector<std::vector<std::string>>& data);

/**
 * @brief Counts and returns the number of bends, wirelength, and number of routing
 *        resource segments in net inet's routing.
 */
void get_num_bends_and_length(ParentNetId inet, int* bends, int* length, int* segments, bool* is_absorbed_ptr);
