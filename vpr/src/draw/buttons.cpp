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

#include "draw.h"
#include "buttons.h"

#include "ezgl/application.hpp"

//location of spin buttons, combo boxes, and labels on grid
gint box_width = 1;
gint box_height = 1;
gint label_left_start_col = 0;
gint box_left_start_col = 0;
gint button_row = 2; // 2 is the row num of the window button in main.ui, add buttons starting from this row

[[deprecated("todo: move to ezgl")]]
void delete_button(const char* button_name) {
    QWidget* main_window_grid = application.get_widget("InnerGrid");
    QList<QWidget*> list_of_widgets = gtk_container_get_children(GTK_CONTAINER(main_window_grid));
    GtkWidget* target_button = nullptr;

    // loop through the list to find the button
    for (QWidget* widget : list_of_widgets) {
        if (strcmp(gtk_widget_get_name(widget), button_name) == 0) {
            // found text entry
            target_button = widget;
            break;
        }
    }

    if (target_button)
        target_button->deleteLater();
}

[[deprecated("todo: move to ezgl")]]
GtkWidget* find_button(const char* button_name) {
    QWidget* main_window_grid = application.get_widget("InnerGrid");
    QList<QWidget*> list_of_widgets = gtk_container_get_children(GTK_CONTAINER(main_window_grid));

    // loop through the list to find the button
    for (QWidget* widget : list_of_widgets) {
        if (strcmp(gtk_widget_get_name(widget), button_name) == 0) {
            return widget;
        }
    }

    return nullptr;
}

#endif /* NO_GRAPHICS */
