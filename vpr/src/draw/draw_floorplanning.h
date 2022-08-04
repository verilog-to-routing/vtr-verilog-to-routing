/** This file contains all functions reagrding the graphics related to the setting of place and route breakpoints **/
#ifndef DRAW_FLOORPLANNING_H
#define DRAW_FLOORPLANNING_H

#include "globals.h"

#ifndef NO_GRAPHICS

#    include "draw_global.h"

#    include "ezgl/point.hpp"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"

void highlight_regions(ezgl::renderer* g);

#endif /*NO_GRAPHICS*/

#endif /*DRAW_FLOORPLANNING_H*/
