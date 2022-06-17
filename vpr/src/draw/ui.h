#ifndef UI_H
#define UI_H

#ifndef NO_GRAPHICS

#    include "draw_global.h"

#    include "ezgl/point.hpp"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"

void basic_button_setup(ezgl::application* app);
void net_button_setup(ezgl::application* app);




void hide_widget(std::string widgetName, ezgl::application* app);
void show_widget(std::string widgetName, ezgl::application* app);


#endif /* NO_GRAPHICS */

#endif /* UI_H */