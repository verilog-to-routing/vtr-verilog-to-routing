/*draw_toggle_functions.cpp contains callback functions that change draw_state variables
 * connected to buttons and sliders on the GUI.
 */
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

void toggle_nets(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/) {
    /* this is the callback function for runtime created toggle_nets button
     * which is written in button.cpp                                         */
    t_draw_state* draw_state = get_draw_state_vars();

    // get the pointer to the toggle_nets button
    std::string button_name = "toggle_nets";
    auto toggle_nets = find_button(button_name.c_str());

    // use the pointer to get the active text
    enum e_draw_nets new_state;
    gchar* combo_box_content = gtk_combo_box_text_get_active_text(
        GTK_COMBO_BOX_TEXT(toggle_nets));

    // assign corresponding enum value to draw_state->show_nets
    if (strcmp(combo_box_content, "None") == 0)
        new_state = DRAW_NO_NETS;
    else if (strcmp(combo_box_content, "Nets") == 0) {
        new_state = DRAW_NETS;
    } else { // "Logical Connections"
        new_state = DRAW_LOGICAL_CONNECTIONS;
    }
    draw_state->reset_nets_congestion_and_rr();
    draw_state->show_nets = new_state;

    //free dynamically allocated pointers
    g_free(combo_box_content);

    //redraw
    application.update_message(draw_state->default_message);
    application.refresh_drawing();
}

void toggle_rr(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/) {
    /* this is the callback function for runtime created toggle_rr button
     * which is written in button.cpp                                         */
    t_draw_state* draw_state = get_draw_state_vars();
    std::string button_name = "toggle_rr";
    auto toggle_rr = find_button(button_name.c_str());

    enum e_draw_rr_toggle new_state;
    gchar* combo_box_content = gtk_combo_box_text_get_active_text(
        GTK_COMBO_BOX_TEXT(toggle_rr));
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

    application.update_message(draw_state->default_message);
    application.refresh_drawing();
}

void toggle_congestion(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/) {
    /* this is the callback function for runtime created toggle_congestion button
     * which is written in button.cpp                                         */
    t_draw_state* draw_state = get_draw_state_vars();
    std::string button_name = "toggle_congestion";
    auto toggle_congestion = find_button(button_name.c_str());

    enum e_draw_congestion new_state;
    gchar* combo_box_content = gtk_combo_box_text_get_active_text(
        GTK_COMBO_BOX_TEXT(toggle_congestion));
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
        application.update_message(draw_state->default_message);
    }
    g_free(combo_box_content);
    application.refresh_drawing();
}

void toggle_routing_congestion_cost(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/) {
    /* this is the callback function for runtime created toggle_routing_congestion_cost button
     * which is written in button.cpp                                         */
    t_draw_state* draw_state = get_draw_state_vars();
    std::string button_name = "toggle_routing_congestion_cost";
    auto toggle_routing_congestion_cost = find_button(button_name.c_str());
    enum e_draw_routing_costs new_state;
    gchar* combo_box_content = gtk_combo_box_text_get_active_text(
        GTK_COMBO_BOX_TEXT(toggle_routing_congestion_cost));
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
        application.update_message(draw_state->default_message);
    }
    application.refresh_drawing();
}

void toggle_routing_bounding_box(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/) {
    /* this is the callback function for runtime created toggle_routing_bounding_box button
     * which is written in button.cpp                                         */
    t_draw_state* draw_state = get_draw_state_vars();
    auto& route_ctx = g_vpr_ctx.routing();
    // get the pointer to the toggle_routing_bounding_box button
    std::string button_name = "toggle_routing_bounding_box";
    auto toggle_routing_bounding_box = find_button(button_name.c_str());

    if (route_ctx.route_bb.size() == 0)
        return; //Nothing to draw

    // use the pointer to get the active value
    int new_value = gtk_spin_button_get_value_as_int(
        (GtkSpinButton*)toggle_routing_bounding_box);

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
        application.update_message(draw_state->default_message);
    }
    application.refresh_drawing();
}

void toggle_routing_util(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/) {
    /* this is the callback function for runtime created toggle_routing_util button
     * which is written in button.cpp                                         */
    t_draw_state* draw_state = get_draw_state_vars();
    std::string button_name = "toggle_routing_util";
    auto toggle_routing_util = find_button(button_name.c_str());

    enum e_draw_routing_util new_state;
    gchar* combo_box_content = gtk_combo_box_text_get_active_text(
        GTK_COMBO_BOX_TEXT(toggle_routing_util));
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
        application.update_message(draw_state->default_message);
    }
    application.refresh_drawing();
}

void toggle_blk_internal(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/) {
    /* this is the callback function for runtime created toggle_blk_internal button
     * which is written in button.cpp                                         */
    t_draw_state* draw_state = get_draw_state_vars();
    std::string button_name = "toggle_blk_internal";
    auto toggle_blk_internal = find_button(button_name.c_str());

    int new_value = gtk_spin_button_get_value_as_int(
        (GtkSpinButton*)toggle_blk_internal);
    if (new_value < 0)
        draw_state->show_blk_internal = 0;
    else if (new_value >= draw_state->max_sub_blk_lvl)
        draw_state->show_blk_internal = draw_state->max_sub_blk_lvl - 1;
    else
        draw_state->show_blk_internal = new_value;
    application.refresh_drawing();
}

void toggle_block_pin_util(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/) {
    /* this is the callback function for runtime created toggle_block_pin_util button
     * which is written in button.cpp                                         */
    t_draw_state* draw_state = get_draw_state_vars();
    std::string button_name = "toggle_block_pin_util";
    auto toggle_block_pin_util = find_button(button_name.c_str());
    gchar* combo_box_content = gtk_combo_box_text_get_active_text(
        GTK_COMBO_BOX_TEXT(toggle_block_pin_util));
    if (strcmp(combo_box_content, "None") == 0) {
        draw_state->show_blk_pin_util = DRAW_NO_BLOCK_PIN_UTIL;
        draw_reset_blk_colors();
        application.update_message(draw_state->default_message);
    } else if (strcmp(combo_box_content, "All") == 0)
        draw_state->show_blk_pin_util = DRAW_BLOCK_PIN_UTIL_TOTAL;
    else if (strcmp(combo_box_content, "Inputs") == 0)
        draw_state->show_blk_pin_util = DRAW_BLOCK_PIN_UTIL_INPUTS;
    else
        draw_state->show_blk_pin_util = DRAW_BLOCK_PIN_UTIL_OUTPUTS;

    g_free(combo_box_content);
    application.refresh_drawing();
}

void toggle_placement_macros(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/) {
    /* this is the callback function for runtime created toggle_placement_macros button
     * which is written in button.cpp                                         */
    t_draw_state* draw_state = get_draw_state_vars();
    std::string button_name = "toggle_placement_macros";
    auto toggle_placement_macros = find_button(button_name.c_str());

    gchar* combo_box_content = gtk_combo_box_text_get_active_text(
        GTK_COMBO_BOX_TEXT(toggle_placement_macros));
    if (strcmp(combo_box_content, "None") == 0)
        draw_state->show_placement_macros = DRAW_NO_PLACEMENT_MACROS;
    else
        draw_state->show_placement_macros = DRAW_PLACEMENT_MACROS;

    g_free(combo_box_content);
    application.refresh_drawing();
}

void toggle_crit_path(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/) {
    /* this is the callback function for runtime created toggle_crit_path button
     * which is written in button.cpp                                         */
    t_draw_state* draw_state = get_draw_state_vars();
    std::string button_name = "toggle_crit_path";
    auto toggle_crit_path = find_button(button_name.c_str());

    gchar* combo_box_content = gtk_combo_box_text_get_active_text(
        GTK_COMBO_BOX_TEXT(toggle_crit_path));
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
    application.refresh_drawing();
}

void toggle_router_expansion_costs(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/) {
    /* this is the callback function for runtime created toggle_router_expansion_costs button
     * which is written in button.cpp                                         */
    t_draw_state* draw_state = get_draw_state_vars();
    std::string button_name = "toggle_router_expansion_costs";
    auto toggle_router_expansion_costs = find_button(button_name.c_str());

    e_draw_router_expansion_cost new_state;
    gchar* combo_box_content = gtk_combo_box_text_get_active_text(
        GTK_COMBO_BOX_TEXT(toggle_router_expansion_costs));
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
        application.update_message(draw_state->default_message);
    }
    application.refresh_drawing();
}

// Callback function for NetMax Fanout checkbox
void net_max_fanout(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/) {
    /* this is the callback function for runtime created net_max_fanout widget
     * which is written in button.cpp                                         */
    std::string button_name = "netMaxFanout";
    auto max_fanout = find_button(button_name.c_str());
    t_draw_state* draw_state = get_draw_state_vars();

    //set draw_state->draw_net_max_fanout to its corresponding value in the ui
    int new_value = gtk_spin_button_get_value_as_int(
        (GtkSpinButton*)max_fanout);
    draw_state->draw_net_max_fanout = new_value;

    //redraw
    application.refresh_drawing();
}

void set_net_alpha_value(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/) {
    std::string button_name = "netAlpha";
    auto net_alpha = find_button(button_name.c_str());
    t_draw_state* draw_state = get_draw_state_vars();

    //set draw_state->net_alpha to its corresponding value in the ui
    int new_value = gtk_spin_button_get_value_as_int((GtkSpinButton*)net_alpha);
    draw_state->net_alpha = new_value;

    //redraw
    application.refresh_drawing();
}

#endif
