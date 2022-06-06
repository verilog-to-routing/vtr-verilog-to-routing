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
#    include "route_util.h"
#    include "place_macro.h"
#    include "buttons.h"

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
