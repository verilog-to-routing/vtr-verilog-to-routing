#ifndef UISETUP_H
#define UISETUP_H

#ifndef NO_GRAPHICS
/**
 * @file UI_SETUP.H
 * @author Sebastian Lievano
 * @brief declares ui setup functions
 */

#    include "draw_global.h"

#    include "ezgl/point.hpp"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"

//UI_SETUP.H
void basic_button_setup(ezgl::application* app);
void net_button_setup(ezgl::application* app);
void block_button_setup(ezgl::application* app);
void routing_button_setup(ezgl::application* app);
void crit_path_button_setup(ezgl::application* app);
void hide_crit_path_button(ezgl::application* app);

void hide_widget(std::string widgetName, ezgl::application* app);
void show_widget(std::string widgetName, ezgl::application* app);

#endif /* NO_GRAPHICS */

#endif /* UISETUP_H */