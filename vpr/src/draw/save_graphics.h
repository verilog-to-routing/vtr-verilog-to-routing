/**
 * @file save_graphics.h
 * 
 * Manages saving of graphics in different file formats
 */

#pragma once

#ifndef NO_GRAPHICS

#include "ezgl/application.hpp"
#include <gtk/gtk.h>

void save_graphics(std::string extension, std::string file_name);
void save_graphics_dialog_box(GtkWidget* /*widget*/, ezgl::application* /*app*/);
void save_graphics_from_button(GtkWidget* /*widget*/, gint response_id, gpointer data);

#endif /* NO_GRAPHICS */
