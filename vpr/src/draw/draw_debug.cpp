#include "draw_debug.h"

//window that pops up when an entry is not valid
void invalid_breakpoint_entry_window(std::string error) {
    //window settings
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position((GtkWindow*)window, GTK_WIN_POS_CENTER);
    gtk_window_set_title((GtkWindow*)window, "ERROR");
    gtk_window_set_modal((GtkWindow*)window, TRUE);

    //grid settings
    GtkWidget* grid = gtk_grid_new();

    //label settings
    GtkWidget* label = gtk_label_new(error.c_str());
    gtk_widget_set_margin_left(label, 30);
    gtk_widget_set_margin_right(label, 30);
    gtk_widget_set_margin_top(label, 30);
    gtk_widget_set_margin_bottom(label, 30);
    gtk_grid_attach((GtkGrid*)grid, label, 0, 0, 1, 1);

    //button settings
    GtkWidget* button = gtk_button_new_with_label("OK");
    gtk_widget_set_margin_bottom(button, 30);
    gtk_widget_set_margin_right(button, 30);
    gtk_widget_set_margin_left(button, 30);
    gtk_grid_attach((GtkGrid*)grid, button, 0, 1, 1, 1);
    g_signal_connect(button, "clicked", G_CALLBACK(ok_close_window), window);

    gtk_container_add(GTK_CONTAINER(window), grid);
    gtk_widget_show_all(window);
}

//closes the "invalid entry" window
void ok_close_window(GtkWidget* /*widget*/, GtkWidget* window) {
    gtk_window_close((GtkWindow*)window);
}
