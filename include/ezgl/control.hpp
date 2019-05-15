/*
 * Copyright 2019 University of Toronto
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Authors: Mario Badr, Sameh Attia and Tanner Young-Schultz
 */

#ifndef EZGL_CONTROL_HPP
#define EZGL_CONTROL_HPP

#include "ezgl/point.hpp"
#include "ezgl/rectangle.hpp"

namespace ezgl {

/**** Functions to manipulate what is visible on the screen; used by ezgl's predefined buttons. ****/
/**** Application code does not have to ever call these functions.                              ****/

class canvas;

/**
 * Zoom in on the center of the currently visible world.
 */
void zoom_in(canvas *cnv, double zoom_factor);

/**
 * Zoom out from the center of the currently visible world.
 */
void zoom_out(canvas *cnv, double zoom_factor);

/**
 * Zoom in on a specific point in the GTK widget.
 */
void zoom_in(canvas *cnv, point2d zoom_point, double zoom_factor);

/**
 * Zoom out from a specific point in GTK widget.
 */
void zoom_out(canvas *cnv, point2d zoom_point, double zoom_factor);

/**
 * Zoom in or out to fit an exact region of the world.
 */
void zoom_fit(canvas *cnv, rectangle region);

/**
 * Translate by delta x and delta y (dx, dy)
 */
void translate(canvas *cnv, double dx, double dy);

/**
 * Translate up
 */
void translate_up(canvas *cnv, double translate_factor);

/**
 * Translate down
 */
void translate_down(canvas *cnv, double translate_factor);

/**
 * Translate left
 */
void translate_left(canvas *cnv, double translate_factor);

/**
 * Translate right
 */
void translate_right(canvas *cnv, double translate_factor);
}

#endif //EZGL_CONTROL_HPP
