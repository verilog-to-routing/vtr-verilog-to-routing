

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
#include "draw_color.h"
#include "draw.h"
#include "draw_rr.h"
#include "draw_rr_edges.h"
#include "draw_toggle_functions.h"
#include "draw_triangle.h"
#include "draw_searchbar.h"
#include "draw_mux.h"
#include "read_xml_arch_file.h"
#include "draw_global.h"
#include "draw_basic.h"
#include "hsl.h"
#include "move_utils.h"
#include "intra_logic_block.h"

#ifdef VTR_ENABLE_DEBUG_LOGGING
#    include "move_utils.h"
#endif

#ifdef WIN32 /* For runtime tracking in WIN32. The clock() function defined in time.h will *
              * track CPU runtime.														   */
#    include <time.h>
#else /* For X11. The clock() function in time.h will not output correct time difference   *
       * for X11, because the graphics is processed by the Xserver rather than local CPU,  *
       * which means tracking CPU time will not be the same as the actual wall clock time. *
       * Thus, so use gettimeofday() in sys/time.h to track actual calendar time.          */
#    include <sys/time.h>
#endif

#ifndef NO_GRAPHICS

//To process key presses we need the X11 keysym definitions,
//which are unavailable when building with MINGW
#    if defined(X11) && !defined(__MINGW32__)
#        include <X11/keysym.h>
#    endif

//The arrow head position for turning/straight-thru connections in a switch box
constexpr float SB_EDGE_TURN_ARROW_POSITION = 0.2;
constexpr float SB_EDGE_STRAIGHT_ARROW_POSITION = 0.95;
constexpr float EMPTY_BLOCK_LIGHTEN_FACTOR = 0.20;

/**
 * @brief toggles net drawing status based on status of combo box
 * updates draw_state->show_nets
 * 
 * @param self ptr to gtkComboBox
 * @param app ezgl::application
 */
void toggle_nets_cbk(GtkComboBox* self, ezgl::application* app) {
    std::cout << "Nets toggled" << std::endl;
    enum e_draw_nets new_state;
    t_draw_state* draw_state = get_draw_state_vars();
    std::cout << draw_state << std::endl;
    gchar* setting = gtk_combo_box_text_get_active_text(
        GTK_COMBO_BOX_TEXT(self));
    std::cout << setting << std::endl;
    // assign corresponding enum value to draw_state->show_nets
    if (strcmp(setting, "None") == 0)
        new_state = DRAW_NO_NETS;
    else if (strcmp(setting, "Cluster Nets") == 0) {
        new_state = DRAW_CLUSTER_NETS;
    } else { // Primitive Nets - Used to be called "Logical Connections"
        new_state = DRAW_PRIMITIVE_NETS;
    }
    draw_state->reset_nets_congestion_and_rr();
    draw_state->show_nets = new_state;

    //free dynamically allocated pointers
    g_free(setting);

    //redraw
    app->update_message(draw_state->default_message);
    app->refresh_drawing();
}

/**
 * @brief cbk function for toggle rr combo-box. sets rr draw state based on selected option
 * updates draw_state->draw_rr_toggle
 * 
 * @param self ptr to gtkComboBoxText object
 * @param app ezgl application
 */
void toggle_rr_cbk(GtkComboBoxText* self, ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();

    enum e_draw_rr_toggle new_state;
    gchar* combo_box_content = gtk_combo_box_text_get_active_text(self);
    if (strcmp(combo_box_content, "None") == 0)
        new_state = DRAW_NO_RR;
    else if (strcmp(combo_box_content, "Nodes") == 0)
        new_state = DRAW_NODES_RR;
    else if (strcmp(combo_box_content, "Nodes SBox") == 0)
        new_state = DRAW_NODES_SBOX_RR;
    else if (strcmp(combo_box_content, "Nodes SBox CBox") == 0)
        new_state = DRAW_NODES_SBOX_CBOX_RR;
    else if (strcmp(combo_box_content, "Nodes SBox CBox Internal") == 0)
        new_state = DRAW_NODES_SBOX_CBOX_INTERNAL_RR;
    else
        // all rr
        new_state = DRAW_ALL_RR;

    //free dynamically allocated pointers
    g_free(combo_box_content);

    draw_state->reset_nets_congestion_and_rr();
    draw_state->draw_rr_toggle = new_state;

    app->update_message(draw_state->default_message);
    app->refresh_drawing();
}

/**
 * @brief cbk function for toggle congestion comboBox. Updates shown cong. based on selected option
 * updates draw_state->show_congestion
 * 
 * @param self self ptr to GtkComboBoxText
 * @param app ezgl application
 */
void toggle_cong_cbk(GtkComboBoxText* self, ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();
    enum e_draw_congestion new_state;
    gchar* combo_box_content = gtk_combo_box_text_get_active_text(self);
    if (strcmp(combo_box_content, "None") == 0)
        new_state = DRAW_NO_CONGEST;
    else if (strcmp(combo_box_content, "Congested") == 0)
        new_state = DRAW_CONGESTED;
    else
        // congested with nets
        new_state = DRAW_CONGESTED_WITH_NETS;

    draw_state->reset_nets_congestion_and_rr();
    draw_state->show_congestion = new_state;
    if (draw_state->show_congestion == DRAW_NO_CONGEST) {
        app->update_message(draw_state->default_message);
    }
    g_free(combo_box_content);
    app->refresh_drawing();
}

/**
 * @brief cbk function for toggle cong. cost combobox. sets draws state according to new setting
 * updates draw_state->show_routing_costs
 * 
 * @param self self ptr
 * @param app ezgl app
 */
void toggle_cong_cost_cbk(GtkComboBoxText* self, ezgl::application* app) {
    /* this is the callback function for runtime created toggle_routing_congestion_cost button
     * which is written in button.cpp                                         */
    t_draw_state* draw_state = get_draw_state_vars();
    enum e_draw_routing_costs new_state;
    gchar* combo_box_content = gtk_combo_box_text_get_active_text(self);
    if (strcmp(combo_box_content, "None") == 0)
        new_state = DRAW_NO_ROUTING_COSTS;
    else if (strcmp(combo_box_content, "Total Routing Costs") == 0)
        new_state = DRAW_TOTAL_ROUTING_COSTS;
    else if (strcmp(combo_box_content, "Log Total Routing Costs") == 0)
        new_state = DRAW_LOG_TOTAL_ROUTING_COSTS;
    else if (strcmp(combo_box_content, "Acc Routing Costs") == 0)
        new_state = DRAW_ACC_ROUTING_COSTS;
    else if (strcmp(combo_box_content, "Log Acc Routing Costs") == 0)
        new_state = DRAW_LOG_ACC_ROUTING_COSTS;
    else if (strcmp(combo_box_content, "Pres Routing Costs") == 0)
        new_state = DRAW_PRES_ROUTING_COSTS;
    else if (strcmp(combo_box_content, "Log Pres Routing Costs") == 0)
        new_state = DRAW_LOG_PRES_ROUTING_COSTS;
    else
        new_state = DRAW_BASE_ROUTING_COSTS;

    draw_state->reset_nets_congestion_and_rr();
    draw_state->show_routing_costs = new_state;
    g_free(combo_box_content);
    if (draw_state->show_routing_costs == DRAW_NO_ROUTING_COSTS) {
        app->update_message(draw_state->default_message);
    }
    app->refresh_drawing();
}

/**
 * @brief cbk fn for spin button which manages drawing of routing bbox. 
 * Updates draw_state->show_routing_bb
 * 
 * @param self 
 * @param app 
 */
void toggle_routing_bbox_cbk(GtkSpinButton* self, ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();
    auto& route_ctx = g_vpr_ctx.routing();
    // get the pointer to the toggle_routing_bounding_box button

    if (route_ctx.route_bb.size() == 0)
        return; //Nothing to draw

    // use the pointer to get the active value
    int new_value = gtk_spin_button_get_value_as_int(self);

    // assign value to draw_state->show_routing_bb, bound check + set OPEN when it's -1 (draw nothing)
    if (new_value < -1)
        draw_state->show_routing_bb = -1;
    else if (new_value == -1)
        draw_state->show_routing_bb = OPEN;
    else if (new_value >= (int)(route_ctx.route_bb.size()))
        draw_state->show_routing_bb = route_ctx.route_bb.size() - 1;
    else
        draw_state->show_routing_bb = new_value;

    //redraw
    if ((int)(draw_state->show_routing_bb)
        == (int)((int)(route_ctx.route_bb.size()) - 1)) {
        app->update_message(draw_state->default_message);
    }
    app->refresh_drawing();
}

/**
 * @brief cbk fn for toggle router util gtkComboBoxText.
 * Alters draw_state->show_routing_util to reflect new value
 * 
 * @param self self ptr
 * @param app ezgl app
 */
void toggle_router_util_cbk(GtkComboBoxText* self, ezgl::application* app) {
    /* this is the callback function for runtime created toggle_routing_util button
     * which is written in button.cpp                                         */
    t_draw_state* draw_state = get_draw_state_vars();
    enum e_draw_routing_util new_state;

    gchar* combo_box_content = gtk_combo_box_text_get_active_text(self);
    if (strcmp(combo_box_content, "None") == 0)
        new_state = DRAW_NO_ROUTING_UTIL;
    else if (strcmp(combo_box_content, "Routing Util") == 0)
        new_state = DRAW_ROUTING_UTIL;
    else if (strcmp(combo_box_content, "Routing Util with Value") == 0)
        new_state = DRAW_ROUTING_UTIL_WITH_VALUE;
    else if (strcmp(combo_box_content, "Routing Util with Formula") == 0)
        new_state = DRAW_ROUTING_UTIL_WITH_FORMULA;
    else
        new_state = DRAW_ROUTING_UTIL_OVER_BLOCKS;

    g_free(combo_box_content);
    draw_state->show_routing_util = new_state;

    if (draw_state->show_routing_util == DRAW_NO_ROUTING_UTIL) {
        app->update_message(draw_state->default_message);
    }
    app->refresh_drawing();
}

/**
 * @brief cbk fn for block internals spin button, updates values
 * updates draw_state->show_blk_internal
 * 
 * @param self ptr to self
 * @param app ezgl::app
 */
void toggle_blk_internal_cbk(GtkSpinButton* self, ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();
    int new_value = gtk_spin_button_get_value_as_int(self);
    if (new_value < 0)
        draw_state->show_blk_internal = 0;
    else if (new_value >= draw_state->max_sub_blk_lvl)
        draw_state->show_blk_internal = draw_state->max_sub_blk_lvl - 1;
    else
        draw_state->show_blk_internal = new_value;
    app->refresh_drawing();
}

/**
 * @brief cbk function when pin util gets changed in ui; sets pin util drawing to new val
 * updates draw_state->show_blk_pin_util
 * 
 * @param self ptr to selt
 * @param app ezgl::app
 */
void toggle_blk_pin_util_cbk(GtkComboBoxText* self, ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();
    gchar* combo_box_content = gtk_combo_box_text_get_active_text(self);
    if (strcmp(combo_box_content, "None") == 0) {
        draw_state->show_blk_pin_util = DRAW_NO_BLOCK_PIN_UTIL;
        draw_reset_blk_colors();
        app->update_message(draw_state->default_message);
    } else if (strcmp(combo_box_content, "All") == 0)
        draw_state->show_blk_pin_util = DRAW_BLOCK_PIN_UTIL_TOTAL;
    else if (strcmp(combo_box_content, "Inputs") == 0)
        draw_state->show_blk_pin_util = DRAW_BLOCK_PIN_UTIL_INPUTS;
    else
        draw_state->show_blk_pin_util = DRAW_BLOCK_PIN_UTIL_OUTPUTS;

    g_free(combo_box_content);
    app->refresh_drawing();
}

/**
 * @brief cbk function when pin util gets changed in ui; sets pin util drawing to new val
 * updates draw_state->show_placement_macros
 * 
 * @param self ptr to selt
 * @param app ezgl::app
 */
void placement_macros_cbk(GtkComboBoxText* self, ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();
    gchar* combo_box_content = gtk_combo_box_text_get_active_text(self);
    if (strcmp(combo_box_content, "None") == 0)
        draw_state->show_placement_macros = DRAW_NO_PLACEMENT_MACROS;
    else
        draw_state->show_placement_macros = DRAW_PLACEMENT_MACROS;

    g_free(combo_box_content);
    app->refresh_drawing();
}

/**
 * @brief cbk fn for toggling critical path. 
 * alters draw_state->show_crit_path to reflect new state
 * 
 * @param self ptr to combo box w setting
 * @param app ezgl app
 */
void toggle_crit_path_cbk(GtkComboBoxText* self, ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();
    gchar* combo_box_content = gtk_combo_box_text_get_active_text(self);
    if (strcmp(combo_box_content, "None") == 0) {
        draw_state->show_crit_path = DRAW_NO_CRIT_PATH;
    } else if (strcmp(combo_box_content, "Crit Path Flylines") == 0)
        draw_state->show_crit_path = DRAW_CRIT_PATH_FLYLINES;
    else if (strcmp(combo_box_content, "Crit Path Flylines Delays") == 0)
        draw_state->show_crit_path = DRAW_CRIT_PATH_FLYLINES_DELAYS;
    else if (strcmp(combo_box_content, "Crit Path Routing") == 0)
        draw_state->show_crit_path = DRAW_CRIT_PATH_ROUTING;
    else
        // Crit Path Routing Delays
        draw_state->show_crit_path = DRAW_CRIT_PATH_ROUTING_DELAYS;

    g_free(combo_box_content);
    app->refresh_drawing();
}

/**
 * @brief cbk function for toggling routing expansion cost. 
 * alters draw_state->show_router_expansion cost var. to reflect new value
 * 
 * @param self self ptr
 * @param app ezgl::application
 */
void toggle_expansion_cost_cbk(GtkComboBoxText* self, ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();
    e_draw_router_expansion_cost new_state;
    gchar* combo_box_content = gtk_combo_box_text_get_active_text(self);
    if (strcmp(combo_box_content, "None") == 0) {
        new_state = DRAW_NO_ROUTER_EXPANSION_COST;
    } else if (strcmp(combo_box_content, "Total") == 0) {
        new_state = DRAW_ROUTER_EXPANSION_COST_TOTAL;
    } else if (strcmp(combo_box_content, "Known") == 0) {
        new_state = DRAW_ROUTER_EXPANSION_COST_KNOWN;
    } else if (strcmp(combo_box_content, "Expected") == 0) {
        new_state = DRAW_ROUTER_EXPANSION_COST_EXPECTED;
    } else if (strcmp(combo_box_content, "Total (with edges)") == 0) {
        new_state = DRAW_ROUTER_EXPANSION_COST_TOTAL_WITH_EDGES;
    } else if (strcmp(combo_box_content, "Known (with edges)") == 0) {
        new_state = DRAW_ROUTER_EXPANSION_COST_KNOWN_WITH_EDGES;
    } else if (strcmp(combo_box_content, "Expected (with edges)") == 0) {
        new_state = DRAW_ROUTER_EXPANSION_COST_EXPECTED_WITH_EDGES;
    } else {
        VPR_THROW(VPR_ERROR_DRAW, "Unrecognzied draw RR cost option");
    }

    g_free(combo_box_content);
    draw_state->show_router_expansion_cost = new_state;

    if (draw_state->show_router_expansion_cost
        == DRAW_NO_ROUTER_EXPANSION_COST) {
        app->update_message(draw_state->default_message);
    }
    app->refresh_drawing();
}

/**
 * @brief cbk fn to toggle Network-On-Chip (Noc) visibility
 * alters draw_state->draw_noc to reflect new state
 * Reacts to main.ui created combo box, setup in ui_setup.cpp
 * 
 * @param self ptr to combo box
 * @param app ezgl application
 */
void toggle_noc_cbk(GtkComboBoxText* self, ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();

    gchar* combo_box_content = gtk_combo_box_text_get_active_text(self);
    if (strcmp(combo_box_content, "None") == 0) {
        draw_state->draw_noc = DRAW_NO_NOC;
    } else if (strcmp(combo_box_content, "NoC Links") == 0)
        draw_state->draw_noc = DRAW_NOC_LINKS;
    else
        draw_state->draw_noc = DRAW_NOC_LINK_USAGE;

    g_free(combo_box_content);
    app->refresh_drawing();
}

/**
 * @brief CBK for Net Max Fanout spin button. Sets max fanout when val. changes
 * updates draw_state->draw_net_max_fanout
 * 
 * @param self self ptr to GtkSpinButton
 * @param app ezgl::app
 */
void set_net_max_fanout_cbk(GtkSpinButton* self, ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();
    draw_state->draw_net_max_fanout = gtk_spin_button_get_value_as_int(self);
    app->refresh_drawing();
}

/**
 * @brief Set the net alpha value (transparency) based on value of spin button
 * updates draw_state->net_alpha
 * 
 * @param self 
 * @param app 
 */
void set_net_alpha_value_cbk(GtkSpinButton* self, ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();
    draw_state->net_alpha = (255 - gtk_spin_button_get_value_as_int(self)) / 255.0;
    app->refresh_drawing();
}

/**
 * @brief Callback function for 3d layer checkboxes
 */
void select_layer_cbk(GtkWidget* widget, gint /*response_id*/, gpointer /*data*/) {
    t_draw_state* draw_state = get_draw_state_vars();

    GtkWidget* parent = gtk_widget_get_parent(widget);
    GtkBox* box = GTK_BOX(parent);

    GList* children = gtk_container_get_children(GTK_CONTAINER(box));
    int index = 0;
    // Iterate over the checkboxes
    for (GList* iter = children; iter != NULL; iter = g_list_next(iter)) {
        if (GTK_IS_CHECK_BUTTON(iter->data)) {
            GtkWidget* checkbox = GTK_WIDGET(iter->data);
            gboolean state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbox));
            const gchar* name = gtk_button_get_label(GTK_BUTTON(checkbox));

            // Only iterate through checkboxes with name "Layer ...", skip Cross Layer Connection
            if (std::string(name).find("Layer") != std::string::npos
                && std::string(name).find("Cross") == std::string::npos) {
                // Change the the boolean of the draw_layer_display vector depending on checkbox
                if (state) {
                    draw_state->draw_layer_display[index].visible = true;
                } else {
                    draw_state->draw_layer_display[index].visible = false;
                }
                index++;
            }
        }
    }
    application.refresh_drawing();
    g_list_free(children);
}
/**
 * @brief Callback function for 3d layer transparency spin buttons
 */
void transparency_cbk(GtkWidget* widget, gint /*response_id*/, gpointer /*data*/) {
    t_draw_state* draw_state = get_draw_state_vars();

    GtkWidget* parent = gtk_widget_get_parent(widget);
    GtkBox* box = GTK_BOX(parent);
    GList* children = gtk_container_get_children(GTK_CONTAINER(box));

    int index = 0;
    // Iterate over transparency layers
    for (GList* iter = children; iter != NULL; iter = g_list_next(iter)) {
        if (GTK_IS_SPIN_BUTTON(iter->data)) {
            GtkWidget* spin_button = GTK_WIDGET(iter->data);
            const gchar* name = gtk_widget_get_name(spin_button);

            if (std::string(name).find("Transparency") != std::string::npos
                && std::string(name).find("Cross") == std::string::npos) {
                gint value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button));
                draw_state->draw_layer_display[index].alpha = 255 - value;
                index++;
            }
        }
    }
    application.refresh_drawing();
    g_list_free(children);
}

/**
 * @brief Callback function for cross layer connection checkbox
 */
void cross_layer_checkbox_cbk(GtkWidget* widget, gint /*response_id*/, gpointer /*data*/) {
    t_draw_state* draw_state = get_draw_state_vars();

    gboolean state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    if (state) {
        draw_state->cross_layer_display.visible = true;
    } else {
        draw_state->cross_layer_display.visible = false;
    }

    application.refresh_drawing();
}

/**
 * @brief Callback function for cross layer connection spin button
 */
void cross_layer_transparency_cbk(GtkWidget* widget, gint /*response_id*/, gpointer /*data*/) {
    t_draw_state* draw_state = get_draw_state_vars();

    gint value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
    draw_state->cross_layer_display.alpha = 255 - value;

    application.refresh_drawing();
}
#endif
