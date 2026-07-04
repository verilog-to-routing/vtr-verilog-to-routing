#pragma once
/**
 * @file draw_crit_path.h
 * 
 * @brief This file contains two drawing routines related to visualizing critical paths in a placed and / or routed design.
 * 
 * Critical Path visualization includes timing-edge flylines, time delay labels
 * and routed timing connections (available in a routed design only).
 * draw_crit_path() and draw_crit_path_elements() are tied to non-server mode (the main flow) and server mode, respectively.
 * The major differences are types of inputs (server mode can receive multiple critical paths from a VPR server
 * plus customized inputs to control the exact parts to draw, while non-server mode only fetches the single worst timing path
 * and does not customize what to draw) and the drawing style of delay labels (a label decluttering algorithm is implemented
 * for non-server mode, but server mode does not have that).
 */

#ifndef NO_GRAPHICS

#include <map>
#include <set>

#include "tatum/TimingConstraintsFwd.hpp"
#include "tatum/report/TimingPathCollector.hpp"

#include "ezgl/point.hpp"
#include "ezgl/graphics.hpp"

/**
 * @brief Draws critical path elements: edge flylines, time delay labels and routed connections, if they are on.
 * 
 * Note: the current code only supports drawing of time delay labels on edge flylines.
 * 
 * @param g Pointer to the ezgl::renderer object.
 */
void draw_crit_path(ezgl::renderer* g);

#ifndef NO_SERVER

/**
 * @brief Draw critical path elements.
 * 
 * This function draws critical path elements based on the provided timing paths
 * and indexes map. It is primarily used in server mode, where items are drawn upon request.
 *
 * @param paths The vector of TimingPath objects representing the critical paths.
 * @param indexes The map of sets, where the map keys are path indices in std::vector<tatum::TimingPath>, and each set contains the indices of the data_arrival_path elements ( @ref tatum::TimingPath ) to draw.
 * @param g Pointer to the ezgl::renderer object on which the elements will be drawn.
 */
void draw_crit_path_elements(const std::vector<tatum::TimingPath>& paths, const std::map<std::size_t, std::set<std::size_t>>& indexes, bool draw_crit_path_contour, ezgl::renderer* g);

#endif /* NO_SERVER */
#endif /* NO_GRAPHICS */
