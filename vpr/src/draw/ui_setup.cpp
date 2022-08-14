#ifndef NO_GRAPHICS
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

#    include "draw_global.h"
#    include "draw.h"
#    include "draw_toggle_functions.h"
#    include "buttons.h"
#    include "intra_logic_block.h"
#    include "clustered_netlist.h"
#    include "ui_setup.h"
#    include "save_graphics.h"

#    include "ezgl/point.hpp"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"

/**
 * @brief configures basic buttons
 * 
 * Sets up Window, Search, Save, and SearchType buttons. Buttons are 
 * created in glade main.ui file. Connects them to their cbk functions
 * @param app ezgl::application*
 */
void basic_button_setup(ezgl::application* app) {
    //button to enter window_mode, created in main.ui
    GtkButton* window = (GtkButton*)app->get_object("Window");
    gtk_button_set_label(window, "Window");
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

/**
 * @brief sets up net related buttons and connects their signals
 * 
 * Sets up the toggle nets combo box, net alpha spin button, and max fanout
 * spin button which are created in main.ui file. Found in Net Settings dropdown
 * @param app 
 */
void net_button_setup(ezgl::application* app) {
    //Toggle net signal connection
    GtkComboBoxText* toggle_nets = GTK_COMBO_BOX_TEXT(app->get_object("ToggleNets"));
    g_signal_connect(toggle_nets, "changed", G_CALLBACK(toggle_nets_cbk), app);

    //Manages net alpha
    GtkSpinButton* net_alpha = GTK_SPIN_BUTTON(app->get_object("NetAlpha"));
    g_signal_connect(net_alpha, "value-changed", G_CALLBACK(set_net_alpha_value_cbk), app);
    gtk_spin_button_set_increments(net_alpha, 1, 1);
    gtk_spin_button_set_range(net_alpha, 1, 255);

    //Manages net max fanout
    GtkSpinButton* max_fanout = GTK_SPIN_BUTTON(app->get_object("NetMaxFanout"));
    g_signal_connect(max_fanout, "value-changed", G_CALLBACK(set_net_max_fanout_cbk), app);
    gtk_spin_button_set_increments(max_fanout, 1, 1);
    gtk_spin_button_set_range(max_fanout, 0., (double)get_max_fanout());
}

/**
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
    gtk_spin_button_set_range(blk_internals_button, 0., (double)(draw_state->max_sub_blk_lvl - 1));

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
 * @param app 
 */
void routing_button_setup(ezgl::application* app) {
    auto& route_ctx = g_vpr_ctx.routing();

    //Toggle RR
    GtkComboBoxText* toggle_rr_box = GTK_COMBO_BOX_TEXT(app->get_object("ToggleRR"));
    g_signal_connect(toggle_rr_box, "changed", G_CALLBACK(toggle_rr_cbk), app);

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

/**
 * @brief Loads required data for search autocomplete, sets up special completion fn
 * 
 * @param app ezgl app
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
    GtkComboBoxText* toggle_crit_path = GTK_COMBO_BOX_TEXT(app->get_object("ToggleCritPath"));
    g_signal_connect(toggle_crit_path, "changed", G_CALLBACK(toggle_crit_path_cbk), app);
    show_widget("ToggleCritPath", app);
}

/**
 * @brief hides critical path button
 * 
 * @param app ezgl app
 */
void hide_crit_path_button(ezgl::application* app) {
    hide_widget("CritPathLabel", app);
    hide_widget("ToggleCritPath", app);
}

/**
 * @brief Hides the widget with the given name
 * 
 * @param widgetName string of widget name in main.ui
 * @param app ezgl app
 */
void hide_widget(std::string widgetName, ezgl::application* app) {
    GtkWidget* widget = GTK_WIDGET(app->get_object(widgetName.c_str()));
    gtk_widget_hide(widget);
}

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
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.atom();
    GtkTreeIter iter;
    int i = 0;
    for (ClusterBlockId id : cluster_ctx.clb_nlist.blocks()) {
        gtk_list_store_append(blockStorage, &iter);
        gtk_list_store_set(blockStorage, &iter,
                           0, (cluster_ctx.clb_nlist.block_name(id)).c_str(), -1);
        i++;
    }
    for (AtomBlockId id : atom_ctx.nlist.blocks()) {
        gtk_list_store_append(blockStorage, &iter);
        gtk_list_store_set(blockStorage, &iter,
                           0, (atom_ctx.nlist.block_name(id)).c_str(), -1);
        i++;
    }
}

/**
 * @brief loads atom net names into gtk list store item used for completion
 * 
 * @param app ezgl application used for ui
 */
void load_net_names(ezgl::application* app) {
    auto netStorage = GTK_LIST_STORE(app->get_object("NetNames"));
    auto& atom_ctx = g_vpr_ctx.atom();
    GtkTreeIter iter;
    //Loading net names
    int i = 0;
    for (AtomNetId id : atom_ctx.nlist.nets()) {
        gtk_list_store_append(netStorage, &iter);
        gtk_list_store_set(netStorage, &iter,
                           0, (atom_ctx.nlist.net_name(id)).c_str(), -1);
        i++;
    }
}

#endif /* NO_GRAPHICS */
