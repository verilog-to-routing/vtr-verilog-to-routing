/**
 * @file draw_triangle.h
 *
 * draw_triangle.cpp contains functions that draw triangles. Used for drawing arrows for showing switching in the routing,
 * direction of signals, flylines 
 */

#ifndef DRAW_TRIANGLE_H
#define DRAW_TRIANGLE_H

#include <cstdio>
#include <cfloat>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <array>
#include <iostream>

#include "vtr_assert.h"
#include "vtr_ndoffsetmatrix.h"
#include "vtr_memory.h"
#include "vtr_log.h"
#include "vtr_color_map.h"
#include "vtr_path.h"

#include "vpr_utils.h"
#include "vpr_error.h"

#include "globals.h"

#include "move_utils.h"

#ifndef NO_GRAPHICS

#    include "draw_global.h"

#    include "ezgl/point.hpp"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"
#    include "draw_color.h"
#    include "search_bar.h"
#    include "draw_debug.h"
#    include "manual_moves.h"

#    include "rr_graph.h"
#    include "route_utilization.h"
#    include "place_macro.h"
#    include "buttons.h"

/**
 * Retrieves the current zoom level based on the visible world and screen dimensions.
 * The zoom level is calculated as the ratio of the visible world width to the visible screen width.
 * For some features which should be scaled when the user zooms in or out (e.g triangle arrows on the critical path display),
 * this function finds the corresponding scaling factor.
 * A value > 1 means the triangles will be drawn
 *
 * @param g The renderer object.
 * @param min_zoom The minimum allowed zoom level scaling factor (0.4 by default to prevent objects from getting bigger and bigger as user zooms out).
 * @param max_zoom The maximum allowed zoom level scaling factor (2 by default to prevent objects from becoming too small to view beyond a certain zoom level).
 * @return The calculated zoom level as a scaling factor.
 */
double get_scaling_factor_from_zoom(ezgl::renderer* g);
/*
 * Draws a small triangle, at a position along a line from 'start' to 'end'.
 * 'relative_position' [0., 1] defines the triangles position relative to 'start'.
 * A 'relative_position' of 0. draws the triangle centered at 'start'.
 * A 'relative_position' of 1. draws the triangle centered at 'end'.
 * Fractional values draw the triangle along the line
 */
void draw_triangle_along_line(ezgl::renderer* g, ezgl::point2d start, ezgl::point2d end, float relative_position = 1., float arrow_size = DEFAULT_ARROW_SIZE);

/* Draws a triangle with it's center at loc, and of length & width arrow_size,
 * rotated such that it points in the direction of the directed line segment start -> end.
 */
void draw_triangle_along_line(ezgl::renderer* g, ezgl::point2d loc, ezgl::point2d start, ezgl::point2d end, float arrow_size = DEFAULT_ARROW_SIZE);

/**
 * Draws a triangle with it's center at (xend, yend), and of length & width arrow_size,
 * rotated such that it points in the direction of the directed line segment (x1, y1) -> (x2, y2).
 * Note parameter order.
 */
void draw_triangle_along_line(ezgl::renderer* g, float xend, float yend, float x1, float x2, float y1, float y2, float arrow_size = DEFAULT_ARROW_SIZE);

#endif /* NO_GRAPHICS */
#endif /* DRAW_TRIANGLE_H */
