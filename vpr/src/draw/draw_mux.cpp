/*draw_mux.cpp contains all functions that draw muxes.*/
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
#include "draw_basic.h"
#include "draw_toggle_functions.h"
#include "draw_triangle.h"
#include "draw_searchbar.h"
#include "draw_mux.h"
#include "read_xml_arch_file.h"
#include "draw_global.h"

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

//#define TIME_DRAWSCREEN /* Enable if want to track runtime for drawscreen() */

//Draws a mux, height/width define the bounding box, scale [0.,1.] controls the slope of the muxes sides
ezgl::rectangle draw_mux(ezgl::point2d origin, e_side orientation, float height, float width, float scale, ezgl::renderer* g) {
    std::vector<ezgl::point2d> mux_polygon;

    switch (orientation) {
        case TOP:
            //Clock-wise from bottom left
            mux_polygon.push_back({origin.x - height / 2, origin.y - width / 2});
            mux_polygon.push_back(
                {origin.x - (scale * height) / 2, origin.y + width / 2});
            mux_polygon.push_back(
                {origin.x + (scale * height) / 2, origin.y + width / 2});
            mux_polygon.push_back({origin.x + height / 2, origin.y - width / 2});
            break;
        case BOTTOM:
            //Clock-wise from bottom left
            mux_polygon.push_back(
                {origin.x - (scale * height) / 2, origin.y - width / 2});
            mux_polygon.push_back({origin.x - height / 2, origin.y + width / 2});
            mux_polygon.push_back({origin.x + height / 2, origin.y + width / 2});
            mux_polygon.push_back(
                {origin.x + (scale * height) / 2, origin.y - width / 2});
            break;
        case LEFT:
            //Clock-wise from bottom left
            mux_polygon.push_back(
                {origin.x - width / 2, origin.y - (scale * height) / 2});
            mux_polygon.push_back(
                {origin.x - width / 2, origin.y + (scale * height) / 2});
            mux_polygon.push_back({origin.x + width / 2, origin.y + height / 2});
            mux_polygon.push_back({origin.x + width / 2, origin.y - height / 2});
            break;
        case RIGHT:
            //Clock-wise from bottom left
            mux_polygon.push_back({origin.x - width / 2, origin.y - height / 2});
            mux_polygon.push_back({origin.x - width / 2, origin.y + height / 2});
            mux_polygon.push_back(
                {origin.x + width / 2, origin.y + (scale * height) / 2});
            mux_polygon.push_back(
                {origin.x + width / 2, origin.y - (scale * height) / 2});
            break;

        default:
            VTR_ASSERT_MSG(false, "Unrecognized orientation");
    }
    g->fill_poly(mux_polygon);

    ezgl::point2d min((float)mux_polygon[0].x, (float)mux_polygon[0].y);
    ezgl::point2d max((float)mux_polygon[0].x, (float)mux_polygon[0].y);
    for (const auto& point : mux_polygon) {
        min.x = std::min((float)min.x, (float)point.x);
        min.y = std::min((float)min.y, (float)point.y);
        max.x = std::max((float)max.x, (float)point.x);
        max.y = std::max((float)max.y, (float)point.y);
    }

    return ezgl::rectangle(min, max);
}

/* Draws a mux with width = height * 0.4 and scale (slope of the muxes sides) = 0.6.
 * Takes in point of origin, orientation, height and renderer.
 */
ezgl::rectangle draw_mux(ezgl::point2d origin, e_side orientation, float height, ezgl::renderer* g) {
    return draw_mux(origin, orientation, height, 0.4 * height, 0.6, g);
}

/* Draws a mux with width = height * 0.4 and scale (slope of the muxes sides) = 0.6, labelled with its size.
 * Takes in point of origin, orientation, height, mux size and renderer.
 */
void draw_mux_with_size(ezgl::point2d origin, e_side orientation, float height, int size, ezgl::renderer* g) {
    g->set_color(ezgl::YELLOW);
    auto bounds = draw_mux(origin, orientation, height, g);

    g->set_color(ezgl::BLACK);
    g->draw_text(bounds.center(), std::to_string(size), bounds.width(),
                 bounds.height());
}

#endif
