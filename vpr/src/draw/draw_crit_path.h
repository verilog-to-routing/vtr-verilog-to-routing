#pragma once
/**
 * @file draw_crit_path.h
 * 
 * @brief This file contains two drawing routines related to visualizing critical paths in a placed and / or routed design.
 * 
 * Critical Path visualization includes timing-edge flylines, time delay labels and routed timing connections.
 * draw_crit_path() and draw_crit_path_elements() are tied to non-server mode (the main flow) and server mode, respectively.
 * Both modes support the drawing of multiple critical paths. In non-server mode, the user can use a UI widget to set the number of paths.
 * In server mode, the user can use a VPR server to do it. They mainly differ in drawing flexibility (non-server mode draws
 * the entire critical paths, while server mode can receive customized inputs from the server to control what exactly to draw)
 * and the drawing style of delay labels (a label decluttering algorithm is implemented in non-server mode, but server mode does not have that).
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
 * Note: the current code only supports drawing of time delay labels on edge flylines, not on routed connections.
 * 
 * Note: If multiple critical paths are drawn, and some of them share the same timing edge segments, there will be repeated
 * drawings of the corresponding edge flylines and routed connections, because they do not negatively affect visual aesthetics,
 * and running a repetition detection logic would be unnecessary. However, for delay labels, having multiple of them
 * surrounding the same flyline segment would be very confusing, and so a repetition detection logic is implemented for them.
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
