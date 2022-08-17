/** This file contains all functions reagrding the graphics related to the setting of place and route breakpoints **/
#ifndef DRAW_FLOORPLANNING_H
#define DRAW_FLOORPLANNING_H

#include "globals.h"

#ifndef NO_GRAPHICS

#    include "draw_global.h"

#    include "ezgl/point.hpp"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"

void highlight_all_regions(ezgl::renderer* g);
void draw_constrained_atoms(ezgl::renderer* g);
GtkWidget* setup_floorplanning_legend(GtkWidget* content_tree);
void highlight_selected_partition(GtkWidget* widget);

#endif /*NO_GRAPHICS*/

#endif /*DRAW_FLOORPLANNING_H*/
