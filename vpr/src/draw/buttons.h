#ifndef BUTTONS_H
#define BUTTONS_H

#ifndef NO_GRAPHICS

#    include "draw_global.h"

#    include "ezgl/point.hpp"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"

void button_for_displaying_noc();

void delete_button(const char* button_name);
GtkWidget* find_button(const char* button_name);
#endif /* NO_GRAPHICS */

#endif /* BUTTONS_H */
