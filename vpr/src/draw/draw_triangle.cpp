/* draw_triangle.cpp contains functions that draw triangles. Used for drawing arrows for showing switching in the routing,
 * direction of signals, flylines */
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
#include "draw_color.h"
#include "draw.h"
#include "draw_rr.h"
#include "draw_rr_edges.h"
#include "draw_toggle_functions.h"
#include "draw_triangle.h"
#include "draw_searchbar.h"
#include "draw_mux.h"
#include "read_xml_arch_file.h"
#include "draw_global.h"
#include "draw_basic.h"

#include "move_utils.h"

#ifdef VTR_ENABLE_DEBUG_LOGGING
#    include "move_utils.h"
#endif

#ifdef WIN32 /* For runtime tracking in WIN32. The clock() function defined in time.h will *
              * track CPU runtime.														   */
#    include <time.h>
#else /* For X11. The clock() function in time.h will not output correct time difference   *
       * for X11, because the graphics is processed by the Xserver rather than local CPU,  *
       * which means tracking CPU time will not be the same as the actual wall clock time. *
       * Thus, so use gettimeofday() in sys/time.h to track actual calendar time.          */
#    include <sys/time.h>
#endif

#ifndef NO_GRAPHICS

//To process key presses we need the X11 keysym definitions,
//which are unavailable when building with MINGW
#    if defined(X11) && !defined(__MINGW32__)
#        include <X11/keysym.h>
#    endif

/**
 * Draws a small triangle, at a position along a line from 'start' to 'end'.
 *
 * 'relative_position' [0., 1] defines the triangles position relative to 'start'.
 *
 * A 'relative_position' of 0. draws the triangle centered at 'start'.
 * A 'relative_position' of 1. draws the triangle centered at 'end'.
 * Fractional values draw the triangle along the line
 */
void draw_triangle_along_line(ezgl::renderer* g, ezgl::point2d start, ezgl::point2d end, float relative_position, float arrow_size) {
    VTR_ASSERT(relative_position >= 0. && relative_position <= 1.);
    float xdelta = end.x - start.x;
    float ydelta = end.y - start.y;

    float xtri = start.x + xdelta * relative_position;
    float ytri = start.y + ydelta * relative_position;

    draw_triangle_along_line(g, xtri, ytri, start.x, end.x, start.y, end.y,
                             arrow_size);
}

/* Draws a triangle with it's center at loc, and of length & width
 * arrow_size, rotated such that it points in the direction
 * of the directed line segment start -> end.
 */
void draw_triangle_along_line(ezgl::renderer* g, ezgl::point2d loc, ezgl::point2d start, ezgl::point2d end, float arrow_size) {
    draw_triangle_along_line(g, loc.x, loc.y, start.x, end.x, start.y, end.y,
                             arrow_size);
}

/**
 * Draws a triangle with it's center at (xend, yend), and of length & width
 * arrow_size, rotated such that it points in the direction
 * of the directed line segment (x1, y1) -> (x2, y2).
 *
 * Note that the parameters are in a strange order
 */

void draw_triangle_along_line(ezgl::renderer* g, float xend, float yend, float x1, float x2, float y1, float y2, float arrow_size) {
    float switch_rad = arrow_size / 2;
    float xdelta, ydelta;
    float magnitude;
    float xunit, yunit;
    float xbaseline, ybaseline;

    std::vector<ezgl::point2d> poly;

    xdelta = x2 - x1;
    ydelta = y2 - y1;
    magnitude = sqrt(xdelta * xdelta + ydelta * ydelta);

    xunit = xdelta / magnitude;
    yunit = ydelta / magnitude;

    poly.push_back({xend + xunit * switch_rad, yend + yunit * switch_rad});
    xbaseline = xend - xunit * switch_rad;
    ybaseline = yend - yunit * switch_rad;
    poly.push_back(
        {xbaseline + yunit * switch_rad, ybaseline - xunit * switch_rad});
    poly.push_back(
        {xbaseline - yunit * switch_rad, ybaseline + xunit * switch_rad});

    g->fill_poly(poly);
}

#endif
