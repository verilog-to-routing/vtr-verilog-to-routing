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
#    include "buttons.h"
#    include "intra_logic_block.h"

#    include "ezgl/point.hpp"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"

//location of spin buttons, combo boxes, and labels on grid
gint box_width = 1;
gint box_height = 1;
gint label_left_start_col = 0;
gint box_left_start_col = 0;
gint button_row = 2; // 2 is the row num of the window button in main.ui, add buttons starting from this row

void button_for_toggle_nets() {
    GObject* main_window = application.get_object(application.get_main_window_id().c_str());
    GObject* main_window_grid = application.get_object("InnerGrid");

    //combo box for toggle_nets
    GtkWidget* toggle_nets_widget = gtk_combo_box_text_new();
    GtkWidget* toggle_nets_label = gtk_label_new("Toggle Nets:");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_nets_widget), "None");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_nets_widget), "Nets");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_nets_widget), "Logical Connections");
    gtk_combo_box_set_active((GtkComboBox*)toggle_nets_widget, 0); // default set to None which has an index 0
    gtk_widget_set_name(toggle_nets_widget, "toggle_nets");

    //attach to the grid
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_nets_label, label_left_start_col, button_row++, box_width, box_height);
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_nets_widget, box_left_start_col, button_row++, box_width, box_height);

    //show newly added contents
    gtk_widget_show_all((GtkWidget*)main_window);

    //connect signals
    g_signal_connect_swapped(GTK_COMBO_BOX_TEXT(toggle_nets_widget),
                             "changed",
                             G_CALLBACK(toggle_nets),
                             toggle_nets_widget);
}

void button_for_toggle_blk_internal() {
    GObject* main_window = application.get_object(application.get_main_window_id().c_str());
    GObject* main_window_grid = application.get_object("InnerGrid");
    t_draw_state* draw_state = get_draw_state_vars();

    //spin box for toggle_blk_internal, set the range and increment step
    GtkWidget* toggle_blk_internal_widget = gtk_spin_button_new_with_range(0., (double)(draw_state->max_sub_blk_lvl - 1), 1.);
    GtkWidget* toggle_blk_internal_label = gtk_label_new("Toggle Block Internal:");
    gtk_widget_set_name(toggle_blk_internal_widget, "toggle_blk_internal");

    //attach to the grid
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_blk_internal_label, label_left_start_col, button_row++, box_width, box_height);
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_blk_internal_widget, box_left_start_col, button_row++, box_width, box_height);

    //show newly added contents
    gtk_widget_show_all((GtkWidget*)main_window);

    //connect signals
    g_signal_connect_swapped((GtkSpinButton*)toggle_blk_internal_widget,
                             "value_changed",
                             G_CALLBACK(toggle_blk_internal),
                             toggle_blk_internal_widget);
}

void button_for_toggle_block_pin_util() {
    GObject* main_window = application.get_object(application.get_main_window_id().c_str());
    GObject* main_window_grid = application.get_object("InnerGrid");

    //combo box for toggle_block_pin_util
    GtkWidget* toggle_block_pin_util_widget = gtk_combo_box_text_new();
    GtkWidget* toggle_block_pin_util_label = gtk_label_new("Toggle Block Pin Util:");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_block_pin_util_widget), "None");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_block_pin_util_widget), "All");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_block_pin_util_widget), "Inputs");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_block_pin_util_widget), "Outputs");
    gtk_combo_box_set_active((GtkComboBox*)toggle_block_pin_util_widget, 0); // default set to None which has an index 0
    gtk_widget_set_name(toggle_block_pin_util_widget, "toggle_block_pin_util");

    //attach to the grid
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_block_pin_util_label, label_left_start_col, button_row++, box_width, box_height);
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_block_pin_util_widget, box_left_start_col, button_row++, box_width, box_height);

    //show newly added contents
    gtk_widget_show_all((GtkWidget*)main_window);

    //connect signals
    g_signal_connect_swapped(GTK_COMBO_BOX_TEXT(toggle_block_pin_util_widget),
                             "changed",
                             G_CALLBACK(toggle_block_pin_util),
                             toggle_block_pin_util_widget);
}

void button_for_toggle_placement_macros() {
    GObject* main_window = application.get_object(application.get_main_window_id().c_str());
    GObject* main_window_grid = application.get_object("InnerGrid");

    //combo box for toggle_placement_macros
    GtkWidget* toggle_placement_macros_widget = gtk_combo_box_text_new();
    GtkWidget* toggle_placement_macros_label = gtk_label_new("Toggle Placement Macros:");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_placement_macros_widget), "None");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_placement_macros_widget), "Regular");
    gtk_combo_box_set_active((GtkComboBox*)toggle_placement_macros_widget, 0); // default set to None which has an index 0
    gtk_widget_set_name(toggle_placement_macros_widget, "toggle_placement_macros");

    //attach to the grid
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_placement_macros_label, label_left_start_col, button_row++, box_width, box_height);
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_placement_macros_widget, box_left_start_col, button_row++, box_width, box_height);

    //show newly added contents
    gtk_widget_show_all((GtkWidget*)main_window);

    //connect signals
    g_signal_connect_swapped(GTK_COMBO_BOX_TEXT(toggle_placement_macros_widget),
                             "changed",
                             G_CALLBACK(toggle_placement_macros),
                             toggle_placement_macros_widget);
}

void button_for_toggle_crit_path() {
    GObject* main_window = application.get_object(application.get_main_window_id().c_str());
    GObject* main_window_grid = application.get_object("InnerGrid");
    t_draw_state* draw_state = get_draw_state_vars();

    //combo box for toggle_crit_path
    GtkWidget* toggle_crit_path_widget = gtk_combo_box_text_new();
    GtkWidget* toggle_crit_path_label = gtk_label_new("Toggle Crit. Path:");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_crit_path_widget), "None");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_crit_path_widget), "Crit Path Flylines");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_crit_path_widget), "Crit Path Flylines Delays");
    if (draw_state->pic_on_screen == ROUTING) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_crit_path_widget), "Crit Path Routing");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_crit_path_widget), "Crit Path Routing Delays");
    }
    gtk_combo_box_set_active((GtkComboBox*)toggle_crit_path_widget, 0); // default set to None which has an index 0
    gtk_widget_set_name(toggle_crit_path_widget, "toggle_crit_path");

    //attach to the grid
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_crit_path_label, label_left_start_col, button_row++, box_width, box_height);
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_crit_path_widget, box_left_start_col, button_row++, box_width, box_height);

    //show newly added contents
    gtk_widget_show_all((GtkWidget*)main_window);

    //connect signals
    g_signal_connect_swapped(GTK_COMBO_BOX_TEXT(toggle_crit_path_widget),
                             "changed",
                             G_CALLBACK(toggle_crit_path),
                             toggle_crit_path_widget);
}

void button_for_toggle_rr() {
    GObject* main_window = application.get_object(application.get_main_window_id().c_str());
    GObject* main_window_grid = application.get_object("InnerGrid");

    GtkWidget* toggle_rr_widget = gtk_combo_box_text_new();
    GtkWidget* toggle_rr_label = gtk_label_new("Toggle RR:");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_rr_widget), "None");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_rr_widget), "Nodes RR");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_rr_widget), "Nodes and SBox RR");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_rr_widget), "All but Buffers RR");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_rr_widget), "All RR");
    gtk_combo_box_set_active((GtkComboBox*)toggle_rr_widget, 0); // default set to None which has an index 0
    gtk_widget_set_name(toggle_rr_widget, "toggle_rr");

    //attach to the grid
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_rr_label, label_left_start_col, button_row++, box_width, box_height);
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_rr_widget, box_left_start_col, button_row++, box_width, box_height);

    //show newly added contents
    gtk_widget_show_all((GtkWidget*)main_window);

    //connect signals
    g_signal_connect_swapped(GTK_COMBO_BOX_TEXT(toggle_rr_widget),
                             "changed",
                             G_CALLBACK(toggle_rr),
                             toggle_rr_widget);
}

void button_for_toggle_congestion() {
    GObject* main_window = application.get_object(application.get_main_window_id().c_str());
    GObject* main_window_grid = application.get_object("InnerGrid");

    GtkWidget* toggle_congestion_widget = gtk_combo_box_text_new();
    GtkWidget* toggle_congestion_label = gtk_label_new("Toggle Congestion:");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_congestion_widget), "None");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_congestion_widget), "Congested");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_congestion_widget), "Congested with Nets");
    gtk_combo_box_set_active((GtkComboBox*)toggle_congestion_widget, 0); // default set to None which has an index 0
    gtk_widget_set_name(toggle_congestion_widget, "toggle_congestion");

    //attach to the grid
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_congestion_label, label_left_start_col, button_row++, box_width, box_height);
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_congestion_widget, box_left_start_col, button_row++, box_width, box_height);

    //show newly added contents
    gtk_widget_show_all((GtkWidget*)main_window);

    //connect signals
    g_signal_connect_swapped(GTK_COMBO_BOX_TEXT(toggle_congestion_widget),
                             "changed",
                             G_CALLBACK(toggle_congestion),
                             toggle_congestion_widget);
}

void button_for_toggle_congestion_cost() {
    GObject* main_window = application.get_object(application.get_main_window_id().c_str());
    GObject* main_window_grid = application.get_object("InnerGrid");

    GtkWidget* toggle_routing_congestion_cost_widget = gtk_combo_box_text_new();
    GtkWidget* toggle_routing_congestion_cost_label = gtk_label_new("Toggle Routing Cong Cost:");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_routing_congestion_cost_widget), "None");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_routing_congestion_cost_widget), "Total Routing Costs");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_routing_congestion_cost_widget), "Log Total Routing Costs");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_routing_congestion_cost_widget), "Acc Routing Costs");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_routing_congestion_cost_widget), "Log Acc Routing Costs");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_routing_congestion_cost_widget), "Pres Routing Costs");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_routing_congestion_cost_widget), "Log Pres Routing Costs");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_routing_congestion_cost_widget), "Base Routing Costs");
    gtk_combo_box_set_active((GtkComboBox*)toggle_routing_congestion_cost_widget, 0); // default set to None which has an index 0
    gtk_widget_set_name(toggle_routing_congestion_cost_widget, "toggle_routing_congestion_cost");

    //attach to the grid
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_routing_congestion_cost_label, label_left_start_col, button_row++, box_width, box_height);
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_routing_congestion_cost_widget, box_left_start_col, button_row++, box_width, box_height);

    //show newly added contents
    gtk_widget_show_all((GtkWidget*)main_window);

    //connect signals
    g_signal_connect_swapped(GTK_COMBO_BOX_TEXT(toggle_routing_congestion_cost_widget),
                             "changed",
                             G_CALLBACK(toggle_routing_congestion_cost),
                             toggle_routing_congestion_cost_widget);
}

void button_for_toggle_routing_bounding_box() {
    GObject* main_window = application.get_object(application.get_main_window_id().c_str());
    GObject* main_window_grid = application.get_object("InnerGrid");
    auto& route_ctx = g_vpr_ctx.routing();

    GtkWidget* toggle_routing_bounding_box_widget = gtk_spin_button_new_with_range(-1., (route_ctx.route_bb.size() - 1), 1.);
    GtkWidget* toggle_routing_bounding_box_label = gtk_label_new("Toggle Routing Bounding Box:");
    gtk_widget_set_name(toggle_routing_bounding_box_widget, "toggle_routing_bounding_box");

    //attach to the grid
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_routing_bounding_box_label, label_left_start_col, button_row++, box_width, box_height);
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_routing_bounding_box_widget, box_left_start_col, button_row++, box_width, box_height);

    //show newly added contents
    gtk_widget_show_all((GtkWidget*)main_window);

    //connect signals
    g_signal_connect_swapped((GtkSpinButton*)toggle_routing_bounding_box_widget,
                             "value_changed",
                             G_CALLBACK(toggle_routing_bounding_box),
                             toggle_routing_bounding_box_widget);
}

void button_for_toggle_routing_util() {
    GObject* main_window = application.get_object(application.get_main_window_id().c_str());
    GObject* main_window_grid = application.get_object("InnerGrid");

    GtkWidget* toggle_routing_util_widget = gtk_combo_box_text_new();
    GtkWidget* toggle_routing_util_label = gtk_label_new("Toggle Routing Util:");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_routing_util_widget), "None");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_routing_util_widget), "Routing Util");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_routing_util_widget), "Routing Util with Value");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_routing_util_widget), "Routing Util with Formula");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_routing_util_widget), "Routing Util over Blocks");
    gtk_combo_box_set_active((GtkComboBox*)toggle_routing_util_widget, 0); // default set to None which has an index 0
    gtk_widget_set_name(toggle_routing_util_widget, "toggle_routing_util");

    //attach to the grid
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_routing_util_label, label_left_start_col, button_row++, box_width, box_height);
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_routing_util_widget, box_left_start_col, button_row++, box_width, box_height);

    //show newly added contents
    gtk_widget_show_all((GtkWidget*)main_window);

    //connect signals
    g_signal_connect_swapped(GTK_COMBO_BOX_TEXT(toggle_routing_util_widget),
                             "changed",
                             G_CALLBACK(toggle_routing_util),
                             toggle_routing_util_widget);
}

void button_for_toggle_router_rr_costs() {
    GObject* main_window = application.get_object(application.get_main_window_id().c_str());
    GObject* main_window_grid = application.get_object("InnerGrid");

    GtkWidget* toggle_router_rr_costs_widget = gtk_combo_box_text_new();
    GtkWidget* toggle_router_rr_costs_label = gtk_label_new("Toggle Router RR Costs:");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_router_rr_costs_widget), "None");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_router_rr_costs_widget), "Total");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_router_rr_costs_widget), "Known");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_router_rr_costs_widget), "Expected");
    gtk_combo_box_set_active((GtkComboBox*)toggle_router_rr_costs_widget, 0); // default set to None which has an index 0
    gtk_widget_set_name(toggle_router_rr_costs_widget, "toggle_router_rr_costs");

    //attach to the grid
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_router_rr_costs_label, label_left_start_col, button_row++, box_width, box_height);
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_router_rr_costs_widget, box_left_start_col, button_row++, box_width, box_height);

    //show newly added contents
    gtk_widget_show_all((GtkWidget*)main_window);

    //connect signals
    g_signal_connect_swapped(GTK_COMBO_BOX_TEXT(toggle_router_rr_costs_widget),
                             "changed",
                             G_CALLBACK(toggle_router_rr_costs),
                             toggle_router_rr_costs_widget);
}

void delete_button(const char* button_name) {
    GObject* main_window_grid = application.get_object("InnerGrid");
    GList* list_of_widgets = gtk_container_get_children(GTK_CONTAINER(main_window_grid));
    GtkWidget* target_button = NULL;

    // loop through the list to find the button
    GList* current = list_of_widgets;
    while (current != NULL) {
        GList* next = current->next;
        if (strcmp(gtk_widget_get_name(static_cast<GtkWidget*>(current->data)), button_name) == 0) {
            // found text entry
            target_button = static_cast<GtkWidget*>(current->data);
            break;
        }
        current = next;
    }

    //free the list and destroy the button
    g_list_free(list_of_widgets);
    gtk_widget_destroy(target_button);
}

GtkWidget* find_button(const char* button_name) {
    GObject* main_window_grid = application.get_object("InnerGrid");
    GList* list_of_widgets = gtk_container_get_children(GTK_CONTAINER(main_window_grid));
    GtkWidget* target_button = NULL;

    // loop through the list to find the button
    GList* current = list_of_widgets;
    while (current != NULL) {
        GList* next = current->next;
        if (strcmp(gtk_widget_get_name(static_cast<GtkWidget*>(current->data)), button_name) == 0) {
            target_button = static_cast<GtkWidget*>(current->data);
            break;
        }
        current = next;
    }

    //free the list and destroy the button
    g_list_free(list_of_widgets);
    return target_button;
}

#endif /* NO_GRAPHICS */
