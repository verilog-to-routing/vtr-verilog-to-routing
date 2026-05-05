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

// Default arrow-head size in SCREEN PIXELS. Used by draw_triangle_along_line
// which passes this through to renderer->fill_arrow_pointer_triangle, where
// the GPU shader expands it to a pixel-sized triangle at any zoom level.
//
// Historical note: previously this was a world-coord constant (0.3), divided
// by a clamped zoom heuristic at draw time. The renderer now owns the
// constant-pixel-size invariant, so this value is interpreted directly as
// pixels.
constexpr float DEFAULT_ARROW_SIZE = 10.0f;

// a very small area, in (screen pixels)^2
// used for level of detail culling
#define MIN_VISIBLE_AREA 3.0

t_draw_coords* get_draw_coords_vars();

t_draw_state* get_draw_state_vars();

#endif // NO_GRAPHICS
