#pragma once

/**
 * @file setup_vib_utils.h
 * @brief This file contains the utility functions to build device grid
 * when the architecture described in the architecture file use versatile interconnect block (VIB).
 */

#include <vector>

#include "vpr_types.h"

/**
 * @brief Setup the VIB information. This function is called when the 
 * architecture described in the architecture file use versatile interconnect block (VIB).
 * 
 * @param physical_tile_types Physical tile types defined in the architecture file
 * @param switches Switches defined in the architecture file
 * @param segments The segments defined in the architecture file
 * @param vib_infs vector containing the VIB information which will be populated by this function
 */
void setup_vib_inf(const std::vector<t_physical_tile_type>& physical_tile_types,
                   const std::vector<t_arch_switch_inf>& switches,
                   const std::vector<t_segment_inf>& segments,
                   std::vector<VibInf>& vib_infs);
