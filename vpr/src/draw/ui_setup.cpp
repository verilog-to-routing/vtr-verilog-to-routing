/**
 * @file UI_SETUP.CPP
 * @author Sebastian Lievano
 * @date July 4th, 2022
 * @brief Manages setup for main.ui created buttons
 *
 * This file contains the various setup functions for all of the ui functions.
 * As of June 2022, gtk ui items are to be created through Glade/main.ui file (see Docs)
 * Each function here initializes a different set of ui buttons, connecting their callback functions
 */

#ifndef NO_GRAPHICS

#include "clustered_netlist.h"
#include "draw.h"
#include "draw_global.h"
#include "draw_toggle_functions.h"
#include "save_graphics.h"
#include "search_bar.h"
#include "ui_setup.h"
#include "gtkcomboboxhelper.h"

#include "ezgl/application.hpp"

/**
 * @brief Helper function to connect a toggle button to a callback function
 */
static void setup_checkbox_button(std::string button_id, ezgl::application* app, bool* toggle_state) {
    t_draw_state* draw_state = get_draw_state_vars();
    GtkToggleButton* checkbox_button = GTK_TOGGLE_BUTTON(app->get_object(button_id.c_str()));
    draw_state->checkbox_data.emplace_back(app, toggle_state);
    g_signal_connect(checkbox_button, "toggled", G_CALLBACK(toggle_checkbox_cbk), &draw_state->checkbox_data.back());
}

void basic_button_setup(ezgl::application* app) {
    //button to enter window_mode, created in main.ui
    GtkButton* window = (GtkButton*)app->get_object("Window");
    g_signal_connect(window, "clicked", G_CALLBACK(toggle_window_mode), app);

    //button to search, created in main.ui
    GtkButton* search = (GtkButton*)app->get_object("Search");
    gtk_button_set_label(search, "Search");
    g_signal_connect(search, "clicked", G_CALLBACK(search_and_highlight), app);

    //button for save graphcis, created in main.ui
    GtkButton* save = (GtkButton*)app->get_object("SaveGraphics");
    g_signal_connect(save, "clicked", G_CALLBACK(save_graphics_dialog_box),
                     app);

    //combo box for search type, created in main.ui
    GObject* search_type = (GObject*)app->get_object("SearchType");
    g_signal_connect(search_type, "changed", G_CALLBACK(search_type_changed), app);
}

/*
 * @brief sets up net related buttons and connects their signals
 *
 * Sets up the toggle nets combo box, net alpha spin button, and max fanout
 * spin button which are created in main.ui file. Found in Net Settings dropdown
 * @param app ezgl::application ptr
 */
void net_button_setup(ezgl::application* app) {

    t_draw_state* draw_state = get_draw_state_vars();

    GtkSwitch* toggle_nets_switch = GTK_SWITCH(app->get_object("ToggleNets"));
    g_signal_connect(toggle_nets_switch, "state-set", G_CALLBACK(toggle_show_nets_cbk), app);

    // Manages net type
    GtkComboBoxText* toggle_nets = GTK_COMBO_BOX_TEXT(app->get_object("ToggleNetType"));
    g_signal_connect(toggle_nets, "changed", G_CALLBACK(toggle_draw_nets_cbk), app);

    setup_checkbox_button("ToggleInterClusterNets", app, &draw_state->draw_inter_cluster_nets);

    setup_checkbox_button("ToggleIntraClusterNets", app, &draw_state->draw_intra_cluster_nets);

    setup_checkbox_button("FanInFanOut", app, &draw_state->highlight_fan_in_fan_out);

    //Manages net alpha
    GtkSpinButton* net_alpha = GTK_SPIN_BUTTON(app->get_object("NetAlpha"));
    g_signal_connect(net_alpha, "value-changed", G_CALLBACK(set_net_alpha_value_cbk), app);
    gtk_spin_button_set_increments(net_alpha, 1, 1);
    gtk_spin_button_set_range(net_alpha, 0, 255);

    //Manages net max fanout
    GtkSpinButton* max_fanout = GTK_SPIN_BUTTON(app->get_object("NetMaxFanout"));
    g_signal_connect(max_fanout, "value-changed", G_CALLBACK(set_net_max_fanout_cbk), app);
    gtk_spin_button_set_increments(max_fanout, 1, 1);
    gtk_spin_button_set_range(max_fanout, 0., (double)get_max_fanout());
}

/*
 * @brief sets up block related buttons, connects their signals
 *
 * Connects signals and sets init. values for blk internals spin button,
 * blk pin util combo box,placement macros combo box, and noc combo bx created in
 * main.ui. Found in Block Settings dropdown
 * @param app
 */
void block_button_setup(ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();

    //Toggle block internals
    GtkSpinButton* blk_internals_button = GTK_SPIN_BUTTON(app->get_object("ToggleBlkInternals"));
    g_signal_connect(blk_internals_button, "value-changed", G_CALLBACK(toggle_blk_internal_cbk), app);
    gtk_spin_button_set_increments(blk_internals_button, 1, 1);
    gtk_spin_button_set_range(blk_internals_button, 0., (double)(draw_state->max_sub_blk_lvl));

    //Toggle Block Pin Util
    GtkComboBoxText* blk_pin_util = GTK_COMBO_BOX_TEXT(app->get_object("ToggleBlkPinUtil"));
    g_signal_connect(blk_pin_util, "changed", G_CALLBACK(toggle_blk_pin_util_cbk), app);

    //Toggle Placement Macros
    GtkComboBoxText* placement_macros = GTK_COMBO_BOX_TEXT(app->get_object("TogglePlacementMacros"));
    g_signal_connect(placement_macros, "changed", G_CALLBACK(placement_macros_cbk), app);

    //Toggle NoC Display (based on startup cmd --noc on)
    if (!draw_state->show_noc_button) {
        hide_widget("NocLabel", app);
        hide_widget("ToggleNocBox", app);
    } else {
        GtkComboBoxText* toggleNocBox = GTK_COMBO_BOX_TEXT(app->get_object("ToggleNocBox"));
        g_signal_connect(toggleNocBox, "changed", G_CALLBACK(toggle_noc_cbk), app);
    }
}

/**
 * @brief configures and connects signals/functions for routing buttons
 *
 * Connects signals/sets default values for toggleRRButton, ToggleCongestion,
 * ToggleCongestionCost, ToggleRoutingBBox, RoutingExpansionCost, ToggleRoutingUtil
 * buttons.
 */
void routing_button_setup(ezgl::application* app) {
    const RoutingContext& route_ctx = g_vpr_ctx.routing();
    t_draw_state* draw_state = get_draw_state_vars();

    //Toggle RR
    GtkSwitch* toggle_nets_switch = GTK_SWITCH(app->get_object("ToggleRR"));
    g_signal_connect(toggle_nets_switch, "state-set", G_CALLBACK(toggle_rr_cbk), app);

    // RR Checkboxes

    setup_checkbox_button("ToggleRRChannels", app, &draw_state->draw_channel_nodes);
    setup_checkbox_button("ToggleInterClusterPinNodes", app, &draw_state->draw_inter_cluster_pins);
    setup_checkbox_button("ToggleRRIntraClusterNodes", app, &draw_state->draw_intra_cluster_nodes);
    setup_checkbox_button("ToggleRRSBox", app, &draw_state->draw_switch_box_edges);
    setup_checkbox_button("ToggleRRCBox", app, &draw_state->draw_connection_box_edges);
    setup_checkbox_button("ToggleRRIntraClusterEdges", app, &draw_state->draw_intra_cluster_edges);
    setup_checkbox_button("ToggleHighlightRR", app, &draw_state->highlight_rr_edges);

    //Toggle Congestion
    GtkComboBoxText* toggle_congestion = GTK_COMBO_BOX_TEXT(app->get_object("ToggleCongestion"));
    g_signal_connect(toggle_congestion, "changed", G_CALLBACK(toggle_cong_cbk), app);

    //Toggle Congestion Cost
    GtkComboBoxText* toggle_cong_cost = GTK_COMBO_BOX_TEXT(app->get_object("ToggleCongestionCost"));
    g_signal_connect(toggle_cong_cost, "changed", G_CALLBACK(toggle_cong_cost_cbk), app);

    //Toggle Routing BB
    GtkSpinButton* toggle_routing_bbox = GTK_SPIN_BUTTON(app->get_object("ToggleRoutingBBox"));
    g_signal_connect(toggle_routing_bbox, "value-changed", G_CALLBACK(toggle_routing_bbox_cbk), app);
    gtk_spin_button_set_increments(toggle_routing_bbox, 1, 1);
    gtk_spin_button_set_range(toggle_routing_bbox, -1., (double)(route_ctx.route_bb.size() - 1));
    gtk_spin_button_set_value(toggle_routing_bbox, -1.);

    //Toggle Routing Expansion Costs
    GtkComboBoxText* toggle_expansion_cost = GTK_COMBO_BOX_TEXT(app->get_object("ToggleRoutingExpansionCost"));
    g_signal_connect(toggle_expansion_cost, "changed", G_CALLBACK(toggle_expansion_cost_cbk), app);

    //Toggle Router Util
    GtkComboBoxText* toggle_router_util = GTK_COMBO_BOX_TEXT(app->get_object("ToggleRoutingUtil"));
    g_signal_connect(toggle_router_util, "changed", G_CALLBACK(toggle_router_util_cbk), app);
    show_widget("RoutingMenuButton", app);
}

void view_button_setup(ezgl::application* app) {
    int num_layers;

    const DeviceContext& device_ctx = g_vpr_ctx.device();
    num_layers = device_ctx.grid.get_num_layers();

    // Hide the button if we only have one layer
    if (num_layers == 1) {
        hide_widget("3DMenuButton", app);
    } else {
        GtkBox* box = GTK_BOX(app->get_object("LayerBox"));
        GtkBox* trans_box = GTK_BOX(app->get_object("TransparencyBox"));

        // Create checkboxes and spin buttons for each layer
        for (int i = 0; i < num_layers; i++) {
            std::string label = "Layer " + std::to_string(i);
            std::string trans_label = "Transparency " + std::to_string(i);

            GtkWidget* checkbox = gtk_check_button_new_with_label(label.c_str());
            // Add margins to checkboxes to match the transparency spin button height
            gtk_widget_set_margin_top(checkbox, 7);
            gtk_widget_set_margin_bottom(checkbox, 7);

            gtk_box_pack_start(GTK_BOX(box), checkbox, FALSE, FALSE, 0);

            GtkWidget* spin_button = gtk_spin_button_new_with_range(0, 255, 1);
            gtk_widget_set_name(spin_button, g_strdup(trans_label.c_str()));
            gtk_box_pack_start(GTK_BOX(trans_box), spin_button, FALSE, FALSE, 0);

            if (i == 0) {
                // Set the initial state of the first checkbox to checked to represent the default view.
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox), TRUE);
            }

            g_signal_connect(checkbox, "toggled", G_CALLBACK(select_layer_cbk), app);
            g_signal_connect(spin_button, "value-changed", G_CALLBACK(transparency_cbk), app);
        }

        // Set up the final row for cross-layer connections
        std::string label = "Cross Layer Connections";
        std::string trans_label = "CrossLayerConnectionsTransparency";

        GtkWidget* checkbox = gtk_check_button_new_with_label(label.c_str());
        gtk_widget_set_margin_top(checkbox, 7);
        gtk_widget_set_margin_bottom(checkbox, 7);
        gtk_box_pack_start(GTK_BOX(box), checkbox, FALSE, FALSE, 0);

        GtkWidget* spin_button = gtk_spin_button_new_with_range(0, 255, 1);
        gtk_widget_set_name(spin_button, g_strdup(trans_label.c_str()));
        gtk_box_pack_start(GTK_BOX(trans_box), spin_button, FALSE, FALSE, 0);

        // Connect cross layer to callback function:
        g_signal_connect(checkbox, "toggled", G_CALLBACK(cross_layer_checkbox_cbk), app);
        g_signal_connect(spin_button, "value-changed", G_CALLBACK(cross_layer_transparency_cbk), app);

        // Make all widgets in the boxes appear
        gtk_widget_show_all(GTK_WIDGET(box));
        gtk_widget_show_all(GTK_WIDGET(trans_box));
    }
}

/*
 * @brief Loads required data for search autocomplete, sets up special completion fn
 */
void search_setup(ezgl::application* app) {
    load_block_names(app);
    load_net_names(app);
    //Setting custom matching function for entry completion (searches whole string instead of start)
    GtkEntryCompletion* wildcardComp = GTK_ENTRY_COMPLETION(app->get_object("Completion"));
    gtk_entry_completion_set_match_func(wildcardComp, (GtkEntryCompletionMatchFunc)customMatchingFunction, NULL, NULL);
}

/**
 * @brief connects critical path button to its cbk fn
 *
 * @param app ezgl application
 */
void crit_path_button_setup(ezgl::application* app) {

    t_draw_state* draw_state = get_draw_state_vars();

    // Toggle Critical Path
    GtkSwitch* toggle_nets_switch = GTK_SWITCH(app->get_object("ToggleCritPath"));
    g_signal_connect(toggle_nets_switch, "state-set", G_CALLBACK(toggle_crit_path_cbk), app);

    // Checkboxes for critical path
    setup_checkbox_button("ToggleCritPathFlylines", app, &draw_state->show_crit_path_flylines);
    setup_checkbox_button("ToggleCritPathRouting", app, &draw_state->show_crit_path_routing);
    setup_checkbox_button("ToggleCritPathDelays", app, &draw_state->show_crit_path_delays);
}

/**
 * @brief Hides or displays critical path routing / routing delay UI elements
 *
 * @param app ezgl app
 */
void hide_crit_path_routing(ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();
    bool state = draw_state->setup_timing_info && draw_state->pic_on_screen == e_pic_type::ROUTING && draw_state->show_crit_path;

    gtk_widget_set_sensitive(GTK_WIDGET(app->get_object("ToggleCritPathRouting")), state);
}

void hide_draw_routing(ezgl::application* app) {
    t_draw_state* draw_state = get_draw_state_vars();
    GtkComboBoxText* toggle_nets = GTK_COMBO_BOX_TEXT(app->get_object("ToggleNetType"));

    // Enable the option to draw routing only during the routing stage
    int route_item_index = get_item_index_by_text(toggle_nets, "Routing");
    if (draw_state->pic_on_screen == e_pic_type::PLACEMENT) {
        if (route_item_index != -1) {
            gtk_combo_box_text_remove(toggle_nets, route_item_index);
        }
    } else {
        if (route_item_index == -1) {
            gtk_combo_box_text_append(toggle_nets, "2", "Routing");
        }
    }
}

/*
 * @brief Hides the widget with the given name
 *
 * @param widgetName string of widget name in main.ui
 * @param app ezgl app
 */
void hide_widget(std::string widgetName, ezgl::application* app) {
    GtkWidget* widget = GTK_WIDGET(app->get_object(widgetName.c_str()));
    gtk_widget_hide(widget);
}

/**
 * @brief Hides the widget with the given name
 */
void show_widget(std::string widgetName, ezgl::application* app) {
    GtkWidget* widget = GTK_WIDGET(app->get_object(widgetName.c_str()));
    gtk_widget_show(widget);
}

/**
 * @brief loads atom and cluster lvl names into gtk list store item used for completion
 *
 * @param app ezgl application used for ui
 */
void load_block_names(ezgl::application* app) {
    auto blockStorage = GTK_LIST_STORE(app->get_object("BlockNames"));
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    GtkTreeIter iter;
    for (ClusterBlockId id : cluster_ctx.clb_nlist.blocks()) {
        gtk_list_store_append(blockStorage, &iter);
        gtk_list_store_set(blockStorage, &iter,
                           0, (cluster_ctx.clb_nlist.block_name(id)).c_str(), -1);
    }
    for (AtomBlockId id : atom_ctx.netlist().blocks()) {
        gtk_list_store_append(blockStorage, &iter);
        gtk_list_store_set(blockStorage, &iter,
                           0, (atom_ctx.netlist().block_name(id)).c_str(), -1);
    }
}

/*
 * @brief loads atom net names into gtk list store item used for completion
 *
 * @param app ezgl application used for ui
 */
void load_net_names(ezgl::application* app) {
    auto netStorage = GTK_LIST_STORE(app->get_object("NetNames"));
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    GtkTreeIter iter;
    //Loading net names
    for (AtomNetId id : atom_ctx.netlist().nets()) {
        gtk_list_store_append(netStorage, &iter);
        gtk_list_store_set(netStorage, &iter,
                           0, (atom_ctx.netlist().net_name(id)).c_str(), -1);
    }
}

#endif /* NO_GRAPHICS */
