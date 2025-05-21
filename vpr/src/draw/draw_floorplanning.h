/** This file contains all functions regarding the graphics related to drawing floorplanning constraints. **/
#pragma once

#ifndef NO_GRAPHICS

#include <gtk/gtk.h>
#include "ezgl/graphics.hpp"

///@brief Iterates through all partitions described in the constraints file and highlights their respective partitions
void highlight_all_regions(ezgl::renderer* g);

///@brief Draws atoms that're constrained to a partition in the colour of their respective partition.
void draw_constrained_atoms(ezgl::renderer* g);

///@brief Sets up and fills in the floorplanning legend
GtkWidget* setup_floorplanning_legend(GtkWidget* content_tree);

///@brief Highlights partitions clicked on in the legend
void highlight_selected_partition(GtkWidget* widget);

#endif /*NO_GRAPHICS*/
