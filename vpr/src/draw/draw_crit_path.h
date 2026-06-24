#pragma once

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

/**
 * @brief Calculate label positions that give the least number of overlaps and then draws all visible delay labels.
 *
 * @param path Timing path whose consecutive node pairs define the timing edges to place delay labels.
 * @param g Pointer to the ezgl::renderer object.
 */
void calculate_and_draw_labels(const tatum::TimingPath& path, ezgl::renderer* g);

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
