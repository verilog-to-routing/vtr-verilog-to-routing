#pragma once
/**
 * @file draw_triangle.h
 *
 * Helpers for drawing arrow-head triangles along a directed line segment.
 * Used to indicate signal direction in the routing graph, intra-block
 * connectivity, and critical-path flylines.
 *
 * Two families of helpers live here, distinguished by how their size
 * reacts to camera zoom:
 *
 *   draw_triangle_along_line_fixed_px(...)
 *       Arrow size is in SCREEN PIXELS and stays constant at every zoom
 *       level — zooming in/out makes the line and its endpoints move
 *       apart on screen, but the arrow head keeps the same pixel
 *       footprint. Implemented by handing the size off to the renderer's
 *       fill_arrow_pointer_triangle, which expands it on the GPU
 *       (RHI backend) or in a deferred pixel-space replay (Qt painter
 *       backend) so the on-screen extent never drifts.
 *       Best for cases where the arrow is purely a UI marker that should
 *       remain readable at any zoom (e.g. critical-path flylines).
 *
 *   draw_triangle_along_line(...)
 *       Arrow size is in WORLD UNITS — the triangle is built in world
 *       coordinates and projected through the camera, so its on-screen
 *       size scales with zoom (bigger on zoom-in, smaller on zoom-out).
 *       Best when the arrow should feel "attached" to the underlying
 *       geometry it annotates (e.g. RR-graph switch-point arrows, intra-
 *       block atom-net arrows), matching the legacy GTK/world-coord look.
 */

#ifndef NO_GRAPHICS

#include <cstdio>
#include <cfloat>
#include <cstring>
#include <cmath>

#include "draw_global.h"

#include "ezgl/point.hpp"
#include "ezgl/graphics.hpp"

/*
 * Draws a small triangle, at a position along a line from 'start' to 'end'.
 * 'relative_position' [0., 1] defines the triangles position relative to 'start'.
 * A 'relative_position' of 0. draws the triangle centered at 'start'.
 * A 'relative_position' of 1. draws the triangle centered at 'end'.
 * Fractional values draw the triangle along the line.
 *
 * `_fixed_px`: arrow_size is in SCREEN PIXELS, zoom-invariant.
 * (Untagged variant: arrow_size is in WORLD UNITS, scales with zoom.)
 */
void draw_triangle_along_line_fixed_px(ezgl::renderer* g, ezgl::point2d start, ezgl::point2d end, float relative_position = 1., float arrow_size = DEFAULT_ARROW_SIZE);
void draw_triangle_along_line(ezgl::renderer* g, ezgl::point2d start, ezgl::point2d end, float relative_position = 1., float arrow_size = DEFAULT_ARROW_SIZE);

/* Draws a triangle with it's center at loc, and of length & width arrow_size,
 * rotated such that it points in the direction of the directed line segment start -> end.
 *
 * `_fixed_px`: arrow_size is in SCREEN PIXELS, zoom-invariant.
 */
void draw_triangle_along_line_fixed_px(ezgl::renderer* g, ezgl::point2d loc, ezgl::point2d start, ezgl::point2d end, float arrow_size = DEFAULT_ARROW_SIZE);
void draw_triangle_along_line(ezgl::renderer* g, ezgl::point2d loc, ezgl::point2d start, ezgl::point2d end, float arrow_size = DEFAULT_ARROW_SIZE);

/**
 * Draws a triangle with it's center at (xend, yend), and of length & width arrow_size,
 * rotated such that it points in the direction of the directed line segment (x1, y1) -> (x2, y2).
 * Note parameter order.
 *
 * `_fixed_px`: arrow_size is in SCREEN PIXELS, zoom-invariant.
 */
void draw_triangle_along_line_fixed_px(ezgl::renderer* g, float xend, float yend, float x1, float x2, float y1, float y2, float arrow_size = DEFAULT_ARROW_SIZE);
void draw_triangle_along_line(ezgl::renderer* g, float xend, float yend, float x1, float x2, float y1, float y2, float arrow_size = DEFAULT_ARROW_SIZE);

#endif /* NO_GRAPHICS */
