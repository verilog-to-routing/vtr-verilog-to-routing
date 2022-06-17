#ifndef NO_GRAPHICS

/*
 * This file defines all runtime created spin buttons, combo boxes, and labels. All 
 * button_for_toggle_X() functions are called in initial_setup_X functions in draw.cpp.
 * Each function creates a label and a combo box/spin button and connects the signal to 
 * the corresponding toggle_X callback functions, which are also defined in draw.cpp.
 * 
 * Authors: Dingyu (Tina) Yang
 * Last updated: Aug 2019
 */

#    include "draw_global.h"
#    include "draw.h"
#    include "draw_toggle_functions.h"
#    include "buttons.h"
#    include "intra_logic_block.h"
#    include "clustered_netlist.h"
#    include "ui.h"
#    include "save_graphics.h"

#    include "ezgl/point.hpp"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"


static size_t get_max_fanout();

/**
 * @brief configures basic buttons, including window, search, save etc..
 * 
 * @param app ezgl::application*
 */
void basic_button_setup(ezgl::application* app){
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

static size_t get_max_fanout(){
    //find maximum fanout
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& clb_nlist = cluster_ctx.clb_nlist;
    size_t max_fanout = 0;
    for (ClusterNetId net_id : clb_nlist.nets())
        max_fanout = std::max(max_fanout, clb_nlist.net_sinks(net_id).size());

    auto& atom_ctx = g_vpr_ctx.atom();
    auto& atom_nlist = atom_ctx.nlist;
    size_t max_fanout2 = 0;
    for (AtomNetId net_id : atom_nlist.nets())
        max_fanout2 = std::max(max_fanout2, atom_nlist.net_sinks(net_id).size());

    size_t max = std::max(max_fanout2, max_fanout);
    return max;
}

/**
 * @brief sets up net related buttons and connects their signals
 * 
 * @param app 
 */
void net_button_setup(ezgl::application* app){
    //Toggle net signal connection
    GtkComboBoxText* toggle_nets = GTK_COMBO_BOX_TEXT(app->get_object("ToggleNets"));
    g_signal_connect(toggle_nets, "changed", G_CALLBACK(toggle_nets_cbk), app);

    //Manages net alpha
    GtkSpinButton* net_alpha = GTK_SPIN_BUTTON(app->get_object("NetAlpha"));
    g_signal_connect(net_alpha, "value-changed", G_CALLBACK(set_net_alpha_value), app);
    gtk_spin_button_set_increments(net_alpha, 1, 1);
    gtk_spin_button_set_range(net_alpha, 1, 255);

    //Manages net max fanout
    GtkSpinButton* max_fanout = GTK_SPIN_BUTTON(app->get_object("NetMaxFanout"));
    g_signal_connect(max_fanout, "value_changed", G_CALLBACK(set_net_max_fanout), app);
    gtk_spin_button_set_increments(max_fanout, 1, 1);
    gtk_spin_button_set_range(max_fanout, 0, (int)get_max_fanout());
}


#endif /* NO_GRAPHICS */
