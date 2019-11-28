#ifndef SAVE_GRAPHICS_H
#define SAVE_GRAPHICS_H

#ifndef NO_GRAPHICS

#    include "draw_global.h"

#    include "ezgl/point.hpp"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"

void save_graphics(std::string& extension, std::string& file_name);
void save_graphics_dialog_box(GtkWidget* /*widget*/, ezgl::application* /*app*/);
void save_graphics_from_button(GtkWidget* /*widget*/, gint response_id, gpointer data);

#endif /* NO_GRAPHICS */

#endif /* SAVE_GRAPHICS_H */
