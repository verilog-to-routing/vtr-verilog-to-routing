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

#    include "ezgl/point.hpp"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"

//location of spin buttons, combo boxes, and labels on grid
gint box_width = 1;
gint box_height = 1;
gint label_left_start_col = 0;
gint box_left_start_col = 0;
gint button_row = 2; // 2 is the row num of the window button in main.ui, add buttons starting from this row

void button_for_displaying_noc() {
    GObject* main_window = application.get_object(application.get_main_window_id().c_str());
    GObject* main_window_grid = application.get_object("InnerGrid");
    t_draw_state* draw_state = get_draw_state_vars();

    // if the user did not turn on the "noc" option then we don't give the option to display the noc to the user
    if (!draw_state->show_noc_button) {
        return;
    }

    // if we are here then the user turned the "noc" option on, so create a button to allow the user to display the noc

    //combo box for toggle_noc_display
    GtkWidget* toggle_noc_display_widget = gtk_combo_box_text_new();
    GtkWidget* toggle_noc_display_label = gtk_label_new("Toggle NoC Display:");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_noc_display_widget), "None");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_noc_display_widget), "NoC Links");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(toggle_noc_display_widget), "NoC Link Usage");

    gtk_combo_box_set_active((GtkComboBox*)toggle_noc_display_widget, 0); // default set to None which has an index 0
    gtk_widget_set_name(toggle_noc_display_widget, "toggle_noc_display");

    //attach to the grid
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_noc_display_label, label_left_start_col, button_row++, box_width, box_height);
    gtk_grid_attach((GtkGrid*)main_window_grid, toggle_noc_display_widget, box_left_start_col, button_row++, box_width, box_height);

    // show the newy added check box
    gtk_widget_show_all((GtkWidget*)main_window);

    //connect signals
    g_signal_connect_swapped(GTK_COMBO_BOX_TEXT(toggle_noc_display_widget),
                             "changed",
                             G_CALLBACK(toggle_noc_display),
                             toggle_noc_display_widget);
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
