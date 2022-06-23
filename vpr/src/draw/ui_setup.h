#ifndef UI_H
#define UI_H

#ifndef NO_GRAPHICS

#    include "draw_global.h"

#    include "ezgl/point.hpp"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"

//UI_SETUP.H
void basic_button_setup(ezgl::application* app);
void net_button_setup(ezgl::application* app);
void block_button_setup(ezgl::application* app);
void routing_button_setup(ezgl::application* app, bool show_crit_path);


void hide_widget(std::string widgetName, ezgl::application* app);
void show_widget(std::string widgetName, ezgl::application* app);


#endif /* NO_GRAPHICS */

#endif /* UI_H */