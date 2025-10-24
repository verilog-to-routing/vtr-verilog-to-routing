#pragma once

/**
 * @file clb2clb_directs.h
 * @brief This file contains the functions to parse the direct connections between two CLBs
 * and store them in a clb_to_clb_directs data structure.
 */

#include "physical_types.h"

/**
 * @brief A structure to store the direct connection between two CLBs
 */
struct t_clb_to_clb_directs {
    t_physical_tile_type_ptr from_clb_type;
    std::vector<size_t> from_sub_tiles;
    int from_clb_pin_start_index;
    int from_clb_pin_end_index;
    t_physical_tile_type_ptr to_clb_type;
    int to_clb_pin_start_index;
    int to_clb_pin_end_index;
    int switch_index; //The switch type used by this direct connection
};

/**
 * @brief Parse out which CLB pins should connect directly to which other CLB pins then store that in a clb_to_clb_directs data structure
 * This data structure supplements the the info in the "directs" data structure
 * 
 * @param directs The direct connections to parse
 * @param delayless_switch The switch index to use for delayless connections
 * @return A vector of clb_to_clb_directs structures
 */
std::vector<t_clb_to_clb_directs> alloc_and_load_clb_to_clb_directs(const std::vector<t_direct_inf>& directs, const int delayless_switch);
