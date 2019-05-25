/* This file contains declaration of accessor functions that can be used to
 * retrieve global variables declared at filed scope inside draw_global.c.
 * Doing so could reduce the number of global variables in VPR and thus
 * reduced the likelihood of compiler error for declaration of multiple
 * variables with the same name.
 *
 * Author: Long Yu (Mike) Wang
 * Date: August 21, 2013
 */

#ifndef DRAW_GLOBAL_H
#define DRAW_GLOBAL_H

#include "draw_types.h"

#define MAX_BLOCK_COLOURS 5

constexpr float DEFAULT_ARROW_SIZE = 0.3;

// a very small area, in (screen pixels)^2
// used for level of detail culling
#define MIN_VISIBLE_AREA 3.0

t_draw_coords* get_draw_coords_vars();
t_draw_state* get_draw_state_vars();

#endif
