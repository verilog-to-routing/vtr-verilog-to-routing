#pragma once

/**
 * @file interposer_types.h
 * @brief This file contains types used for parsing interposer-related tags such as <interposer_cut> and <interdie_wire>
 * and converting that information into the device architecture-related data structures.
 */

#include <vector>
#include <string>
#include "grid_types.h"

/// @brief Enum for direction of an interposer cut.
enum class e_interposer_cut_type {
    HORZ,
    VERT
};

/**
 * @brief Struct containing information of interdire wires i.e. connections between the dies on an interposer
 */
struct t_interdie_wire_inf {
    std::string sg_name; ///< Name of the scatter-gather pattern to be used for the interdie connection
    std::string sg_link; ///< Name of the scatter-gather link to be used for the interdie connection
    /**
     * @brief 
     * Contains starting and ending point (both inclusive) of scatter-gather instantiations and the increment/distance between the instantiations.
     * offset_definition.repeat_expr is not relevant for interdie wires and is not set to anything or used.
     * 
     * Locations defined by this offset definition define the starting point or the gathering point of the SG pattern.
     * The end or scatter point of the SG pattern is defined by the sg_link.
     */
    t_grid_loc_spec offset_definition;
    int num; ///< Number of scatter-gather instantiations per switchblock location
};

/**
 * @brief Struct containing information of an interposer cut
 */
struct t_interposer_cut_inf {
    e_interposer_cut_type dim;                       ///< Axis of interposer cut location. The cut is perpendicular to this axis. This specifies the dimension of `loc`.
    int loc;                                         ///< Location of the cut on the grid. Locations start from zero and cuts will happen above or to the right of the tiles at location=loc.
    std::vector<t_interdie_wire_inf> interdie_wires; ///< Connectivity specification between the two sides of the cut.
};
