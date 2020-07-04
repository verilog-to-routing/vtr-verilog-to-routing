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

#include "draw_global.h"
#include "draw_types.h"

/*************************** Variables Definition ***************************/

/* Global structure that stores state variables that control drawing and
 * highlighting. (Initializing some state variables for safety.) */
static t_draw_state draw_state;

/* Global variable for storing coordinates and dimensions of grid tiles
 * and logic blocks in the FPGA.
 */
static t_draw_coords draw_coords;

/* Global variable for storing pres_fac: present congestion cost factor */
static float pres_fac = 1.;

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

/* Use this function to access pres_fac. */
float get_draw_pres_fac() {
    return pres_fac;
}

/* Use this function to change pres_fac. */
void set_draw_pres_fac(float new_pres_fac) {
    pres_fac = new_pres_fac;
}

#endif // NO_GRAPHICS
