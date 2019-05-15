#include <gtk/gtk.h>

#include <iostream>

gboolean press_key(GtkWidget *widget, GdkEventKey *event, gpointer data);
gboolean click_mouse(GtkWidget *widget, GdkEventButton *event, gpointer data);
gboolean draw_canvas(GtkWidget *widget, cairo_t *cairo, gpointer data);

int main(int argc, char **argv)
{
  GtkBuilder *builder;
  GObject *window;
  GError *error = nullptr;

  gtk_init(&argc, &argv);

  /* Construct a GtkBuilder instance and load our UI description */
  builder = gtk_builder_new();
  if(gtk_builder_add_from_file(builder, "main.ui", &error) == 0) {
    g_printerr("Error loading file: %s\n", error->message);
    g_clear_error(&error);
    return 1;
  }

  /* Connect signal handlers to the constructed widgets. */
  window = gtk_builder_get_object(builder, "MainWindow");
  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  gtk_main();

  g_object_unref(builder);

  return 0;
}

gboolean press_key(GtkWidget *, GdkEventKey *event, gpointer)
{
  // see: https://developer.gnome.org/gdk3/stable/gdk3-Keyboard-Handling.html
  std::cout << gdk_keyval_name(event->keyval) << " was pressed.\n";

  return FALSE; // propagate the event
}

gboolean click_mouse(GtkWidget *, GdkEventButton *event, gpointer)
{
  if(event->type == GDK_BUTTON_PRESS) {
    std::cout << "User clicked mouse at " << event->x << ", " << event->y << "\n";
  } else if(event->type == GDK_BUTTON_RELEASE) {
    std::cout << "User released mouse button at " << event->x << ", " << event->y << "\n";
  }

  return TRUE; // consume the event
}

gboolean draw_canvas(GtkWidget *, cairo_t *cairo, gpointer)
{
  return FALSE; // propagate event
}
