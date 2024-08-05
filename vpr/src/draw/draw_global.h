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

#ifndef DRAW_GLOBAL_H
#define DRAW_GLOBAL_H

#ifndef NO_GRAPHICS

#    include "draw_types.h"

constexpr float DEFAULT_ARROW_SIZE = 0.3;

// a very small area, in (screen pixels)^2
// used for level of detail culling
#    define MIN_VISIBLE_AREA 3.0

t_draw_coords* get_draw_coords_vars();

t_draw_state* get_draw_state_vars();

/**
 * @brief Set the reference to placement location variable.
 *
 * During the placement stage, this reference should point to a local object
 * in the placement stage because the placement stage does not change the
 * global stage in place_ctx until the end of placement. After the placement is
 * done, the reference should point to the global state stored in place_ctx.
 *
 * @param blk_loc_registry The PlaceLocVars that the reference will point to.
 */
void set_graphics_blk_loc_registry_ref(const BlkLocRegistry& blk_loc_registry);

/**
 * @brief Returns the reference to placement block location variables.
 * @return A const reference to placement block location variables.
 */
const BlkLocRegistry& get_graphics_blk_loc_registry_ref();

#endif // NO_GRAPHICS

#endif
