/**
 * @file draw_floorplanning.h
 *
 * draw_floorplanning.cpp contains all the functions pertaining to drawing
 * floorplanning constraints provided to VPR by the user.
 */
#ifndef DRAW_FLOORPLANNING_H
#define DRAW_FLOORPLANNING_H

#include "globals.h"

#ifndef NO_GRAPHICS

#    include "draw_global.h"

#    include "ezgl/point.hpp"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"

/* Highlights partitions. */
void highlight_regions(ezgl::renderer* g);

/* Function to draw all constrained primitives */
void draw_constrained_atoms(ezgl::renderer* g);

#endif /*NO_GRAPHICS*/

#endif /*DRAW_FLOORPLANNING_H*/
