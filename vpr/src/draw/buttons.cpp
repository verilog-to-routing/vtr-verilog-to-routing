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
