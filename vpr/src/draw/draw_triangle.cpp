
#ifndef NO_GRAPHICS

#include <cmath>

#include "vtr_assert.h"
#include "draw_triangle.h"

#include "ezgl/graphics.hpp"

//To process key presses we need the X11 keysym definitions,
//which are unavailable when building with MINGW
#if defined(X11) && !defined(__MINGW32__)
#include <X11/keysym.h>
#endif

/**
 * Draws a small triangle, at a position along a line from 'start' to 'end',
 * at a CONSTANT SCREEN-PIXEL size (zoom-invariant).
 *
 * 'relative_position' [0., 1] defines the triangle's position relative to 'start'.
 *
 * A 'relative_position' of 0. draws the triangle centered at 'start'.
 * A 'relative_position' of 1. draws the triangle centered at 'end'.
 * Fractional values draw the triangle along the line.
 *
 * 'g' is the renderer object, 'start' and 'end' are the line segment points,
 * 'relative_position' is the position of the triangle along the line,
 * and 'arrow_size' is the size of the triangle in SCREEN PIXELS — the
 * triangle keeps the same pixel footprint at every zoom level.
 */
void draw_triangle_along_line_fixed_px(ezgl::renderer* g, ezgl::point2d start, ezgl::point2d end, float relative_position, float arrow_size) {
    VTR_ASSERT(relative_position >= 0. && relative_position <= 1.);
    float xdelta = end.x - start.x;
    float ydelta = end.y - start.y;

    float xtri = start.x + xdelta * relative_position;
    float ytri = start.y + ydelta * relative_position;

    draw_triangle_along_line_fixed_px(g, xtri, ytri, start.x, end.x, start.y, end.y, arrow_size);
}

void draw_triangle_along_line(ezgl::renderer* g, ezgl::point2d start, ezgl::point2d end, float relative_position, float arrow_size) {
    VTR_ASSERT(relative_position >= 0. && relative_position <= 1.);
    float xdelta = end.x - start.x;
    float ydelta = end.y - start.y;

    float xtri = start.x + xdelta * relative_position;
    float ytri = start.y + ydelta * relative_position;

    draw_triangle_along_line(g, xtri, ytri, start.x, end.x, start.y, end.y, arrow_size);
}

/* Draws a triangle with its center at loc, and of length & width
 * arrow_size, rotated such that it points in the direction
 * of the directed line segment start -> end.
 *
 * Constant SCREEN-PIXEL size (zoom-invariant).
 *
 * 'g' is the renderer object, 'loc' is the center of the triangle,
 * 'start' and 'end' are the line segment points, and 'arrow_size' is the
 * size of the triangle in SCREEN PIXELS.
 */
void draw_triangle_along_line_fixed_px(ezgl::renderer* g, ezgl::point2d loc, ezgl::point2d start, ezgl::point2d end, float arrow_size) {
    draw_triangle_along_line_fixed_px(g, loc.x, loc.y, start.x, end.x, start.y, end.y, arrow_size);
}

void draw_triangle_along_line(ezgl::renderer* g, ezgl::point2d loc, ezgl::point2d start, ezgl::point2d end, float arrow_size) {
    draw_triangle_along_line(g, loc.x, loc.y, start.x, end.x, start.y, end.y, arrow_size);
}

/**
 * Draws a triangle with its center at (xend, yend), and of length & width
 * arrow_size, rotated such that it points in the direction of the
 * directed line segment (x1, y1) -> (x2, y2).
 *
 * Constant SCREEN-PIXEL size (zoom-invariant). Hands the size off to the
 * renderer's fill_arrow_pointer_triangle(anchor, dir, size_px), which
 * keeps the on-screen extent identical at every zoom level — the RHI
 * backend uploads one GPU instance per call and synthesises the 3-vertex
 * triangle in a vertex shader at constant pixel size; the deferred Qt
 * painter backend records the world anchor + pixel offsets and replays
 * them through the current camera.
 *
 * Contrast with draw_triangle_along_line (no `_fixed_px`), which builds
 * the triangle in world units so its on-screen size scales with zoom.
 *
 * 'g' is the renderer object, 'xend' and 'yend' are the coordinates of the
 * triangle's center, 'x1' and 'x2' are the x-coordinates of the line
 * segment points, 'y1' and 'y2' are the y-coordinates of the line segment
 * points, 'arrow_size' is the size of the triangle in SCREEN PIXELS.
 */
void draw_triangle_along_line_fixed_px(ezgl::renderer* g, float xend, float yend, float x1, float x2, float y1, float y2, float arrow_size) {
    const float xdelta = x2 - x1;
    const float ydelta = y2 - y1;
    const float magnitude = std::sqrt(xdelta * xdelta + ydelta * ydelta);
    if (magnitude <= 0.0f)
        return;

    const float xunit = xdelta / magnitude;
    const float yunit = ydelta / magnitude;

    g->fill_arrow_pointer_triangle({xend, yend}, {xunit, yunit}, arrow_size);
}

void draw_triangle_along_line(ezgl::renderer* g, float xend, float yend, float x1, float x2, float y1, float y2, float arrow_size) {
    float switch_rad = arrow_size / 2.0f;
    float xdelta, ydelta;
    float magnitude;
    float xunit, yunit;
    float xbaseline, ybaseline;

    xdelta = x2 - x1;
    ydelta = y2 - y1;
    magnitude = sqrt(xdelta * xdelta + ydelta * ydelta);

    xunit = xdelta / magnitude;
    yunit = ydelta / magnitude;

    xbaseline = xend - xunit * switch_rad;
    ybaseline = yend - yunit * switch_rad;

    g->fill_triangle(
        {xend + xunit * switch_rad, yend + yunit * switch_rad},
        {xbaseline + yunit * switch_rad, ybaseline - xunit * switch_rad},
        {xbaseline - yunit * switch_rad, ybaseline + xunit * switch_rad});
}

#endif
