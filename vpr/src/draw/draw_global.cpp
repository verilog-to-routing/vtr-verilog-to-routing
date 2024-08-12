/* This file defines variables accessor functions that are globally used
 * by all drawing files. The variables are defined at file scope but are
 * accessible through the accessor functions defined here. The purpose of
 * doing so is for cleaner coding style. Reducing the amount of global
 * variables will lessen the likelihood of multiple variables of the same
 * name being declared in different files, and thus will avoid compiler
 * error.
 *
 * Author: Mike Wang
 * Date: August 21, 2013
 */

#ifndef NO_GRAPHICS

#    include "draw_global.h"
#    include "draw_types.h"

/*************************** Variables Definition ***************************/

/* Global structure that stores state variables that control drawing and
 * highlighting. (Initializing some state variables for safety.) */
static t_draw_state draw_state;

/* Global variable for storing coordinates and dimensions of grid tiles
 * and logic blocks in the FPGA.
 */
static t_draw_coords draw_coords;

/**
 * @brief Stores a reference to a PlaceLocVars to be used in the graphics code.
 * @details This reference let us pass in a currently-being-optimized placement state,
 * rather than using the global placement state in placement context that is valid only once placement is done
 */
static std::optional<std::reference_wrapper<const BlkLocRegistry>> blk_loc_registry_ref;

/*********************** Accessor Subroutines Definition ********************/

/* This accessor function returns pointer to the global variable
 * draw_coords. Use this function to access draw_coords.
 */
t_draw_coords* get_draw_coords_vars() {
    return &draw_coords;
}

/* Use this function to access draw_state. */
t_draw_state* get_draw_state_vars() {
    return &draw_state;
}

void set_graphics_blk_loc_registry_ref(const BlkLocRegistry& blk_loc_registry) {
    blk_loc_registry_ref = std::ref(blk_loc_registry);
}

const BlkLocRegistry& get_graphics_blk_loc_registry_ref() {
    return blk_loc_registry_ref->get();
}

#endif // NO_GRAPHICS
