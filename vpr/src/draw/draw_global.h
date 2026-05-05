#pragma once
/**
 * @file draw_global.h
 * This file contains declaration of accessor functions that can be used to
 * retrieve global variables declared at filed scope inside draw_global.c.
 * Doing so could reduce the number of global variables in VPR and thus
 * reduced the likelihood of compiler error for declaration of multiple
 * variables with the same name.
 *
 * Author: Long Yu (Mike) Wang
 * Date: August 21, 2013
 */

#ifndef NO_GRAPHICS

#include "draw_types.h"

// Default arrow-head size, in WORLD UNITS. Used as the default
// `arrow_size` argument by both arrow-drawing families declared in
// draw_triangle.h:
//   * draw_triangle_along_line_fixed_px(...) interprets the value as
//     SCREEN PIXELS, handing it to renderer->fill_arrow_pointer_triangle
//     so the GPU/deferred painter keeps the on-screen size constant at
//     every zoom level.
//   * draw_triangle_along_line(...) interprets the value as WORLD
//     UNITS — the triangle is built in world coords, so its on-screen
//     size scales with camera zoom.
// The same numeric default works (roughly) for both because callers
// pick the variant that matches the visual behaviour they want; tune
// the per-call multiplier if a particular family of arrows needs to
// stand out.
constexpr float DEFAULT_ARROW_SIZE = 0.3f;

// a very small area, in (screen pixels)^2
// used for level of detail culling
#define MIN_VISIBLE_AREA 3.0

t_draw_coords* get_draw_coords_vars();

t_draw_state* get_draw_state_vars();

#endif // NO_GRAPHICS
