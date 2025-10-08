#pragma once
/**
 * @file draw_mux.h
 * 
 * This file contains all functions related to drawing muxes
 */

#ifndef NO_GRAPHICS

#include "ezgl/point.hpp"
#include "ezgl/graphics.hpp"
#include "physical_types.h"

/**
 * @brief Draws a mux with width = height * 0.4 and scale (slope of the muxes sides) = 0.6, labelled with its size.
 * Takes in point of origin, orientation, height, mux size and renderer.
 * Also takes in transparency factor, based on the transparency of the layer the mux is to be drawn on
 * (0 is opaque and 255 is transparent).
 */
void draw_mux_with_size(ezgl::point2d origin, e_side orientation, float height, int size, int transparency_factor, ezgl::renderer* g);

/* Draws a mux with width = height * 0.4 and scale (slope of the muxes sides) = 0.6.
 * Takes in point of origin, orientation, height and renderer.
 */
ezgl::rectangle draw_mux(ezgl::point2d origin, e_side orientation, float height, ezgl::renderer* g);

/* Draws a mux, height/width define the bounding box, scale [0.,1.] controls the slope of the muxes sides */
ezgl::rectangle draw_mux(ezgl::point2d origin, e_side orientation, float height, float width, float height_scale, ezgl::renderer* g);

#endif /* NO_GRAPHICS */
