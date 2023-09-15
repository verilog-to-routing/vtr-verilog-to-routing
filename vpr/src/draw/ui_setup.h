/**
 * @file ui_setup.h
 * @brief declares ui setup functions
 * 
 * UI Members are initialized and created through a main.ui file, which is maintained and edited using the Glade program.
 * This file (ui_setup.h/cpp) contains the setup of these buttons, which are connected to their respective callback functions
 * in draw_toggle_functions.cpp. 
 * 
 * Author: Sebastian Lievano
 */

#ifndef UISETUP_H
#define UISETUP_H

#ifndef NO_GRAPHICS

#    include "draw_global.h"

#    include "ezgl/point.hpp"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"

/**
 * @brief configures basic buttons
 * 
 * Sets up Window, Search, Save, and SearchType buttons. Buttons are 
 * created in glade main.ui file. Connects them to their cbk functions.
 * Always called. 
 */
void basic_button_setup(ezgl::application* app);

/**
 * @brief sets up net related buttons and connects their signals
 * 
 * Sets up the toggle nets combo box, net alpha spin button, and max fanout
 * spin button which are created in main.ui file. Found in Net Settings dropdown.
 * Always called. 
 */
void net_button_setup(ezgl::application* app);

/**
 * @brief sets up block related buttons, connects their signals
 * 
 * Connects signals and sets init. values for blk internals spin button,
 * blk pin util combo box,placement macros combo box, and noc combo bx created in
 * main.ui. Found in Block Settings dropdown. Always Called.
 */
void block_button_setup(ezgl::application* app);

/**
 * @brief Loads required data for search autocomplete, sets up special completion fn
 */
void search_setup(ezgl::application* app);

/**
 * @brief configures and connects signals/functions for routing buttons
 * 
 * Connects signals/sets default values for toggleRRButton, ToggleCongestion,
 * ToggleCongestionCost, ToggleRoutingBBox, RoutingExpansionCost, ToggleRoutingUtil 
 * buttons. Called in all startup options/runs that include Routing
 */
void routing_button_setup(ezgl::application* app);

/**
 * @brief configures and connects signals/functions for View buttons
 *
 * Determines how many layers there are and displays depending on number of layers
 */
void view_button_setup(ezgl::application* app);

/**
 * @brief connects critical path button to its cbk fn. Called in all setup options that show crit. path
 */
void crit_path_button_setup(ezgl::application* app);

/**
 * @brief Hides or displays Critical Path routing / routing delay UI elements,
 * Use to ensure we don't show inactive buttons etc. when routing data doesn't exist
 */
void hide_crit_path_routing(ezgl::application* app, bool hide);

/**
 * @brief Loads block names into Gtk Structures to enable autocomplete
 */
void load_block_names(ezgl::application* app);

/**
 * @brief Loads net names into Gtk ListStore to enable autocomplete
 */
void load_net_names(ezgl::application* app);

/**
 * @brief Hides widget with given name; name is id string created in Glade
 */
void hide_widget(std::string widgetName, ezgl::application* app);

/**
 * @brief Shows widget with given name; name is id string created in Glade
 */
void show_widget(std::string widgetName, ezgl::application* app);

#endif /* NO_GRAPHICS */

#endif /* UISETUP_H */