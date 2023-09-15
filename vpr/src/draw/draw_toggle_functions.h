/**
 * @file draw_toggle_functions.h
 * 
 * This file contains all of the callback functions for main UI elements. 
 * These callback functions alter the state of a set enum member in t_draw_state (draw_types.cpp)
 * which is then reflected in the drawing. 
 * Please add any new callback functions here, and if it makes sense, add _cbk at the end 
 * of function name to prevent someone else calling it in any non gtk context. 
 * 
 * Author: Sebastian Lievano
 */

#ifndef DRAW_TOGGLE_FUNCTIONS_H
#define DRAW_TOGGLE_FUNCTIONS_H

#include <cstdio>
#include <cfloat>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <array>
#include <iostream>

#include "vtr_assert.h"
#include "vtr_ndoffsetmatrix.h"
#include "vtr_memory.h"
#include "vtr_log.h"
#include "vtr_color_map.h"
#include "vtr_path.h"

#include "vpr_utils.h"
#include "vpr_error.h"

#include "globals.h"

#include "move_utils.h"

#ifndef NO_GRAPHICS

#    include "draw_global.h"

#    include "ezgl/point.hpp"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"
#    include "draw_color.h"
#    include "search_bar.h"
#    include "draw_debug.h"
#    include "manual_moves.h"

#    include "rr_graph.h"
#    include "route_util.h"
#    include "place_macro.h"
#    include "buttons.h"

/* Callback function for main.ui created toggle_nets button in ui_setup.cpp. Controls whether or not nets are visualized.
 * Toggles value of draw_state->show_nets.*/
void toggle_nets_cbk(GtkComboBox* self, ezgl::application* app);

/* Callback function for main.ui created netMaxFanout widget in ui_setup.cpp.
 * Sets draw_state->draw_net_max_fanout to its corresponding value in the UI. */
void set_net_max_fanout_cbk(GtkSpinButton* self, ezgl::application* app);

/* Callback function for main.ui created netAlpha widget in ui_setup.cpp.
 * Sets draw_state->net_alpha (a value from 0 to 1 representing transparency) to
 * its corresponding value in the UI. */
void set_net_alpha_value_cbk(GtkSpinButton* self, ezgl::application* app);

/* Callback function for main.ui created toggle_blk_internal button in ui_setup.cpp.
 * With each consecutive click of the button, a lower level in the
 * pb_graph will be shown for every clb. When the number of clicks on the button exceeds
 * the maximum level of sub-blocks that exists in the pb_graph, internals drawing
 * will be disabled. */
void toggle_blk_internal_cbk(GtkSpinButton* self, ezgl::application* app);

/* Callback function for main.ui created toggle_block_pin_util button in ui_setup.cpp.
 * Draws different types of routing block pin utils based on user input. Changes value of draw_state->show_blk_pin_util. */
void toggle_blk_pin_util_cbk(GtkComboBoxText* self, ezgl::application* app);

/* Callback function for main.ui created toggle_placement_macros button in ui_setup.cpp.
 * Controls if placement macros should be visualized. Changes value of draw_state->show_placement_macros. */
void placement_macros_cbk(GtkComboBoxText* self, ezgl::application* app);

/* Callback function for main.ui created toggle_rr button in ui_setup.cpp. Draws different groups of RRs depending on
 * user input. Changes value of draw_state->draw_rr_toggle. */
void toggle_rr_cbk(GtkComboBoxText* self, ezgl::application* app);

/* Callback function for main.ui created toggle_congestion button in ui_setup.cpp. Controls if congestion should be visualized.
 * Changes value of draw_state->show_congestion. */
void toggle_cong_cbk(GtkComboBoxText* self, ezgl::application* app);

/* Callback function for main.ui created ToggleCongestionCost button in ui_setup.cpp. Controls if congestion cost should be visualized.
 * Changes value of draw_state->show_routing_cost. */
void toggle_cong_cost_cbk(GtkComboBoxText* self, ezgl::application* app);

/* Callback function for main.ui created toggle_routing_congestion_cost button in ui_setup.cpp.
 * Draws different types of routing costs based on user input. Changes value of draw_state->show_routing_costs. */
void toggle_routing_congestion_cost(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);

/* Callback function for main.ui created toggle_routing_bounding_box button in ui_setup.cpp.
 * Controls if routing bounding box should be visualized. Changes value of draw_state->show_routing_bb */
void toggle_routing_bbox_cbk(GtkSpinButton* self, ezgl::application* app);

/* Callback function for main.ui created toggle_routing_util button in ui_setup.cpp.
 * Draws different types of routing utils based on user input: . Changes value of draw_state->show_routing_util. */
void toggle_router_util_cbk(GtkComboBoxText* self, ezgl::application* app);

/* Callback function for main.ui created toggle_crit_path button in ui_setup.cpp.
 * Draws different types of critical path based on user input. Changes value of draw_state->show_crit_path. */
void toggle_crit_path_cbk(GtkComboBoxText* self, ezgl::application* app);

/* Callback function for main.ui created toggle_router_expansion_costs in ui_setup.cpp.
 * Draws different router expansion costs based on user input. Changes value of draw_state->show_router_expansion_cost. */
void toggle_expansion_cost_cbk(GtkComboBoxText* self, ezgl::application* app);

/* Callback function for main.ui created ToggleNocBox in ui_setup.cpp
 * Controls if the NoC on chip should be visualized and whether the link usage
 * in the NoC should be visualized. Changes value of draw_state->draw_noc */
void toggle_noc_cbk(GtkComboBoxText* self, ezgl::application* app);

/* Callback function for main.ui created netMaxFanout widget in button.cpp.
 * Sets draw_state->draw_net_max_fanout to its corresponding value in the UI. */
void net_max_fanout(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);

/* Callback function for main.ui created netAlpha widget in button.cpp.
 * Sets draw_state->net_alpha (a value from 0 to 1 representing transparency) to
 * its corresponding value in the UI. */
void set_net_alpha_value(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);

/**
 * @brief Callback function for 3d layer checkboxes
 * Updates draw_state->draw_layer_display based on which checkboxes are checked
 *
 * @param widget: pointer to the gtk widget for 3d layer checkboxes
 */
void select_layer_cbk(GtkWidget* widget, gint /*response_id*/, gpointer /*data*/);

/**
 * @brief Callback function for 3d layer transparency spin buttons
 * Updates draw_state->draw_layer_display based on the values in spin buttons
 *
 * @param widget: gtk widget for layer transparency spin buttons
 */
void transparency_cbk(GtkWidget* widget, gint /*response_id*/, gpointer /*data*/);

/**
 * @brief Callback function for cross layer connection checkbox
 * Updates draw_state->cross_layer_display.visible based on whether the cross layer
 * connection checkbox is checked.
 *
 * @param widget: gtk widget for the cross layer connection checkbox
 */
void cross_layer_checkbox_cbk(GtkWidget* widget, gint /*response_id*/, gpointer /*data*/);

/**
 * @brief Callback function for cross layer connection spin button
 * Updates draw_state->cross_layer_display.alpha based spin button value
 *
 * @param widget: gtk widget for the cross layer connection transparency spin button
 */
void cross_layer_transparency_cbk(GtkWidget* widget, gint /*response_id*/, gpointer /*data*/);
#endif /* NO_GRAPHICS */
#endif /* DRAW_TOGGLE_FUNCTIONS_H */
