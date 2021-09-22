/*
 * Copyright 2019 University of Toronto
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Authors: Mario Badr, Sameh Attia and Tanner Young-Schultz
 */

#include "ezgl/callback.hpp"

namespace ezgl {

/**
 * Provides file wide variables to support mouse panning
 */
struct mouse_pan {
  /**
   * Tracks whether the mouse button used for panning is currently pressed
   */
  bool panning_mouse_button_pressed = false;
  /**
   * Holds the timestamp of the last panning event
   */
  int last_panning_event_time = 0;
  /**
   * The old x and y positions of the mouse pointer
   */
  double prev_x = 0;
  double prev_y = 0;
} g_mouse_pan;

gboolean press_key(GtkWidget *, GdkEventKey *event, gpointer data)
{
  auto application = static_cast<ezgl::application *>(data);

  // Call the user-defined key press callback if defined
  if(application->key_press_callback != nullptr) {
    // see: https://developer.gnome.org/gdk3/stable/gdk3-Keyboard-Handling.html
    application->key_press_callback(application, event, gdk_keyval_name(event->keyval));
  }

  // Returning FALSE to indicate this event should be propagated on to other
  // gtk widgets. This is important since we're grabbing keyboard events
  // for the whole main window. It can have unexpected effects though, such
  // as Enter/Space being treated as press any highlighted button.
  // return TRUE (event consumed) if you want to avoid that, and don't have
  // any widgets that need keyboard events.
  return FALSE;
}

gboolean press_mouse(GtkWidget *, GdkEventButton *event, gpointer data)
{
  auto application = static_cast<ezgl::application *>(data);

  if(event->type == GDK_BUTTON_PRESS) {

    // Check for mouse press to support dragging
    if(event->button == PANNING_MOUSE_BUTTON) {
      g_mouse_pan.panning_mouse_button_pressed = true;
      g_mouse_pan.prev_x = event->x;
      g_mouse_pan.prev_y = event->y;
    }
    // Call the user-defined mouse press callback if defined
    // The user-defined callback is called for mouse buttons other than
    // the PANNING_MOUSE_BUTTON button. If the user pressed the PANNING_MOUSE_BUTTON button,
    // the callback will be called at mouse release only if no panning occurs
    else if(application->mouse_press_callback != nullptr) {
      ezgl::point2d const widget_coordinates(event->x, event->y);

      std::string main_canvas_id = application->get_main_canvas_id();
      ezgl::canvas *canvas = application->get_canvas(main_canvas_id);

      ezgl::point2d const world = canvas->get_camera().widget_to_world(widget_coordinates);
      application->mouse_press_callback(application, event, world.x, world.y);
    }
  }

  return TRUE; // consume the event
}

gboolean release_mouse(GtkWidget *, GdkEventButton *event, gpointer data)
{
  auto application = static_cast<ezgl::application *>(data);

  if(event->type == GDK_BUTTON_RELEASE) {
    // Check for mouse release to support dragging
    if(event->button == PANNING_MOUSE_BUTTON) {
      g_mouse_pan.panning_mouse_button_pressed = false;

      // Call the user-defined mouse press callback for the PANNING_MOUSE_BUTTON button only if no panning occurs
      if(event->x == g_mouse_pan.prev_x && event->y == g_mouse_pan.prev_y && application->mouse_press_callback != nullptr) {
        ezgl::point2d const widget_coordinates(event->x, event->y);

        std::string main_canvas_id = application->get_main_canvas_id();
        ezgl::canvas *canvas = application->get_canvas(main_canvas_id);

        ezgl::point2d const world = canvas->get_camera().widget_to_world(widget_coordinates);
        application->mouse_press_callback(application, event, world.x, world.y);
      }
    }
  }

  return TRUE; // consume the event
}

gboolean move_mouse(GtkWidget *, GdkEventButton *event, gpointer data)
{
  auto application = static_cast<ezgl::application *>(data);

  if(event->type == GDK_MOTION_NOTIFY) {

    // Check if the mouse button is pressed to support dragging
    if(g_mouse_pan.panning_mouse_button_pressed) {
      // drop this panning event if we have just served another one
      if(gtk_get_current_event_time() - g_mouse_pan.last_panning_event_time < 100)
        return true;

      g_mouse_pan.last_panning_event_time = gtk_get_current_event_time();

      GdkEventMotion *motion_event = (GdkEventMotion *)event;

      std::string main_canvas_id = application->get_main_canvas_id();
      auto canvas = application->get_canvas(main_canvas_id);

      point2d curr_trans = canvas->get_camera().widget_to_world({motion_event->x, motion_event->y});
      point2d prev_trans = canvas->get_camera().widget_to_world({g_mouse_pan.prev_x, g_mouse_pan.prev_y});

      double dx = curr_trans.x - prev_trans.x;
      double dy = curr_trans.y - prev_trans.y;

      g_mouse_pan.prev_x = motion_event->x;
      g_mouse_pan.prev_y = motion_event->y;

      // Flip the delta x to avoid inverted dragging
      translate(canvas, -dx, -dy);
    }
    // Else call the user-defined mouse move callback if defined
    else if(application->mouse_move_callback != nullptr) {
      ezgl::point2d const widget_coordinates(event->x, event->y);

      std::string main_canvas_id = application->get_main_canvas_id();
      ezgl::canvas *canvas = application->get_canvas(main_canvas_id);

      ezgl::point2d const world = canvas->get_camera().widget_to_world(widget_coordinates);
      application->mouse_move_callback(application, event, world.x, world.y);
    }
  }

  return TRUE; // consume the event
}

gboolean scroll_mouse(GtkWidget *, GdkEvent *event, gpointer data)
{

  if(event->type == GDK_SCROLL) {
    auto application = static_cast<ezgl::application *>(data);

    std::string main_canvas_id = application->get_main_canvas_id();
    auto canvas = application->get_canvas(main_canvas_id);

    GdkEventScroll *scroll_event = (GdkEventScroll *)event;

    ezgl::point2d scroll_point(scroll_event->x, scroll_event->y);

    if(scroll_event->direction == GDK_SCROLL_UP) {
      // Zoom in at the scroll point
      ezgl::zoom_in(canvas, scroll_point, 5.0 / 3.0);
    } else if(scroll_event->direction == GDK_SCROLL_DOWN) {
      // Zoom out at the scroll point
      ezgl::zoom_out(canvas, scroll_point, 5.0 / 3.0);
    } else if(scroll_event->direction == GDK_SCROLL_SMOOTH) {
      // Doesn't seem to be happening
    } // NOTE: We ignore scroll GDK_SCROLL_LEFT and GDK_SCROLL_RIGHT
  }
  return TRUE;
}

gboolean press_zoom_fit(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  std::string main_canvas_id = application->get_main_canvas_id();
  auto canvas = application->get_canvas(main_canvas_id);

  ezgl::zoom_fit(canvas, canvas->get_camera().get_initial_world());

  return TRUE;
}

gboolean press_zoom_in(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  std::string main_canvas_id = application->get_main_canvas_id();
  auto canvas = application->get_canvas(main_canvas_id);

  ezgl::zoom_in(canvas, 5.0 / 3.0);

  return TRUE;
}

gboolean press_zoom_out(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  std::string main_canvas_id = application->get_main_canvas_id();
  auto canvas = application->get_canvas(main_canvas_id);

  ezgl::zoom_out(canvas, 5.0 / 3.0);

  return TRUE;
}

gboolean press_up(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  std::string main_canvas_id = application->get_main_canvas_id();
  auto canvas = application->get_canvas(main_canvas_id);

  ezgl::translate_up(canvas, 5.0);

  return TRUE;
}

gboolean press_down(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  std::string main_canvas_id = application->get_main_canvas_id();
  auto canvas = application->get_canvas(main_canvas_id);

  ezgl::translate_down(canvas, 5.0);

  return TRUE;
}

gboolean press_left(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  std::string main_canvas_id = application->get_main_canvas_id();
  auto canvas = application->get_canvas(main_canvas_id);

  ezgl::translate_left(canvas, 5.0);

  return TRUE;
}

gboolean press_right(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  std::string main_canvas_id = application->get_main_canvas_id();
  auto canvas = application->get_canvas(main_canvas_id);

  ezgl::translate_right(canvas, 5.0);

  return TRUE;
}

gboolean press_proceed(GtkWidget *, gpointer data)
{
  auto ezgl_app = static_cast<ezgl::application *>(data);
  ezgl_app->quit();

  return TRUE;
}
}
