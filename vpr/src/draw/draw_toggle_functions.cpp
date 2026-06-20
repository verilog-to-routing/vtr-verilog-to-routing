
#ifndef NO_GRAPHICS

#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>

#include <cstring>

#include "vpr_error.h"

#include "globals.h"
#include "draw.h"
#include "draw_toggle_functions.h"

#include "draw_global.h"
#include "draw_basic.h"

//To process key presses we need the X11 keysym definitions,
//which are unavailable when building with MINGW
#if defined(X11) && !defined(__MINGW32__)
#include <X11/keysym.h>
#endif

void toggle_checkbox_cbk(QCheckBox* self, t_checkbox_data* data) {
    *data->toggle_state = self->isChecked();
    data->app->refresh_drawing();
}

void toggle_show_nets_cbk(SwitchButton*, bool state, ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();

    draw_state->show_nets = state;

    app->find_widget("ToggleNetType")->setEnabled(state);
    app->find_widget("ToggleInterClusterNets")->setEnabled(state);
    if (draw_state->is_flat || draw_state->draw_nets != DRAW_ROUTED_NETS) {
        app->find_widget("ToggleIntraClusterNets")->setEnabled(state);
    }
    app->find_widget("FanInFanOut")->setEnabled(state);
    app->find_widget("NetAlpha")->setEnabled(state);
    app->find_widget("NetMaxFanout")->setEnabled(state);

    app->refresh_drawing();
}

void toggle_draw_nets_cbk(QComboBox* self, ezgl::application* app) {
    enum e_draw_nets new_state;
    t_draw_state* draw_state = get_draw_state_vars();
    QString setting = self->currentText();

    // assign corresponding enum value to draw_state->show_nets
    if (setting == "Routing") {
        new_state = DRAW_ROUTED_NETS;

        // Make sure that intra-cluster routed nets is never enabled when flat routing is off
        if (!draw_state->is_flat) {
            app->find_widget("ToggleIntraClusterNets")->setEnabled(false);
            QCheckBox* checkbox = app->find_check_box("ToggleIntraClusterNets");
            if (checkbox) {
                checkbox->setChecked(false);
            }
            draw_state->draw_intra_cluster_nets = false;
        }

    } else { // Flylines - direct connections between sources and sinks
        new_state = DRAW_FLYLINES;

        app->find_widget("ToggleIntraClusterNets")->setEnabled(true);
    }

    draw_state->draw_nets = new_state;

    //redraw
    app->refresh_drawing();
}

/**
 * @brief cbk function for toggle rr combo-box. sets rr draw state based on selected option
 * updates draw_state->show_rr
 * 
 * @param self ptr to gtkComboBoxText object
 * @param app ezgl application
 */
void toggle_rr_cbk(SwitchButton*, bool state, ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();

    draw_state->show_rr = state;

    // Enable/disable the rr drawing sub-options based on the switch state
    app->find_widget("ToggleRRChannels")->setEnabled(state);
    app->find_widget("ToggleInterClusterPinNodes")->setEnabled(state);
    app->find_widget("ToggleRRSBox")->setEnabled(state);
    app->find_widget("ToggleRRCBox")->setEnabled(state);

    //currently intra-cluster nodes and edges are only supported if flat routing is enabled
    if (draw_state->is_flat) {
        app->find_widget("ToggleRRIntraClusterNodes")->setEnabled(state);
        app->find_widget("ToggleRRIntraClusterEdges")->setEnabled(state);
    }
    app->find_widget("ToggleHighlightRR")->setEnabled(state);

    app->refresh_drawing();
}

/**
 * @brief cbk function for toggle congestion comboBox. Updates shown cong. based on selected option
 * updates draw_state->show_congestion
 * 
 * @param self self ptr to QComboBox
 * @param app ezgl application
 */
void toggle_cong_cbk(QComboBox* self, ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();
    enum e_draw_congestion new_state;
    QString combo_box_content = self->currentText();
    if (combo_box_content == "None")
        new_state = DRAW_NO_CONGEST;
    else if (combo_box_content == "Congested")
        new_state = DRAW_CONGESTED;
    else
        // congested with nets
        new_state = DRAW_CONGESTED_WITH_NETS;

    draw_state->show_congestion = new_state;
    if (draw_state->show_congestion == DRAW_NO_CONGEST) {
        app->update_message(draw_state->default_message);
    }
    app->refresh_drawing();
}

/**
 * @brief cbk function for toggle cong. cost combobox. sets draws state according to new setting
 * updates draw_state->show_routing_costs
 * 
 * @param self self ptr
 * @param app ezgl app
 */
void toggle_cong_cost_cbk(QComboBox* self, ezgl::application* app) {
    /* this is the callback function for runtime created toggle_routing_congestion_cost button
     * which is written in button.cpp                                         */
    t_draw_state* draw_state = get_draw_state_vars();
    enum e_draw_routing_costs new_state;
    QString combo_box_content = self->currentText();
    if (combo_box_content == "None")
        new_state = DRAW_NO_ROUTING_COSTS;
    else if (combo_box_content == "Total Routing Costs")
        new_state = DRAW_TOTAL_ROUTING_COSTS;
    else if (combo_box_content == "Log Total Routing Costs")
        new_state = DRAW_LOG_TOTAL_ROUTING_COSTS;
    else if (combo_box_content == "Acc Routing Costs")
        new_state = DRAW_ACC_ROUTING_COSTS;
    else if (combo_box_content == "Log Acc Routing Costs")
        new_state = DRAW_LOG_ACC_ROUTING_COSTS;
    else if (combo_box_content == "Pres Routing Costs")
        new_state = DRAW_PRES_ROUTING_COSTS;
    else if (combo_box_content == "Log Pres Routing Costs")
        new_state = DRAW_LOG_PRES_ROUTING_COSTS;
    else
        new_state = DRAW_BASE_ROUTING_COSTS;

    draw_state->show_routing_costs = new_state;
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
void toggle_routing_bbox_cbk(QSpinBox* self, ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();
    const RoutingContext& route_ctx = g_vpr_ctx.routing();
    // get the pointer to the toggle_routing_bounding_box button

    if (route_ctx.route_bb.size() == 0)
        return; //Nothing to draw

    // use the pointer to get the active value
    int new_value = self->value();

    // assign value to draw_state->show_routing_bb, bound check + set UNDEFINED when it's -1 (draw nothing)
    if (new_value <= -1)
        draw_state->show_routing_bb = UNDEFINED;
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
void toggle_router_util_cbk(QComboBox* self, ezgl::application* app) {
    /* this is the callback function for runtime created toggle_routing_util button
     * which is written in button.cpp                                         */
    t_draw_state* draw_state = get_draw_state_vars();
    enum e_draw_routing_util new_state;

    QString combo_box_content = self->currentText();
    if (combo_box_content == "None")
        new_state = DRAW_NO_ROUTING_UTIL;
    else if (combo_box_content == "Routing Util")
        new_state = DRAW_ROUTING_UTIL;
    else if (combo_box_content == "Routing Util with Value")
        new_state = DRAW_ROUTING_UTIL_WITH_VALUE;
    else if (combo_box_content == "Routing Util with Formula")
        new_state = DRAW_ROUTING_UTIL_WITH_FORMULA;
    else
        new_state = DRAW_ROUTING_UTIL_OVER_BLOCKS;

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
void toggle_blk_internal_cbk(QSpinBox* self, ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();
    int new_value = self->value();
    if (new_value < 0)
        draw_state->show_blk_internal = 0;
    else if (new_value >= draw_state->max_sub_blk_lvl)
        draw_state->show_blk_internal = draw_state->max_sub_blk_lvl;
    else
        draw_state->show_blk_internal = new_value;
    app->refresh_drawing();
}

/**
 * @brief cbk function when pin util gets changed in ui; sets pin util drawing to new val
 * updates draw_state->show_blk_pin_util
 * 
 * @param self ptr to self
 * @param app ezgl::app
 */
void toggle_blk_pin_util_cbk(QComboBox* self, ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();
    QString combo_box_content = self->currentText();
    if (combo_box_content == "None") {
        draw_state->show_blk_pin_util = DRAW_NO_BLOCK_PIN_UTIL;
        draw_reset_blk_colors();
        app->update_message(draw_state->default_message);
    } else if (combo_box_content == "All")
        draw_state->show_blk_pin_util = DRAW_BLOCK_PIN_UTIL_TOTAL;
    else if (combo_box_content == "Inputs")
        draw_state->show_blk_pin_util = DRAW_BLOCK_PIN_UTIL_INPUTS;
    else
        draw_state->show_blk_pin_util = DRAW_BLOCK_PIN_UTIL_OUTPUTS;

    app->refresh_drawing();
}

/**
 * @brief cbk function when pin util gets changed in ui; sets pin util drawing to new val
 * updates draw_state->show_placement_macros
 * 
 * @param self ptr to self
 * @param app ezgl::app
 */
void placement_macros_cbk(QComboBox* self, ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();
    QString combo_box_content = self->currentText();
    if (combo_box_content == "None")
        draw_state->show_placement_macros = DRAW_NO_PLACEMENT_MACROS;
    else
        draw_state->show_placement_macros = DRAW_PLACEMENT_MACROS;

    app->refresh_drawing();
}

void toggle_crit_path_cbk(SwitchButton*, bool state, ezgl::application* app) {

    t_draw_state* draw_state = get_draw_state_vars();

    draw_state->show_crit_path = state;

    app->find_widget("ToggleCritPathFlylines")->setEnabled(state);
    app->find_widget("ToggleCritPathDelays")->setEnabled(state);

    if (draw_state->setup_timing_info && draw_state->pic_on_screen == e_pic_type::ROUTING) {
        app->find_widget("ToggleCritPathRouting")->setEnabled(state);
    }

    app->refresh_drawing();
}

/**
 * @brief cbk function for toggling routing expansion cost. 
 * alters draw_state->show_router_expansion cost var. to reflect new value
 * 
 * @param self self ptr
 * @param app ezgl::application
 */
void toggle_expansion_cost_cbk(QComboBox* self, ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();
    e_draw_router_expansion_cost new_state;
    QString combo_box_content = self->currentText();
    if (combo_box_content == "None") {
        new_state = DRAW_NO_ROUTER_EXPANSION_COST;
    } else if (combo_box_content == "Total") {
        new_state = DRAW_ROUTER_EXPANSION_COST_TOTAL;
    } else if (combo_box_content == "Known") {
        new_state = DRAW_ROUTER_EXPANSION_COST_KNOWN;
    } else if (combo_box_content == "Expected") {
        new_state = DRAW_ROUTER_EXPANSION_COST_EXPECTED;
    } else if (combo_box_content == "Total (with edges)") {
        new_state = DRAW_ROUTER_EXPANSION_COST_TOTAL_WITH_EDGES;
    } else if (combo_box_content == "Known (with edges)") {
        new_state = DRAW_ROUTER_EXPANSION_COST_KNOWN_WITH_EDGES;
    } else if (combo_box_content == "Expected (with edges)") {
        new_state = DRAW_ROUTER_EXPANSION_COST_EXPECTED_WITH_EDGES;
    } else {
        VPR_THROW(VPR_ERROR_DRAW, "Unrecognized draw RR cost option");
    }

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
void toggle_noc_cbk(QComboBox* self, ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();

    QString combo_box_content = self->currentText();
    if (combo_box_content == "None") {
        draw_state->draw_noc = DRAW_NO_NOC;
    } else if (combo_box_content == "NoC Links")
        draw_state->draw_noc = DRAW_NOC_LINKS;
    else
        draw_state->draw_noc = DRAW_NOC_LINK_USAGE;

    app->refresh_drawing();
}

/**
 * @brief CBK for Net Max Fanout spin button. Sets max fanout when val. changes
 * updates draw_state->draw_net_max_fanout
 * 
 * @param self self ptr to QSpinBox
 * @param app ezgl::app
 */
void set_net_max_fanout_cbk(QSpinBox* self, ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();
    draw_state->draw_net_max_fanout = self->value();
    app->refresh_drawing();
}

/**
 * @brief Set the net alpha value (transparency) based on value of spin button
 * updates draw_state->net_alpha
 * 
 * @param self 
 * @param app 
 */
void set_net_alpha_value_cbk(QSpinBox* self, ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();
    draw_state->net_alpha = 255 - self->value();
    app->refresh_drawing();
}

/**
 * @brief Callback function for 3d layer checkboxes
 */
void select_layer_cbk(QWidget* widget, int /*response_id*/, void* /*data*/) {
    t_draw_state* draw_state = get_draw_state_vars();
    int index = 0;
    for (QCheckBox* checkbox : widget->parentWidget()->findChildren<QCheckBox*>(QString(), Qt::FindDirectChildrenOnly)) {
        const QString label = checkbox->text();
        if (label.contains("Layer") && !label.contains("Cross")) {
            draw_state->draw_layer_display[index].visible = checkbox->isChecked();
            index++;
        }
    }
    application->refresh_drawing();
}
/**
 * @brief Callback function for 3d layer transparency spin buttons
 */
void transparency_cbk(QWidget* widget, int /*response_id*/, void* /*data*/) {
    t_draw_state* draw_state = get_draw_state_vars();
    int index = 0;
    for (QSpinBox* spin_button : widget->parentWidget()->findChildren<QSpinBox*>(QString(), Qt::FindDirectChildrenOnly)) {
        const QString name = spin_button->objectName();
        if (name.contains("Transparency") && !name.contains("Cross")) {
            draw_state->draw_layer_display[index].alpha = 255 - spin_button->value();
            index++;
        }
    }
    application->refresh_drawing();
}

/**
 * @brief Callback function for cross layer connection checkbox
 */
void cross_layer_checkbox_cbk(QCheckBox* checkbox, int /*response_id*/, void* /*data*/) {
    t_draw_state* draw_state = get_draw_state_vars();

    draw_state->cross_layer_display.visible = checkbox->isChecked();
    application->refresh_drawing();
}

/**
 * @brief Callback function for cross layer connection spin button
 */
void cross_layer_transparency_cbk(QSpinBox* spinbox, int /*response_id*/, void* /*data*/) {
    t_draw_state* draw_state = get_draw_state_vars();

    int value = spinbox->value();
    draw_state->cross_layer_display.alpha = 255 - value;

    application->refresh_drawing();
}
#endif
